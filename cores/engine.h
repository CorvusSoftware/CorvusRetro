// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
#ifndef CORVUSRETRO_ENGINE_H
#define CORVUSRETRO_ENGINE_H

#include <corvusapi.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define CR_CORE_VERSION "0.14.1"

typedef struct cr_engine {
    void *handle;
    void *module;
    struct cr_engine_output *output;
    corvus_core_api api;
    corvus_achievements_api achievements;
    corvus_options_api options;
    corvus_subsystem_api subsystems;
    corvus_cheats_api cheats_api;
    struct cr_engine_options *saved_options;
    struct cr_engine_cheats *active_cheats;
    struct cr_battery_worker *battery_worker;
    unsigned players;
    int system;
    bool loaded;
    unsigned backup_count;
    bool auto_state;
    int width, height, rate;
    double fps;
    uint32_t *pixels;
    int16_t audio[4096];
    size_t audio_count;
    char id[96], second_id[96], subsystem_id[96], root[4096], module_path[4096], error[256];
    char content_paths[2][4096];
} cr_engine;

/* All operations for an engine run on its owning frontend thread. */
void cr_engine_init(cr_engine *engine, const char *root);
void cr_engine_set_save_policy(cr_engine *engine, unsigned backup_count, bool auto_state);
bool cr_game_id_valid(const char *id);
bool cr_engine_select(cr_engine *engine, int system);
bool cr_engine_core_info(const cr_engine *engine, corvus_core_info *out);
bool cr_engine_open(cr_engine *engine, int system, const char *id, const char *path);
bool cr_engine_open_subsystem(cr_engine *engine, int system, const char *subsystem_id,
                              const char *ids[2], const char *paths[2]);
size_t cr_engine_option_count(const cr_engine *engine);
bool cr_engine_option(const cr_engine *engine, size_t index, corvus_core_option *out);
uint32_t cr_engine_option_value(const cr_engine *engine, const char *key);
bool cr_engine_set_option(cr_engine *engine, const char *key, uint32_t value);
/* Owner-thread operations on a loaded player; replacement copies all code strings. */
uint32_t cr_engine_cheat_capabilities(const cr_engine *engine, unsigned player);
bool cr_engine_validate_cheat(cr_engine *engine, unsigned player, const char *code);
bool cr_engine_replace_cheats(cr_engine *engine, unsigned player,
                              const char *const *codes, size_t count);
size_t cr_engine_cheat_count(const cr_engine *engine, unsigned player);
size_t cr_engine_subsystem_count(const cr_engine *engine);
bool cr_engine_subsystem(const cr_engine *engine, size_t index, corvus_subsystem_info *out);
bool cr_engine_can_reset(const cr_engine *engine);
bool cr_engine_can_state(const cr_engine *engine);
bool cr_engine_check(cr_engine *engine, int system, const char *path);
size_t cr_engine_read_achievement_memory(void *, uint32_t, uint8_t *, size_t);
bool cr_engine_step(cr_engine *engine, unsigned mask);
bool cr_engine_step_players(cr_engine *engine, const unsigned masks[2]);
bool cr_engine_reset(cr_engine *engine);
bool cr_engine_save_state(cr_engine *engine);
bool cr_engine_load_state(cr_engine *engine);
/* Owner-thread raw SDK snapshots. Caller frees copied bytes; restore drains pending battery IO. */
bool cr_engine_state_copy(cr_engine *engine, uint8_t **bytes, size_t *size);
bool cr_engine_state_restore(cr_engine *engine, const uint8_t *bytes, size_t size);
bool cr_engine_save_auto_state(cr_engine *engine);
bool cr_engine_load_auto_state(cr_engine *engine);
/* Filesystem query for opening the state menu, never the draw loop. */
bool cr_engine_has_auto_state(cr_engine *engine);
bool cr_engine_save_battery(cr_engine *engine);
/* Owner-thread calls. Request/poll never wait for storage or acknowledge early. */
bool cr_engine_request_battery_save(cr_engine *engine);
bool cr_engine_poll_battery_save(cr_engine *engine);
/* Fresh owner-thread snapshots; caller owns the bytes. No dirty acknowledgement. */
size_t cr_engine_battery_size(const cr_engine *engine, unsigned player);
bool cr_engine_export_battery(cr_engine *engine, unsigned player, uint8_t **bytes, size_t *size);
bool cr_engine_export_backup(cr_engine *engine, unsigned player, unsigned index,
                             uint8_t **bytes, size_t *size);
/* Validates a fresh candidate before committing battery files and replacing the session. */
bool cr_engine_import_battery(cr_engine *engine, unsigned player, const uint8_t *bytes, size_t size);
/* Close only the content; the selected core and its callback context stay loaded. */
bool cr_engine_unload_game(cr_engine *engine);
/* Failed persistence leaves the current engine alive so the caller can retry. */
bool cr_engine_close(cr_engine *engine);
/* Shutdown attempts persistence, then always releases native resources. */
void cr_engine_destroy(cr_engine *engine);

#endif
