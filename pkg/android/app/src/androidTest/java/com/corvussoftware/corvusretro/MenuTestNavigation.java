// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
package com.corvussoftware.corvusretro;

import android.app.Instrumentation;
import android.os.SystemClock;
import android.view.KeyEvent;
import android.view.MotionEvent;
import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.nio.charset.StandardCharsets;
import java.util.HashMap;
import java.util.Map;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/** Uses the native debug geometry, never a second implementation of its DPI or text layout. */
final class MenuTestNavigation {
    private static final Pattern FIELD = Pattern.compile("([a-z_]+)=(-?[0-9]+(?:\\.[0-9]+)?(?:[eE][-+]?[0-9]+)?)");
    static final class Layout {
        final float width, height, header, bottom, contentWidth, nav, rowY, rowHeight, label, hint, icon;
        final int focus;
        final boolean side;
        Layout(String line) {
            Map<String, Float> values = new HashMap<>();
            Matcher matcher = FIELD.matcher(line);
            while (matcher.find()) values.put(matcher.group(1), Float.parseFloat(matcher.group(2)));
            width = field(values, "width"); height = field(values, "height");
            header = field(values, "header"); bottom = field(values, "bottom");
            contentWidth = field(values, "content_width"); nav = field(values, "nav");
            rowY = field(values, "row_y"); rowHeight = field(values, "row_h");
            label = field(values, "label"); hint = field(values, "hint"); icon = field(values, "icon");
            focus = (int)field(values, "focus"); side = field(values, "side") != 0;
        }
        private static float field(Map<String, Float> values, String key) {
            Float value = values.get(key);
            if (value == null || !Float.isFinite(value)) throw new AssertionError("missing native menu metric " + key);
            return value;
        }
        boolean matches(MainActivity activity) {
            int[] area = activity.safeWindowArea();
            float density = activity.density();
            return area != null && Math.abs(width * density - (area[0] - area[2] - area[4])) <= 2
                    && Math.abs(height * density - (area[1] - area[3] - area[5])) <= 2;
        }
    }
    static final class Snapshot {
        String state = "";
        Layout layout;
    }
    private interface Ready { boolean ready(Snapshot snapshot); }
    private MenuTestNavigation() { }
    static Snapshot read() throws Exception {
        Process process = Runtime.getRuntime().exec(new String[] {"logcat", "-d",
                "--pid=" + android.os.Process.myPid(), "-s", "CorvusRetro:D", "*:S"});
        Snapshot result = new Snapshot();
        try (BufferedReader reader = new BufferedReader(new InputStreamReader(process.getInputStream(), StandardCharsets.UTF_8))) {
            String line;
            while ((line = reader.readLine()) != null) {
                if (line.contains("frontend page=")) result.state = line;
                if (line.contains("menu_layout width=")) result.layout = new Layout(line);
            }
        }
        process.waitFor();
        return result;
    }
    private static Snapshot await(MainActivity activity, Ready ready, String description) throws Exception {
        long deadline = SystemClock.uptimeMillis() + 20000;
        Snapshot latest;
        do {
            latest = read();
            if (activity.hasWindowFocus() && latest.state.contains(" active=1 ")
                    && latest.layout != null && latest.layout.matches(activity) && ready.ready(latest)) return latest;
            SystemClock.sleep(80);
        } while (SystemClock.uptimeMillis() < deadline);
        throw new AssertionError(description + " / " + latest.state);
    }
    static Layout layout(MainActivity activity) throws Exception {
        return await(activity, snapshot -> true, "native menu geometry for the current window").layout;
    }
    static void page(MainActivity activity, int page) throws Exception {
        await(activity, snapshot -> snapshot.state.contains("frontend page=" + page + " "),
                "native menu page " + page);
    }
    static void key(Instrumentation test, MainActivity activity, int key) throws Exception {
        await(activity, snapshot -> true, "native input focus");
        test.sendKeyDownUpSync(key); SystemClock.sleep(80);
    }
    static Layout selectRow(Instrumentation test, MainActivity activity, int row) throws Exception {
        if (row < 0 || row > 512) throw new AssertionError("bounded menu row");
        int current = layout(activity).focus;
        int direction = row > current ? KeyEvent.KEYCODE_DPAD_DOWN : KeyEvent.KEYCODE_DPAD_UP;
        for (int i = 0; i < Math.abs(row - current); ++i) {
            test.sendKeyDownUpSync(direction); SystemClock.sleep(80);
        }
        return await(activity, snapshot -> snapshot.layout.focus == row, "selected native menu row " + row).layout;
    }
    static void tapRow(Instrumentation test, MainActivity activity, int row) throws Exception {
        Layout layout = selectRow(test, activity, row);
        float top = Math.max(layout.header, layout.rowY);
        float bottom = Math.min(layout.bottom, layout.rowY + layout.rowHeight);
        if (layout.rowHeight <= 0 || bottom <= top) throw new AssertionError("selected native row is not visible");
        tapLogical(test, activity, layout.contentWidth / 2, (top + bottom) / 2);
    }
    static void tapTab(Instrumentation test, MainActivity activity, int tab) throws Exception {
        if (tab < 0 || tab > 2) throw new AssertionError("navigation tab");
        Layout layout = layout(activity);
        float x = layout.side ? layout.width - layout.nav / 2 : layout.width * (tab + .5f) / 3;
        float y = layout.side ? layout.height * (tab + .5f) / 3 : layout.height - layout.nav / 2;
        tapLogical(test, activity, x, y);
        page(activity, tab);
    }
    static int[] origin(Instrumentation test, MainActivity activity) {
        int[] result = new int[2];
        test.runOnMainSync(() -> activity.getWindow().getDecorView().getLocationOnScreen(result));
        return result;
    }
    static void tapLogical(Instrumentation test, MainActivity activity, float x, float y) throws Exception {
        Layout layout = layout(activity);
        if (x < 0 || y < 0 || x >= layout.width || y >= layout.height)
            throw new AssertionError("touch outside actual safe native viewport");
        int[] safe = activity.safeWindowArea(), origin = origin(test, activity);
        float density = activity.density();
        float px = origin[0] + safe[2] + x * density, py = origin[1] + safe[3] + y * density;
        long time = SystemClock.uptimeMillis();
        MotionEvent down = MotionEvent.obtain(time, time, MotionEvent.ACTION_DOWN, px, py, 0);
        MotionEvent up = MotionEvent.obtain(time, time + 16, MotionEvent.ACTION_UP, px, py, 0);
        try { test.sendPointerSync(down); test.sendPointerSync(up); }
        finally { down.recycle(); up.recycle(); }
        SystemClock.sleep(400);
    }
}
