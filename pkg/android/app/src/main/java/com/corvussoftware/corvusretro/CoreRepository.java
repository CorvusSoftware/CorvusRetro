// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
package com.corvussoftware.corvusretro;

import android.os.Build;
import android.util.Log;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.net.URL;
import java.nio.charset.StandardCharsets;
import java.security.MessageDigest;
import java.util.Arrays;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.atomic.AtomicBoolean;
import javax.net.ssl.HttpsURLConnection;
import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

/** HTTPS services for the integrated native core downloader. */
final class CoreRepository {
    private static final String INDEX =
            "https://raw.githubusercontent.com/CorvusSoftware/CorvusRepository/main/catalog/cores-v3.json";
    private static final String RELEASE =
            "https://github.com/CorvusSoftware/CorvusRetro/releases/download/";
    private static final long MAX_ARCHIVE = 32L * 1024 * 1024;
    private static final int MAX_INDEX = 96 * 1024;
    private static final int MAX_CORES = 32;
    private static final AtomicBoolean DOWNLOADING = new AtomicBoolean();
    private final CoreStore store;
    private final MainActivity activity;
    private final ExecutorService executor;
    private volatile Map<String, Core> cores = new HashMap<>();

    static final class Core {
        final String id, name, version, abi, sha256, url;
        final long size;
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
            if (size <= 0 || size > MAX_ARCHIVE) throw new IOException("Taille du module invalide");
            if (integer(json, "api_version") != 0)
                throw new IOException("Version CorvusAPI incompatible");
            if (!id.matches("corvus\\.(gb|gbc|gba)") ||
                    !libraryName().equals(string(json, "library_name")))
                throw new IOException("Identité du cœur invalide");
            url = string(json, "archive_url");
            if (url.length() > 256 || !url.startsWith(RELEASE) || !url.endsWith(".zip") ||
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
        store = new CoreStore(activity);
    }

    private static HttpsURLConnection connection(String address) throws IOException {
        HttpsURLConnection conn = (HttpsURLConnection) new URL(address).openConnection();
        conn.setConnectTimeout(10000);
        conn.setReadTimeout(30000);
        conn.setInstanceFollowRedirects(false);
        conn.setRequestProperty("User-Agent", "CorvusRetro-CoreRepository/3");
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

    static boolean downloading() { return DOWNLOADING.get(); }
    // Native initialization and download completion only, never the frame loop.
    String[] installedModules() { return store.modules(); }

    static final class Catalog {
        final Map<String, Core> cores;
        final String[] records;
        final String error;
        Catalog(Map<String, Core> cores, String[] records, String error) {
            this.cores = cores; this.records = records; this.error = error;
        }
    }

    static Catalog parseCatalog(String payload, String deviceAbi)
            throws JSONException, IOException {
        JSONObject json = new JSONObject(payload);
        if (json.length() != 2 || Core.integer(json, "format") != 3)
            throw new IOException("Version de catalogue incompatible");
        JSONArray array = json.getJSONArray("cores");
        if (array.length() > MAX_CORES) throw new IOException("Trop de cœurs");
        Map<String, Core> next = new HashMap<>();
        Set<String> identities = new HashSet<>();
        String[] names = new String[array.length() * 5];
        int entries = 0;
        for (int i = 0; i < array.length(); i++) {
            Core core = new Core(array.getJSONObject(i));
            if (!identities.add(core.id + "/" + core.abi))
                throw new IOException("Cœur dupliqué");
            if (!core.abi.equals(deviceAbi)) continue;
            next.put(core.id, core);
            names[entries * 5] = core.id;
            names[entries * 5 + 1] = core.name;
            names[entries * 5 + 2] = core.version;
            names[entries * 5 + 3] = core.id;
            names[entries * 5 + 4] = core.abi;
            entries++;
        }
        return new Catalog(next, Arrays.copyOf(names, entries * 5), "");
    }

    void refresh() {
        if (!activity.isAlive()) return;
        executor.execute(() -> {
            try {
                HttpsURLConnection conn = connection(INDEX);
                try {
                    if (conn.getResponseCode() != 200)
                        throw new IOException("Catalogue HTTP " + conn.getResponseCode());
                    byte[] bytes;
                    try (InputStream in = conn.getInputStream()) { bytes = readSmall(in); }
                    Catalog catalog = parseCatalog(new String(bytes, StandardCharsets.UTF_8),
                            Build.SUPPORTED_ABIS[0]);
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

    void install(String coreId) {
        if (!activity.isAlive()) return;
        final Core core = cores.get(coreId);
        if (core == null) {
            MainActivity.coreInstallStatus("Cœur absent du catalogue vérifié", true);
            return;
        }
        if (!DOWNLOADING.compareAndSet(false, true)) return;
        try { executor.execute(() -> {
            File archive = null;
            String statusMessage = "Téléchargement arrêté";
            try {
                if (!activity.isAlive()) return;
                MainActivity.coreInstallStatus("Téléchargement et vérification du cœur...", false);
                archive = File.createTempFile("corvus-core-", ".zip", activity.getCacheDir());
                MessageDigest digest = MessageDigest.getInstance("SHA-256");
                long total = 0;
                int lastProgress = -10;
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
                             FileOutputStream out = new FileOutputStream(archive)) {
                            byte[] buffer = new byte[65536];
                            int count;
                            while ((count = in.read(buffer)) != -1) {
                                total += count;
                                if (total > MAX_ARCHIVE || total > core.size)
                                    throw new IOException("Dépassement de la taille annoncée");
                                digest.update(buffer, 0, count);
                                out.write(buffer, 0, count);
                                int percent = (int) (total * 100 / core.size);
                                if (percent >= lastProgress + 10) {
                                    lastProgress = percent;
                                    MainActivity.coreInstallStatus("Téléchargement " + core.name + " : " + percent + " %", false);
                                }
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
                store.install(core, archive);
                MainActivity.coreModulesUpdated(store.modules());
                statusMessage = core.name + " installé";
            } catch (Exception error) {
                Log.w("CorvusRetro", "Core install failed (" + error.getClass().getSimpleName() + ")");
                String detail = error.getMessage();
                if (detail == null || detail.contains("/") || detail.length() > 160)
                    detail = "Vérifiez le réseau et l'espace disponible";
                statusMessage = "Téléchargement échoué : " + detail;
            } finally {
                DOWNLOADING.set(false);
                MainActivity.coreInstallStatus(statusMessage, true);
                if (archive != null) archive.delete();
            }
        }); } catch (java.util.concurrent.RejectedExecutionException error) {
            DOWNLOADING.set(false);
            MainActivity.coreInstallStatus("Téléchargement arrêté", true);
        }
    }
}
