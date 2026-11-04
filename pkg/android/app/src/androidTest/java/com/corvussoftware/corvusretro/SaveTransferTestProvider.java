// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
package com.corvussoftware.corvusretro;

import android.content.ContentProvider;
import android.content.ContentValues;
import android.database.Cursor;
import android.database.MatrixCursor;
import android.net.Uri;
import android.os.ParcelFileDescriptor;
import android.provider.OpenableColumns;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.util.Arrays;

/** Test APK only: fixed synthetic battery files, never application saves or ROMs. */
public final class SaveTransferTestProvider extends ContentProvider {
    static final String AUTHORITY = "com.corvussoftware.corvusretro.test.saves";
    private File directory;

    static byte[] battery() {
        byte[] bytes = new byte[8192];
        for (int i = 0; i < bytes.length; i++) bytes[i] = (byte) (i * 31 + 7);
        return bytes;
    }

    @Override public boolean onCreate() {
        directory = new File(getContext().getCacheDir(), "save-document-fixtures");
        return directory.isDirectory() || directory.mkdirs();
    }

    private String name(Uri uri) {
        if (!AUTHORITY.equals(uri.getAuthority()) || uri.getPathSegments().size() != 1)
            throw new IllegalArgumentException("Unknown synthetic document");
        String name = uri.getLastPathSegment();
        if (!Arrays.asList("battery.sav", "battery.SRM", "wrong.state", "empty.sav", "large.sav",
                "output.sav", "output.srm", "codes.cht", "output.cht").contains(name))
            throw new IllegalArgumentException("Unknown synthetic document");
        return name;
    }

    @Override public Cursor query(Uri uri, String[] projection, String selection, String[] args, String order) {
        String name = name(uri);
        MatrixCursor cursor = new MatrixCursor(new String[] {OpenableColumns.DISPLAY_NAME});
        cursor.addRow(new Object[] {name});
        return cursor;
    }

    @Override public String getType(Uri uri) { name(uri); return "application/octet-stream"; }

    @Override public ParcelFileDescriptor openFile(Uri uri, String mode) throws FileNotFoundException {
        String name = name(uri);
        boolean output = name.startsWith("output.");
        if ((!"r".equals(mode) && !"w".equals(mode) && !"wt".equals(mode)) || (!output && !"r".equals(mode)))
            throw new FileNotFoundException("Unsupported fixture access");
        File file = new File(directory, name);
        if (!output) {
            try (FileOutputStream stream = new FileOutputStream(file)) {
                if ("codes.cht".equals(name)) stream.write(("cheats = 1\ncheat0_desc = \"Test triche\"\ncheat0_code = \"32000000 0042\"\ncheat0_enable = true\ncheat0_handler = 0\n").getBytes(java.nio.charset.StandardCharsets.UTF_8));
                else if ("large.sav".equals(name)) stream.write(new byte[1024 * 1024 + 1]);
                else if (!"empty.sav".equals(name)) stream.write(battery());
            } catch (IOException failure) { throw new FileNotFoundException("Fixture unavailable"); }
        }
        int flags = "r".equals(mode) ? ParcelFileDescriptor.MODE_READ_ONLY
                : ParcelFileDescriptor.MODE_CREATE | ParcelFileDescriptor.MODE_WRITE_ONLY;
        // A document provider is permitted to leave an existing file's tail intact for "w".
        if ("wt".equals(mode)) flags |= ParcelFileDescriptor.MODE_TRUNCATE;
        return ParcelFileDescriptor.open(file, flags);
    }

    @Override public Uri insert(Uri uri, ContentValues values) { throw new UnsupportedOperationException(); }
    @Override public int delete(Uri uri, String selection, String[] args) { throw new UnsupportedOperationException(); }
    @Override public int update(Uri uri, ContentValues values, String selection, String[] args) { throw new UnsupportedOperationException(); }
}
