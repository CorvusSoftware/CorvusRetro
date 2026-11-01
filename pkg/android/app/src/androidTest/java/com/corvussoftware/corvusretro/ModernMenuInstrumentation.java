// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
package com.corvussoftware.corvusretro;

import android.app.Activity;
import android.app.Instrumentation;
import android.app.UiAutomation;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.graphics.Bitmap;
import android.os.Bundle;
import android.os.SystemClock;
import android.view.KeyEvent;
import java.io.File;
import java.io.FileOutputStream;
import java.util.HashMap;
import java.util.Map;
import org.json.JSONObject;

/** Touches native row/nav bounds and checks actual chrome on both phone orientations. */
public final class ModernMenuInstrumentation extends Instrumentation {
    private int checks;
    private boolean landscape;
    private String theme;
    private MainActivity activity;
    private SharedPreferences preferences;
    private void check(boolean value, String message) {
        if (!value) throw new AssertionError(message);
        ++checks;
    }
    private void preference(String key, int wanted) throws Exception {
        long end = SystemClock.uptimeMillis() + 15000;
        while (activity.preference(key, Integer.MIN_VALUE) != wanted && SystemClock.uptimeMillis() < end)
            SystemClock.sleep(70);
        check(activity.preference(key, Integer.MIN_VALUE) == wanted, "actual native action persists " + key + "=" + wanted);
    }
    private String prefix() { return "test-modern-menu-" + (landscape ? "landscape-" : "portrait-") + theme; }
    private Bitmap capture(String name) throws Exception {
        Bitmap image = getUiAutomation().takeScreenshot();
        check(image != null, "menu screenshot " + name);
        try (FileOutputStream output = new FileOutputStream(new File(getTargetContext().getFilesDir(), prefix() + "-" + name + ".png"))) {
            image.compress(Bitmap.CompressFormat.PNG, 100, output);
        }
        return image;
    }
    private void screenshot(String name) throws Exception { capture(name).recycle(); }
    private void state(int wanted) throws Exception {
        MenuTestNavigation.page(activity, wanted);
        check(MenuTestNavigation.read().state.contains("core=-1 content=0 quick=0"), "menu browsing keeps the core unselected");
    }
    private void chrome() throws Exception {
        MenuTestNavigation.Layout layout = MenuTestNavigation.layout(activity);
        check(layout.side == landscape && layout.side == (layout.width > layout.height), "navigation follows actual viewport orientation");
        check(layout.header > 0 && layout.nav > 0 && layout.label > layout.hint && layout.hint > 0, "actual native text and chrome metrics are positive");
        check(Math.abs(layout.contentWidth - (layout.side ? layout.width - layout.nav : layout.width)) < .02f,
                "only the side navigation consumes content width");
        check(Math.abs(layout.bottom - (layout.side ? layout.height : layout.height - layout.nav)) < .02f,
                "body reaches the navigation with no status footer reserve");
        SystemClock.sleep(800);
        Bitmap image = capture("main");
        int[] safe = activity.safeWindowArea(), origin = MenuTestNavigation.origin(this, activity);
        float density = activity.density();
        int left = origin[0] + safe[2], top = origin[1] + safe[3];
        int right = Math.round(left + layout.width * density), bottom = Math.round(top + layout.height * density);
        int navLeft = layout.side ? Math.round(left + layout.contentWidth * density) : left;
        int navTop = layout.side ? top : Math.round(top + layout.bottom * density);
        int background = image.getPixel(right - 2, bottom - 2), outside = 0, sampled = 0;
        for (int y = navTop + 3; y < bottom - 1; ++y) for (int x = navLeft + 3; x < right - 1; ++x) {
            boolean icon = false;
            for (int index = 0; index < 3; ++index) {
                float cx = layout.side ? (navLeft + right) / 2f : left + (right - left) * (index + .5f) / 3;
                float cy = layout.side ? top + (bottom - top) * (index + .5f) / 3 : (navTop + bottom) / 2f;
                if (Math.abs(x - cx) <= layout.icon * density / 2 + 2 &&
                    Math.abs(y - cy) <= layout.icon * density / 2 + 2) icon = true;
            }
            if (!icon) { ++sampled; if (image.getPixel(x,y) != background) ++outside; }
        }
        image.recycle();
        check(sampled > 30 && outside <= 4, "navigation contains only its three icon cells and selection indicator, without text labels: outside=" + outside);
        MenuTestNavigation.Layout last = MenuTestNavigation.selectRow(this, activity, 2);
        check(last.rowY >= last.header - .05f && last.rowY + last.rowHeight <= last.bottom + .05f,
                "last main row stays fully inside the native body");
        Bitmap body = capture("main-last-row");
        int blankTop = Math.round(top + (last.rowY + last.rowHeight) * density) + 3;
        int blankBottom = Math.round(top + last.bottom * density) - 2;
        check(blankBottom > blankTop + 4, "main content leaves a measurable empty region before navigation");
        int color = body.getPixel(left + 2, blankTop), marks = 0;
        for (int y = blankTop; y < blankBottom; ++y)
            for (int x = left + 2; x < Math.round(left + last.contentWidth * density) - 2; ++x)
                if (body.getPixel(x,y) != color) ++marks;
        body.recycle();
        check(marks == 0, "no obsolete core status text is drawn below the last row");
        JSONObject geometry = new JSONObject().put("width",layout.width).put("height",layout.height)
                .put("header",layout.header).put("body_bottom",layout.bottom).put("nav",layout.nav)
                .put("side_nav",layout.side).put("density",density).put("label",layout.label)
                .put("hint",layout.hint).put("last_row_y",last.rowY).put("last_row_height",last.rowHeight);
        try (FileOutputStream out = new FileOutputStream(new File(activity.getFilesDir(),prefix()+"-geometry.json"))) {
            out.write(geometry.toString(2).getBytes(java.nio.charset.StandardCharsets.UTF_8));
        }
    }
    private void interaction() throws Exception {
        MenuTestNavigation.tapTab(this,activity,1); state(1); screenshot("playlists");
        MenuTestNavigation.tapRow(this,activity,0); state(4); screenshot("history");
        MenuTestNavigation.key(this,activity,KeyEvent.KEYCODE_BACK); state(1);
        MenuTestNavigation.tapTab(this,activity,2); state(2); screenshot("settings");
        MenuTestNavigation.tapRow(this,activity,5); state(34); screenshot("achievements-settings");
        MenuTestNavigation.key(this,activity,KeyEvent.KEYCODE_BACK); state(2);
        MenuTestNavigation.tapRow(this,activity,2); state(11);
        MenuTestNavigation.tapRow(this,activity,0); preference("audio",0); screenshot("switch-off");
        MenuTestNavigation.tapRow(this,activity,0); preference("audio",1);
        MenuTestNavigation.tapRow(this,activity,1); state(29);
        MenuTestNavigation.selectRow(this,activity,3); screenshot("choice");
        MenuTestNavigation.tapRow(this,activity,3); state(11); preference("audio_volume",30);
        MenuTestNavigation.tapTab(this,activity,0); state(0);
    }
    private void finishActivity() throws Exception {
        if (activity != null) { MainActivity previous = activity; runOnMainSync(previous::finish); activity = null; waitForIdleSync(); SystemClock.sleep(600); }
    }
    private void execute() throws Exception {
        preferences = getTargetContext().getSharedPreferences("corvus", Context.MODE_PRIVATE);
        String[] keys = {"theme","audio","audio_volume","achievements_enabled"};
        Map<String,Object> previous = new HashMap<>(); Map<String,?> all = preferences.getAll();
        for (String key : keys) previous.put(key,all.get(key));
        try {
            for (String current : new String[] {"corvus","asus","shield"}) {
                theme = current;
                check(preferences.edit().putString("theme",theme).putBoolean("audio",true)
                        .putInt("audio_volume",100).putInt("achievements_enabled",0).commit(), "temporary native menu preferences");
                activity = (MainActivity)startActivitySync(new Intent(getTargetContext(),MainActivity.class).addFlags(Intent.FLAG_ACTIVITY_NEW_TASK));
                check(getUiAutomation().setRotation(landscape ? UiAutomation.ROTATION_FREEZE_90 : UiAutomation.ROTATION_FREEZE_0), "requested test orientation");
                state(0); chrome();
                if ("corvus".equals(theme)) interaction();
                else {
                    MenuTestNavigation.tapTab(this,activity,2); state(2); screenshot("settings");
                    MenuTestNavigation.tapTab(this,activity,1); state(1); screenshot("playlists");
                    MenuTestNavigation.tapTab(this,activity,0); state(0);
                }
                finishActivity();
            }
        } finally {
            finishActivity();
            SharedPreferences.Editor restore = preferences.edit();
            for (Map.Entry<String,Object> entry : previous.entrySet()) {
                restore.remove(entry.getKey()); Object value = entry.getValue();
                if (value instanceof String) restore.putString(entry.getKey(),(String)value);
                else if (value instanceof Boolean) restore.putBoolean(entry.getKey(),(Boolean)value);
                else if (value instanceof Integer) restore.putInt(entry.getKey(),(Integer)value);
            }
            check(restore.commit(),"restore only touched preferences with their original types");
            getUiAutomation().setRotation(UiAutomation.ROTATION_UNFREEZE);
        }
    }
    @Override public void onCreate(Bundle arguments) {
        super.onCreate(arguments); landscape = "landscape".equals(arguments.getString("orientation","portrait")); start();
    }
    @Override public void onStart() {
        Bundle result = new Bundle();
        try { execute(); result.putString("stream","PASS "+checks+" Android modern menu checks orientation="+(landscape?"landscape":"portrait")+"\n"); finish(Activity.RESULT_OK,result); }
        catch (Throwable failure) { result.putString("stream","FAIL "+android.util.Log.getStackTraceString(failure)+"\n"); finish(Activity.RESULT_CANCELED,result); }
    }
}
