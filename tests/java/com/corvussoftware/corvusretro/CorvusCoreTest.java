// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
package com.corvussoftware.corvusretro;

import java.nio.file.Files;
import java.nio.file.Path;
import java.util.Arrays;

public final class CorvusCoreTest {
    private static void check(boolean value, String message) {
        if (!value) throw new AssertionError(message);
    }
    private static byte[] rom(int system) {
        if (system != 2) {
            byte[] data = new byte[32768];
            data[0x100] = (byte)0xc3; data[0x102] = 1;
            data[0x143] = system == 1 ? (byte)0x80 : 0;
            return data;
        }
        byte[] data = new byte[512];
        int[] program = {0xe59f0008, 0xe3a0105a, 0xe5c01000, 0xeafffffe, 0x0e000000};
        for (int i = 0; i < program.length; i++)
            for (int b = 0; b < 4; b++) data[i * 4 + b] = (byte)(program[i] >>> (b * 8));
        data[0xb2] = (byte)0x96;
        byte[] marker = {'S', 'R', 'A', 'M', '_', 'V'};
        System.arraycopy(marker, 0, data, 0x100, marker.length);
        return data;
    }
    public static void main(String[] args) throws Exception {
        check(NativeCore.version().equals("0.14.0"), "Unexpected pinned core version");
        Path folder = Files.createTempDirectory("corvus-core-test");
        try {
            for (int system = 0; system < 3; system++) {
                Path file = folder.resolve("synthetic-" + system + ".rom");
                Files.write(file, rom(system));
                try (NativeCore core = new NativeCore(system)) {
                    check(!core.hasContent(), "Idle core unexpectedly has content");
                    boolean emptyRejected = false;
                    try { core.state(); } catch (IllegalArgumentException expected) { emptyRejected = true; }
                    check(emptyRejected, "Idle core should reject content operations");
                    core.loadContent(file.toString(), null);
                    check(core.hasContent(), "Loaded core still reported idle");
                    check(core.width == (system == 2 ? 240 : 160), "Wrong video width");
                    check(core.height == (system == 2 ? 160 : 144), "Wrong video height");
                    check(core.sampleRate > 0 && core.fps > 50 && core.fps < 65, "Invalid timing");
                    int[] pixels = new int[core.width * core.height];
                    short[] audio = new short[4096];
                    int count = core.frame(pixels, 0, audio);
                    check(count > 0 && count <= audio.length && count % 2 == 0, "No stereo audio");
                    check((pixels[0] >>> 24) == 255, "Missing ARGB output");
                    byte[] state = core.state();
                    core.frame(pixels, 16, audio);
                    core.restore(state);
                    check(Arrays.equals(state, core.state()), "State restoration differs");
                    boolean rejected = false;
                    try { core.frame(new int[1], 0, audio); } catch (IllegalArgumentException expected) { rejected = true; }
                    check(rejected, "Short framebuffer accepted");
                    rejected = false;
                    try { core.frame(pixels, 0, new short[1]); } catch (IllegalArgumentException expected) { rejected = true; }
                    check(rejected, "Short audio buffer accepted");
                    rejected = false;
                    try { core.restore(new byte[8]); } catch (IllegalStateException expected) { rejected = true; }
                    check(rejected, "Malformed state accepted");
                    if (system == 2) {
                        byte[] battery = core.battery();
                        check(battery != null && battery[0] == 0x5a, "SRAM write missing");
                        core.acknowledgeBattery();
                        check(core.battery() == null, "Acknowledged battery remains dirty");
                        try (NativeCore restored = new NativeCore(system, file.toString(), battery)) {
                            restored.frame(new int[restored.width * restored.height], 0, audio);
                        }
                    }
                    core.reset();
                    core.frame(pixels, 0, audio);
                    core.close();
                    check(!core.hasContent(), "Unloaded core retained content flag");
                    boolean closedRejected = false;
                    try { core.frame(pixels, 0, audio); } catch (IllegalArgumentException expected) { closedRejected = true; }
                    check(closedRejected, "Unloaded handle still runs frames");
                    core.close();
                    try (NativeCore reloaded = new NativeCore(system, file.toString(), null)) {
                        reloaded.frame(new int[reloaded.width * reloaded.height], 0, audio);
                    }
                    System.out.println("PASS CorvusCore system " + system + ": idle load, content, video, audio, inputs, state, reset, unload/reload, invalid buffers");
                }
                Files.delete(file);
            }
            Path invalid = folder.resolve("empty.rom"); Files.write(invalid, new byte[0]);
            boolean rejected = false;
            try (NativeCore ignored = new NativeCore(0, invalid.toString(), null)) {
                throw new AssertionError("Empty ROM accepted");
            } catch (IllegalArgumentException expected) { rejected = true; }
            check(rejected, "Empty content not rejected"); Files.delete(invalid);
        } finally { Files.deleteIfExists(folder); }
    }
}
