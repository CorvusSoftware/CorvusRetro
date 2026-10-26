// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
#ifndef CORVUSRETRO_CORE_REGISTRY_H
#define CORVUSRETRO_CORE_REGISTRY_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define CR_MAX_CORES 32

/* Stable order: bundled cores first, followed by verified downloaded modules.
 * Updating a module preserves its index and existing engine handles.
 * Invalid/incompatible modules are skipped, never loaded as game content. */
typedef struct {
    char module[4096];
    char id[96];
    char system_id[96];
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
int cr_registry_find(const char *core_id);
/* Called on the frontend thread with immutable, verified private modules or
 * legacy libraries extracted by Android. Relative paths are rejected. */
void cr_registry_add_installed_modules(const char *const *paths, size_t count);
/* Historic GB/GBC/GBA storage suffix, independent of the runtime module index. */
int cr_registry_legacy_system(int index);
/* A stable content storage identifier independent of the package core order. */
bool cr_registry_content_id(char *out, size_t cap, const char *sha256,
                            const char *core_id);
bool cr_registry_accepts(int index, const char *extension);
int cr_registry_choose(const char *extension, int selected);

#endif
