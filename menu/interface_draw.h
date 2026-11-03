// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
#ifndef CORVUS_INTERFACE_DRAW_H
#define CORVUS_INTERFACE_DRAW_H
#include "menu_driver.h"

typedef struct cr_menu_row {
    char label[512], hint[256];
    int icon, action, value;
    bool toggle, checked, disabled;
    char image[256];
    int console;
    bool destructive;
} cr_menu_row;
/* Index -1 supplies optional page metadata (core, game or selected state). */
typedef cr_menu_row (*cr_menu_row_reader)(const cr_ui *, int);
void cr_interface_draw(cr_ui *, const cr_painter *, const char *title, bool back,
                       int count, cr_menu_row_reader);
#endif
