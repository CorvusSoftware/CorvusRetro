// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
package com.corvussoftware.corvusretro;

final class NativeBridge {
    static { System.loadLibrary("corvusretro"); }
    private NativeBridge() {}
    static native void reset();
    static native void frame(int[] pixels, int input, boolean paused);
}
