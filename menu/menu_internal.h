// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
#ifndef CORVUS_MENU_INTERNAL_H
#define CORVUS_MENU_INTERNAL_H
#include "menu_driver.h"

/* Internal action-to-driver bridge, not part of the platform-facing API. */
void cr_menu_page(cr_ui *, int page);
void cr_menu_push_page(cr_ui *, int page);
bool cr_menu_pop_page(cr_ui *);
void cr_menu_achievement_sync(cr_ui *, bool identify);
#endif
