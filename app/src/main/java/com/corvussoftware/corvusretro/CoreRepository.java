// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
package com.corvussoftware.corvusretro;

import android.app.PendingIntent;
import android.content.Intent;
import android.content.pm.PackageInstaller;
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
import java.net.HttpURLConnection;
import java.net.URL;
import java.nio.charset.StandardCharsets;
import java.security.MessageDigest;
import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.ExecutorService;
import javax.net.ssl.HttpsURLConnection;
import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

/** Android-only network, verification and package installation service.
 * UI remains owned by the C11 frontend. Never dlopen downloaded .so files. */
final class CoreRepository {
    private static final String INDEX =
            "https://raw.githubusercontent.com/CorvusSoftware/CorvusRetro/main/core-repository/index.json";
    private static final String RELEASE =
            "https://github.com/CorvusSoftware/CorvusRetro/releases/download/";
    private static final long MAX_APK = 300L * 1024 * 1024;
    private static final int MAX_INDEX = 96 * 1024;
    private static final int MAX_PACKS = 32;
    private final MainActivity activity;
    private final ExecutorService executor;
    private volatile Map<String, Pack> packages = new HashMap<>();

    static final class Pack {
        final String id, name, version, coreId, abi, sha256, url;
        final long size;
        Pack(JSONObject json) throws JSONException, IOException {
            if (json.length() != 9) throw new IOException("Champs de catalogue invalides");
            id = symbol(json.getString("id"));
            name = text(json.getString("name"));
            text(json.getString("description"));
            version = text(json.getString("version"));
            coreId = symbol(json.getString("core_id"));
            abi = json.getString("abi");
            if (!"arm64-v8a".equals(abi) && !"x86_64".equals(abi))
                throw new IOException("Architecture non reconnue");
            sha256 = json.getString("sha256");
            if (!sha256.matches("[0-9a-f]{64}")) throw new IOException("SHA-256 invalide");
            size = json.getLong("size");
            if (size <= 0 || size > MAX_APK) throw new IOException("Taille du paquet invalide");
            url = json.getString("apk_url");
            if (url.length() > 256 || !url.startsWith(RELEASE) ||
                !url.endsWith(".apk") || url.contains("?") || url.contains("#") ||
                url.contains("/../") || url.contains("/./") ||
                url.substring(RELEASE.length()).contains("//"))
                throw new IOException("Origine du paquet non autorisée");
        }
        private static String symbol(String value) throws IOException {
            if (value.length() > 95 || !value.matches("[A-Za-z0-9][A-Za-z0-9._-]*"))
                throw new IOException("Identifiant invalide");
            return value;
        }
        private static String text(String value) throws IOException {
            if (value.isEmpty() || value.length() > 128) throw new IOException("Texte de catalogue invalide");
            for (int i = 0; i < value.length(); ++i)
                if (Character.isISOControl(value.charAt(i)))
                    throw new IOException("Caractère de contrôle invalide");
            return value;
        }
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
        conn.setRequestProperty("User-Agent", "CorvusRetro-CoreRepository/1");
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

    void refresh() {
        executor.execute(() -> {
            try {
                HttpsURLConnection conn = connection(INDEX);
                try {
                    if (conn.getResponseCode() != 200)
                        throw new IOException("Catalogue HTTP " + conn.getResponseCode());
                    byte[] bytes;
                    try (InputStream in = conn.getInputStream()) { bytes = readSmall(in); }
                    JSONObject json = new JSONObject(new String(bytes, StandardCharsets.UTF_8));
                    if (json.length() != 2 || json.getInt("format") != 1)
                        throw new IOException("Version de catalogue incompatible");
                    JSONArray array = json.getJSONArray("packs");
                    if (array.length() > MAX_PACKS) throw new IOException("Trop de cœurs");
                    Map<String, Pack> next = new HashMap<>();
                    String[] names = new String[array.length() * 5];
                    int entries = 0;
                    String deviceAbi = Build.SUPPORTED_ABIS[0];
                    for (int i = 0; i < array.length(); i++) {
                        Pack p = new Pack(array.getJSONObject(i));
                        if (!p.abi.equals(deviceAbi)) continue;
                        if (next.put(p.id, p) != null)
                            throw new IOException("Cœur dupliqué");
                        names[entries * 5] = p.id;
                        names[entries * 5 + 1] = p.name;
                        names[entries * 5 + 2] = p.version;
                        names[entries * 5 + 3] = p.coreId;
                        names[entries * 5 + 4] = p.abi;
                        entries++;
                    }
                    packages = next;
                    String[] result = java.util.Arrays.copyOf(names, entries * 5);
                    if (activity.isAlive()) MainActivity.coreCatalogUpdated(result, "");
                } finally {
                    conn.disconnect();
                }
            } catch (Exception error) {
                Log.w("CorvusRetro", "Core catalog fetch failed", error);
                if (activity.isAlive())
                    MainActivity.coreCatalogUpdated(new String[0], "Catalogue indisponible");
            }
        });
    }

    void install(String packId) {
        final Pack pack = packages.get(packId);
        if (pack == null) {
            MainActivity.coreInstallStatus("Paquet absent du catalogue vérifié");
            return;
        }
        if (Build.VERSION.SDK_INT >= 26 &&
            !activity.getPackageManager().canRequestPackageInstalls()) {
            MainActivity.coreInstallStatus("Autorise les APK CorvusRetro, puis relance le téléchargement");
            activity.runOnUiThread(() -> {
                if (!activity.isAlive()) return;
                Intent settings = new Intent(Settings.ACTION_MANAGE_UNKNOWN_APP_SOURCES,
                        Uri.parse("package:" + activity.getPackageName()));
                activity.startActivity(settings);
            });
            return;
        }
        executor.execute(() -> {
            File apk = null;
            try {
                MainActivity.coreInstallStatus("Téléchargement et vérification du paquet...");
                apk = File.createTempFile("corvus-core-pack-", ".apk", activity.getCacheDir());
                MessageDigest digest = MessageDigest.getInstance("SHA-256");
                long total = 0;
                String address = pack.url;
                boolean downloaded = false;
                for (int attempt = 0; attempt < 6; ++attempt) {
                    HttpsURLConnection conn = connection(address);
                    try {
                        int status = conn.getResponseCode();
                        if (status == 301 || status == 302 || status == 303 || status == 307 || status == 308) {
                            String dest = conn.getHeaderField("Location");
                            if (dest == null) throw new IOException("Redirection absente");
                            URL next = new URL(new URL(address), dest);
                            if (!"https".equals(next.getProtocol()) ||
                                !("github.com".equals(next.getHost()) ||
                                  "release-assets.githubusercontent.com".equals(next.getHost()) ||
                                  "objects.githubusercontent.com".equals(next.getHost())))
                                throw new IOException("Redirection de paquet non fiable");
                            address = next.toExternalForm();
                            continue;
                        }
                        if (status != 200) throw new IOException("Téléchargement HTTP " + status);
                        try (InputStream in = conn.getInputStream();
                             FileOutputStream out = new FileOutputStream(apk)) {
                            byte[] buffer = new byte[65536];
                            int n;
                            while ((n = in.read(buffer)) != -1) {
                                total += n;
                                if (total > MAX_APK || total > pack.size)
                                    throw new IOException("Dépassement de la taille annoncée");
                                digest.update(buffer, 0, n);
                                out.write(buffer, 0, n);
                            }
                            out.getFD().sync();
                        }
                        downloaded = true;
                        break;
                    } finally { conn.disconnect(); }
                }
                if (!downloaded || total != pack.size)
                    throw new IOException("Téléchargement incomplet");
                byte[] actual = digest.digest();
                StringBuilder sum = new StringBuilder(64);
                for (byte b : actual)
                    sum.append(String.format(java.util.Locale.ROOT, "%02x", b & 255));
                if (!pack.sha256.equals(sum.toString()))
                    throw new IOException("Empreinte SHA-256 du paquet incorrecte");

                PackageInstaller installer = activity.getPackageManager().getPackageInstaller();
                PackageInstaller.SessionParams params = new PackageInstaller.SessionParams(
                        PackageInstaller.SessionParams.MODE_FULL_INSTALL);
                params.setAppPackageName(activity.getPackageName());
                params.setSize(pack.size);
                int sessionId = installer.createSession(params);
                boolean committed = false;
                try (PackageInstaller.Session session = installer.openSession(sessionId)) {
                    try (FileInputStream input = new FileInputStream(apk);
                         OutputStream output = session.openWrite("base.apk", 0, pack.size)) {
                        byte[] buffer = new byte[65536];
                        int n;
                        while ((n = input.read(buffer)) != -1)
                            output.write(buffer, 0, n);
                        session.fsync(output);
                    }
                    Intent result = new Intent(activity, CoreInstallReceiver.class);
                    result.setAction(CoreInstallReceiver.ACTION);
                    result.putExtra("pack", pack.id);
                    PendingIntent pending = PendingIntent.getBroadcast(activity, sessionId, result,
                            PendingIntent.FLAG_UPDATE_CURRENT | PendingIntent.FLAG_MUTABLE);
                    session.commit(pending.getIntentSender());
                    committed = true;
                    MainActivity.coreInstallStatus("Installation Android : confirmation requise");
                } finally {
                    if (!committed) installer.abandonSession(sessionId);
                }
            } catch (Exception error) {
                Log.e("CorvusRetro", "Core pack install failed", error);
                if (activity.isAlive())
                    MainActivity.coreInstallStatus("Installation échouée : " +
                            (error.getMessage() == null ? "erreur" : error.getMessage()));
            } finally {
                if (apk != null && apk.exists() && !apk.delete()) apk.deleteOnExit();
            }
        });
    }
}
