// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
#define _POSIX_C_SOURCE 200809L
#include "menu_driver.h"
#include "core_registry.h"
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
    uint64_t request, finished_request;
    int operation;
    unsigned started, finished;
    uint8_t *snapshot;
    size_t size;
    char title[600];
} fixture;
static bool begin(void *context, uint64_t request, int operation, const char *title,
                  const uint8_t *bytes, size_t size) {
    fixture *f = context;
    free(f->snapshot); f->snapshot = NULL;
    f->request = request; f->operation = operation; ++f->started; f->size = size;
    snprintf(f->title, sizeof(f->title), "%s", title);
    if (size) {
        f->snapshot = malloc(size); check(f->snapshot && bytes, "immutable platform snapshot", NULL);
        memcpy(f->snapshot, bytes, size);
    } else check(!bytes && operation == CR_SAVE_IMPORT, "import sends no live RAM to platform", NULL);
    return true;
}
static void finish(void *context, uint64_t request) {
    fixture *f = context; ++f->finished; f->finished_request = request;
}
static void choose(cr_ui *u, int row) {
    if (u->quick && u->engine.players < 2 && row >= CR_QUICK_PLAYER) --row;
    u->focus = row; cr_ui_move(u, 0); cr_ui_confirm(u);
}
static void open_saves(cr_ui *u) {
    u->page = CR_PLAY; u->quick = true; u->depth = 0;
    choose(u, CR_QUICK_GAME_SAVES);
    check(u->page == CR_GAME_SAVES && !cr_ui_running(u), "quick menu opens separate game battery saves", u);
}
static void write_bytes(const char *path, const void *bytes, size_t size) {
    FILE *f = fopen(path, "wb");
    check(f && fwrite(bytes, 1, size, f) == size && !fclose(f), "write original fixture", NULL);
}
static void make_rom(const char *path) {
    uint8_t bytes[32768] = {0};
    bytes[0x100] = 0xc3; bytes[0x101] = 0x50; bytes[0x102] = 1;
    bytes[0x147] = 3; bytes[0x149] = 2;
    const uint8_t code[] = {0x3e,0x0a,0xea,0,0,0x3e,0x51,0xea,2,0xa0,0x18,0xfe};
    memcpy(bytes + 0x150, code, sizeof(code)); write_bytes(path, bytes, sizeof(bytes));
}
static cr_save_transfer_result import_result(cr_ui *u, const uint8_t *bytes, size_t size) {
    cr_save_transfer_result item = {.request = u->save_request};
    char directory[4352]; snprintf(directory, sizeof(directory), "%s/save-transfer", u->engine.root);
    struct stat st;
    check(!mkdir(directory, 0700) || (!stat(directory, &st) && S_ISDIR(st.st_mode)), "private transfer directory", u);
    int n = snprintf(item.path, sizeof(item.path), "%.4000s/save-transfer/%llu.sav", u->engine.root,
                     (unsigned long long)item.request);
    check(n > 0 && n < (int)sizeof(item.path), "bounded request-owned path", u);
    write_bytes(item.path, bytes, size);
    return item;
}
int main(void) {
    char root[] = "/tmp/corvus-save-transfer-menu-XXXXXX";
    check(mkdtemp(root) != NULL, "private storage", NULL);
    fixture f = {0};
    cr_ui u; cr_ui_init(&u, (cr_platform){.context = &f, .save_transfer = begin,
                                        .finish_save_transfer = finish}, root);
    cr_ui_size(&u, 384, 700);
    u.page = CR_PLAY; u.quick = true; choose(&u, CR_QUICK_GAME_SAVES);
    check(u.quick && !f.started, "absent loaded battery cannot start transfer", &u);
    int system = cr_registry_find("corvus.gb"); check(system >= 0, "real GB installed for host tests", &u);
    char rom[4352]; snprintf(rom, sizeof(rom), "%s/original.gb", root); make_rom(rom);
    const char *id = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa-0";
    check(cr_engine_open(&u.engine, system, id, rom) && cr_engine_step(&u.engine, 0), "real battery game", &u);
    snprintf(u.title, sizeof(u.title), "Original GB");
    /* A loaded game without battery data still exposes no invented transfer. */
    size_t (*battery_size)(void *, uint32_t) = u.engine.subsystems.battery_size;
    size_t (*single_size)(void *) = u.engine.api.battery_size;
    u.engine.subsystems.battery_size = NULL; u.engine.api.battery_size = NULL;
    u.page = CR_PLAY; u.quick = true; choose(&u, CR_QUICK_GAME_SAVES);
    check(u.quick && !f.started, "unsupported battery operation remains disabled", &u);
    u.engine.subsystems.battery_size = battery_size; u.engine.api.battery_size = single_size;
    for (unsigned orientation = 0; orientation < 2; ++orientation) {
        cr_ui_size(&u, orientation ? 700 : 384, orientation ? 384 : 700);
        open_saves(&u); choose(&u, CR_SAVE_EXPORT_SAV);
        check(u.save_request && f.operation == CR_SAVE_EXPORT_SAV && f.size == 8192 &&
              f.snapshot[2] == 0x51 && !cr_ui_running(&u), "export freezes current unsaved live SRAM", &u);
        uint64_t request = f.request; void *active = u.engine.handle;
        u.page = CR_MAIN; choose(&u, 1);
        check(u.engine.handle == active && u.save_request == request, "pending picker blocks core unloading", &u);
        u.page = CR_PLAY; u.quick = true; choose(&u, CR_QUICK_CLOSE);
        check(u.engine.handle == active && u.engine.loaded && u.save_request == request,
              "pending picker blocks game close", &u);
        choose(&u, CR_QUICK_RESTART);
        check(u.save_request == request, "pending picker blocks restart", &u);
        cr_save_transfer_result stale = {.request = request + 100}; cr_ui_save_transfer_result(&u, &stale);
        check(u.save_request == request && f.finished_request == stale.request, "foreign callback cannot consume active request", &u);
        cr_save_transfer_result done = {.request = request}; cr_ui_save_transfer_result(&u, &done);
        check(!u.save_request && f.finished_request == request && u.engine.handle == active,
              "export completion releases picker without replacing game", &u);
        open_saves(&u); choose(&u, CR_SAVE_EXPORT_SRM);
        check(f.request > request && f.operation == CR_SAVE_EXPORT_SRM && f.size == 8192,
              "second raw battery export has a fresh nonce and .srm operation", &u);
        done.request = f.request; done.cancelled = true; cr_ui_save_transfer_result(&u, &done);
        check(!u.save_request && u.engine.handle == active, "picker cancellation retains game", &u);
    }
    uint8_t *replacement = malloc(8192); check(replacement != NULL, "replacement allocation", &u);
    memset(replacement, 0x73, 8192);
    open_saves(&u); choose(&u, CR_SAVE_IMPORT);
    cr_save_transfer_result item = import_result(&u, replacement, 8192);
    void *active = u.engine.handle;
    cr_ui_save_transfer_result(&u, &item);
    check(u.page == CR_SAVE_CONFIRM && u.save_request && !cr_ui_running(&u) && u.engine.handle == active,
          "import waits for explicit restart confirmation on frontend owner thread", &u);
    cr_ui_back(&u);
    check(!u.save_request && u.engine.handle == active && u.page == CR_GAME_SAVES,
          "back from confirmation cancels without touching live game", &u); unlink(item.path);
    choose(&u, CR_SAVE_IMPORT); item = import_result(&u, replacement, 8191);
    cr_ui_save_transfer_result(&u, &item); choose(&u, 0);
    check(!u.save_request && u.engine.handle == active && u.page == CR_GAME_SAVES,
          "malformed imported battery retains live game", &u); unlink(item.path);
    choose(&u, CR_SAVE_IMPORT); item = import_result(&u, replacement, 8192);
    check(!unlink(item.path) && !symlink(rom, item.path), "symlink in expected stage location", &u);
    cr_ui_save_transfer_result(&u, &item); choose(&u, 0);
    check(!u.save_request && u.engine.handle == active, "stage symlink cannot reach candidate loading", &u);
    unlink(item.path);
    choose(&u, CR_SAVE_IMPORT); item = import_result(&u, replacement, 8192);
    cr_ui_save_transfer_result(&u, &item);
    cr_save_transfer_result duplicate = {.request = item.request, .cancelled = true};
    cr_ui_save_transfer_result(&u, &duplicate);
    check(u.page == CR_SAVE_CONFIRM && u.save_request == item.request,
          "duplicate terminal callback cannot cancel a staged import", &u);
    choose(&u, 0);
    check(!u.save_request && u.engine.handle != active && cr_ui_running(&u),
          "confirmed valid import restarts loaded game", &u); unlink(item.path);
    uint8_t *actual = NULL; size_t actual_size = 0;
    check(cr_engine_export_battery(&u.engine, 0, &actual, &actual_size) && actual_size == 8192 &&
          !memcmp(actual, replacement, 8192), "confirmed import loads every byte", &u); free(actual);
    open_saves(&u); choose(&u, 3);
    check(u.page == CR_GAME_BACKUPS && u.backup_available == 1,
          "backup menu reports the actual private safety copy", &u);
    choose(&u, 0);
    check(u.save_request && f.operation == CR_SAVE_EXPORT_SAV && f.size == 8192 &&
          f.snapshot[2] == 0x51 && strstr(f.title, "copie 1"),
          "backup export makes pre-import live progress recoverable through SAF", &u);
    cr_save_transfer_result exported_backup = {.request = u.save_request};
    cr_ui_save_transfer_result(&u, &exported_backup);
    open_saves(&u); choose(&u, CR_SAVE_IMPORT); uint64_t request = u.save_request;
    ++u.core_revision;
    item = (cr_save_transfer_result){.request = request}; cr_ui_save_transfer_result(&u, &item);
    check(!u.save_request && u.engine.loaded, "revision invalidates callback without loading data", &u);
    choose(&u, CR_SAVE_IMPORT); item = (cr_save_transfer_result){.request = u.save_request};
    snprintf(item.path, sizeof(item.path), "%.4095s", rom); cr_ui_save_transfer_result(&u, &item);
    check(!u.save_request && u.page == CR_GAME_SAVES, "import rejects file outside request-owned stage", &u);
    check(cr_engine_save_auto_state(&u.engine), "create a real automatic state for the quick menu", &u);
    u.page = CR_PLAY; u.quick = true; u.depth = 0; choose(&u, CR_QUICK_STATES);
    check(u.page == CR_SAVESTATES && u.auto_state_available && !cr_ui_running(&u),
          "automatic-state availability is refreshed when the state menu opens", &u);
    choose(&u, 2);
    check(cr_ui_running(&u) && u.engine.loaded, "quick menu restores a real automatic state", &u);
    const char *ids[2] = {id, id}, *paths[2] = {rom, rom};
    corvus_subsystem_info info;
    check(cr_engine_subsystem(&u.engine, 0, &info), "declared real subsystem", &u);
    char subsystem[96]; snprintf(subsystem, sizeof(subsystem), "%s", info.id);
    check(cr_engine_open_subsystem(&u.engine, system, subsystem, ids, paths), "real linked battery game", &u);
    open_saves(&u); choose(&u, 0); choose(&u, CR_SAVE_EXPORT_SAV + 1);
    check(u.save_player == 1 && f.operation == CR_SAVE_EXPORT_SAV && strstr(f.title, "Joueur 2") && f.size == 8192,
          "link menu selects P2 and gives it a distinct export name", &u);
    cr_ui_destroy(&u);
    check(f.finished_request == f.request, "destroy releases outstanding picker", NULL);
    free(f.snapshot); free(replacement);
    puts("PASS battery transfer menus, frozen exports, confirmation, cancellation and stale callbacks");
    return 0;
}
