// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
package com.corvussoftware.corvusretro;

import android.content.ClipData;
import android.content.Intent;
import android.net.Uri;
import android.widget.Toast;
import java.io.File;
import java.io.IOException;
import java.util.concurrent.ArrayBlockingQueue;
import java.util.concurrent.RejectedExecutionException;
import java.util.concurrent.ThreadPoolExecutor;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicBoolean;

/** Android share chooser only. A single bounded export survives no Activity ownership change. */
final class StateShares {
    private static final AtomicBoolean BUSY=new AtomicBoolean();
    private static final ThreadPoolExecutor WORKER=new ThreadPoolExecutor(1,1,0,
            TimeUnit.SECONDS,new ArrayBlockingQueue<>(1),new ThreadPoolExecutor.AbortPolicy());
    private final MainActivity activity;
    private volatile boolean closed;
    StateShares(MainActivity activity) { this.activity=activity; }
    void share(String name,byte[] bytes) {
        if (closed || bytes==null || bytes.length==0 || bytes.length>StateShareFiles.MAX_BYTES) return;
        if (!BUSY.compareAndSet(false,true)) { message("Un partage est déjà en cours");return; }
        try { WORKER.execute(()->export(name,bytes)); }
        catch (RejectedExecutionException unavailable) { BUSY.set(false);message("Partage indisponible"); }
    }
    private void export(String name,byte[] bytes) {
        File file=null;
        try {
            file=StateShareFiles.stage(activity.getCacheDir(),name,bytes,()->!closed && activity.isAlive());
            File ready=file;
            activity.runOnUiThread(()-> {
                if (closed || !activity.isAlive()) { ready.delete();return; }
                Uri uri=new Uri.Builder().scheme("content").authority(activity.getPackageName()+".state-share")
                        .appendPath(ready.getName()).build();
                Intent send=new Intent(Intent.ACTION_SEND).setType("application/octet-stream")
                        .putExtra(Intent.EXTRA_STREAM,uri).putExtra(Intent.EXTRA_TITLE,StateShareFiles.name(name))
                        .addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
                send.setClipData(ClipData.newRawUri("État CorvusRetro",uri));
                try { activity.startActivity(Intent.createChooser(send,"Partager l’état")); }
                catch (RuntimeException unavailable) { message("Aucune application de partage disponible"); }
            });
        } catch (IOException | RuntimeException unavailable) {
            if (file!=null) file.delete();
            message("Impossible de préparer le partage de l’état");
        } finally { BUSY.set(false); }
    }
    private void message(String text) {
        activity.runOnUiThread(()-> {
            if (!closed && activity.isAlive()) Toast.makeText(activity,text,Toast.LENGTH_LONG).show();
        });
    }
    void close() { closed=true; }
}
