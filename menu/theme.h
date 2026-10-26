// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
#ifndef CORVUS_THEME_H
#define CORVUS_THEME_H
#include <stdint.h>
/* Published IDs stay stable; retired ID 1 falls back to CorvusRetro. */
enum { CR_THEME_SHIELD = 0, CR_THEME_CORVUS = 2, CR_THEME_ASUS = 3, CR_THEME_COUNT = 3 };
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
