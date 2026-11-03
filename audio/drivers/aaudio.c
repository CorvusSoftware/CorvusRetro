// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
#define _POSIX_C_SOURCE 200809L
#include "aaudio.h"
#include "volume.h"
#include <limits.h>
#include <string.h>
#include <time.h>

_Static_assert(ATOMIC_INT_LOCK_FREE == 2 && UINT_MAX == UINT32_MAX,
               "Audio counters require lock-free 32-bit atomics");
#define CR_AUDIO_RETRY_NS INT64_C(250000000)
/* NativeActivity has one audio owner. Late errors reach only permanent ticket storage. */
static atomic_uint error_state = ATOMIC_VAR_INIT(0);
static atomic_uint next_error_ticket = ATOMIC_VAR_INIT(2);
static bool stream_failed(const cr_audio *a) {
    return a->error_ticket && atomic_load_explicit(&error_state, memory_order_acquire) ==
           (a->error_ticket | 1u);
}
static unsigned begin_error_ticket(void) {
    unsigned ticket = atomic_fetch_add_explicit(&next_error_ticket, 2, memory_order_relaxed);
    if (!ticket) ticket = atomic_fetch_add_explicit(&next_error_ticket, 2, memory_order_relaxed);
    atomic_store_explicit(&error_state, ticket, memory_order_release);
    return ticket;
}
static void end_error_ticket(cr_audio *a) {
    unsigned expected = a->error_ticket;
    if (!expected) return;
    if (!atomic_compare_exchange_strong_explicit(&error_state, &expected, 0,
            memory_order_acq_rel, memory_order_acquire) && expected == (a->error_ticket | 1u))
        atomic_compare_exchange_strong_explicit(&error_state, &expected, 0,
            memory_order_acq_rel, memory_order_acquire);
    a->error_ticket = 0;
}
static int64_t now_ns(void) {
    struct timespec value;
    if (clock_gettime(CLOCK_MONOTONIC, &value) != 0) return 0;
    return (int64_t)value.tv_sec * INT64_C(1000000000) + value.tv_nsec;
}
static aaudio_data_callback_result_t output(AAudioStream *stream, void *ctx, void *buffer,
                                            int32_t frames) {
    (void)stream;
    cr_audio *a = ctx;
    if (!buffer || frames <= 0) return AAUDIO_CALLBACK_RESULT_CONTINUE;
    int16_t *out = buffer;
    if (!atomic_load_explicit(&a->enabled, memory_order_acquire)) {
        memset(out, 0, (size_t)frames * 2 * sizeof(*out));
        return AAUDIO_CALLBACK_RESULT_CONTINUE;
    }
    unsigned read = atomic_load_explicit(&a->read_frame, memory_order_relaxed);
    unsigned write = atomic_load_explicit(&a->write_frame, memory_order_acquire);
    unsigned available = write - read;
    size_t take = available < (size_t)frames ? available : (size_t)frames;
    int volume = atomic_load_explicit(&a->volume, memory_order_relaxed);
    for (size_t i = 0; i < take; ++i) {
        size_t slot = ((read + (unsigned)i) % CR_AUDIO_RING_FRAMES) * 2;
        out[i * 2] = cr_audio_gain(a->ring[slot], volume);
        out[i * 2 + 1] = cr_audio_gain(a->ring[slot + 1], volume);
    }
    memset(out + take * 2, 0, ((size_t)frames - take) * 2 * sizeof(*out));
    atomic_store_explicit(&a->read_frame, read + (unsigned)take, memory_order_release);
    return AAUDIO_CALLBACK_RESULT_CONTINUE;
}
static void failed(AAudioStream *stream, void *ctx, aaudio_result_t error) {
    (void)stream; (void)error;
    unsigned expected = (unsigned)(uintptr_t)ctx;
    if (expected)
        atomic_compare_exchange_strong_explicit(&error_state, &expected, expected | 1u,
            memory_order_acq_rel, memory_order_acquire);
}
void cr_audio_init(cr_audio *a) {
    memset(a, 0, sizeof(*a));
    atomic_init(&a->read_frame, 0);
    atomic_init(&a->write_frame, 0);
    atomic_init(&a->enabled, false);
    atomic_init(&a->volume, 100);
}
void cr_audio_close(cr_audio *a) {
    atomic_store_explicit(&a->enabled, false, memory_order_release);
    end_error_ticket(a);
    if (a->stream) {
        AAudioStream_requestStop(a->stream);
        AAudioStream_close(a->stream);
        a->stream = NULL;
    }
    /* Closing on the owner thread ends callbacks before resetting their counters. */
    atomic_store_explicit(&a->read_frame, 0, memory_order_relaxed);
    atomic_store_explicit(&a->write_frame, 0, memory_order_relaxed);
    a->playing = false;
    a->rate = a->burst_frames = a->prime_frames = 0;
    a->buffer_frames = a->capacity_frames = a->last_xrun = 0;
    a->retry_after_ns = 0;
}
static bool retry_later(cr_audio *a) {
    cr_audio_close(a);
    a->retry_after_ns = now_ns() + CR_AUDIO_RETRY_NS;
    return false;
}
static void adjust_buffer(cr_audio *a) {
    int xruns = AAudioStream_getXRunCount(a->stream);
    if (xruns < 0) return;
    if (xruns > a->last_xrun) {
        int maximum = a->burst_frames * 8;
        if (maximum > a->capacity_frames) maximum = a->capacity_frames;
        if (maximum > (int)CR_AUDIO_RING_FRAMES) maximum = CR_AUDIO_RING_FRAMES;
        int next = a->buffer_frames + a->burst_frames;
        if (next > maximum) next = maximum;
        if (next > a->buffer_frames) {
            int actual = AAudioStream_setBufferSizeInFrames(a->stream, next);
            if (actual > 0) a->buffer_frames = actual;
        }
    }
    a->last_xrun = xruns;
}
bool cr_audio_start(cr_audio *a, int rate) {
    if (rate <= 0) return false;
    if (stream_failed(a)) return retry_later(a);
    if (a->stream && a->rate != rate) cr_audio_close(a);
    if (!a->stream) {
        if (now_ns() < a->retry_after_ns) return false;
        AAudioStreamBuilder *builder = NULL;
        if (AAudio_createStreamBuilder(&builder) != AAUDIO_OK) return retry_later(a);
        AAudioStreamBuilder_setDirection(builder, AAUDIO_DIRECTION_OUTPUT);
        AAudioStreamBuilder_setFormat(builder, AAUDIO_FORMAT_PCM_I16);
        AAudioStreamBuilder_setChannelCount(builder, 2);
        AAudioStreamBuilder_setSampleRate(builder, rate);
        AAudioStreamBuilder_setSharingMode(builder, AAUDIO_SHARING_MODE_SHARED);
        AAudioStreamBuilder_setPerformanceMode(builder, AAUDIO_PERFORMANCE_MODE_LOW_LATENCY);
        AAudioStreamBuilder_setDataCallback(builder, output, a);
        a->error_ticket = begin_error_ticket();
        AAudioStreamBuilder_setErrorCallback(builder, failed, (void *)(uintptr_t)a->error_ticket);
        aaudio_result_t result = AAudioStreamBuilder_openStream(builder, &a->stream);
        AAudioStreamBuilder_delete(builder);
        if (result != AAUDIO_OK || !a->stream) return retry_later(a);
        if (AAudioStream_getSampleRate(a->stream) != rate ||
            AAudioStream_getChannelCount(a->stream) != 2 ||
            AAudioStream_getFormat(a->stream) != AAUDIO_FORMAT_PCM_I16) return retry_later(a);
        a->rate = rate;
        int burst = AAudioStream_getFramesPerBurst(a->stream);
        if (burst <= 0) burst = rate / 100;
        if (burst < 1) burst = 1;
        if (burst > (int)CR_AUDIO_RING_FRAMES / 2) burst = CR_AUDIO_RING_FRAMES / 2;
        a->burst_frames = burst;
        a->prime_frames = burst * 2;
        a->capacity_frames = AAudioStream_getBufferCapacityInFrames(a->stream);
        if (a->capacity_frames <= 0) a->capacity_frames = a->prime_frames;
        int target = a->prime_frames < a->capacity_frames ? a->prime_frames : a->capacity_frames;
        int actual = AAudioStream_setBufferSizeInFrames(a->stream, target);
        a->buffer_frames = actual > 0 ? actual : target;
        a->last_xrun = AAudioStream_getXRunCount(a->stream);
        if (a->last_xrun < 0) a->last_xrun = 0;
    }
    if (a->playing) adjust_buffer(a);
    return true;
}
void cr_audio_pause(cr_audio *a) { cr_audio_close(a); }
unsigned cr_audio_queued_frames(const cr_audio *a) {
    if (!a->playing || !a->stream || stream_failed(a)) return 0;
    unsigned read = atomic_load_explicit(&a->read_frame, memory_order_acquire);
    unsigned write = atomic_load_explicit(&a->write_frame, memory_order_relaxed);
    return write - read;
}
void cr_audio_set_volume(cr_audio *a, int percent) {
    int value = percent < 0 ? 0 : percent > 100 ? 100 : percent;
    atomic_store_explicit(&a->volume, value, memory_order_relaxed);
}
void cr_audio_write(cr_audio *a, const int16_t *samples, size_t count) {
    if (!a->stream || !samples || stream_failed(a)) return;
    unsigned write = atomic_load_explicit(&a->write_frame, memory_order_relaxed);
    unsigned read = atomic_load_explicit(&a->read_frame, memory_order_acquire);
    size_t space = CR_AUDIO_RING_FRAMES - (write - read);
    size_t frames = count / 2;
    if (frames > space) frames = space;
    if (!a->playing && frames) {
        /* Two PCM batches cover a missed display tick. Fractional rates alternate by one frame. */
        size_t target = frames * 2 - 1;
        if (target > CR_AUDIO_RING_FRAMES) target = CR_AUDIO_RING_FRAMES;
        if (target > (size_t)a->prime_frames) a->prime_frames = (int)target;
    }
    /* Only the callback advances read; overflow discards new complete stereo frames. */
    for (size_t i = 0; i < frames; ++i) {
        size_t slot = ((write + (unsigned)i) % CR_AUDIO_RING_FRAMES) * 2;
        a->ring[slot] = samples[i * 2];
        a->ring[slot + 1] = samples[i * 2 + 1];
    }
    write += (unsigned)frames;
    atomic_store_explicit(&a->write_frame, write, memory_order_release);
    if (!a->playing && write - read >= (unsigned)a->prime_frames) {
        atomic_store_explicit(&a->enabled, true, memory_order_release);
        if (AAudioStream_requestStart(a->stream) != AAUDIO_OK) { retry_later(a); return; }
        a->playing = true;
    }
}
void cr_audio_destroy(cr_audio *a) { cr_audio_close(a); }
