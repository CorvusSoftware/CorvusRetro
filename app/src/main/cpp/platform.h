// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
#ifndef CORVUS_PLATFORM_H
#define CORVUS_PLATFORM_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
typedef struct {
    char id[68], title[512];
    int system;
    int64_t played;
} cr_game;
typedef struct {
    char id[68], title[512], path[4096], error[512];
    int system;
} cr_import;
typedef struct {
    void *context;
    int (*preference)(void *, const char *, int);
    void (*set_preference)(void *, const char *, int);
    void (*pick)(void *, int);
    bool (*finish_import)(void *, bool, int, char *, size_t);
    cr_game *(*games)(void *, size_t *);
    void (*touch)(void *, const char *);
    char *(*credits)(void *);
    void (*appearance)(void *, int, bool);
    void (*quit)(void *);
} cr_platform;
#endif
