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
    CR_ACHIEVEMENTS,
    CR_SAVESTATES,
    CR_SUBSYSTEMS,
    CR_CORE_OPTIONS,
    CR_CORE_OPTION_VALUES,
    CR_SUBSYSTEM_CONFIG,
    CR_SUBSYSTEM_CONTENTS,
    CR_CONTENT,
    CR_LIBRARY
};
/* Stable quick-menu row IDs; unsupported core operations stay disabled. */
enum {
    CR_QUICK_RESUME,
    CR_QUICK_RESTART,
    CR_QUICK_CLOSE,
    CR_QUICK_STATES,
    CR_QUICK_CORE_OPTIONS,
    CR_QUICK_CONTROLS,
    CR_QUICK_ACHIEVEMENTS,
    CR_QUICK_CORE_INFO,
    CR_QUICK_MAIN,
    CR_QUICK_PLAYER,
    CR_QUICK_COUNT
};
enum { CR_TOUCH_DOWN, CR_TOUCH_MOVE, CR_TOUCH_UP, CR_TOUCH_CANCEL };
typedef struct {
    bool active, dragged;
    float x, y, last_x, last_y, held;
    double started, last_event;
    float velocity;
    int target;
    unsigned mask;
} cr_pointer;
typedef struct {
    int page, focus, filter, tab;
    float scroll, quick_scroll;
    bool quick;
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
    unsigned input_player;
    unsigned hardware_keys[2], hardware_pulses[2];
    uint64_t core_revision, import_revision;
    bool import_link, import_folder;
    int link_slot, import_slot;
    char option_key[96], subsystem_id[96], import_subsystem_id[96];
    char link_ids[2][96], link_titles[2][512];
    float width, height, scroll, quick_scroll, scroll_velocity, text_height;
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
void cr_ui_player_key(cr_ui *, unsigned, unsigned, bool);
void cr_ui_release_player(cr_ui *, unsigned);
void cr_ui_inputs(cr_ui *, unsigned[2]);
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
