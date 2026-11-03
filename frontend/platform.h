// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
#ifndef CORVUS_PLATFORM_H
#define CORVUS_PLATFORM_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
typedef struct {
    char id[96], title[512];
    int system;
    char core_id[96], system_id[96];
    int64_t played;
    char cover[256];
} cr_game;
typedef struct {
    char id[96], title[512], path[4096], error[512];
    int system;
} cr_import;
#define CR_MAX_CORE_PACKS 32
typedef struct {
    char id[96], name[128], version[64], core_id[96], abi[24];
    char description[256], system_name[128];
    uint64_t size;
} cr_core_pack;
typedef struct {
    cr_core_pack packs[CR_MAX_CORE_PACKS];
    size_t count;
    char error[256];
} cr_core_catalog;
enum { CR_SAVE_IMPORT, CR_SAVE_EXPORT_SAV, CR_SAVE_EXPORT_SRM };
typedef struct {
    uint64_t request;
    char path[4096], error[512];
    bool cancelled;
} cr_save_transfer_result;

typedef struct {
    void *context;
    int (*preference)(void *, const char *, int);
    void (*set_preference)(void *, const char *, int);
    void (*pick)(void *, int);
    bool (*finish_import)(void *, bool, int, const char *, const char *, char *, size_t);
    cr_game *(*games)(void *, size_t *);
    void (*touch)(void *, const char *);
    char *(*credits)(void *);
    void (*appearance)(void *, int, bool);
    void (*quit)(void *);
    void (*refresh_core_catalog)(void *);
    void (*install_core_pack)(void *, const char *);
    bool (*core_install_pending)(void *);
    void (*achievement_request)(void *, uint64_t, const char *, const char *);
    void (*achievement_credentials)(void *, char *, size_t, char *, size_t);
    void (*save_achievement_credentials)(void *, const char *, const char *);
    void (*edit_text)(void *, int, const char *, bool);
    void (*pick_folder)(void *, int, const char *);
    bool (*save_transfer)(void *, uint64_t, int, const char *, const uint8_t *, size_t);
    void (*finish_save_transfer)(void *, uint64_t);
    bool (*controller_info)(void *, unsigned port, char *name, size_t capacity);
    void (*achievement_sound)(void *, int volume_percent);
    bool (*can_remove_core)(void *, const char *core_id);
    void (*remove_core)(void *, const char *core_id);
    void (*game_image)(void *, const char *game_id, const char *url);
    void (*share_state)(void *, const char *name, const uint8_t *, size_t);
} cr_platform;
#endif
