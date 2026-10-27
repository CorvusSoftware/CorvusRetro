// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
package com.corvussoftware.corvusretro;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.os.SystemClock;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.net.URL;
import java.nio.charset.StandardCharsets;
import java.security.MessageDigest;
import java.util.Arrays;
import java.util.Comparator;
import java.util.Iterator;
import java.util.LinkedHashMap;
import java.util.Map;
import java.util.concurrent.ArrayBlockingQueue;
import java.util.concurrent.RejectedExecutionException;
import java.util.concurrent.ThreadPoolExecutor;
import java.util.concurrent.TimeUnit;
import java.util.regex.Pattern;
import javax.net.ssl.HttpsURLConnection;

/** Public badge and game art only. Rendering never waits for image IO or decoding. */
final class RetroAchievementsImages {
    private static final int MAX_BYTES = 512 * 1024, MAX_SIDE = 1024, TILE = 96;
    private static final int MEMORY_IMAGES = 32, STATES = 64, DISK_IMAGES = 64;
    private static final long RETRY_MS = 60000;
    private static final Pattern OFFICIAL = Pattern.compile(
            "https://media\\.retroachievements\\.org/(?:Images|Badge)/[A-Za-z0-9_-]{1,160}\\.png");
    // Rotation cannot accumulate one blocked HTTP worker for every old Activity.
    private static final ThreadPoolExecutor WORKERS = new ThreadPoolExecutor(2, 2, 0,
            TimeUnit.SECONDS, new ArrayBlockingQueue<>(16), new ThreadPoolExecutor.AbortPolicy());
    private final Context context;
    private final Runnable ready;
    private final Object lock = new Object();
    private final LinkedHashMap<String, Entry> entries = new LinkedHashMap<>(16, .75f, true);
    private volatile boolean closed;

    private static final class Entry {
        Bitmap bitmap;
        long retryAt;
        boolean pending;
        Runnable task;
    }

    RetroAchievementsImages(Context context, Runnable ready) {
        this.context = context;
        this.ready = ready;
    }

    static boolean allowed(String url) { return url != null && OFFICIAL.matcher(url).matches(); }

    Bitmap get(String url) {
        if (!allowed(url) || closed) return null;
        synchronized (lock) {
            if (closed) return null;
            Entry entry = entries.get(url);
            if (entry == null) {
                if (entries.size() >= STATES) {
                    Iterator<Map.Entry<String, Entry>> iterator = entries.entrySet().iterator();
                    while (iterator.hasNext()) {
                        if (!iterator.next().getValue().pending) { iterator.remove(); break; }
                    }
                    if (entries.size() >= STATES) return null;
                }
                entry = new Entry();
                entries.put(url, entry);
            }
            if (entry.bitmap != null) return entry.bitmap;
            long now = SystemClock.elapsedRealtime();
            if (entry.pending || now < entry.retryAt) return null;
            entry.pending = true;
            Entry target = entry;
            entry.task = () -> load(url, target);
            try { WORKERS.execute(entry.task); }
            catch (RejectedExecutionException full) {
                entry.pending = false;
                entry.task = null;
                entry.retryAt = now + RETRY_MS;
            }
            return null;
        }
    }

    private static byte[] read(InputStream input) throws IOException {
        ByteArrayOutputStream bytes = new ByteArrayOutputStream();
        byte[] buffer = new byte[8192];
        int count;
        while ((count = input.read(buffer)) != -1) {
            if (bytes.size() + count > MAX_BYTES) throw new IOException("image byte limit");
            bytes.write(buffer, 0, count);
        }
        return bytes.toByteArray();
    }

    private static Bitmap decode(byte[] data) throws IOException {
        byte[] png = {(byte) 137, 80, 78, 71, 13, 10, 26, 10};
        if (data.length < png.length || !Arrays.equals(Arrays.copyOf(data, png.length), png))
            throw new IOException("not PNG");
        BitmapFactory.Options bounds = new BitmapFactory.Options();
        bounds.inJustDecodeBounds = true;
        BitmapFactory.decodeByteArray(data, 0, data.length, bounds);
        if (bounds.outWidth <= 0 || bounds.outHeight <= 0 || bounds.outWidth > MAX_SIDE || bounds.outHeight > MAX_SIDE)
            throw new IOException("image dimension limit");
        BitmapFactory.Options options = new BitmapFactory.Options();
        options.inPreferredConfig = Bitmap.Config.ARGB_8888;
        options.inScaled = false;
        Bitmap source = BitmapFactory.decodeByteArray(data, 0, data.length, options);
        if (source == null) throw new IOException("invalid image");
        if (source.getConfig() != Bitmap.Config.ARGB_8888) {
            Bitmap rgba = source.copy(Bitmap.Config.ARGB_8888, false);
            source.recycle(); source = rgba;
            if (source == null) throw new IOException("image conversion");
        }
        int side = Math.max(source.getWidth(), source.getHeight());
        if (side <= TILE) return source;
        int width = Math.max(1, source.getWidth() * TILE / side);
        int height = Math.max(1, source.getHeight() * TILE / side);
        Bitmap tile = Bitmap.createScaledBitmap(source, width, height, true);
        if (tile != source) source.recycle();
        return tile;
    }

    private File cache(String url) throws Exception {
        File directory = new File(context.getCacheDir(), "achievement-images");
        if (!directory.isDirectory() && !directory.mkdirs()) throw new IOException("image cache unavailable");
        byte[] digest = MessageDigest.getInstance("SHA-256").digest(url.getBytes(StandardCharsets.US_ASCII));
        StringBuilder key = new StringBuilder(64);
        for (byte value : digest) key.append(String.format(java.util.Locale.ROOT, "%02x", value & 255));
        return new File(directory, key + ".png");
    }

    private byte[] download(String url) throws Exception {
        HttpsURLConnection connection = (HttpsURLConnection) new URL(url).openConnection();
        try {
            connection.setInstanceFollowRedirects(false);
            connection.setConnectTimeout(10000);
            connection.setReadTimeout(10000);
            connection.setRequestMethod("GET");
            connection.setUseCaches(false);
            if (connection.getResponseCode() != 200 || connection.getContentLengthLong() > MAX_BYTES)
                throw new IOException("image response unavailable");
            if (closed) throw new IOException("image request cancelled");
            try (InputStream input = connection.getInputStream()) { return read(input); }
        } finally { connection.disconnect(); }
    }

    private static synchronized void persist(File file, byte[] data) {
        // Only workers enter this lock; a fixed temporary also bounds storage after a process kill.
        File temporary = new File(file.getParentFile(), "image.pending");
        try {
            try (FileOutputStream output = new FileOutputStream(temporary)) { output.write(data); }
            if (!temporary.renameTo(file)) throw new IOException("cache replacement");
            File[] images = file.getParentFile().listFiles((directory, name) -> name.matches("[a-f0-9]{64}\\.png"));
            if (images != null && images.length > DISK_IMAGES) {
                Arrays.sort(images, Comparator.comparingLong(File::lastModified));
                for (int i = 0; i < images.length - DISK_IMAGES; ++i) images[i].delete();
            }
        } catch (IOException ignored) {
            // A cache write failure must not prevent displaying a valid downloaded image.
        } finally { temporary.delete(); }
    }

    private void load(String url, Entry entry) {
        if (closed) return;
        Bitmap bitmap = null;
        File file = null;
        try {
            file = cache(url);
            if (file.isFile() && file.length() <= MAX_BYTES) {
                try (InputStream input = new FileInputStream(file)) { bitmap = decode(read(input)); }
                catch (IOException invalid) { file.delete(); }
            }
        } catch (Exception unavailable) {
            // Cache storage is optional; HTTPS remains available without it.
        }
        try {
            if (bitmap == null && !closed) {
                byte[] data = download(url);
                bitmap = decode(data);
                if (file != null && !closed) persist(file, data);
            }
        } catch (Exception unavailable) {
            // Native widgets retain their Corvus fallback and retry after the negative-cache window.
        }
        boolean published = false;
        synchronized (lock) {
            if (closed || entries.get(url) != entry) return;
            entry.pending = false;
            entry.task = null;
            entry.bitmap = bitmap;
            entry.retryAt = SystemClock.elapsedRealtime() + RETRY_MS;
            if (bitmap != null) {
                int count = 0;
                for (Entry value : entries.values()) if (value.bitmap != null) count++;
                for (Entry value : entries.values()) {
                    if (count <= MEMORY_IMAGES) break;
                    if (value != entry && value.bitmap != null) { value.bitmap = null; value.retryAt = 0; count--; }
                }
                published = true;
            }
        }
        // Do not recycle evicted Bitmaps: a JNI local reference may still be uploading pixels.
        if (published && !closed && ready != null) ready.run();
    }

    void close() {
        synchronized (lock) {
            closed = true;
            for (Entry entry : entries.values()) if (entry.task != null) WORKERS.remove(entry.task);
            entries.clear();
        }
    }
}
