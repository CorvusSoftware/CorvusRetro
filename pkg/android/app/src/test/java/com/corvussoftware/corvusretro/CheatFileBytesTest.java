// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
package com.corvussoftware.corvusretro;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.concurrent.atomic.AtomicInteger;
import org.junit.Test;
import static org.junit.Assert.*;

public final class CheatFileBytesTest {
    private interface Failure { void run() throws IOException; }
    private static void rejects(Failure action) throws IOException {
        try { action.run(); fail("Invalid document accepted"); }
        catch (IOException expected) { }
    }

    @Test public void preservesRawBytesAtTheLimitAndLeavesFormatValidationToNative() throws Exception {
        byte[] bytes = new byte[CheatFileBytes.MAX_BYTES];
        for (int i = 0; i < bytes.length; i++) bytes[i] = (byte) (i * 31);
        assertArrayEquals(bytes, CheatFileBytes.read(new ByteArrayInputStream(bytes), bytes.length, () -> true));
        assertArrayEquals(new byte[0], CheatFileBytes.read(new ByteArrayInputStream(new byte[0]), 0, () -> true));
        ByteArrayOutputStream output = new ByteArrayOutputStream();
        CheatFileBytes.write(output, bytes, () -> true);
        assertArrayEquals(bytes, output.toByteArray());
    }

    @Test public void rejectsOversizeAndTruncatedProviderDocuments() throws Exception {
        rejects(() -> CheatFileBytes.read(null, -1, () -> true));
        rejects(() -> CheatFileBytes.read(new ByteArrayInputStream(new byte[1]), 2, () -> true));
        rejects(() -> CheatFileBytes.read(new ByteArrayInputStream(new byte[2]), 1, () -> true));
        rejects(() -> CheatFileBytes.read(new ByteArrayInputStream(new byte[0]),
                (long) CheatFileBytes.MAX_BYTES + 1, () -> true));
        rejects(() -> CheatFileBytes.read(new ByteArrayInputStream(new byte[CheatFileBytes.MAX_BYTES + 1]),
                -1, () -> true));
        ByteArrayOutputStream output = new ByteArrayOutputStream();
        rejects(() -> CheatFileBytes.write(output, new byte[CheatFileBytes.MAX_BYTES + 1], () -> true));
        assertEquals(0, output.size());
    }

    @Test public void acceptsUnknownLengthButRejectsStalledOrFailedReads() throws Exception {
        byte[] bytes = {0, -1, -2, 42};
        assertArrayEquals(bytes, CheatFileBytes.read(new ByteArrayInputStream(bytes), -1, () -> true));
        rejects(() -> CheatFileBytes.read(new InputStream() {
            @Override public int read() { return 0; }
            @Override public int read(byte[] buffer, int offset, int length) { return 0; }
        }, -1, () -> true));
        rejects(() -> CheatFileBytes.read(new InputStream() {
            @Override public int read() throws IOException { throw new IOException("Provider disconnected"); }
        }, -1, () -> true));
    }

    @Test public void cancellationDiscardsReadAndStopsWritesBetweenChunks() throws Exception {
        AtomicInteger checks = new AtomicInteger();
        rejects(() -> CheatFileBytes.read(new ByteArrayInputStream(new byte[] {1}), -1,
                () -> checks.incrementAndGet() < 2));
        rejects(() -> CheatFileBytes.read(new ByteArrayInputStream(new byte[0]), -1, () -> false));
        ByteArrayOutputStream output = new ByteArrayOutputStream();
        checks.set(0);
        rejects(() -> CheatFileBytes.write(output, new byte[65536], () -> checks.incrementAndGet() < 2));
        assertEquals(32768, output.size());
        rejects(() -> CheatFileBytes.write(output, new byte[0], () -> false));
    }

    @Test public void writeFailuresPropagateAndSuccessFlushes() throws Exception {
        rejects(() -> CheatFileBytes.write(new ByteArrayOutputStream() {
            @Override public void flush() throws IOException { throw new IOException("Provider disconnected"); }
        }, new byte[] {1}, () -> true));
        AtomicInteger flushes = new AtomicInteger();
        CheatFileBytes.write(new ByteArrayOutputStream() {
            @Override public void flush() { flushes.incrementAndGet(); }
        }, new byte[0], () -> true);
        assertEquals(1, flushes.get());
    }

    @Test public void exportNamesAreSafeBoundedAndKeepUnicodeCharactersWhole() {
        assertEquals("Partie.cht", CheatFileBytes.exportName("Partie.gba"));
        assertEquals("Partie.cht", CheatFileBytes.exportName("Partie.CHT"));
        assertEquals("a_b_c_.cht", CheatFileBytes.exportName("a/b\\c\n"));
        for (String title : new String[] {null, "", ".", ".."})
            assertEquals("codes.cht", CheatFileBytes.exportName(title));
        String name = CheatFileBytes.exportName("🪶".repeat(150));
        assertEquals(124, name.codePointCount(0, name.length()));
        assertFalse(Character.isHighSurrogate(name.charAt(name.length() - 5)));
    }
}
