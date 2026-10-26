// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
#include "task_achievements.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static void check(bool value, const char *what) {
    if (!value) { fprintf(stderr, "FAIL %s\n", what); exit(1); }
}
static void *worker(void *context) {
    uint64_t id = *(uint64_t *)context;
    char body[] = "owned response";
    cr_achievement_response_push(id, 200, body, strlen(body));
    memset(body, 0, sizeof(body));
    return NULL;
}
int main(void) {
    cr_achievement_tasks_clear();
    cr_achievement_response value;
    cr_achievement_response_push(0, 200, "stale replay", 12);
    check(!cr_achievement_response_pop(&value), "background replays cannot occupy active request slots");
    check(!cr_achievement_request_track(0), "zero ID cannot be registered");
    uint64_t id = 41;
    check(cr_achievement_request_track(id), "request registered");
    pthread_t thread;
    check(pthread_create(&thread, NULL, worker, &id) == 0 && pthread_join(thread, NULL) == 0, "worker completion");
    check(cr_achievement_response_pop(&value) && value.id == id && value.status == 200 &&
          strcmp(value.body, "owned response") == 0, "mailbox owns a bounded copy, consumed by frontend");
    cr_achievement_response_dispose(&value);
    cr_achievement_response_push(id, 200, "late", 4);
    check(!cr_achievement_response_pop(&value), "late duplicate response ignored");
    for (id = 1; id <= 32; ++id) check(cr_achievement_request_track(id), "bounded slots filled");
    check(!cr_achievement_request_track(33), "outstanding request count bounded");
    cr_achievement_tasks_clear();
    cr_achievement_response_push(32, 200, "old activity", 12);
    check(!cr_achievement_response_pop(&value), "old activity cannot deliver into replacement");
    check(cr_achievement_request_track(99), "replacement request registered");
    cr_achievement_response_push(99, 200, "x", 4u * 1024u * 1024u + 1u);
    check(cr_achievement_response_pop(&value) && value.status == -2 && !value.body && !value.size,
          "oversized response becomes retryable without copying untrusted bytes");
    cr_achievement_response_dispose(&value); cr_achievement_tasks_clear();
    puts("PASS native achievement mailbox ownership, replay IDs and lifecycle");
    return 0;
}
