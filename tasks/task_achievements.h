// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
#ifndef CORVUS_TASK_ACHIEVEMENTS_H
#define CORVUS_TASK_ACHIEVEMENTS_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
typedef struct {
    uint64_t id;
    int status;
    char *body;
    size_t size;
} cr_achievement_response;
bool cr_achievement_request_track(uint64_t id);
void cr_achievement_response_push(uint64_t id, int status, const void *body, size_t size);
bool cr_achievement_response_pop(cr_achievement_response *out);
void cr_achievement_response_dispose(cr_achievement_response *response);
void cr_achievement_tasks_clear(void);
#endif
