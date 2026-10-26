// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
package com.corvussoftware.corvusretro;

import android.app.Activity;
import android.app.Instrumentation;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageInfo;
import android.content.pm.PackageInstaller;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.Bundle;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.lang.reflect.Method;
import java.util.Arrays;
import org.json.JSONArray;
import org.json.JSONObject;

/** Framework-only tests: no network, downloaded executable or private platform API. */
public final class CoreRepositoryInstrumentation extends Instrumentation {
    private int checks;
    private String phase;

    private void check(boolean condition, String message) {
        if (!condition) throw new AssertionError(message);
        checks++;
    }

    private JSONObject core(String id, String abi, long code) throws Exception {
        String split = "core" + id.substring("corvus.".length());
        return new JSONObject().put("id", id).put("name", "Official test core")
                .put("description", "Synthetic catalog fixture").put("version", "0.14.0")
                .put("abi", abi).put("sha256", "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa").put("size", 42)
                .put("apk_url", "https://github.com/CorvusSoftware/CorvusRetro/releases/download/test/" + split + ".apk")
                .put("split_name", split).put("base_version_code", code);
    }

    private String feed(JSONObject... entries) throws Exception {
        JSONArray cores = new JSONArray();
        for (JSONObject entry : entries) cores.put(entry);
        return new JSONObject().put("format", 2).put("cores", cores).toString();
    }

    private void reject(JSONObject entry, String description) throws Exception {
        try {
            new CoreRepository.Core(entry);
            throw new AssertionError(description);
        } catch (IOException | org.json.JSONException expected) { checks++; }
    }

    private void catalogs() throws Exception {
        CoreRepository.Catalog empty = CoreRepository.parseCatalog(feed(), 12, "x86_64");
        check(empty.cores.isEmpty() && empty.records.length == 0 && empty.error.isEmpty(), "empty catalog");
        CoreRepository.Catalog selected = CoreRepository.parseCatalog(feed(
                core("corvus.gb", "arm64-v8a", 12), core("corvus.gba", "x86_64", 12),
                core("corvus.gba", "x86_64", 13)), 12, "x86_64");
        check(selected.cores.size() == 1 && selected.cores.containsKey("corvus.gba"), "ABI and base version filter");
        check(Arrays.equals(selected.records, new String[] {"corvus.gba", "Official test core", "0.14.0", "corvus.gba", "x86_64"}), "JNI identity fields");
        CoreRepository.Catalog newer = CoreRepository.parseCatalog(feed(core("corvus.gb", "x86_64", 13)), 12, "x86_64");
        check(newer.cores.isEmpty() && !newer.error.isEmpty(), "base update required message");
        try {
            CoreRepository.parseCatalog(feed(core("corvus.gb", "x86_64", 12), core("corvus.gb", "x86_64", 12)), 12, "x86_64");
            throw new AssertionError("duplicate core accepted");
        } catch (IOException expected) { checks++; }
        reject(core("corvus.gb", "x86_64", 12).put("split_name", "coregba"), "foreign split identity");
        reject(core("corvus.gb", "x86_64", 12).put("apk_url", "https://example.com/core.apk"), "untrusted origin");
        reject(core("corvus.gb", "x86_64", 12).put("apk_url", "https://github.com/CorvusSoftware/CorvusRetro/releases/download/../core.apk"), "traversal URL");
        reject(core("corvus.gb", "x86_64", 12).put("apk_url", "https://github.com/CorvusSoftware/CorvusRetro/releases/download/test/co%72e.apk"), "encoded URL");
        reject(core("corvus.gb", "x86_64", 12).put("name", "bad\u0085name"), "control character");
        reject(core("corvus.gb", "x86_64", 12).put("name", 1), "numeric name");
        reject(core("corvus.gb", "x86_64", 12).put("description", true), "boolean description");
        reject(core("corvus.gb", "x86_64", 12).put("version", 0.14), "numeric version");
        reject(core("corvus.gb", "x86_64", 12).put("base_version_code", 12.0), "fractional base version");
        reject(core("corvus.gb", "x86_64", 12).put("sha256", "bad"), "invalid hash");
        reject(core("corvus.gb", "x86_64", 12).put("extra", "field"), "unknown field");
    }

    private void identities() throws Exception {
        Method storage = GameLibrary.class.getDeclaredMethod("storageId", String.class, String.class);
        storage.setAccessible(true);
        String hash = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
        check((hash + "-0").equals(storage.invoke(null, hash, "corvus.gb")), "historic GB suffix");
        check((hash + "-1").equals(storage.invoke(null, hash, "corvus.gbc")), "historic GBC suffix");
        check((hash + "-2").equals(storage.invoke(null, hash, "corvus.gba")), "historic GBA suffix without registry order");
        String community = (String) storage.invoke(null, hash, "other.core");
        check(community.length() == 82 && community.equals(storage.invoke(null, hash, "other.core")), "stable community ID");
    }

    private void copy(String source, File target) throws IOException {
        try (FileInputStream input = new FileInputStream(source); FileOutputStream output = new FileOutputStream(target)) {
            byte[] buffer = new byte[65536];
            int count;
            while ((count = input.read(buffer)) != -1) output.write(buffer, 0, count);
        }
    }

    private void archiveProbe(Bundle result) throws Exception {
        Context context = getTargetContext();
        int flags = Build.VERSION.SDK_INT >= 28 ? PackageManager.GET_SIGNING_CERTIFICATES : PackageManager.GET_SIGNATURES;
        PackageInfo host = context.getPackageManager().getPackageInfo(context.getPackageName(), flags);
        check(host.applicationInfo != null, "installed application metadata");
        if (host.splitNames == null || host.splitNames.length == 0) {
            result.putString("archive_probe", "SKIPPED: no installed core split");
            return;
        }
        String[] paths = host.applicationInfo.splitSourceDirs;
        check(paths != null && paths.length == host.splitNames.length, "split archive mapping");
        PackageInfo alone = context.getPackageManager().getPackageArchiveInfo(paths[0], flags);
        File cluster = new File(context.getCacheDir(), "core-archive-probe");
        check(cluster.mkdir() || cluster.isDirectory(), "temporary archive cluster");
        File base = new File(cluster, "base.apk"), split = new File(cluster, "core.apk");
        try {
            copy(host.applicationInfo.sourceDir, base);
            copy(paths[0], split);
            PackageInfo info = context.getPackageManager().getPackageArchiveInfo(cluster.getAbsolutePath(), flags);
            result.putString("archive_probe", "standalone=" + (alone != null) + ", cluster=" + (info != null));
            if (info != null) {
                check(context.getPackageName().equals(info.packageName), "cluster package identity");
                check(info.splitNames != null && info.splitNames.length == 1 && host.splitNames[0].equals(info.splitNames[0]), "cluster split identity");
            }
        } finally {
            base.delete(); split.delete(); cluster.delete();
        }
    }

    private void installationLifecycle() throws Exception {
        Context context = getTargetContext();
        check(!CoreInstallState.pending(context), "no pending installation before lifecycle test");
        String download = CoreInstallState.begin(context);
        check(download != null && CoreInstallState.pending(context), "download survives service recreation");
        check(CoreInstallState.begin(context) == null, "another Activity cannot start a second download");
        CoreInstallState.downloaded(context, "old-operation", false, "ignored");
        check(CoreInstallState.pending(context), "stale download completion cannot release pause");
        CoreInstallState.downloaded(context, download, false, "Test cancellation");
        check(!CoreInstallState.pending(context), "download cancellation releases pause");

        PackageInfo host = context.getPackageManager().getPackageInfo(context.getPackageName(), 0);
        check(host.applicationInfo != null && host.applicationInfo.splitSourceDirs != null &&
                host.applicationInfo.splitSourceDirs.length > 0, "installed core for real pending session");
        PackageInstaller installer = context.getPackageManager().getPackageInstaller();
        PackageInstaller.SessionParams params = new PackageInstaller.SessionParams(
                PackageInstaller.SessionParams.MODE_INHERIT_EXISTING);
        params.setAppPackageName(context.getPackageName());
        if (Build.VERSION.SDK_INT >= 31)
            params.setRequireUserAction(PackageInstaller.SessionParams.USER_ACTION_REQUIRED);
        int id = installer.createSession(params);
        String operation = CoreInstallState.begin(context);
        MainActivity first = null, recreated = null;
        try {
            try (PackageInstaller.Session session = installer.openSession(id)) {
                try (FileInputStream input = new FileInputStream(host.applicationInfo.splitSourceDirs[0]);
                     OutputStream output = session.openWrite(host.splitNames[0] + ".apk", 0, -1)) {
                    byte[] bytes = new byte[65536];
                    int count;
                    while ((count = input.read(bytes)) != -1) output.write(bytes, 0, count);
                    session.fsync(output);
                }
                Intent result = new Intent(context, CoreInstallReceiver.class).setAction(CoreInstallReceiver.ACTION)
                        .putExtra("operation", operation).putExtra("session", id);
                PendingIntent callback = PendingIntent.getBroadcast(context, id, result,
                        PendingIntent.FLAG_UPDATE_CURRENT | PendingIntent.FLAG_MUTABLE);
                CoreInstallState.commit(context, operation, id, session, callback.getIntentSender());
            }
            CoreInstallState.downloaded(context, operation, true, "unused");
            check(CoreInstallState.pending(context), "sealed session retains pause after worker exits");
            awaitConfirmation(context);
            first = (MainActivity) startActivitySync(new Intent(context, MainActivity.class)
                    .addFlags(Intent.FLAG_ACTIVITY_NEW_TASK));
            check(first.coreInstallPending(), "new Activity restores sealed session pause");
            ActivityMonitor monitor = addMonitor(MainActivity.class.getName(), null, false);
            MainActivity old = first;
            runOnMainSync(old::recreate);
            recreated = (MainActivity) monitor.waitForActivityWithTimeout(15000);
            removeMonitor(monitor);
            check(recreated != null && recreated != first && recreated.coreInstallPending(),
                    "recreated Activity keeps pending confirmation paused");
            CoreInstallState.status(context, "stale-operation", id, "ignored", true);
            check(CoreInstallState.pending(context), "stale installer callback cannot release pause");
            CoreInstallState.cancel(context, operation, id, "Test cancellation");
            check(!CoreInstallState.pending(context), "abandoning pending session releases pause");
        } finally {
            CoreInstallState.cancel(context, operation, id, "Test cleanup");
            CoreInstallState.downloaded(context, operation, false, "Test cleanup");
            try { installer.abandonSession(id); } catch (RuntimeException ignored) { }
            MainActivity active = recreated != null ? recreated : first;
            if (active != null) runOnMainSync(active::finish);
        }
    }

    private void awaitConfirmation(Context context) {
        long deadline = android.os.SystemClock.uptimeMillis() + 10000;
        while (android.os.SystemClock.uptimeMillis() < deadline) {
            android.content.SharedPreferences state = context.getSharedPreferences("core_install", Context.MODE_PRIVATE);
            if (!state.getString("confirmation", "").isEmpty() &&
                    CoreInstallState.owns(context, state.getString("operation", ""), state.getInt("session", -1))) {
                check(true, "Android confirmation intent is persisted before foreground launch");
                return;
            }
            android.os.SystemClock.sleep(25);
        }
        throw new AssertionError("Android confirmation was not persisted");
    }

    private void prepareProcessDeath() throws Exception {
        Context context = getTargetContext();
        check(!CoreInstallState.pending(context), "no earlier pending installation");
        PackageInfo host = context.getPackageManager().getPackageInfo(context.getPackageName(), 0);
        PackageInstaller installer = context.getPackageManager().getPackageInstaller();
        PackageInstaller.SessionParams params = new PackageInstaller.SessionParams(
                PackageInstaller.SessionParams.MODE_INHERIT_EXISTING);
        params.setAppPackageName(context.getPackageName());
        if (Build.VERSION.SDK_INT >= 31)
            params.setRequireUserAction(PackageInstaller.SessionParams.USER_ACTION_REQUIRED);
        int id = installer.createSession(params);
        String operation = CoreInstallState.begin(context);
        boolean committed = false;
        try (PackageInstaller.Session session = installer.openSession(id)) {
            try (FileInputStream input = new FileInputStream(host.applicationInfo.splitSourceDirs[0]);
                 OutputStream output = session.openWrite(host.splitNames[0] + ".apk", 0, -1)) {
                byte[] bytes = new byte[65536];
                int count;
                while ((count = input.read(bytes)) != -1) output.write(bytes, 0, count);
                session.fsync(output);
            }
            Intent intent = new Intent(context, CoreInstallReceiver.class).setAction(CoreInstallReceiver.ACTION)
                    .putExtra("operation", operation).putExtra("session", id);
            PendingIntent callback = PendingIntent.getBroadcast(context, id, intent,
                    PendingIntent.FLAG_UPDATE_CURRENT | PendingIntent.FLAG_MUTABLE);
            CoreInstallState.commit(context, operation, id, session, callback.getIntentSender());
            committed = true;
        } finally {
            if (!committed) installer.abandonSession(id);
            CoreInstallState.downloaded(context, operation, committed, "Test cleanup");
        }
        check(CoreInstallState.pending(context), "sealed session persists before process death");
        awaitConfirmation(context);
    }

    private void recoverProcessDeath() throws Exception {
        Context context = getTargetContext();
        String operation = context.getSharedPreferences("core_install", Context.MODE_PRIVATE)
                .getString("operation", "");
        int id = context.getSharedPreferences("core_install", Context.MODE_PRIVATE).getInt("session", -1);
        MainActivity activity = null;
        try {
            check(CoreInstallState.pending(context), "sealed session restores after process death");
            check(!context.getSharedPreferences("core_install", Context.MODE_PRIVATE)
                    .getString("confirmation", "").isEmpty(), "confirmation URI survives process death");
            activity = (MainActivity) startActivitySync(new Intent(context, MainActivity.class)
                    .addFlags(Intent.FLAG_ACTIVITY_NEW_TASK));
            check(activity.coreInstallPending(), "fresh Activity recovers the persistent pause");
        } finally {
            CoreInstallState.cancel(context, operation, id, "Test cleanup");
            if (activity != null) {
                MainActivity active = activity;
                runOnMainSync(active::finish);
            }
        }
        check(!CoreInstallState.pending(context), "pending session is safely cancelled after recovery");
    }

    @Override public void onCreate(Bundle arguments) {
        super.onCreate(arguments);
        phase = arguments == null ? "" : arguments.getString("phase", "");
        start();
    }
    @Override public void onStart() {
        Bundle result = new Bundle();
        try {
            if ("prepare_process".equals(phase)) prepareProcessDeath();
            else if ("recover_process".equals(phase)) recoverProcessDeath();
            else { catalogs(); identities(); archiveProbe(result); installationLifecycle(); }
            result.putString("stream", "PASS " + checks + " Android core catalog/storage/archive checks"
                    + " phase=" + phase + " pid=" + android.os.Process.myPid() + "\n");
            finish(Activity.RESULT_OK, result);
        } catch (Throwable failure) {
            result.putString("stream", "FAIL " + failure.toString() + "\n");
            finish(Activity.RESULT_CANCELED, result);
        }
    }
}
