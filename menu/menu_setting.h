// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
#ifndef CORVUS_MENU_SETTING_H
#define CORVUS_MENU_SETTING_H
#include "menu_driver.h"
#include <stddef.h>

enum { SET_VOLUME, SET_INTEGER, SET_FILTER, SET_OPACITY, SET_SIZE, SET_POSITION,
       SET_CARDS, SET_PROGRESS, SET_BACKUPS, SET_AUTO_STATE, SET_COUNT };
typedef struct {
    const char *key, *label;
    int minimum, maximum, step, fallback;
    size_t offset;
} cr_menu_setting_spec;

extern const cr_menu_setting_spec cr_menu_settings[SET_COUNT];
int cr_menu_setting_get(const cr_ui *, int id);
void cr_menu_setting_label(int id, int value, char *out, size_t size);
void cr_menu_setting_apply(cr_ui *, int id, int value);
int cr_menu_setting_icon(int id);
void cr_menu_settings_load(cr_ui *);
#endif
