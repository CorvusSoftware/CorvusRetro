// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
#ifndef CORVUS_CHEEVOS_H
#define CORVUS_CHEEVOS_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct cr_cheevos cr_cheevos;
typedef size_t (*cr_cheevos_read_memory)(void *, uint32_t, uint8_t *, size_t);
typedef struct {
    void *context;
    /* Must copy request strings before returning; completion is delivered on the owner thread. */
    void (*request)(void *, uint64_t, const char *, const char *);
    void (*save_credentials)(void *, const char *, const char *);
    void (*notice)(void *, const char *);
} cr_cheevos_host;
typedef struct {
    bool connected, pending, game_loaded, memory_supported;
    char username[96], game_title[192], message[256];
    uint32_t unlocked, total, points;
} cr_cheevos_status;
typedef struct {
    uint32_t id, points;
    bool unlocked;
    char title[160], description[256];
} cr_cheevos_achievement;

/* All methods and callbacks belong exclusively to the frontend thread. */
cr_cheevos *cr_cheevos_create(cr_cheevos_host);
void cr_cheevos_destroy(cr_cheevos *);
bool cr_cheevos_login(cr_cheevos *, const char *, const char *);
bool cr_cheevos_login_token(cr_cheevos *, const char *, const char *);
void cr_cheevos_logout(cr_cheevos *);
/* Hashes the ROM once during content loading. No file I/O occurs during frame/idle. */
bool cr_cheevos_load_game(cr_cheevos *, const char *, uint32_t, cr_cheevos_read_memory, void *);
void cr_cheevos_unload_game(cr_cheevos *);
void cr_cheevos_frame(cr_cheevos *);
void cr_cheevos_idle(cr_cheevos *);
void cr_cheevos_reset(cr_cheevos *);
/* Missing/mismatched progress resets evaluation instead of keeping future trigger state. */
size_t cr_cheevos_progress_size(cr_cheevos *);
bool cr_cheevos_progress_save(cr_cheevos *, void *, size_t);
bool cr_cheevos_progress_restore(cr_cheevos *, const void *, size_t);
/* Unknown/stale IDs are ignored, including completions belonging to an older client. */
void cr_cheevos_response(cr_cheevos *, uint64_t, int, const char *, size_t);
void cr_cheevos_get_status(cr_cheevos *, cr_cheevos_status *);
size_t cr_cheevos_achievement_count(cr_cheevos *);
bool cr_cheevos_get_achievement(cr_cheevos *, size_t, cr_cheevos_achievement *);
#endif
