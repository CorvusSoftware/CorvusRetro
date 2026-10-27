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
    /* Shrink only multi-window surfaces; phone controls retain 48dp targets. */
    s = fminf(s, fminf(w / 320, h / (p.portrait ? 384 : 320)));
    p.scale = s;
    float cell = 48 * s, margin = (p.portrait ? 12 : 8) * s, footer = h - 60 * s;
    float panel = p.portrait ? fmaxf(h / 2, 276 * s) : 0;
    p.panel = (cr_area){0, h - panel, w, panel};
    p.menu = p.portrait ? (cr_area){(w - cell) / 2, p.panel.y + 12 * s, cell, cell}
                        : (cr_area){104 * s, 12 * s, cell, cell};
    p.hide = (cr_area){p.portrait ? w - margin - cell : margin, footer, cell, cell};
    float cross_top = system == 2 || p.menu.x < margin + cell * 3 ? 68 * s : 24 * s;
    p.dpad = (cr_area){margin, p.portrait ? p.panel.y + fmaxf(cross_top, panel * .10f)
                                        : (h - cell * 3) / 2,
                       cell * 3, cell * 3};
    p.controls[0] = control(p.dpad.x + cell, p.dpad.y, cell, cell, 1, "↑");
    p.controls[1] = control(p.dpad.x + cell, p.dpad.y + cell * 2, cell, cell, 2, "↓");
    p.controls[2] = control(p.dpad.x, p.dpad.y + cell, cell, cell, 4, "←");
    p.controls[3] = control(p.dpad.x + cell * 2, p.dpad.y + cell, cell, cell, 8, "→");
    float a_top = p.portrait ? p.dpad.y + 24 * s : h / 2 - 56 * s;
    p.controls[4] = control(w - margin - 56 * s, a_top, 56 * s, 56 * s, 16, "A");
    p.controls[5] = control(w - margin - 116 * s, a_top + 56 * s, 56 * s, 56 * s, 32, "B");
    float utility_top = p.portrait ? p.dpad.y + p.dpad.h + 12 * s : footer;
    p.controls[6] = control(p.portrait ? w / 2 + 12 * s : w - margin - 64 * s,
                            utility_top, 64 * s, cell, 64, "START");
    p.controls[7] = control(p.portrait ? w / 2 - 76 * s : 68 * s,
                            utility_top, 64 * s, cell, 128, "SELECT");
    p.count = 8;
    if (system == 2) {
        float top = p.portrait ? p.panel.y + 12 * s : 12 * s;
        p.controls[p.count++] = control(margin, top, 72 * s, cell, 256, "L");
        p.controls[p.count++] = control(w - margin - 72 * s, top, 72 * s, cell, 512, "R");
    }
    if (vw <= 0 || vh <= 0) {
        vw = system == 2 ? 240 : 160;
        vh = system == 2 ? 160 : 144;
    }
    /* Reserve both landscape gutters, including the cross's diagonal hit zones. */
    float video_width = p.portrait ? w : w - 312 * s;
    float video_height = h - panel, fit = fminf(video_width / vw, video_height / vh);
    p.video = (cr_area){(w - vw * fit) / 2, (video_height - vh * fit) / 2,
                       vw * fit, vh * fit};
    return p;
}
bool cr_area_contains(cr_area a, float x, float y) {
    return x >= a.x && x < a.x + a.w && y >= a.y && y < a.y + a.h;
}
unsigned cr_play_hit(const cr_play_layout *p, float x, float y) {
    if (cr_area_contains(p->dpad, x, y)) {
        float cell = p->dpad.w / 3;
        float dx = (x - p->dpad.x - p->dpad.w / 2) / cell;
        float dy = (y - p->dpad.y - p->dpad.h / 2) / cell;
        float ax = fabsf(dx), ay = fabsf(dy);
        if (ax <= .125f && ay <= .125f) return 0;
        return (ax > ay * .5f ? (dx < 0 ? 4u : 8u) : 0u) |
               (ay > ax * .5f ? (dy < 0 ? 1u : 2u) : 0u);
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
