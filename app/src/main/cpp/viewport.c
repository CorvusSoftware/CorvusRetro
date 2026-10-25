// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
#include "viewport.h"
cr_rect cr_viewport(int width, int height, const int area[6], cr_rect content) {
    /* Reject snapshots from before a rotation instead of applying stale side insets. */
    if (area && area[0] == width && area[1] == height && area[2] >= 0 && area[3] >= 0 &&
        area[4] >= 0 && area[5] >= 0 && area[2] < width && area[3] < height &&
        area[4] < width - area[2] && area[5] < height - area[3])
        return (cr_rect){area[2], area[3], width - area[4], height - area[5]};
    /* NativeActivity contentRect is only a fallback before Java receives insets. */
    if (content.left >= 0 && content.top >= 0 && content.right > content.left &&
        content.bottom > content.top && content.right <= width && content.bottom <= height)
        return content;
    return (cr_rect){0, 0, width, height};
}
