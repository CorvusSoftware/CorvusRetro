// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
#ifndef CORVUS_INTERFACE_DRAW_H
#define CORVUS_INTERFACE_DRAW_H
#include "menu_entries.h"

void cr_interface_draw(cr_ui *, const cr_painter *, const char *title, bool back,
                       int count, cr_menu_row_reader);
#endif
