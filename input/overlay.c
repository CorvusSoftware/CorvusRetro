// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
#include "overlay.h"
#include <math.h>
#include <string.h>

static uint32_t color(float level, unsigned idle) {
    float t = isfinite(level) ? fmaxf(0, fminf(1, level)) : 0;
    return 0xffffffu | ((uint32_t)(idle + (255 - idle) * t) << 24);
}
static void segment(const cr_painter *p, float x1, float y1, float x2, float y2,
                    float width, uint32_t value) {
    if (p->line) p->line(p->context, x1, y1, x2, y2, width, value);
}
static void path(const cr_painter *p, const float *points, int count, float width,
                 uint32_t value) {
    for (int i = 0; i < count; ++i) {
        int next = (i + 1) % count;
        segment(p, points[i * 2], points[i * 2 + 1], points[next * 2], points[next * 2 + 1], width, value);
    }
}
static void ring(const cr_painter *p, float x, float y, float radius, float width, uint32_t value) {
    for (int i = 0; i < 32; ++i) {
        float a = i * 6.2831853f / 32, b = (i + 1) * 6.2831853f / 32;
        segment(p, x + cosf(a) * radius, y + sinf(a) * radius,
                x + cosf(b) * radius, y + sinf(b) * radius, width, value);
    }
}
static void caption(const cr_painter *p, const char *text, float x, float y, float size,
                    uint32_t value, float maximum) {
    if (!p->text) return;
    float width = p->measure ? p->measure(p->context, text, size) : strlen(text) * size * .5f;
    float span = fminf(width, maximum);
    p->text(p->context, text, x - span / 2, y, size, value,
            fminf(maximum, span + 1));
}
static void rectangle(const cr_painter *p, cr_area a, float inset, float width, uint32_t value) {
    float points[] = {a.x + inset, a.y + inset, a.x + a.w - inset, a.y + inset,
                      a.x + a.w - inset, a.y + a.h - inset, a.x + inset, a.y + a.h - inset};
    path(p, points, 4, width, value);
}
static void dpad(const cr_painter *p, cr_area a, int direction, float scale, uint32_t value) {
    const float shape[] = {-.40f, -.40f, .40f, -.40f, .40f, .35f, 0, .85f, -.40f, .35f};
    const float arrow[] = {0, -.12f, -.10f, .08f, .10f, .08f};
    const float cosine[] = {1, -1, 0, 0}, sine[] = {0, 0, -1, 1};
    float points[10];
    for (int pass = 0; pass < 2; ++pass) {
        const float *source = pass ? arrow : shape;
        int count = pass ? 3 : 5;
        for (int i = 0; i < count; ++i) {
            float x = source[i * 2], y = source[i * 2 + 1];
            points[i * 2] = a.x + a.w / 2 + (x * cosine[direction] - y * sine[direction]) * a.w;
            points[i * 2 + 1] = a.y + a.h / 2 + (x * sine[direction] + y * cosine[direction]) * a.h;
        }
        path(p, points, count, 2 * scale, value);
    }
}
void cr_overlay_draw(const cr_play_layout *layout, const cr_painter *p, unsigned held,
                     const float feedback[10], bool visible, bool menu_down, bool hide_down) {
    float scale = layout->scale, stroke = 2 * scale;
    if (visible) for (int i = 0; i < layout->count; ++i) {
        const cr_control *button = &layout->controls[i];
        cr_area a = button->area;
        float level = held & button->mask ? 1 : feedback[i];
        uint32_t edge = color(level, 128), text = color(level, 160);
        if (i < 4) dpad(p, a, i, scale, edge);
        else if (i < 6) {
            float radius = a.w / 2 - stroke;
            if (level > 0 && p->circle)
                p->circle(p->context, a.x + a.w / 2, a.y + a.h / 2, radius - stroke,
                          color(level * .18f, 0));
            ring(p, a.x + a.w / 2, a.y + a.h / 2, radius, stroke, edge);
            caption(p, button->label, a.x + a.w / 2, a.y + a.h / 2 + 8 * scale,
                    26 * scale, text, a.w);
        } else if (i == 6) {
            float cx = a.x + a.w / 2, cy = a.y + a.h * .40f;
            float points[] = {cx - 14 * scale, cy - 10 * scale, cx + 16 * scale, cy,
                              cx - 14 * scale, cy + 10 * scale};
            path(p, points, 3, stroke, edge);
            caption(p, button->label, cx, a.y + a.h - 2 * scale, 8 * scale, text, a.w);
        } else if (i == 7) {
            cr_area mark = {a.x + a.w / 2 - 14 * scale, a.y + a.h * .40f - 9 * scale,
                            28 * scale, 18 * scale};
            rectangle(p, mark, 0, stroke, edge);
            caption(p, button->label, a.x + a.w / 2, a.y + a.h - 2 * scale, 8 * scale, text, a.w);
        } else {
            rectangle(p, a, 5 * scale, stroke, edge);
            caption(p, button->label, a.x + a.w / 2, a.y + a.h / 2 + 6 * scale,
                    20 * scale, text, a.w);
        }
    }
    cr_area menu = layout->menu, hide = layout->hide;
    float radius = 15 * scale;
    ring(p, menu.x + menu.w / 2, menu.y + menu.h / 2, radius, stroke, color(menu_down, 128));
    if (p->icon)
        p->icon(p->context, CR_ICON_MENU, menu.x + menu.w / 2 - 10 * scale,
                menu.y + menu.h / 2 - 10 * scale, 20 * scale, color(menu_down, 160));
    float cx = hide.x + hide.w / 2, cy = hide.y + hide.h / 2;
    ring(p, cx, cy, radius, stroke, color(hide_down, 128));
    float sign = visible ? 1 : -1;
    segment(p, cx - 7 * scale, cy - 3 * scale * sign, cx, cy + 4 * scale * sign,
            stroke, color(hide_down, 160));
    segment(p, cx, cy + 4 * scale * sign, cx + 7 * scale, cy - 3 * scale * sign,
            stroke, color(hide_down, 160));
}
