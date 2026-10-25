// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
package com.corvussoftware.corvusretro;

import android.app.Activity;
import android.app.AlertDialog;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.os.Bundle;
import android.view.MotionEvent;
import android.view.View;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;
import java.io.InputStream;
import java.nio.charset.StandardCharsets;

public final class MainActivity extends Activity {
    private DemoView demo;
    private boolean paused;
    private Button pauseButton;
    private int input;
    private boolean touchClick;

    @Override public void onCreate(Bundle state) {
        super.onCreate(state);
        NativeBridge.reset();
        ScrollView scroll = new ScrollView(this);
        scroll.setFillViewport(true);
        scroll.setBackgroundColor(Color.rgb(12, 17, 29));
        LinearLayout content = new LinearLayout(this);
        content.setOrientation(LinearLayout.VERTICAL);
        content.setPadding(dp(24), dp(28), dp(24), dp(24));
        scroll.addView(content);
        content.addView(label("CORVUS / MOBILE", 13, 0xff57e0c2));
        content.addView(label("CorvusRetro", 34, Color.WHITE));
        content.addView(label("Votre espace rétro, signé CorvusSoftware.", 16, 0xffb3bed4));
        content.addView(label("LABORATOIRE  •  DÉMO NATIVE C", 13, 0xffb9a0ff));
        demo = new DemoView();
        content.addView(demo, new LinearLayout.LayoutParams(-1, dp(240)));
        content.addView(label("Déplacez le carré avec les commandes.\nCette démo maison ne lance pas encore de jeux.", 15, 0xffb3bed4));

        LinearLayout controls = row();
        addDirection(controls, "←", "Gauche", 4);
        addDirection(controls, "↑", "Haut", 1);
        addDirection(controls, "↓", "Bas", 2);
        addDirection(controls, "→", "Droite", 8);
        content.addView(controls);
        LinearLayout actions = row();
        pauseButton = button("Pause");
        pauseButton.setOnClickListener(v -> {
            paused = !paused;
            pauseButton.setText(paused ? "Reprendre" : "Pause");
        });
        actions.addView(pauseButton, weighted());
        Button reset = button("Réinitialiser");
        reset.setOnClickListener(v -> {
            input = 0;
            paused = false;
            pauseButton.setText("Pause");
            NativeBridge.reset();
        });
        actions.addView(reset, weighted());
        content.addView(actions);
        Button about = button("À propos et licence");
        about.setOnClickListener(v -> showLicense());
        content.addView(about);
        content.addView(label("© 2026 CorvusSoftware · AGPL-3.0-only\nVersion 0.1.0 · Prototype Android", 12, 0xffb3bed4));
        setContentView(scroll);
        // Respect system bars, including Android 15's edge-to-edge layout.
        scroll.setOnApplyWindowInsetsListener((view, insets) -> {
            view.setPadding(insets.getSystemWindowInsetLeft(), insets.getSystemWindowInsetTop(),
                    insets.getSystemWindowInsetRight(), insets.getSystemWindowInsetBottom());
            return insets;
        });
    }

    private void showLicense() {
        String license;
        try (InputStream stream = getAssets().open("LICENSE.txt")) {
            java.io.ByteArrayOutputStream bytes = new java.io.ByteArrayOutputStream();
            byte[] buffer = new byte[4096];
            int count;
            while ((count = stream.read(buffer)) != -1) bytes.write(buffer, 0, count);
            license = bytes.toString(StandardCharsets.UTF_8.name());
        } catch (java.io.IOException error) {
            license = "Licence indisponible. Consultez le fichier LICENSE du dépôt.";
        }
        new AlertDialog.Builder(this).setTitle("CorvusRetro · CorvusSoftware")
                .setMessage("Copyright (C) 2026 CorvusSoftware\nAGPL-3.0-only · Aucune garantie.\n\n" + license)
                .setPositiveButton("Fermer", null).show();
    }

    private int dp(int value) { return Math.round(value * getResources().getDisplayMetrics().density); }
    private LinearLayout row() { return new LinearLayout(this); }
    private LinearLayout.LayoutParams weighted() { return new LinearLayout.LayoutParams(0, dp(56), 1); }
    private TextView label(String text, int size, int color) {
        TextView view = new TextView(this);
        view.setText(text); view.setTextSize(size); view.setTextColor(color);
        view.setPadding(0, dp(8), 0, dp(12));
        return view;
    }
    private Button button(String text) {
        Button view = new Button(this);
        view.setText(text); view.setAllCaps(false);
        view.setTextColor(Color.WHITE);
        view.setBackgroundTintList(android.content.res.ColorStateList.valueOf(0xff34304f));
        view.setMinHeight(dp(48));
        return view;
    }
    private void addDirection(LinearLayout parent, String symbol, String description, int mask) {
        Button view = button(symbol);
        view.setContentDescription(description);
        view.setOnTouchListener((button, event) -> {
            switch (event.getActionMasked()) {
                case MotionEvent.ACTION_DOWN:
                    input |= mask;
                    button.setPressed(true);
                    button.getParent().requestDisallowInterceptTouchEvent(true);
                    return true;
                case MotionEvent.ACTION_UP:
                case MotionEvent.ACTION_CANCEL:
                    input &= ~mask;
                    button.setPressed(false);
                    if (event.getActionMasked() == MotionEvent.ACTION_UP) {
                        touchClick = true;
                        try { button.performClick(); } finally { touchClick = false; }
                    }
                    return true;
                default: return true;
            }
        });
        // Keyboard and accessibility activation move one step as well.
        view.setOnClickListener(v -> {
            if (!touchClick && input == 0) {
                NativeBridge.frame(demo.pixels, mask, paused);
                demo.invalidate();
            }
        });
        parent.addView(view, weighted());
    }

    @Override protected void onResume() {
        super.onResume();
        if (demo != null) demo.start();
    }
    @Override protected void onPause() {
        input = 0;
        if (demo != null) demo.stop();
        super.onPause();
    }

    private final class DemoView extends View {
        private final int[] pixels = new int[256 * 192];
        private final Bitmap bitmap = Bitmap.createBitmap(256, 192, Bitmap.Config.ARGB_8888);
        private final Paint paint = new Paint();
        private final android.graphics.RectF destination = new android.graphics.RectF();
        private boolean active;
        private final Runnable tick = new Runnable() {
            @Override public void run() {
                if (!active) return;
                NativeBridge.frame(pixels, input, paused);
                bitmap.setPixels(pixels, 0, 256, 0, 0, 256, 192);
                invalidate();
                postDelayed(this, 16);
            }
        };
        DemoView() {
            super(MainActivity.this);
            setContentDescription("Démo native : carré mobile sur une grille");
        }
        void start() { if (!active) { active = true; post(tick); } }
        void stop() { active = false; removeCallbacks(tick); }
        @Override protected void onDetachedFromWindow() { stop(); super.onDetachedFromWindow(); }
        @Override protected void onDraw(Canvas canvas) {
            super.onDraw(canvas);
            float scale = Math.min(getWidth() / 256f, getHeight() / 192f);
            float width = 256 * scale, height = 192 * scale;
            float left = (getWidth() - width) / 2, top = (getHeight() - height) / 2;
            destination.set(left, top, left + width, top + height);
            canvas.drawBitmap(bitmap, null, destination, paint);
        }
    }
}
