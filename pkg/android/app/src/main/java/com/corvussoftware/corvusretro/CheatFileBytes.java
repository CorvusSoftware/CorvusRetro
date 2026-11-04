// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
package com.corvussoftware.corvusretro;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.function.BooleanSupplier;

/** Raw bounded documents; UTF-8 and cheat syntax belong to the native parser. */
final class CheatFileBytes {
    static final int MAX_BYTES = 2 * 1024 * 1024;

    private CheatFileBytes() { }

    static String exportName(String title) {
        String base = title == null ? "" : title.replaceAll("[\\p{Cntrl}\\\\/:*?\"<>|]", "_").trim();
        int dot = base.lastIndexOf('.');
        if (dot > 0 && dot >= base.length() - 5) base = base.substring(0, dot);
        if (base.isEmpty() || base.equals(".") || base.equals("..")) base = "codes";
        if (base.codePointCount(0, base.length()) > 120)
            base = base.substring(0, base.offsetByCodePoints(0, 120));
        return base + ".cht";
    }

    static byte[] read(InputStream input, long expectedLength, BooleanSupplier active) throws IOException {
        if (input == null || expectedLength > MAX_BYTES) throw new IOException("Document invalide");
        ByteArrayOutputStream output = new ByteArrayOutputStream();
        byte[] buffer = new byte[32768];
        int total = 0;
        while (true) {
            checkActive(active);
            int count = input.read(buffer);
            checkActive(active);
            if (count < 0) break;
            if (count == 0 || count > MAX_BYTES - total) throw new IOException("Lecture incomplète");
            output.write(buffer, 0, count);
            total += count;
        }
        if (expectedLength >= 0 && total != expectedLength) throw new IOException("Document tronqué");
        return output.toByteArray();
    }

    static void write(OutputStream output, byte[] bytes, BooleanSupplier active) throws IOException {
        if (output == null || bytes == null || bytes.length > MAX_BYTES)
            throw new IOException("Document invalide");
        for (int offset = 0; offset < bytes.length;) {
            checkActive(active);
            int count = Math.min(32768, bytes.length - offset);
            output.write(bytes, offset, count);
            offset += count;
        }
        checkActive(active);
        output.flush();
        checkActive(active);
    }

    private static void checkActive(BooleanSupplier active) throws IOException {
        if (!active.getAsBoolean()) throw new IOException("Transfert annulé");
    }
}
