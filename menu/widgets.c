// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
#include "widgets.h"
#include <math.h>
#include <string.h>

uint32_t cr_widget_mix(uint32_t a, uint32_t b, float fraction) {
    fraction = fminf(1, fmaxf(0, fraction));
    uint32_t color = 0;
    for (unsigned shift = 0; shift < 32; shift += 8) {
        float x = (float)((a >> shift) & 255), y = (float)((b >> shift) & 255);
        color |= (uint32_t)(x + (y - x) * fraction + .5f) << shift;
    }
    return color;
}
void cr_widget_round(const cr_painter *p, cr_menu_rect r, float radius, uint32_t color) {
    if (!p->rect || r.w <= 0 || r.h <= 0) return;
    radius = fmaxf(0, fminf(radius, fminf(r.w, r.h) / 2));
    if (!p->circle || radius < .5f) { p->rect(p->context, r.x, r.y, r.w, r.h, color); return; }
    p->rect(p->context, r.x + radius, r.y, r.w - radius * 2, r.h, color);
    p->rect(p->context, r.x, r.y + radius, r.w, r.h - radius * 2, color);
    p->circle(p->context, r.x + radius, r.y + radius, radius, color);
    p->circle(p->context, r.x + r.w - radius, r.y + radius, radius, color);
    p->circle(p->context, r.x + radius, r.y + r.h - radius, radius, color);
    p->circle(p->context, r.x + r.w - radius, r.y + r.h - radius, radius, color);
}
void cr_widget_gradient(const cr_painter *p, cr_menu_rect r, float radius, uint32_t top, uint32_t bottom) {
    if (!p->rect || r.w <= 0 || r.h <= 0) return;
    radius = fmaxf(0, fminf(radius, fminf(r.w, r.h) / 2));
    unsigned steps = (unsigned)fminf(radius > 0 ? 256 : 48,
        fmaxf(1, ceilf(r.h / (radius > 0 ? 1 : 3))));
    for (unsigned i = 0; i < steps; ++i) {
        float y = r.h * i / steps, next = r.h * (i + 1) / steps;
        float center = (y + next) / 2, inset = 0;
        float edge = fminf(center, r.h - center);
        if (edge < radius) {
            float dy = radius - edge;
            inset = radius - sqrtf(fmaxf(0, radius * radius - dy * dy));
        }
        p->rect(p->context, r.x + inset, r.y + y, r.w - 2 * inset, next - y + .05f,
                cr_widget_mix(top, bottom, center / r.h));
    }
}
void cr_widget_card(const cr_painter *p, cr_menu_rect r, const cr_theme *theme, float active) {
    active = fminf(1, fmaxf(0, active));
    uint32_t shadow = theme->background == 0xffffffff ? 0x12000000 : 0x33000000;
    cr_widget_round(p, (cr_menu_rect){r.x, r.y + 3, r.w, r.h}, 15, shadow);
    cr_widget_round(p, r, 15,
        cr_widget_mix(cr_widget_mix(theme->surface, theme->divider, .6f), theme->accent, active));
    cr_widget_gradient(p, (cr_menu_rect){r.x + 1, r.y + 1, r.w - 2, r.h - 2}, 14,
        cr_widget_mix(cr_widget_mix(theme->surface, theme->accent, .035f),
                      cr_widget_mix(theme->focus, theme->accent, .18f), active),
        cr_widget_mix(theme->surface, theme->focus, active));
}
void cr_widget_center(const cr_painter *p, const char *text, float x, float baseline,
                      float size, uint32_t color, float max_width) {
    if (!p->text || !text) return;
    float width = p->measure ? p->measure(p->context, text, size) : strlen(text) * size * .48f;
    width = fminf(width, fmaxf(0, max_width));
    p->text(p->context, text, x - width / 2, baseline, size, color, max_width);
}
void cr_widget_line(const cr_painter *p, float x, float y, float end_x, float end_y,
                    float width, uint32_t color) {
    if (p->line) p->line(p->context, x, y, end_x, end_y, width, color);
    else if (p->rect) p->rect(p->context, fminf(x, end_x), fminf(y, end_y),
                             fmaxf(width, fabsf(end_x - x)), fmaxf(width, fabsf(end_y - y)), color);
}
void cr_widget_console(const cr_painter *p, cr_menu_rect r, int console, const cr_theme *theme) {
    if (r.w <= 0 || r.h <= 0) return;
    if (console < 0 || console > 2) {
        if (p->icon) p->icon(p->context, CR_ICON_CORE, r.x + r.w * .15f, r.y + r.h * .15f,
                             fminf(r.w, r.h) * .7f, theme->accent);
        return;
    }
    float w = console == 2 ? r.w : fminf(r.w * .63f, r.h * .65f);
    float h = console == 2 ? fminf(r.h, w * .61f) : fminf(r.h, w * 1.47f);
    float x = r.x + (r.w - w) / 2, y = r.y + (r.h - h) / 2;
    uint32_t body = console == 0 ? 0xffd9dcf0 : cr_widget_mix(theme->accent, 0xff7953e0, .38f);
    cr_widget_round(p, (cr_menu_rect){x, y + h * .04f, w, h}, w * .12f, 0x66000000);
    cr_widget_round(p, (cr_menu_rect){x, y, w, h}, w * .12f, cr_widget_mix(body, 0xff0b0923, .6f));
    cr_widget_gradient(p, (cr_menu_rect){x + 1, y + 1, w - 2, h - 3}, w * .11f,
                       cr_widget_mix(body, 0xffffffff, .24f), body);
    cr_menu_rect screen = console == 2
        ? (cr_menu_rect){x + w * .24f, y + h * .14f, w * .52f, h * .7f}
        : (cr_menu_rect){x + w * .1f, y + h * .12f, w * .8f, h * .42f};
    cr_widget_round(p, screen, w * .035f, 0xff17172e);
    cr_widget_gradient(p, (cr_menu_rect){screen.x + screen.w * .09f, screen.y + screen.h * .12f,
                                       screen.w * .82f, screen.h * .76f}, 1,
                       console == 0 ? 0xffb7c7ae : 0xff9b8ce5,
                       console == 0 ? 0xff7b9279 : 0xff5c4ca4);
    float cx = x + w * (console == 2 ? .12f : .26f);
    float cy = y + h * (console == 2 ? .49f : .72f);
    float size = w * (console == 2 ? .17f : .26f);
    cr_widget_round(p, (cr_menu_rect){cx - size * .18f, cy - size * .5f, size * .36f, size}, 1, 0xff312d54);
    cr_widget_round(p, (cr_menu_rect){cx - size * .5f, cy - size * .18f, size, size * .36f}, 1, 0xff312d54);
    if (p->circle) {
        float button = w * (console == 2 ? .033f : .07f);
        p->circle(p->context, x + w * .87f, y + h * .38f + (console == 2 ? 0 : h * .29f), button, 0xfff1d5ff);
        p->circle(p->context, x + w * .81f, y + h * .60f + (console == 2 ? 0 : h * .18f), button, 0xfff1d5ff);
    }
    if (console != 2) {
        cr_widget_round(p, (cr_menu_rect){x + w * .42f, y + h * .88f, w * .15f, h * .025f}, 1, 0xff554c78);
        cr_widget_round(p, (cr_menu_rect){x + w * .64f, y + h * .85f, w * .15f, h * .025f}, 1, 0xff554c78);
    }
}
