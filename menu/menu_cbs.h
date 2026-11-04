// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
#ifndef CORVUS_MENU_CBS_H
#define CORVUS_MENU_CBS_H
#include "menu_entries.h"

enum {
    ACT_NONE,
    ACT_CORE_REMOVE_CANCEL,
    ACT_CORE_REMOVE_CONFIRM,
    ACT_CORE_REMOVE,
    ACT_CORES,
    ACT_UNLOAD,
    ACT_PICK,
    ACT_HISTORY,
    ACT_ABOUT,
    ACT_GAME,
    ACT_THEME,
    ACT_AUDIO,
    ACT_AWAKE,
    ACT_LICENSE,
    ACT_SELECT,
    ACT_SET_THEME,
    ACT_QUICK,
    ACT_PAGE,
    ACT_SHOW_INPUT,
    ACT_OPEN_DOWNLOADER,
    ACT_REFRESH_CATALOG,
    ACT_INSTALL_CORE_PACK,
    ACT_RA_ENABLE,
    ACT_RA_USER,
    ACT_RA_PASSWORD,
    ACT_RA_CONNECT,
    ACT_QUICK_ACTION,
    ACT_SAVE_STATE,
    ACT_LOAD_STATE,
    ACT_CORE_OPTION,
    ACT_OPTION_VALUE,
    ACT_SUBSYSTEM,
    ACT_SUBSYSTEM_SLOT,
    ACT_SUBSYSTEM_GAME,
    ACT_SUBSYSTEM_IMPORT,
    ACT_SUBSYSTEM_START,
    ACT_LIBRARY,
    ACT_FOLDER,
    ACT_SAVE_TRANSFER,
    ACT_SAVE_PLAYER,
    ACT_SAVE_COMMIT,
    ACT_SAVE_CANCEL,
    ACT_SETTING_VALUES,
    ACT_SETTING_VALUE,
    ACT_SETTING_TOGGLE,
    ACT_LAYOUT_RESET,
    ACT_BIND_CAPTURE,
    ACT_BIND_RESET,
    ACT_LOAD_AUTO_STATE,
    ACT_BACKUP_MENU,
    ACT_EXPORT_BACKUP,
    ACT_CORE_DETAILS,
    ACT_STATE_GAME,
    ACT_STATE_CREATE,
    ACT_STATE_ACTIONS,
    ACT_STATE_LOAD,
    ACT_STATE_RENAME,
    ACT_STATE_DUPLICATE,
    ACT_STATE_SHARE,
    ACT_STATE_DELETE_CONFIRM,
    ACT_STATE_DELETE
};

/* Frontend-thread actions; navigation and engine ownership stay with cr_ui. */
void cr_menu_activate(cr_ui *, cr_menu_row);
void cr_menu_quick_action(cr_ui *, int action);
void cr_menu_finish_save_transfer(cr_ui *);
void cr_menu_refresh_saved_states(cr_ui *);
void cr_menu_open_core_details(cr_ui *, int index);
#endif
