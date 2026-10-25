// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
#ifndef CORVUS_VIEWPORT_H
#define CORVUS_VIEWPORT_H
typedef struct {
    int left, top, right, bottom;
} cr_rect;
/* area: full window width/height, then safe insets left/top/right/bottom, in pixels. */
cr_rect cr_viewport(int width, int height, const int area[6], cr_rect content);
#endif
