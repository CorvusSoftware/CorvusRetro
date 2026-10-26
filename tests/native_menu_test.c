// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
#include "frontend.h"
#include "play_layout.h"
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
typedef struct {
    char text[16384];
    uint32_t highlight, a_body, a_text;
    float ax, ay;
} capture;
static void rect(void *ctx, float x, float y, float w, float h, uint32_t color) {
    capture *c = ctx;
    if (x == 0 && y == 216 && w == 384 && h == 76)
        c->highlight = color;
}
static void circle(void *ctx, float x, float y, float r, uint32_t color) {
    (void)r;
    capture *c = ctx;
    if (x > c->ax - 1 && x < c->ax + 1 && y > c->ay - 1 && y < c->ay + 1)
        c->a_body = color;
}
static void text(void *ctx, const char *s, float x, float y, float size, uint32_t color,
                 float max) {
    (void)x;
    (void)y;
    (void)size;
    (void)max;
    capture *c = ctx;
    size_t n = strlen(c->text);
    snprintf(c->text + n, sizeof(c->text) - n, "%s\n", s);
    if (strcmp(s, "A") == 0)
        c->a_text = color;
}
static float measure(void *ctx, const char *s, float size) {
    (void)ctx;
    return strlen(s) * size * .5f;
}
static void icon(void *ctx, int kind, float x, float y, float size, uint32_t color) {
    (void)ctx;
    (void)kind;
    (void)x;
    (void)y;
    (void)size;
    (void)color;
}
static void game(void *ctx, const uint32_t *pixels, int iw, int ih, float x, float y, float w,
                 float h) {
    (void)ctx;
    (void)pixels;
    (void)iw;
    (void)ih;
    (void)x;
    (void)y;
    (void)w;
    (void)h;
}
static capture draw(cr_ui *u) {
    cr_play_layout p = cr_play_layout_make(u->width, u->height, u->engine.system, 160, 144);
    cr_area a = p.controls[4].area;
    capture c = {.ax = a.x + a.w / 2, .ay = a.y + a.h / 2};
    cr_painter painter = {&c, rect, circle, text, measure, icon, NULL, game};
    cr_ui_draw(u, &painter);
    return c;
}
static int saved_show = -1, saved_audio = -1, writes;
static int core_refreshes, core_installs;
static char selected_pack[96];
static void refresh_core_catalog(void *ctx) {
    (void)ctx;
    core_refreshes++;
}
static void install_core_pack(void *ctx, const char *pack_id) {
    (void)ctx;
    core_installs++;
    snprintf(selected_pack, sizeof(selected_pack), "%s", pack_id);
}
static void set(void *ctx, const char *key, int value) {
    (void)ctx;
    writes++;
    if (strcmp(key, "show_input") == 0)
        saved_show = value;
    if (strcmp(key, "audio") == 0)
        saved_audio = value;
}
static void tap(cr_ui *u, float x, float y) {
    cr_ui_touch(u, 0, CR_TOUCH_DOWN, x, y);
    cr_ui_touch(u, 0, CR_TOUCH_UP, x, y);
}
static void row(cr_ui *u, int i) {
    tap(u, 160, 100 + 76 * i);
}
static void tab(cr_ui *u, int i) {
    tap(u, 64 + i * 128, 660);
}
int main(void) {
    char root[] = "/tmp/corvus-menu-XXXXXX";
    check(mkdtemp(root) != NULL, "temporary storage");
    cr_ui u;
    cr_ui_init(&u, (cr_platform){.set_preference = set,
                                .refresh_core_catalog = refresh_core_catalog,
                                .install_core_pack = install_core_pack}, root);
    cr_ui_size(&u, 384, 700);
    capture c = draw(&u);
    check(strstr(c.text, "Charger un cœur") && strstr(c.text, "Informations") &&
              !strstr(c.text, "Historique"),
          "main contains loading and information, history lives in playlists");
    row(&u, 0);
    check(u.page == CR_CORES, "core menu opens before downloader");
    c = draw(&u);
    check(strstr(c.text, "Téléchargeur de cœurs"), "core menu exposes online updater");
    row(&u, 3);
    check(u.page == CR_CORE_DOWNLOADS && core_refreshes == 1 &&
          u.catalog_loading, "downloader starts catalog request on entry");
    cr_core_catalog catalog = {0};
    catalog.count = 1;
    snprintf(catalog.packs[0].id, sizeof(catalog.packs[0].id), "pack-gb");
    snprintf(catalog.packs[0].name, sizeof(catalog.packs[0].name), "Corvus Pack GB");
    snprintf(catalog.packs[0].version, sizeof(catalog.packs[0].version), "1.0");
    snprintf(catalog.packs[0].abi, sizeof(catalog.packs[0].abi), "arm64-v8a");
    snprintf(catalog.packs[0].core_id, sizeof(catalog.packs[0].core_id), "corvus.gb");
    cr_ui_catalog(&u, &catalog);
    c = draw(&u);
    check(!u.catalog_loading && strstr(c.text, "Corvus Pack GB"),
          "remote packs are rendered in native UI");
    row(&u, 1);
    check(core_installs == 1 && strcmp(selected_pack, "pack-gb") == 0,
          "frontend requests verified pack installation by catalog ID only");
    row(&u, 0);
    check(core_refreshes == 2 && u.catalog_loading,
          "catalog can be refreshed without leaving menu");
    cr_core_catalog empty = {0};
    cr_ui_catalog(&u, &empty);
    check(u.core_catalog.count == 0 && !u.catalog_loading,
          "empty official catalog is a valid state");
    cr_ui_back(&u);
    cr_ui_back(&u);
    check(u.page == CR_MAIN, "downloader returns to main menu without touching a core");
    row(&u, 2);
    check(u.page == CR_INFORMATION && u.depth == 1 && u.tab == 0, "information category");
    c = draw(&u);
    check(strstr(c.text, "Licence et crédits") && !strstr(c.text, "Informations sur le cœur"),
          "core info shown only for a real loaded core");
    row(&u, 0);
    c = draw(&u);
    check(u.page == CR_ABOUT && !strstr(c.text, "sombre"), "about lists current themes only");
    cr_ui_back(&u);
    cr_ui_back(&u);
    check(u.page == CR_MAIN && u.focus == 2 && u.depth == 0, "back restores main selection");
    row(&u, 0);
    row(&u, 0);
    check(u.engine.handle && u.engine.system == 0, "real idle GB core");
    row(&u, 3);
    row(&u, 1);
    c = draw(&u);
    check(u.page == CR_CORE_INFO && strstr(c.text, "CorvusGB") && strstr(c.text, "0.14.0"),
          "real core information is reachable");
    cr_ui_back(&u);
    cr_ui_back(&u);
    row(&u, 1);
    check(!u.engine.handle, "core unloaded");
    tab(&u, 1);
    c = draw(&u);
    check(u.page == CR_LISTS && strstr(c.text, "Historique") && strstr(c.text, "Game Boy Advance"),
          "playlists contain history and console categories");
    row(&u, 0);
    check(u.page == CR_GAMES && u.filter == -1 && u.tab == 1, "history category");
    cr_ui_back(&u);
    row(&u, 3);
    check(u.page == CR_GAMES && u.filter == 2, "GBA playlist");
    cr_ui_back(&u);
    check(u.page == CR_LISTS && u.focus == 3, "back restores console selection");
    tab(&u, 2);
    c = draw(&u);
    check(strstr(c.text, "Interface utilisateur") && strstr(c.text, "Vidéo") &&
              strstr(c.text, "Audio") && strstr(c.text, "Affichage à l’écran"),
          "four real settings categories");
    // A stationary touch fades in before release, without opening the submenu.
    cr_ui_touch_at(&u, 0, CR_TOUCH_DOWN, 160, 252, 10);
    cr_ui_update(&u, .1);
    c = draw(&u);
    check(u.page == CR_SETTINGS && u.feedback_alpha > .4f && u.feedback_alpha < .6f &&
              (c.highlight >> 24) > 100 && (c.highlight >> 24) < 160,
          "progressive touch highlight is rendered");
    cr_ui_touch_at(&u, 0, CR_TOUCH_UP, 160, 252, 10.1);
    check(u.page == CR_AUDIO_SETTINGS, "tap activates at release");
    bool audio = u.audio;
    cr_ui_touch_at(&u, 0, CR_TOUCH_DOWN, 160, 100, 20);
    cr_ui_update(&u, .3);
    cr_ui_touch_at(&u, 0, CR_TOUCH_UP, 160, 100, 20.3);
    check(u.audio == audio && writes == 0, "hold selects without toggling or resetting");
    // Event timestamps prevent a slow rendering frame from turning a tap into a hold.
    cr_ui_touch_at(&u, 0, CR_TOUCH_DOWN, 160, 100, 30);
    cr_ui_update(&u, 1);
    cr_ui_touch_at(&u, 0, CR_TOUCH_UP, 160, 100, 30.05);
    check(u.audio != audio && saved_audio == u.audio && writes == 1,
          "tap duration independent of frame timing");
    cr_ui_touch_at(&u, 0, CR_TOUCH_DOWN, 160, 100, 40);
    cr_ui_touch_at(&u, 1, CR_TOUCH_DOWN, 160, 100, 40.01);
    cr_ui_touch_at(&u, 1, CR_TOUCH_UP, 160, 100, 40.02);
    cr_ui_touch_at(&u, 0, CR_TOUCH_UP, 160, 100, 40.05);
    check(writes == 2 && u.audio == audio, "menus use one finger and toggle exactly once");
    cr_ui_touch(&u, 0, CR_TOUCH_DOWN, 160, 100);
    cr_ui_update(&u, .1);
    cr_ui_touch(&u, 0, CR_TOUCH_CANCEL, 0, 0);
    check(u.feedback_alpha == 0 && writes == 2, "cancel clears highlight without action");
    row(&u, 0);
    check(!u.audio, "audio disabled before reset gesture");
    cr_ui_touch_at(&u, 0, CR_TOUCH_DOWN, 160, 100, 50);
    cr_ui_touch_at(&u, 0, CR_TOUCH_UP, 160, 100, 51.1);
    check(u.audio && saved_audio == 1, "long press restores a boolean setting default");
    int before_reset = writes;
    cr_ui_touch_at(&u, 0, CR_TOUCH_DOWN, 160, 100, 60);
    cr_ui_touch_at(&u, 0, CR_TOUCH_UP, 160, 100, 61.1);
    check(u.audio && writes == before_reset, "resetting an existing default is idempotent");
    cr_ui_back(&u);
    check(u.page == CR_SETTINGS && u.focus == 2 && u.tab == 2, "settings parent restored");
    cr_ui_touch(&u, 0, CR_TOUCH_DOWN, 160, 100);
    cr_ui_update(&u, .1);
    cr_ui_touch(&u, 0, CR_TOUCH_MOVE, 160, 150);
    cr_ui_update(&u, .2);
    cr_ui_touch(&u, 0, CR_TOUCH_UP, 160, 150);
    check(u.page == CR_SETTINGS && u.feedback_alpha == 0,
          "scroll cancels touch action and feedback");
    row(&u, 0);
    row(&u, 0);
    row(&u, 1);
    check(u.page == CR_UI_SETTINGS && u.theme == CR_THEME_SHIELD,
          "theme selection returns to interface category");
    cr_ui_back(&u);
    row(&u, 3);
    row(&u, 0);
    row(&u, 0);
    check(u.page == CR_OVERLAY_SETTINGS && !u.show_input && saved_show == 0 && u.depth == 2,
          "OSD overlay preference is functional and saved");
    cr_ui_back(&u);
    cr_ui_back(&u);
    check(u.page == CR_SETTINGS && u.focus == 3, "nested settings parent selection restored");
    int page = u.page;
    tap(&u, -1, 690);
    check(u.page == page, "outside viewport cannot switch tabs");
    // Drawing a quick game tap must still show the press after emulation consumes pulses.
    u.page = CR_PLAY;
    u.engine.loaded = true;
    u.engine.system = 0;
    u.engine.width = 160;
    u.engine.height = 144;
    u.show_input = true;
    cr_ui_clear_input(&u);
    c = draw(&u);
    uint32_t idle = c.a_body;
    unsigned idle_text = c.a_text >> 24;
    float ax = c.ax, ay = c.ay;
    cr_ui_touch_at(&u, 0, CR_TOUCH_DOWN, ax, ay, 100);
    check(cr_ui_input(&u) == 16, "game input immediate");
    cr_ui_touch_at(&u, 0, CR_TOUCH_UP, ax, ay, 100.02);
    cr_ui_update(&u, 1);
    c = draw(&u);
    check((c.a_body >> 24) == 255 && (c.a_text >> 24) == 255 && c.a_body != idle,
          "brief touch remains visibly pressed after release and pulse consumption");
    cr_ui_update(&u, .05);
    c = draw(&u);
    check((c.a_body >> 24) > (idle >> 24) && (c.a_body >> 24) < 255, "press fades out smoothly");
    cr_ui_update(&u, .2);
    c = draw(&u);
    check(c.a_body == idle && (c.a_text >> 24) == idle_text && cr_ui_input(&u) == 0,
          "fade returns to idle without holding game input");
    u.show_input = false;
    cr_ui_touch(&u, 0, CR_TOUCH_DOWN, ax, ay);
    c = draw(&u);
    check(c.a_body == idle && (c.a_text >> 24) == idle_text && cr_ui_input(&u) == 16,
          "disabling feedback preserves game input");
    cr_ui_touch(&u, 0, CR_TOUCH_CANCEL, 0, 0);
    u.engine.loaded = false;
    cr_ui_destroy(&u);
    rmdir(root);
    puts("PASS menu hierarchy, touch feedback, event timing, saved settings and game opacity");
    return 0;
}
