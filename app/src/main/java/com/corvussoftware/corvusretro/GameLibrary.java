// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
package com.corvussoftware.corvusretro;

import android.content.Context;
import android.content.SharedPreferences;
import android.database.Cursor;
import android.net.Uri;
import android.provider.OpenableColumns;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.security.MessageDigest;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.List;
import org.json.JSONArray;
import org.json.JSONObject;

/** Android document/storage adapter. Core validation belongs to the native frontend. */
final class GameLibrary {
    private static final int MAX_ROM = 32 * 1024 * 1024;
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
    static final class Pending {
        final Game game;
        final File temporary;
        Pending(Game game, File temporary) { this.game = game; this.temporary = temporary; }
        void discard() {
            if (temporary.exists() && !temporary.delete()) temporary.deleteOnExit();
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
    private void record(Game game) throws IOException {
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
            String oldIndex = preferences.getString("games", "[]");
            if (!preferences.edit().putString("games", array.toString()).commit()) {
                // SharedPreferences also updates its memory on disk failure. Restore that view.
                preferences.edit().putString("games", oldIndex).commit();
                throw new IOException("Bibliothèque non enregistrée");
            }
        }
    }
    void touch(String id) throws IOException {
        synchronized (INDEX_LOCK) {
            Game current = find(id);
            if (current != null) record(new Game(current.id, current.title, current.system, System.currentTimeMillis()));
        }
    }
    Pending prepare(Uri uri, int selected) throws Exception {
        String title = "Contenu";
        try (Cursor cursor = context.getContentResolver().query(uri, new String[]{OpenableColumns.DISPLAY_NAME}, null, null, null)) {
            if (cursor != null && cursor.moveToFirst() && !cursor.isNull(0)) title = cursor.getString(0);
        }
        if (title.isEmpty()) title = "Contenu";
        if (title.length() > 128) {
            int end = Character.isHighSurrogate(title.charAt(127)) ? 127 : 128;
            title = title.substring(0, end);
        }
        int system = selected >= 0 && selected < 3 ? selected : 0;
        File temporary = File.createTempFile("import-", ".part", directory("roms"));
        boolean copied = false;
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
                output.getFD().sync();
            }
            if (total == 0) throw new IOException("Le contenu est vide");
            StringBuilder hash = new StringBuilder();
            for (byte value : digest.digest()) hash.append(String.format(java.util.Locale.ROOT, "%02x", value & 255));
            Pending result = new Pending(new Game(hash + "-" + system, title, system, 0), temporary);
            copied = true;
            return result;
        } finally {
            if (!copied && temporary.exists() && !temporary.delete()) temporary.deleteOnExit();
        }
    }
    void commit(Pending pending, int system) throws IOException {
        if (system < 0 || system > 2) throw new IOException("Cœur inconnu");
        Game accepted = new Game(pending.game.id.substring(0, 64) + "-" + system, pending.game.title, system, 0);
        synchronized (INDEX_LOCK) {
            File destination = new File(directory("roms"), accepted.id + ".rom");
            boolean created = !destination.exists();
            try {
                if (created && !pending.temporary.renameTo(destination)) throw new IOException("Import impossible");
                try { record(accepted); }
                catch (IOException error) {
                    if (created && destination.exists() && !destination.delete()) destination.deleteOnExit();
                    throw error;
                }
            } finally { pending.discard(); }
        }
    }
}
