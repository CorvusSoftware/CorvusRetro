// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
#ifndef CORVUS_PLAY_LAYOUT_H
#define CORVUS_PLAY_LAYOUT_H
#include <stdbool.h>
typedef struct {
    float x, y, w, h;
} cr_area;
typedef struct {
    cr_area area;
    unsigned mask;
    const char *label;
} cr_control;
typedef struct {
    cr_area video, panel, dpad, menu, hide;
    cr_control controls[10];
    int count;
    float scale;
    bool portrait;
} cr_play_layout;
cr_play_layout cr_play_layout_make(float width, float height, int system, int video_width,
                                   int video_height);
cr_play_layout cr_play_layout_configured(float width, float height, int system, int video_width,
                                         int video_height, int size_percent, int vertical_percent);
/* Integer multiples are measured in physical pixels; coordinates remain logical dp. */
cr_area cr_play_video_integer(cr_area reserved, int video_width, int video_height, float density);
bool cr_area_contains(cr_area area, float x, float y);
unsigned cr_play_hit(const cr_play_layout *, float x, float y);
#endif
