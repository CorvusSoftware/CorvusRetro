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
    CR_ICON_FILE
};
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
} cr_painter;
#endif
