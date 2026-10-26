// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
#include "menu_driver.h"
#include "viewport.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
static void check(bool value, const char *text) {
    if (!value) {
        fprintf(stderr, "FAIL %s\n", text);
        exit(1);
    }
}
static int picked = -1, theme = -1;
static void pick(void *ctx, int system) {
    (void)ctx;
    picked = system;
}
static void set(void *ctx, const char *key, int value) {
    (void)ctx;
    if (strcmp(key, "theme") == 0)
        theme = value;
}
static void tap(cr_ui *u, float x, float y) {
    cr_ui_touch(u, 0, CR_TOUCH_DOWN, x, y);
    cr_ui_touch(u, 0, CR_TOUCH_UP, x, y);
}
static int fixture_reset(void *handle) { (void)handle; return 1; }
static size_t fixture_state_size(void *handle) { (void)handle; return 1; }
static int fixture_serialize(void *handle, uint8_t *out, size_t size) {
    (void)handle; (void)out; (void)size; return 1;
}
static int fixture_unserialize(void *handle, const uint8_t *data, size_t size) {
    (void)handle; (void)data; (void)size; return 1;
}

int main(void) {
    check(cr_content_system("test.GBC", 0) == 1 && cr_content_system("test.gba", 1) == 2,
          "native system association");
    check(cr_content_system("test.gb", 1) == 1 && cr_content_system("test.gb", 2) == 0,
          "GB selection compatibility");
    char root[] = "/tmp/corvus-frontend-XXXXXX";
    check(mkdtemp(root) != NULL, "temporary storage");
    cr_platform p = {.pick = pick, .set_preference = set};
    cr_ui u;
    cr_ui_init(&u, p, root);
    cr_ui_size(&u, 384, 700);
    check(u.page == CR_MAIN && !u.engine.handle, "initial menu without core");
    check(u.theme == CR_THEME_CORVUS, "logo palette is the default without saved preference");
    check(cr_theme_valid(-1) == CR_THEME_CORVUS && cr_theme_valid(99) == CR_THEME_CORVUS,
          "unknown themes fall back safely");
    check(CR_THEME_SHIELD == 0 && CR_THEME_CORVUS == 2 && CR_THEME_ASUS == 3,
          "remaining published theme IDs retain compatibility");
    check(cr_theme_valid(1) == CR_THEME_CORVUS, "retired theme falls back to Corvus");
    check(CR_THEME_COUNT == 3 && cr_theme_at(0) == CR_THEME_CORVUS &&
              cr_theme_at(1) == CR_THEME_SHIELD && cr_theme_at(2) == CR_THEME_ASUS &&
              cr_theme_at(3) == CR_THEME_CORVUS,
          "three active themes only");
    tap(&u, 100, 100);
    check(u.page == CR_CORES, "touch opens native core list");
    cr_ui_move(&u, 1);
    cr_ui_confirm(&u);
    check(u.page == CR_MAIN && u.engine.system == 1 && u.engine.handle,
          "controller selects real idle GBC");
    tap(&u, 100, 100);
    check(!u.engine.handle, "native unload releases engine");
    cr_ui_touch(&u, 0, CR_TOUCH_DOWN, 100, 100);
    cr_ui_touch(&u, 0, CR_TOUCH_MOVE, 100, 150);
    cr_ui_touch(&u, 0, CR_TOUCH_UP, 100, 150);
    check(u.page == CR_MAIN, "drag does not activate row");
    tap(&u, 100, 170);
    check(picked == 0 && u.importing, "content picker dispatched to platform");
    cr_import cancel = {.system = -1};
    cr_ui_import(&u, &cancel);
    check(!u.importing, "cancel releases importer");
    tap(&u, 330, 670);
    check(u.page == CR_SETTINGS, "native bottom tab");
    tap(&u, 100, 100);
    check(u.page == CR_UI_SETTINGS, "interface settings category");
    tap(&u, 100, 100);
    check(u.page == CR_THEMES, "native theme list");
    tap(&u, 100, 176);
    check(theme == CR_THEME_SHIELD && u.theme == CR_THEME_SHIELD && u.page == CR_UI_SETTINGS,
          "Shield selection remains stable");
    tap(&u, 100, 100);
    tap(&u, 100, 252);
    check(theme == CR_THEME_ASUS && u.theme == CR_THEME_ASUS, "ASUS is selectable and saved");
    tap(&u, 100, 100);
    tap(&u, 100, 100);
    check(theme == CR_THEME_CORVUS && u.theme == CR_THEME_CORVUS, "Corvus is selectable and saved");
    cr_ui_back(&u);
    check(u.page == CR_SETTINGS, "back restores settings root");
    cr_ui_back(&u);
    check(u.page == CR_MAIN, "back navigation");
    // Multiple pointers and cancellation are independent of the rendering driver.
    u.page = CR_PLAY;
    u.engine.loaded = true;
    u.engine.system = 2;
    /* This test drives the menu without loading a real game core. */
    u.engine.api.reset = fixture_reset;
    u.engine.api.serialize_size = fixture_state_size;
    u.engine.api.serialize = fixture_serialize;
    u.engine.api.unserialize = fixture_unserialize;
    cr_ui_size(&u, 384, 700);
    cr_ui_touch(&u, 0, CR_TOUCH_DOWN, 330, 520);
    cr_ui_touch(&u, 1, CR_TOUCH_DOWN, 40, 565);
    unsigned held = cr_ui_input(&u);
    check((held & 16) && (held & 4), "simultaneous A and left");
    cr_ui_touch(&u, 0, CR_TOUCH_UP, 330, 520);
    held = cr_ui_input(&u);
    check(!(held & 16) && (held & 4), "one release preserves other pointer");
    cr_ui_touch(&u, 1, CR_TOUCH_CANCEL, 0, 0);
    check(cr_ui_input(&u) == 0, "cancel clears held input");
    cr_ui_touch(&u, 0, CR_TOUCH_DOWN, 330, 520);
    cr_ui_touch(&u, 0, CR_TOUCH_UP, 330, 520);
    check(cr_ui_input(&u) & 16, "short tap latched until frame");
    check(cr_ui_input(&u) == 0, "latched tap consumed exactly once");
    cr_ui_back(&u);
    check(u.quick && !cr_ui_running(&u), "quick menu pauses native loop");
    cr_ui_back(&u);
    check(!u.quick && cr_ui_running(&u), "back resumes native loop");
    cr_ui_size(&u, 700, 300);
    cr_ui_back(&u);
    for (int i = 0; i < 5; i++)
        cr_ui_move(&u, 1);
    check(u.focus == 5 && u.quick_scroll >= 336 - (300 - 16 - 56),
          "quick menu controller focus stays visible in landscape");
    cr_ui_size(&u, 384, 700);
    check(u.quick_scroll == 0, "quick menu scroll clamps on rotation");
    // No engine handle is attached to this input-only test state.
    u.engine.loaded = false;
    cr_ui_destroy(&u);
    rmdir(root);
    puts("PASS native navigation, picker, themes, multi-pointer and pause contracts");
    return 0;
}
