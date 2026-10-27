// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
#ifndef CORVUS_NOTIFICATIONS_H
#define CORVUS_NOTIFICATIONS_H
#include "render.h"
#include "theme.h"

enum { CR_NOTIFICATION_GAME, CR_NOTIFICATION_UNLOCK };
enum { CR_NOTIFICATION_CAPACITY = 8 };
typedef struct {
    int kind;
    uint32_t id;
    char title[192], detail[256], image_url[512];
} cr_notification_card;
typedef struct {
    uint32_t id;
    char text[64], image_url[512];
    float percent;
    bool visible;
} cr_notification_progress;
typedef struct {
    cr_notification_card cards[CR_NOTIFICATION_CAPACITY];
    size_t count;
    double elapsed, progress_time;
    bool card_started;
    cr_notification_progress progress;
} cr_notifications;

/* All state belongs to the frontend thread. Strings are copied before return. */
void cr_notifications_push(cr_notifications *, int kind, uint32_t id, const char *title,
                           const char *detail, const char *image_url);
/* Measured percentage is 0..100; a non-finite value omits the percentage bar. */
void cr_notifications_progress(cr_notifications *, uint32_t id, const char *text,
                               const char *image_url, float percent);
/* id == 0 hides the current indicator regardless of its achievement. */
void cr_notifications_hide_progress(cr_notifications *, uint32_t id);
void cr_notifications_clear(cr_notifications *);
void cr_notifications_update(cr_notifications *, double seconds);
void cr_notifications_draw(const cr_notifications *, const cr_painter *, float width,
                           float height, const cr_theme *);
/* The caller supplies a safe area and restores its clip after drawing animated cards. */
void cr_notifications_draw_area(const cr_notifications *, const cr_painter *, float width,
                                float top, float bottom, const cr_theme *);
void cr_notifications_draw_region(const cr_notifications *, const cr_painter *, float left,
                                  float top, float right, float bottom, const cr_theme *);
#endif
