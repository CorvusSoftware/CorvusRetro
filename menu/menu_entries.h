// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
#ifndef CORVUS_MENU_ENTRIES_H
#define CORVUS_MENU_ENTRIES_H
#include "menu_driver.h"
struct cr_state_entry;

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

int cr_menu_entry_count(const cr_ui *);
cr_menu_row cr_menu_entry_get(const cr_ui *, int index);
cr_menu_row cr_menu_entry_visual(const cr_ui *, int index);

/* Read-only selectors shared by list construction, navigation and actions. */
const char *cr_menu_system_name(int index);
bool cr_menu_selected_option(const cr_ui *, corvus_core_option *);
bool cr_menu_selected_subsystem(const cr_ui *, corvus_subsystem_info *);
bool cr_menu_link_game(const cr_ui *, size_t index);
int cr_menu_game_index(const cr_ui *, const char *id);
int cr_menu_library_count(const cr_ui *);
bool cr_menu_selected_state_entry(const cr_ui *, struct cr_state_entry *);
#endif
