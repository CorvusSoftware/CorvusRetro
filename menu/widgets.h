// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
#ifndef CORVUS_MENU_WIDGETS_H
#define CORVUS_MENU_WIDGETS_H
#include "interface_layout.h"

uint32_t cr_widget_mix(uint32_t, uint32_t, float);
void cr_widget_round(const cr_painter *, cr_menu_rect, float radius, uint32_t);
void cr_widget_gradient(const cr_painter *, cr_menu_rect, float radius, uint32_t top, uint32_t bottom);
void cr_widget_card(const cr_painter *, cr_menu_rect, const cr_theme *, float active);
void cr_widget_console(const cr_painter *, cr_menu_rect, int console, const cr_theme *);
void cr_widget_center(const cr_painter *, const char *, float x, float baseline, float size, uint32_t, float max_width);
void cr_widget_line(const cr_painter *, float, float, float, float, float, uint32_t);
#endif
