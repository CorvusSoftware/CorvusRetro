// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
package com.corvussoftware.corvusretro;

import android.content.Context;
import android.view.HapticFeedbackConstants;
import android.view.MotionEvent;
import android.widget.Button;

final class CoreInputButton extends Button {
    private final EmulationSession session;
    private final int mask;
    private boolean touchClick;
    CoreInputButton(Context context, String label, int description, int button, EmulationSession session) {
        super(context);
        this.session = session; mask = 1 << button;
        setText(label); setTextSize(label.length() > 2 ? 11 : 22); setAllCaps(false);
        setTextColor(0xffeeeeee); setPadding(0, 0, 0, 0);
        setContentDescription(context.getString(description));
        setBackground(Ui.ripple(context, Ui.CONTROL, 28));
        setMinimumWidth(0); setMinWidth(0); setClickable(true); setFocusable(true);
    }
    @Override public boolean onTouchEvent(MotionEvent event) {
        if (!isEnabled()) return false;
        switch (event.getActionMasked()) {
            case MotionEvent.ACTION_DOWN:
                session.press(mask); setPressed(true);
                getParent().requestDisallowInterceptTouchEvent(true);
                performHapticFeedback(HapticFeedbackConstants.KEYBOARD_TAP); return true;
            case MotionEvent.ACTION_UP:
            case MotionEvent.ACTION_CANCEL:
                session.release(mask); setPressed(false);
                if (event.getActionMasked() == MotionEvent.ACTION_UP) {
                    touchClick = true;
                    try { performClick(); } finally { touchClick = false; }
                }
                return true;
            default: return true;
        }
    }
    @Override public boolean performClick() {
        super.performClick(); if (!touchClick) session.pulse(mask); return true;
    }
    @Override public void onWindowFocusChanged(boolean focused) {
        super.onWindowFocusChanged(focused);
        if (!focused) { session.release(mask); setPressed(false); }
    }
}
