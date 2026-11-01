// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
package com.corvussoftware.corvusretro;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.concurrent.atomic.AtomicInteger;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.TemporaryFolder;
import static org.junit.Assert.*;

public final class SaveFilesTest {
    @Rule public TemporaryFolder temporary = new TemporaryFolder();

    private interface Failure { void run() throws IOException; }
    private static void rejects(Failure action) throws IOException {
        try { action.run(); fail("Invalid save accepted"); }
        catch (IOException expected) { }
    }

    @Test public void rawBatteryNamesAndSafeUnicodeExportNames() throws Exception {
        SaveFiles.validateName("Partie.SAV");
        SaveFiles.validateName("Partie.SrM");
        for (String name : new String[] {null, "", "game.state", "game.sav.zip"})
            rejects(() -> SaveFiles.validateName(name));
        assertEquals("Partie.sav", SaveFiles.exportName("Partie.gba", false));
        assertEquals("Joueur 2.srm", SaveFiles.exportName("Joueur 2", true));
        assertEquals("a_b_c_.sav", SaveFiles.exportName("a/b\\c\n", false));
        assertEquals("sauvegarde.sav", SaveFiles.exportName("..", false));
        String name = SaveFiles.exportName("🪶".repeat(150), true);
        assertEquals(124, name.codePointCount(0, name.length()));
        assertFalse(Character.isHighSurrogate(name.charAt(name.length() - 5)));
    }

    @Test public void rawBytesPreservedAndLimitsEnforced() throws Exception {
        byte[] bytes = new byte[SaveFiles.MAX_BYTES];
        for (int i = 0; i < bytes.length; i++) bytes[i] = (byte) (i * 31);
        ByteArrayOutputStream output = new ByteArrayOutputStream();
        assertEquals(bytes.length, SaveFiles.copy(new ByteArrayInputStream(bytes), output, () -> true));
        assertArrayEquals(bytes, output.toByteArray());
        rejects(() -> SaveFiles.copy(new ByteArrayInputStream(new byte[0]), output, () -> true));
        ByteArrayOutputStream overflow = new ByteArrayOutputStream();
        rejects(() -> SaveFiles.copy(new ByteArrayInputStream(new byte[SaveFiles.MAX_BYTES + 1]), overflow, () -> true));
        assertTrue(overflow.size() <= SaveFiles.MAX_BYTES);
        AtomicInteger checks = new AtomicInteger();
        ByteArrayOutputStream cancelled = new ByteArrayOutputStream();
        rejects(() -> SaveFiles.copy(new ByteArrayInputStream(bytes), cancelled, () -> checks.incrementAndGet() < 2));
        assertEquals(0, cancelled.size());
    }

    @Test public void stagesAreExclusiveAndFailureNeverLeavesPartialFiles() throws Exception {
        File root = temporary.newFolder();
        File directory = new File(root, "save-transfer");
        assertTrue(directory.mkdir());
        Files.write(new File(directory, "17.sav").toPath(), new byte[] {99});
        Files.write(new File(directory, "unrelated.txt").toPath(), new byte[] {88});
        byte[] bytes = {0, 1, -1, 42};
        File stage = SaveFiles.stage(root, 18, new ByteArrayInputStream(bytes), () -> true);
        assertArrayEquals(bytes, Files.readAllBytes(stage.toPath()));
        assertFalse(new File(directory, "17.sav").exists());
        assertTrue(new File(directory, "unrelated.txt").exists());
        rejects(() -> SaveFiles.stage(root, 18, new ByteArrayInputStream(new byte[] {55}), () -> true));
        assertArrayEquals(bytes, Files.readAllBytes(stage.toPath()));
        rejects(() -> SaveFiles.stage(root, 19, new ByteArrayInputStream(bytes), () -> false));
        assertFalse(new File(directory, "19.sav").exists());
        rejects(() -> SaveFiles.stage(root, 20, new ByteArrayInputStream(new byte[0]), () -> true));
        assertFalse(new File(directory, "20.sav").exists());
        // A later live request must not clean another request's staged bytes.
        File second = SaveFiles.stage(root, 21, new ByteArrayInputStream(bytes), () -> true);
        assertTrue(stage.exists());
        assertTrue(second.exists());
    }

    @Test public void stagingRejectsSymlinkDirectoryAndAcceptsTrustedRootAlias() throws Exception {
        File root = temporary.newFolder();
        File elsewhere = temporary.newFolder();
        Path link = new File(root, "save-transfer").toPath();
        Files.createSymbolicLink(link, elsewhere.toPath());
        rejects(() -> SaveFiles.stage(root, 1, new ByteArrayInputStream(new byte[] {1}), () -> true));
        assertEquals(0, elsewhere.list().length);
        Files.delete(link);
        Path alias = temporary.getRoot().toPath().resolve("root-alias");
        Files.createSymbolicLink(alias, root.toPath());
        File stage = SaveFiles.stage(alias.toFile(), 2, new ByteArrayInputStream(new byte[] {2}), () -> true);
        assertEquals(new File(alias.toFile(), "save-transfer/2.sav").getAbsolutePath(), stage.getAbsolutePath());
        assertArrayEquals(new byte[] {2}, Files.readAllBytes(stage.toPath()));
    }
}
