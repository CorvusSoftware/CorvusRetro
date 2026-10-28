// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
#define _POSIX_C_SOURCE 200809L
#include "engine.h"
#include "core_registry.h"
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#ifdef CR_TEST_WRAP_FSYNC
static unsigned fail_sync_after, fail_sync_count;
int __real_fsync(int fd);
int __wrap_fsync(int fd) {
    if (fail_sync_after && !--fail_sync_after) {
        if (fail_sync_count > 1) { --fail_sync_count; fail_sync_after = 1; }
        else fail_sync_count = 0;
        errno = EIO; return -1;
    }
    return __real_fsync(fd);
}
#endif

static void check(bool ok, const char *message, const cr_engine *engine) {
    if (ok) return;
    fprintf(stderr, "FAIL %s: %s\n", message, engine ? engine->error : ""); exit(1);
}
static void write_bytes(const char *path, const void *data, size_t size) {
    FILE *file = fopen(path, "wb");
    check(file && fwrite(data, 1, size, file) == size && !fclose(file), "write fixture", NULL);
}
static size_t read_bytes(const char *path, char *out, size_t capacity) {
    FILE *file = fopen(path, "rb"); check(file != NULL, "read fixture", NULL);
    size_t n = fread(out, 1, capacity, file);
    check(!ferror(file) && !fclose(file), "read fixture bytes", NULL); return n;
}
static void copy_module(const char *source, const char *destination) {
    FILE *input = fopen(source, "rb"), *output = fopen(destination, "wb");
    check(input && output, "open module replacement fixture", NULL);
    unsigned char bytes[4096];
    size_t count;
    while ((count = fread(bytes, 1, sizeof(bytes), input)) != 0)
        check(fwrite(bytes, 1, count, output) == count, "copy module replacement fixture", NULL);
    check(!ferror(input) && !fclose(input) && !fclose(output), "finish module replacement fixture", NULL);
}
static void path(char out[4096], const char *root, const char *tail) {
    int n = snprintf(out, 4096, "%s/%s", root, tail);
    check(n > 0 && n < 4096, "bounded path", NULL);
}
static void register_module(const char *module) { cr_registry_add_installed_modules(&module, 1); }
static void content_id(char out[96], char digit, const char *core) {
    char hash[65]; memset(hash, digit, 64); hash[64] = 0;
    check(cr_registry_content_id(out, 96, hash, core), "content identity", NULL);
}
static void fixture_tests(const char *modules[4]) {
    char root[] = "/tmp/corvus-engine-config-XXXXXX";
    check(mkdtemp(root) != NULL, "temporary storage", NULL);
    char roms[2][4096], ids_storage[2][96], config[4096], block[4096];
    const char *ids[2] = {ids_storage[0], ids_storage[1]}, *paths[2] = {roms[0], roms[1]};
    for (unsigned i = 0; i < 2; ++i) {
        content_id(ids_storage[i], (char)('a' + i), "test.configuration");
        path(roms[i], root, i ? "second.rom" : "first.rom");
        uint8_t seed = (uint8_t)(20 + i); write_bytes(roms[i], &seed, 1);
    }
    path(config, root, "options/test.configuration.cfg");
    path(block, root, "options/test.configuration.cfg.new");
    register_module(modules[0]); int system = cr_registry_find("test.configuration");
    check(system >= 0, "configuration fixture registered", NULL);
    cr_engine e; cr_engine_init(&e, root);
    check(cr_engine_select(&e, system) && cr_engine_option_count(&e) == 1 &&
          cr_engine_option_value(&e, "accuracy") == 0, "options negotiated with default", &e);
    corvus_core_option option;
    check(cr_engine_option(&e, 0, &option) && !cr_engine_option(&e, 1, &option), "bounded option metadata", &e);
    check(cr_engine_set_option(&e, "accuracy", 1) && !cr_engine_set_option(&e, "accuracy", 2),
          "set valid value only", &e);
    check(cr_engine_open(&e, system, ids[0], paths[0]) && cr_engine_option_value(&e, "accuracy") == 1,
          "pending option survives fresh game candidate", &e);
    check(cr_engine_can_reset(&e) && cr_engine_can_state(&e), "single session capabilities", &e);
    check(cr_engine_unload_game(&e) && e.handle && !e.players && cr_engine_option_value(&e, "accuracy") == 1,
          "unload keeps options and selected module", &e);
    check(!mkdir(block, 0700), "block atomic config write", &e);
    check(!cr_engine_set_option(&e, "accuracy", 0) && cr_engine_option_value(&e, "accuracy") == 1,
          "failed durable write rolls back selected value", &e);
    check(!rmdir(block), "clear temporary-write obstruction", &e);
    char backup[4096]; path(backup, root, "options/test.configuration.cfg.bak");
    check(!mkdir(backup, 0700), "block legacy backup cleanup", &e);
    check(!cr_engine_set_option(&e, "accuracy", 0) && cr_engine_option_value(&e, "accuracy") == 1,
          "backup obstruction rejects new choice before config publication", &e);
    check(!rmdir(backup) && cr_engine_close(&e) && cr_engine_select(&e, system) &&
          cr_engine_option_value(&e, "accuracy") == 1,
          "restart after failed backup cleanup retains previous durable choice", &e);
    check(cr_engine_close(&e), "close after backup failure recovery", &e);
#ifdef CR_TEST_WRAP_FSYNC
    for (unsigned sync = 3; sync <= 4; ++sync) {
        check(cr_engine_select(&e, system), "select before injected commit sync failure", &e);
        fail_sync_after = sync;
        check(!cr_engine_set_option(&e, "accuracy", 0) && cr_engine_option_value(&e, "accuracy") == 1,
              "post-publication fsync failure restores old runtime choice", &e);
        check(!fail_sync_after && cr_engine_close(&e) && cr_engine_select(&e, system) &&
              cr_engine_option_value(&e, "accuracy") == 1,
              "restart after commit or backup-cleanup sync failure restores old disk choice", &e);
        check(cr_engine_close(&e), "close sync failure recovery", &e);
    }
    check(cr_engine_select(&e, system), "select before recovery sync failure", &e);
    fail_sync_after = 3; fail_sync_count = 2;
    check(!cr_engine_set_option(&e, "accuracy", 0) && cr_engine_option_value(&e, "accuracy") == 1 &&
          strstr(e.error, "non garantie"), "failed recovery sync reports uncertain durability explicitly", &e);
    check(cr_engine_close(&e) && cr_engine_select(&e, system) && cr_engine_option_value(&e, "accuracy") == 1,
          "restart recovers retained previous config after recovery sync failure", &e);
    check(cr_engine_close(&e), "close uncertain durability recovery", &e);
    char first_root[] = "/tmp/corvus-options-first-commit-XXXXXX";
    check(mkdtemp(first_root) != NULL, "first config commit storage", &e);
    cr_engine fresh; cr_engine_init(&fresh, first_root);
    check(cr_engine_select(&fresh, system), "select before first config failure", &fresh);
    fail_sync_after = 3;
    check(!cr_engine_set_option(&fresh, "accuracy", 1) && cr_engine_option_value(&fresh, "accuracy") == 0 &&
          cr_engine_close(&fresh) && cr_engine_select(&fresh, system) &&
          cr_engine_option_value(&fresh, "accuracy") == 0, "failed first commit restores prior config absence across restart", &fresh);
    cr_engine_destroy(&fresh);
#endif
    const char previous_config[] = "CORVUS_OPTIONS_1\naccuracy=accurate\n";
    const char interrupted_config[] = "CORVUS_OPTIONS_1\naccuracy=fast\n";
    write_bytes(backup, previous_config, sizeof(previous_config) - 1);
    write_bytes(config, interrupted_config, sizeof(interrupted_config) - 1);
    check(cr_engine_select(&e, system) && cr_engine_option_value(&e, "accuracy") == 1 &&
          access(backup, F_OK) != 0, "startup recovers backup from interrupted unacknowledged config commit", &e);
    void *old_handle = e.handle;
    register_module(modules[1]);
    corvus_core_info loaded_info;
    check(e.handle == old_handle && !strcmp(e.module_path, modules[0]) &&
          cr_engine_core_info(&e, &loaded_info) && !strcmp(loaded_info.version, "1") &&
          !strcmp(cr_registry_get(system)->version, "2") &&
          cr_engine_option(&e, 0, &option) && !strcmp(option.values[0].id, "fast") &&
          cr_engine_option_value(&e, "accuracy") == 1,
          "registry refresh retains selected old module until explicit selection", &e);
    check(cr_engine_select(&e, system) && cr_engine_option_value(&e, "accuracy") == 0,
          "same-index idle reselect opens updated module and restores stable value ID", &e);
    check(e.handle != old_handle && !strcmp(e.module_path, modules[1]) &&
          cr_engine_core_info(&e, &loaded_info) && !strcmp(loaded_info.version, "2"),
          "updated module replaces idle handle and loaded metadata", &e);
    old_handle = e.handle;
    check(cr_engine_select(&e, system) && e.handle == old_handle,
          "unchanged idle selection retains its current handle", &e);
    char missing_module[4096]; path(missing_module, root, "removed-module.so");
    copy_module(modules[0], missing_module); register_module(missing_module);
    check(!unlink(missing_module) && !cr_engine_select(&e, system) && e.handle == old_handle &&
          !strcmp(e.module_path, modules[1]) && cr_engine_option_value(&e, "accuracy") == 0,
          "failed updated-module selection preserves current module and options", &e);
    register_module(modules[1]);
    check(cr_engine_close(&e), "close reordered core", &e);
    const char unknown[] = "CORVUS_OPTIONS_1\nfuture.option=retained\naccuracy=accurate\n";
    write_bytes(config, unknown, sizeof(unknown) - 1);
    check(cr_engine_select(&e, system) && cr_engine_set_option(&e, "accuracy", 1), "load unknown saved option", &e);
    char contents[1024] = {0}; size_t n = read_bytes(config, contents, sizeof(contents) - 1); contents[n] = 0;
    check(strstr(contents, "future.option=retained\n") != NULL, "preserve unavailable keys on save", &e);
    check(cr_engine_close(&e), "close config", &e);
    const char corrupt[] = "CORVUS_OPTIONS_1\naccuracy=accurate\naccuracy=fast\n";
    write_bytes(config, corrupt, sizeof(corrupt) - 1);
    check(cr_engine_select(&e, system) && cr_engine_option_value(&e, "accuracy") == 1,
          "corrupt duplicate config recovers bounded defaults", &e);
    check(cr_engine_subsystem_count(&e) == 1 && cr_engine_open_subsystem(&e, system, "link", ids, paths),
          "load both local contents", &e);
    void *active = e.handle;
    check(e.players == 2 && e.width == 2 && cr_engine_can_reset(&e) && !cr_engine_can_state(&e),
          "combined output and paired session capabilities", &e);
    unsigned masks[2] = {0, 1u << 4};
    check(cr_engine_step_players(&e, masks) && e.pixels[0] == 0xff000000u &&
          e.pixels[1] == 0xff000001u && e.audio_count == 2, "both player inputs and shared audio", &e);
    check(cr_engine_step(&e, 0) && e.pixels[1] == 0xff000000u, "single wrapper releases stale P2 input", &e);
    uint8_t byte;
    check(!cr_engine_read_achievement_memory(&e, 0, &byte, 1) && !cr_engine_save_state(&e) &&
          !cr_engine_load_state(&e), "paired states and achievements unavailable", &e);
    char state_dir[4096]; path(state_dir, root, "states");
    check(access(state_dir, F_OK) != 0, "unsupported paired states do not touch files", &e);
    char p2[4096], p2new[4096];
    int length = snprintf(p2, sizeof(p2), "%s/saves/%s-link-p2.sav", root, ids[1]);
    check(length > 0 && length < (int)sizeof(p2), "P2 path", &e);
    length = snprintf(p2new, sizeof(p2new), "%s.new", p2);
    check(length > 0 && length < (int)sizeof(p2new) && !mkdir(p2new, 0700), "block P2 write", &e);
    check(!cr_engine_open(&e, system, ids[0], paths[0]) && e.handle == active && e.players == 2 &&
          e.subsystems.battery_dirty(e.handle, 0) && e.subsystems.battery_dirty(e.handle, 1),
          "P2 save failure preserves session and both generations", &e);
    check(!cr_engine_unload_game(&e) && e.handle == active, "failed close keeps paired core", &e);
    check(!rmdir(p2new) && cr_engine_save_battery(&e), "retry persists both battery generations", &e);
    char p1[4096]; length = snprintf(p1, sizeof(p1), "%s/saves/%s.sav", root, ids[0]);
    check(length > 0 && length < (int)sizeof(p1), "P1 legacy path", &e);
    char first[2], second[2];
    check(read_bytes(p1, first, 2) == 2 && read_bytes(p2, second, 2) == 2 &&
          first[0] == 20 && second[0] == 21 && first[1] == 2 && second[1] == 3,
          "independent persisted player batteries", &e);
    const char *bad_paths[2] = {paths[0], "/missing/second.rom"};
    check(!cr_engine_open_subsystem(&e, system, "link", ids, bad_paths) && e.handle == active,
          "invalid second ROM leaves active session intact", &e);
    const char *same_ids[2] = {ids[0], ids[0]}, *same_paths[2] = {paths[0], paths[0]};
    check(cr_engine_open_subsystem(&e, system, "link", same_ids, same_paths) && cr_engine_step_players(&e, masks) &&
          cr_engine_save_battery(&e), "same ROM twice uses independent slots", &e);
    length = snprintf(p2, sizeof(p2), "%s/saves/%s-link-p2.sav", root, ids[0]);
    check(length > 0 && length < (int)sizeof(p2) && read_bytes(p1, first, 2) == 2 &&
          read_bytes(p2, second, 2) == 2 && first[1] != second[1], "same-ROM P2 has dedicated save path", &e);
    check(cr_engine_unload_game(&e) && e.handle && !e.players && !e.second_id[0] && !e.subsystem_id[0] &&
          cr_engine_option_count(&e) == 1 && cr_engine_close(&e), "close linked content retains selected core", &e);
    register_module(modules[2]);
    check(cr_engine_select(&e, system) && !cr_engine_option_count(&e) && !cr_engine_subsystem_count(&e) &&
          cr_engine_open(&e, system, ids[0], paths[0]) && cr_engine_can_state(&e) && cr_engine_can_reset(&e),
          "legacy module falls back to base operations", &e);
    check(cr_engine_close(&e), "close legacy", &e);
    register_module(modules[3]);
    check(cr_engine_select(&e, system) && !cr_engine_option_count(&e) && !cr_engine_subsystem_count(&e) &&
          cr_engine_open(&e, system, ids[0], paths[0]), "malformed counts and partial group disabled safely", &e);
    cr_engine_destroy(&e);
}

static void state_tests(const char *module) {
    register_module(module);
    int system = cr_registry_find("test.configuration");
    char root[] = "/tmp/corvus-session-states-XXXXXX";
    check(mkdtemp(root) != NULL, "session state storage", NULL);
    char roms[3][4096], ids_storage[3][96];
    for (unsigned i = 0; i < 3; ++i) {
        content_id(ids_storage[i], (char)('a' + i), "test.configuration");
        char name[16]; snprintf(name, sizeof(name), "slot%u.rom", i); path(roms[i], root, name);
        uint8_t seed = (uint8_t)(30 + i); write_bytes(roms[i], &seed, 1);
    }
    cr_engine e; cr_engine_init(&e, root);
    check(cr_engine_open(&e, system, ids_storage[0], roms[0]) && cr_engine_step(&e, 0) &&
          cr_engine_save_state(&e), "save normal session state", &e);
    char solo_path[4096];
    int length = snprintf(solo_path, sizeof(solo_path), "%s/states/%s-" CR_CORE_VERSION ".state", root, ids_storage[0]);
    check(length > 0 && length < (int)sizeof(solo_path), "original solo state filename", &e);
    char solo[2]; check(read_bytes(solo_path, solo, sizeof(solo)) == 2, "solo state at unchanged historical path", &e);
    const char *ids[2] = {ids_storage[0], ids_storage[1]}, *paths[2] = {roms[0], roms[1]};
    uint8_t saved[3][4];
    for (unsigned session = 0; session < 3; ++session) {
        ids[1] = ids_storage[session == 1 ? 2 : 1]; paths[1] = roms[session == 1 ? 2 : 1];
        const char *subsystem = session == 2 ? "link.alternate" : "link";
        check(cr_engine_open_subsystem(&e, system, subsystem, ids, paths) && cr_engine_can_state(&e),
              "generic third-party pair advertises complete states", &e);
        for (unsigned frame = 0; frame <= session; ++frame) check(cr_engine_step(&e, 0), "advance distinct paired state", &e);
        check(e.api.serialize(e.handle, saved[session], sizeof(saved[session])) && cr_engine_save_state(&e),
              "save namespaced paired state", &e);
        char unchanged[2];
        check(read_bytes(solo_path, unchanged, sizeof(unchanged)) == 2 && !memcmp(unchanged, solo, 2),
              "paired state never overwrites first player's solo state", &e);
    }
    for (unsigned session = 0; session < 3; ++session) {
        ids[1] = ids_storage[session == 1 ? 2 : 1]; paths[1] = roms[session == 1 ? 2 : 1];
        const char *subsystem = session == 2 ? "link.alternate" : "link";
        check(cr_engine_open_subsystem(&e, system, subsystem, ids, paths) && cr_engine_load_state(&e),
              "load pair-specific and subsystem-specific state", &e);
        uint8_t actual[4];
        check(e.api.serialize(e.handle, actual, sizeof(actual)) && !memcmp(actual, saved[session], sizeof(actual)),
              "paired states isolated across different partners and subsystems", &e);
    }
    check(cr_engine_open(&e, system, ids_storage[0], roms[0]) && cr_engine_load_state(&e), "reload historical solo state", &e);
    uint8_t actual[2];
    check(e.api.serialize(e.handle, actual, sizeof(actual)) && !memcmp(actual, solo, sizeof(actual)),
          "normal state remains isolated from all paired sessions", &e);
    cr_engine_destroy(&e);
}

static void real_engine_tests(void) {
    const char *cores[] = {"corvus.gb", "corvus.gbc", "corvus.gba"};
    for (unsigned s = 0; s < 3; ++s) {
        int system = cr_registry_find(cores[s]);
        if (system < 0) continue;
        char root[] = "/tmp/corvus-engine-real-config-XXXXXX";
        check(mkdtemp(root) != NULL, "real engine storage", NULL);
        cr_engine e; cr_engine_init(&e, root);
        check(cr_engine_select(&e, system) && cr_engine_option_count(&e) >= 2, "real public core options", &e);
        corvus_core_option option;
        check(cr_engine_option(&e, 0, &option) && option.value_count >= 2, "real option metadata", &e);
        char key[96]; snprintf(key, sizeof(key), "%s", option.key);
        uint32_t selected = s == 2 ? 2 : 1;
        check(cr_engine_set_option(&e, key, selected) && cr_engine_close(&e) && cr_engine_select(&e, system) &&
              cr_engine_option_value(&e, key) == selected, "real option persists across process-style recreation", &e);
        corvus_subsystem_info info;
        check(cr_engine_subsystem_count(&e) && cr_engine_subsystem(&e, 0, &info), "real link metadata", &e);
        char subsystem[96]; snprintf(subsystem, sizeof(subsystem), "%s", info.id);
        char rom[4096], ids_storage[2][96]; path(rom, root, "original.rom");
        size_t size = s == 2 ? 512 : 32768;
        uint8_t *bytes = calloc(size, 1); check(bytes != NULL, "original ROM", &e);
        if (s < 2) {
            bytes[0x100] = 0xc3; bytes[0x101] = 0x50; bytes[0x102] = 1;
            bytes[0x147] = 3; bytes[0x149] = 2;
            const uint8_t program[] = {0x3e,0x0a,0xea,0,0,0x3e,0x10,0xe0,0,
                0xf0,0,0xe6,0x0f,0xea,1,0xa0,0x18,0xf7};
            memcpy(bytes + 0x150, program, sizeof(program));
        } else {
            const uint32_t program[] = {0xe59f000c, 0xe59f100c, 0xe1d120b0, 0xe5c02001,
                0xeafffffc, 0x0e000000, 0x04000130};
            for (size_t i = 0; i < sizeof(program) / sizeof(program[0]); ++i)
                for (unsigned b = 0; b < 4; ++b) bytes[i * 4 + b] = (uint8_t)(program[i] >> (b * 8));
            bytes[0xb2] = 0x96; memcpy(bytes + 0x100, "SRAM_V", 6);
        }
        write_bytes(rom, bytes, size); free(bytes);
        content_id(ids_storage[0], 'c', cores[s]); content_id(ids_storage[1], 'd', cores[s]);
        const char *ids[2] = {ids_storage[0], ids_storage[1]}, *paths[2] = {rom, rom};
        char saves[4096]; path(saves, root, "saves");
        check(!mkdir(saves, 0700), "real battery directory", &e);
        size_t battery_size = s == 2 ? 32768 : 8192;
        uint8_t *battery = malloc(battery_size); check(battery != NULL, "real battery fixture", &e);
        for (unsigned slot = 0; slot < 2; ++slot) {
            char saved[4096];
            int length = snprintf(saved, sizeof(saved), "%s/%s%s", saves, ids[slot],
                                  slot ? "-link-p2.sav" : ".sav");
            check(length > 0 && length < (int)sizeof(saved), "real slot save path", &e);
            memset(battery, slot ? 0x72 : 0x31, battery_size); write_bytes(saved, battery, battery_size);
        }
        check(cr_engine_open_subsystem(&e, system, subsystem, ids, paths) && e.players == 2 &&
              e.width == (s == 2 ? 480 : 320) && e.height == (s == 2 ? 160 : 144) &&
              cr_engine_option_value(&e, key) == selected && cr_engine_step(&e, 0) && e.audio_count > 0,
              "real linked frame and options on fresh candidate", &e);
        if (s < 2) check(e.pixels[0] == 0xffffffffu, "real selected grayscale changes video", &e);
        unsigned player_masks[2] = {0, 1u << 4};
        check(cr_engine_step_players(&e, player_masks), "real distinct player inputs", &e);
        for (unsigned slot = 0; slot < 2; ++slot) {
            uint64_t generation = 0;
            check(e.subsystems.battery_size(e.handle, slot) == battery_size &&
                  e.subsystems.battery_copy(e.handle, slot, battery, battery_size, &generation) &&
                  battery[s == 2 ? 20 : 0] == (slot ? 0x72 : 0x31),
                  "real slots load independent frontend SRAM files", &e);
            check(battery[1] == (s == 2 ? (slot ? 0xfe : 0xff) : (slot ? 0x0e : 0x0f)),
                  "real ROM reads independent player buttons into SRAM", &e);
        }
        check(cr_engine_step(&e, 0), "real P2 stale input release", &e);
        uint64_t generation = 0;
        check(e.subsystems.battery_copy(e.handle, 1, battery, battery_size, &generation) &&
              battery[1] == (s == 2 ? 0xff : 0x0f), "real P2 receives released buttons", &e);
        free(battery);
        char p2new[4096];
        int length = snprintf(p2new, sizeof(p2new), "%s/%s-link-p2.sav.new", saves, ids[1]);
        check(length > 0 && length < (int)sizeof(p2new) && !mkdir(p2new, 0700), "block real P2 persistence", &e);
        void *active = e.handle;
        check(!cr_engine_open(&e, system, ids[0], paths[0]) && e.handle == active && e.players == 2 &&
              e.subsystems.battery_dirty(e.handle, 0) && e.subsystems.battery_dirty(e.handle, 1),
              "real P2 write failure preserves old session and dirty generations", &e);
        check(!rmdir(p2new) && cr_engine_save_battery(&e), "real battery retry", &e);
        check(cr_engine_can_reset(&e) && !cr_engine_can_state(&e) && cr_engine_reset(&e), "real linked capability gates", &e);
        const char *same_ids[2] = {ids[0], ids[0]};
        check(cr_engine_open_subsystem(&e, system, subsystem, same_ids, paths) &&
              cr_engine_step_players(&e, player_masks) && cr_engine_save_battery(&e),
              "real same-ROM same-identity players save independently", &e);
        char p1save[4096], p2save[4096];
        length = snprintf(p1save, sizeof(p1save), "%s/%s.sav", saves, ids[0]);
        check(length > 0 && length < (int)sizeof(p1save), "real P1 original identity", &e);
        length = snprintf(p2save, sizeof(p2save), "%s/%s-link-p2.sav", saves, ids[0]);
        check(length > 0 && length < (int)sizeof(p2save), "real P2 separate identity", &e);
        char p1bytes[2], p2bytes[2];
        check(read_bytes(p1save, p1bytes, 2) == 2 && read_bytes(p2save, p2bytes, 2) == 2 &&
              p1bytes[1] != p2bytes[1], "real same-ROM persisted slot bytes differ", &e);
        check(cr_engine_unload_game(&e) && e.handle && cr_engine_option_value(&e, key) == selected,
              "real unload keeps selected options", &e);
        cr_engine_destroy(&e);
    }
}
int main(int argc, char **argv) {
    check(argc == 6, "five ABI fixture modules", NULL);
    real_engine_tests();
    fixture_tests((const char **)&argv[1]);
    state_tests(argv[5]);
    puts("engine configuration and sessions passed"); return 0;
}
