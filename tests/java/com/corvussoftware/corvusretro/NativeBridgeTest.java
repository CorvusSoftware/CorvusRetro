// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
package com.corvussoftware.corvusretro;

import java.util.Arrays;

public final class NativeBridgeTest {
    private static void check(boolean value, String message) {
        if (!value) throw new AssertionError(message);
    }
    public static void main(String[] args) {
        int[] frame = new int[256 * 192];
        NativeBridge.reset();
        NativeBridge.frame(frame, 0, false);
        int center = 90 * 256 + 122;
        check(frame[center] == 0xff57e0c2, "initial sprite missing");
        NativeBridge.frame(frame, 8, false);
        check(frame[center] != 0xff57e0c2, "right input did not move sprite");
        NativeBridge.frame(frame, 0, true);
        int[] paused = frame.clone();
        NativeBridge.frame(frame, 8, true);
        check(Arrays.equals(frame, paused), "paused sprite moved");
        NativeBridge.reset();
        NativeBridge.frame(frame, 0, false);
        check(frame[center] == 0xff57e0c2, "reset did not restore sprite");
        for (int[] invalid : new int[][] {null, new int[1]}) {
            boolean rejected = false;
            try { NativeBridge.frame(invalid, 0, false); }
            catch (IllegalArgumentException expected) { rejected = true; }
            check(rejected, "invalid buffer was not rejected");
        }
        System.out.println("PASS: JNI load, ARGB transfer, input, pause, reset, invalid buffers");
    }
}
