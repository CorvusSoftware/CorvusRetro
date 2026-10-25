// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
package com.corvussoftware.corvusretro;

import android.content.Context;
import android.view.HapticFeedbackConstants;
import android.view.MotionEvent;
import android.widget.Button;

/** Continuous touch input, with a separate one-step accessibility click. */
final class DirectionButton extends Button {
    private final DemoView demo;
    private final int mask;
    private boolean touchClick;
    DirectionButton(Context context, String symbol, int description, int mask, DemoView demo) {
        super(context);
        this.demo = demo;
        this.mask = mask;
        setText(symbol);
        setTextSize(24);
        setTextColor(Ui.TEXT);
        setContentDescription(context.getString(description));
        setPadding(0, 0, 0, 0);
        setBackground(Ui.ripple(context, Ui.BORDER, 12));
        setClickable(true);
        setFocusable(true);
    }
    @Override public boolean onTouchEvent(MotionEvent event) {
        if (!isEnabled()) return false;
        switch (event.getActionMasked()) {
            case MotionEvent.ACTION_DOWN:
                demo.press(mask);
                setPressed(true);
                getParent().requestDisallowInterceptTouchEvent(true);
                performHapticFeedback(HapticFeedbackConstants.KEYBOARD_TAP);
                return true;
            case MotionEvent.ACTION_UP:
            case MotionEvent.ACTION_CANCEL:
                demo.release(mask);
                setPressed(false);
                if (event.getActionMasked() == MotionEvent.ACTION_UP) {
                    touchClick = true;
                    try { performClick(); } finally { touchClick = false; }
                }
                return true;
            default: return true;
        }
    }
    @Override public boolean performClick() {
        super.performClick();
        if (!touchClick) demo.stepOnce(mask);
        return true;
    }
    @Override public void onWindowFocusChanged(boolean focused) {
        super.onWindowFocusChanged(focused);
        if (!focused) { demo.release(mask); setPressed(false); }
    }
}
