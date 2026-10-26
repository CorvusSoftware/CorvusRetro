// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
package com.corvussoftware.corvusretro;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageInstaller;
import android.os.Build;
import android.util.Log;

/** The OS, not CorvusRetro, confirms and authorizes installing an APK upgrade. */
public final class CoreInstallReceiver extends BroadcastReceiver {
    static final String ACTION = "com.corvussoftware.corvusretro.CORE_PACK_INSTALL";
    @Override public void onReceive(Context context, Intent intent) {
        if (intent == null || !ACTION.equals(intent.getAction())) return;
        int status = intent.getIntExtra(PackageInstaller.EXTRA_STATUS,
                                         PackageInstaller.STATUS_FAILURE);
        String detail = intent.getStringExtra(PackageInstaller.EXTRA_STATUS_MESSAGE);
        if (status == PackageInstaller.STATUS_PENDING_USER_ACTION) {
            Intent confirmation;
            if (Build.VERSION.SDK_INT >= 33) {
                confirmation = intent.getParcelableExtra(Intent.EXTRA_INTENT, Intent.class);
            } else {
                confirmation = intent.getParcelableExtra(Intent.EXTRA_INTENT);
            }
            if (confirmation == null) {
                report("Confirmation d'installation indisponible");
                return;
            }
            confirmation.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            try {
                context.startActivity(confirmation);
                report("Confirme l'installation du pack dans Android");
            } catch (RuntimeException error) {
                Log.w("CorvusRetro", "Core update confirmation refused", error);
                report("Installation annulée ou non autorisée");
            }
        } else if (status == PackageInstaller.STATUS_SUCCESS) {
            report("CorvusRetro mis à jour avec les cœurs sélectionnés");
        } else {
            Log.w("CorvusRetro", "Core package install status " + status +
                    (detail == null ? "" : (": " + detail)));
            report("Installation refusée par Android (statut " + status + ")");
        }
    }

    private static void report(String message) {
        try {
            MainActivity.coreInstallStatus(message);
        } catch (UnsatisfiedLinkError error) {
            Log.i("CorvusRetro", message);
        }
    }
}
