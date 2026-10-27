// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
#ifndef CORVUS_FRONTEND_H
#define CORVUS_FRONTEND_H
#include "engine.h"
#include "platform.h"
#include "render.h"
#include "theme.h"
#include "cheevos.h"
#include "notifications.h"
enum {
    CR_MAIN,
    CR_LISTS,
    CR_SETTINGS,
    CR_CORES,
    CR_GAMES,
    CR_ABOUT,
    CR_LICENSE,
    CR_THEMES,
    CR_PLAY,
    CR_UI_SETTINGS,
    CR_VIDEO_SETTINGS,
    CR_AUDIO_SETTINGS,
    CR_OSD_SETTINGS,
    CR_OVERLAY_SETTINGS,
    CR_INFORMATION,
    CR_CORE_INFO,
    CR_CORE_DOWNLOADS,
    CR_ACHIEVEMENTS
};
enum { CR_TOUCH_DOWN, CR_TOUCH_MOVE, CR_TOUCH_UP, CR_TOUCH_CANCEL };
typedef struct {
    bool active, dragged;
    float x, y, last_x, last_y, held;
    double started;
    int target;
    unsigned mask;
} cr_pointer;
typedef struct {
    int page, focus, filter;
    float scroll;
} cr_navigation;
typedef struct {
    cr_engine engine;
    cr_platform platform;
    cr_cheevos *achievements;
    cr_notifications notifications;
    bool notifications_loaded;
    bool ra_enabled;
    int ra_editing;
    void *ra_handle;
    char ra_username[96], ra_password[513], ra_preview[513], ra_game_id[96];
    cr_game *games;
    size_t game_count;
    cr_core_catalog core_catalog;
    bool catalog_loading;
    int page, filter, theme, focus, tab, menu_pointer, feedback_target;
    bool quick, audio, awake, importing, show_input, installing;
    float width, height, scroll, quick_scroll, text_height;
    float feedback_alpha, control_feedback[10];
    cr_navigation history[8];
    unsigned depth;
    cr_pointer pointers[32];
    unsigned keys, pulses, feedback_pulses;
    char title[512], message[512];
    char *license;
    double message_time, time;
} cr_ui;
void cr_ui_init(cr_ui *, cr_platform, const char *);
void cr_ui_destroy(cr_ui *);
void cr_ui_size(cr_ui *, float, float);
void cr_ui_draw(cr_ui *, const cr_painter *);
void cr_ui_touch(cr_ui *, int, int, float, float);
/* Platform event time keeps tap/hold classification independent of rendering speed. */
void cr_ui_touch_at(cr_ui *, int, int, float, float, double);
void cr_ui_back(cr_ui *);
void cr_ui_confirm(cr_ui *);
void cr_ui_move(cr_ui *, int);
void cr_ui_key(cr_ui *, unsigned, bool);
void cr_ui_clear_input(cr_ui *);
unsigned cr_ui_input(cr_ui *);
bool cr_ui_running(const cr_ui *);
int cr_content_system(const char *, int);
void cr_ui_import(cr_ui *, const cr_import *);
void cr_ui_message(cr_ui *, const char *);
void cr_ui_reload(cr_ui *);
void cr_ui_catalog(cr_ui *, const cr_core_catalog *);
/* Installer events cross the Android mailbox; only terminal events release the barrier. */
void cr_ui_install_status(cr_ui *, const char *, bool finished);
void cr_ui_text_result(cr_ui *, int, const char *, bool);
void cr_ui_achievement_frame(cr_ui *);
void cr_ui_update(cr_ui *, double);
#endif
