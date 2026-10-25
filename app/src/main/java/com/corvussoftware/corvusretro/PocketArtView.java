// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
package com.corvussoftware.corvusretro;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.view.View;

/** Original vector illustration; no game artwork or third-party assets. */
final class PocketArtView extends View {
    private final Paint paint = new Paint(Paint.ANTI_ALIAS_FLAG);
    PocketArtView(Context context) {
        super(context);
        setImportantForAccessibility(IMPORTANT_FOR_ACCESSIBILITY_NO);
    }
    private void color(int color) { paint.setColor(color); }
    @Override protected void onDraw(Canvas canvas) {
        super.onDraw(canvas);
        canvas.save();
        float scale = Math.min(getWidth() / 180f, getHeight() / 210f);
        canvas.translate((getWidth() - 180 * scale) / 2, (getHeight() - 210 * scale) / 2);
        canvas.scale(scale, scale);
        color(0xff26392a); canvas.drawCircle(90, 103, 82, paint);
        color(0xff3a5036); canvas.drawCircle(155, 37, 4, paint); canvas.drawCircle(22, 155, 3, paint);
        canvas.rotate(12, 90, 105);
        color(0xff111d13); canvas.drawRoundRect(43, 17, 150, 201, 22, 22, paint);
        color(0xffc5f277); canvas.drawRoundRect(35, 9, 142, 193, 20, 20, paint);
        color(0xff101c18); canvas.drawRoundRect(45, 23, 132, 108, 10, 10, paint);
        color(0xff38583c); canvas.drawRoundRect(51, 29, 126, 100, 5, 5, paint);
        color(0xffc5f277);
        canvas.drawRect(61, 77, 73, 89, paint); canvas.drawRect(73, 65, 85, 89, paint);
        canvas.drawRect(85, 53, 97, 89, paint); canvas.drawRect(97, 65, 109, 89, paint);
        canvas.drawRect(109, 77, 120, 89, paint); canvas.drawRect(61, 41, 65, 45, paint);
        canvas.drawRect(111, 37, 115, 41, paint);
        color(0xff263c24);
        canvas.drawRoundRect(50, 133, 86, 145, 3, 3, paint);
        canvas.drawRoundRect(62, 121, 74, 157, 3, 3, paint);
        canvas.drawCircle(115, 130, 8, paint); canvas.drawCircle(101, 149, 8, paint);
        color(0xff85b34e); canvas.drawRoundRect(71, 172, 105, 176, 2, 2, paint);
        canvas.restore();
    }
}
