// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
#include "menu_driver.h"
#include "viewport.h"
#include "play_layout.h"
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
static bool import_called, import_valid;
static bool finish_import(void *context, bool valid, int system, const char *core_id,
                          const char *system_id, char *error, size_t capacity) {
    (void)context; (void)system; (void)core_id; (void)system_id; (void)error; (void)capacity;
    import_called = true;
    import_valid = valid;
    return valid;
}
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
    cr_platform p = {.pick = pick, .set_preference = set, .finish_import = finish_import};
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
    check(picked == -1 && !u.importing && strstr(u.message, "Chargez un cœur"),
          "installed modules alone cannot open the content picker");
    cr_import late = {.system = 0, .title = "Original.gb", .path = "/missing/original.gb"};
    snprintf(late.id, sizeof(late.id), "%064d-0", 0);
    cr_ui_import(&u, &late);
    check(import_called && !import_valid && !u.engine.handle,
          "late picker completion is rejected without committing when the core was unloaded");
    u.games = calloc(1, sizeof(*u.games));
    check(u.games != NULL, "allocate historical game");
    u.game_count = 1;
    u.games[0].system = 0; u.games[0].played = 1;
    u.page = CR_GAMES; u.filter = -1; u.focus = 0; u.message[0] = 0;
    cr_ui_confirm(&u);
    check(!u.engine.handle && !u.engine.loaded && strstr(u.message, "Chargez un cœur"),
          "history cannot implicitly load a core");
    check(cr_engine_select(&u.engine, 1), "select a real incompatible GBC core");
    void *selected = u.engine.handle;
    u.message[0] = 0;
    cr_ui_confirm(&u);
    check(u.engine.handle == selected && !u.engine.loaded && strstr(u.message, "associé"),
          "history cannot silently replace the selected core");
    import_called = false;
    cr_ui_import(&u, &late);
    check(import_called && !import_valid && u.engine.handle == selected && !u.engine.loaded,
          "late incompatible picker completion keeps the selected core");
    free(u.games); u.games = NULL; u.game_count = 0;
    check(cr_engine_close(&u.engine), "release incompatible core");
    u.page = CR_MAIN;
    tap(&u, 100, 100); tap(&u, 100, 100);
    tap(&u, 100, 170);
    check(picked == 0 && u.importing, "content picker dispatched to platform");
    cr_import cancel = {.system = -1};
    cr_ui_import(&u, &cancel);
    check(!u.importing, "cancel releases importer");
    check(cr_engine_close(&u.engine), "release core before input-only fixtures");
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
    cr_play_layout input_layout = cr_play_layout_make(u.width, u.height, u.engine.system,
                                                    u.engine.width, u.engine.height);
    cr_area a = input_layout.controls[4].area, left = input_layout.controls[2].area;
    float ax = a.x + a.w / 2, ay = a.y + a.h / 2;
    float lx = left.x + left.w / 2, ly = left.y + left.h / 2;
    cr_ui_touch(&u, 0, CR_TOUCH_DOWN, ax, ay);
    cr_ui_touch(&u, 1, CR_TOUCH_DOWN, lx, ly);
    unsigned held = cr_ui_input(&u);
    check((held & 16) && (held & 4), "simultaneous A and left");
    cr_ui_touch(&u, 0, CR_TOUCH_UP, ax, ay);
    held = cr_ui_input(&u);
    check(!(held & 16) && (held & 4), "one release preserves other pointer");
    cr_ui_touch(&u, 1, CR_TOUCH_CANCEL, 0, 0);
    check(cr_ui_input(&u) == 0, "cancel clears held input");
    cr_ui_touch(&u, 0, CR_TOUCH_DOWN, ax, ay);
    cr_ui_touch(&u, 0, CR_TOUCH_UP, ax, ay);
    check(cr_ui_input(&u) & 16, "short tap latched until frame");
    check(cr_ui_input(&u) == 0, "latched tap consumed exactly once");
    cr_ui_back(&u);
    check(u.quick && !cr_ui_running(&u), "quick menu pauses native loop");
    cr_ui_back(&u);
    check(!u.quick && cr_ui_running(&u), "back resumes native loop");
    cr_ui_size(&u, 700, 300);
    cr_ui_back(&u);
    for (int i = 0; i < CR_QUICK_ACHIEVEMENTS; i++)
        cr_ui_move(&u, 1);
    check(u.focus == CR_QUICK_ACHIEVEMENTS && u.quick_scroll >= 6 * 76 - (300 - 100 - 64),
          "quick menu controller focus stays visible in landscape");
    cr_ui_size(&u, 384, 700);
    check(u.quick_scroll <= CR_QUICK_COUNT * 76 - (700 - 100 - 64) &&
              u.focus * 76 >= u.quick_scroll && u.focus * 76 + 76 <= u.quick_scroll + 700 - 100 - 64,
          "quick menu scroll clamps on rotation while keeping the focused row visible");
    u.focus = CR_QUICK_STATES;
    cr_ui_confirm(&u);
    check(u.page == CR_SAVESTATES && !u.quick && !cr_ui_running(&u),
          "supported state actions open their own paused submenu");
    cr_ui_back(&u);
    check(u.page == CR_PLAY && u.quick && u.focus == CR_QUICK_STATES && u.depth == 0,
          "state submenu back restores the quick-menu selection");
    cr_ui_size(&u, 700, 300);
    u.focus = CR_QUICK_CORE_INFO;
    cr_ui_move(&u, 0);
    float quick_scroll = u.quick_scroll;
    cr_ui_confirm(&u);
    check(u.page == CR_CORE_INFO && !cr_ui_running(&u), "quick core information stays paused");
    cr_ui_back(&u);
    check(u.quick && u.focus == CR_QUICK_CORE_INFO && u.quick_scroll == quick_scroll,
          "core information back restores landscape focus and scroll");
    cr_ui_touch(&u, 0, CR_TOUCH_DOWN, 350, 150);
    cr_ui_touch(&u, 0, CR_TOUCH_MOVE, 350, 220);
    cr_ui_touch(&u, 0, CR_TOUCH_UP, 350, 220);
    check(u.quick && u.quick_scroll < quick_scroll, "quick list supports drag scrolling");
    cr_ui_back(&u);
    cr_play_layout layout = cr_play_layout_make(u.width, u.height, 2, 0, 0);
    tap(&u, layout.menu.x + layout.menu.w / 2, layout.menu.y + layout.menu.h / 2);
    check(u.quick, "landscape menu hit follows the gutter layout");
    u.focus = CR_QUICK_MAIN;
    cr_ui_confirm(&u);
    check(u.page == CR_MAIN && !u.quick && u.engine.loaded && !cr_ui_running(&u),
          "quick main-menu action preserves paused content");
    cr_ui_confirm(&u);
    check(u.page == CR_PLAY && u.quick, "main menu reopens the quick menu");
    u.focus = CR_QUICK_RESUME;
    cr_ui_confirm(&u);
    check(cr_ui_running(&u) && cr_ui_input(&u) == 0, "explicit resume releases menu input");
    // No engine handle is attached to this input-only test state.
    u.engine.loaded = false;
    cr_ui_destroy(&u);
    rmdir(root);
    puts("PASS native navigation, picker, themes, multi-pointer and pause contracts");
    return 0;
}
