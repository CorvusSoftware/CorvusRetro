// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
package com.corvussoftware.corvusretro;

import android.content.Context;
import android.content.SharedPreferences;
import android.database.Cursor;
import android.net.Uri;
import android.provider.OpenableColumns;
import android.util.AtomicFile;
import org.json.JSONArray;
import org.json.JSONObject;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.security.MessageDigest;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.List;
import java.util.Locale;

/** Imported content lives in app-private storage; external filenames are display-only. */
final class GameLibrary {
    static final int MAX_ROM = 32 * 1024 * 1024;
    private static final Object INDEX_LOCK = new Object();
    final Context context;
    final SharedPreferences preferences;
    static final class Game {
        final String id;
        final String title;
        final int system;
        final long played;
        Game(String id, String title, int system, long played) {
            this.id = id; this.title = title; this.system = system; this.played = played;
        }
    }
    GameLibrary(Context context) {
        this.context = context.getApplicationContext();
        preferences = this.context.getSharedPreferences("corvus", Context.MODE_PRIVATE);
    }
    List<Game> games() {
        List<Game> result = new ArrayList<>();
        try {
            JSONArray records = new JSONArray(preferences.getString("games", "[]"));
            for (int i = 0; i < records.length(); i++) {
                JSONObject record = records.optJSONObject(i);
                if (record == null) continue;
                String id = record.optString("id");
                int system = record.optInt("system", -1);
                if (!id.matches("[0-9a-f]{64}-[012]") || system < 0 || system > 2 || !id.endsWith("-" + system)) continue;
                result.add(new Game(id, record.optString("title", id), system, record.optLong("played", 0)));
            }
        } catch (org.json.JSONException ignored) { /* An unreadable index is shown as empty. */ }
        result.sort(Comparator.comparingLong((Game game) -> game.played).reversed().thenComparing(game -> game.title));
        return result;
    }
    Game find(String id) {
        for (Game game : games()) if (game.id.equals(id)) return game;
        return null;
    }
    private File directory(String name) throws IOException {
        File folder = new File(context.getFilesDir(), name);
        if (!folder.isDirectory() && !folder.mkdirs()) throw new IOException("Dossier de stockage indisponible");
        return folder;
    }
    File rom(Game game) throws IOException { return new File(directory("roms"), game.id + ".rom"); }
    File battery(Game game) throws IOException { return new File(directory("saves"), game.id + ".sav"); }
    File state(Game game) throws IOException { return new File(directory("states"), game.id + "-" + CoreCatalog.VERSION + ".state"); }
    void record(Game game) throws IOException {
        synchronized (INDEX_LOCK) {
        List<Game> records = games();
        for (Game previous : records) {
            if (previous.id.equals(game.id) && previous.played > game.played)
                game = new Game(game.id, game.title, game.system, previous.played);
        }
        String id = game.id;
        records.removeIf(existing -> existing.id.equals(id));
        records.add(game);
        JSONArray array = new JSONArray();
        try {
            for (Game item : records) {
                JSONObject object = new JSONObject();
                object.put("id", item.id).put("title", item.title).put("system", item.system).put("played", item.played);
                array.put(object);
            }
        } catch (org.json.JSONException error) { throw new IOException(error); }
        if (!preferences.edit().putString("games", array.toString()).commit()) throw new IOException("Bibliothèque non enregistrée");
        }
    }
    void touch(Game game) throws IOException {
        synchronized (INDEX_LOCK) {
            Game current = find(game.id);
            if (current == null) current = game;
            record(new Game(current.id, current.title, current.system, System.currentTimeMillis()));
        }
    }
    Game importContent(Uri uri, int selected) throws Exception {
        String title = "Contenu";
        try (Cursor cursor = context.getContentResolver().query(uri, new String[]{OpenableColumns.DISPLAY_NAME}, null, null, null)) {
            if (cursor != null && cursor.moveToFirst() && !cursor.isNull(0)) title = cursor.getString(0);
        }
        if (title.length() > 128) title = title.substring(0, 128);
        String extension = title.toLowerCase(Locale.ROOT);
        int system = extension.endsWith(".gba") ? 2 : extension.endsWith(".gbc") ? 1
                : extension.endsWith(".gb") && selected == 2 ? 0 : CoreCatalog.valid(selected);
        File temporary = File.createTempFile("import-", ".part", directory("roms"));
        try {
            MessageDigest digest = MessageDigest.getInstance("SHA-256");
            int total = 0;
            try (InputStream input = context.getContentResolver().openInputStream(uri);
                 FileOutputStream output = new FileOutputStream(temporary)) {
                if (input == null) throw new IOException("Contenu illisible");
                byte[] buffer = new byte[65536];
                int count;
                while ((count = input.read(buffer)) != -1) {
                    total += count;
                    if (total > MAX_ROM) throw new IOException("Contenu trop volumineux (maximum 32 Mio)");
                    output.write(buffer, 0, count); digest.update(buffer, 0, count);
                }
            }
            if (total == 0) throw new IOException("Le contenu est vide");
            StringBuilder hash = new StringBuilder();
            for (byte value : digest.digest()) hash.append(String.format(Locale.ROOT, "%02x", value & 255));
            Game game = new Game(hash + "-" + system, title, system, 0);
            // Reject malformed/incompatible content before adding a playlist entry.
            try (NativeCore validation = new NativeCore(system, temporary.getAbsolutePath(), null)) {
                if (validation.width <= 0) throw new IOException("Contenu incompatible");
            }
            File destination = rom(game);
            if (!destination.exists() && !temporary.renameTo(destination)) throw new IOException("Import impossible");
            Game previous = find(game.id);
            if (previous != null) game = new Game(game.id, title, system, previous.played);
            record(game);
            return game;
        } finally {
            if (temporary.exists() && !temporary.delete()) temporary.deleteOnExit();
        }
    }
    static byte[] read(File file, int maximum) throws IOException {
        AtomicFile atomic = new AtomicFile(file);
        try (FileInputStream input = atomic.openRead()) {
            long size = input.getChannel().size();
            if (size < 0 || size > maximum) throw new IOException("Sauvegarde trop volumineuse");
            byte[] bytes = new byte[(int)size];
            int at = 0;
            while (at < bytes.length) {
                int count = input.read(bytes, at, bytes.length - at);
                if (count < 0) throw new IOException("Sauvegarde incomplète");
                at += count;
            }
            return bytes;
        } catch (FileNotFoundException error) {
            if (!file.exists() && !new File(file.getPath() + ".bak").exists()) return null;
            throw error;
        }
    }
    static void write(File file, byte[] bytes) throws IOException {
        AtomicFile atomic = new AtomicFile(file);
        FileOutputStream output = atomic.startWrite();
        try { output.write(bytes); atomic.finishWrite(output); }
        catch (IOException error) { atomic.failWrite(output); throw error; }
    }
}
