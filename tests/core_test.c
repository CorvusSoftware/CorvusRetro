// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
#include "corvus_core.h"
#include <stdio.h>
#include <stdlib.h>

#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #condition); exit(1); \
} } while (0)

int main(void)
{
    cr_session session;
    cr_reset(&session);
    int x = session.x, y = session.y;
    cr_step(&session, CR_RIGHT | CR_UP);
    CHECK(session.x == x + 2 && session.y == y - 2);
    session.paused = true;
    cr_step(&session, CR_RIGHT);
    CHECK(session.x == x + 2);
    cr_reset(&session);
    CHECK(!session.paused && session.x == x && session.y == y);
    cr_step(&session, CR_LEFT | CR_RIGHT | CR_UP | CR_DOWN);
    CHECK(session.x == x && session.y == y);
    for (int i = 0; i < 1000; ++i) cr_step(&session, CR_RIGHT | CR_DOWN);
    CHECK(session.x == CR_WIDTH - CR_SPRITE_SIZE && session.y == CR_HEIGHT - CR_SPRITE_SIZE);
    for (int i = 0; i < 1000; ++i) cr_step(&session, CR_LEFT | CR_UP);
    CHECK(session.x == 0 && session.y == 0);
    uint32_t pixels[CR_WIDTH * CR_HEIGHT + 1];
    pixels[0] = 0x12345678u;
    pixels[CR_WIDTH * CR_HEIGHT] = 0x87654321u;
    CHECK(!cr_render(&session, pixels, 1) && pixels[0] == 0x12345678u);
    CHECK(!cr_render(&session, NULL, CR_WIDTH * CR_HEIGHT));
    CHECK(cr_render(&session, pixels, CR_WIDTH * CR_HEIGHT));
    CHECK(pixels[0] == 0xff57e0c2u);
    CHECK(pixels[CR_WIDTH * CR_HEIGHT - 1] == 0xff121b2du);
    CHECK(pixels[CR_WIDTH * CR_HEIGHT] == 0x87654321u);
    puts("PASS: movement, pause, reset, opposing inputs, bounds, ARGB output, buffer protection");
    return 0;
}
