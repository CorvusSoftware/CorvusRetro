// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
package com.corvussoftware.corvusretro;

import android.app.Activity;
import android.content.ContentResolver;
import android.content.Intent;
import android.content.res.AssetFileDescriptor;
import android.database.Cursor;
import android.net.Uri;
import android.os.CancellationSignal;
import android.provider.OpenableColumns;
import java.io.ByteArrayInputStream;
import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.concurrent.Executor;
import java.util.concurrent.RejectedExecutionException;

/** Android document selection and IO only. The native owner confirms and commits imports. */
final class SaveTransfers {
    static final int REQUEST_CODE = 12;
    interface Host {
        boolean alive();
        boolean canPick();
        void pick(Intent intent, int code);
        void result(long request, String path, String error, boolean cancelled);
    }
    private static final class Session {
        final long request;
        final int operation;
        final byte[] snapshot;
        final CancellationSignal signal = new CancellationSignal();
        File stage;
        Session(long request, int operation, byte[] snapshot) {
            this.request = request; this.operation = operation; this.snapshot = snapshot;
        }
    }
    private final Object lock = new Object();
    private final ContentResolver resolver;
    private final File root;
    private final Executor executor;
    private final Host host;
    private Session pending, picker;
    private long finishedThrough;
    private boolean closed;

    SaveTransfers(ContentResolver resolver, File root, Executor executor, Host host) {
        this.resolver = resolver; this.root = root; this.executor = executor; this.host = host;
    }

    boolean busy() { synchronized (lock) { return pending != null || picker != null; } }

    // Called on Android's UI thread. Results and finish may arrive on the worker/native thread.
    void request(long request, int operation, String title, byte[] snapshot) {
        Session session;
        boolean canPick = host.canPick();
        synchronized (lock) {
            if (closed || !host.alive() || request <= finishedThrough) return;
            if (request <= 0 || operation < 0 || operation > 2) {
                host.result(request, "", "Transfert de sauvegarde invalide", false); return;
            }
            if (pending != null || picker != null || !canPick) {
                host.result(request, "", "Terminez le sélecteur de fichiers déjà ouvert", false); return;
            }
            if (operation != 0 && (snapshot == null || snapshot.length == 0 || snapshot.length > SaveFiles.MAX_BYTES)) {
                host.result(request, "", "Aucune sauvegarde compatible à exporter", false); return;
            }
            session = new Session(request, operation, operation == 0 ? null : snapshot.clone());
            pending = picker = session;
        }
        Intent intent = new Intent(operation == 0 ? Intent.ACTION_OPEN_DOCUMENT : Intent.ACTION_CREATE_DOCUMENT)
                .addCategory(Intent.CATEGORY_OPENABLE)
                .setType(operation == 0 ? "*/*" : "application/octet-stream");
        if (operation != 0) intent.putExtra(Intent.EXTRA_TITLE, SaveFiles.exportName(title, operation == 2));
        try {
            host.pick(intent, REQUEST_CODE);
        } catch (RuntimeException error) {
            synchronized (lock) { if (picker == session) picker = null; }
            complete(session, "", "Sélecteur de documents indisponible", false);
        }
    }

    boolean activityResult(int code, int result, Intent data) {
        if (code != REQUEST_CODE) return false;
        Session session;
        synchronized (lock) { session = picker; picker = null; }
        if (session == null || !active(session)) return true;
        if (result != Activity.RESULT_OK || data == null || data.getData() == null) {
            complete(session, "", "", true); return true;
        }
        Uri uri = data.getData();
        if (!"content".equals(uri.getScheme())) {
            complete(session, "", "Document de sauvegarde non reconnu", false); return true;
        }
        try {
            executor.execute(() -> transfer(session, uri));
        } catch (RejectedExecutionException error) {
            complete(session, "", "Transfert de sauvegarde interrompu", false);
        }
        return true;
    }

    private boolean active(Session session) {
        synchronized (lock) { return !closed && pending == session && host.alive(); }
    }

    private String displayName(Uri uri) throws IOException {
        try (Cursor cursor = resolver.query(uri, new String[] {OpenableColumns.DISPLAY_NAME}, null, null, null)) {
            if (cursor != null && cursor.moveToFirst()) {
                int column = cursor.getColumnIndex(OpenableColumns.DISPLAY_NAME);
                if (column >= 0) return cursor.getString(column);
            }
        }
        throw new IOException("Nom de sauvegarde indisponible");
    }

    private void transfer(Session session, Uri uri) {
        File stage = null;
        boolean delivered = false;
        try {
            if (!active(session)) return;
            if (session.operation == 0) {
                SaveFiles.validateName(displayName(uri));
                try (AssetFileDescriptor document = resolver.openAssetFileDescriptor(uri, "r", session.signal)) {
                    if (document == null) throw new IOException("Document absent");
                    try (InputStream input = document.createInputStream()) {
                        stage = SaveFiles.stage(root, session.request, input, () -> active(session));
                    }
                }
                synchronized (lock) {
                    if (!active(session)) return;
                    session.stage = stage;
                    delivered = true;
                    complete(session, stage.getAbsolutePath(), "", false);
                }
            } else {
                try (AssetFileDescriptor document = resolver.openAssetFileDescriptor(uri, "wt", session.signal)) {
                    if (document == null) throw new IOException("Destination absente");
                    try (OutputStream output = document.createOutputStream()) {
                        SaveFiles.copy(new ByteArrayInputStream(session.snapshot), output, () -> active(session));
                        output.flush();
                    }
                }
                complete(session, "", "", false);
            }
        } catch (IOException | RuntimeException error) {
            complete(session, "", session.operation == 0
                    ? "Import impossible : vérifiez le fichier .sav ou .srm et sa taille (1 Mio maximum)"
                    : "Export incomplet : vérifiez le document de destination", false);
        } finally {
            if (!delivered && stage != null) delete(stage);
        }
    }

    private void complete(Session session, String path, String error, boolean cancelled) {
        synchronized (lock) {
            if (active(session)) host.result(session.request, path, error, cancelled);
        }
    }

    void finish(long request) {
        Session session;
        synchronized (lock) {
            finishedThrough = Math.max(finishedThrough, request);
            session = pending != null && pending.request == request ? pending : null;
            if (session != null) pending = null;
            // Keep a cancelled picker bound to its original request until its OS result arrives.
        }
        if (session != null) {
            session.signal.cancel();
            if (session.stage != null) deleteLater(session.stage);
        }
    }

    void close() {
        Session session;
        synchronized (lock) { closed = true; session = pending; pending = picker = null; }
        if (session != null) {
            session.signal.cancel();
            if (session.stage != null) deleteLater(session.stage);
        }
    }

    private void deleteLater(File file) {
        try { executor.execute(() -> delete(file)); }
        catch (RejectedExecutionException error) { delete(file); }
    }
    private static void delete(File file) { if (file.exists() && !file.delete()) file.deleteOnExit(); }
}
