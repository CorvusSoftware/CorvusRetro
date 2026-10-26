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

/** Android confirms and validates adding a signed feature split to this package. */
public final class CoreInstallReceiver extends BroadcastReceiver {
    static final String ACTION = "com.corvussoftware.corvusretro.CORE_SPLIT_INSTALL";
    @Override public void onReceive(Context context, Intent intent) {
        if (intent == null || !ACTION.equals(intent.getAction())) return;
        String operation = intent.getStringExtra("operation");
        int session = intent.getIntExtra("session", -1);
        if (!CoreInstallState.owns(context, operation, session)) return;
        int status = intent.getIntExtra(PackageInstaller.EXTRA_STATUS,
                                         PackageInstaller.STATUS_FAILURE);
        if (status == PackageInstaller.STATUS_PENDING_USER_ACTION) {
            Intent confirmation;
            if (Build.VERSION.SDK_INT >= 33) {
                confirmation = intent.getParcelableExtra(Intent.EXTRA_INTENT, Intent.class);
            } else {
                confirmation = intent.getParcelableExtra(Intent.EXTRA_INTENT);
            }
            if (confirmation == null) {
                CoreInstallState.cancel(context, operation, session, "Confirmation d'installation indisponible");
                return;
            }
            CoreInstallState.confirmation(context, operation, session, confirmation);
        } else if (status == PackageInstaller.STATUS_SUCCESS) {
            CoreInstallState.status(context, operation, session,
                    "Cœur installé. Relancez CorvusRetro pour le charger", true);
        } else {
            Log.w("CorvusRetro", "Core split install status " + status);
            CoreInstallState.status(context, operation, session,
                    "Installation refusée par Android (statut " + status + ")", true);
        }
    }

}
