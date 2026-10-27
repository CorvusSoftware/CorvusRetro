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
    private File fixtures;
    private int checks;
    private volatile int failedAwards, awards, logins, acceptedRequests, imageRequests;
    private volatile boolean replyAlreadyAwarded;
    private volatile boolean serverUnlocked;
    private volatile boolean metadataRequested, gameImageRequested;
    private final java.util.concurrent.CountDownLatch metadataRelease = new java.util.concurrent.CountDownLatch(1);
    private final java.util.concurrent.CountDownLatch gameImageRelease = new java.util.concurrent.CountDownLatch(1);
    private File onlineFlag;
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
        File archive = new File(fixtures, "CorvusGB-0.14.1-android-x86_64.zip");
        JSONObject core = new JSONObject().put("id", "corvus.gb").put("name", "CorvusGB").put("description", "Qualification")
                .put("version", "0.14.1").put("abi", "x86_64").put("api_version", 0).put("library_name", "libcorvus_gb.so")
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
        long deadline = android.os.SystemClock.uptimeMillis() + 1800;
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
    private void tap(MainActivity activity, float x, float y) {
        int[] area = activity.safeWindowArea();
        float density = activity.getResources().getDisplayMetrics().density;
        long time = android.os.SystemClock.uptimeMillis();
        float px = area[2] + x * density, py = area[3] + y * density;
        sendPointerSync(android.view.MotionEvent.obtain(time, time, android.view.MotionEvent.ACTION_DOWN, px, py, 0));
        sendPointerSync(android.view.MotionEvent.obtain(time, time + 16, android.view.MotionEvent.ACTION_UP, px, py, 0));
        android.os.SystemClock.sleep(500);
    }
    private void noLaunchPlaceholder(MainActivity activity, String stage) throws Exception {
        android.graphics.Bitmap screen = getUiAutomation().takeScreenshot();
        check(screen != null, stage + " screenshot available");
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
        context.getSharedPreferences("corvus", Context.MODE_PRIVATE).edit().putInt("achievements_enabled", 1).putInt("theme", 2).commit();
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
            int[] area = activity.safeWindowArea();
            float density = activity.getResources().getDisplayMetrics().density;
            tap(activity, (area[0] - area[2] - area[4]) / density / 2,
                    (area[1] - area[3] - area[5]) / density - 32);
            tap(activity, 150, 100); tap(activity, 150, 100);
            waitFor(() -> metadataRequested, "game identification is pending");
            noLaunchPlaceholder(activity, "waiting-metadata");
            metadataRelease.countDown();
            waitFor(() -> gameImageRequested, "game image requested after identification");
            noLaunchPlaceholder(activity, "waiting-image");
            gameImageRelease.countDown();
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
            android.os.SystemClock.sleep(1000);
            sendKeyDownUpSync(android.view.KeyEvent.KEYCODE_BUTTON_B);
            capturePopup("unlocked", UNLOCK_COLOR);
            waitFor(() -> awards == 1, "actual core memory unlocked one achievement with decoded images");
            sendKeyDownUpSync(android.view.KeyEvent.KEYCODE_BACK);
            for (int i = 0; i < 4; ++i) sendKeyDownUpSync(android.view.KeyEvent.KEYCODE_DPAD_DOWN);
            sendKeyDownUpSync(android.view.KeyEvent.KEYCODE_DPAD_CENTER); android.os.SystemClock.sleep(300);
            tap(activity, 150, 176);
            android.graphics.Bitmap cleared = getUiAutomation().takeScreenshot();
            check(cleared != null && !hasColor(cleared, UNLOCK_COLOR), "unloading removes the old game's popup");
            if (cleared != null) cleared.recycle();
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
        fixtures = new File(arguments.getString("fixture_dir", "")); start();
    }
    @Override public void onStart() {
        Bundle result = new Bundle();
        try {
            if ("popups".equals(phase)) popups(); else if ("ui".equals(phase)) ui(); else if ("outbox".equals(phase)) outbox(); else if ("recover".equals(phase)) recover(); else credentials();
            result.putString("stream", "PASS " + checks + " Android RetroAchievements checks phase=" + phase + " pid=" + android.os.Process.myPid() + "\n");
            finish(Activity.RESULT_OK, result);
        } catch (Throwable failure) {
            result.putString("stream", "FAIL " + failure + "\n"); finish(Activity.RESULT_CANCELED, result);
        }
    }
}
