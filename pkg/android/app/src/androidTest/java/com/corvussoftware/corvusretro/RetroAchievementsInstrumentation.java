// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
package com.corvussoftware.corvusretro;

import android.app.Activity;
import android.app.Instrumentation;
import android.content.Context;
import android.content.ContextWrapper;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Bundle;
import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.net.URL;
import java.net.URLConnection;
import java.net.URLStreamHandler;
import java.nio.charset.StandardCharsets;
import java.security.cert.Certificate;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.TimeUnit;
import javax.net.ssl.HttpsURLConnection;
import org.json.JSONObject;

/** Only synthetic account/protocol data and an original button-controlled homebrew. */
public final class RetroAchievementsInstrumentation extends Instrumentation {
    private String phase;
    private boolean landscape;
    private int overlaySystem;
    private File fixtures;
    private int checks;
    private volatile int failedAwards, awards, logins, acceptedRequests, imageRequests;
    private volatile boolean replyAlreadyAwarded;
    private volatile boolean serverUnlocked;
    private volatile boolean metadataRequested, gameImageRequested;
    private final java.util.concurrent.CountDownLatch metadataRelease = new java.util.concurrent.CountDownLatch(1);
    private final java.util.concurrent.CountDownLatch gameImageRelease = new java.util.concurrent.CountDownLatch(1);
    private File onlineFlag;
    private final java.util.Map<String, Object> displayPreferences = new java.util.HashMap<>();
    private void displayDefaults() {
        SharedPreferences preferences = getTargetContext().getSharedPreferences("corvus", Context.MODE_PRIVATE);
        java.util.Map<String, ?> current = preferences.getAll();
        String[] keys = {"notify_cards", "notify_progress", "video_integer", "video_smooth",
                "overlay_opacity", "overlay_size", "overlay_position"};
        int[] values = {1, 1, 0, 0, 100, 100, 0};
        SharedPreferences.Editor edit = preferences.edit();
        for (int i = 0; i < keys.length; ++i) {
            displayPreferences.put(keys[i], current.get(keys[i]));
            edit.putInt(keys[i], values[i]);
        }
        check(edit.commit(), "original display and notification defaults for the fixture");
    }
    private void restoreDisplayPreferences() {
        if (displayPreferences.isEmpty()) return;
        SharedPreferences.Editor edit = getTargetContext().getSharedPreferences("corvus", Context.MODE_PRIVATE).edit();
        for (java.util.Map.Entry<String, Object> entry : displayPreferences.entrySet()) {
            edit.remove(entry.getKey());
            if (entry.getValue() instanceof Integer) edit.putInt(entry.getKey(), (Integer)entry.getValue());
            else if (entry.getValue() instanceof Boolean) edit.putBoolean(entry.getKey(), (Boolean)entry.getValue());
            else if (entry.getValue() instanceof String) edit.putString(entry.getKey(), (String)entry.getValue());
        }
        check(edit.commit(), "restore only display preferences touched by the fixture");
    }
    private static final String USER = "CorvusTest", TOKEN = "SyntheticSessionToken";
    private static final String LOGIN = "{\"Success\":true,\"User\":\"CorvusTest\",\"Token\":\"SyntheticSessionToken\",\"Score\":0,\"SoftcoreScore\":0,\"Messages\":0}";
    private static final String GAME = "{\"Success\":true,\"GameId\":731,\"Title\":\"Corvus Original Test\",\"ConsoleId\":4,\"ImageIconUrl\":\"https://media.retroachievements.org/Images/1.png\",\"RichPresenceGameId\":731,\"RichPresencePatch\":\"\",\"Sets\":[{\"AchievementSetId\":731,\"GameId\":731,\"Title\":null,\"Type\":\"core\",\"ImageIconUrl\":\"https://media.retroachievements.org/Images/1.png\",\"Achievements\":[{\"ID\":9001,\"Title\":\"Corvus memory transition\",\"Description\":\"Press A in the original test game\",\"Flags\":3,\"Points\":5,\"MemAddr\":\"0xH00c000=66\",\"Author\":\"CorvusSoftware\",\"BadgeName\":\"00001\",\"Created\":1700000000,\"Modified\":1700000000}],\"Leaderboards\":[]}]}";
    private void check(boolean condition, String message) {
        if (!condition) throw new AssertionError(message);
        checks++;
    }
    private void storageDrained() throws Exception {
        java.lang.reflect.Field field = RetroAchievementsService.class.getDeclaredField("credentialsExecutor");
        field.setAccessible(true);
        ((ExecutorService) field.get(null)).submit(() -> { }).get(15, TimeUnit.SECONDS);
    }
    private Context isolated() {
        return new ContextWrapper(getTargetContext()) {
            @Override public SharedPreferences getSharedPreferences(String name, int mode) {
                return super.getSharedPreferences("qualification_" + name, mode);
            }
        };
    }
    private void frontendDoesNotWaitForStorage(RetroAchievementsService service) throws Exception {
        java.lang.reflect.Field field = RetroAchievementsService.class.getDeclaredField("KEY_LOCK");
        field.setAccessible(true);
        Object storageLock = field.get(null);
        java.util.concurrent.CountDownLatch entered = new java.util.concurrent.CountDownLatch(1);
        java.util.concurrent.CountDownLatch release = new java.util.concurrent.CountDownLatch(1);
        Thread writer = new Thread(() -> {
            synchronized (storageLock) {
                entered.countDown();
                try { release.await(5, TimeUnit.SECONDS); }
                catch (InterruptedException interrupted) { Thread.currentThread().interrupt(); }
            }
        });
        writer.start(); check(entered.await(5, TimeUnit.SECONDS), "slow storage fixture acquired its lock");
        ExecutorService frontend = java.util.concurrent.Executors.newSingleThreadExecutor();
        try {
            frontend.submit(() -> {
                service.credentials(); service.saveCredentials(USER, TOKEN);
                service.request(0, "https://retroachievements.org/dorequest.php",
                        "r=awardachievement&u=CorvusTest&t=SyntheticSessionToken&a=9001&h=0&m=aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa&v=fixture");
            }).get(500, TimeUnit.MILLISECONDS);
            check(true, "frontend credentials and award submission never wait for storage IO");
        } finally {
            release.countDown(); writer.join(5000); frontend.shutdown();
        }
        storageDrained();
    }
    private void credentials() throws Exception {
        transport();
        onlineFlag = new File(getTargetContext().getCacheDir(), "credential-storage-network-fixture");
        onlineFlag.delete();
        check(RetroAchievementsService.allowed("https://retroachievements.org/dorequest.php"), "official HTTPS endpoint");
        for (String value : new String[] {"http://retroachievements.org/dorequest.php", "https://evil.test/dorequest.php",
                "https://retroachievements.org.evil.test/dorequest.php", "https://retroachievements.org/dorequest.php?token=x",
                "https://user@retroachievements.org/dorequest.php", "https://retroachievements.org/../dorequest.php"})
            check(!RetroAchievementsService.allowed(value), "untrusted endpoint rejected");
        Context context = isolated();
        RetroAchievementsService first = new RetroAchievementsService(context);
        first.saveCredentials(USER, TOKEN); storageDrained();
        String stored = context.getSharedPreferences("retroachievements", Context.MODE_PRIVATE).getString("session", "");
        check(!stored.isEmpty() && !stored.contains(USER) && !stored.contains(TOKEN), "session is encrypted at rest");
        check(new JSONObject(stored).getString("iv").length() > 0, "authenticated encryption nonce present");
        RetroAchievementsService second = new RetroAchievementsService(context);
        check(TOKEN.equals(second.credentials()[1]), "new service sees current session");
        // Queue an old-instance write, then sign out through a replacement instance.
        java.lang.reflect.Field field = RetroAchievementsService.class.getDeclaredField("credentialsExecutor");
        field.setAccessible(true);
        ExecutorService queue = (ExecutorService) field.get(null);
        java.util.concurrent.CountDownLatch release = new java.util.concurrent.CountDownLatch(1);
        queue.submit(() -> { try { release.await(10, TimeUnit.SECONDS); } catch (InterruptedException interrupted) { Thread.currentThread().interrupt(); } });
        first.saveCredentials(USER, TOKEN);
        first.close(); second.saveCredentials("", ""); release.countDown(); storageDrained();
        check(!context.getSharedPreferences("retroachievements", Context.MODE_PRIVATE).contains("session"), "old service cannot resurrect a signed-out session");
        check(first.credentials()[1].isEmpty() && second.credentials()[1].isEmpty(), "session snapshots agree across recreation");
        frontendDoesNotWaitForStorage(second);
        second.saveCredentials("", ""); storageDrained();
        second.saveCredentials(USER, TOKEN); storageDrained(); second.close();
    }
    private void recover() throws Exception {
        RetroAchievementsService service = new RetroAchievementsService(isolated());
        check(USER.equals(service.credentials()[0]) && TOKEN.equals(service.credentials()[1]), "Keystore session restored in a fresh process");
        service.saveCredentials("", ""); storageDrained(); service.close();
    }
    private void awaitFixture(java.util.concurrent.CountDownLatch release) {
        try {
            if (!release.await(15, TimeUnit.SECONDS)) throw new AssertionError("delayed popup fixture timed out");
        } catch (InterruptedException interrupted) {
            Thread.currentThread().interrupt(); throw new AssertionError(interrupted);
        }
    }
    private void transport() {
        URL.setURLStreamHandlerFactory(protocol -> "https".equals(protocol) ? new URLStreamHandler() {
            @Override protected URLConnection openConnection(URL address) {
                return new HttpsURLConnection(address) {
                    private final ByteArrayOutputStream output = new ByteArrayOutputStream();
                    private boolean responseStarted;
                    private String response;
                    private int status;
                    private byte[] image;
                    @Override public java.io.OutputStream getOutputStream() { return output; }
                    private void respond() {
                        if (responseStarted) return;
                        responseStarted = true;
                        if (RetroAchievementsImages.allowed(address.toString())) {
                            if ("popups".equals(phase) && address.getPath().contains("/Images/")) {
                                gameImageRequested = true; awaitFixture(gameImageRelease);
                            }
                            image = fixtureImage(address.toString()); imageRequests++; status = 200; return;
                        }
                        check(RetroAchievementsService.allowed(address.toString()), "no credential redirect or foreign server");
                        String body = new String(output.toByteArray(), StandardCharsets.UTF_8);
                        status = 200;
                        if (body.contains("r=login2")) { response = LOGIN; logins++; }
                        else if (body.contains("r=achievementsets")) {
                            if ("popups".equals(phase)) { metadataRequested = true; awaitFixture(metadataRelease); }
                            response = "popups".equals(phase) ? GAME.replace("0xH00c000=66", "M:0xH00c000=66") : GAME;
                        }
                        else if (body.contains("r=startsession")) response = "{\"Success\":true,\"Unlocks\":[],\"HardcoreUnlocks\":[]}";
                        else if (body.contains("r=awardachievement")) {
                            if (!onlineFlag.exists()) {
                                failedAwards++; status = 503; response = "";
                                android.util.Log.i("CorvusRetroTest", "AWARD_DEFERRED");
                            } else {
                                acceptedRequests++;
                                if (!serverUnlocked) { serverUnlocked = true; awards++; }
                                response = "{\"Success\":true,\"AchievementID\":9001,\"Score\":0,\"SoftcoreScore\":5,\"AchievementsRemaining\":0}";
                                if (replyAlreadyAwarded) response = "{\"Success\":false,\"Error\":\"User already has this achievement unlocked\"}";
                                android.util.Log.i("CorvusRetroTest", "AWARD_SYNCHRONIZED");
                            }
                        } else if (body.contains("r=ping")) response = "{\"Success\":true}";
                        else throw new AssertionError("Unexpected test protocol request");
                    }
                    @Override public int getResponseCode() { respond(); return status; }
                    @Override public InputStream getInputStream() { respond(); return new ByteArrayInputStream(image != null ? image : response.getBytes(StandardCharsets.UTF_8)); }
                    @Override public InputStream getErrorStream() { return getInputStream(); }
                    @Override public void disconnect() { }
                    @Override public boolean usingProxy() { return false; }
                    @Override public void connect() { }
                    @Override public String getCipherSuite() { return "synthetic test transport"; }
                    @Override public Certificate[] getLocalCertificates() { return null; }
                    @Override public Certificate[] getServerCertificates() { return null; }
                };
            }
        } : null);
    }
    private void waitFor(java.util.concurrent.Callable<Boolean> condition, String what) throws Exception {
        long end = android.os.SystemClock.uptimeMillis() + 10000;
        while (!condition.call() && android.os.SystemClock.uptimeMillis() < end) android.os.SystemClock.sleep(50);
        check(condition.call(), what);
    }
    private void outbox() throws Exception {
        transport();
        Context context = isolated();
        onlineFlag = new File(context.getCacheDir(), "qualification-ra-online"); onlineFlag.delete();
        SharedPreferences preferences = context.getSharedPreferences("retroachievements", Context.MODE_PRIVATE);
        RetroAchievementsService old = new RetroAchievementsService(context);
        old.saveCredentials("", ""); storageDrained();
        String endpoint = "https://retroachievements.org/dorequest.php";
        String award = "r=awardachievement&u=CorvusTest&t=SyntheticSessionToken&a=9001&h=0&m=aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
        old.request(41, endpoint, award + "&o=1&v=first");
        waitFor(() -> failedAwards > 0, "offline award attempted after persistence"); storageDrained();
        check(preferences.contains("awards"), "offline award survives in durable storage");
        String encrypted = preferences.getString("awards", "");
        check(!encrypted.contains(TOKEN) && !encrypted.contains("awardachievement"), "pending request including its token is encrypted");
        int failures = failedAwards;
        old.request(42, endpoint, award + "&o=2&v=second");
        waitFor(() -> failedAwards > failures, "updated retry was processed"); storageDrained();
        old.close(); onlineFlag.createNewFile();
        RetroAchievementsService replacement = new RetroAchievementsService(context);
        waitFor(() -> acceptedRequests == 1 && !preferences.contains("awards"), "replacement service synchronizes a single deduplicated award");
        replyAlreadyAwarded = true;
        replacement.request(43, endpoint, award.replace("a=9001", "a=9002") + "&v=third");
        waitFor(() -> acceptedRequests == 2 && !preferences.contains("awards"), "already-credited acknowledgement clears durable request");
        replyAlreadyAwarded = false; onlineFlag.delete();
        int beforeLogout = failedAwards;
        replacement.request(44, endpoint, award.replace("a=9001", "a=9003") + "&v=fourth");
        waitFor(() -> failedAwards > beforeLogout, "logout fixture has a pending award"); storageDrained();
        replacement.saveCredentials("", ""); storageDrained(); replacement.close();
        onlineFlag.createNewFile();
        RetroAchievementsService afterLogout = new RetroAchievementsService(context);
        storageDrained(); android.os.SystemClock.sleep(300);
        check(acceptedRequests == 2 && !preferences.contains("awards") && !preferences.contains("session"),
                "logout prevents replay and resurrection of older pending data");
        afterLogout.close(); onlineFlag.delete();
    }
    private void prepareGame() throws Exception {
        Context context = getTargetContext();
        File archive = new File(fixtures, "CorvusGB-0.15.0-android-x86_64.zip");
        JSONObject core = new JSONObject().put("id", "corvus.gb").put("name", "CorvusGB").put("description", "Qualification")
                .put("version", "0.15.0").put("abi", "x86_64").put("api_version", 0).put("library_name", "libcorvus_gb.so")
                .put("size", archive.length()).put("sha256", CoreStore.hash(archive))
                .put("archive_url", "https://github.com/CorvusSoftware/CorvusRetro/releases/download/v0.13.0/" + archive.getName());
        new CoreStore(context).install(new CoreRepository.Core(core), archive);
        byte[] rom = new byte[32768];
        rom[0x100] = (byte) 0xc3; rom[0x101] = 0x50; rom[0x102] = 1;
        byte[] program = {(byte)0x3e,0x10,(byte)0xe0,0,(byte)0xf0,0,(byte)0xe6,1,0x20,(byte)0xfa,0x3e,0x42,(byte)0xea,0,(byte)0xc0,0x18,(byte)0xfe};
        if ("popups".equals(phase)) program = new byte[] {
            (byte)0x3e,0x10,(byte)0xe0,0,(byte)0xf0,0,(byte)0xe6,1,0x20,(byte)0xfa,
            0x3e,0x21,(byte)0xea,0,(byte)0xc0,(byte)0xf0,0,(byte)0xe6,1,0x28,(byte)0xfa,
            (byte)0xf0,0,(byte)0xe6,2,0x20,(byte)0xfa,0x3e,0x42,(byte)0xea,0,(byte)0xc0,0x18,(byte)0xfe};
        System.arraycopy(program, 0, rom, 0x150, program.length);
        System.arraycopy("CORVUS RA TEST".getBytes(StandardCharsets.US_ASCII), 0, rom, 0x134, 14);
        int checksum = 0;
        for (int i = 0x134; i <= 0x14c; i++) checksum = (checksum - (rom[i] & 255) - 1) & 255;
        rom[0x14d] = (byte) checksum;
        GameLibrary library = new GameLibrary(context);
        File roms = new File(context.getFilesDir(), "roms");
        check(roms.isDirectory() || roms.mkdirs(), "private ROM directory");
        File temporary = File.createTempFile("original-ra-", ".part", roms);
        try (FileOutputStream file = new FileOutputStream(temporary)) { file.write(rom); file.getFD().sync(); }
        java.security.MessageDigest digest = java.security.MessageDigest.getInstance("SHA-256");
        StringBuilder hash = new StringBuilder();
        for (byte value : digest.digest(rom)) hash.append(String.format(java.util.Locale.ROOT, "%02x", value & 255));
        GameLibrary.Game game = new GameLibrary.Game(hash + "-0", "CorvusAchievement.gb", 0, 0, "corvus.gb", "nintendo.game_boy");
        library.commit(new GameLibrary.Pending(game, temporary), 0, "corvus.gb", "nintendo.game_boy");
        library.touch(game.id);
    }
    private static final int GAME_COLOR = 0xff0bd997, PROGRESS_COLOR = 0xffe56f13, UNLOCK_COLOR = 0xff28a5ed;
    private byte[] fixtureImage(String url) {
        android.graphics.Bitmap bitmap = android.graphics.Bitmap.createBitmap(64, 64, android.graphics.Bitmap.Config.ARGB_8888);
        bitmap.eraseColor(url.contains("/Images/") ? GAME_COLOR : url.contains("_lock") ? PROGRESS_COLOR : UNLOCK_COLOR);
        android.graphics.Canvas canvas = new android.graphics.Canvas(bitmap);
        android.graphics.Paint paint = new android.graphics.Paint(); paint.setColor(0xffffffff);
        canvas.drawCircle(32, 32, 15, paint);
        ByteArrayOutputStream bytes = new ByteArrayOutputStream();
        bitmap.compress(android.graphics.Bitmap.CompressFormat.PNG, 100, bytes); bitmap.recycle();
        return bytes.toByteArray();
    }
    private boolean hasColor(android.graphics.Bitmap bitmap, int color) {
        int count = 0, width = bitmap.getWidth(), height = bitmap.getHeight();
        int[] pixels = new int[width * height];
        bitmap.getPixels(pixels, 0, width, 0, 0, width, height);
        for (int y = 0; y < height; y += 2)
            for (int x = 0; x < width; x += 2)
                if ((pixels[y * width + x] & 0xffffff) == (color & 0xffffff) && ++count >= 30) return true;
        return false;
    }
    private void capturePopup(String name, int color) throws Exception {
        // Launch artwork may use the three-second image wait before its descent.
        long deadline = android.os.SystemClock.uptimeMillis() + ("launch".equals(name) ? 4500 : 1800);
        android.graphics.Bitmap bitmap = null;
        do {
            if (bitmap != null) bitmap.recycle();
            android.os.SystemClock.sleep(100);
            bitmap = getUiAutomation().takeScreenshot();
        } while ((bitmap == null || !hasColor(bitmap, color)) && android.os.SystemClock.uptimeMillis() < deadline);
        if (("launch".equals(name) || "unlocked".equals(name)) && bitmap != null && hasColor(bitmap, color)) {
            try (FileOutputStream out = new FileOutputStream(new File(getTargetContext().getFilesDir(), "test-popup-" + name + "-entry.png"))) {
                bitmap.compress(android.graphics.Bitmap.CompressFormat.PNG, 100, out);
            }
            bitmap.recycle();
            android.os.SystemClock.sleep(750);
            bitmap = getUiAutomation().takeScreenshot();
        }
        boolean visible = bitmap != null && hasColor(bitmap, color);
        if (bitmap != null) {
            try (FileOutputStream out = new FileOutputStream(new File(getTargetContext().getFilesDir(), "test-popup-" + name + ".png"))) {
                bitmap.compress(android.graphics.Bitmap.CompressFormat.PNG, 100, out);
            } finally { bitmap.recycle(); }
        }
        check(visible, name + " draws the actual badge image (image requests=" + imageRequests + ")");
    }
    private void tap(MainActivity activity, float x, float y) throws Exception {
        waitFor(activity::hasWindowFocus, "activity owns the input window");
        int[] area = activity.safeWindowArea();
        float density = activity.getResources().getDisplayMetrics().density;
        long time = android.os.SystemClock.uptimeMillis();
        float px = area[2] + x * density, py = area[3] + y * density;
        sendPointerSync(android.view.MotionEvent.obtain(time, time, android.view.MotionEvent.ACTION_DOWN, px, py, 0));
        sendPointerSync(android.view.MotionEvent.obtain(time, time + 16, android.view.MotionEvent.ACTION_UP, px, py, 0));
        android.os.SystemClock.sleep(500);
    }
    private void tab(MainActivity activity, int index) throws Exception {
        int[] area = activity.safeWindowArea();
        float density = activity.getResources().getDisplayMetrics().density;
        tap(activity, (area[0] - area[2] - area[4]) / density * (index + .5f) / 3,
                (area[1] - area[3] - area[5]) / density - 32);
    }
    private String frontendState() throws Exception {
        Process process = Runtime.getRuntime().exec(new String[] {"logcat", "-d", "--pid=" + android.os.Process.myPid(),
                "-s", "CorvusRetro:D", "*:S"});
        String state = "";
        try (java.io.BufferedReader reader = new java.io.BufferedReader(new java.io.InputStreamReader(process.getInputStream(), StandardCharsets.UTF_8))) {
            String line;
            while ((line = reader.readLine()) != null) if (line.contains("frontend page=")) state = line;
        }
        process.waitFor();
        return state;
    }
    private void state(int page, int core, int content, int quick) throws Exception {
        waitFor(() -> { String current = frontendState(); return current.contains("frontend page=" + page + " core=" + core + " content=" + content + " quick=" + quick) && current.contains(" active=1 "); },
                "native page/core/content state " + page + "/" + core + "/" + content + "/" + quick);
    }
    private void screenshot(String name) throws Exception {
        android.graphics.Bitmap image = getUiAutomation().takeScreenshot();
        check(image != null, "navigation screenshot " + name);
        try (FileOutputStream out = new FileOutputStream(new File(getTargetContext().getFilesDir(), "test-navigation-" + name + ".png"))) {
            image.compress(android.graphics.Bitmap.CompressFormat.PNG, 100, out);
        } finally { image.recycle(); }
    }
    private void selectFixtureCore(MainActivity activity) throws Exception {
        tab(activity, 0); tap(activity, 150, 100); tap(activity, 150, 100);
        state(0, 0, 0, 0);
    }
    private void closeContent() throws Exception {
        sendKeyDownUpSync(android.view.KeyEvent.KEYCODE_BACK);
        state(8, 0, 1, 1);
        for (int i = 0; i < 2; ++i) sendKeyDownUpSync(android.view.KeyEvent.KEYCODE_DPAD_DOWN);
        sendKeyDownUpSync(android.view.KeyEvent.KEYCODE_DPAD_CENTER);
        state(0, 0, 0, 0);
    }
    private String prepareOverlayGame(int system) throws Exception {
        return prepareOverlayGame(system, false);
    }
    private String prepareOverlayGame(int system, boolean battery) throws Exception {
        Context context = getTargetContext();
        String[] names = {"CorvusGB", "CorvusGBC", "CorvusGBA"};
        String[] ids = {"corvus.gb", "corvus.gbc", "corvus.gba"};
        for (int i = 0; i < names.length; ++i) {
            File archive = new File(fixtures, names[i] + "-0.15.0-android-x86_64.zip");
            JSONObject metadata = new JSONObject().put("id", ids[i]).put("name", names[i]).put("description", "Qualification")
                    .put("version", "0.15.0").put("abi", "x86_64").put("api_version", 0)
                    .put("library_name", "libcorvus_" + ids[i].substring(7) + ".so")
                    .put("size", archive.length()).put("sha256", CoreStore.hash(archive))
                    .put("archive_url", "https://github.com/CorvusSoftware/CorvusRetro/releases/download/v0.16.0/" + archive.getName());
            new CoreStore(context).install(new CoreRepository.Core(metadata), archive);
        }
        byte[] rom = new byte[system == 2 ? 1024 : 32768];
        if (system == 2) {
            // Original ARM program paints a green Mode3 frame and writes one SRAM byte.
            int[] words = {0xe3a00301, 0xe59f1028, 0xe1c010b0, 0xe3a00406, 0xe3a01e3e, 0xe3a02c96,
                    0xe0c010b2, 0xe2522001, 0x1afffffc, 0xe3a0040e, 0xe3a0105a, 0xe5c01000, 0xeafffffe, 0x00000403};
            int branch = 0xea00003e;
            for (int b = 0; b < 4; ++b) rom[b] = (byte) (branch >>> (b * 8));
            for (int i = 0; i < words.length; ++i) for (int b = 0; b < 4; ++b) rom[0x100 + i * 4 + b] = (byte) (words[i] >>> (b * 8));
            rom[0xb2] = (byte) 0x96;
            System.arraycopy("SRAM_V".getBytes(StandardCharsets.US_ASCII), 0, rom, 0x200, 6);
        } else {
            rom[0x100] = (byte) 0xc3; rom[0x101] = 0x50; rom[0x102] = 1;
            rom[0x150] = 0x18; rom[0x151] = (byte) 0xfe;
            System.arraycopy("CORVUS OVERLAY".getBytes(StandardCharsets.US_ASCII), 0, rom, 0x134, 14);
            rom[0x143] = system == 1 ? (byte) 0x80 : 0;
            if (battery) { rom[0x147] = 3; rom[0x149] = 2; }
            int checksum = 0;
            for (int i = 0x134; i <= 0x14c; ++i) checksum = (checksum - (rom[i] & 255) - 1) & 255;
            rom[0x14d] = (byte) checksum;
        }
        File directory = new File(context.getFilesDir(), "roms");
        check(directory.isDirectory() || directory.mkdirs(), "overlay private fixture directory");
        File temporary = File.createTempFile("overlay-", ".part", directory);
        try (FileOutputStream out = new FileOutputStream(temporary)) { out.write(rom); out.getFD().sync(); }
        StringBuilder hash = new StringBuilder();
        for (byte value : java.security.MessageDigest.getInstance("SHA-256").digest(rom)) hash.append(String.format(java.util.Locale.ROOT, "%02x", value & 255));
        String[] systems = {"nintendo.game_boy", "nintendo.game_boy_color", "nintendo.game_boy_advance"};
        String id = hash + "-" + system;
        GameLibrary library = new GameLibrary(context);
        GameLibrary.Game game = new GameLibrary.Game(id, names[system] + " overlay test", system, 0, ids[system], systems[system]);
        library.commit(new GameLibrary.Pending(game, temporary), system, ids[system], systems[system]);
        library.touch(id);
        return id;
    }
    private byte[] batteryFile(File file) throws Exception {
        check(file.isFile() && file.length() == 8192, "raw game battery remains exactly 8 KiB");
        try (InputStream input = new java.io.FileInputStream(file); ByteArrayOutputStream bytes = new ByteArrayOutputStream()) {
            byte[] buffer = new byte[8192]; int size;
            while ((size = input.read(buffer)) != -1) {
                check(bytes.size() + size <= 8192, "battery read remains bounded");
                bytes.write(buffer, 0, size);
            }
            return bytes.toByteArray();
        }
    }
    private android.net.Uri saveDocument(String name) {
        return android.net.Uri.parse("content://" + SaveTransferTestProvider.AUTHORITY + "/" + name);
    }
    private byte[] exportedBattery(String name) throws Exception {
        try (InputStream input = getTargetContext().getContentResolver().openInputStream(saveDocument(name));
             ByteArrayOutputStream bytes = new ByteArrayOutputStream()) {
            check(input != null, "exported document can be read through its provider");
            byte[] buffer = new byte[8192]; int size;
            while ((size = input.read(buffer)) != -1) {
                check(bytes.size() + size <= 8192, "exported document read remains bounded");
                bytes.write(buffer, 0, size);
            }
            check(bytes.size() == 8192, "native snapshot exports raw battery bytes without a state header");
            return bytes.toByteArray();
        }
    }
    private SaveTransfers gameSaveService(MainActivity activity) throws Exception {
        java.lang.reflect.Field field = MainActivity.class.getDeclaredField("saveTransfers");
        field.setAccessible(true);
        return (SaveTransfers) field.get(activity);
    }
    private void savePicker(MainActivity activity, String action, int keySteps, String document,
                            boolean cancelled) throws Exception {
        android.content.IntentFilter filter = new android.content.IntentFilter(action);
        filter.addCategory(Intent.CATEGORY_OPENABLE); filter.addDataType("*/*");
        Intent data = document == null ? null : new Intent().setData(saveDocument(document));
        ActivityMonitor monitor = addMonitor(filter, new ActivityResult(cancelled ? Activity.RESULT_CANCELED : Activity.RESULT_OK, data), true);
        try {
            configurationRow(activity, keySteps);
            waitFor(() -> monitor.getHits() == 1, "native save action opens Android's document contract exactly once");
            if (cancelled || Intent.ACTION_CREATE_DOCUMENT.equals(action))
                waitFor(() -> !gameSaveService(activity).busy(), "native consumes the matching document completion");
        } finally { removeMonitor(monitor); }
    }
    private void saveTransfers() throws Exception {
        check(overlaySystem >= 0 && overlaySystem < 2, "save transfer fixture supports GB and GBC");
        String id = prepareOverlayGame(overlaySystem, true);
        Context context = getTargetContext();
        String prefix = "save-transfer-" + overlaySystem + (landscape ? "-landscape" : "-portrait");
        context.getSharedPreferences("corvus", Context.MODE_PRIVATE).edit().putInt("achievements_enabled", 0)
                .putString("theme", "corvus").putInt("show_input", 1).commit();
        MainActivity activity = (MainActivity) startActivitySync(new Intent(context, MainActivity.class).addFlags(Intent.FLAG_ACTIVITY_NEW_TASK));
        try {
            check(getUiAutomation().setRotation(landscape ? android.app.UiAutomation.ROTATION_FREEZE_90 : android.app.UiAutomation.ROTATION_FREEZE_0), "save transfer test rotation");
            waitFor(() -> { int[] area = activity.safeWindowArea(); return area != null && (area[0] > area[1]) == landscape; }, "save transfer safe viewport");
            state(0, -1, 0, 0);
            configurationRow(activity, 0); state(3, -1, 0, 0);
            configurationRow(activity, overlaySystem); state(0, overlaySystem, 0, 0);
            screenshot(prefix + "-main-loaded");
            configurationRow(activity, 2); state(19, overlaySystem, 0, 0);
            screenshot(prefix + "-subsystems");
            configurationKey(activity, android.view.KeyEvent.KEYCODE_BACK); state(0, overlaySystem, 0, 0);
            tab(activity, 1); configurationRow(activity, 0); state(4, overlaySystem, 0, 0);
            check(activity.games()[0].equals(id), "new original battery fixture is the first history item");
            configurationRow(activity, 0); state(8, overlaySystem, 1, 0);
            quickOpen(activity, overlaySystem); configurationRow(activity, 9); state(26, overlaySystem, 1, 0);
            screenshot(prefix + "-saves");
            savePicker(activity, Intent.ACTION_CREATE_DOCUMENT, 1, "output.sav", false);
            byte[] previous = exportedBattery("output.sav");
            savePicker(activity, Intent.ACTION_CREATE_DOCUMENT, 1, "output.srm", false);
            check(java.util.Arrays.equals(previous, exportedBattery("output.srm")), ".sav and .srm exports contain the same actual native SRAM snapshot");
            configurationKey(activity, android.view.KeyEvent.KEYCODE_DPAD_UP);
            configurationKey(activity, android.view.KeyEvent.KEYCODE_DPAD_UP);
            savePicker(activity, Intent.ACTION_OPEN_DOCUMENT, 0, null, true);
            state(26, overlaySystem, 1, 0);
            savePicker(activity, Intent.ACTION_OPEN_DOCUMENT, 0, "battery.sav", false);
            state(27, overlaySystem, 1, 0); screenshot(prefix + "-confirm");
            configurationRow(activity, 1); state(26, overlaySystem, 1, 0);
            waitFor(() -> !gameSaveService(activity).busy(), "confirmation cancellation releases only its own transfer");
            savePicker(activity, Intent.ACTION_CREATE_DOCUMENT, 1, "output.sav", false);
            check(java.util.Arrays.equals(previous, exportedBattery("output.sav")), "cancelled import leaves the running core battery unchanged");
            configurationKey(activity, android.view.KeyEvent.KEYCODE_DPAD_UP);
            savePicker(activity, Intent.ACTION_OPEN_DOCUMENT, 0, "battery.SRM", false);
            state(27, overlaySystem, 1, 0);
            configurationRow(activity, 0); state(8, overlaySystem, 1, 0);
            waitFor(() -> !gameSaveService(activity).busy(), "confirmed import releases its stage after native commit");
            byte[] imported = SaveTransferTestProvider.battery();
            check(java.util.Arrays.equals(imported, batteryFile(new File(context.getFilesDir(), "saves/" + id + ".sav"))), "confirmed SRM import commits the selected game's raw battery");
            check(java.util.Arrays.equals(previous, batteryFile(new File(context.getFilesDir(), "saves/" + id + ".sav.import-backup"))), "pre-import live SRAM remains recoverable in the safety copy");
            quickOpen(activity, overlaySystem); configurationRow(activity, 9); state(26, overlaySystem, 1, 0);
            savePicker(activity, Intent.ACTION_CREATE_DOCUMENT, 1, "output.sav", false);
            check(java.util.Arrays.equals(imported, exportedBattery("output.sav")), "the recreated core actually loaded imported bytes, not only the disk file");
            screenshot(prefix + "-imported");
            configurationKey(activity, android.view.KeyEvent.KEYCODE_BACK); state(8, overlaySystem, 1, 1);
            // Returning restores the original quick-menu selection at its battery row.
            for (int i = 0; i < 7; ++i) configurationKey(activity, android.view.KeyEvent.KEYCODE_DPAD_UP);
            configurationRow(activity, 0); state(0, overlaySystem, 0, 0);
            configurationRow(activity, 2); state(19, overlaySystem, 0, 0);
            configurationKey(activity, android.view.KeyEvent.KEYCODE_BACK); state(0, overlaySystem, 0, 0);
            configurationKey(activity, android.view.KeyEvent.KEYCODE_DPAD_UP);
            configurationKey(activity, android.view.KeyEvent.KEYCODE_DPAD_UP);
            configurationRow(activity, 0); state(0, -1, 0, 0);
            screenshot(prefix + "-main-unloaded");
            configurationRow(activity, 2); state(14, -1, 0, 0);
            screenshot(prefix + "-unloaded");
        } finally { runOnMainSync(activity::finish); }
    }
    private float[] overlayMetrics(MainActivity activity) {
        int[] area = activity.safeWindowArea();
        float density = activity.getResources().getDisplayMetrics().density;
        float w = (area[0] - area[2] - area[4]) / density, h = (area[1] - area[3] - area[5]) / density;
        float scale = Math.min(1.4f, Math.max(1, (h >= w ? w : h) / 360));
        scale = Math.min(scale, Math.min(w / 320, h / (h >= w ? 384 : 320)));
        return new float[] {w, h, scale};
    }
    private void quickOpen(MainActivity activity, int system) throws Exception {
        float[] m = overlayMetrics(activity); float w = m[0], h = m[1], s = m[2];
        float x = h >= w ? w / 2 : 128 * s;
        float y = h >= w ? h - Math.max(h / 2, 276 * s) + 36 * s : 36 * s;
        tap(activity, x, y); state(8, system, 1, 1);
    }
    private int[] settingsVideoBounds(android.graphics.Bitmap screen, int[] safe) {
        int w = screen.getWidth(), h = screen.getHeight();
        int[] pixels = new int[w * h], queue = new int[w * h];
        boolean[] marked = new boolean[w * h];
        screen.getPixels(pixels, 0, w, 0, 0, w, h);
        int[] best = {w, h, -1, -1}; int largest = 0;
        for (int y = safe[3]; y < h - safe[5]; ++y) for (int x = safe[2]; x < w - safe[4]; ++x) {
            int point = y * w + x;
            if (marked[point]) continue;
            int rgb = pixels[point];
            if (Math.max((rgb >> 16) & 255, Math.max((rgb >> 8) & 255, rgb & 255)) <= 200) continue;
            int count = 1, at = 0, left = x, right = x, top = y, bottom = y;
            queue[0] = point; marked[point] = true;
            while (at < count) {
                int current = queue[at++], px = current % w, py = current / w;
                left = Math.min(left, px); right = Math.max(right, px);
                top = Math.min(top, py); bottom = Math.max(bottom, py);
                for (int direction = 0; direction < 4; ++direction) {
                    int nx = px + (direction == 0 ? -1 : direction == 1 ? 1 : 0);
                    int ny = py + (direction == 2 ? -1 : direction == 3 ? 1 : 0);
                    if (nx < safe[2] || nx >= w - safe[4] || ny < safe[3] || ny >= h - safe[5]) continue;
                    int next = ny * w + nx; if (marked[next]) continue;
                    int color = pixels[next];
                    if (Math.max((color >> 16) & 255, Math.max((color >> 8) & 255, color & 255)) > 200) {
                        marked[next] = true; queue[count++] = next;
                    }
                }
            }
            // A real video region dominates the tiny bright intersections of utility glyphs.
            if (count > largest) { largest = count; best = new int[] {left, top, right, bottom}; }
        }
        return best;
    }
    private int[] settingsVisibleButton(android.graphics.Bitmap image, int[] safe) {
        int w = image.getWidth(), h = image.getHeight();
        boolean[] marked = new boolean[w * h]; int[] queue = new int[w * h];
        int[] best = null;
        int xStart = safe[2] + (w - safe[2] - safe[4]) * 2 / 3;
        int yStart = landscape ? safe[3] : safe[3] + (h - safe[3] - safe[5]) * 2 / 5;
        for (int y = yStart; y < h - safe[5]; ++y) for (int x = xStart; x < w - safe[4]; ++x) {
            int point = y * w + x;
            if (marked[point]) continue;
            int rgb = image.getPixel(x, y), r = (rgb >> 16) & 255, g = (rgb >> 8) & 255, b = rgb & 255;
            if (r < 20 || r > 90 || Math.abs(r - g) > 2 || Math.abs(g - b) > 2) continue;
            int count = 1, at = 0, left = x, right = x, top = y, bottom = y;
            queue[0] = point; marked[point] = true;
            while (at < count) {
                int p = queue[at++], px = p % w, py = p / w;
                left = Math.min(left, px); right = Math.max(right, px);
                top = Math.min(top, py); bottom = Math.max(bottom, py);
                for (int dy = -1; dy <= 1; ++dy) for (int dx = -1; dx <= 1; ++dx) {
                    int nx = px + dx, ny = py + dy;
                    if (nx < xStart || nx >= w - safe[4] || ny < yStart || ny >= h - safe[5]) continue;
                    int next = ny * w + nx; if (marked[next]) continue;
                    int color = image.getPixel(nx, ny), red = (color >> 16) & 255, green = (color >> 8) & 255, blue = color & 255;
                    if (red >= 20 && red <= 90 && Math.abs(red - green) <= 2 && Math.abs(green - blue) <= 2) {
                        marked[next] = true; queue[count++] = next;
                    }
                }
            }
            int width = right - left + 1, height = bottom - top + 1;
            if (width < 45 || height < 45 || width > height * 1.2f || height > width * 1.2f) continue;
            if (best == null || width > (best[2] - best[0] + 1) * 1.3f
                    || (width >= (best[2] - best[0] + 1) * .85f && left > best[0]))
                best = new int[] {left, top, right, bottom};
        }
        check(best != null, "configured A button is located from its actual visible ring, without duplicating layout calculations");
        return best;
    }
    private int settingsBrightPixels(android.graphics.Bitmap image, int[] area) {
        int count = 0;
        for (int y = area[1]; y <= area[3]; ++y) for (int x = area[0]; x <= area[2]; ++x) {
            int c = image.getPixel(x, y);
            if (((c >> 16) & 255) > 200 && ((c >> 8) & 255) > 200 && (c & 255) > 200) ++count;
        }
        return count;
    }
    private void settingsMedia() throws Exception {
        check(overlaySystem == 0, "settings media uses the original GB video fixture");
        String id = prepareOverlayGame(0);
        SharedPreferences prefs = getTargetContext().getSharedPreferences("corvus", Context.MODE_PRIVATE);
        java.util.Map<String, Object> previous = new java.util.HashMap<>();
        String[] keys = {"audio_volume", "overlay_opacity", "overlay_size", "overlay_position",
                "video_integer", "video_smooth", "show_input", "achievements_enabled"};
        int[] values = {30, 30, 110, -10, 1, 1, 1, 0};
        SharedPreferences.Editor edit = prefs.edit(); java.util.Map<String, ?> stored = prefs.getAll();
        for (int i = 0; i < keys.length; ++i) { previous.put(keys[i], stored.get(keys[i])); edit.putInt(keys[i], values[i]); }
        check(edit.commit(), "non-default media fixture persisted before native activity starts");
        MainActivity activity = (MainActivity) startActivitySync(new Intent(getTargetContext(), MainActivity.class).addFlags(Intent.FLAG_ACTIVITY_NEW_TASK));
        try {
            check(getUiAutomation().setRotation(landscape ? android.app.UiAutomation.ROTATION_FREEZE_90 : android.app.UiAutomation.ROTATION_FREEZE_0), "media settings rotation");
            waitFor(() -> { int[] safe = activity.safeWindowArea(); return safe != null && (safe[0] > safe[1]) == landscape; }, "media settings safe orientation");
            state(0, -1, 0, 0); selectFixtureCore(activity);
            tab(activity, 1); configurationRow(activity, 0); state(4, 0, 0, 0);
            check(activity.games()[0].equals(id), "original media test fixture selected from history");
            configurationRow(activity, 0); state(8, 0, 1, 0); android.os.SystemClock.sleep(700);
            android.graphics.Bitmap idle = getUiAutomation().takeScreenshot();
            check(idle != null, "actual game frame with non-default media settings");
            screenshot("settings-media-" + (landscape ? "landscape" : "portrait") + "-idle");
            int[] safe = activity.safeWindowArea(), video = settingsVideoBounds(idle, safe);
            int width = video[2] - video[0] + 1, height = video[3] - video[1] + 1;
            check(width >= 160 && height >= 144 && width % 160 == 0 && height % 144 == 0
                    && width / 160 == height / 144, "integer scaling uses whole physical pixels with the native GB aspect ratio: "
                    + width + "x" + height + " bounds=" + java.util.Arrays.toString(video) + " safe=" + java.util.Arrays.toString(safe));
            int[] button = settingsVisibleButton(idle, safe);
            int before = settingsBrightPixels(idle, button); idle.recycle();
            float x = (button[0] + button[2]) / 2f, y = (button[1] + button[3]) / 2f;
            long time = android.os.SystemClock.uptimeMillis();
            sendPointerSync(android.view.MotionEvent.obtain(time, time, android.view.MotionEvent.ACTION_DOWN, x, y, 0));
            android.os.SystemClock.sleep(180);
            try {
                android.graphics.Bitmap held = null; int brightness = 0;
                long deadline = android.os.SystemClock.uptimeMillis() + 3000;
                do {
                    if (held != null) held.recycle();
                    held = getUiAutomation().takeScreenshot();
                    brightness = held == null ? 0 : settingsBrightPixels(held, button);
                    if (brightness > before + 40) break;
                    android.os.SystemClock.sleep(100);
                } while (android.os.SystemClock.uptimeMillis() < deadline);
                screenshot("settings-media-" + (landscape ? "landscape" : "portrait") + "-held");
                check(held != null && brightness > before + 40,
                        "pressing the rendered configured button reaches its native hit target and brightens feedback: "
                        + "button=" + java.util.Arrays.toString(button) + " before=" + before + " held=" + brightness);
                if (held != null) held.recycle();
            } finally {
                sendPointerSync(android.view.MotionEvent.obtain(time, android.os.SystemClock.uptimeMillis(), android.view.MotionEvent.ACTION_UP, x, y, 0));
            }
            android.os.SystemClock.sleep(450);
            android.graphics.Bitmap released = getUiAutomation().takeScreenshot();
            check(released != null && settingsBrightPixels(released, button) <= before + 10, "release returns to configured resting opacity");
            if (released != null) released.recycle();
            configurationKey(activity, android.view.KeyEvent.KEYCODE_BACK); state(8, 0, 1, 1);
            configurationRow(activity, 2); state(0, 0, 0, 0);
        } finally {
            runOnMainSync(activity::finish);
            SharedPreferences.Editor restore = prefs.edit();
            for (java.util.Map.Entry<String, Object> e : previous.entrySet()) {
                restore.remove(e.getKey());
                if (e.getValue() instanceof Integer) restore.putInt(e.getKey(), (Integer)e.getValue());
                else if (e.getValue() instanceof Boolean) restore.putBoolean(e.getKey(), (Boolean)e.getValue());
                else if (e.getValue() instanceof String) restore.putString(e.getKey(), (String)e.getValue());
            }
            check(restore.commit(), "media test restores only its touched preferences");
        }
    }
    private void quickRow(int index) {
        for (int i = 0; i < index; ++i) sendKeyDownUpSync(android.view.KeyEvent.KEYCODE_DPAD_DOWN);
        sendKeyDownUpSync(android.view.KeyEvent.KEYCODE_DPAD_CENTER);
    }
    private void configurationKey(MainActivity activity, int key) throws Exception {
        waitFor(activity::hasWindowFocus, "configuration activity owns its input window");
        sendKeyDownUpSync(key);
        android.os.SystemClock.sleep(80);
    }
    private void configurationRow(MainActivity activity, int index) throws Exception {
        for (int i = 0; i < index; ++i) configurationKey(activity, android.view.KeyEvent.KEYCODE_DPAD_DOWN);
        configurationKey(activity, android.view.KeyEvent.KEYCODE_DPAD_CENTER);
    }
    private void configurationPlayers(int players, int controlled) throws Exception {
        waitFor(() -> { String current = frontendState(); return current.contains(" players=" + players + " controlled=" + controlled)
                && current.contains(" active=1 "); }, "native paired players and selected input port " + players + "/" + controlled);
    }
    private String configurationFile(File file) throws Exception {
        if (!file.exists()) return "";
        check(file.isFile() && file.length() <= 128 * 1024, "bounded native options file");
        try (InputStream in = new java.io.FileInputStream(file); ByteArrayOutputStream bytes = new ByteArrayOutputStream()) {
            byte[] buffer = new byte[4096]; int size;
            while ((size = in.read(buffer)) != -1) {
                check(bytes.size() + size <= 128 * 1024, "native options read remains bounded");
                bytes.write(buffer, 0, size);
            }
            return new String(bytes.toByteArray(), StandardCharsets.UTF_8);
        }
    }
    private void configuration() throws Exception {
        check(overlaySystem >= 0 && overlaySystem < 3, "supported configuration console");
        String id = prepareOverlayGame(overlaySystem);
        Context context = getTargetContext();
        String coreId = new String[] {"corvus.gb", "corvus.gbc", "corvus.gba"}[overlaySystem];
        File options = new File(context.getFilesDir(), "options/" + coreId + ".cfg");
        String prefix = "configuration-" + overlaySystem + (landscape ? "-landscape" : "-portrait");
        context.getSharedPreferences("corvus", Context.MODE_PRIVATE).edit().putInt("achievements_enabled", 0)
                .putString("theme", "corvus").putInt("show_input", 1).putBoolean("audio", true).commit();
        MainActivity activity = (MainActivity) startActivitySync(new Intent(context, MainActivity.class).addFlags(Intent.FLAG_ACTIVITY_NEW_TASK));
        try {
            check(getUiAutomation().setRotation(landscape ? android.app.UiAutomation.ROTATION_FREEZE_90 : android.app.UiAutomation.ROTATION_FREEZE_0), "configuration test rotation");
            waitFor(() -> { int[] area = activity.safeWindowArea(); return area != null && (area[0] > area[1]) == landscape; }, "configuration safe viewport");
            state(0, -1, 0, 0);
            configurationRow(activity, 0); state(3, -1, 0, 0);
            configurationRow(activity, overlaySystem); state(0, overlaySystem, 0, 0);
            configurationPlayers(0, 0);
            configurationRow(activity, 2); state(19, overlaySystem, 0, 0);
            screenshot(prefix + "-subsystems");
            configurationRow(activity, 0); state(22, overlaySystem, 0, 0);
            configurationRow(activity, 2);
            android.os.SystemClock.sleep(200);
            state(22, overlaySystem, 0, 0); configurationPlayers(0, 0);
            configurationKey(activity, android.view.KeyEvent.KEYCODE_DPAD_DOWN);
            configurationRow(activity, 0); state(23, overlaySystem, 0, 0);
            String[] games = activity.games(); int row = 1, selectedRow = -1;
            for (int i = 0; i < games.length; i += 6) {
                if (!coreId.equals(games[i + 4])) continue;
                if (id.equals(games[i])) selectedRow = row;
                row++;
            }
            check(selectedRow > 0, "private synthetic game appears in its core's slot selector");
            configurationRow(activity, selectedRow); state(22, overlaySystem, 0, 0);
            configurationRow(activity, 1); state(23, overlaySystem, 0, 0);
            configurationRow(activity, selectedRow); state(22, overlaySystem, 0, 0);
            screenshot(prefix + "-both-slots");
            configurationRow(activity, 1); state(8, overlaySystem, 1, 0); configurationPlayers(2, 0);
            android.os.SystemClock.sleep(700); screenshot(prefix + "-playing");
            quickOpen(activity, overlaySystem); screenshot(prefix + "-quick");
            configurationRow(activity, 3);
            android.os.SystemClock.sleep(200); state(8, overlaySystem, 1, 1);
            configurationKey(activity, android.view.KeyEvent.KEYCODE_BACK); state(8, overlaySystem, 1, 0);
            quickOpen(activity, overlaySystem); configurationRow(activity, 4); state(20, overlaySystem, 1, 0);
            screenshot(prefix + "-options");
            String key = overlaySystem == 0 ? "gb.palette" : overlaySystem == 1 ? "gbc.colors" : "gba.save_type";
            String[] values = overlaySystem == 0 ? new String[] {"green", "gray", "amber"} : overlaySystem == 1 ?
                    new String[] {"auto", "gray", "up", "a_up", "b_up", "left", "a_left", "b_left", "down", "a_down", "b_down", "right", "a_right", "b_right"} :
                    new String[] {"auto", "none", "sram", "flash_64k", "flash_128k", "eeprom_512", "eeprom_8k"};
            String currentOptions = configurationFile(options); int current = 0;
            for (int i = 0; i < values.length; ++i)
                if (currentOptions.contains("\n" + key + "=" + values[i] + "\n")) current = i;
            int preferred = overlaySystem == 2 ? 2 : 1;
            int target = current == preferred ? 0 : preferred;
            configurationRow(activity, 0); state(21, overlaySystem, 1, 0);
            screenshot(prefix + "-values");
            configurationRow(activity, (target - current + values.length) % values.length);
            state(20, overlaySystem, 1, 0);
            check(configurationFile(options).contains("\n" + key + "=" + values[target] + "\n"), "native option selection is durably persisted by stable value ID");
            configurationKey(activity, android.view.KeyEvent.KEYCODE_BACK); state(8, overlaySystem, 1, 1);
            configurationKey(activity, android.view.KeyEvent.KEYCODE_BACK); state(8, overlaySystem, 1, 0);
            quickOpen(activity, overlaySystem); configurationRow(activity, 1);
            state(8, overlaySystem, 1, 0); configurationPlayers(2, 0);
            android.os.SystemClock.sleep(700);
            quickOpen(activity, overlaySystem); configurationRow(activity, 9);
            state(8, overlaySystem, 1, 1); configurationPlayers(2, 1);
            screenshot(prefix + "-player-two");
            configurationKey(activity, android.view.KeyEvent.KEYCODE_BACK); state(8, overlaySystem, 1, 0);
            quickOpen(activity, overlaySystem); configurationRow(activity, 2);
            state(0, overlaySystem, 0, 0); configurationPlayers(0, 0);
            check(configurationFile(options).contains("\n" + key + "=" + values[target] + "\n"), "closing content preserves its core options");
            if (overlaySystem == 2) {
                check(new File(context.getFilesDir(), "saves/" + id + ".sav").length() == 32 * 1024, "player one keeps the historic SRAM path");
                check(new File(context.getFilesDir(), "saves/" + id + "-link-p2.sav").length() == 32 * 1024, "same-ROM player two has independent SRAM storage");
            }
            configurationRow(activity, 2); state(19, overlaySystem, 0, 0);
            configurationKey(activity, android.view.KeyEvent.KEYCODE_BACK); state(0, overlaySystem, 0, 0);
            configurationKey(activity, android.view.KeyEvent.KEYCODE_DPAD_UP);
            configurationKey(activity, android.view.KeyEvent.KEYCODE_DPAD_UP);
            configurationRow(activity, 0); state(0, -1, 0, 0); configurationPlayers(0, 0);
            configurationRow(activity, 2); state(14, -1, 0, 0);
            screenshot(prefix + "-unloaded-information");
            configurationKey(activity, android.view.KeyEvent.KEYCODE_BACK); state(0, -1, 0, 0);
        } finally { runOnMainSync(activity::finish); }
    }
    private void overlays() throws Exception {
        check(overlaySystem >= 0 && overlaySystem < 3, "supported overlay console");
        String id = prepareOverlayGame(overlaySystem);
        Context context = getTargetContext();
        context.getSharedPreferences("corvus", Context.MODE_PRIVATE).edit().putInt("achievements_enabled", 0)
                .putString("theme", "corvus").putInt("show_input", 1).commit();
        MainActivity activity = (MainActivity) startActivitySync(new Intent(context, MainActivity.class).addFlags(Intent.FLAG_ACTIVITY_NEW_TASK));
        try {
            check(getUiAutomation().setRotation(landscape ? android.app.UiAutomation.ROTATION_FREEZE_90 : android.app.UiAutomation.ROTATION_FREEZE_0), "overlay test rotation");
            waitFor(() -> { int[] area = activity.safeWindowArea(); return area != null && (area[0] > area[1]) == landscape; }, "overlay safe viewport");
            state(0, -1, 0, 0);
            tap(activity, 150, 100);
            for (int i = 0; i < overlaySystem; ++i) sendKeyDownUpSync(android.view.KeyEvent.KEYCODE_DPAD_DOWN);
            sendKeyDownUpSync(android.view.KeyEvent.KEYCODE_DPAD_CENTER);
            state(0, overlaySystem, 0, 0);
            sendKeyDownUpSync(android.view.KeyEvent.KEYCODE_DPAD_DOWN);
            sendKeyDownUpSync(android.view.KeyEvent.KEYCODE_DPAD_DOWN);
            sendKeyDownUpSync(android.view.KeyEvent.KEYCODE_DPAD_CENTER); state(19, overlaySystem, 0, 0);
            screenshot("overlay-" + overlaySystem + "-subsystems");
            sendKeyDownUpSync(android.view.KeyEvent.KEYCODE_BACK); state(0, overlaySystem, 0, 0);
            tab(activity, 1); tap(activity, 150, 100); tap(activity, 150, 100);
            state(8, overlaySystem, 1, 0); android.os.SystemClock.sleep(700);
            screenshot("overlay-" + overlaySystem + "-playing");
            float[] m = overlayMetrics(activity); float w = m[0], h = m[1], s = m[2];
            float hx = h >= w ? w - 36 * s : 32 * s, hy = h - 36 * s;
            tap(activity, hx, hy);
            waitFor(() -> activity.preference("show_input", -1) == 0, "overlay hides without stopping emulation");
            screenshot("overlay-" + overlaySystem + "-hidden");
            quickOpen(activity, overlaySystem); screenshot("overlay-" + overlaySystem + "-quick");
            sendKeyDownUpSync(android.view.KeyEvent.KEYCODE_BACK); state(8, overlaySystem, 1, 0);
            tap(activity, hx, hy);
            waitFor(() -> activity.preference("show_input", -1) == 1, "hidden overlay restores with its own button");
            quickOpen(activity, overlaySystem); quickRow(3); state(18, overlaySystem, 1, 0);
            screenshot("overlay-" + overlaySystem + "-states");
            sendKeyDownUpSync(android.view.KeyEvent.KEYCODE_DPAD_CENTER); state(8, overlaySystem, 1, 0);
            check(new File(context.getFilesDir(), "states/" + id + "-0.14.1.state").length() > 0, "native state file saved by quick submenu");
            quickOpen(activity, overlaySystem); quickRow(3); state(18, overlaySystem, 1, 0);
            sendKeyDownUpSync(android.view.KeyEvent.KEYCODE_DPAD_DOWN);
            sendKeyDownUpSync(android.view.KeyEvent.KEYCODE_DPAD_CENTER); state(8, overlaySystem, 1, 0);
            quickOpen(activity, overlaySystem); quickRow(4); state(20, overlaySystem, 1, 0);
            screenshot("overlay-" + overlaySystem + "-core-options");
            sendKeyDownUpSync(android.view.KeyEvent.KEYCODE_BACK); state(8, overlaySystem, 1, 1);
            sendKeyDownUpSync(android.view.KeyEvent.KEYCODE_BACK); state(8, overlaySystem, 1, 0);
            quickOpen(activity, overlaySystem); quickRow(5); state(13, overlaySystem, 1, 0);
            screenshot("overlay-" + overlaySystem + "-controls");
            sendKeyDownUpSync(android.view.KeyEvent.KEYCODE_BACK); state(8, overlaySystem, 1, 1);
            sendKeyDownUpSync(android.view.KeyEvent.KEYCODE_BACK); state(8, overlaySystem, 1, 0);
            quickOpen(activity, overlaySystem); quickRow(2); state(0, overlaySystem, 0, 0);
        } finally { runOnMainSync(activity::finish); }
    }
    private void navigation() throws Exception {
        prepareGame();
        Context context = getTargetContext();
        android.content.SharedPreferences preferences = context.getSharedPreferences("corvus", Context.MODE_PRIVATE);
        preferences.edit().putInt("achievements_enabled", 0).putString("theme", "corvus").putBoolean("audio", true).commit();
        MainActivity activity = (MainActivity) startActivitySync(new Intent(context, MainActivity.class).addFlags(Intent.FLAG_ACTIVITY_NEW_TASK));
        try {
            check(getUiAutomation().setRotation(landscape ? android.app.UiAutomation.ROTATION_FREEZE_90 : android.app.UiAutomation.ROTATION_FREEZE_0), "navigation rotation requested");
            waitFor(() -> { int[] area = activity.safeWindowArea(); return area != null && (area[0] > area[1]) == landscape; }, "navigation viewport orientation");
            state(0, -1, 0, 0);
            android.graphics.drawable.Drawable icon = context.getPackageManager().getApplicationIcon(context.getPackageName());
            check(icon instanceof android.graphics.drawable.AdaptiveIconDrawable, "APK exposes the adaptive launcher logo");
            android.graphics.Bitmap launcher = android.graphics.Bitmap.createBitmap(256, 256, android.graphics.Bitmap.Config.ARGB_8888);
            icon.setBounds(0, 0, 256, 256); icon.draw(new android.graphics.Canvas(launcher));
            try (FileOutputStream out = new FileOutputStream(new File(context.getFilesDir(), "test-navigation-launcher.png"))) {
                launcher.compress(android.graphics.Bitmap.CompressFormat.PNG, 100, out);
            } finally { launcher.recycle(); }
            tap(activity, 150, 176);
            check(activity.hasWindowFocus() && frontendState().contains("core=-1 content=0"), "content picker stays closed without a selected core");
            tab(activity, 1); screenshot("history-only");
            sendKeyDownUpSync(android.view.KeyEvent.KEYCODE_DPAD_DOWN);
            sendKeyDownUpSync(android.view.KeyEvent.KEYCODE_DPAD_CENTER);
            state(4, -1, 0, 0);
            tap(activity, 150, 100); state(4, -1, 0, 0); screenshot("blocked-history");
            selectFixtureCore(activity);
            tab(activity, 1); tap(activity, 150, 100); tap(activity, 150, 100);
            state(8, 0, 1, 0); screenshot("playing");
            closeContent(); screenshot("closed-core-kept");
            tab(activity, 1); tap(activity, 150, 100); tap(activity, 150, 100);
            state(8, 0, 1, 0);
            closeContent(); tap(activity, 150, 100); state(0, -1, 0, 0);
            tab(activity, 1); tap(activity, 150, 100); tap(activity, 150, 100);
            state(4, -1, 0, 0);
            for (int theme : new int[] {0, 3, 2}) {
                tab(activity, 2); tap(activity, 150, 100); tap(activity, 150, 100);
                tap(activity, 150, 100 + 76 * (theme == 0 ? 1 : theme == 3 ? 2 : 0));
                check(activity.preference("theme", -1) == theme, "native theme selection persisted");
                tab(activity, 2); tap(activity, 150, 252);
                screenshot("switch-on-" + theme);
                tap(activity, 150, 100);
                check(activity.preference("audio", -1) == 0, "native switch disabled audio once");
                screenshot("switch-off-" + theme);
                tap(activity, 150, 100);
                check(activity.preference("audio", -1) == 1, "native switch enabled audio once");
            }
            tab(activity, 0); screenshot("main");
        } finally { runOnMainSync(activity::finish); }
    }
    private void noLaunchPlaceholder(MainActivity activity, String stage,
                                     java.util.concurrent.CountDownLatch captured) throws Exception {
        android.graphics.Bitmap screen = null;
        long deadline = android.os.SystemClock.uptimeMillis() + 5000;
        do {
            if (screen != null) screen.recycle();
            android.os.SystemClock.sleep(100);
            screen = getUiAutomation().takeScreenshot();
        } while ((screen == null || !hasColor(screen, 0xffe0f8d0)) && android.os.SystemClock.uptimeMillis() < deadline);
        // The screenshot is immutable; image delivery need not wait for PNG encoding.
        if (screen != null && captured != null) captured.countDown();
        check(screen != null && hasColor(screen, 0xffe0f8d0), stage + " shows an actual GB video frame");
        int[] area = activity.safeWindowArea();
        int height = Math.min((int) (120 * activity.getResources().getDisplayMetrics().density), screen.getHeight() - area[3]);
        android.graphics.Bitmap popupArea = android.graphics.Bitmap.createBitmap(screen, area[2], area[3],
                screen.getWidth() - area[2] - area[4], height);
        try {
            try (FileOutputStream out = new FileOutputStream(new File(getTargetContext().getFilesDir(), "test-popup-" + stage + ".png"))) {
                screen.compress(android.graphics.Bitmap.CompressFormat.PNG, 100, out);
            }
            check(!hasColor(popupArea, 0xffb69aff), stage + " shows no preliminary crow popup");
        } finally { popupArea.recycle(); screen.recycle(); }
    }
    private void popups() throws Exception {
        transport(); prepareGame();
        Context context = getTargetContext();
        File[] cachedImages = new File(context.getCacheDir(), "achievement-images").listFiles();
        if (cachedImages != null) for (File image : cachedImages) check(image.delete(), "cold image cache fixture");
        onlineFlag = new File(context.getExternalFilesDir(null), "ra-network-online");
        try (FileOutputStream out = new FileOutputStream(onlineFlag)) { out.write(1); }
        RetroAchievementsService account = new RetroAchievementsService(context);
        account.saveCredentials("", ""); storageDrained(); account.saveCredentials(USER, TOKEN); storageDrained(); account.close();
        context.getSharedPreferences("corvus", Context.MODE_PRIVATE).edit().putInt("achievements_enabled", 1).putString("theme", "corvus").commit();
        MainActivity activity = (MainActivity) startActivitySync(new Intent(context, MainActivity.class).addFlags(Intent.FLAG_ACTIVITY_NEW_TASK));
        try {
            check(getUiAutomation().setRotation(landscape ? android.app.UiAutomation.ROTATION_FREEZE_90 : android.app.UiAutomation.ROTATION_FREEZE_0),
                    "requested popup test rotation");
            waitFor(() -> activity.safeWindowArea() != null && logins > 0, "native account and safe viewport ready");
            waitFor(() -> {
                int[] area = activity.safeWindowArea();
                return area != null && (area[0] > area[1]) == landscape;
            }, "actual native viewport matches requested orientation");
            android.graphics.Bitmap warmup = getUiAutomation().takeScreenshot();
            if (warmup != null) warmup.recycle();
            for (String image : new String[] {"Badge/00001_lock.png", "Badge/00001.png"})
                waitFor(() -> activity.achievementImage("https://media.retroachievements.org/" + image) != null,
                        "fixture image ready before timed popup capture");
            android.os.SystemClock.sleep(800);
            selectFixtureCore(activity);
            int[] area = activity.safeWindowArea();
            float density = activity.getResources().getDisplayMetrics().density;
            tap(activity, (area[0] - area[2] - area[4]) / density / 2,
                    (area[1] - area[3] - area[5]) / density - 32);
            tap(activity, 150, 100); tap(activity, 150, 100);
            state(8, 0, 1, 0);
            waitFor(() -> metadataRequested, "game identification is pending");
            noLaunchPlaceholder(activity, "waiting-metadata", null);
            metadataRelease.countDown();
            waitFor(() -> gameImageRequested, "game image requested after identification");
            noLaunchPlaceholder(activity, "waiting-image", gameImageRelease);
            capturePopup("launch", GAME_COLOR);
            android.os.SystemClock.sleep(4500);
            sendKeyDownUpSync(android.view.KeyEvent.KEYCODE_BUTTON_A);
            capturePopup("progress", PROGRESS_COLOR);
            android.os.SystemClock.sleep(2200);
            android.graphics.Bitmap hidden = getUiAutomation().takeScreenshot();
            check(hidden != null && !hasColor(hidden, PROGRESS_COLOR), "progress indicator expires after two seconds");
            if (hidden != null) hidden.recycle();
            check(getUiAutomation().performGlobalAction(android.accessibilityservice.AccessibilityService.GLOBAL_ACTION_HOME),
                    "pause to exercise native window lifecycle");
            android.os.SystemClock.sleep(500);
            context.startActivity(new Intent(context, MainActivity.class).addFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_REORDER_TO_FRONT));
            waitFor(activity::hasWindowFocus, "game window focused after resume");
            state(8, 0, 1, 0);
            sendKeyDownUpSync(android.view.KeyEvent.KEYCODE_BUTTON_B);
            capturePopup("unlocked", UNLOCK_COLOR);
            waitFor(() -> awards == 1, "actual core memory unlocked one achievement with decoded images");
            closeContent();
            android.graphics.Bitmap cleared = getUiAutomation().takeScreenshot();
            check(cleared != null && !hasColor(cleared, UNLOCK_COLOR), "closing content keeps the core and removes the old game's popup");
            if (cleared != null) cleared.recycle();
            tap(activity, 150, 100); state(0, -1, 0, 0);
        } finally {
            metadataRelease.countDown(); gameImageRelease.countDown();
            activity.achievementCredentialsSet("", ""); storageDrained(); runOnMainSync(activity::finish);
            context.getSharedPreferences("corvus", Context.MODE_PRIVATE).edit().putInt("achievements_enabled", 0).commit();
        }
    }
    private void ui() throws Exception {
        transport(); prepareGame();
        Context context = getTargetContext();
        onlineFlag = new File(context.getExternalFilesDir(null), "ra-network-online"); onlineFlag.delete();
        RetroAchievementsService clear = new RetroAchievementsService(context);
        clear.saveCredentials("", ""); storageDrained(); clear.close();
        context.getSharedPreferences("corvus", Context.MODE_PRIVATE).edit().putInt("achievements_enabled", 0).commit();
        MainActivity activity = (MainActivity) startActivitySync(new Intent(context, MainActivity.class).addFlags(Intent.FLAG_ACTIVITY_NEW_TASK));
        waitFor(() -> activity.safeWindowArea() != null, "safe viewport ready before selecting a core");
        selectFixtureCore(activity);
        android.util.Log.i("CorvusRetroTest", "RA_UI_READY");
        long deadline = android.os.SystemClock.uptimeMillis() + 150000;
        try {
            while (!serverUnlocked && android.os.SystemClock.uptimeMillis() < deadline) android.os.SystemClock.sleep(100);
            check(logins > 0, "native login reached the official protocol adapter");
            check(failedAwards > 0, "real emulation reached achievement and handled offline failure");
            check(awards == 1, "persisted award synchronized once after closing its game");
            storageDrained();
        } finally {
            activity.achievementCredentialsSet("", ""); storageDrained();
            runOnMainSync(activity::finish);
            context.getSharedPreferences("corvus", Context.MODE_PRIVATE).edit().putInt("achievements_enabled", 0).commit();
        }
    }
    @Override public void onCreate(Bundle arguments) {
        super.onCreate(arguments);
        phase = arguments.getString("phase", "credentials");
        landscape = "landscape".equals(arguments.getString("orientation", "portrait"));
        overlaySystem = Integer.parseInt(arguments.getString("system", "0"));
        fixtures = new File(arguments.getString("fixture_dir", "")); start();
    }
    @Override public void onStart() {
        Bundle result = new Bundle();
        try {
            if ("savetransfer".equals(phase) || "configuration".equals(phase) || "overlays".equals(phase)
                    || "navigation".equals(phase) || "popups".equals(phase) || "ui".equals(phase)
                    || "settings_media".equals(phase)) displayDefaults();
            if ("settings_media".equals(phase)) settingsMedia(); else if ("savetransfer".equals(phase)) saveTransfers(); else if ("configuration".equals(phase)) configuration(); else if ("overlays".equals(phase)) overlays(); else if ("navigation".equals(phase)) navigation(); else if ("popups".equals(phase)) popups(); else if ("ui".equals(phase)) ui(); else if ("outbox".equals(phase)) outbox(); else if ("recover".equals(phase)) recover(); else credentials();
            restoreDisplayPreferences();
            result.putString("stream", "PASS " + checks + " Android RetroAchievements checks phase=" + phase + " pid=" + android.os.Process.myPid() + "\n");
            finish(Activity.RESULT_OK, result);
        } catch (Throwable failure) {
            try { restoreDisplayPreferences(); } catch (Throwable cleanup) { failure.addSuppressed(cleanup); }
            result.putString("stream", "FAIL " + failure + "\n"); finish(Activity.RESULT_CANCELED, result);
        }
    }
}
