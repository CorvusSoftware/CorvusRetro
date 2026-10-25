// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
#ifndef CORVUS_THEME_H
#define CORVUS_THEME_H
#include <stdint.h>
/* Published IDs 0/1 must keep their stored meaning. */
enum { CR_THEME_SHIELD, CR_THEME_DARK, CR_THEME_CORVUS, CR_THEME_ASUS, CR_THEME_COUNT };
typedef struct {
    const char *name, *description;
    uint32_t background, header, surface, navigation;
    uint32_t text, muted, accent, icon, divider, focus;
    uint32_t header_text, header_hint, header_icon, navigation_active, navigation_passive;
    uint32_t control, control_text, control_pressed_text, system_bar;
} cr_theme;
int cr_theme_valid(int id);
int cr_theme_at(int index);
const cr_theme *cr_theme_get(int id);
#endif
