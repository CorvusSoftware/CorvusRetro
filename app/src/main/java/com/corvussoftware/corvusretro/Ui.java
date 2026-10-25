// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
package com.corvussoftware.corvusretro;

import android.app.Activity;
import android.content.Context;
import android.content.res.ColorStateList;
import android.graphics.Insets;
import android.graphics.Typeface;
import android.graphics.drawable.GradientDrawable;
import android.graphics.drawable.RippleDrawable;
import android.os.Build;
import android.view.Gravity;
import android.view.View;
import android.view.WindowInsets;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;

final class Ui {
    static final int BG = 0xff0d1117, SURFACE = 0xff171d26, BORDER = 0xff29313d;
    static final int TEXT = 0xfff2f5f8, MUTED = 0xff919dac, ACCENT = 0xffc5f277;
    private Ui() {}
    static int dp(Context c, int value) {
        return Math.round(value * c.getResources().getDisplayMetrics().density);
    }
    static GradientDrawable shape(Context c, int color, int radius, boolean border) {
        GradientDrawable result = new GradientDrawable();
        result.setColor(color);
        result.setCornerRadius(dp(c, radius));
        if (border) result.setStroke(dp(c, 1), BORDER);
        return result;
    }
    static RippleDrawable ripple(Context c, int color, int radius) {
        return new RippleDrawable(ColorStateList.valueOf(0x335e7953),
                shape(c, color, radius, false), shape(c, 0xffffffff, radius, false));
    }
    static LinearLayout column(Context c) {
        LinearLayout result = new LinearLayout(c);
        result.setOrientation(LinearLayout.VERTICAL);
        return result;
    }
    static LinearLayout row(Context c) {
        LinearLayout result = new LinearLayout(c);
        result.setGravity(Gravity.CENTER_VERTICAL);
        return result;
    }
    static TextView text(Context c, int resource, int size, int color, boolean medium) {
        TextView result = new TextView(c);
        result.setText(resource);
        result.setTextSize(size);
        result.setTextColor(color);
        result.setTypeface(Typeface.create(medium ? "sans-serif-medium" : "sans-serif", Typeface.NORMAL));
        result.setIncludeFontPadding(false);
        result.setLineSpacing(dp(c, 3), 1);
        return result;
    }
    static Button button(Context c, int resource, boolean primary) {
        Button result = new Button(c);
        result.setText(resource);
        result.setAllCaps(false);
        result.setTextSize(14);
        result.setTypeface(Typeface.create("sans-serif-medium", Typeface.NORMAL));
        result.setTextColor(primary ? 0xff14200d : TEXT);
        result.setBackground(ripple(c, primary ? ACCENT : BORDER, 16));
        result.setMinimumWidth(0);
        result.setMinWidth(0);
        result.setMinHeight(dp(c, 52));
        result.setPadding(dp(c, 12), dp(c, 8), dp(c, 12), dp(c, 8));
        return result;
    }
    static ImageView icon(Context c, int resource, int color) {
        ImageView result = new ImageView(c);
        result.setImageResource(resource);
        result.setImageTintList(ColorStateList.valueOf(color));
        result.setImportantForAccessibility(View.IMPORTANT_FOR_ACCESSIBILITY_NO);
        return result;
    }
    static TextView badge(Context c, int resource) {
        TextView result = text(c, resource, 10, ACCENT, true);
        result.setLetterSpacing(.12f);
        result.setPadding(dp(c, 10), dp(c, 7), dp(c, 10), dp(c, 7));
        result.setBackground(shape(c, 0xff273124, 8, false));
        return result;
    }
    static void space(LinearLayout parent, int height) {
        parent.addView(new View(parent.getContext()), new LinearLayout.LayoutParams(1, dp(parent.getContext(), height)));
    }
    static LinearLayout card(Context c) {
        LinearLayout result = column(c);
        result.setPadding(dp(c, 20), dp(c, 20), dp(c, 20), dp(c, 20));
        result.setBackground(shape(c, SURFACE, 24, true));
        return result;
    }
    @SuppressWarnings("deprecation")
    static void insets(Activity activity, View root) {
        root.setOnApplyWindowInsetsListener((view, insets) -> {
            if (Build.VERSION.SDK_INT >= 30) {
                Insets bars = insets.getInsets(WindowInsets.Type.systemBars() | WindowInsets.Type.displayCutout());
                view.setPadding(bars.left, bars.top, bars.right, bars.bottom);
            } else {
                view.setPadding(insets.getSystemWindowInsetLeft(), insets.getSystemWindowInsetTop(),
                        insets.getSystemWindowInsetRight(), insets.getSystemWindowInsetBottom());
            }
            return insets;
        });
        activity.setContentView(root);
        root.requestApplyInsets();
    }
}
