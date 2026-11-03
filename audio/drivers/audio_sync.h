// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
#ifndef CORVUS_AUDIO_SYNC_H
#define CORVUS_AUDIO_SYNC_H
#include <stdbool.h>

enum { CR_AUDIO_SYNC_MAX_CATCHUP = 3 };
typedef struct {
    double filtered, target, correction, last_time, fps;
    int rate;
    bool active;
} cr_audio_sync;

void cr_audio_sync_reset(cr_audio_sync *);
/* Owner-thread queue feedback. PCM and core cycles per frame remain unchanged. */
void cr_audio_sync_update(cr_audio_sync *, double now, unsigned queued, unsigned capacity,
                          unsigned prime, int rate, double fps, bool playing);
double cr_audio_sync_period(const cr_audio_sync *, double fps);
#endif
