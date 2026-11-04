// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
package com.corvussoftware.corvusretro;

import android.app.Activity;
import android.app.Instrumentation;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.PorterDuff;
import android.graphics.PorterDuffColorFilter;
import android.graphics.Rect;
import android.graphics.RectF;
import android.os.Bundle;
import java.io.File;
import java.io.FileOutputStream;
import java.lang.reflect.Field;
import java.util.HashSet;
import java.util.Set;

/** Validates the actual Android resource atlas, not a second vector renderer. */
public final class IconAtlasInstrumentation extends Instrumentation {
    private int checks;
    private void check(boolean condition, String message) {
        if (!condition) throw new AssertionError(message);
        checks++;
    }
    @Override public void onCreate(Bundle arguments) { super.onCreate(arguments); start(); }
    @Override public void onStart() {
        Bundle output = new Bundle();
        MainActivity activity = null;
        Bitmap atlas = null, montage = null;
        int resultCode = Activity.RESULT_CANCELED;
        try {
            activity = (MainActivity) startActivitySync(new Intent(getTargetContext(), MainActivity.class)
                    .addFlags(Intent.FLAG_ACTIVITY_NEW_TASK));
            Field field = MainActivity.class.getDeclaredField("ICONS"); field.setAccessible(true);
            int[] resources = (int[]) field.get(null);
            Field baseField = MainActivity.class.getDeclaredField("ICON_ATLAS_BASE"); baseField.setAccessible(true);
            int iconBase = baseField.getInt(null);
            check(resources.length == 42 && iconBase == 464 && iconBase + resources.length <= 512,
                    "42 native icons fit their reserved block after the text glyph cache");
            atlas = activity.atlas();
            check(atlas.getWidth() == 1024 && atlas.getHeight() == 2048, "glyph atlas dimensions remain stable");
            montage = Bitmap.createBitmap(1024, ((resources.length + 7) / 8) * 132, Bitmap.Config.ARGB_8888);
            Canvas canvas = new Canvas(montage); canvas.drawColor(Color.rgb(12, 18, 30));
            Paint bitmapPaint = new Paint(Paint.ANTI_ALIAS_FLAG | Paint.FILTER_BITMAP_FLAG);
            Paint textPaint = new Paint(Paint.ANTI_ALIAS_FLAG); textPaint.setTextSize(11); textPaint.setColor(Color.rgb(200, 210, 230));
            Set<String> shapes = new HashSet<>();
            Set<String> smallShapes = new HashSet<>();
            for (int i = 0; i < resources.length; ++i) {
                int slot = iconBase + i, sourceX = (slot % 16) * 64, sourceY = (slot / 16) * 64;
                int[] pixels = new int[64 * 64]; atlas.getPixels(pixels, 0, 64, sourceX, sourceY, 64, 64);
                int visible = 0; StringBuilder shape = new StringBuilder(48 * 48);
                for (int y = 8; y < 56; ++y) for (int x = 8; x < 56; ++x) {
                    int alpha = pixels[y * 64 + x] >>> 24;
                    if (alpha > 16) visible++;
                    shape.append((char) alpha);
                }
                check(visible >= 80, "Icon " + i + " is not blank at its actual atlas coordinates");
                check(shapes.add(shape.toString()), "Icon " + i + " has a distinct semantic shape");
                // The atlas's solid-quad texel is intentionally outside the final icon inset.
                boolean clean = true;
                for (int y = 0; y < 64; ++y) for (int x = 0; x < 64; ++x) {
                    if (x >= 8 && x < 56 && y >= 8 && y < 56) continue;
                    if (slot == 511 && x < 4 && y < 4) continue;
                    if ((pixels[y * 64 + x] >>> 24) > 0) clean = false;
                }
                check(clean, "Icon " + i + " never leaks into glyphs or the solid-quad marker");
                Bitmap small = Bitmap.createBitmap(24, 24, Bitmap.Config.ARGB_8888);
                try {
                    new Canvas(small).drawBitmap(atlas,
                            new Rect(sourceX + 8, sourceY + 8, sourceX + 56, sourceY + 56),
                            new RectF(0, 0, 24, 24), bitmapPaint);
                    StringBuilder smallShape = new StringBuilder(576); int smallVisible = 0;
                    for (int y = 0; y < 24; ++y) for (int x = 0; x < 24; ++x) {
                        int alpha = small.getPixel(x,y) >>> 24;
                        if (alpha > 16) ++smallVisible;
                        smallShape.append((char)alpha);
                    }
                    check(smallVisible >= 20 && smallShapes.add(smallShape.toString()),
                            "Icon " + i + " remains visible and distinct at mobile size");
                } finally { small.recycle(); }
                int left = (i % 8) * 128, top = (i / 8) * 132;
                canvas.drawBitmap(atlas, new Rect(sourceX + 8, sourceY + 8, sourceX + 56, sourceY + 56),
                        new RectF(left + 8, top + 8, left + 69, top + 69), bitmapPaint);
                canvas.drawBitmap(atlas, new Rect(sourceX + 8, sourceY + 8, sourceX + 56, sourceY + 56),
                        new RectF(left + 82, top + 28, left + 106, top + 52), bitmapPaint);
                String name = getTargetContext().getResources().getResourceEntryName(resources[i]);
                int[] backgrounds = {0xff090b24, 0xffffffff, 0xff121212};
                int[] colors = {0xffd4a4ff, 0xff000000, 0xff7ab547};
                for (int theme = 0; theme < colors.length; ++theme) {
                    Paint preview = new Paint(Paint.ANTI_ALIAS_FLAG | Paint.FILTER_BITMAP_FLAG);
                    preview.setColor(backgrounds[theme]);
                    canvas.drawRoundRect(left + 8 + theme * 36, top + 74,
                            left + 38 + theme * 36, top + 104, 5, 5, preview);
                    preview.setColorFilter(new PorterDuffColorFilter(colors[theme], PorterDuff.Mode.SRC_IN));
                    canvas.drawBitmap(atlas, new Rect(sourceX + 8, sourceY + 8, sourceX + 56, sourceY + 56),
                            new RectF(left + 11 + theme * 36, top + 77, left + 35 + theme * 36, top + 101), preview);
                }
                canvas.drawText(i + " " + name.substring(3), left + 8, top + 124, textPaint);
            }
            check(Color.alpha(atlas.getPixel(961, 1985)) == 255, "solid-quad white texel remains opaque");
            File capture = new File(getTargetContext().getFilesDir(), "test-icon-atlas.png");
            try (FileOutputStream file = new FileOutputStream(capture)) {
                check(montage.compress(Bitmap.CompressFormat.PNG, 100, file), "save actual atlas montage");
            }
            output.putString("stream", "PASS " + checks + " vector atlas bounds/uniqueness/raster checks\n");
            resultCode = Activity.RESULT_OK;
        } catch (Throwable error) {
            output.putString("stream", "FAIL " + android.util.Log.getStackTraceString(error) + "\n");
        } finally {
            if (atlas != null) atlas.recycle();
            if (montage != null) montage.recycle();
            if (activity != null) { MainActivity owned = activity; runOnMainSync(owned::finish); }
        }
        finish(resultCode, output);
    }
}
