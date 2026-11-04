// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
#ifndef CORVUS_RENDER_H
#define CORVUS_RENDER_H
#include "platform.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
enum {
    CR_ICON_CORE,
    CR_ICON_CROW,
    CR_ICON_CONTENT,
    CR_ICON_LIST,
    CR_ICON_SETTINGS,
    CR_ICON_HISTORY,
    CR_ICON_INFO,
    CR_ICON_UNLOAD,
    CR_ICON_THEME,
    CR_ICON_MENU,
    CR_ICON_NEXT,
    CR_ICON_BACK,
    CR_ICON_FILE,
    CR_ICON_HOME,
    CR_ICON_VIDEO,
    CR_ICON_AUDIO,
    CR_ICON_INPUT,
    CR_ICON_SAVES,
    CR_ICON_ACHIEVEMENTS,
    CR_ICON_DOWNLOAD,
    CR_ICON_RESTART,
    CR_ICON_PLAY,
    CR_ICON_CLOSE,
    CR_ICON_NOTIFICATIONS,
    CR_ICON_ACCOUNT,
    CR_ICON_SUBSYSTEM,
    CR_ICON_FOLDER,
    CR_ICON_DISPLAY,
    CR_ICON_TOUCH,
    CR_ICON_VOLUME,
    CR_ICON_BACKUP,
    CR_ICON_RESTORE,
    CR_ICON_GRID,
    CR_ICON_SEARCH,
    CR_ICON_RENAME,
    CR_ICON_DUPLICATE,
    CR_ICON_SHARE,
    CR_ICON_TRASH,
    CR_ICON_MORE,
    CR_ICON_CHECK,
    CR_ICON_EXIT,
    CR_ICON_ADD,
    CR_ICON_SYSTEM,
    CR_ICON_IMPORT,
    CR_ICON_EXPORT,
    CR_ICON_LOCK,
    CR_ICON_SLIDERS,
    CR_ICON_FILTER,
    CR_ICON_CHEATS,
    CR_ICON_COUNT
};
/* Mirrored by MainActivity: text glyphs must never overwrite the icon block. */
enum { CR_ICON_ATLAS_BASE = 463, CR_ICON_ATLAS_CAPACITY = 49 };
typedef struct {
    void *context;
    void (*rect)(void *, float, float, float, float, uint32_t);
    void (*circle)(void *, float, float, float, uint32_t);
    void (*text)(void *, const char *, float, float, float, uint32_t, float);
    float (*measure)(void *, const char *, float);
    void (*icon)(void *, int, float, float, float, uint32_t);
    void (*clip)(void *, float, float, float, float);
    void (*game)(void *, const uint32_t *, int, int, float, float, float, float);
    bool (*image)(void *, const char *, float, float, float, float, float);
    bool (*image_ready)(void *, const char *);
    void (*line)(void *, float, float, float, float, float, uint32_t);
} cr_painter;
#endif
