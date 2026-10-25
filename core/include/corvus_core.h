// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
#ifndef CORVUS_CORE_H
#define CORVUS_CORE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum { CR_WIDTH = 256, CR_HEIGHT = 192, CR_SPRITE_SIZE = 12 };
enum { CR_UP = 1, CR_DOWN = 2, CR_LEFT = 4, CR_RIGHT = 8 };

/* State belongs to the caller. All calls for one state use the same thread. */
typedef struct {
    int x;
    int y;
    bool paused;
} cr_session;

void cr_reset(cr_session *session);
void cr_step(cr_session *session, unsigned input);
/* ARGB pixels; undersized buffers are rejected without writing anything. */
bool cr_render(const cr_session *session, uint32_t *pixels, size_t count);

#endif
