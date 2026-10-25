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
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;

final class Ui {
    static int BG, SURFACE, HEADER, NAV, BORDER, TEXT, MUTED, ACCENT, ICON, PASSIVE, HIGHLIGHT, CONTROL;
    private Ui() {}
    static int dp(Context c, int value) {
        return Math.round(value * c.getResources().getDisplayMetrics().density);
    }
    static boolean isShield(Context c) {
        return !"dark".equals(c.getSharedPreferences("corvus", Context.MODE_PRIVATE).getString("theme", "shield"));
    }
    static void applyTheme(Activity activity) {
        boolean shield = isShield(activity);
        activity.setTheme(shield ? R.style.CorvusTheme : R.style.CorvusThemeDark);
        BG = shield ? 0xff121212 : 0xff202124;
        HEADER = shield ? 0xff1f1f1f : 0xff292b30;
        NAV = shield ? 0xff242424 : 0xff292b30;
        SURFACE = shield ? 0xff1d1d1d : 0xff292b30;
        BORDER = shield ? 0xff498515 : 0xff36383e;
        TEXT = shield ? 0xffdedede : 0xfff1f3f4;
        MUTED = shield ? 0xff999999 : 0xffa4abb5;
        ACCENT = shield ? 0xff7ab547 : 0xff81c7ff;
        ICON = shield ? ACCENT : MUTED;
        PASSIVE = shield ? 0xff558b2f : MUTED;
        HIGHLIGHT = shield ? 0xff255d00 : 0xff243b50;
        CONTROL = shield ? 0xc0263320 : 0x9940444b;
    }
    static GradientDrawable shape(Context c, int color, int radius, boolean border) {
        GradientDrawable result = new GradientDrawable();
        result.setColor(color);
        result.setCornerRadius(dp(c, radius));
        if (border) result.setStroke(dp(c, 1), BORDER);
        return result;
    }
    static RippleDrawable ripple(Context c, int color, int radius) {
        return new RippleDrawable(ColorStateList.valueOf((ACCENT & 0x00ffffff) | 0x44000000),
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
    static android.graphics.drawable.Drawable listBackground(Context c) {
        android.graphics.drawable.StateListDrawable states = new android.graphics.drawable.StateListDrawable();
        states.addState(new int[]{android.R.attr.state_focused}, shape(c, HIGHLIGHT, 0, false));
        states.addState(new int[]{}, ripple(c, BG, 0));
        return states;
    }
    static ImageView icon(Context c, int resource, int color) {
        ImageView result = new ImageView(c);
        result.setImageResource(resource);
        result.setImageTintList(ColorStateList.valueOf(color));
        result.setImportantForAccessibility(View.IMPORTANT_FOR_ACCESSIBILITY_NO);
        return result;
    }
    static void space(LinearLayout parent, int height) {
        parent.addView(new View(parent.getContext()), new LinearLayout.LayoutParams(1, dp(parent.getContext(), height)));
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
