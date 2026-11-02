// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
package com.corvussoftware.corvusretro;

import android.content.Context;
import android.media.AudioAttributes;
import android.media.SoundPool;
import android.os.Handler;
import android.os.Looper;
import android.os.SystemClock;

/** Android owns decoding and short-effect playback; native emulation audio stays in AAudio. */
final class AchievementSound implements AutoCloseable {
    private static final long MAX_DELAY_MS = 1000;
    private final Handler handler = new Handler(Looper.getMainLooper());
    private SoundPool pool;
    private int sound, stream, pendingVolume, currentVolume = 100;
    private long pendingAt;
    private volatile long generation;
    private boolean loaded, active, closed;

    AchievementSound(Context context) {
        AudioAttributes attributes = new AudioAttributes.Builder()
                .setUsage(AudioAttributes.USAGE_GAME)
                .setContentType(AudioAttributes.CONTENT_TYPE_SONIFICATION).build();
        pool = new SoundPool.Builder().setMaxStreams(1).setAudioAttributes(attributes).build();
        pool.setOnLoadCompleteListener((owner, id, status) -> {
            if (closed || owner != pool || id != sound) return;
            loaded = status == 0;
            if (loaded && active && pendingAt != 0 && SystemClock.uptimeMillis() - pendingAt <= MAX_DELAY_MS)
                playNow(pendingVolume);
            pendingAt = 0;
        });
        sound = pool.load(context.getApplicationContext(), R.raw.achievement_unlock, 1);
    }
    void resume() { ++generation; active = true; }
    void pause() {
        ++generation; active = false; pendingAt = 0;
        if (stream != 0) { pool.stop(stream); stream = 0; }
    }
    void play(int volumePercent) {
        long issued = SystemClock.uptimeMillis(), epoch = generation;
        int volume = Math.max(0, Math.min(100, volumePercent));
        handler.post(() -> {
            if (closed || !active || epoch != generation || volume == 0 || currentVolume == 0 ||
                    SystemClock.uptimeMillis() - issued > MAX_DELAY_MS) return;
            if (!loaded) { pendingAt = issued; pendingVolume = volume; return; }
            playNow(volume);
        });
    }
    void volume(int volumePercent) {
        int volume = Math.max(0, Math.min(100, volumePercent));
        handler.post(() -> {
            if (closed) return;
            currentVolume = volume;
            if (volume == 0) {
                ++generation; pendingAt = 0;
                if (stream != 0) { pool.stop(stream); stream = 0; }
            } else if (stream != 0) pool.setVolume(stream, volume / 100f, volume / 100f);
        });
    }
    private void playNow(int volume) {
        if (stream != 0) { pool.stop(stream); stream = 0; }
        if (currentVolume == 0) return;
        float gain = Math.min(volume,currentVolume) / 100f;
        stream = pool.play(sound, gain, gain, 1, 0, 1);
    }
    @Override public void close() {
        if (closed) return;
        pause(); closed = true;
        handler.removeCallbacksAndMessages(null);
        pool.setOnLoadCompleteListener(null);
        pool.release(); pool = null;
    }
}
