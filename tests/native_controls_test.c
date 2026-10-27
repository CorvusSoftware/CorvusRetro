// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
#include "menu_driver.h"
#include "play_layout.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
static void check(bool ok, const char *message) {
    if (!ok) {
        fprintf(stderr, "FAIL %s\n", message);
        exit(1);
    }
}
static bool inside(cr_area a, float w, float h) {
    return isfinite(a.x) && isfinite(a.y) && a.x >= -.01f && a.y >= -.01f && a.w > 0 && a.h > 0 &&
           a.x + a.w <= w + .01f && a.y + a.h <= h + .01f;
}
static bool overlaps(cr_area a, cr_area b) {
    return a.x < b.x + b.w - .01f && b.x < a.x + a.w - .01f && a.y < b.y + b.h - .01f &&
           b.y < a.y + a.h - .01f;
}
static void geometry_tests(void) {
    const float sizes[][2] = {{320, 480}, {360, 640}, {384, 696}, {600, 960}, {720, 320},
                              {640, 360}, {900, 450}, {320, 240}, {321, 320}, {320, 321},
                              {240, 320}, {100, 100}, {100, 60}, {60, 100}, {1920, 1080}};
    for (int system = 0; system < 3; system++)
        for (size_t k = 0; k < sizeof(sizes) / sizeof(sizes[0]); k++) {
            float w = sizes[k][0], h = sizes[k][1];
            cr_play_layout p = cr_play_layout_make(w, h, system, 0, 0);
            check(inside(p.video, w, h), "native aspect video fits viewport");
            float ratio = system == 2 ? 1.5f : 160.f / 144;
            check(fabsf(p.video.w / p.video.h - ratio) < .001f, "GB/GBC/GBA native aspect");
            check(p.count == (system == 2 ? 10 : 8), "only native console buttons are present");
            check(inside(p.menu, w, h) && inside(p.hide, w, h) && inside(p.dpad, w, h),
                  "menu, overlay toggle and cross fit viewport");
            if (p.portrait) {
                check(fabsf(p.menu.x + p.menu.w / 2 - w / 2) < .01f && p.menu.y >= p.panel.y,
                      "portrait menu centered between lower controls");
                check(p.hide.x > w / 2 && p.hide.y > h / 2,
                      "portrait overlay toggle in lower right corner");
                check(p.panel.y <= h / 2 + .01f && inside(p.panel, w, h),
                      "portrait reserves lower half for controls");
                if (system == 2 || p.menu.x < p.dpad.x + p.dpad.w)
                    check(p.dpad.y >= p.menu.y + p.menu.h + 8 * p.scale - .01f,
                          "portrait cross leaves a safe gap below menu and shoulders");
                if (w >= 320 && h >= 640) {
                    check(p.dpad.y + p.dpad.h / 2 < h * .75f,
                          "tall portrait cross remains near video rather than bottom edge");
                    if (system != 2 && p.menu.x >= p.dpad.x + p.dpad.w)
                        check(p.dpad.y + p.dpad.h / 2 < h * .7f,
                              "wide GB portrait cross sits beside centered menu");
                    check(p.controls[6].area.y + p.controls[6].area.h / 2 < h * .9f,
                          "tall portrait utility buttons follow primary controls");
                }
            } else {
                check(p.menu.x + p.menu.w <= p.video.x + .01f && p.menu.y < h / 2,
                      "landscape menu stays above cross in left gutter");
                check(p.hide.x + p.hide.w < w / 2 && p.hide.y > h / 2,
                      "landscape overlay toggle in lower left corner");
                check(fabsf(p.video.y + p.video.h / 2 - h / 2) < .01f,
                      "landscape video centered vertically");
            }
            check(fabsf(p.video.x + p.video.w / 2 - w / 2) < .01f,
                  "video centered horizontally");
            check(!overlaps(p.menu, p.video) && !overlaps(p.hide, p.video) &&
                      !overlaps(p.dpad, p.video), "video separated from overlay targets");
            check(!overlaps(p.menu, p.dpad) && !overlaps(p.hide, p.dpad) &&
                      !overlaps(p.menu, p.hide), "overlay actions separated from diagonal zones");
            check(cr_play_hit(&p, p.menu.x + p.menu.w / 2, p.menu.y + p.menu.h / 2) == 0,
                  "menu never sends game input");
            check(cr_play_hit(&p, p.hide.x + p.hide.w / 2, p.hide.y + p.hide.h / 2) == 0,
                  "overlay toggle never sends game input");
            if (p.portrait)
                check(p.video.y + p.video.h <= p.panel.y + .01f,
                      "portrait controls do not cover the video");
            for (int i = 0; i < p.count; i++) {
                cr_control b = p.controls[i];
                check(b.mask == (1u << i), "native input order and masks preserved");
                check(inside(b.area, w, h), "every touch target fits viewport");
                check(cr_play_hit(&p, b.area.x + b.area.w / 2, b.area.y + b.area.h / 2) == b.mask,
                      "every drawn control center activates its own game input");
                check(!overlaps(p.menu, b.area), "menu separated from all game buttons");
                check(!overlaps(p.hide, b.area), "overlay toggle separated from all game buttons");
                check(!overlaps(p.video, b.area), "game buttons never cover video");
                if (p.portrait)
                    check(b.area.y >= p.panel.y - .01f, "portrait buttons stay below video");
                if (w >= 320 && h >= (p.portrait ? 480 : 320)) {
                    check(b.area.w >= 48 && b.area.h >= 48, "phone touch targets at least 48dp");
                    check(p.menu.w >= 48 && p.menu.h >= 48 && p.hide.w >= 48 && p.hide.h >= 48,
                          "phone overlay actions at least 48dp");
                }
                if (i >= 4) {
                    check(!overlaps(p.dpad, b.area), "cross diagonal zones separated from buttons");
                    for (int j = i + 1; j < p.count; j++)
                        check(!overlaps(b.area, p.controls[j].area),
                              "game button targets separated");
                }
            }
            float x = p.dpad.x, y = p.dpad.y, c = p.dpad.w / 3;
            check(cr_play_hit(&p, x + c / 2, y + c / 2) == (1 | 4), "up-left diagonal");
            check(cr_play_hit(&p, x + c * 2.5f, y + c * 2.5f) == (2 | 8), "down-right diagonal");
            check(cr_play_hit(&p, x + c * 1.5f, y + c * 1.5f) == 0, "neutral cross center");
            check(cr_play_hit(&p, x + c * 1.5f, y + c * 1.35f) == 1,
                  "the visible upper arrow tip near the center remains active");
            check(cr_play_hit(&p, x + c * 1.55f, y + c * 1.55f) == 0,
                  "the small central gap is a stable neutral zone");
            check(cr_play_hit(&p, -1, -1) == 0, "outside touch ignored");
            check(cr_play_hit(&p, p.controls[4].area.x, p.controls[4].area.y) == 0,
                  "round face button corners do not activate input");
        }
    cr_play_layout full_height = cr_play_layout_make(720, 320, 0, 160, 144);
    check(fabsf(full_height.video.h - 320) < .01f,
          "landscape uses full video height when native aspect fits between gutters");
}
typedef struct {
    cr_area video, menu;
    int menu_icons, games;
    float width, height;
    unsigned lines;
    bool title;
} capture;
static void line(void *context, float x1, float y1, float x2, float y2, float width, uint32_t color) {
    capture *c = context;
    check(isfinite(x1) && isfinite(y1) && isfinite(x2) && isfinite(y2) && isfinite(width) && width > 0,
          "overlay strokes have finite geometry");
    check(x1 >= 0 && y1 >= 0 && x2 >= 0 && y2 >= 0 && x1 <= c->width && x2 <= c->width &&
              y1 <= c->height && y2 <= c->height, "overlay outlines stay inside the safe viewport");
    check((color & 0xffffff) == 0xffffff, "overlay contours use neutral gray without theme tint");
    ++c->lines;
}
static void box(void *c, float x, float y, float w, float h, uint32_t color) {
    (void)c;
    (void)x;
    (void)y;
    (void)w;
    (void)h;
    (void)color;
}
static void circle(void *c, float x, float y, float r, uint32_t color) {
    (void)c;
    (void)x;
    (void)y;
    (void)r;
    (void)color;
}
static void text(void *ctx, const char *text, float x, float y, float size, uint32_t color,
                 float max) {
    (void)x;
    (void)y;
    (void)size;
    (void)color;
    (void)max;
    ((capture *)ctx)->title |= strcmp(text, "REMOVED TITLE") == 0;
}
static float measure(void *ctx, const char *text, float size) {
    (void)ctx;
    return strlen(text) * size * .5f;
}
static void icon(void *ctx, int kind, float x, float y, float size, uint32_t color) {
    (void)color;
    capture *c = ctx;
    if (kind == CR_ICON_MENU) {
        c->menu_icons++;
        c->menu = (cr_area){x, y, size, size};
    }
}
static void game(void *ctx, const uint32_t *pixels, int width, int height, float x, float y,
                 float w, float h) {
    (void)pixels;
    (void)width;
    (void)height;
    capture *c = ctx;
    c->games++;
    c->video = (cr_area){x, y, w, h};
}
static void tap(cr_ui *u, int id, float x, float y) {
    cr_ui_touch(u, id, CR_TOUCH_DOWN, x, y);
    cr_ui_touch(u, id, CR_TOUCH_UP, x, y);
}
int main(void) {
    geometry_tests();
    char root[] = "/tmp/corvus-controls-XXXXXX";
    check(mkdtemp(root) != NULL, "temporary storage");
    cr_ui u;
    cr_ui_init(&u, (cr_platform){0}, root);
    u.page = CR_PLAY;
    u.engine.loaded = true;
    u.engine.system = 0;
    u.engine.width = 160;
    u.engine.height = 144;
    snprintf(u.title, sizeof(u.title), "REMOVED TITLE");
    const float sizes[][2] = {{384, 696}, {720, 320}, {320, 480}};
    for (size_t k = 0; k < sizeof(sizes) / sizeof(sizes[0]); k++) {
        cr_ui_size(&u, sizes[k][0], sizes[k][1]);
        cr_play_layout p = cr_play_layout_make(u.width, u.height, 0, 160, 144);
        capture c = {.width = u.width, .height = u.height};
        cr_painter painter = {&c, box, circle, text, measure, icon, NULL, game, NULL, NULL, line};
        cr_ui_draw(&u, &painter);
        check(c.games == 1 && c.menu_icons == 1 && !c.title,
              "game and one menu icon without title bar");
        check(c.lines > 100, "game buttons and overlay actions have visible outlines");
        float mx = c.menu.x + c.menu.w / 2, my = c.menu.y + c.menu.h / 2;
        check(fabsf(c.video.w / c.video.h - 160.f / 144) < .001f,
              "rendered video aspect preserved");
        tap(&u, 0, u.width - 28, 28);
        check(!u.quick && cr_ui_input(&u) == 0, "former title button no longer opens menu");
        cr_area a = p.controls[4].area;
        cr_ui_touch(&u, 0, CR_TOUCH_DOWN, a.x + a.w / 2, a.y + a.h / 2);
        check(cr_ui_input(&u) == 16, "A held before opening menu");
        float diagonal_x = p.dpad.x + p.dpad.w / 6;
        float diagonal_y = p.dpad.y + p.dpad.h / 6;
        cr_ui_touch(&u, 2, CR_TOUCH_DOWN, diagonal_x, diagonal_y);
        check(cr_ui_input(&u) == (16 | 1 | 4), "multitouch combines face button and diagonal");
        cr_ui_touch(&u, 2, CR_TOUCH_UP, diagonal_x, diagonal_y);
        check(cr_ui_input(&u) == 16, "releasing one finger preserves the other held button");
        tap(&u, 1, mx, my);
        check(u.quick && !cr_ui_running(&u) && cr_ui_input(&u) == 0,
              "touching rendered central icon pauses and releases all game input");
        cr_ui_back(&u);
        check(!u.quick && cr_ui_running(&u) && cr_ui_input(&u) == 0, "resume without stuck button");
        cr_ui_touch(&u, 0, CR_TOUCH_DOWN, mx, my);
        cr_ui_touch(&u, 0, CR_TOUCH_MOVE, a.x + a.w / 2, a.y + a.h / 2);
        check(cr_ui_input(&u) == 0, "drag from menu does not activate game buttons");
        cr_ui_touch(&u, 0, CR_TOUCH_MOVE, mx, my);
        cr_ui_touch(&u, 0, CR_TOUCH_UP, mx, my);
        check(!u.quick, "drag back to menu does not open it");
        cr_ui_touch(&u, 0, CR_TOUCH_DOWN, mx, my);
        cr_ui_touch(&u, 0, CR_TOUCH_CANCEL, mx, my);
        check(!u.quick && cr_ui_input(&u) == 0, "cancelled menu gesture ignored");
    }
    u.engine.loaded = false;
    cr_ui_destroy(&u);
    rmdir(root);
    puts("PASS adaptive controls, aspect ratio, rendered menu touch, diagonals and pause");
    return 0;
}
