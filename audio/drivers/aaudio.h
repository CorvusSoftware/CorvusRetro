// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
#ifndef CORVUS_AUDIO_ANDROID_H
#define CORVUS_AUDIO_ANDROID_H
#include <aaudio/AAudio.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
typedef struct {
    AAudioStream *stream;
    pthread_mutex_t mutex;
    int16_t ring[32768];
    size_t read, count;
    bool playing;
    int rate;
    atomic_bool failed;
} cr_audio;
void cr_audio_init(cr_audio *);
bool cr_audio_start(cr_audio *, int);
void cr_audio_pause(cr_audio *);
void cr_audio_write(cr_audio *, const int16_t *, size_t);
void cr_audio_close(cr_audio *);
void cr_audio_destroy(cr_audio *);
#endif
