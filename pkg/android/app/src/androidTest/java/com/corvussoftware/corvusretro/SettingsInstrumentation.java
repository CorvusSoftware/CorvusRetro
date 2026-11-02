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
import android.view.MotionEvent;
import java.io.BufferedReader;
import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStreamReader;
import java.nio.charset.StandardCharsets;
import java.util.ArrayDeque;
import java.util.HashMap;
import java.util.Map;

/** Exercises the real C menu, Android input and typed storage without a selected core. */
public final class SettingsInstrumentation extends Instrumentation {
    private static final int ROOT = 2, UI = 9, VIDEO = 10, AUDIO = 11, COMMANDS = 12,
            TACTILE = 13, NOTIFICATIONS = 28, VALUES = 29, LAYOUT = 30,
            CONTROLLERS = 31, BINDINGS = 32, SAVES = 33, RA = 34, ACCOUNT = 35;
    private static final String[] INTEGER_KEYS = {"audio_volume", "video_integer", "video_smooth",
            "overlay_opacity", "overlay_size", "overlay_position", "notify_cards", "notify_progress",
            "backup_count", "auto_state", "show_input", "achievements_enabled"};
    private static final int[] DEFAULTS = {100, 0, 0, 100, 100, 0, 1, 1, 1, 0, 1, 0};
    private final ArrayDeque<int[]> parents = new ArrayDeque<>();
    private boolean landscape;
    private int checks, page, focus, rows;
    private MainActivity activity;
    private SharedPreferences preferences;

    private void check(boolean value, String message) {
        if (!value) throw new AssertionError(message);
        ++checks;
    }
    private interface Condition { boolean ready() throws Exception; }
    private void await(Condition condition, String message) throws Exception {
        long deadline = SystemClock.uptimeMillis() + 20000;
        do {
            if (condition.ready()) { check(true, message); return; }
            SystemClock.sleep(100);
        } while (SystemClock.uptimeMillis() < deadline);
        throw new AssertionError(message + " / " + nativeState());
    }
    private String nativeState() throws Exception {
        Process process = Runtime.getRuntime().exec(new String[] {"logcat", "-d", "--pid=" + android.os.Process.myPid(),
                "-s", "CorvusRetro:D", "*:S"});
        String state = "";
        try (BufferedReader reader = new BufferedReader(new InputStreamReader(process.getInputStream(), StandardCharsets.UTF_8))) {
            String line;
            while ((line = reader.readLine()) != null) if (line.contains("frontend page=")) state = line;
        }
        process.waitFor();
        return state;
    }
    private void state(int wanted) throws Exception {
        await(() -> nativeState().contains("frontend page=" + wanted + " core=-1 content=0 quick=0")
                && nativeState().contains(" active=1 "), "native settings page " + wanted + " remains active without a selected core");
    }
    private void key(int key) throws Exception {
        await(activity::hasWindowFocus, "settings activity owns input focus");
        sendKeyDownUpSync(key); SystemClock.sleep(65);
    }
    private void go(int target) throws Exception {
        check(target >= 0 && target < rows, "selection within the actual page");
        int distance = target - focus;
        for (int i = 0; i < Math.abs(distance); ++i)
            key(distance > 0 ? KeyEvent.KEYCODE_DPAD_DOWN : KeyEvent.KEYCODE_DPAD_UP);
        focus = target;
    }
    private void open(int row, int child, int childRows) throws Exception {
        go(row); parents.push(new int[] {page, focus, rows});
        key(KeyEvent.KEYCODE_DPAD_CENTER); state(child);
        page = child; rows = childRows; focus = 0;
    }
    private void back() throws Exception {
        int[] previous = parents.pop(); key(KeyEvent.KEYCODE_BACK); state(previous[0]);
        page = previous[0]; focus = previous[1]; rows = previous[2];
    }
    private void settingsRoot() throws Exception {
        await(activity::hasWindowFocus, "settings tab input ready");
        int[] safe = activity.safeWindowArea();
        float density = activity.getResources().getDisplayMetrics().density;
        float x = safe[2] + (safe[0] - safe[2] - safe[4]) * 5f / 6;
        float y = safe[1] - safe[5] - 32 * density;
        long now = SystemClock.uptimeMillis();
        sendPointerSync(MotionEvent.obtain(now, now, MotionEvent.ACTION_DOWN, x, y, 0));
        sendPointerSync(MotionEvent.obtain(now, now + 16, MotionEvent.ACTION_UP, x, y, 0));
        state(ROOT); parents.clear(); page = ROOT; focus = 0; rows = 6;
    }
    private void preference(String key, int wanted) throws Exception {
        await(() -> activity.preference(key, Integer.MIN_VALUE) == wanted, "native action persists " + key + "=" + wanted);
    }
    private void toggle(int row, String key, int wanted) throws Exception {
        go(row); key(KeyEvent.KEYCODE_DPAD_CENTER); preference(key, wanted);
    }
    private void choice(int row, String key, int wanted, int minimum, int step, int count) throws Exception {
        int old = activity.preference(key, Integer.MIN_VALUE);
        open(row, VALUES, count); focus = (old - minimum) / step;
        go((wanted - minimum) / step); key(KeyEvent.KEYCODE_DPAD_CENTER);
        int[] previous = parents.pop(); state(previous[0]);
        page = previous[0]; focus = previous[1]; rows = previous[2]; preference(key, wanted);
    }
    private void unchangedChoice(int row, String key, int wanted, int count) throws Exception {
        open(row, VALUES, count);
        // No movement: the selected row must come from the recreated native preference.
        key(KeyEvent.KEYCODE_DPAD_CENTER);
        int[] previous = parents.pop(); state(previous[0]);
        page = previous[0]; focus = previous[1]; rows = previous[2]; preference(key, wanted);
    }
    private void screenshot(String name) throws Exception {
        Bitmap image = getUiAutomation().takeScreenshot(); check(image != null, "settings screenshot " + name);
        try (FileOutputStream output = new FileOutputStream(new File(getTargetContext().getFilesDir(),
                "test-settings-" + (landscape ? "landscape-" : "portrait-") + name + ".png"))) {
            image.compress(Bitmap.CompressFormat.PNG, 100, output);
        } finally { image.recycle(); }
    }
    private void launch() throws Exception {
        activity = (MainActivity) startActivitySync(new Intent(getTargetContext(), MainActivity.class).addFlags(Intent.FLAG_ACTIVITY_NEW_TASK));
        check(getUiAutomation().setRotation(landscape ? UiAutomation.ROTATION_FREEZE_90 : UiAutomation.ROTATION_FREEZE_0), "settings rotation requested");
        await(() -> { int[] safe = activity.safeWindowArea(); return safe != null && (safe[0] > safe[1]) == landscape; }, "settings safe viewport orientation");
        state(0); settingsRoot();
    }
    private void finishActivity() throws Exception {
        if (activity != null) { MainActivity previous = activity; runOnMainSync(previous::finish); activity = null; waitForIdleSync(); SystemClock.sleep(700); }
    }
    private void firstPass() throws Exception {
        screenshot("root");
        open(0, UI, 2); screenshot("interface");
        open(1, NOTIFICATIONS, 2); toggle(0, "notify_cards", 0); toggle(1, "notify_progress", 0);
        screenshot("notifications"); back(); back();
        open(1, VIDEO, 3); toggle(0, "keep_awake", 0);
        choice(1, "video_integer", 1, 0, 1, 2); choice(2, "video_smooth", 1, 0, 1, 2);
        screenshot("video"); back();
        open(2, AUDIO, 2); toggle(0, "audio", 0); choice(1, "audio_volume", 30, 0, 10, 11);
        screenshot("audio"); back();
        open(3, COMMANDS, 2); screenshot("commands");
        open(0, TACTILE, 3); toggle(0, "show_input", 0); choice(1, "overlay_opacity", 30, 10, 10, 10);
        open(2, LAYOUT, 3); choice(0, "overlay_size", 120, 100, 10, 4); choice(1, "overlay_position", 10, -15, 5, 7);
        screenshot("layout"); go(2); key(KeyEvent.KEYCODE_DPAD_CENTER);
        preference("overlay_size", 100); preference("overlay_position", 0);
        choice(0, "overlay_size", 110, 100, 10, 4); back(); screenshot("tactile"); back();
        open(1, CONTROLLERS, 3); screenshot("controllers");
        open(2, BINDINGS, 11); screenshot("bindings"); back(); back(); back();
        open(4, SAVES, 2); choice(0, "backup_count", 3, 1, 1, 3); toggle(1, "auto_state", 1);
        screenshot("saves"); back();
        open(5, RA, 2); screenshot("retroachievements");
        open(1, ACCOUNT, 4); check(!preferences.contains("password"), "opening account never persists a password");
        back(); back(); screenshot("root-scrolled");
        check(preferences.getAll().get("audio") instanceof Boolean && preferences.getAll().get("keep_awake") instanceof Boolean
                && preferences.getAll().get("show_input") instanceof Integer && preferences.getAll().get("theme") instanceof String,
                "historical Android preference types remain unchanged");
    }
    private void recreatedPass() throws Exception {
        open(0, UI, 2); open(1, NOTIFICATIONS, 2);
        toggle(0, "notify_cards", 1); toggle(1, "notify_progress", 1); back(); back();
        open(1, VIDEO, 3); toggle(0, "keep_awake", 1);
        unchangedChoice(1, "video_integer", 1, 2); unchangedChoice(2, "video_smooth", 1, 2); back();
        open(2, AUDIO, 2); toggle(0, "audio", 1); unchangedChoice(1, "audio_volume", 30, 11); back();
        open(3, COMMANDS, 2); open(0, TACTILE, 3); toggle(0, "show_input", 1);
        unchangedChoice(1, "overlay_opacity", 30, 10); open(2, LAYOUT, 3);
        unchangedChoice(0, "overlay_size", 110, 4); unchangedChoice(1, "overlay_position", 0, 7); back(); back(); back();
        open(4, SAVES, 2); unchangedChoice(0, "backup_count", 3, 3); toggle(1, "auto_state", 0); back();
        screenshot("recreated");
    }
    private void execute() throws Exception {
        preferences = getTargetContext().getSharedPreferences("corvus", Context.MODE_PRIVATE);
        Map<String, Object> previous = new HashMap<>();
        Map<String, ?> all = preferences.getAll();
        for (String key : INTEGER_KEYS) previous.put(key, all.get(key));
        for (String key : new String[] {"audio", "keep_awake", "theme"}) previous.put(key, all.get(key));
        SharedPreferences.Editor baseline = preferences.edit();
        for (int i = 0; i < INTEGER_KEYS.length; ++i) baseline.putInt(INTEGER_KEYS[i], DEFAULTS[i]);
        baseline.putBoolean("audio", true).putBoolean("keep_awake", true).putString("theme", "corvus");
        check(baseline.commit(), "isolated preference baseline persisted");
        try { launch(); firstPass(); finishActivity(); launch(); recreatedPass(); }
        finally {
            finishActivity();
            SharedPreferences.Editor restore = preferences.edit();
            for (Map.Entry<String, Object> entry : previous.entrySet()) {
                String key = entry.getKey(); Object value = entry.getValue(); restore.remove(key);
                if (value instanceof Integer) restore.putInt(key, (Integer)value);
                else if (value instanceof Boolean) restore.putBoolean(key, (Boolean)value);
                else if (value instanceof String) restore.putString(key, (String)value);
            }
            check(restore.commit(), "only touched preferences restored to their original typed values");
            getUiAutomation().setRotation(UiAutomation.ROTATION_UNFREEZE);
        }
    }
    @Override public void onCreate(Bundle arguments) {
        super.onCreate(arguments); landscape = "landscape".equals(arguments.getString("orientation", "portrait")); start();
    }
    @Override public void onStart() {
        Bundle result = new Bundle();
        try { execute(); result.putString("stream", "PASS " + checks + " Android native settings checks orientation="
                    + (landscape ? "landscape" : "portrait") + "\n"); finish(Activity.RESULT_OK, result); }
        catch (Throwable failure) { result.putString("stream", "FAIL " + android.util.Log.getStackTraceString(failure) + "\n"); finish(Activity.RESULT_CANCELED, result); }
    }
}
