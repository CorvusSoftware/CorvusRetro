// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
#ifndef CORVUS_TEST_AAUDIO_H
#define CORVUS_TEST_AAUDIO_H
#include <stdint.h>
typedef struct AAudioStream AAudioStream;
typedef struct AAudioStreamBuilder AAudioStreamBuilder;
typedef int32_t aaudio_result_t;
typedef int32_t aaudio_data_callback_result_t;
typedef aaudio_data_callback_result_t (*AAudioStream_dataCallback)(AAudioStream *,void *,void *,int32_t);
typedef void (*AAudioStream_errorCallback)(AAudioStream *,void *,aaudio_result_t);
enum { AAUDIO_OK=0, AAUDIO_CALLBACK_RESULT_CONTINUE=0, AAUDIO_DIRECTION_OUTPUT=0,
    AAUDIO_FORMAT_PCM_I16=1, AAUDIO_SHARING_MODE_SHARED=1, AAUDIO_PERFORMANCE_MODE_LOW_LATENCY=12 };
aaudio_result_t AAudio_createStreamBuilder(AAudioStreamBuilder **);
void AAudioStreamBuilder_setDirection(AAudioStreamBuilder *,int32_t);
void AAudioStreamBuilder_setFormat(AAudioStreamBuilder *,int32_t);
void AAudioStreamBuilder_setChannelCount(AAudioStreamBuilder *,int32_t);
void AAudioStreamBuilder_setSampleRate(AAudioStreamBuilder *,int32_t);
void AAudioStreamBuilder_setSharingMode(AAudioStreamBuilder *,int32_t);
void AAudioStreamBuilder_setPerformanceMode(AAudioStreamBuilder *,int32_t);
void AAudioStreamBuilder_setDataCallback(AAudioStreamBuilder *,AAudioStream_dataCallback,void *);
void AAudioStreamBuilder_setErrorCallback(AAudioStreamBuilder *,AAudioStream_errorCallback,void *);
aaudio_result_t AAudioStreamBuilder_openStream(AAudioStreamBuilder *,AAudioStream **);
aaudio_result_t AAudioStreamBuilder_delete(AAudioStreamBuilder *);
aaudio_result_t AAudioStream_requestStart(AAudioStream *);
aaudio_result_t AAudioStream_requestStop(AAudioStream *);
aaudio_result_t AAudioStream_close(AAudioStream *);
int32_t AAudioStream_getSampleRate(AAudioStream *);
int32_t AAudioStream_getChannelCount(AAudioStream *);
int32_t AAudioStream_getFormat(AAudioStream *);
int32_t AAudioStream_getFramesPerBurst(AAudioStream *);
int32_t AAudioStream_getBufferCapacityInFrames(AAudioStream *);
int32_t AAudioStream_getXRunCount(AAudioStream *);
int32_t AAudioStream_setBufferSizeInFrames(AAudioStream *,int32_t);
#endif
