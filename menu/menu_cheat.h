// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
#ifndef CORVUS_MENU_CHEAT_H
#define CORVUS_MENU_CHEAT_H
#include "menu_entries.h"

enum {
    ACT_CHEAT_APPLY = 1000, ACT_CHEAT_ADD, ACT_CHEAT_IMPORT, ACT_CHEAT_EXPORT,
    ACT_CHEAT_SETTINGS, ACT_CHEAT_PLAYER, ACT_CHEAT_ENTRY, ACT_CHEAT_ENABLE,
    ACT_CHEAT_NAME, ACT_CHEAT_CODE, ACT_CHEAT_DELETE, ACT_CHEAT_DELETE_OK,
    ACT_CHEAT_CLEAR, ACT_CHEAT_CLEAR_OK, ACT_CHEAT_AUTO_LOAD, ACT_CHEAT_AUTO_TOGGLE,
    ACT_CHEAT_UPDATE_CORE, ACT_CHEAT_BACK
};
bool cr_menu_cheat_page(int page);
int cr_menu_cheat_count(const cr_ui *);
cr_menu_row cr_menu_cheat_entry(const cr_ui *, int index);
bool cr_menu_cheat_activate(cr_ui *, cr_menu_row);
void cr_menu_cheats_loaded(cr_ui *);
void cr_menu_cheats_destroy(cr_ui *);
void cr_menu_cheats_open(cr_ui *);
void cr_menu_cheats_cancel_file(cr_ui *);
bool cr_menu_cheats_cancel_edit(cr_ui *);
bool cr_menu_cheats_text_result(cr_ui *, int field, const char *, bool accepted);
const char *cr_menu_cheats_preview(const cr_ui *);
void cr_ui_cheat_file_result(cr_ui *, const cr_cheat_file_result *);
#endif
