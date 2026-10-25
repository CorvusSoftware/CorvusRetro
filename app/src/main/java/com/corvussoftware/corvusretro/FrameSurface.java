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

/** Bitmap mutations and Canvas drawing both happen on the UI thread. */
final class FrameSurface extends View {
    private Bitmap bitmap;
    private final Paint paint = new Paint();
    private final RectF rectangle = new RectF();
    private int controlsHeight;
    FrameSurface(Context context) {
        super(context);
        setBackgroundColor(0xff000000);
        setContentDescription(context.getString(R.string.emulation_screen));
    }
    void configure(int width, int height) {
        bitmap = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888);
    }
    void reserveControls(int height) { controlsHeight = height; invalidate(); }
    void present(int[] pixels) {
        if (bitmap == null) return;
        bitmap.setPixels(pixels, 0, bitmap.getWidth(), 0, 0, bitmap.getWidth(), bitmap.getHeight());
        invalidate();
    }
    @Override protected void onDraw(Canvas canvas) {
        super.onDraw(canvas);
        if (bitmap == null) return;
        int available = Math.max(1, getHeight() - controlsHeight);
        float scale = Math.min(getWidth() / (float)bitmap.getWidth(), available / (float)bitmap.getHeight());
        float width = bitmap.getWidth() * scale, height = bitmap.getHeight() * scale;
        float left = (getWidth() - width) / 2, top = (available - height) / 2f;
        rectangle.set(left, top, left + width, top + height);
        canvas.drawBitmap(bitmap, null, rectangle, paint);
    }
}
