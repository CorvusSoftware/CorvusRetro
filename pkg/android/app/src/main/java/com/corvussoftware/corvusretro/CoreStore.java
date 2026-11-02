// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
package com.corvussoftware.corvusretro;

import android.content.Context;
import android.content.SharedPreferences;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.security.MessageDigest;
import java.util.ArrayList;
import java.util.LinkedHashSet;
import java.util.HashSet;
import java.util.Set;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;
import java.nio.file.Files;
import org.json.JSONArray;
import java.util.Locale;
import java.util.zip.ZipEntry;
import java.util.zip.ZipFile;
import org.json.JSONObject;

/** Verified, immutable native modules. No Android package installation is involved. */
final class CoreStore {
    private static final Object LOCK = new Object();
    private static final long MAX_LIBRARY = 64L * 1024 * 1024;
    private final File directory;
    private final SharedPreferences preferences;
    private static final int MAX_OWNED_FILES = 64;
    private final ConcurrentHashMap<String, String> removable = new ConcurrentHashMap<>();

    CoreStore(Context context) {
        this(new File(context.getFilesDir(), "cores"),
                context.getSharedPreferences("native_cores", Context.MODE_PRIVATE));
    }

    CoreStore(File directory, SharedPreferences preferences) {
        this.directory = directory;
        this.preferences = preferences;
        // Complete the initial preference load before native menu rendering begins.
        preferences.getAll();
    }

    private static boolean validId(String id) {
        return id != null && id.length() <= 95 &&
                id.matches("[a-z][a-z0-9_-]{0,39}(\\.[a-z0-9_-]{1,39})+");
    }
    private static String library(String id, JSONObject record) throws Exception {
        String library = record.optString("library_name", "");
        if (library.isEmpty() && id.matches("corvus\\.(gb|gbc|gba)"))
            library = "libcorvus_" + id.substring("corvus.".length()) + ".so";
        if (!library.matches("lib[a-z0-9_]{1,60}\\.so")) throw new IOException("Métadonnées de cœur invalides");
        return library;
    }
    private static JSONObject record(String id, String raw) throws Exception {
        if (!validId(id) || raw == null || raw.length() > 16384)
            throw new IOException("Cœur téléchargé introuvable");
        JSONObject record = new JSONObject(raw);
        String library = library(id, record), digest = record.getString("sha256");
        if (!digest.matches("[0-9a-f]{64}") || !record.getString("file").equals(
                library.substring(0, library.length() - 3) + "-" + digest + ".so"))
            throw new IOException("Métadonnées de cœur invalides");
        return record;
    }
    private static boolean moduleName(String name) {
        return name != null && name.matches("lib[a-z0-9_]{1,60}-[0-9a-f]{64}\\.so");
    }
    private static Set<String> owned(JSONObject record) throws Exception {
        Set<String> files = new LinkedHashSet<>();
        files.add(record.getString("file"));
        JSONArray history = record.optJSONArray("files");
        if (record.has("files") && history == null) throw new IOException("Historique de cœur invalide");
        if (history != null) {
            if (history.length() > MAX_OWNED_FILES) throw new IOException("Historique de cœur trop long");
            for (int i = 0; i < history.length(); ++i) files.add(history.getString(i));
        }
        for (String name : files)
            if (!moduleName(name)) throw new IOException("Nom de module invalide");
        return files;
    }
    boolean canRemove(String id) {
        if (!validId(id)) return false;
        try {
            String raw = preferences.getString(id, null);
            if (raw == null) { removable.remove(id); return false; }
            if (raw.equals(removable.get(id))) return true;
            owned(record(id, raw));
            removable.put(id, raw);
            return true;
        } catch (Exception invalid) { removable.remove(id); return false; }
    }
    @SuppressWarnings("unchecked")
    private void restoreRecord(String id, Object old) {
        SharedPreferences.Editor editor = preferences.edit();
        if (old instanceof String) editor.putString(id, (String) old);
        else if (old instanceof Integer) editor.putInt(id, (Integer) old);
        else if (old instanceof Long) editor.putLong(id, (Long) old);
        else if (old instanceof Float) editor.putFloat(id, (Float) old);
        else if (old instanceof Boolean) editor.putBoolean(id, (Boolean) old);
        else if (old instanceof Set) editor.putStringSet(id, (Set<String>) old);
        else editor.remove(id);
        editor.commit();
    }
    boolean remove(String id) throws Exception {
        synchronized (LOCK) {
            String raw = validId(id) ? preferences.getString(id, null) : null;
            Set<String> files = owned(record(id, raw));
            if (Files.isSymbolicLink(directory.toPath())) throw new IOException("Dossier des cœurs invalide");
            Set<String> protectedFiles = new HashSet<>();
            for (Map.Entry<String, ?> entry : preferences.getAll().entrySet()) {
                if (entry.getKey().equals(id) || !(entry.getValue() instanceof String)) continue;
                try {
                    JSONObject other = new JSONObject((String) entry.getValue());
                    if (moduleName(other.optString("file", ""))) protectedFiles.add(other.getString("file"));
                    JSONArray history = other.optJSONArray("files");
                    if (history != null && history.length() <= MAX_OWNED_FILES)
                        for (int i = 0; i < history.length(); ++i)
                            if (moduleName(history.optString(i, ""))) protectedFiles.add(history.getString(i));
                } catch (Exception invalid) { /* Untrusted metadata is never a path to delete. */ }
            }
            if (!preferences.edit().remove(id).commit()) {
                restoreRecord(id, raw);
                throw new IOException("Enregistrement de la suppression impossible");
            }
            removable.remove(id);
            boolean complete = true;
            File parent = directory.getCanonicalFile();
            for (String name : files) {
                if (protectedFiles.contains(name)) continue;
                File file = new File(directory, name);
                if (Files.isSymbolicLink(file.toPath()) ||
                        !file.getCanonicalFile().equals(new File(parent, name))) { complete = false; continue; }
                if (file.exists() && (!file.isFile() || !file.delete())) complete = false;
            }
            return complete;
        }
    }

    static String hash(File file) throws Exception {
        MessageDigest digest = MessageDigest.getInstance("SHA-256");
        try (InputStream input = new FileInputStream(file)) {
            byte[] bytes = new byte[65536];
            int count;
            while ((count = input.read(bytes)) != -1) digest.update(bytes, 0, count);
        }
        StringBuilder result = new StringBuilder(64);
        for (byte value : digest.digest()) result.append(String.format(Locale.ROOT, "%02x", value & 255));
        return result.toString();
    }

    String[] modules() {
        synchronized (LOCK) {
            ArrayList<String> paths = new ArrayList<>();
            java.util.Map<String, ?> all = preferences.getAll();
            ArrayList<String> ids = new ArrayList<>();
            for (String legacy : new String[] {"corvus.gb", "corvus.gbc", "corvus.gba"})
                if (all.containsKey(legacy)) ids.add(legacy);
            java.util.TreeSet<String> others = new java.util.TreeSet<>(all.keySet());
            others.removeAll(ids);
            ids.addAll(others);
            for (String id : ids) {
                if (!id.matches("[a-z][a-z0-9_-]{0,39}(\\.[a-z0-9_-]{1,39})+") ||
                        paths.size() >= 32) continue;
                try {
                    Object value = all.get(id);
                    if (!(value instanceof String)) continue;
                    JSONObject record = new JSONObject((String) value);
                    String digest = record.getString("sha256");
                    String name = record.getString("file");
                    String library = record.optString("library_name", "");
                    if (library.isEmpty() && id.matches("corvus\\.(gb|gbc|gba)"))
                        library = "libcorvus_" + id.substring("corvus.".length()) + ".so";
                    if (!library.matches("lib[a-z0-9_]{1,60}\\.so") ||
                            !digest.matches("[0-9a-f]{64}") ||
                            !name.equals(library.substring(0, library.length() - 3) +
                                    "-" + digest + ".so")) continue;
                    File file = new File(directory, name);
                    if (!directory.getCanonicalFile().equals(file.getCanonicalFile().getParentFile()) ||
                            !file.isFile() || file.canWrite() || !digest.equals(hash(file))) continue;
                    paths.add(file.getAbsolutePath());
                } catch (Exception invalid) {
                    // Invalid metadata never becomes an executable module.
                }
            }
            return paths.toArray(new String[0]);
        }
    }

    void install(CoreRepository.Core core, File archive) throws Exception {
        synchronized (LOCK) {
            if (archive.length() != core.size || !core.sha256.equals(hash(archive)))
                throw new IOException("Empreinte de l'archive incorrecte");
            if (!directory.isDirectory() && !directory.mkdirs())
                throw new IOException("Impossible de créer le dossier des cœurs");
            File staged = File.createTempFile("module-", ".part", directory);
            try {
                try (ZipFile zip = new ZipFile(archive)) {
                    if (zip.size() != 1) throw new IOException("Une archive doit contenir un seul cœur");
                    ZipEntry entry = zip.entries().nextElement();
                    if (!entry.getName().equals(core.libraryName()) || entry.isDirectory() ||
                            entry.getSize() <= 0 || entry.getSize() > MAX_LIBRARY)
                        throw new IOException("Contenu d'archive incompatible");
                    try (InputStream input = zip.getInputStream(entry);
                         FileOutputStream output = new FileOutputStream(staged)) {
                        // Keep the open writer, but make the file read-only before filling it.
                        if (!staged.setReadOnly()) throw new IOException("Protection du cœur impossible");
                        byte[] bytes = new byte[65536];
                        long size = 0;
                        int count;
                        while ((count = input.read(bytes)) != -1) {
                            size += count;
                            if (size > MAX_LIBRARY || size > entry.getSize())
                                throw new IOException("Cœur trop volumineux");
                            output.write(bytes, 0, count);
                        }
                        output.getFD().sync();
                        if (size != entry.getSize()) throw new IOException("Cœur incomplet");
                    }
                }
                verifyElf(staged, core.abi);
                String digest = hash(staged);
                File destination = new File(directory, core.libraryName().replace(".so", "-" + digest + ".so"));
                Object previous = preferences.getAll().get(core.id);
                Set<String> files = new LinkedHashSet<>();
                if (previous instanceof String) {
                    try {
                        JSONObject old = record(core.id, (String) previous);
                        files.add(old.getString("file"));
                        files.addAll(owned(old));
                    } catch (Exception invalid) { /* Never infer ownership from a filename prefix. */ }
                }
                files.add(destination.getName());
                if (files.size() > MAX_OWNED_FILES)
                    throw new IOException("Supprimez le cœur avant d'installer une nouvelle version");
                // Different versions have different paths: a running core keeps its original inode.
                if (destination.exists()) {
                    if (destination.canWrite() || !digest.equals(hash(destination)))
                        throw new IOException("Module existant altéré");
                } else if (!staged.renameTo(destination)) {
                    throw new IOException("Enregistrement du cœur impossible");
                }
                JSONObject record = new JSONObject().put("file", destination.getName())
                        .put("sha256", digest).put("library_name", core.libraryName())
                        .put("files", new JSONArray(files));
                if (!preferences.edit().putString(core.id, record.toString()).commit()) {
                    restoreRecord(core.id, previous);
                    throw new IOException("Enregistrement du catalogue local impossible");
                }
                removable.put(core.id, record.toString());
            } finally {
                staged.delete();
            }
        }
    }

    private static void verifyElf(File file, String abi) throws IOException {
        byte[] header = new byte[20];
        try (FileInputStream input = new FileInputStream(file)) {
            if (input.read(header) != header.length || header[0] != 0x7f || header[1] != 'E' ||
                    header[2] != 'L' || header[3] != 'F' || header[4] != 2 || header[5] != 1 ||
                    header[16] != 3 || header[17] != 0 || header[19] != 0 ||
                    (header[18] & 255) != ("arm64-v8a".equals(abi) ? 183 : 62))
                throw new IOException("Bibliothèque native incompatible avec cet appareil");
        }
    }
}
