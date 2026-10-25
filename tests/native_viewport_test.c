// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
#include "viewport.h"
#include <stdio.h>
#include <stdlib.h>
static void check(cr_rect r, int left, int top, int right, int bottom, const char *message) {
    if (r.left != left || r.top != top || r.right != right || r.bottom != bottom) {
        fprintf(stderr, "FAIL %s: %d,%d,%d,%d\n", message, r.left, r.top, r.right, r.bottom);
        exit(1);
    }
}
int main(void) {
    cr_rect full = {0, 0, 576, 1280};
    int portrait[] = {576, 1280, 0, 36, 0, 72};
    check(cr_viewport(576, 1280, portrait, full), 0, 36, 576, 1208,
          "status and three-button navigation excluded from full NativeActivity surface");
    int gesture[] = {576, 1280, 0, 54, 0, 24};
    check(cr_viewport(576, 1280, gesture, full), 0, 54, 576, 1256,
          "top cutout and gesture navigation use system pixel dimensions");
    int landscape[] = {1280, 576, 54, 24, 72, 0};
    check(cr_viewport(1280, 576, landscape, (cr_rect){0, 0, 1280, 576}), 54, 24, 1208, 576,
          "landscape cutout and navigation reserve both sides");
    check(cr_viewport(1280, 576, portrait, (cr_rect){0, 24, 1208, 576}), 0, 24, 1208, 576,
          "rotation rejects stale portrait snapshot");
    int no_bars[] = {480, 870, 0, 0, 0, 0};
    check(cr_viewport(480, 870, no_bars, (cr_rect){0, 30, 480, 900}), 0, 0, 480, 870,
          "already inset surface has no duplicate margins");
    int invalid[] = {576, 1280, -1, 36, 0, 72};
    check(cr_viewport(576, 1280, invalid, full), 0, 0, 576, 1280, "negative inset rejected");
    invalid[2] = 576;
    check(cr_viewport(576, 1280, invalid, full), 0, 0, 576, 1280, "empty safe area rejected");
    check(cr_viewport(576, 1280, NULL, (cr_rect){0, 36, 576, 1208}), 0, 36, 576, 1208,
          "initial NativeActivity content rectangle used before inset delivery");
    check(cr_viewport(576, 1280, NULL, (cr_rect){0, 0, 0, 0}), 0, 0, 576, 1280,
          "empty initial content rectangle handled");
    puts("PASS native viewport");
    return 0;
}
