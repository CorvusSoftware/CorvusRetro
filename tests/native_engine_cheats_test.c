// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
#define _POSIX_C_SOURCE 200809L
#ifdef CR_CHEATS_FIXTURE
#include "engine_configuration_fixture.c"
/* Build this source as small optional-extension protocol fixtures. */
static uint32_t cheat_caps(void *core, uint32_t player) {
    return core && player < ((fixture *)core)->players ? CORVUS_CHEAT_FORMAT_CORE_TEXT : 0;
}
static int cheat_validate(void *core, uint32_t player, const char *code) {
    return cheat_caps(core, player) && code && !strcmp(code, "valid");
}
static int cheat_replace(void *core, uint32_t player, const char *const *codes, size_t count) {
    if (!cheat_caps(core, player)) return 0;
    for (size_t i = 0; i < count; ++i) if (!cheat_validate(core, player, codes[i])) return 0;
    return 1;
}
int corvus_get_cheats_api(uint32_t version, corvus_cheats_api *out) {
    if (version != CORVUS_CHEATS_API_VERSION || !out || out->struct_size < sizeof(*out)) return 0;
    *out = (corvus_cheats_api){sizeof(*out), CORVUS_CHEATS_API_VERSION,
                              cheat_caps, cheat_validate, cheat_replace};
#if CR_CHEATS_FIXTURE == 1
    out->struct_size = sizeof(*out) - 1;
#elif CR_CHEATS_FIXTURE == 2
    out->api_version = CORVUS_CHEATS_API_VERSION + 1;
#elif CR_CHEATS_FIXTURE == 3
    out->capabilities = NULL;
#elif CR_CHEATS_FIXTURE == 4
    out->validate = NULL;
#elif CR_CHEATS_FIXTURE == 5
    out->replace = NULL;
#elif CR_CHEATS_FIXTURE == 6
    return 0;
#endif
    return 1;
}
#else
#include "engine.h"
#include "core_registry.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef CR_TEST_WRAP_CHEAT_ALLOC
static bool fail_cheat_calloc, fail_cheat_strdup;
void *__real_calloc(size_t count, size_t size);
char *__real_strdup(const char *text);
void *__wrap_calloc(size_t count, size_t size) {
    if (fail_cheat_calloc) { fail_cheat_calloc = false; return NULL; }
    return __real_calloc(count, size);
}
char *__wrap_strdup(const char *text) {
    if (fail_cheat_strdup) { fail_cheat_strdup = false; return NULL; }
    return __real_strdup(text);
}
#endif

static void check(bool condition, const char *message, const cr_engine *engine) {
    if (condition) return;
    fprintf(stderr, "FAIL %s: %s\n", message, engine ? engine->error : "");
    exit(1);
}
static void write_bytes(const char *path, const void *data, size_t size) {
    FILE *file = fopen(path, "wb");
    check(file && fwrite(data, 1, size, file) == size, "write synthetic ROM", NULL);
    check(!fclose(file), "close synthetic ROM", NULL);
}
static void make_rom(const char *path, int system) {
    size_t size = system == 2 ? 512 : 32768;
    uint8_t *rom = calloc(size, 1);
    check(rom != NULL, "allocate synthetic ROM", NULL);
    if (system < 2) {
        rom[0x100] = 0xc3; rom[0x101] = 0x50; rom[0x102] = 1;
        /* Read ROM[0200] into C010 repeatedly, preserving cartridge RAM. */
        const uint8_t program[] = {0xfa, 0x00, 0x02, 0xea, 0x10, 0xc0, 0xc3, 0x50, 0x01};
        memcpy(rom + 0x150, program, sizeof(program));
        rom[0x200] = 0x11;
        rom[0x143] = system == 1 ? 0x80 : 0;
        rom[0x147] = 0x09; rom[0x149] = 2; /* ROM + 8 KiB battery RAM. */
    } else {
        /* Copy WRAM[0] to SRAM[0] in a loop, making each linked slot observable. */
        const uint32_t words[] = {0xe59f000c, 0xe59f200c, 0xe5d01000, 0xe5c21000,
                                  0xeafffffc, 0x02000000, 0x0e000000};
        for (size_t i = 0; i < sizeof(words) / sizeof(words[0]); ++i)
            for (unsigned byte = 0; byte < 4; ++byte)
                rom[i * 4 + byte] = (uint8_t)(words[i] >> (byte * 8));
        rom[0xb2] = 0x96;
        memcpy(rom + 0x100, "SRAM_V", 6);
    }
    write_bytes(path, rom, size);
    free(rom);
}
static uint8_t battery_byte(cr_engine *engine, unsigned player) {
    uint8_t *bytes = NULL; size_t size = 0;
    check(cr_engine_export_battery(engine, player, &bytes, &size) && size,
          "export real player battery", engine);
    uint8_t value = bytes[0]; free(bytes); return value;
}
static void real_core_tests(const char *root, int system) {
    const char *core_ids[] = {"corvus.gb", "corvus.gbc", "corvus.gba"};
    int index = cr_registry_find(core_ids[system]);
    check(index >= 0, "official core available", NULL);
    char id[96], rom[4096];
    memset(id, 'a' + system, 64); id[64] = '-'; id[65] = (char)('0' + system); id[66] = 0;
    int n = snprintf(rom, sizeof(rom), "%s/cheat-%d.rom", root, system);
    check(n > 0 && n < (int)sizeof(rom), "synthetic path", NULL);
    make_rom(rom, system);
    cr_engine engine; cr_engine_init(&engine, root);
    check(!cr_engine_cheat_capabilities(&engine, 0) &&
          !cr_engine_validate_cheat(&engine, 0, "x") &&
          !cr_engine_replace_cheats(&engine, 0, NULL, 0), "unselected engine rejects cheats", &engine);
    check(cr_engine_select(&engine, index) && !cr_engine_cheat_capabilities(&engine, 0),
          "idle core has no cheat player", &engine);
    check(cr_engine_open(&engine, index, id, rom), "load real synthetic ROM", &engine);
    uint32_t expected = system == 2 ? CORVUS_CHEAT_FORMAT_GBA_CODEBREAKER_RAW |
        CORVUS_CHEAT_FORMAT_GBA_GAMESHARK_V1_V2 | CORVUS_CHEAT_FORMAT_GBA_ACTION_REPLAY_V3 :
        CORVUS_CHEAT_FORMAT_GB_GAME_GENIE | CORVUS_CHEAT_FORMAT_GB_GAME_SHARK;
    check((cr_engine_cheat_capabilities(&engine, 0) & expected) == expected &&
          !cr_engine_cheat_capabilities(&engine, 1), "real format capabilities and unavailable player", &engine);
    check(cr_engine_step(&engine, 0), "initialize synthetic program", &engine);
    if (system < 2) {
        uint8_t value = 0;
        const char *genie = "222-00F"; /* Replace ROM[0200] with 22. */
        check(cr_engine_read_achievement_memory(&engine, 0xc010, &value, 1) == 1 && value == 0x11 &&
              cr_engine_validate_cheat(&engine, 0, genie) &&
              cr_engine_replace_cheats(&engine, 0, &genie, 1) && cr_engine_step(&engine, 0) &&
              cr_engine_read_achievement_memory(&engine, 0xc010, &value, 1) == 1 && value == 0x22,
              "real Game Genie changes executed ROM reads", &engine);
        check(cr_engine_reset(&engine) && cr_engine_step(&engine, 0) &&
              cr_engine_read_achievement_memory(&engine, 0xc010, &value, 1) == 1 && value == 0x22 &&
              cr_engine_replace_cheats(&engine, 0, NULL, 0) && cr_engine_step(&engine, 0) &&
              cr_engine_read_achievement_memory(&engine, 0xc010, &value, 1) == 1 && value == 0x11,
              "Game Genie survives reset and disable restores ROM reads", &engine);
    } else {
        /* Standard encrypted device codes: write 66, then 77, to EWRAM[0]. */
        const char *shark = "3E0EC45E 2263E31D", *replay = "FD1E2088 49E53448";
        check(cr_engine_validate_cheat(&engine, 0, shark) &&
              cr_engine_replace_cheats(&engine, 0, &shark, 1) && cr_engine_step(&engine, 0) &&
              battery_byte(&engine, 0) == 0x66 && cr_engine_validate_cheat(&engine, 0, replay) &&
              cr_engine_replace_cheats(&engine, 0, &replay, 1) && cr_engine_step(&engine, 0) &&
              battery_byte(&engine, 0) == 0x77,
              "real GBA GameShark and Action Replay device codes", &engine);
        check(cr_engine_replace_cheats(&engine, 0, NULL, 0) && cr_engine_reset(&engine) &&
              cr_engine_step(&engine, 0), "prepare GBA baseline after format tests", &engine);
    }
    uint8_t baseline = battery_byte(&engine, 0);
    uint8_t *state = NULL; size_t state_size = 0;
    check(cr_engine_state_copy(&engine, &state, &state_size), "capture state before cheats", &engine);
    char code[32]; snprintf(code, sizeof(code), "%s", system == 2 ? "32000000 00A5" : "01A500A0");
    const char *codes[] = {code};
    size_t audio_count = engine.audio_count;
    check(cr_engine_validate_cheat(&engine, 0, code) && !cr_engine_cheat_count(&engine, 0) &&
          battery_byte(&engine, 0) == baseline && engine.audio_count == audio_count,
          "validation preserves machine, active list and output", &engine);
    check(cr_engine_replace_cheats(&engine, 0, codes, 1) && cr_engine_cheat_count(&engine, 0) == 1 &&
          battery_byte(&engine, 0) == baseline, "replacement starts on next frame", &engine);
    memset(code, 'x', strlen(code)); /* Active list must own its source strings. */
    check(cr_engine_step(&engine, 0) && battery_byte(&engine, 0) == 0xa5,
          "real cheat changes battery RAM", &engine);
#ifdef CR_TEST_WRAP_CHEAT_ALLOC
    const char *replacement = system == 2 ? "32000000 003C" : "013C00A0";
    fail_cheat_calloc = true;
    check(!cr_engine_replace_cheats(&engine, 0, &replacement, 1) && !fail_cheat_calloc &&
          cr_engine_cheat_count(&engine, 0) == 1 && battery_byte(&engine, 0) == 0xa5,
          "list allocation failure preserves provider and frontend lists", &engine);
    fail_cheat_strdup = true;
    check(!cr_engine_replace_cheats(&engine, 0, &replacement, 1) && !fail_cheat_strdup &&
          cr_engine_cheat_count(&engine, 0) == 1 && cr_engine_step(&engine, 0) &&
          battery_byte(&engine, 0) == 0xa5, "string allocation failure preserves active effect", &engine);
#endif
    const char *bad[] = {"invalid"};
    check(!cr_engine_validate_cheat(&engine, 0, bad[0]) &&
          !cr_engine_replace_cheats(&engine, 0, bad, 1) &&
          !cr_engine_replace_cheats(&engine, 1, NULL, 0) &&
          !cr_engine_replace_cheats(&engine, 0, NULL, 1) &&
          !cr_engine_replace_cheats(&engine, 0, bad, CORVUS_CHEATS_MAX_CODES + 1u) &&
          cr_engine_cheat_count(&engine, 0) == 1, "invalid input preserves active list", &engine);
    char oversized[CORVUS_CHEATS_MAX_CODE_BYTES + 2u];
    memset(oversized, 'A', sizeof(oversized)); oversized[sizeof(oversized) - 1] = 0;
    const char *too_long[] = {oversized};
    check(!cr_engine_validate_cheat(&engine, 0, oversized) &&
          !cr_engine_replace_cheats(&engine, 0, too_long, 1), "bounded code input", &engine);
    check(cr_engine_state_restore(&engine, state, state_size) && cr_engine_cheat_count(&engine, 0) == 1 &&
          cr_engine_step(&engine, 0) && battery_byte(&engine, 0) == 0xa5,
          "active cheat survives restore and invalid replacement", &engine);
    check(cr_engine_reset(&engine) && cr_engine_cheat_count(&engine, 0) == 1 &&
          cr_engine_step(&engine, 0) && battery_byte(&engine, 0) == 0xa5, "active cheat survives reset", &engine);
    uint8_t *battery = NULL; size_t size = 0;
    check(cr_engine_export_battery(&engine, 0, &battery, &size), "capture import bytes", &engine);
    battery[0] = 0x3c;
    check(cr_engine_import_battery(&engine, 0, battery, size) && cr_engine_cheat_count(&engine, 0) == 1 &&
          battery_byte(&engine, 0) == 0x3c && cr_engine_step(&engine, 0) && battery_byte(&engine, 0) == 0xa5,
          "battery candidate preserves and reapplies copied codes", &engine);
    free(battery);
    check(cr_engine_replace_cheats(&engine, 0, NULL, 0) && !cr_engine_cheat_count(&engine, 0) &&
          battery_byte(&engine, 0) == 0xa5 && cr_engine_state_restore(&engine, state, state_size) &&
          cr_engine_step(&engine, 0) && battery_byte(&engine, 0) == baseline,
          "disable stops later effects and restore does not serialize codes", &engine);
    free(state);
    const char *p1 = system == 2 ? "32000000 005A" : "015A00A0";
    check(cr_engine_replace_cheats(&engine, 0, &p1, 1) &&
          !cr_engine_open(&engine, index, id, "/missing.rom") && cr_engine_cheat_count(&engine, 0) == 1,
          "failed new content preserves old cheats", &engine);
    check(cr_engine_open(&engine, index, id, rom) && !cr_engine_cheat_count(&engine, 0),
          "successful new content clears active list", &engine);
    const char *ids[] = {id, id}, *paths[] = {rom, rom};
    check(cr_engine_open_subsystem(&engine, index, "corvus.link.two_player", ids, paths),
          "open real two-player link", &engine);
    const char *p2 = system == 2 ? "32000000 00C3" : "01C300A0";
    check(cr_engine_replace_cheats(&engine, 0, &p1, 1) && cr_engine_replace_cheats(&engine, 1, &p2, 1) &&
          cr_engine_step(&engine, 0) && battery_byte(&engine, 0) == 0x5a && battery_byte(&engine, 1) == 0xc3,
          "linked players have independent real code effects", &engine);
    check(cr_engine_export_battery(&engine, 0, &battery, &size), "capture paired import", &engine);
    battery[0] = 0x71;
    check(cr_engine_import_battery(&engine, 0, battery, size) &&
          cr_engine_cheat_count(&engine, 0) == 1 && cr_engine_cheat_count(&engine, 1) == 1 &&
          cr_engine_step(&engine, 0) && battery_byte(&engine, 0) == 0x5a && battery_byte(&engine, 1) == 0xc3,
          "paired battery candidate preserves each player's list", &engine);
    free(battery);
    check(!cr_engine_replace_cheats(&engine, 1, bad, 1) &&
          cr_engine_replace_cheats(&engine, 0, NULL, 0) &&
          cr_engine_cheat_count(&engine, 0) == 0 && cr_engine_cheat_count(&engine, 1) == 1 &&
          cr_engine_reset(&engine) && cr_engine_step(&engine, 0) && battery_byte(&engine, 1) == 0xc3,
          "one player's disable or invalid input preserves the other", &engine);
    check(cr_engine_unload_game(&engine) && !cr_engine_cheat_count(&engine, 1) &&
          !cr_engine_cheat_capabilities(&engine, 0), "unload clears all lists", &engine);
    cr_engine_destroy(&engine);
}
static void protocol_tests(const char *root, int argc, char **argv) {
    char rom[4096], id[96];
    int n = snprintf(rom, sizeof(rom), "%s/protocol.rom", root);
    check(n > 0 && n < (int)sizeof(rom), "protocol path", NULL);
    uint8_t seed = 1; write_bytes(rom, &seed, 1);
    memset(id, 'f', 64); id[64] = 0;
    char hash[65]; memcpy(hash, id, sizeof(hash));
    check(cr_registry_content_id(id, sizeof(id), hash, "test.configuration"), "fixture content id", NULL);
    for (int i = 1; i < argc; ++i) {
        const char *module = argv[i]; cr_registry_add_installed_modules(&module, 1);
        int index = cr_registry_find("test.configuration");
        cr_engine engine; cr_engine_init(&engine, root);
        check(index >= 0 && cr_engine_open(&engine, index, id, rom), "legacy/protocol fixture remains playable", &engine);
        check(!cr_engine_cheat_capabilities(&engine, 0) &&
              !cr_engine_validate_cheat(&engine, 0, "valid") &&
              !cr_engine_replace_cheats(&engine, 0, NULL, 0) &&
              cr_engine_step(&engine, 0), "absent or malformed extension disabled safely", &engine);
        cr_engine_destroy(&engine);
    }
}
int main(int argc, char **argv) {
    char root[] = "/tmp/corvus-engine-cheats-XXXXXX";
    check(mkdtemp(root) != NULL, "temporary cheat test storage", NULL);
    for (int system = 0; system < 3; ++system) real_core_tests(root, system);
    protocol_tests(root, argc, argv);
    puts("native engine cheats passed");
    return 0;
}
#endif
