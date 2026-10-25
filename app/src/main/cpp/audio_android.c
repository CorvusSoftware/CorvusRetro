// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
#include "audio_android.h"
#include <string.h>
static aaudio_data_callback_result_t output(AAudioStream *stream, void *ctx, void *buffer,
                                            int32_t frames) {
    (void)stream;
    cr_audio *a = ctx;
    int16_t *out = buffer;
    size_t needed = (size_t)frames * 2;
    pthread_mutex_lock(&a->mutex);
    for (size_t i = 0; i < needed; i++) {
        if (a->count) {
            out[i] = a->ring[a->read];
            a->read = (a->read + 1) % 32768;
            a->count--;
        } else
            out[i] = 0;
    }
    pthread_mutex_unlock(&a->mutex);
    return AAUDIO_CALLBACK_RESULT_CONTINUE;
}
static void failed(AAudioStream *stream, void *ctx, aaudio_result_t error) {
    (void)stream;
    (void)error;
    cr_audio *a = ctx;
    atomic_store(&a->failed, true);
}
void cr_audio_init(cr_audio *a) {
    memset(a, 0, sizeof(*a));
    pthread_mutex_init(&a->mutex, NULL);
    atomic_init(&a->failed, false);
}
void cr_audio_close(cr_audio *a) {
    if (a->stream) {
        AAudioStream_requestStop(a->stream);
        AAudioStream_close(a->stream);
        a->stream = NULL;
    }
    a->playing = false;
    a->rate = 0;
    pthread_mutex_lock(&a->mutex);
    a->count = 0;
    a->read = 0;
    pthread_mutex_unlock(&a->mutex);
    atomic_store(&a->failed, false);
}
bool cr_audio_start(cr_audio *a, int rate) {
    if (atomic_load(&a->failed) || a->rate != rate)
        cr_audio_close(a);
    if (!a->stream) {
        AAudioStreamBuilder *builder = NULL;
        if (AAudio_createStreamBuilder(&builder) != AAUDIO_OK)
            return false;
        AAudioStreamBuilder_setDirection(builder, AAUDIO_DIRECTION_OUTPUT);
        AAudioStreamBuilder_setFormat(builder, AAUDIO_FORMAT_PCM_I16);
        AAudioStreamBuilder_setChannelCount(builder, 2);
        AAudioStreamBuilder_setSampleRate(builder, rate);
        AAudioStreamBuilder_setSharingMode(builder, AAUDIO_SHARING_MODE_SHARED);
        AAudioStreamBuilder_setPerformanceMode(builder, AAUDIO_PERFORMANCE_MODE_LOW_LATENCY);
        AAudioStreamBuilder_setDataCallback(builder, output, a);
        AAudioStreamBuilder_setErrorCallback(builder, failed, a);
        aaudio_result_t result = AAudioStreamBuilder_openStream(builder, &a->stream);
        AAudioStreamBuilder_delete(builder);
        if (result != AAUDIO_OK || !a->stream) {
            a->stream = NULL;
            return false;
        }
        if (AAudioStream_getSampleRate(a->stream) != rate ||
            AAudioStream_getChannelCount(a->stream) != 2 ||
            AAudioStream_getFormat(a->stream) != AAUDIO_FORMAT_PCM_I16) {
            cr_audio_close(a);
            return false;
        }
        a->rate = rate;
    }
    if (!a->playing) {
        if (AAudioStream_requestStart(a->stream) != AAUDIO_OK) {
            cr_audio_close(a);
            return false;
        }
        a->playing = true;
    }
    return true;
}
void cr_audio_pause(cr_audio *a) {
    if (a->playing && a->stream)
        AAudioStream_requestPause(a->stream);
    a->playing = false;
    pthread_mutex_lock(&a->mutex);
    a->count = 0;
    a->read = 0;
    pthread_mutex_unlock(&a->mutex);
}
void cr_audio_write(cr_audio *a, const int16_t *samples, size_t count) {
    if (!a->stream || !a->playing)
        return;
    count &= ~(size_t)1;
    pthread_mutex_lock(&a->mutex);
    for (size_t i = 0; i < count; i++) {
        if (a->count == 32768) {
            a->read = (a->read + 2) % 32768;
            a->count -= 2;
        }
        a->ring[(a->read + a->count) % 32768] = samples[i];
        a->count++;
    }
    pthread_mutex_unlock(&a->mutex);
}
void cr_audio_destroy(cr_audio *a) {
    cr_audio_close(a);
    pthread_mutex_destroy(&a->mutex);
}
