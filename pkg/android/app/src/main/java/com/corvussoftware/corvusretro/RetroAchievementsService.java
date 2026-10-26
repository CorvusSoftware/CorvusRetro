// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
package com.corvussoftware.corvusretro;

import android.content.Context;
import android.content.SharedPreferences;
import android.os.SystemClock;
import android.security.keystore.KeyGenParameterSpec;
import android.security.keystore.KeyProperties;
import android.util.Base64;
import java.io.ByteArrayOutputStream;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.URL;
import java.net.URLDecoder;
import java.nio.charset.StandardCharsets;
import java.security.KeyStore;
import java.security.MessageDigest;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.ArrayBlockingQueue;
import java.util.concurrent.RejectedExecutionException;
import java.util.concurrent.ThreadPoolExecutor;
import java.util.concurrent.TimeUnit;
import javax.crypto.Cipher;
import javax.crypto.KeyGenerator;
import javax.crypto.SecretKey;
import javax.crypto.spec.GCMParameterSpec;
import javax.net.ssl.HttpsURLConnection;
import org.json.JSONArray;
import org.json.JSONObject;

/** HTTPS transport and encrypted session storage. The native frontend owns the account UI. */
final class RetroAchievementsService {
    private static final String ALIAS = "CorvusRetro.RetroAchievements";
    private static final String ENDPOINT = "https://retroachievements.org/dorequest.php";
    private static final int MAX_RESPONSE = 4 * 1024 * 1024;
    private static final int MAX_AWARDS = 128;
    private static final int MAX_AWARD_POST = 8192;
    private static final int MAX_AWARDS_BYTES = 1024 * 1024;
    private static final Object KEY_LOCK = new Object();
    private static final Object FRONT_ORDER = new Object();
    private static final java.util.concurrent.ExecutorService credentialsExecutor =
            java.util.concurrent.Executors.newSingleThreadExecutor();
    private static final java.util.concurrent.ConcurrentHashMap<SharedPreferences, String[]> sessions = new java.util.concurrent.ConcurrentHashMap<>();
    private static final java.util.concurrent.ConcurrentHashMap<SharedPreferences, Outbox> outboxes = new java.util.concurrent.ConcurrentHashMap<>();
    private final SharedPreferences preferences;
    private final ThreadPoolExecutor executor = new ThreadPoolExecutor(2, 2, 0, TimeUnit.SECONDS,
            new ArrayBlockingQueue<>(16), new ThreadPoolExecutor.AbortPolicy());

    private static final class Outbox {
        final LinkedHashMap<String, String> posts = new LinkedHashMap<>();
        final Set<String> inFlight = new HashSet<>();
        final Map<String, List<Long>> waiters = new HashMap<>();
        final Map<String, Long> retryAfter = new HashMap<>();
        final java.util.concurrent.atomic.AtomicLong epoch = new java.util.concurrent.atomic.AtomicLong();
        volatile long dataEpoch;
        boolean loaded;
        volatile RetroAchievementsService owner;
    }

    private static final class Response {
        int status = -2;
        byte[] body = new byte[0];
    }

    RetroAchievementsService(Context context) {
        preferences = context.getSharedPreferences("retroachievements", Context.MODE_PRIVATE);
        // Startup precedes NativeActivity: later frontend reads only touch the immutable cache.
        String[] initial = sessions.get(preferences);
        if (initial == null) initial = readCredentials();
        Outbox fresh = new Outbox();
        fresh.epoch.set(preferences.getLong("award_epoch", 0));
        fresh.dataEpoch = fresh.epoch.get();
        synchronized (FRONT_ORDER) {
            sessions.putIfAbsent(preferences, initial);
            outboxes.putIfAbsent(preferences, fresh);
            outboxes.get(preferences).owner = this;
        }
        resumeAwards();
    }

    private SecretKey key() throws Exception {
        synchronized (KEY_LOCK) {
            KeyStore store = KeyStore.getInstance("AndroidKeyStore");
            store.load(null);
            if (store.containsAlias(ALIAS)) return (SecretKey) store.getKey(ALIAS, null);
            KeyGenerator generator = KeyGenerator.getInstance(KeyProperties.KEY_ALGORITHM_AES, "AndroidKeyStore");
            generator.init(new KeyGenParameterSpec.Builder(ALIAS,
                    KeyProperties.PURPOSE_ENCRYPT | KeyProperties.PURPOSE_DECRYPT)
                    .setBlockModes(KeyProperties.BLOCK_MODE_GCM)
                    .setEncryptionPaddings(KeyProperties.ENCRYPTION_PADDING_NONE).build());
            return generator.generateKey();
        }
    }

    private String encrypt(byte[] clear) throws Exception {
        Cipher cipher = Cipher.getInstance("AES/GCM/NoPadding");
        cipher.init(Cipher.ENCRYPT_MODE, key());
        return new JSONObject().put("iv", Base64.encodeToString(cipher.getIV(), Base64.NO_WRAP))
                .put("data", Base64.encodeToString(cipher.doFinal(clear), Base64.NO_WRAP)).toString();
    }

    private byte[] decrypt(String stored) throws Exception {
        if (stored.length() > 2 * MAX_AWARDS_BYTES) throw new java.io.IOException("storage limit");
        JSONObject object = new JSONObject(stored);
        Cipher cipher = Cipher.getInstance("AES/GCM/NoPadding");
        cipher.init(Cipher.DECRYPT_MODE, key(), new GCMParameterSpec(128,
                Base64.decode(object.getString("iv"), Base64.NO_WRAP)));
        return cipher.doFinal(Base64.decode(object.getString("data"), Base64.NO_WRAP));
    }

    private String[] readCredentials() {
        byte[] clear = null;
        try {
            String stored = preferences.getString("session", "");
            if (stored.isEmpty()) return new String[] {"", ""};
            clear = decrypt(stored);
            JSONObject session = new JSONObject(new String(clear, StandardCharsets.UTF_8));
            String user = session.getString("user"), token = session.getString("token");
            if (user.length() > 64 || token.length() > 256) return new String[] {"", ""};
            return new String[] {user, token};
        } catch (Exception unavailable) {
            // A missing/invalidated Keystore key requires a fresh login, never plaintext fallback.
            return new String[] {"", ""};
        } finally { if (clear != null) java.util.Arrays.fill(clear, (byte) 0); }
    }

    String[] credentials() {
        String[] cached = sessions.get(preferences);
        return cached == null ? new String[] {"", ""} : cached.clone();
    }

    void saveCredentials(String user, String token) {
        if (user == null || token == null || user.length() > 64 || token.length() > 256) return;
        final boolean logout = user.isEmpty() || token.isEmpty();
        // This lock orders submissions only. No crypto or preference IO is performed while held.
        synchronized (FRONT_ORDER) {
            sessions.put(preferences, new String[] {user, token});
            Outbox box = outboxes.get(preferences);
            final long epoch = logout ? box.epoch.incrementAndGet() : box.epoch.get();
            try {
                credentialsExecutor.execute(() -> {
                    if (box.epoch.get() != epoch) return;
                    if (logout) {
                        List<Long> cancelled = new ArrayList<>();
                        for (List<Long> ids : box.waiters.values()) cancelled.addAll(ids);
                        box.posts.clear(); box.inFlight.clear(); box.retryAfter.clear(); box.waiters.clear();
                        box.loaded = true; box.dataEpoch = epoch;
                        preferences.edit().remove("session").remove("awards")
                                .putLong("award_epoch", epoch).commit();
                        for (long id : cancelled) MainActivity.achievementResponse(id, -1, new byte[0]);
                        return;
                    }
                    byte[] clear = null;
                    try {
                        clear = new JSONObject().put("user", user).put("token", token).toString()
                                .getBytes(StandardCharsets.UTF_8);
                        String encrypted = encrypt(clear);
                        if (box.epoch.get() != epoch) return;
                        if (!preferences.edit().putString("session", encrypted).commit())
                            preferences.edit().remove("session").commit();
                    } catch (Exception unavailable) {
                        if (box.epoch.get() == epoch) preferences.edit().remove("session").commit();
                    } finally { if (clear != null) java.util.Arrays.fill(clear, (byte) 0); }
                });
            } catch (RejectedExecutionException stopped) {
                // This process-wide serial writer is never stopped by an Activity teardown.
            }
        }
    }

    static boolean allowed(String address) { return ENDPOINT.equals(address); }

    private static Map<String, String> parameters(String post) throws Exception {
        Map<String, String> fields = new HashMap<>();
        for (String field : post.split("&")) {
            int separator = field.indexOf('=');
            if (separator < 0) throw new java.io.IOException("invalid request");
            String key = URLDecoder.decode(field.substring(0, separator), "UTF-8");
            String value = URLDecoder.decode(field.substring(separator + 1), "UTF-8");
            if (fields.put(key, value) != null) throw new java.io.IOException("duplicate parameter");
        }
        return fields;
    }

    private static boolean awardCommand(String post) {
        try { return "awardachievement".equals(parameters(post).get("r")); }
        catch (Exception invalid) { return false; }
    }

    private static boolean award(String post) {
        if (post == null || post.length() > MAX_AWARD_POST) return false;
        try {
            Map<String, String> fields = parameters(post);
            if (!"awardachievement".equals(fields.get("r"))) return false;
            for (String key : fields.keySet())
                if (!java.util.Arrays.asList("r", "u", "t", "a", "h", "m", "o", "v").contains(key)) return false;
            return fields.getOrDefault("a", "").matches("[1-9][0-9]*") &&
                    ("0".equals(fields.get("h")) || "1".equals(fields.get("h"))) &&
                    !fields.getOrDefault("u", "").isEmpty() && !fields.getOrDefault("t", "").isEmpty();
        } catch (Exception invalid) { return false; }
    }

    private static String digest(String post) throws Exception {
        Map<String, String> fields = parameters(post);
        // The client changes elapsed time/signature on retries of the same earned achievement.
        String identity = fields.get("u") + "\0" + fields.get("a") + "\0" + fields.get("h");
        byte[] sum = MessageDigest.getInstance("SHA-256").digest(identity.getBytes(StandardCharsets.UTF_8));
        StringBuilder result = new StringBuilder(64);
        for (byte value : sum) result.append(String.format(java.util.Locale.ROOT, "%02x", value & 255));
        return result.toString();
    }

    private void loadAwards(Outbox box) {
        if (box.loaded || box.dataEpoch != box.epoch.get()) return;
        box.loaded = true;
        byte[] clear = null;
        try {
            String stored = preferences.getString("awards", "");
            if (stored.isEmpty()) return;
            clear = decrypt(stored);
            if (clear.length > MAX_AWARDS_BYTES) throw new java.io.IOException("storage limit");
            JSONArray items = new JSONArray(new String(clear, StandardCharsets.UTF_8));
            if (items.length() > MAX_AWARDS) throw new java.io.IOException("award count");
            for (int i = 0; i < items.length(); ++i) {
                String post = items.getString(i);
                if (!award(post)) throw new java.io.IOException("invalid saved request");
                box.posts.put(digest(post), post);
            }
        } catch (Exception unavailable) {
            box.posts.clear();
            preferences.edit().remove("awards").commit();
        } finally { if (clear != null) java.util.Arrays.fill(clear, (byte) 0); }
    }

    private boolean persistAwards(Outbox box) {
        if (box.dataEpoch != box.epoch.get()) return false;
        byte[] clear = null;
        try {
            if (box.posts.isEmpty()) return preferences.edit().remove("awards").commit();
            JSONArray items = new JSONArray();
            for (String post : box.posts.values()) items.put(post);
            clear = items.toString().getBytes(StandardCharsets.UTF_8);
            if (clear.length > MAX_AWARDS_BYTES) return false;
            String encrypted = encrypt(clear);
            if (box.dataEpoch != box.epoch.get()) return false;
            return preferences.edit().putString("awards", encrypted).commit();
        } catch (Exception unavailable) { return false; }
        finally { if (clear != null) java.util.Arrays.fill(clear, (byte) 0); }
    }

    /** Replays durable awards after Activity recreation or an explicit foreground resume. */
    void resumeAwards() {
        synchronized (FRONT_ORDER) {
            Outbox box = outboxes.get(preferences);
            final long epoch = box.epoch.get();
            try {
                credentialsExecutor.execute(() -> {
                    if (box.epoch.get() != epoch || box.dataEpoch != epoch) return;
                    loadAwards(box);
                    box.retryAfter.clear();
                    pumpAwards(box);
                });
            } catch (RejectedExecutionException stopped) { /* No plaintext or untracked fallback. */ }
        }
    }

    void request(long id, String address, String body) {
        if (!allowed(address) || body == null || body.length() > 65536) {
            MainActivity.achievementResponse(id, -1, new byte[0]);
            return;
        }
        if (awardCommand(body)) {
            if (!award(body)) { MainActivity.achievementResponse(id, -1, new byte[0]); return; }
            enqueueAward(id, body);
            return;
        }
        try {
            executor.execute(() -> {
                Response response = exchange(address, body, null, 0);
                MainActivity.achievementResponse(id, response.status, response.body);
                java.util.Arrays.fill(response.body, (byte) 0);
                if (response.status >= 200 && response.status < 300) pumpAfterSuccess();
            });
        } catch (RejectedExecutionException stopped) {
            MainActivity.achievementResponse(id, -2, new byte[0]);
        }
    }

    private void enqueueAward(long id, String body) {
        synchronized (FRONT_ORDER) {
            Outbox box = outboxes.get(preferences);
            final long epoch = box.epoch.get();
            try {
                credentialsExecutor.execute(() -> {
                    if (box.epoch.get() != epoch || box.dataEpoch != epoch) {
                        MainActivity.achievementResponse(id, -1, new byte[0]); return;
                    }
                    loadAwards(box);
                    String key;
                    try { key = digest(body); }
                    catch (Exception unavailable) { MainActivity.achievementResponse(id, -2, new byte[0]); return; }
                    boolean added = !box.posts.containsKey(key);
                    if (added && box.posts.size() >= MAX_AWARDS) {
                        MainActivity.achievementResponse(id, -2, new byte[0]); return;
                    }
                    box.posts.put(key, body);
                    if (!persistAwards(box)) {
                        if (added) box.posts.remove(key);
                        MainActivity.achievementResponse(id, -2, new byte[0]); return;
                    }
                    if (id != 0) box.waiters.computeIfAbsent(key, ignored -> new ArrayList<>()).add(id);
                    box.retryAfter.remove(key);
                    pumpAwards(box);
                });
            } catch (RejectedExecutionException stopped) {
                MainActivity.achievementResponse(id, -2, new byte[0]);
            }
        }
    }

    // Mutable outbox data belongs exclusively to the serial persistence worker.
    private void pumpAwards(Outbox box) {
        if (box.dataEpoch != box.epoch.get()) return;
        RetroAchievementsService owner = box.owner;
        if (owner == null || owner.executor.isShutdown()) return;
        long now = SystemClock.elapsedRealtime();
        for (Map.Entry<String, String> item : box.posts.entrySet()) {
            if (box.inFlight.size() >= 2) break;
            String key = item.getKey();
            if (box.inFlight.contains(key) || box.retryAfter.getOrDefault(key, 0L) > now) continue;
            final long epoch = box.dataEpoch;
            final String body = item.getValue();
            box.inFlight.add(key);
            try {
                owner.executor.execute(() -> owner.finishAward(key, epoch, owner.exchange(ENDPOINT, body, key, epoch)));
            } catch (RejectedExecutionException stopped) {
                box.inFlight.remove(key);
                box.retryAfter.put(key, now + 30000);
                List<Long> ids = box.waiters.remove(key);
                if (ids != null) for (long id : ids) MainActivity.achievementResponse(id, -2, new byte[0]);
            }
        }
    }

    private boolean current(String awardKey, long epoch) {
        if (awardKey == null) return true;
        Outbox box = outboxes.get(preferences);
        return box.epoch.get() == epoch && box.dataEpoch == epoch;
    }

    private Response exchange(String address, String body, String awardKey, long epoch) {
        Response response = new Response();
        HttpsURLConnection connection = null;
        byte[] post = body.getBytes(StandardCharsets.UTF_8);
        try {
            if (!current(awardKey, epoch)) { response.status = -1; return response; }
            connection = (HttpsURLConnection) new URL(address).openConnection();
            connection.setConnectTimeout(10000);
            connection.setReadTimeout(20000);
            connection.setInstanceFollowRedirects(false);
            connection.setRequestProperty("User-Agent", "CorvusRetro/0.13 (Android; RetroAchievements)");
            connection.setRequestProperty("Content-Type", "application/x-www-form-urlencoded");
            connection.setRequestMethod("POST");
            connection.setDoOutput(true);
            connection.setFixedLengthStreamingMode(post.length);
            if (!current(awardKey, epoch)) { response.status = -1; return response; }
            try (OutputStream output = connection.getOutputStream()) {
                if (!current(awardKey, epoch)) { response.status = -1; return response; }
                output.write(post);
            }
            response.status = connection.getResponseCode();
            InputStream stream = response.status >= 400 ? connection.getErrorStream() : connection.getInputStream();
            if (stream != null) {
                try (InputStream input = stream; ByteArrayOutputStream output = new ByteArrayOutputStream()) {
                    byte[] buffer = new byte[8192];
                    int count;
                    while ((count = input.read(buffer)) != -1) {
                        if (output.size() + count > MAX_RESPONSE) throw new java.io.IOException("response limit");
                        output.write(buffer, 0, count);
                    }
                    response.body = output.toByteArray();
                }
            }
        } catch (Exception unavailable) {
            response.status = -2;
            response.body = new byte[0];
        } finally {
            java.util.Arrays.fill(post, (byte) 0);
            if (connection != null) connection.disconnect();
        }
        return response;
    }

    private static boolean accepted(Response response, String post) {
        if (response.status < 200 || response.status >= 300) return false;
        try {
            JSONObject json = new JSONObject(new String(response.body, StandardCharsets.UTF_8));
            Object success = json.opt("Success");
            if (Boolean.FALSE.equals(success) && json.optString("Error", "").startsWith("User already has"))
                return true;
            if (!Boolean.TRUE.equals(success)) return false;
            if (json.has("AchievementID")) {
                long expected = Long.parseLong(parameters(post).get("a"));
                if (!(json.get("AchievementID") instanceof Number) || json.getLong("AchievementID") != expected)
                    return false;
            }
            return true;
        } catch (Exception invalid) { return false; }
    }

    private static boolean permanent(Response response) {
        return response.status >= 400 && response.status < 500 && response.status != 408 &&
                response.status != 425 && response.status != 429;
    }

    private void finishAward(String key, long epoch, Response response) {
        try {
            credentialsExecutor.execute(() -> {
                try {
                    Outbox box = outboxes.get(preferences);
                    if (box.epoch.get() != epoch || box.dataEpoch != epoch) return;
                    box.inFlight.remove(key);
                    List<Long> ids = box.waiters.remove(key);
                    boolean confirmed = accepted(response, box.posts.get(key));
                    if (confirmed || permanent(response)) {
                        box.posts.remove(key);
                        box.retryAfter.remove(key);
                        persistAwards(box);
                    } else box.retryAfter.put(key, SystemClock.elapsedRealtime() + 30000);
                    if (ids != null) {
                        for (long id : ids) MainActivity.achievementResponse(id, response.status, response.body);
                    }
                    if (confirmed) pumpAwards(box);
                } finally { java.util.Arrays.fill(response.body, (byte) 0); }
            });
        } catch (RejectedExecutionException stopped) {
            // Only process shutdown can stop the shared writer; the encrypted entry remains durable.
            java.util.Arrays.fill(response.body, (byte) 0);
        }
    }

    private void pumpAfterSuccess() {
        synchronized (FRONT_ORDER) {
            Outbox box = outboxes.get(preferences);
            final long epoch = box.epoch.get();
            try {
                credentialsExecutor.execute(() -> {
                    if (box.epoch.get() != epoch || box.dataEpoch != epoch) return;
                    loadAwards(box);
                    pumpAwards(box);
                });
            } catch (RejectedExecutionException stopped) { /* Retain the encrypted outbox for next resume. */ }
        }
    }

    void close() {
        synchronized (FRONT_ORDER) {
            Outbox box = outboxes.get(preferences);
            if (box.owner == this) box.owner = null;
        }
        executor.shutdown();
    }
}
