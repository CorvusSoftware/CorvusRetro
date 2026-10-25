// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
package com.corvussoftware.corvusretro;

import android.app.Activity;
import android.os.Bundle;
import android.view.Gravity;
import android.view.View;
import android.widget.Button;
import android.widget.ImageButton;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;

public final class GameActivity extends Activity {
    private DemoView demo;
    private boolean paused;
    private Button pauseButton;
    private TextView status;
    private int dp(int value) { return Ui.dp(this, value); }

    @Override public void onCreate(Bundle state) {
        super.onCreate(state);
        NativeBridge.reset();
        LinearLayout root = Ui.column(this);
        root.setBackgroundColor(Ui.BG);
        LinearLayout header = Ui.row(this);
        header.setPadding(dp(12), dp(12), dp(20), dp(12));
        ImageButton back = new ImageButton(this);
        back.setImageResource(R.drawable.ic_back);
        back.setImageTintList(android.content.res.ColorStateList.valueOf(Ui.TEXT));
        back.setBackground(Ui.ripple(this, Ui.BG, 14));
        back.setContentDescription(getString(R.string.back));
        back.setOnClickListener(view -> finish());
        header.addView(back, new LinearLayout.LayoutParams(dp(48), dp(48)));
        LinearLayout title = Ui.column(this);
        title.addView(Ui.text(this, R.string.lab, 20, Ui.TEXT, true));
        Ui.space(title, 4);
        title.addView(Ui.text(this, R.string.demo_subtitle, 12, Ui.MUTED, false));
        header.addView(title, new LinearLayout.LayoutParams(0, -2, 1));
        root.addView(header);
        ScrollView scroll = new ScrollView(this);
        scroll.setFillViewport(true);
        LinearLayout body = Ui.column(this);
        body.setPadding(dp(20), dp(16), dp(20), dp(24));
        scroll.addView(body);
        root.addView(scroll, new LinearLayout.LayoutParams(-1, 0, 1));
        LinearLayout screen = Ui.card(this);
        screen.setPadding(dp(12), dp(16), dp(12), dp(12));
        LinearLayout screenHeading = Ui.row(this);
        TextView name = Ui.text(this, R.string.screen_label, 10, Ui.MUTED, true);
        name.setLetterSpacing(.12f);
        screenHeading.addView(name, new LinearLayout.LayoutParams(0, -2, 1));
        status = Ui.badge(this, R.string.playing);
        screenHeading.addView(status);
        screen.addView(screenHeading);
        Ui.space(screen, 16);
        demo = new DemoView(this);
        screen.addView(demo, new LinearLayout.LayoutParams(-1, -2));
        body.addView(screen);
        Ui.space(body, 16);
        TextView hint = Ui.text(this, R.string.game_hint, 13, Ui.MUTED, false);
        hint.setGravity(Gravity.CENTER);
        body.addView(hint);
        Ui.space(body, 30);
        body.addView(Ui.text(this, R.string.controls, 19, Ui.TEXT, true));
        Ui.space(body, 8);
        body.addView(Ui.text(this, R.string.controls_hint, 13, Ui.MUTED, false));
        Ui.space(body, 20);
        LinearLayout controls = Ui.row(this);
        LinearLayout dpad = Ui.column(this);
        String[][] symbols = {{"", "↑", ""}, {"←", "", "→"}, {"", "↓", ""}};
        int[][] masks = {{0, 1, 0}, {4, 0, 8}, {0, 2, 0}};
        int[][] descriptions = {{0, R.string.up, 0}, {R.string.left, 0, R.string.right}, {0, R.string.down, 0}};
        for (int y = 0; y < 3; ++y) {
            LinearLayout row = Ui.row(this);
            for (int x = 0; x < 3; ++x) {
                View cell;
                if (masks[y][x] != 0) {
                    cell = new DirectionButton(this, symbols[y][x], descriptions[y][x], masks[y][x], demo);
                } else {
                    cell = new View(this);
                    if (x == 1 && y == 1) cell.setBackground(Ui.shape(this, 0xff1b242a, 12, false));
                }
                LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(dp(48), dp(48));
                params.setMargins(dp(2), dp(2), dp(2), dp(2));
                row.addView(cell, params);
            }
            dpad.addView(row);
        }
        controls.addView(dpad);
        LinearLayout actions = Ui.column(this);
        pauseButton = Ui.button(this, R.string.pause, true);
        pauseButton.setOnClickListener(view -> {
            paused = !paused;
            demo.setPaused(paused);
            updatePause();
        });
        actions.addView(pauseButton, new LinearLayout.LayoutParams(-1, -2));
        Ui.space(actions, 12);
        Button reset = Ui.button(this, R.string.reset, false);
        reset.setOnClickListener(view -> {
            paused = false;
            demo.reset();
            updatePause();
        });
        actions.addView(reset, new LinearLayout.LayoutParams(-1, -2));
        LinearLayout.LayoutParams actionsParams = new LinearLayout.LayoutParams(0, -2, 1);
        actionsParams.leftMargin = dp(20);
        controls.addView(actions, actionsParams);
        body.addView(controls);
        Ui.space(body, 24);
        TextView note = Ui.text(this, R.string.demo_note, 12, Ui.MUTED, false);
        note.setGravity(Gravity.CENTER);
        body.addView(note);
        Ui.insets(this, root);
    }
    private void updatePause() {
        pauseButton.setText(paused ? R.string.resume : R.string.pause);
        status.setText(paused ? R.string.paused : R.string.playing);
    }
    @Override protected void onResume() { super.onResume(); if (demo != null) demo.start(); }
    @Override protected void onPause() { if (demo != null) demo.stop(); super.onPause(); }
}
