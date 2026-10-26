// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
package com.corvussoftware.corvusretro;

import android.content.Context;
import android.content.SharedPreferences;
import android.content.Intent;
import android.content.pm.PackageInstaller;
import java.io.IOException;
import java.lang.ref.WeakReference;
import java.util.UUID;

/** Owns the game pause across Activity recreation and Android confirmation. */
final class CoreInstallState {
    private static final Object LOCK = new Object();
    private static String download;
    private static WeakReference<MainActivity> foreground = new WeakReference<>(null);
    private static String attempted;

    private static SharedPreferences preferences(Context context) {
        return context.getSharedPreferences("core_install", Context.MODE_PRIVATE);
    }

    private static boolean pendingLocked(Context context) {
        if (download != null) return true;
        SharedPreferences state = preferences(context);
        int id = state.getInt("session", -1);
        if (id < 0) return false;
        PackageInstaller installer = context.getPackageManager().getPackageInstaller();
        PackageInstaller.SessionInfo info = installer.getSessionInfo(id);
        if (info != null && context.getPackageName().equals(info.getInstallerPackageName())) {
            if (info.isSealed()) return true;
            // No worker remains after a process restart to commit this staged session.
            installer.abandonSession(id);
        }
        state.edit().clear().commit();
        return false;
    }

    static boolean pending(Context context) {
        synchronized (LOCK) {
            try { return pendingLocked(context); }
            catch (RuntimeException error) { return true; }
        }
    }

    static String begin(Context context) {
        synchronized (LOCK) {
            if (pendingLocked(context)) return null;
            download = UUID.randomUUID().toString();
            return download;
        }
    }

    static void commit(Context context, String operation, int id,
                       PackageInstaller.Session session, android.content.IntentSender result)
            throws IOException {
        synchronized (LOCK) {
            if (!operation.equals(download)) throw new IOException("Installation annulée");
            if (!preferences(context).edit().putInt("session", id)
                    .putString("operation", operation).commit())
                throw new IOException("État d'installation non enregistré");
            // Persist ownership before Android can return a confirmation or kill the process.
            session.commit(result);
        }
    }

    static void downloaded(Context context, String operation, boolean committed, String message) {
        synchronized (LOCK) {
            if (!operation.equals(download)) return;
            download = null;
            if (!committed) {
                SharedPreferences state = preferences(context);
                if (operation.equals(state.getString("operation", ""))) {
                    int id = state.getInt("session", -1);
                    PackageInstaller.SessionInfo info = context.getPackageManager()
                            .getPackageInstaller().getSessionInfo(id);
                    if (info != null && info.isSealed()) {
                        report("Une installation Android est encore en attente", false);
                        return;
                    }
                    state.edit().clear().commit();
                }
                report(message, true);
            }
        }
    }

    static boolean owns(Context context, String operation, int id) {
        synchronized (LOCK) {
            SharedPreferences state = preferences(context);
            return operation != null && operation.equals(state.getString("operation", "")) &&
                    id == state.getInt("session", -1);
        }
    }

    static void progress(Context context, String operation, String message) {
        synchronized (LOCK) {
            if (operation.equals(download) ||
                    operation.equals(preferences(context).getString("operation", "")))
                report(message, false);
        }
    }

    static void resumed(MainActivity activity) {
        synchronized (LOCK) {
            foreground = new WeakReference<>(activity);
            attempted = null;
        }
        activity.getWindow().getDecorView().post(() -> showConfirmation(activity));
    }

    static void paused(MainActivity activity) {
        synchronized (LOCK) {
            if (foreground.get() == activity) foreground.clear();
        }
    }

    static void confirmation(Context context, String operation, int id, Intent intent) {
        MainActivity activity;
        synchronized (LOCK) {
            if (!owns(context, operation, id)) return;
            if (intent.getIntExtra(PackageInstaller.EXTRA_SESSION_ID, -1) != id ||
                    !preferences(context).edit().putString("confirmation",
                            intent.toUri(Intent.URI_INTENT_SCHEME)).commit()) {
                cancel(context, operation, id, "Confirmation d'installation indisponible");
                return;
            }
            activity = foreground.get();
            report("Revenez à CorvusRetro pour confirmer l'installation du cœur", false);
        }
        if (activity != null)
            activity.getWindow().getDecorView().post(() -> showConfirmation(activity));
    }

    static void showConfirmation(MainActivity activity) {
        synchronized (LOCK) {
            if (foreground.get() != activity || !activity.isAlive() || !activity.hasWindowFocus()) return;
            SharedPreferences state = preferences(activity);
            String operation = state.getString("operation", "");
            String uri = state.getString("confirmation", "");
            int id = state.getInt("session", -1);
            if (operation.isEmpty() || uri.isEmpty() || operation.equals(attempted)) return;
            try {
                if (!pendingLocked(activity) || !owns(activity, operation, id)) return;
                Intent intent = Intent.parseUri(uri, Intent.URI_INTENT_SCHEME);
                if (intent.getIntExtra(PackageInstaller.EXTRA_SESSION_ID, -1) != id)
                    throw new IllegalArgumentException("Incorrect installation session");
                attempted = operation;
                activity.startActivity(intent);
                report("Confirmez l'installation du cœur dans Android", false);
            } catch (Exception error) {
                cancel(activity, operation, id, "Confirmation d'installation indisponible");
            }
        }
    }

    static void status(Context context, String operation, int id, String message, boolean terminal) {
        synchronized (LOCK) {
            if (!owns(context, operation, id)) return;
            if (terminal) {
                preferences(context).edit().clear().commit();
                if (operation.equals(download)) download = null;
            }
            report(message, terminal);
        }
    }

    static void cancel(Context context, String operation, int id, String message) {
        synchronized (LOCK) {
            if (!owns(context, operation, id)) return;
            PackageInstaller installer = context.getPackageManager().getPackageInstaller();
            try { installer.abandonSession(id); }
            catch (RuntimeException error) {
                if (installer.getSessionInfo(id) != null) {
                    report("Une installation Android est encore en attente", false);
                    return;
                }
            }
            status(context, operation, id, message, true);
        }
    }

    static void report(String message, boolean terminal) {
        try { MainActivity.coreInstallStatus(message, terminal); }
        catch (UnsatisfiedLinkError error) { android.util.Log.i("CorvusRetro", message); }
    }
}
