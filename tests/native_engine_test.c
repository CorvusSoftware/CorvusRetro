// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
#define _POSIX_C_SOURCE 200809L
#include "engine.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static void check(bool condition, const char *message, const cr_engine *engine) {
    if (condition)
        return;
    fprintf(stderr, "FAIL %s: %s\n", message, engine ? engine->error : "");
    exit(1);
}
static void write_bytes(const char *path, const void *bytes, size_t size) {
    FILE *file = fopen(path, "wb");
    check(file != NULL, "create fixture", NULL);
    check(fwrite(bytes, 1, size, file) == size, "write fixture", NULL);
    check(fclose(file) == 0, "close fixture", NULL);
}
static uint8_t *read_bytes(const char *path, size_t *size) {
    FILE *file = fopen(path, "rb");
    check(file != NULL && fseek(file, 0, SEEK_END) == 0, "open persisted data", NULL);
    long length = ftell(file);
    check(length >= 0 && fseek(file, 0, SEEK_SET) == 0, "measure persisted data", NULL);
    *size = (size_t)length;
    uint8_t *bytes = malloc(*size ? *size : 1);
    check(bytes != NULL && fread(bytes, 1, *size, file) == *size, "read persisted data", NULL);
    check(fclose(file) == 0, "close persisted data", NULL);
    return bytes;
}
static void make_rom(const char *path, int system) {
    size_t size = system == 2 ? 512 : 32768;
    uint8_t *bytes = calloc(size, 1);
    check(bytes != NULL, "allocate original program", NULL);
    if (system < 2) {
        bytes[0x100] = 0xc3;
        bytes[0x102] = 1;
        bytes[0x143] = system == 1 ? 0x80 : 0;
    } else {
        const uint32_t words[] = {0xe59f0008, 0xe3a0105a, 0xe5c01000, 0xeafffffe, 0x0e000000};
        for (size_t i = 0; i < sizeof(words) / sizeof(words[0]); ++i)
            for (unsigned b = 0; b < 4; ++b)
                bytes[i * 4 + b] = (uint8_t)(words[i] >> (b * 8));
        bytes[0xb2] = 0x96;
        memcpy(bytes + 0x100, "SRAM_V", 6);
    }
    write_bytes(path, bytes, size);
    free(bytes);
}
static uint8_t *machine_state(cr_engine *engine, size_t *size) {
    *size = engine->api.serialize_size(engine->handle);
    check(*size > 0, "state size", engine);
    uint8_t *bytes = malloc(*size);
    check(bytes && engine->api.serialize(engine->handle, bytes, *size), "capture state", engine);
    return bytes;
}
static void assert_state(cr_engine *engine, const uint8_t *expected, size_t size,
                         const char *message) {
    size_t actual_size;
    uint8_t *actual = machine_state(engine, &actual_size);
    check(actual_size == size && memcmp(expected, actual, size) == 0, message, engine);
    free(actual);
}
static void path(char out[4096], const char *root, const char *folder, const char *id,
                 const char *suffix) {
    int count = snprintf(out, 4096, "%s/%s/%s%s", root, folder, id, suffix);
    check(count > 0 && count < 4096, "test path", NULL);
}
int main(void) {
    /* Verify actual core metadata through CorvusAPI after loading the module. */
    char root[] = "/tmp/corvus-native-engine-XXXXXX";
    check(mkdtemp(root) != NULL, "temporary storage", NULL);
    char ids[3][68], roms[3][4096];
    for (int system = 0; system < 3; ++system) {
        memset(ids[system], 'a' + system, 64);
        ids[system][64] = '-';
        ids[system][65] = (char)('0' + system);
        ids[system][66] = '\0';
        snprintf(roms[system], sizeof(roms[system]), "%s/original-%d.rom", root, system);
        make_rom(roms[system], system);
        check(cr_game_id_valid(ids[system]), "valid hashed ID", NULL);
    }
    check(!cr_game_id_valid(NULL) && !cr_game_id_valid("../unsafe") && !cr_game_id_valid("A"),
          "reject unsafe IDs", NULL);
    cr_engine engine;
    cr_engine_init(&engine, root);
    check(!engine.handle && engine.system == -1, "start unloaded", &engine);
    check(!cr_engine_step(&engine, 0) && !cr_engine_save_state(&engine), "idle operations rejected",
          &engine);
    check(!cr_engine_select(&engine, 3) && !engine.handle, "unknown core rejected", &engine);
    for (int system = 0; system < 3; ++system) {
        check(cr_engine_select(&engine, system), "load idle core", &engine);
        check(engine.handle && engine.system == system && !engine.loaded, "real idle handle",
              &engine);
        corvus_core_info info = {.struct_size = sizeof(info)};
        check(engine.api.get_info(&info) && info.version &&
                  strcmp(info.version, CR_CORE_VERSION) == 0,
              "pinned core version via CorvusAPI", &engine);
        check(!cr_engine_step(&engine, 0), "idle frame rejected", &engine);
        check(cr_engine_open(&engine, system, ids[system], roms[system]), "load content", &engine);
        check(engine.loaded && engine.width == (system == 2 ? 240 : 160) &&
                  engine.height == (system == 2 ? 160 : 144),
              "dimensions", &engine);
        check(engine.rate > 0 && engine.fps > 50 && engine.fps < 65, "timing", &engine);
        check(cr_engine_step(&engine, 0), "native frame", &engine);
        check(engine.pixels && engine.pixels[0] >> 24 == 255, "ARGB pixels", &engine);
        check(engine.audio_count > 0 && engine.audio_count <= 4096 && engine.audio_count % 2 == 0,
              "stereo PCM", &engine);
        check(!cr_engine_step(&engine, 1u << 10), "invalid input mask", &engine);
        if (system == 2) {
            uint64_t generation = 0;
            size_t size = engine.api.battery_size(engine.handle);
            uint8_t *stale = malloc(size);
            check(stale && size > 0, "allocate stale battery", &engine);
            memset(stale, 0x33, size);
            char battery_path[4096];
            path(battery_path, root, "saves", ids[system], ".sav");
            write_bytes(battery_path, stale, size);
            check(cr_engine_open(&engine, system, ids[system], roms[system]),
                  "reopen current game with dirty battery", &engine);
            check(engine.api.battery_copy(engine.handle, stale, size, &generation) && stale[0] == 0x5a,
                  "reopen must load latest in-memory battery", &engine);
            free(stale);
        }
        check(cr_engine_step(&engine, 16), "native input", &engine);
        size_t state_size;
        uint8_t *state = machine_state(&engine, &state_size);
        check(cr_engine_save_state(&engine), "atomic state write", &engine);
        char state_path[4096], backup_path[4096];
        path(state_path, root, "states", ids[system], "-" CR_CORE_VERSION ".state");
        path(backup_path, root, "states", ids[system], "-" CR_CORE_VERSION ".state.bak");
        size_t disk_size;
        uint8_t *disk_state = read_bytes(state_path, &disk_size);
        check(disk_size == state_size && memcmp(disk_state, state, state_size) == 0,
              "persisted state bytes", &engine);
        free(disk_state);
        check(cr_engine_step(&engine, 0) && cr_engine_load_state(&engine), "restore state",
              &engine);
        assert_state(&engine, state, state_size, "exact state roundtrip");
        void *current = engine.handle;
        check(!cr_engine_open(&engine, system, "../unsafe", roms[system]), "unsafe content ID",
              &engine);
        check(engine.handle == current, "invalid ID retains current engine", &engine);
        char invalid_path[4096];
        snprintf(invalid_path, sizeof(invalid_path), "%s/invalid.rom", root);
        write_bytes(invalid_path, "invalid", 7);
        check(!cr_engine_open(&engine, system, ids[system], invalid_path), "malformed ROM rejected",
              &engine);
        check(!cr_engine_check(&engine, system, invalid_path), "import validation rejected",
              &engine);
        check(engine.handle == current && engine.loaded, "malformed ROM retains current engine",
              &engine);
        assert_state(&engine, state, state_size, "invalid content retains machine state");
        write_bytes(state_path, "invalid", 7);
        check(!cr_engine_load_state(&engine), "invalid state rejected", &engine);
        assert_state(&engine, state, state_size, "invalid state retains machine state");
        write_bytes(state_path, state, state_size);
        check(rename(state_path, backup_path) == 0, "simulate Android AtomicFile backup", &engine);
        write_bytes(state_path, "broken", 6);
        check(cr_engine_step(&engine, 0) && cr_engine_load_state(&engine), "recover legacy backup",
              &engine);
        assert_state(&engine, state, state_size, "legacy backup restores state");
        check(access(backup_path, F_OK) != 0, "backup consumed after recovery", &engine);
        free(state);
        check(cr_engine_reset(&engine) && cr_engine_step(&engine, 0), "reset", &engine);
        if (system == 2) {
            char new_path[4096];
            path(new_path, root, "saves", ids[system], ".sav.new");
            check(mkdir(new_path, 0700) == 0, "inject atomic writer failure", &engine);
            check(engine.api.battery_dirty(engine.handle), "GBA battery dirty", &engine);
            current = engine.handle;
            check(!cr_engine_select(&engine, 0), "failed save prevents replacement", &engine);
            check(!cr_engine_open(&engine, 0, ids[0], roms[0]),
                  "failed save prevents content replacement", &engine);
            check(!cr_engine_close(&engine), "failed save prevents unload", &engine);
            check(engine.handle == current && engine.loaded && engine.system == 2,
                  "failure retains current owner", &engine);
            check(engine.api.battery_dirty(engine.handle), "failed save not acknowledged", &engine);
            check(rmdir(new_path) == 0 && cr_engine_save_battery(&engine), "retry battery write",
                  &engine);
            check(!engine.api.battery_dirty(engine.handle), "committed battery acknowledged",
                  &engine);
            char battery_path[4096];
            path(battery_path, root, "saves", ids[system], ".sav");
            size_t battery_size;
            uint8_t *battery = read_bytes(battery_path, &battery_size);
            check(battery_size > 0 && battery[0] == 0x5a, "real GBA SRAM persisted", &engine);
            free(battery);
        }
        check(cr_engine_close(&engine), "unload", &engine);
        check(!engine.handle && !engine.loaded && engine.system == -1 && !engine.pixels,
              "native resources released", &engine);
        check(!cr_engine_step(&engine, 0), "closed engine cannot frame", &engine);
        check(cr_engine_close(&engine), "idempotent unload", &engine);
        check(cr_engine_open(&engine, system, ids[system], roms[system]) &&
                  cr_engine_step(&engine, 0),
              "reload content", &engine);
        if (system == 2) {
            uint64_t generation = 0;
            size_t size = engine.api.battery_size(engine.handle);
            uint8_t *battery = malloc(size);
            check(battery && engine.api.battery_copy(engine.handle, battery, size, &generation) &&
                      battery[0] == 0x5a,
                  "GBA battery reloaded", &engine);
            free(battery);
        }
        check(cr_engine_close(&engine), "final unload", &engine);
        unlink(state_path);
        unlink(invalid_path);
        printf("PASS native engine %d: idle, frames, PCM, input, state, atomic backup, invalid "
               "data, persistence, unload/reload\n",
               system);
    }
    cr_engine_destroy(&engine);
    char battery_path[4096];
    path(battery_path, root, "saves", ids[2], ".sav");
    unlink(battery_path);
    for (int system = 0; system < 3; ++system)
        unlink(roms[system]);
    char folder[4096];
    snprintf(folder, sizeof(folder), "%s/states", root);
    check(rmdir(folder) == 0, "remove test state directory", NULL);
    snprintf(folder, sizeof(folder), "%s/saves", root);
    check(rmdir(folder) == 0, "remove test battery directory", NULL);
    check(rmdir(root) == 0, "remove test storage", NULL);
    return 0;
}
