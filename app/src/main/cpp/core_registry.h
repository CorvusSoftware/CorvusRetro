// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
#ifndef CORVUSRETRO_CORE_REGISTRY_H
#define CORVUSRETRO_CORE_REGISTRY_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define CR_MAX_CORES 32

/* Stable order: bundled official cores first, followed by configured cores.
 * Only native libraries shipped in the application are considered.
 * Invalid/incompatible modules are skipped, never loaded as game content. */
typedef struct {
    char module[128];
    char id[96];
    char name[128];
    char version[64];
    char system_name[128];
    char extensions[160];
    uint32_t max_players;
    bool state_support;
    bool battery_support;
} cr_core_descriptor;

size_t cr_registry_count(void);
const cr_core_descriptor *cr_registry_get(int index);
bool cr_registry_accepts(int index, const char *extension);
int cr_registry_choose(const char *extension, int selected);

#endif
