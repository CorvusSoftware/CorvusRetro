// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
package com.corvussoftware.corvusretro;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.LinearGradient;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.Shader;
import android.os.SystemClock;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.util.Iterator;
import java.util.LinkedHashMap;
import java.util.Map;
import java.util.concurrent.ArrayBlockingQueue;
import java.util.concurrent.RejectedExecutionException;
import java.util.concurrent.ThreadPoolExecutor;
import java.util.concurrent.TimeUnit;

/** Optional art service. Every file read and rasterization stays off the frontend thread. */
final class FrontendImages {
    private static final ThreadPoolExecutor WORKERS=new ThreadPoolExecutor(2,2,0,
            TimeUnit.SECONDS,new ArrayBlockingQueue<>(16),new ThreadPoolExecutor.AbortPolicy());
    private static final int ENTRIES=32, BITMAPS=12;
    private final Object lock=new Object();
    private final File files;
    private final GameLibrary library;
    private final Runnable ready;
    private final RetroAchievementsImages covers;
    private final LinkedHashMap<String,Entry> entries=new LinkedHashMap<>(16,.75f,true);
    private volatile boolean closed;
    private static final class Entry {
        Bitmap bitmap;
        String source;
        Runnable task;
        boolean pending;
        long retryAt;
    }
    FrontendImages(Context context,GameLibrary library,Runnable ready) {
        this.files=context.getFilesDir();this.library=library;this.ready=ready;
        covers=new RetroAchievementsImages(context.getApplicationContext(),ready,512);
    }
    Bitmap get(String key) {
        if (closed || !FrontendImageFiles.allowed(key)) return null;
        synchronized (lock) {
            if (closed) return null;
            Entry entry=entries.get(key);
            if (entry==null) {
                if (entries.size()>=ENTRIES) {
                    Iterator<Map.Entry<String,Entry>> iterator=entries.entrySet().iterator();
                    while (iterator.hasNext()) if (!iterator.next().getValue().pending) { iterator.remove();break; }
                    if (entries.size()>=ENTRIES) return null;
                }
                entry=new Entry();entries.put(key,entry);
            }
            if (entry.bitmap!=null) return entry.bitmap;
            if (entry.source!=null) return covers.get(entry.source);
            long now=SystemClock.elapsedRealtime();
            if (entry.pending || now<entry.retryAt) return null;
            entry.pending=true;
            Entry target=entry;
            entry.task=()->load(key,target);
            try { WORKERS.execute(entry.task); }
            catch (RejectedExecutionException full) {
                entry.pending=false;entry.task=null;entry.retryAt=now+2000;
            }
            return null;
        }
    }
    void invalidate(String key) { synchronized (lock) { entries.remove(key); } }
    private void load(String key,Entry target) {
        Bitmap bitmap=null;String source=null;
        try {
            if (closed) return;
            String game=FrontendImageFiles.coverGame(key);
            if (game!=null) {
                String url=library.cover(game);
                if (RetroAchievementsImages.allowed(url)) source=url;
            } else if ("asset:home".equals(key)) bitmap=home();
            else {
                File file=FrontendImageFiles.preview(files,key);
                byte[] bytes=new byte[(int)file.length()];
                try (FileInputStream input=new FileInputStream(file)) {
                    int at=0;
                    while (at<bytes.length) {
                        int n=input.read(bytes,at,bytes.length-at);
                        if (n<=0 || closed) throw new IOException("Preview cancelled or truncated");
                        at+=n;
                    }
                    if (input.read()!=-1) throw new IOException("Preview changed during read");
                }
                FrontendImageFiles.Pixels preview=FrontendImageFiles.ppm(bytes);
                bitmap=Bitmap.createBitmap(preview.argb,preview.width,preview.height,Bitmap.Config.ARGB_8888);
            }
        } catch (IOException | RuntimeException unavailable) {
            // The native console illustration remains the fallback for optional art.
        }
        synchronized (lock) {
            if (closed || entries.get(key)!=target) return;
            target.bitmap=bitmap;target.source=source;target.pending=false;target.task=null;
            target.retryAt=SystemClock.elapsedRealtime()+2000;
            int count=0;
            for (Entry entry:entries.values()) if (entry.bitmap!=null) count++;
            for (Entry entry:entries.values()) {
                if (count<=BITMAPS) break;
                if (entry!=target && entry.bitmap!=null) { entry.bitmap=null;entry.retryAt=0;count--; }
            }
        }
        // JNI may still hold an evicted Bitmap, so publication never recycles it.
        if (!closed && ready!=null && (bitmap!=null || source!=null)) ready.run();
    }
    private static void polygon(Canvas canvas,Paint paint,int color,float...points) {
        paint.setColor(color);paint.setShader(null);
        Path path=new Path();path.moveTo(points[0],points[1]);
        for (int i=2;i<points.length;i+=2) path.lineTo(points[i],points[i+1]);
        path.close();canvas.drawPath(path,paint);
    }
    /** Original Corvus landscape artwork; no ROM or game artwork is bundled. */
    private static Bitmap home() {
        Bitmap image=Bitmap.createBitmap(512,224,Bitmap.Config.ARGB_8888);
        Canvas canvas=new Canvas(image);Paint paint=new Paint();
        paint.setShader(new LinearGradient(0,0,440,224,new int[]{0xff121236,0xff4e238d,0xff28113f},null,Shader.TileMode.CLAMP));
        canvas.drawRect(0,0,512,224,paint);paint.setShader(null);
        for (int i=0;i<64;i++) {
            int x=(i*83+19)%510,y=(i*41+7)%112;
            paint.setColor(i%4==0 ? 0xff7ad8ef : 0xffa48ae4);canvas.drawRect(x,y,x+(i%3==0?2:1),y+1,paint);
        }
        paint.setShader(new LinearGradient(0,94,0,179,0xffbd4df2,0x00182e65,Shader.TileMode.CLAMP));
        canvas.drawRect(0,94,512,184,paint);paint.setShader(null);
        polygon(canvas,paint,0xff593383,0,165,43,122,77,146,117,100,171,141,220,120,273,153,326,104,384,150,429,122,512,170,512,224,0,224);
        polygon(canvas,paint,0xff30245b,0,190,64,152,104,175,171,126,240,182,284,154,359,193,412,156,512,195,512,224,0,224);
        polygon(canvas,paint,0xff161936,0,196,74,183,127,202,208,169,291,207,359,180,430,207,512,194,512,224,0,224);
        for (int i=0;i<12;i++) {
            int x=i*47-12,h=24+(i*17)%43,y=202+(i%3)*7;
            polygon(canvas,paint,0xff10162e,x,y-h,x-13,y-10,x-8,y-10,x-17,y+1,x+17,y+1,x+8,y-10,x+13,y-10);
            paint.setColor(0xff10162e);canvas.drawRect(x-2,y-3,x+3,224,paint);
        }
        canvas.save();canvas.rotate(10,410,144);
        paint.setColor(0xff0d1029);canvas.drawRoundRect(372,75,455,213,13,13,paint);
        paint.setShader(new LinearGradient(375,75,454,210,0xffc267f4,0xff7043bb,Shader.TileMode.CLAMP));
        canvas.drawRoundRect(367,69,450,207,13,13,paint);paint.setShader(null);
        paint.setColor(0xffd8a8ff);canvas.drawRoundRect(373,75,444,140,6,6,paint);
        paint.setColor(0xff252648);canvas.drawRoundRect(378,81,439,134,3,3,paint);
        paint.setColor(0xff605487);canvas.drawRect(384,87,433,128,paint);
        paint.setColor(0xff35224f);canvas.drawRect(382,155,391,182,paint);canvas.drawRect(373,164,400,173,paint);
        paint.setColor(0xffe5bbff);canvas.drawCircle(428,161,5,paint);canvas.drawCircle(416,173,5,paint);
        paint.setColor(0xff4c2a73);canvas.drawRoundRect(398,190,409,193,2,2,paint);canvas.drawRoundRect(413,190,424,193,2,2,paint);
        canvas.restore();return image;
    }
    void close() {
        synchronized (lock) {
            closed=true;
            for (Entry entry:entries.values()) if (entry.task!=null) WORKERS.remove(entry.task);
            entries.clear();
        }
        covers.close();
    }
}
