// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
#ifndef CORVUS_FRONTEND_H
#define CORVUS_FRONTEND_H
#include "engine.h"
#include "theme.h"
#include "platform.h"
#include "render.h"
enum {
    CR_MAIN,
    CR_LISTS,
    CR_SETTINGS,
    CR_CORES,
    CR_GAMES,
    CR_ABOUT,
    CR_LICENSE,
    CR_THEMES,
    CR_PLAY
};
enum { CR_TOUCH_DOWN, CR_TOUCH_MOVE, CR_TOUCH_UP, CR_TOUCH_CANCEL };
typedef struct {
    bool active, dragged;
    float x, y, last_y;
    int target;
    unsigned mask;
} cr_pointer;
typedef struct {
    cr_engine engine;
    cr_platform platform;
    cr_game *games;
    size_t game_count;
    int page, return_page, detail_parent, filter, theme, focus;
    bool quick, audio, awake, importing;
    float width, height, scroll, quick_scroll, text_height;
    cr_pointer pointers[32];
    unsigned keys, pulses;
    char title[512], message[512];
    char *license;
    double message_time;
} cr_ui;
void cr_ui_init(cr_ui *, cr_platform, const char *);
void cr_ui_destroy(cr_ui *);
void cr_ui_size(cr_ui *, float, float);
void cr_ui_draw(cr_ui *, const cr_painter *);
void cr_ui_touch(cr_ui *, int, int, float, float);
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
void cr_ui_update(cr_ui *, double);
#endif
