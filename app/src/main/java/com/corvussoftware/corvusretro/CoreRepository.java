// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
package com.corvussoftware.corvusretro;

import android.app.PendingIntent;
import android.content.Intent;
import android.content.pm.PackageInfo;
import android.content.pm.PackageInstaller;
import android.content.pm.PackageManager;
import android.content.pm.Signature;
import android.net.Uri;
import android.os.Build;
import android.provider.Settings;
import android.util.Log;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.URL;
import java.nio.charset.StandardCharsets;
import java.security.MessageDigest;
import java.util.Arrays;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.ExecutorService;
import java.util.zip.ZipFile;
import javax.net.ssl.HttpsURLConnection;
import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

/** Android services for individual, OS-installed feature splits. */
final class CoreRepository {
    private static final String INDEX =
            "https://raw.githubusercontent.com/CorvusSoftware/CorvusRetro/main/core-repository/cores-v2.json";
    private static final String RELEASE =
            "https://github.com/CorvusSoftware/CorvusRetro/releases/download/";
    private static final long MAX_APK = 300L * 1024 * 1024;
    private static final int MAX_INDEX = 96 * 1024;
    private static final int MAX_CORES = 32;
    private final MainActivity activity;
    private final ExecutorService executor;
    private volatile Map<String, Core> cores = new HashMap<>();

    static final class Core {
        final String id, name, version, abi, sha256, url, splitName;
        final long size, baseVersionCode;
        Core(JSONObject json) throws JSONException, IOException {
            if (json.length() != 10) throw new IOException("Champs de catalogue invalides");
            id = symbol(string(json, "id"));
            name = text(string(json, "name"));
            text(string(json, "description"));
            version = string(json, "version");
            if (!version.matches("[A-Za-z0-9.\\-]{1,64}"))
                throw new IOException("Version de cœur invalide");
            abi = string(json, "abi");
            if (!"arm64-v8a".equals(abi) && !"x86_64".equals(abi))
                throw new IOException("Architecture non reconnue");
            sha256 = string(json, "sha256");
            if (!sha256.matches("[0-9a-f]{64}")) throw new IOException("SHA-256 invalide");
            size = integer(json, "size");
            if (size <= 0 || size > MAX_APK) throw new IOException("Taille du module invalide");
            baseVersionCode = integer(json, "base_version_code");
            if (baseVersionCode <= 0 || baseVersionCode > Integer.MAX_VALUE)
                throw new IOException("Version de base invalide");
            splitName = string(json, "split_name");
            String expected = "corvus.gb".equals(id) ? "coregb"
                    : "corvus.gbc".equals(id) ? "coregbc"
                    : "corvus.gba".equals(id) ? "coregba" : "";
            if (expected.isEmpty() || !expected.equals(splitName))
                throw new IOException("Identité du module invalide");
            url = string(json, "apk_url");
            if (url.length() > 256 || !url.startsWith(RELEASE) || !url.endsWith(".apk") ||
                    url.contains("?") || url.contains("#") || url.contains("\\") ||
                    url.contains("%") || url.contains("/../") || url.contains("/./") ||
                    url.substring(RELEASE.length()).contains("//"))
                throw new IOException("Origine du module non autorisée");
            for (int i = 0; i < url.length(); i++)
                if (url.charAt(i) <= 32 || Character.isISOControl(url.charAt(i)))
                    throw new IOException("URL de module invalide");
        }
        private static String string(JSONObject json, String key) throws JSONException, IOException {
            Object value = json.get(key);
            if (!(value instanceof String)) throw new IOException("Chaîne de catalogue invalide");
            return (String) value;
        }
        private static long integer(JSONObject json, String key) throws JSONException, IOException {
            Object value = json.get(key);
            if (!(value instanceof Integer) && !(value instanceof Long))
                throw new IOException("Entier de catalogue invalide");
            return ((Number) value).longValue();
        }
        private static String symbol(String value) throws IOException {
            if (value.length() > 95 || !value.matches("[A-Za-z0-9][A-Za-z0-9._-]*"))
                throw new IOException("Identifiant invalide");
            return value;
        }
        private static String text(String value) throws IOException {
            if (value.isEmpty() || value.length() > 128) throw new IOException("Texte de catalogue invalide");
            for (int i = 0; i < value.length(); i++)
                if (Character.isISOControl(value.charAt(i)))
                    throw new IOException("Caractère de contrôle invalide");
            return value;
        }
        String libraryName() { return "libcorvus_" + id.substring("corvus.".length()) + ".so"; }
    }

    CoreRepository(MainActivity activity, ExecutorService executor) {
        this.activity = activity;
        this.executor = executor;
    }

    private static HttpsURLConnection connection(String address) throws IOException {
        HttpsURLConnection conn = (HttpsURLConnection) new URL(address).openConnection();
        conn.setConnectTimeout(10000);
        conn.setReadTimeout(30000);
        conn.setInstanceFollowRedirects(false);
        conn.setRequestProperty("User-Agent", "CorvusRetro-CoreRepository/2");
        return conn;
    }

    private static byte[] readSmall(InputStream input) throws IOException {
        java.io.ByteArrayOutputStream out = new java.io.ByteArrayOutputStream();
        byte[] buffer = new byte[4096];
        int count;
        while ((count = input.read(buffer)) != -1) {
            if (out.size() + count > MAX_INDEX) throw new IOException("Catalogue trop volumineux");
            out.write(buffer, 0, count);
        }
        return out.toByteArray();
    }

    private PackageInfo installedPackage(int flags) throws PackageManager.NameNotFoundException {
        return activity.getPackageManager().getPackageInfo(activity.getPackageName(), flags);
    }

    private static long versionCode(PackageInfo info) {
        return Build.VERSION.SDK_INT >= 28 ? info.getLongVersionCode() : info.versionCode;
    }

    static final class Catalog {
        final Map<String, Core> cores;
        final String[] records;
        final String error;
        Catalog(Map<String, Core> cores, String[] records, String error) {
            this.cores = cores; this.records = records; this.error = error;
        }
    }

    static Catalog parseCatalog(String payload, long hostVersion, String deviceAbi)
            throws JSONException, IOException {
        JSONObject json = new JSONObject(payload);
        if (json.length() != 2 || Core.integer(json, "format") != 2)
            throw new IOException("Version de catalogue incompatible");
        JSONArray array = json.getJSONArray("cores");
        if (array.length() > MAX_CORES) throw new IOException("Trop de cœurs");
        Map<String, Core> next = new HashMap<>();
        Set<String> identities = new HashSet<>();
        String[] names = new String[array.length() * 5];
        int entries = 0;
        boolean otherBase = false;
        for (int i = 0; i < array.length(); i++) {
            Core core = new Core(array.getJSONObject(i));
            if (!identities.add(core.id + "/" + core.abi + "/" + core.baseVersionCode))
                throw new IOException("Cœur dupliqué");
            if (!core.abi.equals(deviceAbi)) continue;
            if (core.baseVersionCode != hostVersion) { otherBase = true; continue; }
            next.put(core.id, core);
            names[entries * 5] = core.id;
            names[entries * 5 + 1] = core.name;
            names[entries * 5 + 2] = core.version;
            names[entries * 5 + 3] = core.id;
            names[entries * 5 + 4] = core.abi;
            entries++;
        }
        return new Catalog(next, Arrays.copyOf(names, entries * 5), entries == 0 && otherBase
                ? "Mettez à jour CorvusRetro pour installer ces cœurs" : "");
    }

    void refresh() {
        if (!activity.isAlive()) return;
        executor.execute(() -> {
            try {
                long hostVersion = versionCode(installedPackage(0));
                HttpsURLConnection conn = connection(INDEX);
                try {
                    if (conn.getResponseCode() != 200)
                        throw new IOException("Catalogue HTTP " + conn.getResponseCode());
                    byte[] bytes;
                    try (InputStream in = conn.getInputStream()) { bytes = readSmall(in); }
                    Catalog catalog = parseCatalog(new String(bytes, StandardCharsets.UTF_8),
                            hostVersion, Build.SUPPORTED_ABIS[0]);
                    cores = catalog.cores;
                    if (activity.isAlive()) MainActivity.coreCatalogUpdated(catalog.records, catalog.error);
                } finally {
                    conn.disconnect();
                }
            } catch (Exception error) {
                cores = new HashMap<>();
                Log.w("CorvusRetro", "Core catalog fetch failed (" + error.getClass().getSimpleName() + ")");
                if (activity.isAlive())
                    MainActivity.coreCatalogUpdated(new String[0], "Catalogue indisponible");
            }
        });
    }

    private static Signature[] signers(PackageInfo info) {
        return Build.VERSION.SDK_INT >= 28
                ? info.signingInfo == null ? new Signature[0] : info.signingInfo.getApkContentsSigners()
                : info.signatures == null ? new Signature[0] : info.signatures;
    }

    private static boolean sameSigners(PackageInfo one, PackageInfo two) {
        Signature[] first = signers(one), second = signers(two);
        return first.length > 0 && first.length == second.length &&
                new HashSet<>(Arrays.asList(first)).equals(new HashSet<>(Arrays.asList(second)));
    }

    private void verifyModule(Core core, File apk) throws IOException, PackageManager.NameNotFoundException {
        PackageInfo host = installedPackage(Build.VERSION.SDK_INT >= 28
                ? PackageManager.GET_SIGNING_CERTIFICATES : PackageManager.GET_SIGNATURES);
        if (versionCode(host) != core.baseVersionCode)
            throw new IOException("Mettez à jour CorvusRetro avant d'installer ce cœur");
        try (ZipFile archive = new ZipFile(apk)) {
            if (archive.getEntry("AndroidManifest.xml") == null ||
                    archive.getEntry("lib/" + core.abi + "/" + core.libraryName()) == null)
                throw new IOException("Le module ne contient pas le cœur demandé");
            for (String abi : new String[] {"arm64-v8a", "x86_64"}) {
                if (archive.getEntry("lib/" + abi + "/libcorvusretro.so") != null)
                    throw new IOException("Un module ne doit pas remplacer l'application");
                for (String id : new String[] {"gb", "gbc", "gba"}) {
                    String name = "libcorvus_" + id + ".so";
                    if (!core.libraryName().equals(name) && archive.getEntry("lib/" + abi + "/" + name) != null)
                        throw new IOException("Le module contient plusieurs cœurs");
                }
            }
        }
        PackageInfo archive = activity.getPackageManager().getPackageArchiveInfo(apk.getAbsolutePath(),
                Build.VERSION.SDK_INT >= 28 ? PackageManager.GET_SIGNING_CERTIFICATES : PackageManager.GET_SIGNATURES);
        if (archive != null && (!activity.getPackageName().equals(archive.packageName) ||
                versionCode(archive) != core.baseVersionCode || archive.splitNames == null ||
                archive.splitNames.length != 1 || !core.splitName.equals(archive.splitNames[0]) ||
                !sameSigners(host, archive)))
            throw new IOException("Identité ou signature du module incompatible");
        // Some Android parsers reject a standalone feature archive. PackageInstaller
        // verifies its package, exact base version and signer as part of the inherited session.
    }

    void install(String coreId) {
        if (!activity.isAlive()) return;
        final Core core = cores.get(coreId);
        if (core == null) {
            MainActivity.coreInstallStatus("Cœur absent du catalogue vérifié", true);
            return;
        }
        if (!activity.getPackageManager().canRequestPackageInstalls()) {
            MainActivity.coreInstallStatus("Autorisez les cœurs CorvusRetro, puis relancez le téléchargement", true);
            activity.runOnUiThread(() -> {
                if (!activity.isAlive()) return;
                Intent settings = new Intent(Settings.ACTION_MANAGE_UNKNOWN_APP_SOURCES,
                        Uri.parse("package:" + activity.getPackageName()));
                try { activity.startActivity(settings); }
                catch (RuntimeException error) {
                    MainActivity.coreInstallStatus("Autorisation d'installation indisponible", true);
                }
            });
            return;
        }
        final String operation = CoreInstallState.begin(activity);
        if (operation == null) return;
        try { executor.execute(() -> {
            File apk = null;
            boolean committed = false;
            String failure = "Installation arrêtée";
            try {
                if (!activity.isAlive()) return;
                CoreInstallState.progress(activity, operation, "Téléchargement et vérification du cœur...");
                apk = File.createTempFile("corvus-core-", ".apk", activity.getCacheDir());
                MessageDigest digest = MessageDigest.getInstance("SHA-256");
                long total = 0;
                String address = core.url;
                boolean downloaded = false;
                for (int attempt = 0; attempt < 6; attempt++) {
                    HttpsURLConnection conn = connection(address);
                    try {
                        int status = conn.getResponseCode();
                        if (status == 301 || status == 302 || status == 303 || status == 307 || status == 308) {
                            String dest = conn.getHeaderField("Location");
                            if (dest == null) throw new IOException("Redirection absente");
                            URL next = new URL(new URL(address), dest);
                            if (!"https".equals(next.getProtocol()) || next.getUserInfo() != null ||
                                    next.getPort() != -1 ||
                                    !("github.com".equals(next.getHost()) ||
                                      "release-assets.githubusercontent.com".equals(next.getHost()) ||
                                      "objects.githubusercontent.com".equals(next.getHost())))
                                throw new IOException("Redirection du module non fiable");
                            address = next.toExternalForm();
                            continue;
                        }
                        if (status != 200) throw new IOException("Téléchargement HTTP " + status);
                        try (InputStream in = conn.getInputStream();
                             FileOutputStream out = new FileOutputStream(apk)) {
                            byte[] buffer = new byte[65536];
                            int count;
                            while ((count = in.read(buffer)) != -1) {
                                total += count;
                                if (total > MAX_APK || total > core.size)
                                    throw new IOException("Dépassement de la taille annoncée");
                                digest.update(buffer, 0, count);
                                out.write(buffer, 0, count);
                            }
                            out.getFD().sync();
                        }
                        downloaded = true;
                        break;
                    } finally { conn.disconnect(); }
                }
                if (!downloaded || total != core.size) throw new IOException("Téléchargement incomplet");
                StringBuilder sum = new StringBuilder(64);
                for (byte b : digest.digest())
                    sum.append(String.format(java.util.Locale.ROOT, "%02x", b & 255));
                if (!core.sha256.equals(sum.toString())) throw new IOException("Empreinte SHA-256 incorrecte");
                verifyModule(core, apk);
                if (!activity.isAlive()) return;

                PackageInstaller installer = activity.getPackageManager().getPackageInstaller();
                PackageInstaller.SessionParams params = new PackageInstaller.SessionParams(
                        PackageInstaller.SessionParams.MODE_INHERIT_EXISTING);
                params.setAppPackageName(activity.getPackageName());
                params.setSize(core.size);
                if (Build.VERSION.SDK_INT >= 31)
                    params.setRequireUserAction(PackageInstaller.SessionParams.USER_ACTION_REQUIRED);
                int sessionId = installer.createSession(params);
                try (PackageInstaller.Session session = installer.openSession(sessionId)) {
                    try (FileInputStream input = new FileInputStream(apk);
                         OutputStream output = session.openWrite(core.splitName + ".apk", 0, core.size)) {
                        byte[] buffer = new byte[65536];
                        int count;
                        while ((count = input.read(buffer)) != -1) output.write(buffer, 0, count);
                        session.fsync(output);
                    }
                    Intent result = new Intent(activity, CoreInstallReceiver.class);
                    result.setAction(CoreInstallReceiver.ACTION);
                    result.putExtra("core", core.id);
                    result.putExtra("operation", operation);
                    result.putExtra("session", sessionId);
                    PendingIntent pending = PendingIntent.getBroadcast(activity, sessionId, result,
                            PendingIntent.FLAG_UPDATE_CURRENT | PendingIntent.FLAG_MUTABLE);
                    CoreInstallState.progress(activity, operation, "Confirmez l'installation du cœur dans Android");
                    CoreInstallState.commit(activity, operation, sessionId, session, pending.getIntentSender());
                    committed = true;
                } finally {
                    if (!committed) installer.abandonSession(sessionId);
                }
            } catch (Exception error) {
                Log.w("CorvusRetro", "Core install failed (" + error.getClass().getSimpleName() + ")");
                String detail = error.getMessage();
                if (detail == null || detail.contains("/") || detail.length() > 160)
                    detail = "Vérifiez le réseau et l'espace disponible";
                failure = "Installation échouée : " + detail;
            } finally {
                CoreInstallState.downloaded(activity, operation, committed, failure);
                if (apk != null && apk.exists() && !apk.delete()) apk.deleteOnExit();
            }
        }); } catch (java.util.concurrent.RejectedExecutionException error) {
            CoreInstallState.downloaded(activity, operation, false, "Installation arrêtée");
        }
    }
}
