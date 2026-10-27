// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
package com.corvussoftware.corvusretro;

import android.app.Activity;
import android.app.Instrumentation;
import android.content.Context;
import android.content.ContextWrapper;
import android.graphics.Bitmap;
import android.os.Bundle;
import android.os.SystemClock;
import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.InputStream;
import java.net.URL;
import java.net.URLConnection;
import java.net.URLStreamHandler;
import java.security.cert.Certificate;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicInteger;
import javax.net.ssl.HttpsURLConnection;

/** Original PNG fixtures and a synthetic HTTPS transport, no remote accounts or artwork. */
public final class RetroAchievementsImagesInstrumentation extends Instrumentation {
    private int checks;
    private final AtomicInteger requests = new AtomicInteger(), completions = new AtomicInteger();
    private volatile boolean offline;
    private byte[] png, oversized;
    private final CountDownLatch entered = new CountDownLatch(1), release = new CountDownLatch(1);
    private File cache;
    private static final String BASE = "https://media.retroachievements.org/Badge/";
    private void check(boolean condition, String message) {
        if (!condition) throw new AssertionError(message);
        checks++;
    }
    private static byte[] png(int width, int height) {
        Bitmap bitmap = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888);
        bitmap.eraseColor(0xff7856ab);
        ByteArrayOutputStream bytes = new ByteArrayOutputStream();
        bitmap.compress(Bitmap.CompressFormat.PNG, 100, bytes); bitmap.recycle();
        return bytes.toByteArray();
    }
    private void transport() {
        URL.setURLStreamHandlerFactory(protocol -> "https".equals(protocol) ? new URLStreamHandler() {
            @Override protected URLConnection openConnection(URL url) {
                return new HttpsURLConnection(url) {
                    @Override public int getResponseCode() throws java.io.IOException {
                        requests.incrementAndGet();
                        if (offline) throw new java.io.IOException("offline fixture");
                        return url.getPath().contains("redirect") ? 302 : 200;
                    }
                    @Override public long getContentLengthLong() { return -1; }
                    @Override public InputStream getInputStream() throws java.io.IOException {
                        if (url.getPath().contains("slow")) {
                            entered.countDown();
                            try { if (!release.await(5, TimeUnit.SECONDS)) throw new java.io.IOException("fixture timeout"); }
                            catch (InterruptedException error) { throw new java.io.IOException(error); }
                        }
                        byte[] bytes = url.getPath().contains("dimensions") ? oversized :
                                url.getPath().contains("bytes") ? new byte[512 * 1024 + 1] :
                                url.getPath().contains("invalid") ? new byte[] {1, 2, 3} : png;
                        return new ByteArrayInputStream(bytes);
                    }
                    @Override public void disconnect() { }
                    @Override public boolean usingProxy() { return false; }
                    @Override public void connect() { }
                    @Override public String getCipherSuite() { return "fixture"; }
                    @Override public Certificate[] getLocalCertificates() { return null; }
                    @Override public Certificate[] getServerCertificates() { return null; }
                };
            }
        } : null);
    }
    private Context isolated() {
        return new ContextWrapper(getTargetContext()) {
            @Override public File getCacheDir() { return cache; }
        };
    }
    private Bitmap image(RetroAchievementsImages images, String url) throws Exception {
        long deadline = SystemClock.uptimeMillis() + 5000;
        Bitmap bitmap;
        do {
            bitmap = images.get(url);
            if (bitmap != null) return bitmap;
            Thread.sleep(10);
        } while (SystemClock.uptimeMillis() < deadline);
        throw new AssertionError("image was not decoded " + url.substring(url.lastIndexOf('/') + 1));
    }
    private static void clean(File directory) {
        File[] files = directory.listFiles();
        if (files != null) for (File file : files) { if (file.isDirectory()) clean(file); file.delete(); }
    }
    private void runChecks() throws Exception {
        cache = new File(getTargetContext().getCacheDir(), "image-qualification");
        clean(cache); check(cache.isDirectory() || cache.mkdirs(), "isolated image cache");
        png = png(192, 96); oversized = png(1025, 1); transport();
        String[] invalid = {"http://media.retroachievements.org/Badge/1.png",
                "https://example.org/Badge/1.png", "https://media.retroachievements.org.evil/Badge/1.png",
                "https://media.retroachievements.org:443/Badge/1.png", "https://user@media.retroachievements.org/Badge/1.png",
                BASE + "../Images/1.png", BASE + "1.png?token=secret", BASE + "1.png#fragment", BASE + "%2e%2e.png"};
        for (String url : invalid) check(!RetroAchievementsImages.allowed(url), "unknown origin or path rejected");
        check(RetroAchievementsImages.allowed(BASE + "123_lock.png") &&
                RetroAchievementsImages.allowed("https://media.retroachievements.org/Images/1.png"), "official art URLs allowed");
        RetroAchievementsImages images = new RetroAchievementsImages(isolated(), completions::incrementAndGet);
        try {
            long begin = SystemClock.elapsedRealtime();
            check(images.get(BASE + "slow.png") == null && SystemClock.elapsedRealtime() - begin < 500,
                    "frontend returns before blocked HTTPS");
            check(entered.await(5, TimeUnit.SECONDS), "worker reached slow transport");
            for (int i = 0; i < 20; i++) images.get(BASE + "slow.png");
            check(requests.get() == 1, "in-flight image requests are deduplicated");
            release.countDown();
            Bitmap result = image(images, BASE + "slow.png");
            check(result.getWidth() == 96 && result.getHeight() == 48 && result.getConfig() == Bitmap.Config.ARGB_8888,
                    "bounded RGBA tile preserves aspect ratio");
            check(images.get(BASE + "slow.png") == result && requests.get() == 1, "memory cache avoids repeated HTTPS");
            for (String bad : new String[] {"dimensions", "bytes", "invalid", "redirect"}) {
                int before = requests.get(); images.get(BASE + bad + ".png");
                long deadline = SystemClock.uptimeMillis() + 5000;
                while (requests.get() == before && SystemClock.uptimeMillis() < deadline) Thread.sleep(10);
                Thread.sleep(100);
                boolean absent = true;
                for (int i = 0; i < 20; i++) absent &= images.get(BASE + bad + ".png") == null;
                check(absent, "invalid response has no bitmap");
                check(requests.get() == before + 1, "failed response is negative-cached");
            }
            int before = requests.get();
            for (int i = 0; i < 34; ++i) image(images, BASE + "tile_" + i + ".png");
            check(requests.get() == before + 34, "distinct official badges load independently");
            before = requests.get(); image(images, BASE + "tile_0.png");
            check(requests.get() == before, "evicted memory tile is restored from disk without HTTP");
            for (int i = 34; i < 70; ++i) image(images, BASE + "tile_" + i + ".png");
            File[] files = new File(cache, "achievement-images").listFiles((directory, name) -> name.endsWith(".png"));
            check(files != null && files.length <= 64, "disk cache is bounded");
        } finally { release.countDown(); images.close(); }
        offline = true;
        RetroAchievementsImages restored = new RetroAchievementsImages(isolated(), completions::incrementAndGet);
        try {
            int before = requests.get();
            check(image(restored, BASE + "tile_69.png").getWidth() == 96 && requests.get() == before,
                    "new Activity restores official art while offline");
            restored.close();
            check(restored.get(BASE + "tile_69.png") == null, "destroyed service rejects further work");
        } finally { restored.close(); }
    }
    @Override public void onCreate(Bundle arguments) { super.onCreate(arguments); start(); }
    @Override public void onStart() {
        Bundle result = new Bundle();
        try {
            runChecks(); result.putString("stream", "PASS " + checks + " image transport/cache/lifecycle checks\n");
            finish(Activity.RESULT_OK, result);
        } catch (Throwable failure) {
            result.putString("stream", "FAIL " + failure + "\n"); finish(Activity.RESULT_CANCELED, result);
        }
    }
}
