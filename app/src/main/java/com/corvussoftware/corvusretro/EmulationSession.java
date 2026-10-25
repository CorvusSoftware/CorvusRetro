// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
package com.corvussoftware.corvusretro;

import android.media.AudioAttributes;
import android.media.AudioFormat;
import android.media.AudioTrack;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import java.io.IOException;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicInteger;

/** Owns the native handle, audio and file IO on one worker, never the UI thread. */
final class EmulationSession {
    interface Listener {
        void ready(int width, int height);
        void message(String message);
        void failed(String message);
    }
    private static final HandlerThread THREAD;
    static {
        THREAD = new HandlerThread("CorvusCore");
        THREAD.start();
    }
    private final Handler worker;
    private final Handler ui = new Handler(Looper.getMainLooper());
    private final AtomicInteger input = new AtomicInteger();
    private final AtomicInteger pulse = new AtomicInteger();
    private final AtomicBoolean deliveryPending = new AtomicBoolean();
    private final GameLibrary library;
    private final GameLibrary.Game game;
    private final FrameSurface surface;
    private final Listener listener;
    private volatile boolean foreground;
    private volatile boolean paused;
    private volatile boolean closed;
    private NativeCore core;
    private AudioTrack audio;
    private int[] pixels;
    private int[] delivery;
    private short[] samples;
    private long period;
    private long deadline;
    private long lastBattery;
    private boolean sound;
    EmulationSession(GameLibrary library, GameLibrary.Game game, FrameSurface surface, Listener listener) {
        this.library = library; this.game = game; this.surface = surface; this.listener = listener;
        // One queue makes an old session's close/save precede the next load.
        worker = new Handler(THREAD.getLooper()); worker.post(this::load);
    }
    private void postMessage(String text) { ui.post(() -> { if (!closed) listener.message(text); }); }
    private void load() {
        if (closed) return;
        try {
            byte[] battery = GameLibrary.read(library.battery(game), 1024 * 1024);
            core = new NativeCore(game.system, library.rom(game).getAbsolutePath(), battery);
            pixels = new int[core.width * core.height]; delivery = new int[pixels.length]; samples = new short[4096];
            period = Math.round(1_000_000_000 / core.fps);
            sound = library.preferences.getBoolean("audio", true);
            if (sound) {
                int minimum = AudioTrack.getMinBufferSize(core.sampleRate, AudioFormat.CHANNEL_OUT_STEREO, AudioFormat.ENCODING_PCM_16BIT);
                try {
                    if (minimum <= 0) throw new IllegalStateException("Format audio indisponible");
                    audio = new AudioTrack.Builder()
                            .setAudioAttributes(new AudioAttributes.Builder().setUsage(AudioAttributes.USAGE_GAME).setContentType(AudioAttributes.CONTENT_TYPE_MUSIC).build())
                            .setAudioFormat(new AudioFormat.Builder().setSampleRate(core.sampleRate).setChannelMask(AudioFormat.CHANNEL_OUT_STEREO).setEncoding(AudioFormat.ENCODING_PCM_16BIT).build())
                            .setBufferSizeInBytes(Math.max(minimum, core.sampleRate / 5 * 4))
                            .setTransferMode(AudioTrack.MODE_STREAM).build();
                    if (audio.getState() != AudioTrack.STATE_INITIALIZED) throw new IllegalStateException("Sortie audio indisponible");
                } catch (RuntimeException error) {
                    if (audio != null) { audio.release(); audio = null; }
                    sound = false;
                    postMessage(library.context.getString(R.string.audio_unavailable));
                }
            }
            try { library.touch(game); } catch (IOException error) { postMessage(error.getMessage()); }
            ui.post(() -> { if (!closed) listener.ready(core.width, core.height); });
            updateRunning();
        } catch (Exception | OutOfMemoryError error) { fatal(error); }
    }
    private final Runnable tick = new Runnable() {
        @Override public void run() {
            if (closed || !foreground || paused || core == null) return;
            try {
                int count = core.frame(pixels, input.get() | pulse.getAndSet(0), samples);
                if (sound && audio != null && count > 0) {
                    int offset = 0;
                    while (offset < count && foreground && !paused && !closed) {
                        int written = audio.write(samples, offset, count - offset, AudioTrack.WRITE_BLOCKING);
                        if (written <= 0) { sound = false; postMessage(library.context.getString(R.string.audio_unavailable)); break; }
                        offset += written;
                    }
                }
                if (deliveryPending.compareAndSet(false, true)) {
                    System.arraycopy(pixels, 0, delivery, 0, pixels.length);
                    ui.post(() -> {
                        if (!closed) surface.present(delivery);
                        deliveryPending.set(false);
                    });
                }
                long now = System.nanoTime();
                if (now - lastBattery > 5_000_000_000L) { saveBattery(); lastBattery = now; }
                deadline += period;
                if (now - deadline > period * 2) deadline = now;
                worker.postDelayed(this, Math.max(0, (deadline - System.nanoTime()) / 1_000_000));
            } catch (Exception | OutOfMemoryError error) { fatal(error); }
        }
    };
    private void updateRunning() {
        worker.removeCallbacks(tick);
        if (core == null || closed) return;
        if (foreground && !paused) {
            deadline = System.nanoTime();
            if (audio != null && sound) {
                try { audio.play(); }
                catch (RuntimeException error) { audio.release(); audio = null; sound = false; postMessage(library.context.getString(R.string.audio_unavailable)); }
            }
            worker.post(tick);
        } else {
            if (audio != null) {
                try { audio.pause(); audio.flush(); }
                catch (RuntimeException error) { audio.release(); audio = null; sound = false; }
            }
            try { saveBattery(); } catch (Exception error) { postMessage(error.getMessage()); }
        }
    }
    void foreground(boolean value) { foreground = value; if (!value) clearInput(); worker.post(this::updateRunning); }
    void paused(boolean value) { paused = value; if (value) clearInput(); worker.post(this::updateRunning); }
    void press(int mask) { input.getAndUpdate(value -> value | mask); }
    void release(int mask) { input.getAndUpdate(value -> value & ~mask); }
    void pulse(int mask) { if (!paused && !closed) pulse.getAndUpdate(value -> value | mask); }
    void clearInput() { input.set(0); pulse.set(0); }
    private void saveBattery() throws IOException {
        if (core == null) return;
        byte[] bytes = core.battery();
        if (bytes != null) { GameLibrary.write(library.battery(game), bytes); core.acknowledgeBattery(); }
    }
    void reset() { command(() -> { saveBattery(); core.reset(); }, R.string.restarted); }
    void saveState() { command(() -> GameLibrary.write(library.state(game), core.state()), R.string.state_saved); }
    void loadState() {
        command(() -> {
            byte[] state = GameLibrary.read(library.state(game), 16 * 1024 * 1024);
            if (state == null) throw new IOException(library.context.getString(R.string.no_state));
            core.restore(state);
        }, R.string.state_loaded);
    }
    private interface Command { void run() throws Exception; }
    private void command(Command command, int success) {
        worker.post(() -> {
            if (closed || core == null) return;
            try { command.run(); postMessage(library.context.getString(success)); }
            catch (Exception error) { postMessage(error.getMessage()); }
        });
    }
    private void fatal(Throwable error) {
        foreground = false; paused = true; clearInput(); worker.removeCallbacks(tick);
        if (audio != null) { audio.release(); audio = null; }
        ui.post(() -> { if (!closed) listener.failed(error.getMessage() == null ? error.getClass().getSimpleName() : error.getMessage()); });
    }
    void close() {
        if (closed) return;
        closed = true; foreground = false; clearInput(); ui.removeCallbacksAndMessages(null);
        worker.removeCallbacks(tick);
        worker.post(() -> {
            try { saveBattery(); } catch (Exception error) {
                android.content.Context app = library.context;
                ui.post(() -> android.widget.Toast.makeText(app, R.string.save_failed, android.widget.Toast.LENGTH_LONG).show());
            }
            if (audio != null) { audio.release(); audio = null; }
            if (core != null) { core.close(); core = null; }
        });
    }
}
