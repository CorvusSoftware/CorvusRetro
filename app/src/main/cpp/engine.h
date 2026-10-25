// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
#ifndef CORVUSRETRO_ENGINE_H
#define CORVUSRETRO_ENGINE_H

#include <corvuscore/bridge.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define CR_CORE_VERSION "0.14.0"

typedef struct cr_engine {
    corvuscore_bridge *handle;
    int system;
    bool loaded;
    int width, height, rate;
    double fps;
    uint32_t *pixels;
    int16_t audio[4096];
    size_t audio_count;
    char id[68], root[4096], error[256];
} cr_engine;

/* All operations for an engine run on its owning frontend thread. */
void cr_engine_init(cr_engine *engine, const char *root);
bool cr_game_id_valid(const char *id);
bool cr_engine_select(cr_engine *engine, int system);
bool cr_engine_open(cr_engine *engine, int system, const char *id, const char *path);
bool cr_engine_check(cr_engine *engine, int system, const char *path);
bool cr_engine_step(cr_engine *engine, unsigned mask);
bool cr_engine_reset(cr_engine *engine);
bool cr_engine_save_state(cr_engine *engine);
bool cr_engine_load_state(cr_engine *engine);
bool cr_engine_save_battery(cr_engine *engine);
/* Failed persistence leaves the current engine alive so the caller can retry. */
bool cr_engine_close(cr_engine *engine);
/* Shutdown attempts persistence, then always releases native resources. */
void cr_engine_destroy(cr_engine *engine);

#endif
