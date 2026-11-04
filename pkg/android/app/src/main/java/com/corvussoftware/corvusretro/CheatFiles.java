// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
package com.corvussoftware.corvusretro;

import android.app.Activity;
import android.content.ContentResolver;
import android.content.Intent;
import android.content.res.AssetFileDescriptor;
import android.net.Uri;
import android.os.CancellationSignal;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.concurrent.Executor;
import java.util.concurrent.RejectedExecutionException;

/** SAF and document IO only. Native code owns parsing and applying cheat lists. */
final class CheatFiles {
    static final int REQUEST_CODE = 13;

    interface Host {
        boolean alive();
        boolean canPick();
        void pick(Intent intent, int requestCode);
        void result(long request, byte[] bytes, String error, boolean cancelled);
    }

    private static final class Session {
        final long request;
        final boolean export;
        final byte[] snapshot;
        final CancellationSignal signal = new CancellationSignal();
        boolean delivered;

        Session(long request, boolean export, byte[] snapshot) {
            this.request = request;
            this.export = export;
            this.snapshot = snapshot;
        }
    }

    private final Object lock = new Object();
    private final ContentResolver resolver;
    private final Executor executor;
    private final Host host;
    private Session pending, picker;
    private long finishedThrough;
    private boolean closed;

    CheatFiles(ContentResolver resolver, Executor executor, Host host) {
        this.resolver = resolver;
        this.executor = executor;
        this.host = host;
    }

    boolean busy() { synchronized (lock) { return pending != null || picker != null; } }

    // Requests and picker results use the UI thread; finish and results may use another thread.
    void request(long request, boolean export, String title, byte[] snapshot) {
        Session session;
        boolean canPick = host.canPick();
        synchronized (lock) {
            if (closed || !host.alive() || request <= finishedThrough) return;
            if (pending != null && pending.request == request) return;
            if (pending != null || picker != null || !canPick) {
                host.result(request, new byte[0], "Terminez le sélecteur de fichiers déjà ouvert", false);
                return;
            }
            if (export && (snapshot == null || snapshot.length > CheatFileBytes.MAX_BYTES)) {
                host.result(request, new byte[0], "Liste de codes impossible à exporter", false);
                return;
            }
            session = new Session(request, export, export ? snapshot.clone() : null);
            pending = picker = session;
        }
        Intent intent = new Intent(export ? Intent.ACTION_CREATE_DOCUMENT : Intent.ACTION_OPEN_DOCUMENT)
                .addCategory(Intent.CATEGORY_OPENABLE).setType(export ? "text/plain" : "*/*");
        if (export) intent.putExtra(Intent.EXTRA_TITLE, CheatFileBytes.exportName(title));
        else intent.putExtra(Intent.EXTRA_MIME_TYPES, new String[] {"text/plain", "application/octet-stream"});
        try {
            synchronized (lock) {
                if (!active(session)) {
                    if (picker == session) picker = null;
                    return;
                }
                host.pick(intent, REQUEST_CODE);
            }
        } catch (RuntimeException error) {
            synchronized (lock) { if (picker == session) picker = null; }
            complete(session, new byte[0], "Sélecteur de documents indisponible", false);
        }
    }

    boolean activityResult(int requestCode, int resultCode, Intent data) {
        if (requestCode != REQUEST_CODE) return false;
        Session session;
        synchronized (lock) { session = picker; picker = null; }
        if (session == null || !active(session)) return true;
        if (resultCode != Activity.RESULT_OK) {
            complete(session, new byte[0], "", true);
            return true;
        }
        Uri uri = data == null ? null : data.getData();
        if (uri == null || !"content".equals(uri.getScheme())) {
            complete(session, new byte[0], "Document de codes non reconnu", false);
            return true;
        }
        try {
            executor.execute(() -> transfer(session, uri));
        } catch (RejectedExecutionException error) {
            complete(session, new byte[0], "Transfert de codes interrompu", false);
        }
        return true;
    }

    private boolean active(Session session) {
        synchronized (lock) { return !closed && pending == session && !session.delivered && host.alive(); }
    }

    private void transfer(Session session, Uri uri) {
        try {
            if (!active(session)) return;
            byte[] bytes = new byte[0];
            try (AssetFileDescriptor document = resolver.openAssetFileDescriptor(
                    uri, session.export ? "wt" : "r", session.signal)) {
                if (document == null) throw new IOException("Document absent");
                if (session.export) {
                    try (OutputStream output = document.createOutputStream()) {
                        CheatFileBytes.write(output, session.snapshot, () -> active(session));
                    }
                } else {
                    try (InputStream input = document.createInputStream()) {
                        bytes = CheatFileBytes.read(input, document.getLength(), () -> active(session));
                    }
                }
            }
            complete(session, bytes, "", false);
        } catch (IOException | RuntimeException error) {
            complete(session, new byte[0], session.export
                    ? "Export incomplet : vérifiez le document de destination"
                    : "Import impossible : vérifiez le fichier .cht et sa taille (2 Mio maximum)", false);
        }
    }

    private void complete(Session session, byte[] bytes, String error, boolean cancelled) {
        synchronized (lock) {
            if (!active(session)) return;
            session.delivered = true;
            host.result(session.request, bytes, error, cancelled);
        }
    }

    void finish(long request) {
        Session session;
        synchronized (lock) {
            finishedThrough = Math.max(finishedThrough, request);
            session = pending != null && pending.request == request ? pending : null;
            if (session != null) pending = null;
            // An outstanding OS picker must consume its original result before another can open.
        }
        if (session != null) session.signal.cancel();
    }

    void close() {
        Session session;
        synchronized (lock) { closed = true; session = pending; pending = picker = null; }
        if (session != null) session.signal.cancel();
    }
}
