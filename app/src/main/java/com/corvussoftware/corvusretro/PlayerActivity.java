// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
package com.corvussoftware.corvusretro;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Intent;
import android.os.Bundle;
import android.text.TextUtils;
import android.view.Gravity;
import android.view.KeyEvent;
import android.view.View;
import android.view.WindowManager;
import android.widget.FrameLayout;
import android.widget.ImageButton;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.Toast;

public final class PlayerActivity extends Activity {
    private EmulationSession session;
    private GameLibrary.Game game;
    private FrameSurface surface;
    private FrameLayout controls;
    private boolean ready;
    private AlertDialog menu;
    private int dp(int value) { return Ui.dp(this, value); }
    @Override public void onCreate(Bundle state) {
        super.onCreate(state);
        GameLibrary library = new GameLibrary(this);
        game = library.find(getIntent().getStringExtra("game"));
        if (game == null) { Toast.makeText(this, R.string.import_error, Toast.LENGTH_LONG).show(); finish(); return; }
        if (library.preferences.getBoolean("keep_awake", true)) getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        LinearLayout root = Ui.column(this); root.setBackgroundColor(0xff000000);
        LinearLayout toolbar = Ui.row(this); toolbar.setBackgroundColor(Ui.SURFACE);
        toolbar.setPadding(dp(16), dp(4), dp(8), dp(4));
        LinearLayout title = Ui.column(this);
        TextView filename = Ui.text(this, R.string.app_name, 15, Ui.TEXT, true);
        filename.setText(game.title); filename.setSingleLine(true); filename.setEllipsize(TextUtils.TruncateAt.END);
        title.addView(filename); Ui.space(title, 3);
        TextView core = Ui.text(this, R.string.app_name, 11, Ui.MUTED, false);
        core.setText(CoreCatalog.NAMES[game.system] + " · " + CoreCatalog.VERSION); title.addView(core);
        toolbar.addView(title, new LinearLayout.LayoutParams(0, -2, 1));
        ImageButton quick = new ImageButton(this); quick.setImageResource(R.drawable.ic_menu);
        quick.setImageTintList(android.content.res.ColorStateList.valueOf(Ui.TEXT));
        quick.setBackground(Ui.ripple(this, Ui.SURFACE, 0)); quick.setContentDescription(getString(R.string.quick_menu));
        quick.setOnClickListener(view -> quickMenu());
        toolbar.addView(quick, new LinearLayout.LayoutParams(dp(48), dp(48))); root.addView(toolbar);
        FrameLayout field = new FrameLayout(this);
        surface = new FrameSurface(this); field.addView(surface, new FrameLayout.LayoutParams(-1, -1));
        TextView loading = Ui.text(this, R.string.loading_game, 15, Ui.TEXT, false); loading.setGravity(Gravity.CENTER);
        field.addView(loading, new FrameLayout.LayoutParams(-1, -1));
        controls = new FrameLayout(this); field.addView(controls, new FrameLayout.LayoutParams(-1, -1));
        root.addView(field, new LinearLayout.LayoutParams(-1, 0, 1));
        session = new EmulationSession(library, game, surface, new EmulationSession.Listener() {
            @Override public void ready(int width, int height) {
                ready = true; surface.configure(width, height); loading.setVisibility(View.GONE); layoutControls(field.getWidth(), field.getHeight());
            }
            @Override public void message(String message) { Toast.makeText(PlayerActivity.this, message, Toast.LENGTH_LONG).show(); }
            @Override public void failed(String message) {
                ready = false;
                if (isFinishing() || isDestroyed()) return;
                new AlertDialog.Builder(PlayerActivity.this).setTitle(R.string.import_error).setMessage(message)
                        .setPositiveButton(R.string.close, (dialog, which) -> finish()).setOnCancelListener(dialog -> finish()).show();
            }
        });
        field.addOnLayoutChangeListener((view, left, top, right, bottom, oldLeft, oldTop, oldRight, oldBottom) -> {
            if (right - left != oldRight - oldLeft || bottom - top != oldBottom - oldTop) layoutControls(right - left, bottom - top);
        });
        Ui.insets(this, root);
    }
    private void layoutControls(int width, int height) {
        if (session == null || width <= 0 || height <= 0) return;
        session.clearInput(); controls.removeAllViews();
        boolean portrait = height > width;
        surface.reserveControls(portrait ? dp(game.system == 2 ? 256 : 216) : 0);
        if (!ready) return;
        LinearLayout dpad = Ui.column(this);
        int[][] buttons = {{-1, 0, -1}, {2, -1, 3}, {-1, 1, -1}};
        String[][] labels = {{"", "↑", ""}, {"←", "", "→"}, {"", "↓", ""}};
        int[] names = {R.string.up, R.string.down, R.string.left, R.string.right};
        for (int y = 0; y < 3; y++) {
            LinearLayout row = Ui.row(this);
            for (int x = 0; x < 3; x++) {
                int button = buttons[y][x];
                View cell = button < 0 ? new View(this) : new CoreInputButton(this, labels[y][x], names[button], button, session);
                LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(dp(48), dp(48));
                params.setMargins(dp(2), dp(2), dp(2), dp(2)); row.addView(cell, params);
            }
            dpad.addView(row);
        }
        FrameLayout.LayoutParams left = new FrameLayout.LayoutParams(dp(156), dp(156), Gravity.BOTTOM | Gravity.START);
        left.leftMargin = dp(8); left.bottomMargin = dp(portrait ? 56 : 20); controls.addView(dpad, left);
        FrameLayout ab = new FrameLayout(this);
        FrameLayout.LayoutParams a = new FrameLayout.LayoutParams(dp(52), dp(52), Gravity.TOP | Gravity.END);
        ab.addView(new CoreInputButton(this, "A", R.string.button_a, 4, session), a);
        FrameLayout.LayoutParams b = new FrameLayout.LayoutParams(dp(52), dp(52), Gravity.BOTTOM | Gravity.START);
        ab.addView(new CoreInputButton(this, "B", R.string.button_b, 5, session), b);
        FrameLayout.LayoutParams right = new FrameLayout.LayoutParams(dp(112), dp(112), Gravity.BOTTOM | Gravity.END);
        right.rightMargin = dp(16); right.bottomMargin = dp(portrait ? 84 : 32); controls.addView(ab, right);
        LinearLayout center = Ui.row(this);
        center.addView(new CoreInputButton(this, "SELECT", R.string.button_select, 7, session), new LinearLayout.LayoutParams(dp(68), dp(48)));
        View gap = new View(this); center.addView(gap, new LinearLayout.LayoutParams(dp(12), 1));
        center.addView(new CoreInputButton(this, "START", R.string.button_start, 6, session), new LinearLayout.LayoutParams(dp(68), dp(48)));
        FrameLayout.LayoutParams middle = new FrameLayout.LayoutParams(-2, -2, Gravity.BOTTOM | Gravity.CENTER_HORIZONTAL);
        middle.bottomMargin = dp(4); controls.addView(center, middle);
        if (game.system == 2) {
            FrameLayout.LayoutParams l = new FrameLayout.LayoutParams(dp(68), dp(48), (portrait ? Gravity.BOTTOM : Gravity.TOP) | Gravity.START);
            l.leftMargin = dp(16); if (portrait) l.bottomMargin = dp(208); else l.topMargin = dp(8);
            controls.addView(new CoreInputButton(this, "L", R.string.button_l, 8, session), l);
            FrameLayout.LayoutParams r = new FrameLayout.LayoutParams(dp(68), dp(48), (portrait ? Gravity.BOTTOM : Gravity.TOP) | Gravity.END);
            r.rightMargin = dp(16); if (portrait) r.bottomMargin = dp(208); else r.topMargin = dp(8);
            controls.addView(new CoreInputButton(this, "R", R.string.button_r, 9, session), r);
        }
    }
    private void quickMenu() {
        if (!ready || session == null || (menu != null && menu.isShowing())) return;
        session.paused(true);
        String[] items = {getString(R.string.resume_content), getString(R.string.restart_content), getString(R.string.save_state), getString(R.string.load_state), getString(R.string.close_content)};
        menu = new AlertDialog.Builder(this).setTitle(R.string.quick_menu).setItems(items, (dialog, which) -> {
            if (which == 1) session.reset();
            else if (which == 2) session.saveState();
            else if (which == 3) session.loadState();
            else if (which == 4) finish();
        }).create();
        menu.setOnDismissListener(dialog -> { if (!isFinishing() && session != null) session.paused(false); });
        menu.show();
    }
    @Override public void onBackPressed() { if (ready) quickMenu(); else finish(); }
    private int keyButton(int key) {
        switch (key) {
            case KeyEvent.KEYCODE_DPAD_UP: return 0;
            case KeyEvent.KEYCODE_DPAD_DOWN: return 1;
            case KeyEvent.KEYCODE_DPAD_LEFT: return 2;
            case KeyEvent.KEYCODE_DPAD_RIGHT: return 3;
            case KeyEvent.KEYCODE_BUTTON_A: case KeyEvent.KEYCODE_Z: return 4;
            case KeyEvent.KEYCODE_BUTTON_B: case KeyEvent.KEYCODE_X: return 5;
            case KeyEvent.KEYCODE_BUTTON_START: case KeyEvent.KEYCODE_ENTER: return 6;
            case KeyEvent.KEYCODE_BUTTON_SELECT: case KeyEvent.KEYCODE_SPACE: return 7;
            case KeyEvent.KEYCODE_BUTTON_L1: return 8;
            case KeyEvent.KEYCODE_BUTTON_R1: return 9;
            default: return -1;
        }
    }
    @Override public boolean onKeyDown(int key, KeyEvent event) {
        int button = keyButton(key);
        if (button >= 0 && session != null && ready && (menu == null || !menu.isShowing())) { session.press(1 << button); return true; }
        return super.onKeyDown(key, event);
    }
    @Override public boolean onKeyUp(int key, KeyEvent event) {
        int button = keyButton(key);
        if (button >= 0 && session != null && ready) { session.release(1 << button); return true; }
        return super.onKeyUp(key, event);
    }
    @Override public void onWindowFocusChanged(boolean focused) { super.onWindowFocusChanged(focused); if (!focused && session != null) session.clearInput(); }
    @Override protected void onResume() { super.onResume(); if (session != null) session.foreground(true); }
    @Override protected void onPause() { if (session != null) session.foreground(false); super.onPause(); }
    @Override protected void onDestroy() {
        if (menu != null) { menu.setOnDismissListener(null); menu.dismiss(); }
        if (session != null) session.close();
        super.onDestroy();
    }
    @Override protected void onNewIntent(Intent intent) {
        super.onNewIntent(intent);
        if (game == null || !game.id.equals(intent.getStringExtra("game"))) { setIntent(intent); recreate(); }
    }
}
