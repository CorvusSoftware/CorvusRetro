// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
package com.corvussoftware.corvusretro;

/** CorvusCore calls for an instance must all run on its emulation thread. */
final class NativeCore implements AutoCloseable {
    static { System.loadLibrary("corvusretro"); }
    private long handle;
    final int width;
    final int height;
    final int sampleRate;
    final double fps;

    NativeCore(int system, String romPath, byte[] battery) {
        handle = create(system);
        try {
            load(handle, romPath, battery);
            int[] info = info(handle);
            width = info[0]; height = info[1]; sampleRate = info[2]; fps = info[3] / 1000.0;
        } catch (RuntimeException | Error error) {
            close();
            throw error;
        }
    }
    static native String version();
    int frame(int[] pixels, int input, short[] audio) { return frame(handle, pixels, input, audio); }
    void reset() { reset(handle); }
    byte[] state() { return state(handle); }
    void restore(byte[] data) { restore(handle, data); }
    byte[] battery() { return battery(handle); }
    void acknowledgeBattery() { acknowledgeBattery(handle); }
    @Override public void close() {
        if (handle != 0) { destroy(handle); handle = 0; }
    }
    private static native long create(int system);
    private static native void load(long handle, String romPath, byte[] battery);
    private static native int[] info(long handle);
    private static native int frame(long handle, int[] pixels, int input, short[] audio);
    private static native void reset(long handle);
    private static native byte[] state(long handle);
    private static native void restore(long handle, byte[] data);
    private static native byte[] battery(long handle);
    private static native void acknowledgeBattery(long handle);
    private static native void destroy(long handle);
}
