// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
#include "task_achievements.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#define REQUESTS 32
#define RESPONSE_LIMIT (4u * 1024u * 1024u)
#define QUEUE_LIMIT (8u * 1024u * 1024u)
static struct { cr_achievement_response value; bool ready; } slots[REQUESTS];
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static size_t queued_bytes;
void cr_achievement_response_dispose(cr_achievement_response *response) {
    if (response->body) {
        volatile unsigned char *p = (volatile unsigned char *)response->body;
        for (size_t i = 0; i < response->size; ++i) p[i] = 0;
        free(response->body);
    }
    *response = (cr_achievement_response){0};
}
bool cr_achievement_request_track(uint64_t id) {
    if (!id) return false;
    bool tracked = false;
    size_t available = REQUESTS;
    pthread_mutex_lock(&mutex);
    for (size_t i = 0; i < REQUESTS; ++i) {
        if (slots[i].value.id == id) { tracked = true; break; }
        if (!slots[i].value.id && available == REQUESTS) available = i;
    }
    if (!tracked && available < REQUESTS) {
        slots[available].value.id = id;
        tracked = true;
    }
    pthread_mutex_unlock(&mutex);
    return tracked;
}
void cr_achievement_response_push(uint64_t id, int status, const void *body, size_t size) {
    if (!id) return;
    pthread_mutex_lock(&mutex);
    for (size_t i = 0; i < REQUESTS; ++i) {
        if (slots[i].value.id != id || slots[i].ready) continue;
        cr_achievement_response *response = &slots[i].value;
        response->status = status;
        if (body && size && size <= RESPONSE_LIMIT && size <= QUEUE_LIMIT - queued_bytes) {
            response->body = malloc(size + 1);
            if (response->body) {
                memcpy(response->body, body, size);
                response->body[size] = 0;
                response->size = size;
                queued_bytes += size;
            } else response->status = -2;
        } else if (size) response->status = -2;
        slots[i].ready = true;
        break;
    }
    pthread_mutex_unlock(&mutex);
}
bool cr_achievement_response_pop(cr_achievement_response *out) {
    if (!out) return false;
    bool found = false;
    pthread_mutex_lock(&mutex);
    for (size_t i = 0; i < REQUESTS; ++i) {
        if (!slots[i].ready) continue;
        *out = slots[i].value;
        queued_bytes -= out->size;
        memset(&slots[i], 0, sizeof(slots[i]));
        found = true;
        break;
    }
    pthread_mutex_unlock(&mutex);
    return found;
}
void cr_achievement_tasks_clear(void) {
    pthread_mutex_lock(&mutex);
    for (size_t i = 0; i < REQUESTS; ++i) {
        cr_achievement_response_dispose(&slots[i].value);
        slots[i].ready = false;
    }
    queued_bytes = 0;
    pthread_mutex_unlock(&mutex);
}
