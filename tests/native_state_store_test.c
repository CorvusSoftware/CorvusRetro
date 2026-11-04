// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
#define _POSIX_C_SOURCE 200809L
#define _XOPEN_SOURCE 700
#include "state_store.h"
#include "core_registry.h"
#include <errno.h>
#include <ftw.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

static unsigned checks;
static cr_engine *tested_engine;
static pthread_t owner;
static atomic_int fail_after = -1;
static pthread_mutex_t gate = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t changed = PTHREAD_COND_INITIALIZER;
static bool blocked, entered;
static atomic_bool released;
static atomic_uint worker_syncs;
static corvus_core_api original;
static unsigned restored;
static void check(bool ok, const char *message) {
    if (!ok) {
        fprintf(stderr, "FAIL state store: %s (%s)\n", message, tested_engine ? tested_engine->error : "");
        exit(1);
    }
    ++checks;
}
int __real_fsync(int fd);
int __wrap_fsync(int fd) {
    pthread_mutex_lock(&gate);
    if (blocked) {
        if (pthread_equal(owner, pthread_self())) {
            fputs("FAIL state store: synchronous IO while waiting for the async writer\n", stderr);
            abort();
        }
        entered = true; pthread_cond_broadcast(&changed);
        while (blocked) pthread_cond_wait(&changed, &gate);
    }
    pthread_mutex_unlock(&gate);
    int left = atomic_load(&fail_after);
    if (left == 0) { atomic_store(&fail_after, -1); errno = EIO; return -1; }
    if (left > 0) atomic_fetch_sub(&fail_after, 1);
    int result = __real_fsync(fd);
    if (!pthread_equal(owner, pthread_self())) atomic_fetch_add(&worker_syncs, 1);
    return result;
}
static int restore(void *handle, const uint8_t *bytes, size_t size) {
    check(pthread_equal(owner, pthread_self()), "restore uses owner thread");
    check(!entered || (atomic_load(&released) && atomic_load(&worker_syncs) >= 2),
          "restore waits for both pending battery fsync operations");
    ++restored;
    return original.unserialize(handle, bytes, size);
}
static void write_file(const char *path, const void *bytes, size_t size) {
    FILE *file = fopen(path, "wb");
    check(file && fwrite(bytes, 1, size, file) == size && !fclose(file), "write owned fixture");
}
static uint8_t *read_file(const char *path, size_t *size) {
    FILE *file = fopen(path, "rb");
    check(file && !fseek(file, 0, SEEK_END), "open fixture readback");
    long length = ftell(file);
    check(length >= 0 && !fseek(file, 0, SEEK_SET), "bounded fixture size");
    *size = (size_t)length;
    uint8_t *bytes = malloc(*size + 1);
    check(bytes && fread(bytes, 1, *size, file) == *size && !fclose(file), "read fixture bytes");
    return bytes;
}
static void make_rom(const char *path, bool gba) {
    uint8_t bytes[32768] = {0};
    if (gba) {
        const uint32_t code[] = {0xe59f0010, 0xe3a01000, 0xe2811001, 0xe5c01000,
                                 0xeafffffc, 0xeafffffe, 0x0e000000};
        for (size_t i = 0; i < sizeof(code) / sizeof(*code); ++i)
            for (unsigned b = 0; b < 4; ++b) bytes[i * 4 + b] = (uint8_t)(code[i] >> (b * 8));
        bytes[0xb2] = 0x96; memcpy(bytes + 0x100, "SRAM_V", 6);
    } else {
        bytes[0x100] = 0xc3; bytes[0x102] = 1;
    }
    write_file(path, bytes, gba ? 512 : sizeof(bytes));
}
static void entry_path(char path[4096], const char *root, const cr_state_entry *e, const char *file) {
    int n = snprintf(path, 4096, "%s/states/library/%s/%u.entry/%s", root, e->game_id, e->slot, file);
    check(n > 0 && n < 4096, "fixture entry path");
}
static void same_bytes(const char *path, const uint8_t *expected, size_t count) {
    size_t size; uint8_t *bytes = read_file(path, &size);
    check(size == count && !memcmp(bytes, expected, count), "legacy bytes preserved exactly"); free(bytes);
}
static int cleanup(const char *path, const struct stat *status, int type, struct FTW *walk) {
    (void)status; (void)type; (void)walk; return remove(path);
}
static void *release_disk(void *unused) {
    (void)unused;
    struct timespec pause = {0, 20000000}; nanosleep(&pause, NULL);
    pthread_mutex_lock(&gate); atomic_store(&released, true); blocked = false;
    pthread_cond_broadcast(&changed); pthread_mutex_unlock(&gate);
    return NULL;
}
int main(void) {
    owner = pthread_self();
    char root[] = "/tmp/corvus-state-store-XXXXXX", rom[4096], path[4096], legacy[4096], automatic[4096];
    check(mkdtemp(root) != NULL, "private state collection");
    const char *id = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa-0";
    const char *other = "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb-0";
    const char *gba_id = "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc-2";
    cr_state_entry entries[CR_STATE_STORE_LIMIT], first, copy;
    char error[256];
    check(cr_state_store_list(root, id, entries, CR_STATE_STORE_LIMIT, error, sizeof(error)) == 0 && !error[0], "new collection is truly empty");
    check(!cr_state_store_list(root, "../bad", entries, CR_STATE_STORE_LIMIT, error, sizeof(error)) && error[0], "reject path traversal ID");
    snprintf(rom, sizeof(rom), "%s/original.gb", root); make_rom(rom, false);
    cr_engine e; cr_engine_init(&e, root); tested_engine = &e;
    check(cr_engine_open(&e, cr_registry_find("corvus.gb"), id, rom) && cr_engine_step(&e, 0), "real GB synthetic program");
    check(cr_engine_save_state(&e) && cr_engine_save_auto_state(&e), "legacy manual and automatic states");
    snprintf(legacy, sizeof(legacy), "%s/states/%s-%s.state", root, id, CR_CORE_VERSION);
    snprintf(automatic, sizeof(automatic), "%s/states/%s-%s.auto.state", root, id, CR_CORE_VERSION);
    size_t legacy_size, auto_size;
    uint8_t *legacy_bytes = read_file(legacy, &legacy_size), *auto_bytes = read_file(automatic, &auto_size);
    e.pixels[0] = UINT32_C(0xff123456);
    check(cr_state_store_create(&e, "Avant le pont", &first), "create a real named snapshot");
    check(first.slot == 1 && first.players == 1 && !strcmp(first.game_id, id) &&
          !strcmp(first.core_id, "corvus.gb") && !strcmp(first.name, "Avant le pont") &&
          first.created_ms > 0 && first.revision > 0 && strstr(first.preview_key, "state:") == first.preview_key,
          "entry carries real identity, timestamp and versioned preview key");
    entry_path(path, root, &first, "preview.ppm"); size_t preview_size;
    uint8_t *preview = read_file(path, &preview_size);
    const char *header = "P6\n160 144\n255\n"; size_t header_size = strlen(header);
    check(preview_size == header_size + 160u * 144u * 3u && !memcmp(preview, header, header_size) &&
          preview[header_size] == 0x12 && preview[header_size + 1] == 0x34 && preview[header_size + 2] == 0x56,
          "preview is bounded RGB from the documented ARGB8888 core frame"); free(preview);
    uint8_t *raw = NULL; size_t raw_size = 0;
    check(cr_state_store_read(root, &first, &raw, &raw_size, error, sizeof(error)) &&
          raw_size == legacy_size && !memcmp(raw, legacy_bytes, raw_size), "sharing returns exact SDK state bytes");
    check(cr_engine_step(&e, 0) && cr_state_store_load(&e, &first), "restore named state into matching real engine");
    uint8_t *actual = NULL; size_t actual_size;
    check(cr_engine_state_copy(&e, &actual, &actual_size) && actual_size == raw_size && !memcmp(actual, raw, raw_size),
          "named restoration reproduces the saved machine state"); free(actual);
    check(cr_state_store_rename(root, &first, "État du pont", error, sizeof(error)), "rename Unicode label without touching raw state");
    check(cr_state_store_list(root, id, entries, CR_STATE_STORE_LIMIT, error, sizeof(error)) == 1 &&
          !strcmp(entries[0].name, "État du pont") && entries[0].revision == first.revision, "metadata rename preserves immutable preview revision");
    first = entries[0];
    check(!cr_state_store_rename(root, &first, "\nunsafe", error, sizeof(error)) &&
          !cr_state_store_rename(root, &first, "   ", error, sizeof(error)) &&
          !cr_state_store_rename(root, &first, "\xc3", error, sizeof(error)), "reject control, blank and invalid UTF-8 labels");
    check(cr_state_store_duplicate(root, &first, &copy, error, sizeof(error)) && copy.slot != first.slot &&
          copy.revision != first.revision && !strcmp(copy.name, "État du pont (copie)"), "duplicate uses a distinct real slot and new preview identity");
    actual = NULL;
    check(cr_state_store_read(root, &copy, &actual, &actual_size, error, sizeof(error)) &&
          actual_size == raw_size && !memcmp(actual, raw, raw_size), "duplicate raw state remains exact"); free(actual);
    size_t count = cr_state_store_list(root, id, entries, CR_STATE_STORE_LIMIT, error, sizeof(error));
    check(count == 2 && entries[0].slot == copy.slot, "entries sort newest first");
    for (int failure = 0; failure <= 4; ++failure) {
        atomic_store(&fail_after, failure);
        cr_state_entry failed;
        check(!cr_state_store_create(&e, "Écriture interrompue", &failed), "injected fsync failure is reported");
        atomic_store(&fail_after, -1);
        check(cr_state_store_list(root, id, entries, CR_STATE_STORE_LIMIT, error, sizeof(error)) == count,
              "unacknowledged state/metadata/preview transaction is not exposed");
        check(cr_state_store_read(root, &first, &actual, &actual_size, error, sizeof(error)) &&
              actual_size == raw_size && !memcmp(actual, raw, raw_size), "failed create preserves existing saved progress"); free(actual);
    }
    atomic_store(&fail_after, 0);
    check(!cr_state_store_rename(root, &first, "Failed", error, sizeof(error)), "failed label write is reported");
    atomic_store(&fail_after, -1);
    check(cr_state_store_list(root, id, entries, CR_STATE_STORE_LIMIT, error, sizeof(error)) == count &&
          !strcmp(entries[1].name, first.name), "failed label write keeps previous metadata");
    atomic_store(&fail_after, 0);
    check(!cr_state_store_delete(root, &copy, error, sizeof(error)), "failed deletion durability is reported");
    atomic_store(&fail_after, -1);
    check(cr_state_store_read(root, &copy, &actual, &actual_size, error, sizeof(error)), "failed deletion keeps selected state available"); free(actual);
    cr_state_entry wrong = first; snprintf(wrong.game_id, sizeof(wrong.game_id), "%s", other);
    check(!cr_state_store_load(&e, &wrong), "different content identity cannot restore a state");
    memset(wrong.game_id, 'a', sizeof(wrong.game_id));
    check(!cr_state_store_read(root, &wrong, &actual, &actual_size, error, sizeof(error)),
          "unterminated external entry IDs are rejected without reading beyond the record");
    wrong = first; snprintf(wrong.core_version, sizeof(wrong.core_version), "foreign");
    check(!cr_state_store_load(&e, &wrong), "different core version is refused before core restore");
    entry_path(path, root, &first, "state");
    actual = read_file(path, &actual_size); actual[0] ^= 1; write_file(path, actual, actual_size); free(actual);
    check(!cr_state_store_read(root, &first, &actual, &actual_size, error, sizeof(error)) && !actual && !actual_size,
          "same-size corrupted raw state is rejected by integrity check");
    write_file(path, raw, raw_size);
    entry_path(path, root, &first, "meta"); size_t meta_size;
    actual = read_file(path, &meta_size); uint8_t *meta = malloc(meta_size);
    check(meta != NULL, "copy metadata fixture"); memcpy(meta, actual, meta_size);
    actual[8] = 33; write_file(path, actual, meta_size); free(actual);
    check(!cr_state_store_read(root, &first, &actual, &actual_size, error, sizeof(error)), "malformed slot metadata is rejected");
    write_file(path, meta, meta_size); free(meta);
    entry_path(path, root, &copy, "state");
    check(!unlink(path) && !symlink(legacy, path), "inject symlink state fixture");
    check(!cr_state_store_read(root, &copy, &actual, &actual_size, error, sizeof(error)), "state symlinks never expose another private file");
    check(!unlink(path), "remove owned symlink fixture"); write_file(path, raw, raw_size);
    cr_state_entry stale = copy;
    check(cr_state_store_delete(root, &copy, error, sizeof(error)), "delete explicitly selected state only");
    check(cr_state_store_duplicate(root, &first, &copy, error, sizeof(error)) && copy.slot == stale.slot &&
          strcmp(copy.preview_key, stale.preview_key), "slot reuse receives a fresh cache identity");
    check(!cr_state_store_read(root, &stale, &actual, &actual_size, error, sizeof(error)), "stale UI entry cannot read a replacement state");
    same_bytes(legacy, legacy_bytes, legacy_size); same_bytes(automatic, auto_bytes, auto_size);
    free(legacy_bytes); free(auto_bytes);
    for (unsigned i = 2; i < CR_STATE_STORE_LIMIT; ++i)
        check(cr_state_store_duplicate(root, &first, &copy, error, sizeof(error)), "bounded collection accepts available slots");
    check(cr_state_store_list(root, id, entries, CR_STATE_STORE_LIMIT, error, sizeof(error)) == CR_STATE_STORE_LIMIT,
          "collection contains exactly its bounded slots");
    check(!cr_state_store_duplicate(root, &first, &copy, error, sizeof(error)) && error[0], "full collection reports error without overwrite");
    check(cr_state_store_read(root, &first, &actual, &actual_size, error, sizeof(error)) &&
          actual_size == raw_size && !memcmp(actual, raw, raw_size), "full collection retains oldest saved progress"); free(actual); free(raw);
    check(cr_engine_close(&e), "close single-player state fixture");

    const char *gbc_id = "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb-1";
    check(cr_engine_open(&e, cr_registry_find("corvus.gbc"), gbc_id, rom) && cr_engine_step(&e, 0) &&
          cr_state_store_create(&e, "Partie couleur", &copy), "capture a real GBC state");
    check(!strcmp(copy.core_id, "corvus.gbc") && !strcmp(copy.game_id, gbc_id), "GBC keeps its independent stable identity");
    check(cr_engine_step(&e, 0) && cr_state_store_load(&e, &copy), "restore a named GBC state");
    check(cr_engine_close(&e), "close GBC fixture");

    const char *link1 = "dddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd-0";
    const char *link2 = "eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee-0";
    const char *ids[] = {link1, link2}, *paths[] = {rom, rom};
    corvus_subsystem_info subsystem;
    check(cr_engine_select(&e, cr_registry_find("corvus.gb")) && cr_engine_subsystem(&e, 0, &subsystem), "discover actual link subsystem");
    check(cr_engine_open_subsystem(&e, e.system, subsystem.id, ids, paths), "open linked state fixture");
    unsigned masks[] = {0, 0};
    check(cr_engine_step_players(&e, masks) && !cr_engine_can_state(&e) &&
          !cr_state_store_create(&e, "Deux joueurs", &copy),
          "unsupported real linked states stay disabled instead of creating a fake snapshot");
    check(!cr_state_store_list(root, link1, entries, CR_STATE_STORE_LIMIT, error, sizeof(error)),
          "unsupported linked operation leaves no collection entry");
    check(cr_engine_close(&e), "close linked fixtures");

    snprintf(rom, sizeof(rom), "%s/original.gba", root); make_rom(rom, true);
    check(cr_engine_open(&e, cr_registry_find("corvus.gba"), gba_id, rom) && cr_engine_step(&e, 0) &&
          cr_state_store_create(&e, "GBA avant écriture", &first), "capture real GBA state with dirty SRAM");
    original = e.api; e.api.unserialize = restore;
    pthread_mutex_lock(&gate); blocked = true; entered = false;
    atomic_store(&released, false); atomic_store(&worker_syncs, 0); pthread_mutex_unlock(&gate);
    check(cr_engine_request_battery_save(&e), "queue periodic battery snapshot before named restore");
    struct timespec until; clock_gettime(CLOCK_REALTIME, &until); until.tv_sec += 5;
    pthread_mutex_lock(&gate); int waited = 0;
    while (!entered && !waited) waited = pthread_cond_timedwait(&changed, &gate, &until);
    pthread_mutex_unlock(&gate); check(!waited && entered, "writer reaches controlled fsync barrier");
    pthread_t release;
    check(!pthread_create(&release, NULL, release_disk, NULL), "release worker from a dedicated test helper");
    check(cr_state_store_load(&e, &first) && restored == 1, "named restore observes the same battery barrier as legacy restore");
    check(!pthread_join(release, NULL), "join durability helper");
    check(cr_engine_close(&e), "close state collection engine"); cr_engine_destroy(&e);
    check(!nftw(root, cleanup, 16, FTW_DEPTH | FTW_PHYS), "clean owned synthetic state fixtures");
    printf("PASS %u named-state, preview, identity, durability and compatibility checks\n", checks);
    return 0;
}
