// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
#ifndef CORVUS_AUDIO_ANDROID_H
#define CORVUS_AUDIO_ANDROID_H
#include <aaudio/AAudio.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#define CR_AUDIO_RING_FRAMES 16384u
typedef struct {
    AAudioStream *stream;
    int16_t ring[CR_AUDIO_RING_FRAMES * 2];
    atomic_uint read_frame, write_frame;
    atomic_bool enabled;
    unsigned error_ticket;
    atomic_int volume;
    bool playing;
    int rate, burst_frames, prime_frames, buffer_frames, capacity_frames, last_xrun;
    int64_t retry_after_ns;
} cr_audio;
void cr_audio_init(cr_audio *);
/* Prepares the stream. PCM is primed before cr_audio_write starts playback. */
bool cr_audio_start(cr_audio *, int);
void cr_audio_pause(cr_audio *);
void cr_audio_set_volume(cr_audio *, int percent);
void cr_audio_write(cr_audio *, const int16_t *, size_t);
void cr_audio_close(cr_audio *);
void cr_audio_destroy(cr_audio *);
#endif
