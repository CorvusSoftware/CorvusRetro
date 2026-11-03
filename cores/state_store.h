// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
#ifndef CORVUS_STATE_STORE_H
#define CORVUS_STATE_STORE_H
#include "engine.h"

#define CR_STATE_STORE_LIMIT 32u
#define CR_STATE_STORE_MAX_BYTES (16u * 1024u * 1024u)

typedef struct cr_state_entry {
    unsigned slot, players;
    int64_t created_ms;
    uint64_t revision, size;
    char game_id[96], second_id[96], subsystem_id[96];
    char core_id[96], core_version[64], name[96];
    char preview_key[256];
} cr_state_entry;

/* Menu-action IO only. An empty successful list leaves error empty. Entries are newest first. */
size_t cr_state_store_list(const char *root, const char *game_id,
                          cr_state_entry *entries, size_t capacity, char *error, size_t error_size);
/* Core calls stay on the owning frontend thread; errors use engine.error. */
bool cr_state_store_create(cr_engine *, const char *name, cr_state_entry *created);
bool cr_state_store_load(cr_engine *, const cr_state_entry *);
/* Metadata operations never load a core or modify legacy manual/automatic states. */
bool cr_state_store_rename(const char *root, const cr_state_entry *, const char *name,
                           char *error, size_t error_size);
bool cr_state_store_duplicate(const char *root, const cr_state_entry *, cr_state_entry *created,
                              char *error, size_t error_size);
bool cr_state_store_delete(const char *root, const cr_state_entry *, char *error, size_t error_size);
/* Caller owns a bounded exact raw SDK state for sharing, not an executable module. */
bool cr_state_store_read(const char *root, const cr_state_entry *, uint8_t **bytes, size_t *size,
                         char *error, size_t error_size);
#endif
