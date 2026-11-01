// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
#ifndef CORVUS_AUDIO_VOLUME_H
#define CORVUS_AUDIO_VOLUME_H
#include <stdint.h>
static inline int16_t cr_audio_gain(int16_t sample, int percent) {
    if (percent <= 0) return 0;
    if (percent >= 100) return sample;
    return (int16_t)((int32_t)sample * percent / 100);
}
#endif
