// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
#define _POSIX_C_SOURCE 200809L
#include "core_registry.h"
#include "menu_driver.h"
#include "play_layout.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static void check(bool ok, const char *message) {
    if (!ok) {
        fprintf(stderr, "FAIL %s\n", message);
        exit(1);
    }
}
static const char hash[] = "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb";
static bool committed, picker_requested;
static void pick(void *ctx, int system) {
    (void)ctx;
    check(system == 0, "picker receives the selected runtime GBA index");
    picker_requested = true;
}
static bool finish(void *ctx, bool valid, int system, const char *core_id,
                   const char *system_id, char *error, size_t capacity) {
    (void)ctx;
    (void)error;
    (void)capacity;
    committed = valid && system == 0 && strcmp(core_id, "corvus.gba") == 0 &&
                strcmp(system_id, "nintendo.game_boy_advance") == 0;
    return committed;
}
static cr_game *games(void *ctx, size_t *count) {
    (void)ctx;
    cr_game *result = calloc(3, sizeof(*result));
    check(result != NULL, "allocate persistent library records");
    const char *ids[] = {"corvus.gb", "corvus.gbc", "corvus.gba"};
    for (int i = 0; i < 3; ++i) {
        snprintf(result[i].id, sizeof(result[i].id), "%s-%d", hash, i);
        snprintf(result[i].core_id, sizeof(result[i].core_id), "%s", ids[i]);
        snprintf(result[i].title, sizeof(result[i].title), "Original game %d", i);
        result[i].system = cr_registry_find(ids[i]);
        result[i].played = 1;
    }
    *count = 3;
    return result;
}
static void original_gba(const char *path) {
    uint8_t bytes[512] = {0};
    const uint32_t words[] = {0xe59f0008, 0xe3a0105a, 0xe5c01000, 0xeafffffe, 0x0e000000};
    for (size_t i = 0; i < sizeof(words) / sizeof(words[0]); ++i)
        for (unsigned b = 0; b < 4; ++b)
            bytes[i * 4 + b] = (uint8_t)(words[i] >> (b * 8));
    bytes[0xb2] = 0x96;
    memcpy(bytes + 0x100, "SRAM_V", 6);
    FILE *file = fopen(path, "wb");
    check(file && fwrite(bytes, 1, sizeof(bytes), file) == sizeof(bytes), "write original GBA ROM");
    check(fclose(file) == 0, "close original GBA ROM");
}
int main(int argc, char **argv) {
    check(argc == 4 && cr_registry_count() == 0, "base registry has zero bundled emulators");
    const char *invalid[] = {NULL, "libcorvus_gba.so", "/missing/libcorvus_gba.so"};
    cr_registry_add_installed_modules(invalid, 3);
    check(cr_registry_count() == 0, "missing and relative installed module paths are ignored");
    cr_registry_add_installed_modules((const char *const *)&argv[1], 1);
    check(cr_registry_count() == 1 && cr_registry_find("corvus.gba") == 0 &&
              cr_registry_legacy_system(0) == 2,
          "GBA installed alone has index zero and historical system two");
    char id[96], wrong_id[96], community[96];
    check(cr_registry_content_id(id, sizeof(id), hash, "corvus.gba") &&
              strcmp(id + 64, "-2") == 0 &&
              cr_registry_content_id(wrong_id, sizeof(wrong_id), hash, "corvus.gb") &&
              strcmp(wrong_id + 64, "-0") == 0 &&
              cr_registry_content_id(community, sizeof(community), hash, "community.test") &&
              strncmp(community + 64, "-c", 2) == 0,
          "content storage follows permanent core identity instead of registry position");

    char root[] = "/tmp/corvus-split-registry-XXXXXX", directory[4096], path[4096];
    check(mkdtemp(root) != NULL, "private test directory");
    snprintf(directory, sizeof(directory), "%s/roms", root);
    check(mkdir(directory, 0700) == 0, "ROM library directory");
    snprintf(path, sizeof(path), "%s/roms/%s.rom", root, id);
    original_gba(path);
    cr_ui ui;
    cr_ui_init(&ui, (cr_platform){.games = games, .pick = pick, .finish_import = finish}, root);
    check(ui.game_count == 3 && ui.games[0].system == -1 && ui.games[1].system == -1 &&
              ui.games[2].system == 0,
          "games whose cores are absent stay in the library without reassignment");
    cr_ui_confirm(&ui);
    cr_ui_confirm(&ui);
    check(ui.engine.handle && ui.engine.system == 0, "select the installed GBA core");
    ui.focus = 1;
    cr_ui_confirm(&ui);
    check(picker_requested && ui.importing, "start a picker request for the selected core");
    cr_import incoming = {.system = 0};
    snprintf(incoming.id, sizeof(incoming.id), "%s", wrong_id);
    snprintf(incoming.title, sizeof(incoming.title), "Original.GBA");
    snprintf(incoming.path, sizeof(incoming.path), "%s", path);
    cr_ui_import(&ui, &incoming);
    check(committed && ui.engine.loaded && strcmp(ui.engine.id, id) == 0 &&
              cr_engine_step(&ui.engine, 0),
          "GBA import at runtime index zero reuses historical SHA-2 ROM and battery paths");
    cr_play_layout layout = cr_play_layout_make(384, 700, 2, 240, 160);
    cr_area shoulder = layout.controls[8].area;
    cr_ui_size(&ui, 384, 700);
    cr_ui_touch(&ui, 0, CR_TOUCH_DOWN, shoulder.x + shoulder.w / 2, shoulder.y + shoulder.h / 2);
    check(cr_ui_input(&ui) & (1u << 8), "GBA shoulder controls follow core identity at index zero");
    cr_ui_touch(&ui, 0, CR_TOUCH_CANCEL, 0, 0);
    void *handle = ui.engine.handle;
    check(!cr_engine_open(&ui.engine, 0, wrong_id, path) && ui.engine.handle == handle &&
              ui.engine.loaded, "another console's historic ID cannot overwrite GBA saves");
    cr_registry_add_installed_modules((const char *const *)&argv[1], 3);
    check(cr_registry_count() == 3 && cr_registry_find("corvus.gba") == 0 &&
              cr_registry_find("corvus.gb") == 1 && cr_registry_find("corvus.gbc") == 2,
          "adding GB and GBC keeps the existing GBA module and avoids duplicate IDs");
    cr_ui_reload(&ui);
    check(ui.game_count == 3 && ui.games[0].system == 1 && ui.games[1].system == 2 &&
              ui.games[2].system == 0 && strcmp(ui.engine.id, id) == 0,
          "library identity remains stable after other modules are installed");
    char replacement[4096];
    snprintf(replacement, sizeof(replacement), "%s/replacement-gba.so", root);
    FILE *source = fopen(argv[1], "rb"), *destination = fopen(replacement, "wb");
    check(source && destination, "open a new immutable version of the GBA module");
    uint8_t buffer[65536];
    size_t bytes;
    while ((bytes = fread(buffer, 1, sizeof(buffer), source)) != 0)
        check(fwrite(buffer, 1, bytes, destination) == bytes, "copy replacement module");
    check(!ferror(source) && fclose(source) == 0 && fclose(destination) == 0,
          "finish replacement module");
    const char *replacement_path = replacement;
    cr_registry_add_installed_modules(&replacement_path, 1);
    const cr_core_descriptor *updated = cr_registry_get(0);
    check(cr_registry_count() == 3 && updated && strcmp(updated->module, replacement) == 0 &&
              strcmp(updated->system_id, "nintendo.game_boy_advance") == 0 &&
              updated->state_support && updated->battery_support && updated->max_players > 0,
          "replacement preserves stable index, system identity and core capabilities");
    check(ui.engine.handle == handle && cr_engine_step(&ui.engine, 0),
          "a running game keeps its valid previous module while a replacement is registered");
    cr_ui_destroy(&ui);
    check(unlink(replacement) == 0, "remove replacement module");
    check(unlink(path) == 0 && rmdir(directory) == 0, "remove original test ROM");
    snprintf(path, sizeof(path), "%s/saves/%s.sav", root, id);
    check(access(path, F_OK) == 0 && unlink(path) == 0, "historical GBA battery was persisted");
    snprintf(directory, sizeof(directory), "%s/saves", root);
    check(rmdir(directory) == 0, "remove private battery directory");
    snprintf(directory, sizeof(directory), "%s/options", root);
    check(rmdir(directory) == 0 && rmdir(root) == 0, "remove private test state");
    puts("PASS installed core discovery, partial catalog, stable saves and imports");
    return 0;
}
