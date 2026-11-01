// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
package com.corvussoftware.corvusretro;

import android.app.Activity;
import android.app.Instrumentation;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.system.Os;
import android.system.OsConstants;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.InputStream;
import java.util.ArrayDeque;
import java.util.Arrays;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.Executor;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.TimeUnit;

/** Real document-provider IO; the host substitutes only Android picker UI and native callbacks. */
public final class SaveTransfersInstrumentation extends Instrumentation {
    private int checks, roots;
    private static final class Result {
        final long request;
        final String path, error;
        final boolean cancelled;
        Result(long request, String path, String error, boolean cancelled) {
            this.request = request; this.path = path; this.error = error; this.cancelled = cancelled;
        }
    }
    private static final class Host implements SaveTransfers.Host {
        final BlockingQueue<Result> results = new LinkedBlockingQueue<>();
        Intent intent;
        int picks;
        boolean alive = true, canPick = true;
        @Override public boolean alive() { return alive; }
        @Override public boolean canPick() { return canPick; }
        @Override public void pick(Intent intent, int code) {
            if (code != SaveTransfers.REQUEST_CODE) throw new AssertionError("picker request code");
            this.intent = intent; picks++;
        }
        @Override public void result(long request, String path, String error, boolean cancelled) {
            results.add(new Result(request, path, error, cancelled));
        }
        Result next() throws Exception {
            Result result = results.poll(10, TimeUnit.SECONDS);
            if (result == null) throw new AssertionError("Missing transfer completion");
            return result;
        }
    }
    private static final class Deferred implements Executor {
        final ArrayDeque<Runnable> queue = new ArrayDeque<>();
        @Override public void execute(Runnable job) { queue.add(job); }
        void drain() { while (!queue.isEmpty()) queue.remove().run(); }
    }

    private void check(boolean condition, String message) {
        if (!condition) throw new AssertionError(message);
        checks++;
    }
    private File root() {
        File root = new File(getTargetContext().getCacheDir(), "save-transfer-test-" + System.nanoTime() + "-" + roots++);
        if (!root.mkdirs()) throw new AssertionError("Isolated transfer storage unavailable");
        return root;
    }
    private SaveTransfers service(File root, Executor executor, Host host) {
        return new SaveTransfers(getTargetContext().getContentResolver(), root, executor, host);
    }
    private static Uri document(String name) {
        return Uri.parse("content://" + SaveTransferTestProvider.AUTHORITY + "/" + name);
    }
    private static Intent selected(String name) { return new Intent().setData(document(name)); }
    private static byte[] bytes(InputStream input) throws Exception {
        try (InputStream owned = input; ByteArrayOutputStream output = new ByteArrayOutputStream()) {
            byte[] buffer = new byte[4096]; int count;
            while ((count = owned.read(buffer)) >= 0) output.write(buffer, 0, count);
            return output.toByteArray();
        }
    }
    private void drained(ExecutorService executor) throws Exception { executor.submit(() -> { }).get(10, TimeUnit.SECONDS); }
    private static void remove(File file) {
        if (file.isDirectory()) {
            File[] children = file.listFiles();
            if (children != null) for (File child : children) remove(child);
        }
        file.delete();
    }

    private void exportsAndImports() throws Exception {
        File root = root(); Host host = new Host(); ExecutorService io = Executors.newSingleThreadExecutor();
        SaveTransfers service = service(root, io, host);
        try {
            for (int operation = 1; operation <= 2; operation++) {
                String extension = operation == 1 ? ".sav" : ".srm";
                byte[] original = SaveTransferTestProvider.battery(), snapshot = original.clone();
                try (java.io.OutputStream previous = getTargetContext().getContentResolver().openOutputStream(document("output" + extension), "wt")) {
                    previous.write(new byte[original.length * 2]);
                }
                service.request(operation, operation, "Corvus test.gb", snapshot);
                Arrays.fill(snapshot, (byte) 0);
                check(Intent.ACTION_CREATE_DOCUMENT.equals(host.intent.getAction())
                        && host.intent.hasCategory(Intent.CATEGORY_OPENABLE), "export uses a document destination");
                check(("Corvus test" + extension).equals(host.intent.getStringExtra(Intent.EXTRA_TITLE)), "suggested export extension");
                service.activityResult(SaveTransfers.REQUEST_CODE, Activity.RESULT_OK, selected("output" + extension));
                Result done = host.next();
                check(done.request == operation && done.error.isEmpty() && !done.cancelled && done.path.isEmpty(), "export completes only after document write");
                check(Arrays.equals(original, bytes(getTargetContext().getContentResolver().openInputStream(document("output" + extension)))),
                        "export contains exactly the captured raw battery, without old destination bytes or later snapshot mutations");
                service.finish(operation); drained(io);
            }
            for (int slot = 0; slot < 2; slot++) {
                long request = 3 + slot;
                service.request(request, 0, "", null);
                check(Intent.ACTION_OPEN_DOCUMENT.equals(host.intent.getAction()) && "*/*".equals(host.intent.getType()),
                        "raw save picker accepts providers with generic binary MIME types");
                service.activityResult(SaveTransfers.REQUEST_CODE, Activity.RESULT_OK, selected(slot == 0 ? "battery.sav" : "battery.SRM"));
                Result done = host.next(); File staged = new File(done.path);
                check(done.request == request && done.error.isEmpty() && !done.cancelled,
                        "both SAV and case-insensitive SRM report the original transfer identity");
                check(staged.getAbsolutePath().equals(new File(root, "save-transfer/" + request + ".sav").getAbsolutePath())
                        && Arrays.equals(SaveTransferTestProvider.battery(), bytes(new FileInputStream(staged))), "native receives exact private raw battery stage");
                check((Os.stat(staged.getAbsolutePath()).st_mode & (OsConstants.S_IWUSR | OsConstants.S_IWGRP | OsConstants.S_IWOTH)) == 0,
                        "completed imports are read-only before native validation");
                service.activityResult(SaveTransfers.REQUEST_CODE, Activity.RESULT_OK, selected("battery.sav"));
                drained(io);
                check(host.results.isEmpty(), "duplicate picker completion cannot apply an import twice");
                service.finish(request); drained(io);
                check(!staged.exists() && !service.busy(), "native completion removes only its staging file");
            }
        } finally { service.close(); drained(io); io.shutdown(); remove(root); }
    }

    private void rejectedDocuments() throws Exception {
        File root = root(); Host host = new Host(); ExecutorService io = Executors.newSingleThreadExecutor();
        SaveTransfers service = service(root, io, host);
        try {
            long request = 10;
            for (String name : new String[] {"wrong.state", "empty.sav", "large.sav"}) {
                service.request(request, 0, "", null);
                service.activityResult(SaveTransfers.REQUEST_CODE, Activity.RESULT_OK, selected(name));
                Result failure = host.next();
                check(failure.request == request && failure.path.isEmpty() && !failure.error.isEmpty() && !failure.cancelled,
                        "wrong extension, empty and oversized files report errors without an import stage");
                check(!new File(root, "save-transfer/" + request + ".sav").exists(), "failed copies leave no partial stage");
                service.finish(request++); drained(io);
            }
            service.request(request, 0, "", null);
            service.activityResult(SaveTransfers.REQUEST_CODE, Activity.RESULT_OK, new Intent().setData(Uri.parse("file:///not-a-document.sav")));
            check(!host.next().error.isEmpty(), "foreign URI scheme never opens app or filesystem paths");
            service.finish(request++);
            service.request(request, 0, "", null);
            service.activityResult(SaveTransfers.REQUEST_CODE, Activity.RESULT_CANCELED, null);
            Result cancelled = host.next();
            check(cancelled.request == request && cancelled.cancelled && cancelled.path.isEmpty() && cancelled.error.isEmpty(), "picker cancellation is distinct from IO failure");
            service.finish(request); drained(io);
        } finally { service.close(); drained(io); io.shutdown(); remove(root); }
    }

    private void cancelledAndStaleResults() throws Exception {
        File root = root(); Host host = new Host(); Deferred io = new Deferred();
        SaveTransfers service = service(root, io, host);
        try {
            service.finish(20);
            service.request(20, 0, "", null);
            check(host.picks == 0 && host.results.isEmpty(), "native cancellation arriving before queued UI work prevents a stale picker");
            service.request(21, 0, "", null);
            check(!service.activityResult(999, Activity.RESULT_CANCELED, null) && service.busy(),
                    "unrelated activity results leave the current transfer bound");
            service.finish(21);
            check(service.busy(), "cancelled Android picker retains its original result binding");
            service.request(22, 0, "", null);
            check(host.next().request == 22 && host.picks == 1, "second transfer cannot consume an older picker result");
            service.finish(22);
            service.activityResult(SaveTransfers.REQUEST_CODE, Activity.RESULT_OK, selected("battery.sav"));
            io.drain();
            check(host.results.isEmpty() && !service.busy(), "late cancelled picker creates no native callback or stage");
            service.request(23, 0, "", null);
            service.activityResult(SaveTransfers.REQUEST_CODE, Activity.RESULT_OK, selected("battery.sav"));
            service.finish(23); io.drain();
            check(host.results.isEmpty() && !new File(root, "save-transfer/23.sav").exists(), "native cancellation before queued IO cannot resurrect an import");
            service.request(24, 0, "", null);
            service.activityResult(SaveTransfers.REQUEST_CODE, Activity.RESULT_OK, selected("battery.sav"));
            io.drain(); Result current = host.next(); File stage = new File(current.path);
            service.finish(23); io.drain();
            check(current.request == 24 && stage.exists() && service.busy(), "older completion cannot delete a newer player's stage");
            service.finish(24); io.drain();
            check(!stage.exists(), "matching native completion cleans the current stage");
            service.request(25, 0, "", null);
            service.activityResult(SaveTransfers.REQUEST_CODE, Activity.RESULT_OK, selected("battery.sav"));
            service.close(); io.drain();
            check(host.results.isEmpty() && !new File(root, "save-transfer/25.sav").exists(), "activity destruction discards queued IO and native callbacks");
            int picks = host.picks;
            service.request(26, 0, "", null);
            check(host.picks == picks && !service.busy(), "destroyed service rejects new work");
        } finally { service.close(); io.drain(); remove(root); }
    }

    @Override public void onCreate(Bundle arguments) { super.onCreate(arguments); start(); }
    @Override public void onStart() {
        Bundle result = new Bundle();
        try {
            exportsAndImports(); rejectedDocuments(); cancelledAndStaleResults();
            result.putString("stream", "PASS " + checks + " raw save document/IO/cancellation/lifecycle checks\n");
            finish(Activity.RESULT_OK, result);
        } catch (Throwable failure) {
            result.putString("stream", "FAIL " + android.util.Log.getStackTraceString(failure) + "\n");
            finish(Activity.RESULT_CANCELED, result);
        }
    }
}
