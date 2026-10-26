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
} cr_game;
typedef struct {
    char id[96], title[512], path[4096], error[512];
    int system;
} cr_import;
#define CR_MAX_CORE_PACKS 32
typedef struct {
    char id[96], name[128], version[64], core_id[96], abi[24];
} cr_core_pack;
typedef struct {
    cr_core_pack packs[CR_MAX_CORE_PACKS];
    size_t count;
    char error[256];
} cr_core_catalog;

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
} cr_platform;
#endif
