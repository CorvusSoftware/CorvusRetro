// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
#ifndef CORVUS_OVERLAY_H
#define CORVUS_OVERLAY_H
#include "play_layout.h"
#include "render.h"
/* The caller supplies input state from the frontend thread. */
void cr_overlay_draw(const cr_play_layout *, const cr_painter *, unsigned held,
                     const float feedback[10], bool visible, bool menu_down, bool hide_down);
#endif
