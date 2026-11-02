// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
package com.corvussoftware.corvusretro;

import android.app.Activity;
import android.app.Instrumentation;
import android.content.Intent;
import android.content.Context;
import android.content.SharedPreferences;
import java.util.HashMap;
import java.util.Map;
import android.content.res.AssetFileDescriptor;
import android.media.MediaMetadataRetriever;
import android.os.Bundle;
import android.os.SystemClock;
import java.io.InputStream;
import java.lang.reflect.Field;
import java.security.MessageDigest;
import java.util.concurrent.atomic.AtomicReference;

/** Uses Android's actual decoder and SoundPool with the supplied production MP3. */
public final class AchievementSoundInstrumentation extends Instrumentation {
    private MainActivity activity;
    private AchievementSound sound;
    private int checks;
    private SharedPreferences preferences;
    private final Map<String,Object> previous = new HashMap<>();
    private interface Condition { boolean ready() throws Exception; }
    private void check(boolean value, String message) {
        if (!value) throw new AssertionError(message);
        ++checks;
    }
    private Object field(String name) throws Exception {
        Field field = AchievementSound.class.getDeclaredField(name); field.setAccessible(true);
        AtomicReference<Object> value = new AtomicReference<>();
        runOnMainSync(() -> {
            try { value.set(field.get(sound)); } catch (IllegalAccessException error) { throw new AssertionError(error); }
        });
        return value.get();
    }
    private void await(Condition condition, String message) throws Exception {
        long end = SystemClock.uptimeMillis() + 20000;
        do {
            if (condition.ready()) { check(true,message); return; }
            SystemClock.sleep(80);
        } while (SystemClock.uptimeMillis() < end);
        throw new AssertionError(message);
    }
    private int stream() throws Exception { return (Integer)field("stream"); }
    private void execute() throws Exception {
        MessageDigest digest = MessageDigest.getInstance("SHA-256");
        try (InputStream input = getTargetContext().getResources().openRawResource(R.raw.achievement_unlock)) {
            byte[] block = new byte[4096]; int size;
            while ((size=input.read(block))!=-1) digest.update(block,0,size);
        }
        StringBuilder hash = new StringBuilder();
        for (byte value:digest.digest()) hash.append(String.format(java.util.Locale.ROOT,"%02x",value&255));
        check(hash.toString().equals("f00a367f0653432af12f74bfd8f9c62194910cfba475b265b0057decf81c2531"),
                "APK contains the exact uploaded MP3 without re-encoding");
        MediaMetadataRetriever metadata = new MediaMetadataRetriever();
        try (AssetFileDescriptor file = getTargetContext().getResources().openRawResourceFd(R.raw.achievement_unlock)) {
            metadata.setDataSource(file.getFileDescriptor(),file.getStartOffset(),file.getLength());
            long duration = Long.parseLong(metadata.extractMetadata(MediaMetadataRetriever.METADATA_KEY_DURATION));
            check(duration>=2900 && duration<=3200,"Android decodes the three-second production clip");
        } finally { metadata.release(); }
        preferences=getTargetContext().getSharedPreferences("corvus",Context.MODE_PRIVATE);
        Map<String,?> saved=preferences.getAll();
        for (String key:new String[] {"audio","audio_volume","notify_cards"}) previous.put(key,saved.get(key));
        check(preferences.edit().putBoolean("audio",true).putInt("audio_volume",100).putInt("notify_cards",1).commit(),
                "temporary sound preferences committed");
        activity=(MainActivity)startActivitySync(new Intent(getTargetContext(),MainActivity.class).addFlags(Intent.FLAG_ACTIVITY_NEW_TASK));
        Field service=MainActivity.class.getDeclaredField("achievementSound"); service.setAccessible(true);
        sound=(AchievementSound)service.get(activity);
        await(() -> activity.hasWindowFocus() && (Boolean)field("active") && (Boolean)field("loaded"),
                "SoundPool preloads the actual asset while the activity is resumed");
        check(stream()==0,"no automatic sound on activity or game startup");
        activity.playAchievementSound(0); SystemClock.sleep(200);
        check(stream()==0,"zero gain never allocates a playback stream");
        activity.playAchievementSound(30);
        await(() -> stream()>0,"SoundPool accepts playback at native volume");
        int first=stream();
        activity.playAchievementSound(100);
        await(() -> stream()>0 && stream()!=first,"a new effect replaces the previous stream");
        activity.preferenceSet("audio_volume",0);
        await(() -> stream()==0,"muting game volume stops an effect already playing");
        activity.playAchievementSound(100); SystemClock.sleep(200);
        check(stream()==0,"zero app volume also rejects pending native requests");
        activity.preferenceSet("audio_volume",100); SystemClock.sleep(200);
        check(stream()==0,"restoring volume does not replay the old achievement");
        activity.playAchievementSound(100); await(() -> stream()>0,"new achievement plays at restored volume");
        activity.preferenceSet("audio",0);
        await(() -> stream()==0,"audio switch stops the current effect");
        activity.preferenceSet("audio",1); activity.playAchievementSound(100);
        await(() -> stream()>0,"a fresh event plays with audio enabled");
        activity.preferenceSet("notify_cards",0);
        await(() -> stream()==0,"disabling cards stops their current effect");
        activity.preferenceSet("notify_cards",1); activity.playAchievementSound(100);
        await(() -> stream()>0,"fresh notification plays after re-enabling cards");
        check(getUiAutomation().performGlobalAction(android.accessibilityservice.AccessibilityService.GLOBAL_ACTION_HOME),
                "application goes to background through normal lifecycle");
        await(() -> !(Boolean)field("active") && stream()==0,"onPause stops sound playback");
        activity.playAchievementSound(100); SystemClock.sleep(250);
        check(stream()==0,"background unlock cannot play or remain pending");
        getTargetContext().startActivity(new Intent(getTargetContext(),MainActivity.class)
                .addFlags(Intent.FLAG_ACTIVITY_NEW_TASK|Intent.FLAG_ACTIVITY_REORDER_TO_FRONT));
        await(() -> activity.hasWindowFocus() && (Boolean)field("active"),"activity sound resumes normally");
        check(stream()==0,"resume never replays the previous unlock");
        runOnMainSync(() -> { activity.playAchievementSound(100); sound.pause(); sound.resume(); });
        SystemClock.sleep(200);
        check(stream()==0,"queued request from an older resume generation is discarded");
        activity.playAchievementSound(50);
        await(() -> stream()>0,"fresh unlock still plays after lifecycle changes");
        runOnMainSync(activity::finish);
        await(() -> (Boolean)field("closed"),"activity destruction closes its audio service");
        check(field("pool")==null && stream()==0,"destruction releases SoundPool and its stream");
        activity.playAchievementSound(100); SystemClock.sleep(200);
        check(stream()==0,"late JNI request after destruction remains silent");
    }
    @Override public void onCreate(Bundle arguments) { super.onCreate(arguments); start(); }
    @Override public void onStart() {
        Bundle result=new Bundle(); int code=Activity.RESULT_CANCELED;
        try { execute(); result.putString("stream","PASS "+checks+" Android achievement sound checks\n"); code=Activity.RESULT_OK; }
        catch (Throwable failure) { result.putString("stream","FAIL "+android.util.Log.getStackTraceString(failure)+"\n"); }
        finally {
            if (activity!=null) runOnMainSync(activity::finish);
            if (preferences!=null) {
                SharedPreferences.Editor restore=preferences.edit();
                for (Map.Entry<String,Object> entry:previous.entrySet()) {
                    restore.remove(entry.getKey()); Object value=entry.getValue();
                    if (value instanceof Boolean) restore.putBoolean(entry.getKey(),(Boolean)value);
                    else if (value instanceof Integer) restore.putInt(entry.getKey(),(Integer)value);
                }
                restore.commit();
            }
        }
        finish(code,result);
    }
}
