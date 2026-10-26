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

    CoreStore(Context context) {
        directory = new File(context.getFilesDir(), "cores");
        preferences = context.getSharedPreferences("native_cores", Context.MODE_PRIVATE);
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
                // Different versions have different paths: a running core keeps its original inode.
                if (destination.exists()) {
                    if (destination.canWrite() || !digest.equals(hash(destination)))
                        throw new IOException("Module existant altéré");
                } else if (!staged.renameTo(destination)) {
                    throw new IOException("Enregistrement du cœur impossible");
                }
                JSONObject record = new JSONObject().put("file", destination.getName())
                        .put("sha256", digest).put("library_name", core.libraryName());
                if (!preferences.edit().putString(core.id, record.toString()).commit())
                    throw new IOException("Enregistrement du catalogue local impossible");
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
