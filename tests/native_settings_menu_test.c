// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
#define _POSIX_C_SOURCE 200809L
#include "menu_driver.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static unsigned checks;
static void check(bool ok, const char *message) {
    if (!ok) { fprintf(stderr, "FAIL settings menu: %s\n", message); exit(1); }
    ++checks;
}
typedef struct { char key[64]; int value; } preference;
typedef struct {
    preference values[48];
    unsigned count, writes;
    bool connected, invalid;
} fixture;
static int preference_get(void *context, const char *key, int fallback) {
    fixture *f = context;
    for (unsigned i = 0; i < f->count; ++i)
        if (!strcmp(f->values[i].key, key)) return f->values[i].value;
    if (f->invalid && (!strcmp(key, "audio_volume") || !strcmp(key, "video_integer") ||
        !strcmp(key, "overlay_opacity") || !strcmp(key, "overlay_size") ||
        !strcmp(key, "overlay_position") || !strcmp(key, "backup_count") ||
        !strcmp(key, "notify_cards") || !strcmp(key, "controller_bind_4"))) return 9999;
    return fallback;
}
static void preference_set(void *context, const char *key, int value) {
    fixture *f = context;
    ++f->writes;
    for (unsigned i = 0; i < f->count; ++i)
        if (!strcmp(f->values[i].key, key)) { f->values[i].value = value; return; }
    check(f->count < 48, "preference recorder capacity");
    snprintf(f->values[f->count].key, sizeof(f->values[f->count].key), "%s", key);
    f->values[f->count++].value = value;
}
static bool controller_info(void *context, unsigned port, char *name, size_t capacity) {
    fixture *f = context;
    if (!f->connected || port >= 2) return false;
    snprintf(name, capacity, "Fixture manette %u", port + 1);
    return true;
}
static cr_platform platform(fixture *f) {
    return (cr_platform){.context = f, .preference = preference_get,
                         .set_preference = preference_set, .controller_info = controller_info};
}
typedef struct { char text[32768]; unsigned toggles; } capture;
static void rect(void *ctx, float x, float y, float w, float h, uint32_t color) {
    (void)ctx; (void)x; (void)y; (void)w; (void)h; (void)color;
}
static void circle(void *ctx, float x, float y, float radius, uint32_t color) {
    (void)x; (void)y; (void)color;
    if (radius == 10) ++((capture *)ctx)->toggles;
}
static void text(void *ctx, const char *s, float x, float y, float size, uint32_t color, float max) {
    (void)x; (void)y; (void)size; (void)color; (void)max;
    capture *c = ctx; size_t n = strlen(c->text);
    snprintf(c->text + n, sizeof(c->text) - n, "%s\n", s);
}
static float measure(void *ctx, const char *s, float size) { (void)ctx; return strlen(s) * size / 2; }
static void icon(void *ctx, int kind, float x, float y, float size, uint32_t color) {
    (void)ctx; (void)kind; (void)x; (void)y; (void)size; (void)color;
}
static capture draw(cr_ui *u) {
    capture c = {0};
    cr_painter p = {.context = &c, .rect = rect, .circle = circle,
                    .text = text, .measure = measure, .icon = icon};
    cr_ui_draw(u, &p);
    return c;
}
static void choose(cr_ui *u, int index) {
    u->focus = u->quick ? cr_ui_quick_row(u, index) : index; cr_ui_move(u, 0); cr_ui_confirm(u);
}
static void settings_root(cr_ui *u) {
    float x = u->width * 5 / 6, y = u->height - 30;
    cr_ui_touch_at(u, 0, CR_TOUCH_DOWN, x, y, 100);
    cr_ui_touch_at(u, 0, CR_TOUCH_UP, x, y, 100.05);
    check(u->page == CR_SETTINGS && u->tab == 4 && !u->quick, "settings tab opens the global root");
}
static void category(cr_ui *u, int row, int page) {
    settings_root(u); choose(u, row);
    check(u->page == page && u->tab == 4 && u->depth == 1, "category opens from global settings");
}
static void choose_value(cr_ui *u, int row, int value_index, int return_page) {
    choose(u, row);
    check(u->page == CR_SETTING_VALUES && u->depth >= 2, "real setting opens its selectable values");
    choose(u, value_index);
    check(u->page == return_page && u->focus == row, "choosing a value restores its parent selection");
}
static void defaults(cr_ui *u, fixture *f) {
    check(u->volume == 100 && !u->video_integer && !u->video_smooth &&
          u->overlay_opacity == 100 && u->overlay_size == 100 && !u->overlay_position,
          "media defaults preserve the existing presentation and volume");
    check(u->notify_cards == 1 && u->notify_progress == 1 && u->backup_count == 1 && !u->auto_state,
          "new notification and persistence preferences have compatible defaults");
    check(u->audio && u->awake && u->show_input && u->controller_capture == -1,
          "existing boolean defaults and inactive binding capture are preserved");
    for (unsigned i = 0; i < 10; ++i)
        check(u->controller_bindings[i] == cr_controller_binding_default(i), "controller defaults remain complete and unique");
    check(u->engine.backup_count == 1 && !u->engine.auto_state, "loaded preference policy reaches the engine");
    check(!f->writes, "reading defaults never rewrites existing preferences");
}
static void categories(cr_ui *u) {
    settings_root(u);
    capture c = draw(u);
    const char *labels[] = {"Interface utilisateur", "Vidéo", "Audio", "Commandes", "Sauvegardes", "RetroAchievements"};
    const char *previous = c.text;
    for (unsigned i = 0; i < 6; ++i) {
        const char *found = strstr(previous, labels[i]);
        check(found != NULL, "all six categories appear in the intended order");
        previous = found + strlen(labels[i]);
    }
    check(!strstr(c.text, "Affichage à l’écran"), "obsolete general OSD category is absent");
    const int pages[] = {CR_UI_SETTINGS, CR_VIDEO_SETTINGS, CR_AUDIO_SETTINGS, CR_OSD_SETTINGS, CR_SAVE_SETTINGS, CR_RA_SETTINGS};
    for (int i = 0; i < 6; ++i) {
        choose(u, i); check(u->page == pages[i], "each root category opens its working page");
        cr_ui_back(u); check(u->page == CR_SETTINGS && u->focus == i, "category back preserves selection");
    }
    cr_ui_move(u, 1); check(u->focus == 0, "category navigation wraps after exactly six entries");
}
static void media_preferences(cr_ui *u, fixture *f) {
    category(u, 1, CR_VIDEO_SETTINGS);
    choose_value(u, 1, 1, CR_VIDEO_SETTINGS);
    check(u->video_integer == 1 && preference_get(f, "video_integer", -1) == 1, "integer scaling preference is applied and saved");
    choose_value(u, 2, 1, CR_VIDEO_SETTINGS);
    check(u->video_smooth == 1 && preference_get(f, "video_smooth", -1) == 1, "filter selection is applied and saved");
    category(u, 2, CR_AUDIO_SETTINGS);
    choose_value(u, 1, 3, CR_AUDIO_SETTINGS);
    check(u->volume == 30 && preference_get(f, "audio_volume", -1) == 30 && u->audio,
          "frontend volume is saved independently of the sound-enabled switch");
    choose(u, 0);
    check(!u->audio && preference_get(f, "audio", -1) == 0 && u->volume == 30,
          "existing audio preference key and configured gain remain independent");
    category(u, 3, CR_OSD_SETTINGS); choose(u, 0);
    check(u->page == CR_OVERLAY_SETTINGS && u->depth == 2, "touch controls have one useful subcategory");
    choose_value(u, 1, 2, CR_OVERLAY_SETTINGS);
    check(u->overlay_opacity == 30 && preference_get(f, "overlay_opacity", -1) == 30, "idle opacity is real and persistent");
    choose(u, 2); check(u->page == CR_OVERLAY_LAYOUT, "layout has size, position and reset actions");
    choose_value(u, 0, 3, CR_OVERLAY_LAYOUT);
    choose_value(u, 1, 0, CR_OVERLAY_LAYOUT);
    check(u->overlay_size == 130 && u->overlay_position == -15 &&
          preference_get(f, "overlay_size", -1) == 130 && preference_get(f, "overlay_position", 0) == -15,
          "touch size and vertical position persist separately");
    choose(u, 2);
    check(u->overlay_size == 100 && !u->overlay_position && u->overlay_opacity == 30 &&
          preference_get(f, "overlay_size", -1) == 100 && preference_get(f, "overlay_position", -1) == 0,
          "layout reset restores geometry without erasing opacity or visibility");
    cr_ui_back(u); cr_ui_back(u); cr_ui_back(u);
    check(u->page == CR_SETTINGS && u->focus == 3 && !u->depth, "three-level back restores the Commands category");
}
static void notification_preferences(cr_ui *u, fixture *f) {
    category(u, 0, CR_UI_SETTINGS); choose(u, 1);
    check(u->page == CR_NOTIFICATIONS_SETTINGS, "notification controls are separate from themes");
    cr_notifications_push(&u->notifications, CR_NOTIFICATION_GAME, 0, "Original fixture", "Game", "");
    cr_notifications_progress(&u->notifications, 1, "1/10", "badge", 10);
    choose(u, 0);
    check(!u->notify_cards && !u->notifications.count && u->notifications.progress.visible &&
          preference_get(f, "notify_cards", -1) == 0,
          "hiding cards immediately drops queued cards while retaining progress");
    choose(u, 1);
    check(!u->notify_progress && !u->notifications.progress.visible &&
          preference_get(f, "notify_progress", -1) == 0,
          "hiding progress clears only its indicator and persists the setting");
    cr_ui_message(u, "Erreur de sauvegarde de test");
    capture c = draw(u);
    check(strstr(c.text, "Erreur de sauvegarde de test") != NULL, "disabled success notices never suppress application errors");
    choose(u, 0); choose(u, 1);
    check(u->notify_cards && u->notify_progress, "both optional notification channels can be restored");
}
static void save_and_account_preferences(cr_ui *u, fixture *f) {
    category(u, 4, CR_SAVE_SETTINGS);
    choose_value(u, 0, 2, CR_SAVE_SETTINGS);
    choose(u, 1);
    check(u->backup_count == 3 && u->auto_state && u->engine.backup_count == 3 && u->engine.auto_state &&
          preference_get(f, "backup_count", -1) == 3 && preference_get(f, "auto_state", -1) == 1,
          "bounded backup and automatic-state preferences reach the live engine policy");
    category(u, 5, CR_RA_SETTINGS);
    capture c = draw(u);
    check(strstr(c.text, "Activer RetroAchievements") && strstr(c.text, "Compte") &&
          !strstr(c.text, "Aucun jeu identifié"), "global achievements settings contain activation and account only");
    choose(u, 1); check(u->page == CR_RA_ACCOUNT, "account has a real global subpage without a loaded core");
    c = draw(u);
    check(strstr(c.text, "Identifiant") && strstr(c.text, "Mot de passe") &&
          strstr(c.text, "Se connecter") && !strstr(c.text, "Aucun jeu identifié"),
          "account fields are distinct from the current-game achievement list");
    cr_ui_back(u);
    check(u->page == CR_RA_SETTINGS && u->focus == 1, "account back restores the account row");
    settings_root(u);
    u->page = CR_PLAY; u->quick = true; u->focus = cr_ui_quick_row(u, CR_QUICK_ACHIEVEMENTS);
    cr_ui_confirm(u);
    c = draw(u);
    check(u->page == CR_ACHIEVEMENTS && strstr(c.text, "Aucun jeu identifié") &&
          !strstr(c.text, "Mot de passe") && !strstr(c.text, "Identifiant") &&
          !strstr(c.text, "Activer RetroAchievements"), "quick achievements contains game information, never account editing");
    cr_ui_back(u);
    check(u->page == CR_PLAY && u->quick && u->focus == cr_ui_quick_row(u, CR_QUICK_ACHIEVEMENTS),
          "game achievements return restores the quick-menu context");
}
static void controllers(cr_ui *u, fixture *f) {
    category(u, 3, CR_OSD_SETTINGS); choose(u, 1);
    check(u->page == CR_CONTROLLER_SETTINGS, "controllers page opens independently of touch settings");
    capture c = draw(u);
    check(strstr(c.text, "Aucune manette connectée") && !strstr(c.text, "Fixture manette"),
          "disconnected devices never display invented names");
    choose(u, 2); choose(u, 4);
    check(u->controller_capture == -1 && strstr(u->message, "Connectez une manette"),
          "binding cannot begin without an actual platform device");
    f->connected = true; cr_ui_back(u);
    c = draw(u);
    check(strstr(c.text, "Fixture manette 1") && strstr(c.text, "Fixture manette 2"),
          "controller names are obtained from the two real platform ports");
    choose(u, 2); choose(u, 4);
    check(u->controller_capture == 4, "A button enters explicit capture mode");
    unsigned before = f->writes;
    check(cr_ui_controller_key(u, cr_controller_binding_default(5)) && u->controller_capture == 4 &&
          u->controller_bindings[4] == cr_controller_binding_default(4) && f->writes == before,
          "duplicate binding is rejected without erasing another button or persisting it");
    check(cr_ui_controller_key(u, 4) && u->controller_capture == 4 && f->writes == before,
          "reserved Android Back cannot become an emulator button");
    check(cr_ui_controller_key(u, 188) && u->controller_capture == -1 &&
          u->controller_bindings[4] == 188 && preference_get(f, "controller_bind_4", -1) == 188,
          "accepted physical button is assigned and persisted to its logical action");
    check(!cr_ui_controller_key(u, 189), "ordinary keys outside capture never rewrite bindings");
    choose(u, 5); cr_ui_back(u);
    check(u->page == CR_CONTROLLER_BINDINGS && u->controller_capture == -1 &&
          u->controller_bindings[5] == cr_controller_binding_default(5), "Back cancels capture without leaving its page");
    choose(u, 10);
    for (unsigned i = 0; i < 10; ++i)
        check(u->controller_bindings[i] == cr_controller_binding_default(i), "restoring bindings covers every game button");
    check(preference_get(f, "controller_bind_4", -1) == cr_controller_binding_default(4),
          "restored mapping is persisted, not merely redrawn");
}
static void contexts(cr_ui *u) {
    cr_ui_size(u, 700, 384);
    settings_root(u); choose(u, 5);
    check(u->page == CR_RA_SETTINGS, "landscape scrolling can reach the last category");
    cr_ui_back(u);
    check(u->page == CR_SETTINGS && u->focus == 5 && u->scroll > 0,
          "landscape return restores selected category and nonzero scroll");
    cr_ui_size(u, 384, 700);
    settings_root(u);
    u->page = CR_PLAY; u->quick = true; u->focus = cr_ui_quick_row(u, CR_QUICK_CONTROLS);
    cr_ui_move(u, 0);
    float scroll = u->quick_scroll; int tab = u->tab;
    cr_ui_confirm(u);
    check(u->page == CR_OVERLAY_SETTINGS && !u->quick, "quick controls use the same real preference page");
    choose(u, 2); choose_value(u, 0, 1, CR_OVERLAY_LAYOUT);
    check(u->overlay_size == 110, "quick-context layout adjustment changes the shared setting");
    cr_ui_back(u); cr_ui_back(u);
    check(u->page == CR_PLAY && u->quick && u->focus == cr_ui_quick_row(u, CR_QUICK_CONTROLS) &&
          fabsf(u->quick_scroll - scroll) < .001f && u->tab == tab,
          "nested layout adjustment restores the original quick row, scroll and tab");
}
int main(void) {
    char root[] = "/tmp/corvus-settings-menu-XXXXXX";
    check(mkdtemp(root) != NULL, "private settings test storage");
    fixture f = {0}; cr_ui u;
    cr_ui_init(&u, platform(&f), root); defaults(&u, &f);
    categories(&u); media_preferences(&u, &f); notification_preferences(&u, &f);
    save_and_account_preferences(&u, &f); controllers(&u, &f); contexts(&u);
    cr_ui_destroy(&u);
    cr_ui_init(&u, platform(&f), root);
    check(u.volume == 30 && !u.audio && u.video_integer && u.video_smooth &&
          u.overlay_opacity == 30 && u.overlay_size == 110 && !u.overlay_position &&
          u.backup_count == 3 && u.auto_state && u.engine.backup_count == 3 && u.engine.auto_state,
          "recreated frontend restores new and historical preferences without a core");
    cr_ui_destroy(&u);
    fixture invalid = {.invalid = true}; cr_ui_init(&u, platform(&invalid), root); defaults(&u, &invalid);
    cr_ui_destroy(&u);
    check(rmdir(root) == 0, "settings tests create no game or engine state files");
    printf("PASS %u settings hierarchy, persistent values, capture and navigation checks\n", checks);
    return 0;
}
