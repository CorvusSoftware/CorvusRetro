// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
#define _POSIX_C_SOURCE 200809L
#include "state_store.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#define META_SIZE 592u
#define PREVIEW_LIMIT (384u * 256u * 3u + 64u)
static bool fail(char *error, size_t size, const char *message) {
    if (error && size) snprintf(error, size, "%s", message);
    return false;
}
static void clear_error(char *error, size_t size) { if (error && size) error[0] = 0; }
static bool sync_file(int fd) {
    int result;
    do { result = fsync(fd); } while (result < 0 && errno == EINTR);
    return result == 0;
}
static bool symbol(const char *s, size_t cap) {
    size_t n = strnlen(s, cap);
    if (!n || n == cap) return false;
    for (size_t i = 0; i < n; ++i) {
        unsigned char c = (unsigned char)s[i];
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
              (c >= '0' && c <= '9') || c == '_' || c == '-' || c == '.')) return false;
    }
    return true;
}
static bool label(const char *s, size_t cap) {
    size_t n = strnlen(s, cap);
    bool visible = false;
    if (!n || n == cap) return false;
    for (size_t i = 0; i < n;) {
        unsigned char c = (unsigned char)s[i++];
        if (c < 0x20 || c == 0x7f) return false;
        if (c < 0x80) { visible |= c != ' '; continue; }
        visible = true;
        unsigned extra; uint32_t value, minimum;
        if (c >= 0xc2 && c <= 0xdf) { extra = 1; value = c & 31u; minimum = 0x80; }
        else if (c >= 0xe0 && c <= 0xef) { extra = 2; value = c & 15u; minimum = 0x800; }
        else if (c >= 0xf0 && c <= 0xf4) { extra = 3; value = c & 7u; minimum = 0x10000; }
        else return false;
        if (extra > n - i) return false;
        while (extra--) {
            unsigned char next = (unsigned char)s[i++];
            if ((next & 0xc0) != 0x80) return false;
            value = (value << 6) | (next & 63u);
        }
        if (value < minimum || value > 0x10ffff || (value >= 0xd800 && value <= 0xdfff)) return false;
    }
    return visible;
}
static int child_directory(int parent, const char *name, bool create) {
    if (create && mkdirat(parent, name, 0700) == 0) {
        if (!sync_file(parent)) return -1;
    } else if (create && errno != EEXIST) return -1;
    return openat(parent, name, O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
}
/* -2 denotes a collection that has not been created yet. */
static int collection(const char *root, const char *game, bool create, char *error, size_t error_size) {
    if (!root || !root[0] || strnlen(root, 4096) == 4096 || !cr_game_id_valid(game)) {
        fail(error, error_size, "Identité de sauvegarde invalide"); return -1;
    }
    int fd = open(root, O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
    if (fd < 0) {
        if (!create && errno == ENOENT) return -2;
        fail(error, error_size, "Dossier de sauvegarde inaccessible"); return -1;
    }
    const char *components[] = {"states", "library", game};
    for (unsigned i = 0; i < 3; ++i) {
        int next = child_directory(fd, components[i], create);
        int saved = errno;
        close(fd);
        if (next < 0) {
            if (!create && saved == ENOENT) return -2;
            fail(error, error_size, "Collection d’états inaccessible"); return -1;
        }
        fd = next;
    }
    return fd;
}
static bool read_bytes(int dir, const char *name, size_t limit, uint8_t **bytes, size_t *size) {
    *bytes = NULL; *size = 0;
    int fd = openat(dir, name, O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
    if (fd < 0) return false;
    struct stat st;
    bool ok = fstat(fd, &st) == 0 && S_ISREG(st.st_mode) && st.st_size >= 0 &&
              (uint64_t)st.st_size <= limit;
    size_t count = ok ? (size_t)st.st_size : 0;
    uint8_t *copy = ok ? malloc(count + 1) : NULL;
    if (!copy) ok = false;
    size_t at = 0;
    while (ok && at < count) {
        ssize_t n = read(fd, copy + at, count - at);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) { ok = false; break; }
        at += (size_t)n;
    }
    if (close(fd) != 0) ok = false;
    if (!ok) { free(copy); return false; }
    copy[count] = 0;
    *bytes = copy; *size = count;
    return true;
}
static bool write_bytes(int dir, const char *name, const uint8_t *bytes, size_t size) {
    int fd = openat(dir, name, O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW, 0600);
    if (fd < 0) return false;
    size_t at = 0;
    bool ok = true;
    while (at < size) {
        ssize_t n = write(fd, bytes + at, size - at);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) { ok = false; break; }
        at += (size_t)n;
    }
    if (ok) ok = sync_file(fd);
    if (close(fd) != 0) ok = false;
    return ok;
}
static uint64_t checksum(const uint8_t *bytes, size_t size) {
    uint64_t value = UINT64_C(14695981039346656037);
    for (size_t i = 0; i < size; ++i) value = (value ^ bytes[i]) * UINT64_C(1099511628211);
    return value;
}
static void put64(uint8_t *out, uint64_t value, unsigned count) {
    for (unsigned i = 0; i < count; ++i) out[i] = (uint8_t)(value >> (i * 8));
}
static uint64_t get64(const uint8_t *in, unsigned count) {
    uint64_t value = 0;
    for (unsigned i = 0; i < count; ++i) value |= (uint64_t)in[i] << (i * 8);
    return value;
}
static bool valid_entry(const cr_state_entry *e) {
    if (!e || e->slot < 1 || e->slot > CR_STATE_STORE_LIMIT ||
        (e->players != 1 && e->players != 2) || !e->revision || e->created_ms < 0 ||
        !e->size || e->size > CR_STATE_STORE_MAX_BYTES ||
        !memchr(e->game_id, 0, sizeof(e->game_id)) || !cr_game_id_valid(e->game_id) ||
        !symbol(e->core_id, sizeof(e->core_id)) || !label(e->core_version, sizeof(e->core_version)) ||
        !label(e->name, sizeof(e->name))) return false;
    if (e->players == 2)
        return memchr(e->second_id, 0, sizeof(e->second_id)) && cr_game_id_valid(e->second_id) &&
               symbol(e->subsystem_id, sizeof(e->subsystem_id));
    return !e->second_id[0] && !e->subsystem_id[0];
}
static void preview_key(cr_state_entry *entry) {
    snprintf(entry->preview_key, sizeof(entry->preview_key), "state:%s:%u:%016llx",
             entry->game_id, entry->slot, (unsigned long long)entry->revision);
}
static void encode(const cr_state_entry *e, uint64_t hash, uint8_t out[META_SIZE]) {
    memset(out, 0, META_SIZE); memcpy(out, "CRSTATE1", 8);
    put64(out + 8, e->slot, 4); put64(out + 12, e->players, 4);
    put64(out + 16, (uint64_t)e->created_ms, 8); put64(out + 24, e->revision, 8);
    put64(out + 32, e->size, 8); put64(out + 40, hash, 8);
    memcpy(out + 48, e->game_id, sizeof(e->game_id));
    memcpy(out + 144, e->second_id, sizeof(e->second_id));
    memcpy(out + 240, e->subsystem_id, sizeof(e->subsystem_id));
    memcpy(out + 336, e->core_id, sizeof(e->core_id));
    memcpy(out + 432, e->core_version, sizeof(e->core_version));
    memcpy(out + 496, e->name, sizeof(e->name));
}
static bool decode(const uint8_t *in, size_t size, cr_state_entry *e, uint64_t *hash) {
    if (size != META_SIZE || memcmp(in, "CRSTATE1", 8)) return false;
    memset(e, 0, sizeof(*e));
    e->slot = (unsigned)get64(in + 8, 4); e->players = (unsigned)get64(in + 12, 4);
    uint64_t time = get64(in + 16, 8);
    if (time > INT64_MAX) return false;
    e->created_ms = (int64_t)time; e->revision = get64(in + 24, 8);
    e->size = get64(in + 32, 8); *hash = get64(in + 40, 8);
    memcpy(e->game_id, in + 48, sizeof(e->game_id));
    memcpy(e->second_id, in + 144, sizeof(e->second_id));
    memcpy(e->subsystem_id, in + 240, sizeof(e->subsystem_id));
    memcpy(e->core_id, in + 336, sizeof(e->core_id));
    memcpy(e->core_version, in + 432, sizeof(e->core_version));
    memcpy(e->name, in + 496, sizeof(e->name));
    if (!memchr(e->game_id, 0, sizeof(e->game_id)) ||
        !memchr(e->second_id, 0, sizeof(e->second_id)) ||
        !memchr(e->subsystem_id, 0, sizeof(e->subsystem_id)) ||
        !memchr(e->core_id, 0, sizeof(e->core_id)) ||
        !memchr(e->core_version, 0, sizeof(e->core_version)) ||
        !memchr(e->name, 0, sizeof(e->name)) || !valid_entry(e)) return false;
    preview_key(e);
    return true;
}
static int open_entry(int dir, unsigned slot) {
    char name[32]; snprintf(name, sizeof(name), "%u.entry", slot);
    return openat(dir, name, O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
}
static bool metadata(int dir, const char *game, unsigned slot, cr_state_entry *e, uint64_t *hash) {
    uint8_t *bytes = NULL; size_t size = 0;
    bool ok = read_bytes(dir, "meta", META_SIZE, &bytes, &size) && decode(bytes, size, e, hash) &&
              e->slot == slot && !strcmp(e->game_id, game);
    free(bytes);
    struct stat st;
    return ok && fstatat(dir, "state", &st, AT_SYMLINK_NOFOLLOW) == 0 &&
           S_ISREG(st.st_mode) && st.st_size > 0 && (uint64_t)st.st_size == e->size;
}
static bool same_entry(const cr_state_entry *a, const cr_state_entry *b) {
    return a->slot == b->slot && a->revision == b->revision && a->size == b->size &&
           a->players == b->players && !strcmp(a->game_id, b->game_id) &&
           !strcmp(a->second_id, b->second_id) && !strcmp(a->subsystem_id, b->subsystem_id) &&
           !strcmp(a->core_id, b->core_id) && !strcmp(a->core_version, b->core_version);
}
static int checked_entry(int parent, const cr_state_entry *expected, cr_state_entry *actual, uint64_t *hash) {
    int fd = open_entry(parent, expected->slot);
    if (fd < 0) return -1;
    if (!metadata(fd, expected->game_id, expected->slot, actual, hash) || !same_entry(expected, actual)) {
        close(fd); return -1;
    }
    return fd;
}
static int newest(const void *aa, const void *bb) {
    const cr_state_entry *a = aa, *b = bb;
    if (a->created_ms != b->created_ms) return a->created_ms < b->created_ms ? 1 : -1;
    if (a->revision != b->revision) return a->revision < b->revision ? 1 : -1;
    return (a->slot > b->slot) - (a->slot < b->slot);
}
size_t cr_state_store_list(const char *root, const char *game, cr_state_entry *entries, size_t capacity,
                          char *error, size_t error_size) {
    clear_error(error, error_size);
    if (!entries || !capacity) { fail(error, error_size, "Liste d’états invalide"); return 0; }
    int dir = collection(root, game, false, error, error_size);
    if (dir < 0) return 0;
    cr_state_entry found[CR_STATE_STORE_LIMIT]; size_t count = 0;
    for (unsigned slot = 1; slot <= CR_STATE_STORE_LIMIT; ++slot) {
        int fd = open_entry(dir, slot);
        if (fd < 0) continue;
        cr_state_entry e; uint64_t hash;
        if (metadata(fd, game, slot, &e, &hash)) found[count++] = e;
        close(fd);
    }
    close(dir);
    qsort(found, count, sizeof(*found), newest);
    size_t copied = count < capacity ? count : capacity;
    memcpy(entries, found, copied * sizeof(*entries));
    return copied;
}
static bool clock_entry(cr_state_entry *entry) {
    struct timespec now;
    if (clock_gettime(CLOCK_REALTIME, &now) != 0 || now.tv_sec < 0) return false;
    entry->created_ms = (int64_t)now.tv_sec * 1000 + now.tv_nsec / 1000000;
    static uint64_t last;
    uint64_t revision = (uint64_t)now.tv_sec * UINT64_C(1000000000) + (uint64_t)now.tv_nsec;
    entry->revision = revision > last ? revision : last + 1;
    last = entry->revision;
    return entry->revision != 0;
}
static bool valid_preview(const uint8_t *bytes, size_t size) {
    if (!bytes || size < 12 || size > PREVIEW_LIMIT) return false;
    char first[64]; size_t n = size < sizeof(first) - 1 ? size : sizeof(first) - 1;
    memcpy(first, bytes, n); first[n] = 0;
    unsigned w, h;
    if (sscanf(first, "P6 %u %u", &w, &h) != 2 || !w || !h || w > 384 || h > 256) return false;
    char expected[64]; int header = snprintf(expected, sizeof(expected), "P6\n%u %u\n255\n", w, h);
    return header > 0 && (size_t)header < sizeof(expected) &&
           size == (size_t)header + (size_t)w * h * 3 && !memcmp(bytes, expected, (size_t)header);
}
static bool make_preview(const cr_engine *engine, uint8_t **bytes, size_t *size) {
    if (!engine->pixels || engine->width <= 0 || engine->height <= 0) return false;
    unsigned w = (unsigned)engine->width, h = (unsigned)engine->height;
    if (w > 384 || h > 256) {
        double scale_w = 384.0 / w, scale_h = 256.0 / h;
        double scale = scale_w < scale_h ? scale_w : scale_h;
        w = (unsigned)(w * scale); h = (unsigned)(h * scale);
        if (!w) w = 1;
        if (!h) h = 1;
    }
    char header[64]; int n = snprintf(header, sizeof(header), "P6\n%u %u\n255\n", w, h);
    if (n <= 0 || (size_t)n >= sizeof(header)) return false;
    *size = (size_t)n + (size_t)w * h * 3;
    uint8_t *out = malloc(*size);
    if (!out) return false;
    memcpy(out, header, (size_t)n);
    for (unsigned y = 0; y < h; ++y)
        for (unsigned x = 0; x < w; ++x) {
            size_t index = ((size_t)y * (unsigned)engine->height / h) * (unsigned)engine->width +
                           (size_t)x * (unsigned)engine->width / w;
            uint32_t pixel = engine->pixels[index];
            size_t target = (size_t)n + ((size_t)y * w + x) * 3;
            out[target] = (uint8_t)(pixel >> 16);
            out[target + 1] = (uint8_t)(pixel >> 8);
            out[target + 2] = (uint8_t)pixel;
        }
    *bytes = out;
    return true;
}
static void slot_name(char out[32], unsigned slot, const char *suffix) {
    snprintf(out, 32, "%u.%s", slot, suffix);
}
static bool remove_directory(int parent, const char *name) {
    int dir = openat(parent, name, O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
    if (dir < 0) return errno == ENOENT;
    const char *files[] = {"state", "meta", "preview.ppm", "meta.new"};
    bool ok = true;
    for (unsigned i = 0; i < 4; ++i)
        if (unlinkat(dir, files[i], 0) != 0 && errno != ENOENT) ok = false;
    close(dir);
    if (ok && unlinkat(parent, name, AT_REMOVEDIR) != 0) ok = false;
    return ok;
}
static bool commit_entry(int parent, cr_state_entry *entry, uint64_t hash,
                          const uint8_t *state, size_t state_size,
                          const uint8_t *preview, size_t preview_size) {
    unsigned slot;
    for (slot = 1; slot <= CR_STATE_STORE_LIMIT; ++slot) {
        char name[32]; slot_name(name, slot, "entry");
        struct stat status;
        if (fstatat(parent, name, &status, AT_SYMLINK_NOFOLLOW) != 0) {
            if (errno == ENOENT) break;
            return false;
        }
    }
    if (slot > CR_STATE_STORE_LIMIT) { errno = ENOSPC; return false; }
    entry->slot = slot;
    if (!entry->name[0]) snprintf(entry->name, sizeof(entry->name), "État %u", slot);
    if (!clock_entry(entry)) return false;
    preview_key(entry);
    if (!valid_entry(entry)) return false;
    char pending[32], committed[32];
    slot_name(pending, slot, "pending"); slot_name(committed, slot, "entry");
    if (!remove_directory(parent, pending) || mkdirat(parent, pending, 0700) != 0) return false;
    int dir = openat(parent, pending, O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
    if (dir < 0) return false;
    uint8_t meta[META_SIZE]; encode(entry, hash, meta);
    bool ok = write_bytes(dir, "state", state, state_size) &&
              write_bytes(dir, "preview.ppm", preview, preview_size) &&
              write_bytes(dir, "meta", meta, sizeof(meta)) && sync_file(dir);
    if (close(dir) != 0) ok = false;
    if (ok) ok = renameat(parent, pending, parent, committed) == 0;
    if (ok && !sync_file(parent)) {
        /* No previous entry is replaced; remove an unacknowledged new commit if possible. */
        if (renameat(parent, committed, parent, pending) == 0) (void)sync_file(parent);
        ok = false;
    }
    if (!ok) (void)remove_directory(parent, pending);
    return ok;
}
bool cr_state_store_create(cr_engine *engine, const char *name, cr_state_entry *created) {
    if (!engine || !created) return false;
    if (name && name[0] && !label(name, sizeof(created->name)))
        return fail(engine->error, sizeof(engine->error), "Nom d’état invalide");
    corvus_core_info core = {.struct_size = sizeof(core)};
    if (!engine->loaded || !cr_engine_core_info(engine, &core) ||
        strnlen(core.id, sizeof(created->core_id)) == sizeof(created->core_id) ||
        strnlen(core.version, sizeof(created->core_version)) == sizeof(created->core_version))
        return fail(engine->error, sizeof(engine->error), "Aucun contenu compatible chargé");
    cr_state_entry entry = {0};
    entry.players = engine->players;
    snprintf(entry.game_id, sizeof(entry.game_id), "%s", engine->id);
    snprintf(entry.second_id, sizeof(entry.second_id), "%s", engine->second_id);
    snprintf(entry.subsystem_id, sizeof(entry.subsystem_id), "%s", engine->subsystem_id);
    snprintf(entry.core_id, sizeof(entry.core_id), "%s", core.id);
    snprintf(entry.core_version, sizeof(entry.core_version), "%s", core.version);
    if (name) snprintf(entry.name, sizeof(entry.name), "%s", name);
    uint8_t *state = NULL, *preview = NULL; size_t state_size = 0, preview_size = 0;
    if (!cr_engine_state_copy(engine, &state, &state_size)) return false;
    entry.size = state_size;
    bool ok = make_preview(engine, &preview, &preview_size);
    int dir = ok ? collection(engine->root, engine->id, true, engine->error, sizeof(engine->error)) : -1;
    if (ok && dir >= 0) {
        ok = commit_entry(dir, &entry, checksum(state, state_size), state, state_size, preview, preview_size);
        close(dir);
    } else ok = false;
    free(state); free(preview);
    if (!ok) {
        if (!engine->error[0]) fail(engine->error, sizeof(engine->error), "Création de l’état impossible (collection pleine ou stockage indisponible)");
        return false;
    }
    *created = entry; engine->error[0] = 0;
    return true;
}
bool cr_state_store_read(const char *root, const cr_state_entry *entry, uint8_t **bytes, size_t *size,
                         char *error, size_t error_size) {
    clear_error(error, error_size);
    if (!bytes || !size) return fail(error, error_size, "Destination d’état invalide");
    *bytes = NULL; *size = 0;
    if (!valid_entry(entry)) return fail(error, error_size, "Identité d’état invalide");
    int dir = collection(root, entry->game_id, false, error, error_size);
    if (dir < 0) return fail(error, error_size, "État introuvable ou illisible");
    cr_state_entry actual; uint64_t hash;
    int fd = checked_entry(dir, entry, &actual, &hash); close(dir);
    bool ok = fd >= 0 && read_bytes(fd, "state", CR_STATE_STORE_MAX_BYTES, bytes, size);
    if (fd >= 0) close(fd);
    if (ok) ok = *size == entry->size && checksum(*bytes, *size) == hash;
    if (!ok) {
        free(*bytes); *bytes = NULL; *size = 0;
        return fail(error, error_size, "État modifié, incompatible ou incomplet");
    }
    return true;
}
bool cr_state_store_load(cr_engine *engine, const cr_state_entry *entry) {
    if (!engine || !valid_entry(entry)) return engine ? fail(engine->error, sizeof(engine->error), "État invalide") : false;
    corvus_core_info core = {.struct_size = sizeof(core)};
    if (!engine->loaded || !cr_engine_core_info(engine, &core) ||
        strcmp(core.id, entry->core_id) || strcmp(core.version, entry->core_version) ||
        strcmp(engine->id, entry->game_id) || strcmp(engine->second_id, entry->second_id) ||
        strcmp(engine->subsystem_id, entry->subsystem_id) || engine->players != entry->players)
        return fail(engine->error, sizeof(engine->error), "Chargez les contenus et la version du cœur associés à cet état");
    uint8_t *bytes = NULL; size_t size = 0;
    if (!cr_state_store_read(engine->root, entry, &bytes, &size, engine->error, sizeof(engine->error))) return false;
    bool ok = cr_engine_state_restore(engine, bytes, size);
    free(bytes);
    return ok;
}
bool cr_state_store_rename(const char *root, const cr_state_entry *entry, const char *name,
                           char *error, size_t error_size) {
    clear_error(error, error_size);
    if (!valid_entry(entry) || !name || !label(name, sizeof(entry->name)))
        return fail(error, error_size, "Nom d’état invalide");
    int dir = collection(root, entry->game_id, false, error, error_size);
    if (dir < 0) return fail(error, error_size, "État introuvable ou illisible");
    cr_state_entry actual; uint64_t hash;
    int fd = checked_entry(dir, entry, &actual, &hash); close(dir);
    if (fd < 0) return fail(error, error_size, "État modifié ou introuvable");
    snprintf(actual.name, sizeof(actual.name), "%s", name);
    uint8_t meta[META_SIZE]; encode(&actual, hash, meta);
    bool ok = (unlinkat(fd, "meta.new", 0) == 0 || errno == ENOENT) &&
              write_bytes(fd, "meta.new", meta, sizeof(meta)) &&
              renameat(fd, "meta.new", fd, "meta") == 0 && sync_file(fd);
    if (!ok) (void)unlinkat(fd, "meta.new", 0);
    close(fd);
    return ok || fail(error, error_size, "Renommage de l’état impossible");
}
bool cr_state_store_duplicate(const char *root, const cr_state_entry *entry, cr_state_entry *created,
                              char *error, size_t error_size) {
    clear_error(error, error_size);
    if (!created || !valid_entry(entry)) return fail(error, error_size, "État invalide");
    uint8_t *state = NULL, *preview = NULL; size_t size = 0, preview_size = 0;
    if (!cr_state_store_read(root, entry, &state, &size, error, error_size)) return false;
    int dir = collection(root, entry->game_id, false, error, error_size);
    cr_state_entry actual = {0}; uint64_t hash;
    int fd = dir >= 0 ? checked_entry(dir, entry, &actual, &hash) : -1;
    bool ok = fd >= 0 && read_bytes(fd, "preview.ppm", PREVIEW_LIMIT, &preview, &preview_size) &&
              valid_preview(preview, preview_size);
    if (fd >= 0) close(fd);
    if (ok) {
        static const char suffix[] = " (copie)";
        size_t length = strlen(actual.name);
        size_t maximum = sizeof(actual.name) - sizeof(suffix);
        if (length > maximum) {
            length = maximum;
            while (length && ((unsigned char)actual.name[length] & 0xc0) == 0x80) --length;
        }
        memcpy(actual.name + length, suffix, sizeof(suffix));
        ok = commit_entry(dir, &actual, checksum(state, size), state, size, preview, preview_size);
    }
    if (dir >= 0) close(dir);
    free(state); free(preview);
    if (!ok) return fail(error, error_size, "Duplication de l’état impossible (collection pleine ou stockage indisponible)");
    *created = actual;
    return true;
}
bool cr_state_store_delete(const char *root, const cr_state_entry *entry, char *error, size_t error_size) {
    clear_error(error, error_size);
    if (!valid_entry(entry)) return fail(error, error_size, "État invalide");
    int dir = collection(root, entry->game_id, false, error, error_size);
    if (dir < 0) return fail(error, error_size, "État introuvable ou illisible");
    cr_state_entry actual; uint64_t hash;
    int fd = checked_entry(dir, entry, &actual, &hash);
    if (fd < 0) { close(dir); return fail(error, error_size, "État modifié ou introuvable"); }
    close(fd);
    char committed[32], deleted[32];
    slot_name(committed, entry->slot, "entry"); slot_name(deleted, entry->slot, "deleted");
    bool ok = remove_directory(dir, deleted) && renameat(dir, committed, dir, deleted) == 0;
    if (ok && !sync_file(dir)) {
        (void)renameat(dir, deleted, dir, committed);
        (void)sync_file(dir); ok = false;
    }
    if (ok) ok = remove_directory(dir, deleted) && sync_file(dir);
    close(dir);
    return ok || fail(error, error_size, "Suppression de l’état impossible");
}
