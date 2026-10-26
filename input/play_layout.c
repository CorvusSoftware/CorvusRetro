// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
#include "play_layout.h"
#include <math.h>
static cr_control control(float x, float y, float w, float h, unsigned mask, const char *label) {
    return (cr_control){{x, y, w, h}, mask, label};
}
cr_play_layout cr_play_layout_make(float width, float height, int system, int vw, int vh) {
    cr_play_layout p = {0};
    float w = fmaxf(1, width), h = fmaxf(1, height);
    p.portrait = h >= w;
    float base = (p.portrait ? w : h) / 360;
    float s = fminf(1.4f, fmaxf(1, base));
    /* Keep 48dp targets on phone sizes; still fit unusually small multi-window surfaces. */
    s = fminf(s, fminf(w / 320, h / (p.portrait ? 384 : system == 2 ? 288 : 240)));
    p.scale = s;
    float cell = 48 * s, margin = 12 * s, footer = h - 60 * s;
    float panel = p.portrait ? (system == 2 ? 288 : 232) * s : 0;
    p.panel = (cr_area){0, h - panel, w, panel};
    p.menu = (cr_area){(w - cell) / 2, footer, cell, cell};
    p.dpad = (cr_area){margin, h - 220 * s, cell * 3, cell * 3};
    p.controls[0] = control(p.dpad.x + cell, p.dpad.y, cell, cell, 1, "↑");
    p.controls[1] = control(p.dpad.x + cell, p.dpad.y + cell * 2, cell, cell, 2, "↓");
    p.controls[2] = control(p.dpad.x, p.dpad.y + cell, cell, cell, 4, "←");
    p.controls[3] = control(p.dpad.x + cell * 2, p.dpad.y + cell, cell, cell, 8, "→");
    float a_top = h - 172 * s;
    p.controls[4] = control(w - margin - 56 * s, a_top, 56 * s, 56 * s, 16, "A");
    p.controls[5] = control(w - margin - 116 * s, h - 124 * s, 56 * s, 56 * s, 32, "B");
    p.controls[6] = control(p.menu.x + cell + 12 * s, footer, 64 * s, cell, 64, "START");
    p.controls[7] = control(p.menu.x - 76 * s, footer, 64 * s, cell, 128, "SELECT");
    p.count = 8;
    if (system == 2) {
        float shoulder = p.portrait || h >= 320 * s ? 72 * s : cell;
        float top = p.portrait
                        ? p.panel.y + fminf(12 * s, fmaxf(0, a_top - p.panel.y - cell - 8 * s))
                        : 12 * s;
        p.controls[p.count++] = control(margin, top, shoulder, cell, 256, "L");
        p.controls[p.count++] = control(w - margin - shoulder, top, shoulder, cell, 512, "R");
    }
    if (vw <= 0 || vh <= 0) {
        vw = system == 2 ? 240 : 160;
        vh = system == 2 ? 160 : 144;
    }
    float available = h - panel, fit = fminf(w / vw, available / vh);
    p.video = (cr_area){(w - vw * fit) / 2, (available - vh * fit) / 2, vw * fit, vh * fit};
    return p;
}
bool cr_area_contains(cr_area a, float x, float y) {
    return x >= a.x && x < a.x + a.w && y >= a.y && y < a.y + a.h;
}
unsigned cr_play_hit(const cr_play_layout *p, float x, float y) {
    if (cr_area_contains(p->dpad, x, y)) {
        int col = (int)((x - p->dpad.x) / (p->dpad.w / 3));
        int row = (int)((y - p->dpad.y) / (p->dpad.h / 3));
        return (col == 0 ? 4u : col == 2 ? 8u : 0u) | (row == 0 ? 1u : row == 2 ? 2u : 0u);
    }
    for (int i = 4; i < p->count; i++) {
        const cr_control *b = &p->controls[i];
        if (!cr_area_contains(b->area, x, y))
            continue;
        if (i == 4 || i == 5) {
            float r = b->area.w / 2, dx = x - b->area.x - r, dy = y - b->area.y - r;
            if (dx * dx + dy * dy > r * r)
                continue;
        }
        return b->mask;
    }
    return 0;
}
