// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
#include "corvus_core.h"

void cr_reset(cr_session *session)
{
    session->x = (CR_WIDTH - CR_SPRITE_SIZE) / 2;
    session->y = (CR_HEIGHT - CR_SPRITE_SIZE) / 2;
    session->paused = false;
}

static int clamp(int value, int maximum)
{
    return value < 0 ? 0 : value > maximum ? maximum : value;
}

void cr_step(cr_session *session, unsigned input)
{
    if (session->paused) return;
    session->x = clamp(session->x + 2 * (!!(input & CR_RIGHT) - !!(input & CR_LEFT)),
                       CR_WIDTH - CR_SPRITE_SIZE);
    session->y = clamp(session->y + 2 * (!!(input & CR_DOWN) - !!(input & CR_UP)),
                       CR_HEIGHT - CR_SPRITE_SIZE);
}

bool cr_render(const cr_session *session, uint32_t *pixels, size_t count)
{
    if (!pixels || count < (size_t)CR_WIDTH * CR_HEIGHT) return false;
    for (int y = 0; y < CR_HEIGHT; ++y) {
        for (int x = 0; x < CR_WIDTH; ++x) {
            uint32_t color = (x % 16 == 0 || y % 16 == 0) ? 0xff243149u : 0xff121b2du;
            if (x >= session->x && x < session->x + CR_SPRITE_SIZE &&
                y >= session->y && y < session->y + CR_SPRITE_SIZE)
                color = session->paused ? 0xffa89dcfu : 0xff57e0c2u;
            pixels[y * CR_WIDTH + x] = color;
        }
    }
    return true;
}
