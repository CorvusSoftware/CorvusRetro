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
typedef struct {
    char text[16384];
    uint32_t highlight, a_body, a_text;
    float ax, ay, ar;
    const cr_play_layout *layout;
    uint32_t popup_color;
    unsigned popup_count;
    unsigned badge_count;
    float minimum_badge;
    float viewport_center;
    bool home_tab, settings_tab;
    unsigned switch_ends, games_drawn, header_crows, row_icons;
    bool full_width_selection;
    float switch_thumb, toggle_text_end;
} capture;
static void rect(void *ctx, float x, float y, float w, float h, uint32_t color) {
    capture *c = ctx;
    if (c->layout && color == c->popup_color) {
        if (!c->popup_count)
            check(fabsf(x + w / 2 - c->viewport_center) < .01f, "launch and achievement cards stay centered in the viewport");
        ++c->popup_count;
        cr_area system_buttons[] = {c->layout->menu, c->layout->hide};
        for (size_t i = 0; i < sizeof(system_buttons) / sizeof(system_buttons[0]); ++i) {
            cr_area a = system_buttons[i];
            check(x + w <= a.x || a.x + a.w <= x || y + h <= a.y || a.y + a.h <= y,
                  "notification does not cover menu or hide targets");
        }
        for (int i = 0; i < c->layout->count; ++i) {
            cr_area a = c->layout->controls[i].area;
            check(x + w <= a.x || a.x + a.w <= x || y + h <= a.y || a.y + a.h <= y,
                  "notification does not cover a touch control");
        }
    }
    if (x == 0 && y == 64 && w == 384 && h == 76) c->full_width_selection = true;
    if (x == 0 && y == 216 && w == 384 && h == 76)
        c->highlight = color;
}
static void circle(void *ctx, float x, float y, float r, uint32_t color) {
    (void)r;
    capture *c = ctx;
    if (fabsf(y - 102) < .01f && r == 14) ++c->switch_ends;
    if (fabsf(y - 102) < .01f && r == 10) c->switch_thumb = x;
    if (x > c->ax - 1 && x < c->ax + 1 && y > c->ay - 1 && y < c->ay + 1)
        c->a_body = color;
}
static void line(void *ctx, float x1, float y1, float x2, float y2, float thickness,
                 uint32_t color) {
    capture *c = ctx;
    (void)thickness;
    float r1 = hypotf(x1 - c->ax, y1 - c->ay), r2 = hypotf(x2 - c->ax, y2 - c->ay);
    if (c->ar > 0 && fabsf(r1 - c->ar) < 3 && fabsf(r2 - c->ar) < 3)
        c->a_body = color;
}
static void text(void *ctx, const char *s, float x, float y, float size, uint32_t color,
                 float max) {
    (void)x;
    (void)y;
    (void)size;
    (void)max;
    capture *c = ctx;
    if (x == 66 && y == 93) c->toggle_text_end = x + max;
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
    capture *c = ctx;
    if (kind == CR_ICON_CROW && y == 16) ++c->header_crows;
    if (x == 20 && size == 32) ++c->row_icons;
    if (y > 600 && kind == CR_ICON_HOME) c->home_tab = true;
    if (y > 600 && kind == CR_ICON_SETTINGS) c->settings_tab = true;
    if (c->layout && kind == CR_ICON_CROW) {
        if (!c->badge_count)
            check(size + .001f >= c->minimum_badge, "centered layout never shrinks the card below the original safe area");
        ++c->badge_count;
    }
    (void)x;
    (void)y;
    (void)size;
    (void)color;
}
static void game(void *ctx, const uint32_t *pixels, int iw, int ih, float x, float y, float w,
                 float h) {
    capture *c = ctx;
    ++c->games_drawn;
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
    capture c = {.ax = a.x + a.w / 2, .ay = a.y + a.h / 2, .ar = a.w / 2};
    cr_painter painter = {&c, rect, circle, text, measure, icon, NULL, game, NULL, NULL, line};
    cr_ui_draw(u, &painter);
    return c;
}
static int saved_show = -1, saved_audio = -1, writes;
static int core_refreshes, core_installs, folder_calls, folder_system;
static char folder_extensions[128];
static char selected_pack[96];
static void pick_folder(void *ctx, int system, const char *extensions) {
    (void)ctx;
    folder_calls++;
    folder_system = system;
    snprintf(folder_extensions, sizeof(folder_extensions), "%s", extensions);
}
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
static bool install_pending(void *ctx) {
    return *(bool *)ctx;
}
static void loaded_core_lifecycle(cr_ui *u, const char *root) {
    /* Original ARM program writes SRAM once, then loops. No external ROM. */
    uint8_t rom[512] = {0};
    const uint32_t words[] = {0xe59f0008, 0xe3a0105a, 0xe5c01000, 0xeafffffe, 0x0e000000};
    for (size_t i = 0; i < sizeof(words) / sizeof(words[0]); i++)
        for (unsigned b = 0; b < 4; b++)
            rom[i * 4 + b] = (uint8_t)(words[i] >> (b * 8));
    rom[0xb2] = 0x96;
    memcpy(rom + 0x100, "SRAM_V", 6);
    char path[4096], saves[4096];
    snprintf(path, sizeof(path), "%s/test.gba", root);
    FILE *file = fopen(path, "wb");
    check(file && fwrite(rom, 1, sizeof(rom), file) == sizeof(rom), "write original GBA fixture");
    check(fclose(file) == 0, "close original GBA fixture");
    const char *id = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa-2";
    check(cr_engine_open(&u->engine, 2, id, path) && cr_engine_step(&u->engine, 0),
          "load real game and generate battery data through CorvusAPI");
    check(u->engine.api.battery_dirty(u->engine.handle), "test game has unsaved SRAM");
    cr_notifications_push(&u->notifications, CR_NOTIFICATION_GAME, 0, "Original GBA test", "Jeu lancé", "");
    cr_notifications_progress(&u->notifications, 1, "1/10", "", 10);
    u->notifications_loaded = true;
    cr_ui_update(u, .4);
    snprintf(u->title, sizeof(u->title), "Original GBA test");
    capture c = draw(u);
    check(strstr(c.text, "Menu rapide") && strstr(c.text, "Décharger le cœur") &&
              strstr(c.text, "Configuration du sous-système") && !strstr(c.text, "Charger un cœur"),
          "loaded content keeps one core action after the quick menu row");
    void *state_handle = u->engine.handle;
    for (int landscape = 0; landscape < 2; ++landscape) {
        cr_ui_size(u, landscape ? 700 : 384, landscape ? 384 : 700);
        u->page = CR_PLAY; u->quick = true; u->focus = CR_QUICK_CORE_OPTIONS;
        cr_ui_move(u, 0);
        float scroll = u->quick_scroll;
        cr_ui_confirm(u);
        c = draw(u);
        check(u->page == CR_CORE_OPTIONS && !u->quick && !cr_ui_running(u) &&
                  strstr(c.text, "Type de sauvegarde") && strstr(c.text, "Horloge de cartouche") &&
                  strstr(c.text, "Son du câble link"),
              "real GBA configuration options are visible in both orientations");
        check(!strstr(c.text, "Afficher les commandes") && u->engine.handle == state_handle,
              "core options are separate from frontend overlay settings and preserve the engine");
        cr_ui_confirm(u);
        c = draw(u);
        check(u->page == CR_CORE_OPTION_VALUES && u->engine.loaded && strstr(c.text, "Automatique") &&
                  strstr(c.text, "SRAM") && !cr_ui_running(u),
              "core option opens provider values while keeping the content paused");
        cr_ui_back(u);
        check(u->page == CR_CORE_OPTIONS && u->focus == 0 && !cr_ui_running(u),
              "option values back restores the options row without resuming");
        cr_ui_back(u);
        check(u->quick && u->focus == CR_QUICK_CORE_OPTIONS && u->quick_scroll == scroll &&
                  !cr_ui_running(u) && cr_ui_input(u) == 0,
              "core options back restores quick focus and scroll while keeping the game paused");
        cr_ui_back(u);
        check(cr_ui_running(u), "second back resumes the preserved content");
    }
    cr_ui_size(u, 384, 700);
    u->page = CR_MAIN;
    snprintf(path, sizeof(path), "%s/states", root);
    file = fopen(path, "wb");
    check(file && fclose(file) == 0, "obstruct state storage for the quick submenu");
    u->page = CR_PLAY; u->quick = true; u->focus = CR_QUICK_STATES;
    cr_ui_confirm(u);
    check(u->page == CR_SAVESTATES && !cr_ui_running(u), "real core exposes the state submenu");
    cr_ui_confirm(u);
    check(u->page == CR_SAVESTATES && u->engine.handle == state_handle && u->engine.loaded &&
              u->message[0], "failed state save preserves the loaded core and paused submenu");
    check(unlink(path) == 0, "remove state directory obstruction");
    cr_ui_confirm(u);
    check(cr_ui_running(u) && u->engine.handle == state_handle && u->depth == 0 &&
              strstr(u->message, "État sauvegardé"), "state submenu saves through the real engine and resumes");
    cr_ui_back(u); u->focus = CR_QUICK_STATES;
    cr_ui_confirm(u); u->focus = 1;
    cr_ui_confirm(u);
    check(cr_ui_running(u) && u->engine.handle == state_handle && u->depth == 0 &&
              strstr(u->message, "État chargé"), "state submenu loads through the real engine and resumes");
    snprintf(path, sizeof(path), "%s/states/%s-" CR_CORE_VERSION ".state", root, id);
    check(unlink(path) == 0, "remove quick-menu state fixture");
    snprintf(path, sizeof(path), "%s/states", root);
    check(rmdir(path) == 0, "remove quick-menu state directory");
    u->page = CR_MAIN;
    snprintf(saves, sizeof(saves), "%s/saves", root);
    check(rmdir(saves) == 0, "replace empty battery directory with an obstruction");
    file = fopen(saves, "wb");
    check(file && fclose(file) == 0, "block battery directory to reproduce disk failure");
    void *handle = u->engine.handle;
    int installs = core_installs;
    u->core_catalog.count = 1;
    snprintf(u->core_catalog.packs[0].id, sizeof(u->core_catalog.packs[0].id), "corvus.gb");
    u->page = CR_CORE_DOWNLOADS;
    row(u, 1);
    check(core_installs == installs && !u->installing && u->engine.handle == handle,
          "failed battery persistence refuses installation before downloading");
    u->page = CR_MAIN;
    row(u, 1);
    c = draw(u);
    check(u->page == CR_MAIN && u->engine.handle == handle && u->engine.loaded &&
              strcmp(u->title, "Original GBA test") == 0 && u->message[0] &&
              strstr(c.text, "Décharger le cœur") && !strstr(c.text, "Charger un cœur"),
          "failed battery persistence keeps the game and unload action available for retry");
    cr_ui_update(u, .01);
    check(u->notifications.count == 1 && u->notifications.progress.visible,
          "refused unload keeps the running game's notifications");
    u->page = CR_PLAY; u->quick = true; u->focus = CR_QUICK_CLOSE;
    cr_ui_confirm(u);
    check(u->engine.loaded && u->engine.handle == handle && u->quick &&
              u->notifications.count == 1 && u->notifications.progress.visible,
          "failed save also refuses content close without losing the core or notifications");
    u->quick = false;
    check(unlink(saves) == 0, "remove battery directory obstruction");
    u->page = CR_CORE_DOWNLOADS;
    row(u, 1);
    check(core_installs == installs + 1 && u->installing &&
              !u->engine.api.battery_dirty(u->engine.handle),
          "installation starts only after the running game's battery is persisted");
    u->page = CR_PLAY;
    check(!cr_ui_running(u), "installation pauses the current game before Android can kill it");
    cr_ui_install_status(u, "Téléchargement en cours", false);
    check(u->installing && !cr_ui_running(u), "progress cannot release the installation barrier");
    u->quick = true;
    u->focus = CR_QUICK_CLOSE;
    cr_ui_confirm(u);
    check(u->engine.handle == handle && u->installing,
          "quick-menu close cannot mutate a game during installation");
    u->quick = false;
    cr_ui_install_status(u, "Installation annulée", true);
    check(!u->installing && cr_ui_running(u) && u->engine.handle == handle,
          "terminal cancellation resumes the preserved game");
    u->quick = true; u->focus = CR_QUICK_CLOSE;
    cr_ui_confirm(u);
    c = draw(u);
    check(u->page == CR_MAIN && u->engine.handle == handle && !u->engine.loaded &&
              !u->title[0] && strstr(c.text, "Décharger le cœur") &&
              strstr(c.text, "Configuration du sous-système") && !strstr(c.text, "Menu rapide"),
          "closing content preserves the selected core and returns to the main menu");
    check(!u->notifications.count && !u->notifications.progress.visible && !u->notifications_loaded,
          "content close clears notifications immediately while keeping the core");
    row(u, 0);
    c = draw(u);
    check(!u->engine.handle && !u->engine.loaded && !u->title[0] &&
              strstr(c.text, "Charger un cœur") && !strstr(c.text, "Décharger le cœur") &&
              !strstr(c.text, "Menu rapide") && !strstr(c.text, "Configuration du sous-système"),
          "retry persists the game and restores the single load action");
    cr_ui_update(u, .01);
    check(!u->notifications.count && !u->notifications.progress.visible && !u->notifications_loaded,
          "successful unload clears cards and measured progress even without an account");
    snprintf(path, sizeof(path), "%s/saves/%s.sav", root, id);
    check(access(path, F_OK) == 0 && unlink(path) == 0, "battery survived successful unload");
    check(rmdir(saves) == 0, "remove test saves directory");
    snprintf(path, sizeof(path), "%s/test.gba", root);
    check(unlink(path) == 0, "remove original ROM fixture");
}
int main(void) {
    char root[] = "/tmp/corvus-menu-XXXXXX";
    check(mkdtemp(root) != NULL, "temporary storage");
    bool pending = true;
    cr_ui restored;
    cr_ui_init(&restored, (cr_platform){.context = &pending,
                                       .core_install_pending = install_pending}, root);
    restored.page = CR_PLAY;
    restored.engine.loaded = true;
    check(restored.installing && !cr_ui_running(&restored),
          "recreated frontend restores the pending Android installation barrier");
    cr_ui_install_status(&restored, "Installation annulée", true);
    check(!restored.installing && cr_ui_running(&restored),
          "terminal status releases the restored installation barrier");
    restored.engine.loaded = false;
    cr_ui_destroy(&restored);
    cr_ui u;
    cr_ui_init(&u, (cr_platform){.set_preference = set,
                                .refresh_core_catalog = refresh_core_catalog,
                                .install_core_pack = install_core_pack,
                                .pick_folder = pick_folder}, root);
    cr_ui_size(&u, 384, 700);
    capture c = draw(&u);
    check(c.home_tab && c.settings_tab, "navigation uses home and settings icons");
    check(c.header_crows == 0, "menu header no longer draws the crow icon");
    cr_game *long_history = calloc(1200, sizeof(*long_history));
    check(long_history != NULL, "allocate long history fixture");
    for (size_t i = 0; i < 1200; ++i) {
        long_history[i].played = 1;
        snprintf(long_history[i].title, sizeof(long_history[i].title), "Scroll item %zu", i);
    }
    u.games = long_history;
    u.game_count = 1200;
    u.page = CR_GAMES;
    u.scroll = 1000 * 76;
    capture scrolled = draw(&u);
    check(scrolled.row_icons > 0 && scrolled.row_icons <= 10 &&
          strstr(scrolled.text, "Scroll item 1000") != NULL &&
          strstr(scrolled.text, "Scroll item 0\n") == NULL,
          "long history only renders on-screen rows around the scroll position");
    u.scroll = 0;
    cr_ui_touch_at(&u, 0, CR_TOUCH_DOWN, 160, 340, 100);
    cr_ui_touch_at(&u, 0, CR_TOUCH_MOVE, 160, 260, 100.05);
    cr_ui_touch_at(&u, 0, CR_TOUCH_UP, 160, 260, 100.06);
    float after_release = u.scroll;
    cr_ui_update(&u, .016);
    check(after_release >= 80 && u.scroll > after_release,
          "fast swipe continues scrolling briefly after touch release");
    cr_ui_touch_at(&u, 0, CR_TOUCH_DOWN, 160, 260, 100.08);
    check(u.scroll_velocity == 0, "new touch stops inertial scrolling");
    cr_ui_touch_at(&u, 0, CR_TOUCH_CANCEL, 0, 0, 100.09);
    u.games = NULL;
    u.game_count = 0;
    u.page = CR_MAIN;
    u.scroll = 0;
    free(long_history);
    check(strstr(c.text, "Charger un cœur") && strstr(c.text, "Informations") &&
              !strstr(c.text, "Historique") && !strstr(c.text, "Décharger le cœur") &&
              !strstr(c.text, "Configuration du sous-système"),
          "main contains loading and information, history lives in playlists");
    row(&u, 1);
    check(u.page == CR_CONTENT, "content entry opens import and library choices");
    c = draw(&u);
    check(strstr(c.text, "Importer un jeu") && strstr(c.text, "Bibliothèque"),
          "content submenu displays library");
    row(&u, 1);
    check(u.page == CR_LIBRARY, "library selection opens saved games");
    c = draw(&u);
    check(strstr(c.text, "Importer un dossier") && strstr(c.text, "Aucun jeu"),
          "empty library offers folder import");
    cr_ui_back(&u);
    cr_ui_back(&u);
    check(u.page == CR_MAIN, "back navigation restores content parent");
    row(&u, 0);
    check(u.page == CR_CORES, "core menu opens before downloader");
    c = draw(&u);
    check(strstr(c.text, "Téléchargeur de cœurs"), "core menu exposes online updater");
    row(&u, 3);
    check(u.page == CR_CORE_DOWNLOADS && core_refreshes == 1 &&
          u.catalog_loading, "downloader starts catalog request on entry");
    cr_core_catalog catalog = {0};
    catalog.count = 1;
    snprintf(catalog.packs[0].id, sizeof(catalog.packs[0].id), "corvus.gb");
    snprintf(catalog.packs[0].name, sizeof(catalog.packs[0].name), "CorvusGB");
    snprintf(catalog.packs[0].version, sizeof(catalog.packs[0].version), "1.0");
    snprintf(catalog.packs[0].abi, sizeof(catalog.packs[0].abi), "arm64-v8a");
    snprintf(catalog.packs[0].core_id, sizeof(catalog.packs[0].core_id), "corvus.gb");
    cr_ui_catalog(&u, &catalog);
    c = draw(&u);
    check(!u.catalog_loading && strstr(c.text, "CorvusGB") && strstr(c.text, "mise à jour"),
          "individual core with a different version is presented as an update");
    snprintf(catalog.packs[0].version, sizeof(catalog.packs[0].version), "0.15.0");
    cr_ui_catalog(&u, &catalog);
    c = draw(&u);
    check(strstr(c.text, "Installé") && !strstr(c.text, "mise à jour"),
          "an identical installed core is marked installed instead of updated");
    u.importing = true;
    row(&u, 1);
    check(core_installs == 0 && !u.installing,
          "a pending content import cannot race a module installation");
    u.importing = false;
    row(&u, 1);
    check(core_installs == 1 && strcmp(selected_pack, "corvus.gb") == 0,
          "frontend requests one verified native core by catalog ID only");
    check(u.installing, "download keeps a barrier until its terminal status");
    row(&u, 1);
    check(core_installs == 1, "a second install cannot start while one is pending");
    cr_ui_install_status(&u, "Installation annulée", true);
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
    c = draw(&u);
    check(strstr(c.text, "Décharger le cœur") && !strstr(c.text, "Charger un cœur"),
          "the same core row changes from load to unload");
    row(&u, 1);
    row(&u, 1);
    check(u.page == CR_LIBRARY, "loaded core can open folder library");
    row(&u, 0);
    check(folder_calls == 1 && folder_system == u.engine.system &&
          folder_extensions[0] && u.import_folder && u.importing && !u.engine.loaded,
          "folder picker uses the loaded core extensions without launching a game");
    cr_import folder_done = {.system = -1};
    snprintf(folder_done.error, sizeof(folder_done.error), "2 jeu(x) importé(s)");
    cr_ui_import(&u, &folder_done);
    check(!u.importing && !u.import_folder && strstr(u.message, "2 jeu(x)"),
          "folder import completion restores navigation and reports the result");
    cr_ui_back(&u);
    cr_ui_back(&u);
    check(u.page == CR_MAIN, "folder import returns to main through content page");
    row(&u, 2);
    c = draw(&u);
    check(u.page == CR_SUBSYSTEMS && !cr_ui_running(&u) &&
              strstr(c.text, "Deux joueurs (câble link)"),
          "loading a core exposes the subsystem configuration before any content is loaded");
    void *idle_handle = u.engine.handle;
    cr_ui_confirm(&u);
    c = draw(&u);
    check(u.page == CR_SUBSYSTEM_CONFIG && u.engine.handle == idle_handle && !u.engine.loaded &&
              strstr(c.text, "Joueur 1") && strstr(c.text, "Joueur 2"),
          "real link configuration exposes two ordered content slots before loading");
    cr_ui_back(&u);
    check(u.page == CR_SUBSYSTEMS && u.focus == 0, "link configuration back restores subsystem row");
    cr_ui_back(&u);
    check(u.page == CR_MAIN && u.focus == 2 && u.depth == 0,
          "subsystem configuration back restores its main menu row");
    row(&u, 3);
    row(&u, 1);
    c = draw(&u);
    check(u.page == CR_CORE_INFO && strstr(c.text, "CorvusGB") && strstr(c.text, "0.15.0"),
          "real core information is reachable");
    cr_ui_back(&u);
    cr_ui_back(&u);
    row(&u, 0);
    check(!u.engine.handle, "core unloaded");
    c = draw(&u);
    check(strstr(c.text, "Charger un cœur") && !strstr(c.text, "Décharger le cœur") &&
              !strstr(c.text, "Configuration du sous-système"),
          "successful unload restores the load action");
    loaded_core_lifecycle(&u, root);
    tab(&u, 1);
    c = draw(&u);
    check(u.page == CR_LISTS && strstr(c.text, "Historique") && !strstr(c.text, "Game Boy Advance") &&
              !strstr(c.text, "Game Boy Color"),
          "playlists contain history only");
    row(&u, 0);
    check(u.page == CR_GAMES && u.filter == -1 && u.tab == 1, "history category");
    cr_ui_back(&u);
    cr_ui_move(&u, 1);
    check(u.page == CR_LISTS && u.focus == 0, "history is the only selectable playlist");
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
    c = draw(&u);
    check(c.switch_ends == 2 && c.switch_thumb == (u.audio ? 346 : 326) && c.toggle_text_end <= 300,
          "rounded switch and label have distinct nonoverlapping bounds");
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
    c = draw(&u);
    check(c.switch_thumb == (u.audio ? 346 : 326), "switch thumb follows the actual saved setting");
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
    cr_play_layout controls = cr_play_layout_make(u.width, u.height, 0, 160, 144);
    cr_ui_touch(&u, 1, CR_TOUCH_DOWN, ax, ay);
    cr_ui_key(&u, 16, true);
    tap(&u, controls.hide.x + controls.hide.w / 2, controls.hide.y + controls.hide.h / 2);
    check(!u.show_input && saved_show == 0 && cr_ui_input(&u) == 0,
          "hide button persists visibility and releases every held input");
    cr_ui_touch(&u, 0, CR_TOUCH_DOWN, ax, ay);
    c = draw(&u);
    check(c.a_body == 0 && c.a_text == 0 && cr_ui_input(&u) == 0,
          "hidden virtual controls neither draw nor send game input");
    cr_ui_touch(&u, 0, CR_TOUCH_CANCEL, 0, 0);
    tap(&u, controls.menu.x + controls.menu.w / 2, controls.menu.y + controls.menu.h / 2);
    check(u.quick && !cr_ui_running(&u), "menu target stays interactive with hidden controls");
    c = draw(&u);
    check(c.games_drawn == 0 && c.full_width_selection,
          "quick menu fills the native interface without drawing the game underneath");
    const char *resume = strstr(c.text, "Reprendre le contenu");
    const char *restart = strstr(c.text, "Redémarrer le contenu");
    const char *close = strstr(c.text, "Fermer le contenu");
    const char *states = strstr(c.text, "Sauvegardes d’état");
    const char *options = strstr(c.text, "Options du cœur");
    const char *commands = strstr(c.text, "Commandes");
    check(resume && restart && close && states && options && commands &&
              resume < restart && restart < close && close < states && states < options && options < commands,
          "quick menu places core options after states and before frontend controls");
    check(!strstr(c.text, "Charger l’état") && !strstr(c.text, "Sauvegarder l’état") &&
              !strstr(c.text, "Shaders") && !strstr(c.text, "Triches"),
          "quick root contains only working menus and nests state actions");
    u.focus = CR_QUICK_RESTART;
    cr_ui_confirm(&u);
    check(u.quick, "unsupported restart remains disabled without changing row IDs");
    u.focus = CR_QUICK_STATES;
    cr_ui_confirm(&u);
    check(u.quick, "unsupported state operations remain disabled");
    u.focus = CR_QUICK_CONTROLS;
    cr_ui_move(&u, 0);
    float quick_scroll = u.quick_scroll;
    cr_ui_confirm(&u);
    check(u.page == CR_OVERLAY_SETTINGS && !u.quick && !cr_ui_running(&u),
          "quick commands opens the real overlay settings while paused");
    row(&u, 0);
    check(u.show_input && saved_show == 1, "quick commands updates the saved overlay setting");
    cr_ui_back(&u);
    check(u.page == CR_PLAY && u.quick && u.focus == CR_QUICK_CONTROLS &&
              u.quick_scroll == quick_scroll && cr_ui_input(&u) == 0,
          "submenu back restores quick-menu focus and scroll without resuming the game");
    u.focus = CR_QUICK_ACHIEVEMENTS;
    int quick_tab = u.tab;
    cr_ui_confirm(&u);
    check(u.page == CR_ACHIEVEMENTS && !cr_ui_running(&u), "quick achievements stays paused");
    cr_ui_back(&u);
    check(u.quick && u.focus == CR_QUICK_ACHIEVEMENTS && u.tab == quick_tab,
          "achievements returns to its quick row and navigation tab");
    tap(&u, 28, 28);
    check(!u.quick && cr_ui_running(&u), "quick header back resumes the game");
    tap(&u, controls.hide.x + controls.hide.w / 2, controls.hide.y + controls.hide.h / 2);
    check(!u.show_input, "hide target remains reversible after a menu round trip");
    tap(&u, controls.hide.x + controls.hide.w / 2, controls.hide.y + controls.hide.h / 2);
    check(u.show_input && saved_show == 1, "hidden controls can be shown again directly");
    const float popup_sizes[][2] = {{360, 640}, {640, 360}, {320, 240}, {640, 240}, {960, 540}, {240, 320}};
    u.message_time = 0;
    for (int system = 0; system < 3; ++system) {
        u.engine.system = system;
        u.engine.width = system == 2 ? 240 : 160;
        u.engine.height = system == 2 ? 160 : 144;
        for (size_t i = 0; i < sizeof(popup_sizes) / sizeof(popup_sizes[0]); ++i) {
            cr_ui_size(&u, popup_sizes[i][0], popup_sizes[i][1]);
            cr_notifications_clear(&u.notifications);
            cr_notifications_push(&u.notifications, CR_NOTIFICATION_GAME, 0, "Jeu", "Succès", "");
            cr_notifications_progress(&u.notifications, 1, "33/66", "", 50);
            cr_notifications_update(&u.notifications, 0);
            cr_notifications_update(&u.notifications, .8);
            cr_play_layout layout = cr_play_layout_make(u.width, u.height, system, u.engine.width, u.engine.height);
            capture popup = {.layout = &layout, .popup_color = (cr_theme_get(u.theme)->surface & 0xffffffu) | 0xf7000000u};
            float available_width = layout.portrait ? u.width : layout.video.w;
            float available_height = layout.portrait ? layout.panel.y : u.height;
            popup.viewport_center = u.width / 2;
            popup.minimum_badge = 64 * .7f * fmaxf(0, fminf(1,
                    fminf(available_width / 320, available_height / 104)));
            cr_painter painter = {&popup, rect, circle, text, measure, icon, NULL, game, NULL, NULL, line};
            cr_ui_draw(&u, &painter);
            check(popup.popup_count > 0, "every phone layout retains a visible notification");
        }
    }
    u.engine.loaded = false;
    cr_ui_destroy(&u);
    char options_folder[4096];
    snprintf(options_folder, sizeof(options_folder), "%s/options", root);
    check(rmdir(options_folder) == 0, "remove empty core-options directory");
    check(rmdir(root) == 0, "remove menu test storage");
    puts("PASS menu hierarchy, touch feedback, event timing, saved settings and game opacity");
    return 0;
}
