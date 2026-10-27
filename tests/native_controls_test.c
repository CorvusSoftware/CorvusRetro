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
                              {640, 360}, {900, 450}, {320, 240}, {320, 321}, {100, 100}};
    for (int system = 0; system < 3; system++)
        for (size_t k = 0; k < sizeof(sizes) / sizeof(sizes[0]); k++) {
            float w = sizes[k][0], h = sizes[k][1];
            cr_play_layout p = cr_play_layout_make(w, h, system, 0, 0);
            check(inside(p.video, w, h), "native aspect video fits viewport");
            float ratio = system == 2 ? 1.5f : 160.f / 144;
            check(fabsf(p.video.w / p.video.h - ratio) < .001f, "GB/GBC/GBA native aspect");
            check(inside(p.menu, w, h) && inside(p.dpad, w, h), "menu and cross fit viewport");
            check(fabsf(p.menu.x + p.menu.w / 2 - w / 2) < .01f && p.menu.y > h / 2,
                  "menu centered among lower controls");
            check(cr_play_hit(&p, w / 2, p.menu.y + p.menu.h / 2) == 0,
                  "menu never sends game input");
            if (p.portrait)
                check(p.video.y + p.video.h <= p.panel.y + .01f,
                      "portrait controls do not cover the video");
            for (int i = 0; i < p.count; i++) {
                cr_control b = p.controls[i];
                check(inside(b.area, w, h), "every touch target fits viewport");
                check(cr_play_hit(&p, b.area.x + b.area.w / 2, b.area.y + b.area.h / 2) == b.mask,
                      "every drawn control center activates its own game input");
                check(!overlaps(p.menu, b.area), "menu separated from all game buttons");
                if (p.portrait)
                    check(b.area.y >= p.panel.y - .01f, "portrait buttons stay below video");
                if (w >= 320 && h >= (p.portrait ? 480 : 320))
                    check(b.area.w >= 48 && b.area.h >= 48, "phone touch targets at least 48dp");
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
            check(cr_play_hit(&p, -1, -1) == 0, "outside touch ignored");
        }
}
typedef struct {
    cr_area video, menu;
    int menu_icons, games;
    bool title;
} capture;
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
        capture c = {0};
        cr_painter painter = {&c, box, circle, text, measure, icon, NULL, game, NULL};
        cr_ui_draw(&u, &painter);
        check(c.games == 1 && c.menu_icons == 1 && !c.title,
              "game and one menu icon without title bar");
        float mx = c.menu.x + c.menu.w / 2, my = c.menu.y + c.menu.h / 2;
        check(fabsf(c.video.w / c.video.h - 160.f / 144) < .001f,
              "rendered video aspect preserved");
        tap(&u, 0, u.width - 28, 28);
        check(!u.quick && cr_ui_input(&u) == 0, "former title button no longer opens menu");
        cr_area a = p.controls[4].area;
        cr_ui_touch(&u, 0, CR_TOUCH_DOWN, a.x + a.w / 2, a.y + a.h / 2);
        check(cr_ui_input(&u) == 16, "A held before opening menu");
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
