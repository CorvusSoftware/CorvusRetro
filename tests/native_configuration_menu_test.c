// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
#define _POSIX_C_SOURCE 200809L
#include "menu_driver.h"
#include "core_registry.h"
#include "play_layout.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static void check(bool ok, const char *message, const cr_ui *u) {
    if (ok) return;
    fprintf(stderr, "FAIL %s: %s / %s\n", message, u ? u->message : "", u ? u->engine.error : "");
    exit(1);
}
typedef struct {
    char root[4096];
    cr_game games[8];
    size_t count;
    unsigned picks, finished, accepted, touched;
    int picked;
    cr_import pending;
} fixture;
static void file_path(char out[4352], const char *root, const char *id) {
    int n = snprintf(out, 4352, "%s/roms/%s.rom", root, id);
    check(n > 0 && n < 4352, "bounded content path", NULL);
}
static void write_bytes(const char *path, const void *data, size_t size) {
    FILE *f = fopen(path, "wb");
    check(f && fwrite(data, 1, size, f) == size && !fclose(f), "write original fixture", NULL);
}
static void copy_file(const char *from, const char *to) {
    FILE *f = fopen(from, "rb"); check(f != NULL, "open import fixture", NULL);
    unsigned char bytes[32768]; size_t n = fread(bytes, 1, sizeof(bytes), f);
    check(!ferror(f) && !fclose(f), "read import fixture", NULL); write_bytes(to, bytes, n);
}
static cr_game *games(void *context, size_t *count) {
    fixture *f = context; *count = f->count;
    cr_game *copy = malloc(f->count * sizeof(*copy));
    check(copy != NULL, "copy saved library", NULL); memcpy(copy, f->games, f->count * sizeof(*copy)); return copy;
}
static void pick(void *context, int system) {
    fixture *f = context; ++f->picks; f->picked = system;
}
static void touch(void *context, const char *id) {
    fixture *f = context; check(cr_game_id_valid(id), "valid played identity", NULL); ++f->touched;
}
static bool finish(void *context, bool valid, int system, const char *core_id,
                   const char *system_id, char *error, size_t capacity) {
    fixture *f = context; ++f->finished;
    (void)error; (void)capacity;
    if (!valid) return false;
    check(core_id[0] && system_id[0] && f->count < 8, "import resolved stable core identity", NULL);
    char path[4352]; file_path(path, f->root, f->pending.id); copy_file(f->pending.path, path);
    cr_game *game = &f->games[f->count++]; memset(game, 0, sizeof(*game));
    snprintf(game->id, sizeof(game->id), "%s", f->pending.id);
    snprintf(game->title, sizeof(game->title), "%s", f->pending.title);
    snprintf(game->core_id, sizeof(game->core_id), "%s", core_id);
    snprintf(game->system_id, sizeof(game->system_id), "%s", system_id); game->system = system;
    ++f->accepted; return true;
}
static void choose(cr_ui *u, int row) { u->focus = row; cr_ui_move(u, 0); cr_ui_confirm(u); }
static void select_core(cr_ui *u, int system) {
    check(u->page == CR_MAIN && !u->engine.handle, "core selection starts unloaded", u);
    choose(u, 0); check(u->page == CR_CORES, "open core selection", u); choose(u, system);
    check(u->page == CR_MAIN && u->engine.system == system && u->engine.handle, "select real core", u);
}
static void open_config(cr_ui *u) {
    check(u->page == CR_MAIN, "subsystem configuration starts at main", u);
    choose(u, u->engine.loaded ? 3 : 2);
    check(u->page == CR_SUBSYSTEMS, "main exposes subsystem list", u); choose(u, 0);
    check(u->page == CR_SUBSYSTEM_CONFIG, "real two-player subsystem opens slots", u);
}
static void slot(cr_ui *u, int player, int row) {
    choose(u, player); check(u->page == CR_SUBSYSTEM_CONTENTS && u->link_slot == player,
                              "ordered slot opens compatible library selector", u);
    choose(u, row); check(u->page == CR_SUBSYSTEM_CONFIG, "saved game selection returns to slot configuration", u);
}
static void make_rom(const char *path, int system) {
    size_t size = system == 2 ? 512 : 32768;
    uint8_t *rom = calloc(size, 1); check(rom != NULL, "allocate original ROM", NULL);
    if (system == 0) {
        rom[0x100] = 0xc3; rom[0x101] = 0x50; rom[0x102] = 1; rom[0x147] = 3; rom[0x149] = 2;
        const uint8_t code[] = {0x3e,0x0a,0xea,0,0,0x3e,0x10,0xe0,0,
            0xf0,0,0xe6,0x0f,0xea,1,0xa0,0x18,0xf7};
        memcpy(rom + 0x150, code, sizeof(code));
    } else {
        const uint32_t code[] = {0xe59f000c,0xe59f100c,0xe1d120b0,0xe5c02001,
            0xeafffffc,0x0e000000,0x04000130};
        for (size_t i = 0; i < sizeof(code) / sizeof(code[0]); ++i)
            for (unsigned byte = 0; byte < 4; ++byte) rom[i * 4 + byte] = (uint8_t)(code[i] >> (byte * 8));
        rom[0xb2] = 0x96; memcpy(rom + 0x100, "SRAM_V", 6);
    }
    write_bytes(path, rom, size); free(rom);
}
static void identity(char out[96], char digit, const char *core) {
    char hash[65]; memset(hash, digit, 64); hash[64] = 0;
    check(cr_registry_content_id(out, 96, hash, core), "stable test content ID", NULL);
}
static void input_tests(cr_ui *u) {
    const unsigned a = 1u << 4, b = 1u << 5;
    unsigned masks[2]; cr_ui_clear_input(u);
    cr_ui_key(u, a, true); cr_ui_player_key(u, 0, a, true); cr_ui_player_key(u, 1, b, true);
    cr_ui_inputs(u, masks); check(masks[0] == a && masks[1] == b, "keyboard and hardware combine per player", u);
    cr_ui_player_key(u, 0, a, false); cr_ui_inputs(u, masks);
    check(masks[0] == a && masks[1] == b, "controller release preserves overlapping keyboard input", u);
    cr_ui_release_player(u, 1); cr_ui_inputs(u, masks);
    check(masks[0] == a && !masks[1], "disconnect releases only its hardware player", u);
    cr_ui_player_key(u, 1, b, true); cr_ui_player_key(u, 1, b, false); cr_ui_release_player(u, 1);
    cr_ui_inputs(u, masks); check(masks[0] == a && !masks[1], "disconnect clears unconsumed hardware pulses", u);
    cr_ui_key(u, a, false); cr_ui_inputs(u, masks); check(!masks[0] && !masks[1], "keyboard release", u);
    cr_ui_player_key(u, 1, b, true); cr_ui_player_key(u, 1, b, false);
    cr_ui_inputs(u, masks); check(masks[1] == b, "brief hardware tap survives to one frame", u);
    cr_ui_inputs(u, masks); check(!masks[1], "hardware tap pulse consumed once", u);
    cr_ui_back(u); check(u->quick, "open linked quick menu", u);
    choose(u, CR_QUICK_STATES); check(u->quick && u->page == CR_PLAY, "paired states action disabled", u);
    cr_ui_key(u, a, true); cr_ui_player_key(u, 0, b, true);
    choose(u, CR_QUICK_PLAYER); cr_ui_inputs(u, masks);
    check(u->quick && u->input_player == 1 && !masks[0] && !masks[1], "controlled-player switch releases held inputs and stays paused", u);
    cr_ui_back(u); check(cr_ui_running(u), "resume linked input session", u);
    cr_ui_key(u, b, true); cr_ui_inputs(u, masks);
    check(!masks[0] && masks[1] == b, "keyboard follows controlled player two", u);
    cr_ui_key(u, b, false); cr_ui_inputs(u, masks);
    cr_play_layout layout = cr_play_layout_make(u->width, u->height,
        cr_registry_legacy_system(u->engine.system), u->engine.width, u->engine.height);
    cr_area area = layout.controls[4].area;
    cr_ui_touch_at(u, 0, CR_TOUCH_DOWN, area.x + area.w / 2, area.y + area.h / 2, 100);
    cr_ui_player_key(u, 0, b, true); cr_ui_inputs(u, masks);
    check(masks[0] == b && masks[1] == a, "touch and hardware reach separate linked players", u);
    cr_ui_release_player(u, 1); cr_ui_inputs(u, masks);
    check(masks[0] == b && masks[1] == a, "hardware disconnect preserves controlled player's touch", u);
    check(cr_engine_step_players(&u->engine, masks), "UI masks run complete linked core frame", u);
    cr_ui_touch_at(u, 0, CR_TOUCH_CANCEL, 0, 0, 100.1); cr_ui_release_player(u, 0);
    cr_ui_inputs(u, masks); check(!masks[0] && !masks[1], "touch cancellation and hardware disconnect release both players", u);
    cr_ui_back(u); choose(u, CR_QUICK_PLAYER); cr_ui_back(u);
    check(u->input_player == 0 && cr_ui_running(u), "controlled player cycles back to one", u);
}
static void test_system(int system) {
    char root[] = "/tmp/corvus-configuration-menu-XXXXXX";
    check(mkdtemp(root) != NULL, "temporary menu storage", NULL);
    fixture f = {0}; snprintf(f.root, sizeof(f.root), "%s", root); f.count = 3;
    const cr_core_descriptor *core = cr_registry_get(system); check(core != NULL, "real core available", NULL);
    char directory[4352]; snprintf(directory, sizeof(directory), "%s/roms", root);
    check(!mkdir(directory, 0700), "private ROM directory", NULL);
    char roms[2][4352];
    for (unsigned i = 0; i < 3; ++i) {
        cr_game *game = &f.games[i]; game->system = i == 2 ? (system == 0 ? 2 : 0) : system;
        const cr_core_descriptor *game_core = cr_registry_get(game->system);
        identity(game->id, (char)('a' + i), game_core->id);
        snprintf(game->title, sizeof(game->title), "%s %u.%s", i == 2 ? "Incompatible" : "Unplayed original", i,
                 game->system == 2 ? "gba" : "gb");
        snprintf(game->core_id, sizeof(game->core_id), "%s", game_core->id);
        snprintf(game->system_id, sizeof(game->system_id), "%s", game_core->system_id);
        if (i < 2) { file_path(roms[i], root, game->id); make_rom(roms[i], system); }
    }
    cr_ui u; cr_ui_init(&u, (cr_platform){.context = &f, .games = games, .pick = pick,
        .finish_import = finish, .touch = touch}, root); cr_ui_size(&u, 384, 700);
    select_core(&u, system);
    uint64_t revision = u.core_revision;
    check(cr_engine_open(&u.engine, system, f.games[0].id, roms[0]), "initial normal session", &u);
    snprintf(u.title, sizeof(u.title), "Original normal session");
    u.page = CR_PLAY; cr_ui_back(&u); choose(&u, CR_QUICK_CORE_OPTIONS);
    check(u.page == CR_CORE_OPTIONS && !cr_ui_running(&u), "options submenu pauses real session", &u);
    choose(&u, 0); check(u.page == CR_CORE_OPTION_VALUES, "provider option opens value list", &u);
    char key[96]; snprintf(key, sizeof(key), "%s", u.option_key);
    unsigned selected = system == 2 ? 2 : 1;
    char blocked[4352]; snprintf(blocked, sizeof(blocked), "%s/options/%s.cfg.new", root, core->id);
    check(!mkdir(blocked, 0700), "block option durable commit", &u);
    choose(&u, (int)selected);
    check(u.page == CR_CORE_OPTION_VALUES && !cr_ui_running(&u) && cr_engine_option_value(&u.engine, key) == 0,
          "failed option persistence leaves value page and old selection", &u);
    check(!rmdir(blocked), "remove option obstruction", &u); choose(&u, (int)selected);
    check(u.page == CR_CORE_OPTIONS && cr_engine_option_value(&u.engine, key) == selected && !cr_ui_running(&u),
          "choosing provider value persists and returns to paused list", &u);
    cr_ui_back(&u); check(u.quick && u.focus == CR_QUICK_CORE_OPTIONS && !cr_ui_running(&u), "options back restores paused quick row", &u);
    choose(&u, CR_QUICK_MAIN); open_config(&u);
    choose(&u, 0); check(u.page == CR_SUBSYSTEM_CONTENTS, "P1 selector", &u);
    u.focus = 2; cr_ui_move(&u, 1); check(u.focus == 0, "selector lists compatible unplayed contents and excludes foreign core", &u);
    choose(&u, 1); check(!strcmp(u.link_ids[0], f.games[0].id), "P1 saved library identity copied", &u);
    cr_ui_reload(&u); check(!strcmp(u.link_titles[0], f.games[0].title), "slot title survives library record replacement", &u);
    slot(&u, 1, 2); check(!strcmp(u.link_ids[1], f.games[1].id), "P2 unplayed game selectable", &u);
    void *normal = u.engine.handle;
    uint8_t invalid = 0; write_bytes(roms[1], &invalid, 1); choose(&u, 2);
    check(u.page == CR_SUBSYSTEM_CONFIG && u.engine.handle == normal && u.engine.players == 1 &&
          !strcmp(u.title, "Original normal session"), "invalid second ROM rolls back to paused normal session", &u);
    make_rom(roms[1], system);
    choose(&u, 1); choose(&u, 0);
    check(u.importing && u.import_link && f.picked == system && u.import_slot == 1 &&
          u.import_revision == u.core_revision, "slot import snapshots selected core and player", &u);
    unsigned finished = f.finished; cr_import cancelled = {.system = -1}; cr_ui_import(&u, &cancelled);
    check(!u.importing && !u.import_link && f.finished == finished && !strcmp(u.link_ids[1], f.games[1].id) &&
          u.engine.handle == normal, "picker cancellation preserves slots and old session", &u);
    choose(&u, 0);
    memset(&f.pending, 0, sizeof(f.pending)); f.pending.system = system;
    identity(f.pending.id, 'd', core->id);
    snprintf(f.pending.title, sizeof(f.pending.title), "Imported original.%s", system == 2 ? "gba" : "gb");
    check(strlen(roms[1]) < sizeof(f.pending.path), "bounded pending import path", &u);
    memcpy(f.pending.path, roms[1], strlen(roms[1]) + 1);
    cr_ui_import(&u, &f.pending);
    check(u.page == CR_SUBSYSTEM_CONFIG && !u.importing && f.accepted == 1 &&
          !strcmp(u.link_ids[1], f.pending.id) && u.engine.handle == normal && u.engine.players == 1,
          "successful slot import commits library and selects P2 without launching", &u);
    choose(&u, 2);
    check(cr_ui_running(&u) && u.engine.players == 2 && u.engine.handle != normal &&
          cr_engine_option_value(&u.engine, key) == selected && f.touched == 2,
          "start builds configured paired candidate and touches both library records", &u);
    check(cr_engine_step(&u.engine, 0), "real initial linked frame", &u);
    input_tests(&u);
    char p2_blocked[4352];
    int path_length = snprintf(p2_blocked, sizeof(p2_blocked), "%s/saves/%s-link-p2.sav.new", root, u.link_ids[1]);
    check(path_length > 0 && path_length < (int)sizeof(p2_blocked) && !mkdir(p2_blocked, 0700),
          "block current P2 battery commit", &u);
    void *paired = u.engine.handle;
    char paired_title[512]; snprintf(paired_title, sizeof(paired_title), "%s", u.title);
    cr_ui_back(&u); choose(&u, CR_QUICK_MAIN); open_config(&u); choose(&u, 2);
    check(u.page == CR_SUBSYSTEM_CONFIG && u.engine.handle == paired && u.engine.players == 2 &&
          !strcmp(u.title, paired_title) && f.touched == 2 &&
          u.engine.subsystems.battery_dirty(u.engine.handle, 1),
          "paired start save failure preserves prior session and unsaved P2 generation", &u);
    check(!rmdir(p2_blocked), "remove P2 persistence obstruction", &u); choose(&u, 2);
    check(cr_ui_running(&u) && u.engine.handle != paired && f.touched == 4,
          "retry starts paired session after durable battery commit", &u);
    u.ra_enabled = true; snprintf(u.ra_username, sizeof(u.ra_username), "Retained account");
    cr_ui_achievement_frame(&u); check(!u.ra_game_id[0] && !u.ra_handle && !strcmp(u.ra_username, "Retained account"),
        "linked assessment disabled while account remains available", &u);
    cr_ui_back(&u); choose(&u, CR_QUICK_CLOSE);
    check(u.page == CR_MAIN && u.engine.handle && !u.engine.loaded && !u.engine.players &&
          cr_engine_option_value(&u.engine, key) == selected, "close linked content retains core and options", &u);
    open_config(&u); check(u.link_ids[0][0] && u.link_ids[1][0], "closing content retains configurable subsystem slots", &u);
    choose(&u, 0); choose(&u, 0); check(u.importing && u.import_link, "begin stale slot import", &u);
    cr_ui_back(&u); cr_ui_back(&u); cr_ui_back(&u); check(u.page == CR_MAIN, "return to main during picker", &u);
    choose(&u, 0); check(!u.engine.handle && !u.link_ids[0][0] && !u.link_ids[1][0] &&
        u.core_revision > revision, "explicit unload clears copied selections and advances revision", &u);
    select_core(&u, system);
    check(cr_engine_option_value(&u.engine, key) == selected && !u.subsystem_id[0], "reselect restores durable options with fresh link configuration", &u);
    open_config(&u); choose(&u, 0);
    check(u.page == CR_SUBSYSTEM_CONTENTS && u.link_slot == u.import_slot &&
          !strcmp(u.subsystem_id, u.import_subsystem_id) && u.core_revision != u.import_revision,
          "restore matching slot and subsystem while keeping stale import revision", &u);
    finished = f.finished; cr_ui_import(&u, &f.pending);
    check(f.finished == finished + 1 && f.accepted == 1 && !u.engine.loaded && !u.link_ids[0][0] &&
          !u.importing && !u.import_link, "stale import after core reselection declines storage commit", &u);
    cr_ui_destroy(&u);
}
static void generic_paired_states(const char *module) {
    cr_registry_add_installed_modules(&module, 1);
    int system = cr_registry_find("test.configuration");
    check(system >= 0, "third-party state-capable provider registered", NULL);
    char root[] = "/tmp/corvus-paired-state-menu-XXXXXX";
    check(mkdtemp(root) != NULL, "paired state menu storage", NULL);
    char roms[2][4352], id_storage[2][96];
    for (unsigned i = 0; i < 2; ++i) {
        int length = snprintf(roms[i], sizeof(roms[i]), "%s/player%u.rom", root, i);
        check(length > 0 && length < (int)sizeof(roms[i]), "paired fixture ROM path", NULL);
        uint8_t seed = (uint8_t)(10 + i); write_bytes(roms[i], &seed, 1);
        identity(id_storage[i], (char)('a' + i), "test.configuration");
    }
    const char *ids[2] = {id_storage[0], id_storage[1]}, *paths[2] = {roms[0], roms[1]};
    cr_ui u; cr_ui_init(&u, (cr_platform){0}, root); cr_ui_size(&u, 384, 700);
    check(cr_engine_open_subsystem(&u.engine, system, "link", ids, paths) &&
          u.engine.players == 2 && cr_engine_can_state(&u.engine), "generic provider explicitly supports complete paired states", &u);
    u.page = CR_PLAY; cr_ui_back(&u); choose(&u, CR_QUICK_STATES);
    check(u.page == CR_SAVESTATES && !u.quick && !cr_ui_running(&u),
          "paired state menu enabled by negotiated session capability", &u);
    choose(&u, 0); check(cr_ui_running(&u), "paired state menu saves complete session", &u);
    cr_ui_back(&u); choose(&u, CR_QUICK_STATES); choose(&u, 1);
    check(cr_ui_running(&u) && u.engine.players == 2, "paired state menu restores complete session", &u);
    char solo[4352];
    int length = snprintf(solo, sizeof(solo), "%s/states/%s-" CR_CORE_VERSION ".state", root, ids[0]);
    check(length > 0 && length < (int)sizeof(solo) && access(solo, F_OK) != 0,
          "paired state menu never creates a solo state", &u);
    cr_ui_destroy(&u);
}
int main(int argc, char **argv) {
    check(argc == 2, "state-capable ABI fixture module", NULL);
    test_system(cr_registry_find("corvus.gb")); test_system(cr_registry_find("corvus.gba"));
    generic_paired_states(argv[1]);
    puts("PASS real core configuration, local link menus, imports and two-player input"); return 0;
}
