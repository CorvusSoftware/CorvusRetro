// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
package com.corvussoftware.corvusretro;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.RectF;
import android.view.View;

final class DemoView extends View {
    private final int[] pixels = new int[256 * 192];
    private final Bitmap bitmap = Bitmap.createBitmap(256, 192, Bitmap.Config.ARGB_8888);
    private final Paint paint = new Paint();
    private final RectF destination = new RectF();
    private int input;
    private boolean paused;
    private boolean active;
    private final Runnable tick = new Runnable() {
        @Override public void run() {
            if (!active) return;
            render(input);
            postDelayed(this, 16);
        }
    };
    DemoView(Context context) {
        super(context);
        setContentDescription(context.getString(R.string.demo_description));
        setBackground(Ui.shape(context, 0xff101823, 10, false));
        setClipToOutline(true);
        render(0);
    }
    void press(int mask) { input |= mask; }
    void release(int mask) { input &= ~mask; }
    void clearInput() { input = 0; }
    void stepOnce(int mask) { render(mask); }
    void setPaused(boolean value) { paused = value; render(0); }
    void reset() { clearInput(); NativeBridge.reset(); paused = false; render(0); }
    private void render(int buttons) {
        NativeBridge.frame(pixels, buttons, paused);
        bitmap.setPixels(pixels, 0, 256, 0, 0, 256, 192);
        invalidate();
    }
    void start() { if (!active) { active = true; post(tick); } }
    void stop() { active = false; clearInput(); removeCallbacks(tick); }
    @Override protected void onDetachedFromWindow() { stop(); super.onDetachedFromWindow(); }
    @Override protected void onMeasure(int widthSpec, int heightSpec) {
        int width = getDefaultSize(Ui.dp(getContext(), 256), widthSpec);
        int height = width * 192 / 256;
        if (Ui.compactViewport(getContext())) {
            height = Math.min(height, Ui.dp(getContext(), 144));
        }
        setMeasuredDimension(width, resolveSize(height, heightSpec));
    }
    @Override protected void onDraw(Canvas canvas) {
        super.onDraw(canvas);
        float scale = Math.min(getWidth() / 256f, getHeight() / 192f);
        float width = 256 * scale, height = 192 * scale;
        float left = (getWidth() - width) / 2, top = (getHeight() - height) / 2;
        destination.set(left, top, left + width, top + height);
        canvas.drawBitmap(bitmap, null, destination, paint);
    }
}
