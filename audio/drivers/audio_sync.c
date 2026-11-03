// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
#include "audio_sync.h"
#include <math.h>
#include <string.h>

void cr_audio_sync_reset(cr_audio_sync *s) { memset(s, 0, sizeof(*s)); }
void cr_audio_sync_update(cr_audio_sync *s, double now, unsigned queued, unsigned capacity,
                          unsigned prime, int rate, double fps, bool playing) {
    if (!playing || !isfinite(now) || !isfinite(fps) || fps < 1 || fps > 1000 ||
        rate <= 0 || !capacity || queued > capacity) {
        cr_audio_sync_reset(s);
        return;
    }
    if (!s->active || s->rate != rate || s->fps != fps || now < s->last_time) {
        cr_audio_sync_reset(s);
        s->active = true; s->rate = rate; s->fps = fps;
        s->filtered = queued;
        s->target = fmin(prime + .5 * rate / fps, .75 * capacity);
        s->last_time = now;
        return;
    }
    double dt = fmin(now - s->last_time, .1);
    if (dt <= 0) return;
    s->last_time = now;
    s->filtered += dt / (.5 + dt) * (queued - s->filtered);
    double correction = .25 * (s->target - s->filtered) / rate;
    s->correction = fmax(-.005, fmin(.005, correction));
}
double cr_audio_sync_period(const cr_audio_sync *s, double fps) {
    if (!isfinite(fps) || fps < 1 || fps > 1000) return 1.0 / 60;
    return (1.0 / fps) / (1.0 + (s->active ? s->correction : 0));
}
