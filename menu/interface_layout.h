// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
#ifndef CORVUS_INTERFACE_LAYOUT_H
#define CORVUS_INTERFACE_LAYOUT_H
#include "menu_driver.h"

typedef struct { float x, y, w, h; } cr_menu_rect;
float cr_interface_bottom(const cr_ui *);
float cr_interface_content_top(const cr_ui *);
float cr_interface_content_height(const cr_ui *, int count);
cr_menu_rect cr_interface_row_bounds(const cr_ui *, int row);
int cr_interface_target(const cr_ui *, float x, float y, int count, bool back);
/* Shared geometry for overlays and top-level native controls. */
cr_menu_rect cr_interface_panel(const cr_ui *);
cr_menu_rect cr_interface_search(const cr_ui *);
cr_menu_rect cr_interface_chip(const cr_ui *, int chip);
cr_menu_rect cr_interface_navigation(const cr_ui *, int tab);
#endif
