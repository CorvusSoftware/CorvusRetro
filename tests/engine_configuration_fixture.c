// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
#include <corvusapi.h>
#include <stdlib.h>
#include <string.h>

typedef struct fixture {
    corvus_host host;
    unsigned players, value, inputs[2];
    uint8_t battery[2][2];
    uint64_t generation[2], ack[2];
} fixture;
static int info(corvus_core_info *out) {
    *out = (corvus_core_info){sizeof(*out), "test.configuration", "Configuration fixture",
#ifdef FIXTURE_REORDER
        "2",
#else
        "1",
#endif
        "Fixture", "rom", 2};
    return 1;
}
static void *create(const corvus_host *host) {
    fixture *f = calloc(1, sizeof(*f));
    if (f) {
        f->host = *host;
#ifdef FIXTURE_REORDER
        f->value = 1;
#endif
    }
    return f;
}
static void destroy(void *core) { free(core); }
static int load_slots(void *core, const corvus_game *games, unsigned count) {
    fixture *f = core;
    if (!f || f->players) return 0;
    for (unsigned i = 0; i < count; ++i)
        if (!games[i].rom_data || !games[i].rom_size || games[i].rom_data[0] == 255 ||
            (games[i].battery_size && games[i].battery_size != 2)) return 0;
    for (unsigned i = 0; i < count; ++i) {
        f->battery[i][0] = games[i].rom_data[0];
        f->battery[i][1] = 0;
        if (games[i].battery_size) memcpy(f->battery[i], games[i].battery_data, 2);
        f->generation[i] = 1;
        f->ack[i] = 0;
    }
    f->players = count;
    return 1;
}
static int load(void *core, const corvus_game *game) { return load_slots(core, game, 1); }
static void unload(void *core) { ((fixture *)core)->players = 0; }
static int av(void *core, corvus_av_info *out) {
    fixture *f = core;
    *out = (corvus_av_info){sizeof(*out), f->players, 1, 60, 48000};
    return f->players != 0;
}
static void button(void *core, uint32_t player, uint32_t key, int pressed) {
    fixture *f = core;
    if (player < f->players && key == CORVUS_BUTTON_A) f->inputs[player] = (unsigned)pressed;
}
static int frame(void *core) {
    fixture *f = core;
    if (!f->players) return 0;
    uint32_t pixels[2];
    for (unsigned i = 0; i < f->players; ++i) {
        pixels[i] = 0xff000000u | f->inputs[i];
        f->battery[i][1] = (uint8_t)(f->battery[i][1] + 1 + f->inputs[i]);
        ++f->generation[i];
    }
    int16_t audio[2] = {1, -1};
    f->host.video_frame(f->host.userdata, pixels, f->players, 1, f->players);
    f->host.audio_frames(f->host.userdata, audio, 1);
    return 1;
}
static int reset(void *core) { return ((fixture *)core)->players != 0; }
static size_t state_size(void *core) {
#ifdef FIXTURE_STATES
    return ((fixture *)core)->players * 2;
#else
    return ((fixture *)core)->players == 1 ? 2 : 0;
#endif
}
static int serialize(void *core, uint8_t *out, size_t n) {
    size_t size = state_size(core);
    if (!size || n < size) return 0;
    memcpy(out, ((fixture *)core)->battery, size); return 1;
}
static int unserialize(void *core, const uint8_t *in, size_t n) {
    if (!state_size(core) || n != state_size(core)) return 0;
    fixture *f = core;
    memcpy(f->battery, in, n);
    for (unsigned i = 0; i < f->players; ++i) ++f->generation[i];
    return 1;
}
static size_t slot_size(void *core, uint32_t slot) { return slot < ((fixture *)core)->players ? 2 : 0; }
static int slot_dirty(void *core, uint32_t slot) {
    fixture *f = core; return slot_size(core, slot) && f->generation[slot] != f->ack[slot];
}
static int slot_copy(void *core, uint32_t slot, uint8_t *out, size_t n, uint64_t *gen) {
    fixture *f = core;
    if (!out || !gen || n < 2 || !slot_size(core, slot)) return 0;
    memcpy(out, f->battery[slot], 2); *gen = f->generation[slot]; return 1;
}
static int slot_ack(void *core, uint32_t slot, uint64_t gen) {
    fixture *f = core;
    if (!slot_size(core, slot) || gen != f->generation[slot]) return 0;
    f->ack[slot] = gen; return 1;
}
static size_t battery_size(void *core) { return ((fixture *)core)->players == 1 ? 2 : 0; }
static int battery_dirty(void *core) { return battery_size(core) && slot_dirty(core, 0); }
static int battery_copy(void *core, uint8_t *out, size_t n, uint64_t *gen) {
    return battery_size(core) && slot_copy(core, 0, out, n, gen);
}
static int battery_ack(void *core, uint64_t gen) { return battery_size(core) && slot_ack(core, 0, gen); }
int corvus_get_api(uint32_t version, corvus_core_api *out) {
    if (version || !out || out->struct_size < sizeof(*out)) return 0;
    *out = (corvus_core_api){sizeof(*out), 0, info, create, destroy, load, unload, av,
        button, frame, reset, NULL, state_size, serialize, unserialize,
        battery_size, battery_dirty, battery_copy, battery_ack};
    return 1;
}
#ifndef FIXTURE_LEGACY
static const corvus_option_value values[] = {
#ifdef FIXTURE_REORDER
    {sizeof(corvus_option_value), "accurate", "Accurate"}, {sizeof(corvus_option_value), "fast", "Fast"}
#else
    {sizeof(corvus_option_value), "fast", "Fast"}, {sizeof(corvus_option_value), "accurate", "Accurate"}
#endif
};
static uint32_t option_count(void *core) {
#ifdef FIXTURE_MALFORMED
    return core ? 65 : 0;
#else
    return core ? 1 : 0;
#endif
}
static int describe(void *core, uint32_t index, corvus_core_option *out) {
    if (!core || index || !out || out->struct_size < sizeof(*out)) return 0;
    *out = (corvus_core_option){sizeof(*out), "accuracy", "Accuracy", "Fixture option",
        CORVUS_OPTION_RESTART_REQUIRED, 2, values,
#ifdef FIXTURE_REORDER
        1
#else
        0
#endif
    }; return 1;
}
static uint32_t get_value(void *core, const char *key) {
    return core && key && !strcmp(key, "accuracy") ? ((fixture *)core)->value : UINT32_MAX;
}
static int set_value(void *core, const char *key, uint32_t value) {
    if (!core || !key || strcmp(key, "accuracy") || value >= 2) return 0;
    ((fixture *)core)->value = value; return 1;
}
int corvus_get_options_api(uint32_t version, corvus_options_api *out) {
    if (version != 1 || !out || out->struct_size < sizeof(*out)) return 0;
    *out = (corvus_options_api){sizeof(*out), 1, option_count, describe, get_value, set_value}; return 1;
}
static const corvus_subsystem_rom slots[] = {
    {sizeof(corvus_subsystem_rom), "Player 1", "rom", 1},
    {sizeof(corvus_subsystem_rom), "Player 2", "rom", 1}
};
static uint32_t subsystem_count(void) {
#ifdef FIXTURE_STATES
    return 2;
#else
    return 1;
#endif
}
static int subsystem_describe(uint32_t index, corvus_subsystem_info *out) {
    if (index >= subsystem_count() || !out || out->struct_size < sizeof(*out)) return 0;
    *out = (corvus_subsystem_info){sizeof(*out), index ? "link.alternate" : "link", "Local link", 2, 2, slots}; return 1;
}
static int subsystem_load(void *core, const char *id, const corvus_game *games, size_t count) {
    return id && (!strcmp(id, "link") ||
        (subsystem_count() == 2 && !strcmp(id, "link.alternate"))) && count == 2 && load_slots(core, games, 2);
}
static uint32_t capabilities(void *core) {
    fixture *f = core;
#ifdef FIXTURE_STATES
    return f->players ? CORVUS_SESSION_RESET | CORVUS_SESSION_STATES : 0;
#else
    return f->players ? CORVUS_SESSION_RESET | (f->players == 1 ? CORVUS_SESSION_STATES : 0) : 0;
#endif
}
int corvus_get_subsystem_api(uint32_t version, corvus_subsystem_api *out) {
    if (version != 1 || !out || out->struct_size < sizeof(*out)) return 0;
    *out = (corvus_subsystem_api){sizeof(*out), 1, subsystem_count, subsystem_describe,
        subsystem_load, capabilities, slot_size, slot_dirty, slot_copy,
#ifdef FIXTURE_MALFORMED
        NULL
#else
        slot_ack
#endif
    }; return 1;
}
#endif
