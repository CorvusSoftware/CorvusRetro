// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
package com.corvussoftware.corvusretro;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.HashSet;
import java.util.Locale;
import java.util.Set;
import java.util.function.BooleanSupplier;

/** Bounded raw battery files; neither extension denotes an emulator save state. */
final class SaveFiles {
    static final int MAX_BYTES = 1024 * 1024;
    private static final Set<File> CLEANED_ROOTS = new HashSet<>();

    private SaveFiles() { }

    static void validateName(String name) throws IOException {
        if (name == null || !(name.toLowerCase(Locale.ROOT).endsWith(".sav")
                || name.toLowerCase(Locale.ROOT).endsWith(".srm")))
            throw new IOException("Choisissez une sauvegarde .sav ou .srm");
    }

    static String exportName(String title, boolean srm) {
        String base = title == null ? "" : title.replaceAll("[\\p{Cntrl}\\\\/:*?\"<>|]", "_").trim();
        int dot = base.lastIndexOf('.');
        if (dot > 0 && dot >= base.length() - 5) base = base.substring(0, dot);
        if (base.isEmpty() || base.equals(".") || base.equals("..")) base = "sauvegarde";
        if (base.codePointCount(0, base.length()) > 120)
            base = base.substring(0, base.offsetByCodePoints(0, 120));
        return base + (srm ? ".srm" : ".sav");
    }

    static int copy(InputStream input, OutputStream output, BooleanSupplier active) throws IOException {
        byte[] buffer = new byte[32768];
        int total = 0;
        while (true) {
            if (!active.getAsBoolean()) throw new IOException("Transfert annulé");
            int count = input.read(buffer);
            if (count < 0) break;
            if (count == 0) throw new IOException("Lecture de sauvegarde interrompue");
            if (!active.getAsBoolean()) throw new IOException("Transfert annulé");
            if (count > MAX_BYTES - total)
                throw new IOException("Sauvegarde trop volumineuse (maximum 1 Mio)");
            output.write(buffer, 0, count);
            total += count;
        }
        if (total == 0) throw new IOException("La sauvegarde est vide");
        return total;
    }

    static File stage(File root, long request, InputStream input, BooleanSupplier active) throws IOException {
        if (request <= 0) throw new IOException("Transfert invalide");
        File directory = new File(root, "save-transfer");
        if ((!directory.isDirectory() && !directory.mkdirs())
                || !directory.getCanonicalFile().equals(new File(root.getCanonicalFile(), "save-transfer")))
            throw new IOException("Stockage temporaire indisponible");
        synchronized (CLEANED_ROOTS) {
            if (!CLEANED_ROOTS.contains(directory.getCanonicalFile())) {
                // Only the first import in this process removes stages left by a killed process.
                File[] previous = directory.listFiles();
                if (previous == null) throw new IOException("Stockage temporaire illisible");
                for (File file : previous)
                    if (file.getName().matches("[1-9][0-9]{0,18}\\.sav") && !file.delete())
                        throw new IOException("Ancien transfert impossible à nettoyer");
                CLEANED_ROOTS.add(directory.getCanonicalFile());
            }
        }
        File file = new File(directory, request + ".sav");
        if (!file.createNewFile()) throw new IOException("Transfert temporaire déjà présent");
        boolean ready = false;
        try {
            try (FileOutputStream output = new FileOutputStream(file)) {
                copy(input, output, active);
                output.getFD().sync();
            }
            if (!file.setReadOnly()) throw new IOException("Protection du transfert impossible");
            ready = true;
            return file;
        } finally {
            if (!ready && !file.delete()) file.deleteOnExit();
        }
    }
}
