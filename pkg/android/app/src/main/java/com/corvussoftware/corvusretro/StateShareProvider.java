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
import java.io.IOException;

/** Only specifically granted, immutable cache exports can be opened, always read-only. */
public final class StateShareProvider extends ContentProvider {
    @Override public boolean onCreate() { return true; }
    private File file(Uri uri) throws FileNotFoundException {
        if (getContext()==null || !"content".equals(uri.getScheme())
                || !(getContext().getPackageName()+".state-share").equals(uri.getAuthority())
                || uri.getPathSegments().size()!=1 || uri.getQuery()!=null || uri.getFragment()!=null)
            throw new FileNotFoundException("Invalid state share");
        try { return StateShareFiles.resolve(getContext().getCacheDir(),uri.getLastPathSegment()); }
        catch (IOException unavailable) { throw new FileNotFoundException("State share unavailable"); }
    }
    @Override public ParcelFileDescriptor openFile(Uri uri,String mode) throws FileNotFoundException {
        if (!"r".equals(mode)) throw new FileNotFoundException("Read-only state share");
        return ParcelFileDescriptor.open(file(uri),ParcelFileDescriptor.MODE_READ_ONLY);
    }
    @Override public Cursor query(Uri uri,String[] projection,String selection,String[] arguments,String order) {
        try {
            File file=file(uri);
            String[] columns=projection==null ? new String[]{OpenableColumns.DISPLAY_NAME,OpenableColumns.SIZE}:projection;
            if (columns.length>16) return null;
            MatrixCursor cursor=new MatrixCursor(columns,1);Object[] values=new Object[columns.length];
            for (int i=0;i<columns.length;i++) {
                if (OpenableColumns.DISPLAY_NAME.equals(columns[i])) values[i]=StateShareFiles.displayName(file.getName());
                else if (OpenableColumns.SIZE.equals(columns[i])) values[i]=file.length();
            }
            cursor.addRow(values);return cursor;
        } catch (IOException unavailable) { return null; }
    }
    @Override public String getType(Uri uri) { return "application/octet-stream"; }
    @Override public Uri insert(Uri uri,ContentValues values) { throw new UnsupportedOperationException("Read-only state share"); }
    @Override public int update(Uri uri,ContentValues values,String selection,String[] arguments) { throw new UnsupportedOperationException("Read-only state share"); }
    @Override public int delete(Uri uri,String selection,String[] arguments) { throw new UnsupportedOperationException("Read-only state share"); }
}
