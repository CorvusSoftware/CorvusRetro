// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
#define _POSIX_C_SOURCE 200809L
#include "engine.h"
#include "core_registry.h"
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

/* Heap-owned context: remains valid when candidate engine structs are moved. */
struct cr_engine_output {
    uint32_t *pixels;
    uint32_t width, height;
    int16_t audio[4096];
    size_t audio_count;
    bool got_video, failed;
};

static void on_video(void *userdata, const uint32_t *pixels,
                     uint32_t width, uint32_t height, size_t stride) {
    struct cr_engine_output *output = userdata;
    if (!output || !output->pixels || !pixels ||
        width != output->width || height != output->height || stride < width) {
        if (output) output->failed = true;
        return;
    }
    for (size_t row = 0; row < height; ++row)
        memcpy(output->pixels + row * width, pixels + row * stride,
               (size_t)width * sizeof(uint32_t));
    output->got_video = true;
}

static void on_audio(void *userdata, const int16_t *samples, size_t stereo_frames) {
    struct cr_engine_output *output = userdata;
    if (!output || !samples || stereo_frames > (4096 - output->audio_count) / 2) {
        if (output) output->failed = true;
        return;
    }
    size_t count = stereo_frames * 2;
    memcpy(output->audio + output->audio_count, samples, count * sizeof(int16_t));
    output->audio_count += count;
}

#define MAX_ROM (32u * 1024u * 1024u)
#define MAX_BATTERY (1024u * 1024u)
#define MAX_STATE (16u * 1024u * 1024u)

static bool fail(cr_engine *engine, const char *message) {
    snprintf(engine->error, sizeof(engine->error), "%s", message ? message : "Erreur du moteur");
    return false;
}
static bool sdk_error(cr_engine *engine) {
    return fail(engine, engine->api.last_error ? engine->api.last_error(engine->handle)
                                               : "Erreur du cœur CorvusAPI");
}
static void success(cr_engine *engine) {
    engine->error[0] = '\0';
}
static bool valid_system(cr_engine *engine, int system) {
    return cr_registry_get(system) != NULL || fail(engine, "Cœur incompatible ou indisponible");
}
static bool ready(cr_engine *engine) {
    return (engine->handle && engine->loaded) || fail(engine, "Aucun contenu chargé");
}
bool cr_game_id_valid(const char *id) {
    if (!id) return false;
    size_t length = strlen(id);
    if ((length < 66 || length > 82) || id[64] != '-') return false;
    for (size_t i = 0; i < 64; ++i)
        if (!((id[i] >= '0' && id[i] <= '9') ||
              (id[i] >= 'a' && id[i] <= 'f'))) return false;
    if (length == 82) {
        if (id[65] != 'c') return false;
        for (size_t i = 66; i < 82; ++i)
            if (!((id[i] >= '0' && id[i] <= '9') ||
                  (id[i] >= 'a' && id[i] <= 'f'))) return false;
        return true;
    }
    if (id[65] == '0' && length != 66) return false;
    unsigned number = 0;
    for (size_t i = 65; i < length; ++i) {
        if (id[i] < '0' || id[i] > '9') return false;
        number = number * 10u + (unsigned)(id[i] - '0');
        if (number >= CR_MAX_CORES) return false;
    }
    return true;
}
void cr_engine_init(cr_engine *engine, const char *root) {
    memset(engine, 0, sizeof(*engine));
    engine->system = -1;
    if (!root || !root[0] || strlen(root) >= sizeof(engine->root)) {
        fail(engine, "Dossier de sauvegarde invalide");
        return;
    }
    memcpy(engine->root, root, strlen(root) + 1);
}
static bool directory(cr_engine *engine, const char *path) {
    struct stat status;
    if (mkdir(path, 0700) == 0)
        return true;
    if (errno == EEXIST && stat(path, &status) == 0 && S_ISDIR(status.st_mode))
        return true;
    return fail(engine, "Dossier de stockage indisponible");
}
static bool data_path(cr_engine *engine, char path[4096], const char *folder, const char *suffix) {
    if (!engine->root[0] || !cr_game_id_valid(engine->id))
        return fail(engine, "Chemin de sauvegarde invalide");
    if (!directory(engine, engine->root))
        return false;
    int n = snprintf(path, 4096, "%s/%s", engine->root, folder);
    if (n < 0 || n >= 4096 || !directory(engine, path))
        return fail(engine, "Chemin de sauvegarde indisponible");
    n = snprintf(path, 4096, "%s/%s/%s%s", engine->root, folder, engine->id, suffix);
    return (n >= 0 && n < 4096) || fail(engine, "Chemin de sauvegarde trop long");
}
static bool sibling(cr_engine *engine, char out[4096], const char *path, const char *suffix) {
    int n = snprintf(out, 4096, "%s%s", path, suffix);
    return (n >= 0 && n < 4096) || fail(engine, "Chemin de sauvegarde trop long");
}
static bool read_file(cr_engine *engine, const char *path, size_t limit, bool optional,
                      uint8_t **bytes, size_t *size) {
    *bytes = NULL;
    *size = 0;
    char backup[4096];
    if (!sibling(engine, backup, path, ".bak"))
        return false;
    struct stat status;
    if (lstat(backup, &status) == 0) {
        if (!S_ISREG(status.st_mode) || rename(backup, path) != 0)
            return fail(engine, "Récupération de la sauvegarde impossible");
    } else if (errno != ENOENT)
        return fail(engine, "Sauvegarde inaccessible");
    int fd = open(path, O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
    if (fd < 0) {
        if (optional && errno == ENOENT)
            return true;
        return fail(engine, "Sauvegarde introuvable ou illisible");
    }
    if (fstat(fd, &status) != 0 || !S_ISREG(status.st_mode) || status.st_size < 0 ||
        (uint64_t)status.st_size > limit) {
        close(fd);
        return fail(engine, "Taille de sauvegarde invalide");
    }
    size_t count = (size_t)status.st_size;
    uint8_t *data = count ? malloc(count) : NULL;
    if (count && !data) {
        close(fd);
        return fail(engine, "Mémoire insuffisante");
    }
    size_t offset = 0;
    while (offset < count) {
        ssize_t result = read(fd, data + offset, count - offset);
        if (result < 0 && errno == EINTR)
            continue;
        if (result <= 0) {
            free(data);
            close(fd);
            return fail(engine, "Sauvegarde incomplète");
        }
        offset += (size_t)result;
    }
    if (close(fd) != 0) {
        free(data);
        return fail(engine, "Lecture de sauvegarde impossible");
    }
    *bytes = data;
    *size = count;
    return true;
}
static bool sync_fd(int fd) {
    int result;
    do {
        result = fsync(fd);
    } while (result < 0 && errno == EINTR);
    return result == 0;
}
static bool write_file(cr_engine *engine, const char *path, const uint8_t *bytes, size_t size) {
    char temporary[4096];
    if (!sibling(engine, temporary, path, ".new"))
        return false;
    int fd = open(temporary, O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC | O_NOFOLLOW, 0600);
    if (fd < 0)
        return fail(engine, "Écriture de sauvegarde impossible");
    size_t offset = 0;
    bool written = true;
    while (offset < size) {
        ssize_t count = write(fd, bytes + offset, size - offset);
        if (count < 0 && errno == EINTR)
            continue;
        if (count <= 0) {
            written = false;
            break;
        }
        offset += (size_t)count;
    }
    if (written)
        written = sync_fd(fd);
    if (close(fd) != 0)
        written = false;
    if (!written || rename(temporary, path) != 0) {
        unlink(temporary);
        return fail(engine, "Écriture de sauvegarde impossible");
    }
    /* A .bak from the previous Android writer must not supersede this commit. */
    char backup[4096];
    if (!sibling(engine, backup, path, ".bak"))
        return false;
    if (unlink(backup) != 0 && errno != ENOENT)
        return fail(engine, "Finalisation de sauvegarde impossible");
    char parent[4096];
    memcpy(parent, path, strlen(path) + 1);
    char *last = strrchr(parent, '/');
    if (!last)
        return fail(engine, "Dossier de sauvegarde invalide");
    *last = '\0';
    fd = open(parent, O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    if (fd < 0)
        return fail(engine, "Finalisation de sauvegarde impossible");
    bool synced = sync_fd(fd);
    if (close(fd) != 0)
        synced = false;
    return synced || fail(engine, "Finalisation de sauvegarde impossible");
}
#define MAX_OPTION_RECORDS 256u
#define MAX_CONFIG (MAX_OPTION_RECORDS * 192u + 32u)
struct cr_option_record { char key[96], value[96]; };
struct cr_engine_options {
    size_t count;
    struct cr_option_record records[MAX_OPTION_RECORDS];
};

static bool token(const char *text) {
    if (!text) return false;
    size_t n = strnlen(text, 96);
    if (!n || n == 96) return false;
    for (size_t i = 0; i < n; ++i) {
        unsigned char c = (unsigned char)text[i];
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
              (c >= '0' && c <= '9') || c == '_' || c == '-' || c == '.')) return false;
    }
    return true;
}
static bool text_valid(const char *text) {
    if (!text) return false;
    size_t length = strnlen(text, 256);
    if (length == 256) return false;
    for (size_t i = 0; i < length;) {
        unsigned char first = (unsigned char)text[i++];
        if (first < 0x80) continue;
        unsigned count;
        uint32_t code, minimum;
        if (first >= 0xc2 && first <= 0xdf) { count = 1; code = first & 31u; minimum = 0x80; }
        else if (first >= 0xe0 && first <= 0xef) { count = 2; code = first & 15u; minimum = 0x800; }
        else if (first >= 0xf0 && first <= 0xf4) { count = 3; code = first & 7u; minimum = 0x10000; }
        else return false;
        if (count > length - i) return false;
        while (count--) {
            unsigned char next = (unsigned char)text[i++];
            if ((next & 0xc0) != 0x80) return false;
            code = (code << 6) | (next & 63u);
        }
        if (code < minimum || code > 0x10ffff || (code >= 0xd800 && code <= 0xdfff)) return false;
    }
    return true;
}
static bool option_valid(const corvus_core_option *option) {
    if (option->struct_size < sizeof(*option) || !token(option->key) ||
        !text_valid(option->label) || !text_valid(option->description) ||
        (option->flags & ~CORVUS_OPTION_RESTART_REQUIRED) || !option->values ||
        !option->value_count || option->value_count > 64 ||
        option->default_index >= option->value_count) return false;
    for (uint32_t i = 0; i < option->value_count; ++i) {
        const corvus_option_value *v = &option->values[i];
        if (v->struct_size < sizeof(*v) || !token(v->id) || !text_valid(v->label)) return false;
        for (uint32_t j = 0; j < i; ++j)
            if (!strcmp(v->id, option->values[j].id)) return false;
    }
    return true;
}
size_t cr_engine_option_count(const cr_engine *engine) {
    if (!engine || !engine->handle || !engine->options.count) return 0;
    uint32_t count = engine->options.count(engine->handle);
    return count <= 64 ? count : 0;
}
bool cr_engine_option(const cr_engine *engine, size_t index, corvus_core_option *out) {
    if (!out || index >= cr_engine_option_count(engine)) return false;
    corvus_core_option option = {.struct_size = sizeof(option)};
    if (!engine->options.describe(engine->handle, (uint32_t)index, &option) ||
        !option_valid(&option)) return false;
    *out = option;
    return true;
}
uint32_t cr_engine_option_value(const cr_engine *engine, const char *key) {
    if (!engine || !engine->handle || !engine->options.get_value || !token(key)) return UINT32_MAX;
    for (size_t i = 0; i < cr_engine_option_count(engine); ++i) {
        corvus_core_option option;
        if (cr_engine_option(engine, i, &option) && !strcmp(key, option.key)) {
            uint32_t value = engine->options.get_value(engine->handle, key);
            return value < option.value_count ? value : UINT32_MAX;
        }
    }
    return UINT32_MAX;
}
static bool subsystem_valid(const corvus_subsystem_info *info) {
    if (info->struct_size < sizeof(*info) || !token(info->id) || !text_valid(info->label) ||
        !info->num_players || info->num_players > 4 || !info->game_count ||
        info->game_count > 4 || !info->games) return false;
    for (uint32_t i = 0; i < info->game_count; ++i) {
        const corvus_subsystem_rom *game = &info->games[i];
        if (game->struct_size < sizeof(*game) || !text_valid(game->label) ||
            !text_valid(game->extensions) || game->required > 1) return false;
        size_t n = strlen(game->extensions);
        if (!n || game->extensions[0] == '|' || game->extensions[n - 1] == '|') return false;
        for (size_t j = 0; j < n; ++j) {
            unsigned char c = (unsigned char)game->extensions[j];
            if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                  (c >= '0' && c <= '9') || c == '_' || c == '-' || c == '|') ||
                (c == '|' && j && game->extensions[j - 1] == '|')) return false;
        }
    }
    return true;
}
size_t cr_engine_subsystem_count(const cr_engine *engine) {
    if (!engine || !engine->handle || !engine->subsystems.count) return 0;
    uint32_t count = engine->subsystems.count();
    return count <= 16 ? count : 0;
}
bool cr_engine_subsystem(const cr_engine *engine, size_t index, corvus_subsystem_info *out) {
    if (!out || index >= cr_engine_subsystem_count(engine)) return false;
    corvus_subsystem_info info = {.struct_size = sizeof(info)};
    if (!engine->subsystems.describe((uint32_t)index, &info) || !subsystem_valid(&info)) return false;
    *out = info;
    return true;
}
static bool options_path(cr_engine *engine, char path[4096]) {
    const cr_core_descriptor *core = cr_registry_get(engine->system);
    if (!core || !token(core->id) || !directory(engine, engine->root))
        return fail(engine, "Dossier des options indisponible");
    int n = snprintf(path, 4096, "%s/options", engine->root);
    if (n < 0 || n >= 4096 || !directory(engine, path)) return false;
    n = snprintf(path, 4096, "%s/options/%s.cfg", engine->root, core->id);
    return (n > 0 && n < 4096) || fail(engine, "Chemin des options trop long");
}
static bool parse_options(struct cr_engine_options *saved, uint8_t *bytes, size_t size) {
    static const char magic[] = "CORVUS_OPTIONS_1\n";
    if (size < sizeof(magic) - 1 || memcmp(bytes, magic, sizeof(magic) - 1)) return false;
    size_t offset = sizeof(magic) - 1;
    while (offset < size) {
        if (saved->count == MAX_OPTION_RECORDS) return false;
        struct cr_option_record record = {0};
        size_t key = 0, value = 0;
        while (offset < size && bytes[offset] != '=') {
            if (key == 95 || !bytes[offset]) return false;
            record.key[key++] = (char)bytes[offset++];
        }
        if (offset == size) return false;
        ++offset;
        while (offset < size && bytes[offset] != '\n') {
            if (value == 95 || !bytes[offset]) return false;
            record.value[value++] = (char)bytes[offset++];
        }
        if (offset == size || !token(record.key) || !token(record.value)) return false;
        ++offset;
        for (size_t i = 0; i < saved->count; ++i)
            if (!strcmp(saved->records[i].key, record.key)) return false;
        saved->records[saved->count++] = record;
    }
    return true;
}
static bool load_options(cr_engine *engine) {
    if (!engine->options.count) return true;
    engine->saved_options = calloc(1, sizeof(*engine->saved_options));
    if (!engine->saved_options) return fail(engine, "Mémoire insuffisante");
    char path[4096];
    uint8_t *bytes = NULL;
    size_t size = 0;
    /* Bad or unavailable configuration must never prevent playing a core. */
    bool read = options_path(engine, path) &&
        read_file(engine, path, MAX_CONFIG, true, &bytes, &size);
    if (read && size && !parse_options(engine->saved_options, bytes, size))
        memset(engine->saved_options, 0, sizeof(*engine->saved_options));
    free(bytes);
    for (size_t i = 0; i < cr_engine_option_count(engine); ++i) {
        corvus_core_option option;
        if (!cr_engine_option(engine, i, &option)) return fail(engine, "Options du cœur invalides");
        for (size_t j = 0; j < engine->saved_options->count; ++j) {
            const struct cr_option_record *record = &engine->saved_options->records[j];
            if (strcmp(record->key, option.key)) continue;
            for (uint32_t v = 0; v < option.value_count; ++v)
                if (!strcmp(record->value, option.values[v].id) &&
                    !engine->options.set_value(engine->handle, option.key, v))
                    return sdk_error(engine);
        }
    }
    success(engine);
    return true;
}
static bool remove_config_backup(cr_engine *engine, const char *path) {
    struct stat status;
    if (lstat(path, &status) != 0)
        return errno == ENOENT || fail(engine, "Fichier des options inaccessible");
    if (!S_ISREG(status.st_mode) || unlink(path) != 0)
        return fail(engine, "Finalisation des options impossible");
    return true;
}
static bool stage_config(cr_engine *engine, const char *path, const uint8_t *bytes, size_t size) {
    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC | O_NOFOLLOW, 0600);
    if (fd < 0) return fail(engine, "Écriture des options impossible");
    bool ok = true;
    size_t offset = 0;
    while (offset < size) {
        ssize_t count = write(fd, bytes + offset, size - offset);
        if (count < 0 && errno == EINTR) continue;
        if (count <= 0) { ok = false; break; }
        offset += (size_t)count;
    }
    if (ok) ok = sync_fd(fd);
    if (close(fd) != 0) ok = false;
    return ok || fail(engine, "Écriture des options impossible");
}
static bool write_config(cr_engine *engine, const char *path, const uint8_t *bytes, size_t size) {
    char temporary[4096], backup[4096], parent[4096];
    if (!sibling(engine, temporary, path, ".new") ||
        !sibling(engine, backup, path, ".bak")) return false;
    memcpy(parent, path, strlen(path) + 1);
    char *last = strrchr(parent, '/');
    if (!last) return fail(engine, "Dossier des options invalide");
    *last = '\0';
    int parent_fd = open(parent, O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    if (parent_fd < 0) return fail(engine, "Dossier des options inaccessible");
    /* Reject a .bak obstruction before publication, unlike the battery writer.
     * Keep raw old bytes for rollback even after backup cleanup has begun. */
    bool ok = remove_config_backup(engine, backup);
    struct stat status;
    bool existed = false, linked = false;
    if (ok && lstat(path, &status) == 0) {
        existed = true;
        if (!S_ISREG(status.st_mode)) ok = fail(engine, "Fichier des options invalide");
    } else if (ok && errno != ENOENT) ok = fail(engine, "Fichier des options inaccessible");
    uint8_t *previous = NULL;
    size_t previous_size = 0;
    if (ok && existed) ok = read_file(engine, path, MAX_CONFIG, false, &previous, &previous_size);
    if (ok) ok = stage_config(engine, temporary, bytes, size);
    if (ok && existed) {
        linked = link(path, backup) == 0;
        if (!linked) ok = fail(engine, "Protection des anciennes options impossible");
    }
    if (ok && !sync_fd(parent_fd)) ok = fail(engine, "Synchronisation des options impossible");
    bool published = false;
    if (ok) {
        published = rename(temporary, path) == 0;
        if (!published) ok = fail(engine, "Écriture des options impossible");
    }
    if (ok && !sync_fd(parent_fd)) ok = false;
    if (ok && linked) {
        if (unlink(backup) == 0) linked = false;
        else ok = false;
    }
    if (ok && !sync_fd(parent_fd)) ok = false;
    if (published && !ok) {
        bool restored;
        if (!existed) restored = unlink(path) == 0;
        else if (linked && rename(backup, path) == 0) { restored = true; linked = false; }
        else restored = stage_config(engine, temporary, previous, previous_size) && rename(temporary, path) == 0;
        if (restored && linked) {
            restored = unlink(backup) == 0;
            if (restored) linked = false;
        }
        bool durable = restored && sync_fd(parent_fd);
        /* A filesystem that rejects recovery cannot promise either durable
         * outcome. Retain a recovery copy when possible for read_file(). */
        if (!durable && existed && !linked) {
            if (stage_config(engine, temporary, previous, previous_size) && rename(temporary, backup) == 0)
                sync_fd(parent_fd);
        }
        fail(engine, durable ? "Écriture des options annulée" :
             "Échec des options : restauration durable non garantie");
    }
    if (!published) {
        unlink(temporary);
        if (linked) unlink(backup);
    }
    free(previous);
    close(parent_fd);
    return ok;
}
static bool persist_options(cr_engine *engine, const struct cr_engine_options *saved) {
    char *bytes = malloc(MAX_CONFIG);
    if (!bytes) return fail(engine, "Mémoire insuffisante");
    size_t offset = (size_t)snprintf(bytes, MAX_CONFIG, "CORVUS_OPTIONS_1\n");
    for (size_t i = 0; i < saved->count; ++i) {
        int n = snprintf(bytes + offset, MAX_CONFIG - offset, "%s=%s\n",
                         saved->records[i].key, saved->records[i].value);
        if (n < 0 || (size_t)n >= MAX_CONFIG - offset) {
            free(bytes);
            return fail(engine, "Trop d'options enregistrées");
        }
        offset += (size_t)n;
    }
    char path[4096];
    bool ok = options_path(engine, path) && write_config(engine, path, (uint8_t *)bytes, offset);
    free(bytes);
    return ok;
}
bool cr_engine_set_option(cr_engine *engine, const char *key, uint32_t value) {
    if (!engine || !engine->saved_options || !token(key)) return false;
    corvus_core_option option;
    bool found = false;
    for (size_t i = 0; i < cr_engine_option_count(engine); ++i)
        if (cr_engine_option(engine, i, &option) && !strcmp(option.key, key)) { found = true; break; }
    if (!found || value >= option.value_count) return fail(engine, "Option invalide");
    uint32_t old = cr_engine_option_value(engine, key);
    if (old == UINT32_MAX) return fail(engine, "Valeur d'option invalide");
    struct cr_engine_options *next = malloc(sizeof(*next));
    if (!next) return fail(engine, "Mémoire insuffisante");
    *next = *engine->saved_options;
    size_t index = 0;
    while (index < next->count && strcmp(next->records[index].key, key)) ++index;
    if (index == MAX_OPTION_RECORDS) { free(next); return fail(engine, "Trop d'options enregistrées"); }
    if (index == next->count) ++next->count;
    snprintf(next->records[index].key, sizeof(next->records[index].key), "%s", key);
    snprintf(next->records[index].value, sizeof(next->records[index].value), "%s", option.values[value].id);
    if (!engine->options.set_value(engine->handle, key, value)) { free(next); return sdk_error(engine); }
    if (!persist_options(engine, next)) {
        /* The public setter guarantees valid selections are atomic. */
        engine->options.set_value(engine->handle, key, old);
        free(next);
        return false;
    }
    free(engine->saved_options);
    engine->saved_options = next;
    success(engine);
    return true;
}
static void negotiate_configuration(cr_engine *engine) {
    void *symbol = dlsym(engine->module, CORVUS_OPTIONS_ENTRYPOINT);
    corvus_get_options_api_fn options = NULL;
    if (symbol) memcpy(&options, &symbol, sizeof(options));
    engine->options.struct_size = sizeof(engine->options);
    if (!options || !options(CORVUS_OPTIONS_API_VERSION, &engine->options) ||
        engine->options.struct_size < sizeof(engine->options) ||
        engine->options.api_version != CORVUS_OPTIONS_API_VERSION ||
        !engine->options.count || !engine->options.describe ||
        !engine->options.get_value || !engine->options.set_value)
        memset(&engine->options, 0, sizeof(engine->options));
    symbol = dlsym(engine->module, CORVUS_SUBSYSTEM_ENTRYPOINT);
    corvus_get_subsystem_api_fn subsystems = NULL;
    if (symbol) memcpy(&subsystems, &symbol, sizeof(subsystems));
    engine->subsystems.struct_size = sizeof(engine->subsystems);
    if (!subsystems || !subsystems(CORVUS_SUBSYSTEM_API_VERSION, &engine->subsystems) ||
        engine->subsystems.struct_size < sizeof(engine->subsystems) ||
        engine->subsystems.api_version != CORVUS_SUBSYSTEM_API_VERSION ||
        !engine->subsystems.count || !engine->subsystems.describe ||
        !engine->subsystems.load || !engine->subsystems.capabilities) {
        memset(&engine->subsystems, 0, sizeof(engine->subsystems));
    } else {
        bool any = engine->subsystems.battery_size || engine->subsystems.battery_dirty ||
                   engine->subsystems.battery_copy || engine->subsystems.battery_acknowledge;
        bool all = engine->subsystems.battery_size && engine->subsystems.battery_dirty &&
                   engine->subsystems.battery_copy && engine->subsystems.battery_acknowledge;
        if (any && !all) memset(&engine->subsystems, 0, sizeof(engine->subsystems));
    }
}
static void validate_configuration(cr_engine *engine) {
    if (engine->options.count) {
        uint32_t count = engine->options.count(engine->handle);
        bool valid = count <= 64;
        for (uint32_t i = 0; valid && i < count; ++i) {
            corvus_core_option option;
            valid = cr_engine_option(engine, i, &option) &&
                engine->options.get_value(engine->handle, option.key) < option.value_count;
            for (uint32_t j = 0; valid && j < i; ++j) {
                corvus_core_option previous;
                valid = cr_engine_option(engine, j, &previous) && strcmp(previous.key, option.key);
            }
        }
        if (!valid) memset(&engine->options, 0, sizeof(engine->options));
    }
    if (engine->subsystems.count) {
        uint32_t count = engine->subsystems.count();
        bool valid = count <= 16;
        for (uint32_t i = 0; valid && i < count; ++i) {
            corvus_subsystem_info info;
            valid = cr_engine_subsystem(engine, i, &info);
            for (uint32_t j = 0; valid && j < i; ++j) {
                corvus_subsystem_info previous;
                valid = cr_engine_subsystem(engine, j, &previous) && strcmp(previous.id, info.id);
            }
        }
        if (!valid) memset(&engine->subsystems, 0, sizeof(engine->subsystems));
    }
}

static void release_handle(cr_engine *engine) {
    if (engine->handle && engine->api.destroy)
        engine->api.destroy(engine->handle);
    if (engine->module)
        dlclose(engine->module);
    free(engine->saved_options);
    engine->saved_options = NULL;
    free(engine->output);
    free(engine->pixels);
    engine->handle = NULL;
    engine->module = NULL;
    engine->module_path[0] = '\0';
    engine->output = NULL;
    engine->pixels = NULL;
    memset(&engine->api, 0, sizeof(engine->api));
    memset(&engine->achievements, 0, sizeof(engine->achievements));
    memset(&engine->options, 0, sizeof(engine->options));
    memset(&engine->subsystems, 0, sizeof(engine->subsystems));
    engine->players = 0;
    engine->second_id[0] = engine->subsystem_id[0] = '\0';
    engine->loaded = false;
    engine->system = -1;
    engine->width = engine->height = engine->rate = 0;
    engine->fps = 0;
    engine->audio_count = 0;
    engine->id[0] = '\0';
}

static bool create_handle(cr_engine *engine, int system) {
    const cr_core_descriptor *descriptor = cr_registry_get(system);
    if (!descriptor) return fail(engine, "Cœur inconnu ou incompatible");
    char module_path[4096];
#if defined(__ANDROID__)
    int n = snprintf(module_path, sizeof(module_path), "%s", descriptor->module);
#else
    int n = snprintf(module_path, sizeof(module_path), "%s%s",
                     descriptor->module[0] == '/' ? "" : "./", descriptor->module);
#endif
    if (n <= 0 || (size_t)n >= sizeof(module_path))
        return fail(engine, "Nom de module invalide");
    engine->module = dlopen(module_path, RTLD_NOW | RTLD_LOCAL);
    if (!engine->module)
        return fail(engine, "Module CorvusAPI introuvable");
    snprintf(engine->module_path, sizeof(engine->module_path), "%s", descriptor->module);
    corvus_get_api_fn entry = NULL;
    void *symbol = dlsym(engine->module, CORVUS_CORE_ENTRYPOINT);
    if (!symbol) return fail(engine, "Entrée CorvusAPI absente");
    memcpy(&entry, &symbol, sizeof(entry));
    engine->api.struct_size = sizeof(engine->api);
    if (!entry(CORVUS_API_VERSION, &engine->api) ||
        engine->api.api_version != CORVUS_API_VERSION ||
        engine->api.struct_size < sizeof(engine->api) ||
        !engine->api.get_info || !engine->api.create || !engine->api.destroy ||
        !engine->api.load_game || !engine->api.unload_game ||
        !engine->api.get_av_info || !engine->api.set_button || !engine->api.run_frame)
        return fail(engine, "Version ou fonctions CorvusAPI incompatibles");
    if (!(engine->api.serialize_size && engine->api.serialize && engine->api.unserialize)) {
        engine->api.serialize_size = NULL;
        engine->api.serialize = NULL;
        engine->api.unserialize = NULL;
    }
    if (!(engine->api.battery_size && engine->api.battery_dirty &&
          engine->api.battery_copy && engine->api.battery_acknowledge)) {
        engine->api.battery_size = NULL;
        engine->api.battery_dirty = NULL;
        engine->api.battery_copy = NULL;
        engine->api.battery_acknowledge = NULL;
    }
    corvus_core_info info = {.struct_size = sizeof(info)};
    if (!engine->api.get_info(&info) || !info.id ||
        strcmp(info.id, descriptor->id) != 0)
        return fail(engine, "Identifiant de cœur CorvusAPI invalide");
    corvus_get_achievements_api_fn get_achievements = NULL;
    symbol = dlsym(engine->module, CORVUS_ACHIEVEMENTS_ENTRYPOINT);
    if (symbol) memcpy(&get_achievements, &symbol, sizeof(get_achievements));
    engine->achievements.struct_size = sizeof(engine->achievements);
    if (!get_achievements || !get_achievements(CORVUS_ACHIEVEMENTS_API_VERSION, &engine->achievements) ||
        engine->achievements.api_version != CORVUS_ACHIEVEMENTS_API_VERSION ||
        engine->achievements.struct_size < sizeof(engine->achievements) ||
        !engine->achievements.read_memory)
        memset(&engine->achievements, 0, sizeof(engine->achievements));
    negotiate_configuration(engine);
    engine->output = calloc(1, sizeof(*engine->output));
    if (!engine->output) return fail(engine, "Mémoire insuffisante");
    corvus_host host = {.struct_size = sizeof(host), .userdata = engine->output,
                        .video_frame = on_video, .audio_frames = on_audio};
    engine->handle = engine->api.create(&host);
    if (!engine->handle) return sdk_error(engine);
    engine->system = system;
    validate_configuration(engine);
    return load_options(engine);
}
static bool mapped_load(cr_engine *engine, const char *path, const uint8_t *battery,
                        size_t battery_size) {
    if (!path || !path[0])
        return fail(engine, "Contenu absent");
    int fd = open(path, O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
    if (fd < 0)
        return fail(engine, "Contenu illisible");
    struct stat status;
    if (fstat(fd, &status) != 0 || !S_ISREG(status.st_mode) || status.st_size <= 0 ||
        (uint64_t)status.st_size > MAX_ROM) {
        close(fd);
        return fail(engine, "Taille de ROM invalide (maximum 32 Mio)");
    }
    size_t size = (size_t)status.st_size;
    void *data = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);
    if (data == MAP_FAILED)
        return fail(engine, "Lecture du contenu impossible");
    corvus_game game = {.struct_size = sizeof(game), .rom_data = data, .rom_size = size,
                        .battery_data = battery, .battery_size = battery_size};
    int loaded = engine->api.load_game(engine->handle, &game);
    if (!loaded)
        sdk_error(engine);
    munmap(data, size);
    return loaded != 0;
}
bool cr_engine_check(cr_engine *engine, int system, const char *path) {
    cr_engine candidate;
    cr_engine_init(&candidate, engine->root);
    bool result = create_handle(&candidate, system) && mapped_load(&candidate, path, NULL, 0);
    if (result)
        success(engine);
    else
        fail(engine, candidate.error);
    release_handle(&candidate);
    return result;
}
bool cr_engine_core_info(const cr_engine *engine, corvus_core_info *out) {
    if (!engine || !engine->handle || !engine->api.get_info || !out) return false;
    corvus_core_info info = {.struct_size = sizeof(info)};
    if (!engine->api.get_info(&info) || info.struct_size < sizeof(info) ||
        !info.id || !info.name || !info.version || !info.system_name || !info.extensions)
        return false;
    *out = info;
    return true;
}
bool cr_engine_select(cr_engine *engine, int system) {
    if (!valid_system(engine, system))
        return false;
    const cr_core_descriptor *descriptor = cr_registry_get(system);
    if (engine->handle && !engine->loaded && engine->system == system &&
        !strcmp(engine->module_path, descriptor->module)) {
        success(engine);
        return true;
    }
    cr_engine candidate;
    cr_engine_init(&candidate, engine->root);
    if (!create_handle(&candidate, system)) {
        fail(engine, candidate.error);
        release_handle(&candidate);
        return false;
    }
    if (!cr_engine_save_battery(engine)) {
        release_handle(&candidate);
        return false;
    }
    release_handle(engine);
    *engine = candidate;
    success(engine);
    return true;
}
static bool content_matches(int system, const char *id) {
    const cr_core_descriptor *core = cr_registry_get(system);
    if (!core || !cr_game_id_valid(id)) return false;
    char expected[96], hash[65];
    memcpy(hash, id, 64);
    hash[64] = '\0';
    return cr_registry_content_id(expected, sizeof(expected), hash, core->id) && !strcmp(id, expected);
}
static bool prepare_output(cr_engine *candidate) {
    corvus_av_info av = {.struct_size = sizeof(av)};
    if (!candidate->api.get_av_info(candidate->handle, &av)) return sdk_error(candidate);
    if (av.struct_size < sizeof(av) || !av.width || !av.height || av.width > 1024 ||
        av.height > 1024 || !av.audio_sample_rate || av.audio_sample_rate > 384000 ||
        !isfinite(av.fps) || av.fps <= 0 || av.fps > 1000)
        return fail(candidate, "Format vidéo ou audio invalide");
    candidate->width = (int)av.width;
    candidate->height = (int)av.height;
    candidate->rate = (int)av.audio_sample_rate;
    candidate->fps = av.fps;
    candidate->pixels = calloc((size_t)av.width * av.height, sizeof(*candidate->pixels));
    if (!candidate->pixels) return fail(candidate, "Mémoire insuffisante");
    candidate->output->pixels = candidate->pixels;
    candidate->output->width = av.width;
    candidate->output->height = av.height;
    candidate->loaded = true;
    return true;
}
static bool battery_path(cr_engine *engine, uint32_t slot, char path[4096]) {
    if (slot == 0) return data_path(engine, path, "saves", ".sav");
    if (slot != 1 || !cr_game_id_valid(engine->second_id))
        return fail(engine, "Identifiant du second contenu invalide");
    /* Create/check the shared directory through the unchanged P1 path. */
    if (!data_path(engine, path, "saves", ".sav")) return false;
    int n = snprintf(path, 4096, "%s/saves/%s-link-p2.sav", engine->root, engine->second_id);
    return (n > 0 && n < 4096) || fail(engine, "Chemin de sauvegarde trop long");
}
struct mapped_game { corvus_game game; uint8_t *battery; };
static bool map_game(cr_engine *engine, struct mapped_game *mapped, const char *path,
                     uint32_t slot) {
    mapped->game.struct_size = sizeof(mapped->game);
    if (!path || !path[0]) return fail(engine, "Contenu absent");
    char saved_path[4096];
    if (!battery_path(engine, slot, saved_path) ||
        !read_file(engine, saved_path, MAX_BATTERY, true, &mapped->battery,
                   &mapped->game.battery_size)) return false;
    mapped->game.battery_data = mapped->battery;
    int fd = open(path, O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
    if (fd < 0) return fail(engine, "Contenu illisible");
    struct stat status;
    if (fstat(fd, &status) || !S_ISREG(status.st_mode) || status.st_size <= 0 ||
        (uint64_t)status.st_size > MAX_ROM) {
        close(fd);
        return fail(engine, "Taille de ROM invalide (maximum 32 Mio)");
    }
    size_t size = (size_t)status.st_size;
    void *data = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);
    if (data == MAP_FAILED) return fail(engine, "Lecture du contenu impossible");
    mapped->game.rom_data = data;
    mapped->game.rom_size = size;
    return true;
}
static void unmap_game(struct mapped_game *mapped) {
    if (mapped->game.rom_data) munmap((void *)mapped->game.rom_data, mapped->game.rom_size);
    free(mapped->battery);
}
static bool open_session(cr_engine *engine, int system, const char *subsystem_id,
                         const char *ids[2], const char *paths[2]) {
    if (!valid_system(engine, system)) return false;
    bool linked = subsystem_id != NULL;
    if (!ids || !paths || !content_matches(system, ids[0]) ||
        (linked && (!token(subsystem_id) || !content_matches(system, ids[1]))))
        return fail(engine, "Identifiant de contenu ou de sous-système invalide");
    cr_engine candidate;
    cr_engine_init(&candidate, engine->root);
    bool ok = create_handle(&candidate, system);
    if (ok && linked) {
        bool supported = false;
        for (size_t i = 0; i < cr_engine_subsystem_count(&candidate); ++i) {
            corvus_subsystem_info info;
            if (cr_engine_subsystem(&candidate, i, &info) && !strcmp(info.id, subsystem_id) &&
                info.game_count == 2 && info.num_players == 2) { supported = true; break; }
        }
        if (!supported || !candidate.subsystems.battery_size)
            ok = fail(&candidate, "Sous-système local à deux joueurs indisponible");
    }
    if (ok) {
        snprintf(candidate.id, sizeof(candidate.id), "%s", ids[0]);
        candidate.players = linked ? 2 : 1;
        if (linked) {
            snprintf(candidate.second_id, sizeof(candidate.second_id), "%s", ids[1]);
            snprintf(candidate.subsystem_id, sizeof(candidate.subsystem_id), "%s", subsystem_id);
        }
        /* Flush before reading either slot, including reopening the same ROM. */
        if (!cr_engine_save_battery(engine)) { release_handle(&candidate); return false; }
    }
    struct mapped_game mapped[2] = {0};
    for (uint32_t i = 0; ok && i < candidate.players; ++i)
        ok = map_game(&candidate, &mapped[i], paths[i], i);
    if (ok) {
        corvus_game games[2] = {mapped[0].game, mapped[1].game};
        int loaded = linked ? candidate.subsystems.load(candidate.handle, subsystem_id, games, 2)
                            : candidate.api.load_game(candidate.handle, &games[0]);
        if (!loaded) ok = sdk_error(&candidate);
    }
    for (size_t i = 0; i < 2; ++i) unmap_game(&mapped[i]);
    if (ok) ok = prepare_output(&candidate);
    if (!ok) {
        fail(engine, candidate.error);
        release_handle(&candidate);
        return false;
    }
    release_handle(engine);
    *engine = candidate;
    success(engine);
    return true;
}
bool cr_engine_open(cr_engine *engine, int system, const char *id, const char *path) {
    const char *ids[2] = {id, NULL}, *paths[2] = {path, NULL};
    return open_session(engine, system, NULL, ids, paths);
}
bool cr_engine_open_subsystem(cr_engine *engine, int system, const char *subsystem_id,
                              const char *ids[2], const char *paths[2]) {
    if (!subsystem_id) return fail(engine, "Sous-système absent");
    return open_session(engine, system, subsystem_id, ids, paths);
}
bool cr_engine_can_reset(const cr_engine *engine) {
    return engine && engine->handle && engine->loaded && engine->api.reset &&
        (!engine->subsystems.capabilities ||
         (engine->subsystems.capabilities(engine->handle) & CORVUS_SESSION_RESET));
}
bool cr_engine_can_state(const cr_engine *engine) {
    return engine && engine->handle && engine->loaded && engine->api.serialize_size &&
        engine->api.serialize && engine->api.unserialize &&
        (!engine->subsystems.capabilities ||
         (engine->subsystems.capabilities(engine->handle) & CORVUS_SESSION_STATES));
}
size_t cr_engine_read_achievement_memory(void *context, uint32_t address, uint8_t *buffer, size_t capacity) {
    cr_engine *engine = context;
    if (!engine || engine->players > 1 || !engine->loaded || !engine->handle || !engine->achievements.read_memory ||
        !buffer || !capacity) return 0;
    size_t copied = engine->achievements.read_memory(engine->handle, address, buffer, capacity);
    return copied <= capacity ? copied : 0;
}
bool cr_engine_step(cr_engine *engine, unsigned mask) {
    const unsigned masks[2] = {mask, 0};
    return cr_engine_step_players(engine, masks);
}
bool cr_engine_step_players(cr_engine *engine, const unsigned masks[2]) {
    if (!ready(engine)) return false;
    if (!masks || (masks[0] & ~1023u) || (masks[1] & ~1023u))
        return fail(engine, "Boutons invalides");
    static const uint32_t input_map[10] = {
        CORVUS_BUTTON_UP, CORVUS_BUTTON_DOWN, CORVUS_BUTTON_LEFT,
        CORVUS_BUTTON_RIGHT, CORVUS_BUTTON_A, CORVUS_BUTTON_B,
        CORVUS_BUTTON_START, CORVUS_BUTTON_SELECT, CORVUS_BUTTON_L, CORVUS_BUTTON_R
    };
    struct cr_engine_output *output = engine->output;
    output->got_video = false;
    output->failed = false;
    output->audio_count = 0;
    for (unsigned player = 0; player < engine->players; ++player)
        for (int button = 0; button < 10; ++button)
            engine->api.set_button(engine->handle, player, input_map[button],
                                   (masks[player] & (1u << button)) != 0);
    if (!engine->api.run_frame(engine->handle))
        return sdk_error(engine);
    if (output->failed || !output->got_video)
        return fail(engine, "Sortie video ou audio CorvusAPI invalide");
    engine->audio_count = output->audio_count;
    memcpy(engine->audio, output->audio, engine->audio_count * sizeof(int16_t));
    success(engine);
    return true;
}
bool cr_engine_save_battery(cr_engine *engine) {
    if (!engine->handle || !engine->loaded) { success(engine); return true; }
    bool slots = engine->subsystems.battery_size != NULL;
    if (engine->players > 1 && !slots)
        return fail(engine, "Sauvegardes des joueurs indisponibles");
    if (!slots && !engine->api.battery_size) { success(engine); return true; }
    uint8_t *bytes[2] = {NULL, NULL};
    size_t sizes[2] = {0, 0};
    uint64_t generations[2] = {0, 0};
    unsigned count = engine->players > 1 ? 2 : 1;
    bool ok = true;
    for (unsigned i = 0; ok && i < count; ++i) {
        int dirty = slots ? engine->subsystems.battery_dirty(engine->handle, i)
                          : engine->api.battery_dirty(engine->handle);
        if (!dirty) continue;
        size_t size = slots ? engine->subsystems.battery_size(engine->handle, i)
                            : engine->api.battery_size(engine->handle);
        if (!size) continue;
        if (size > MAX_BATTERY) { ok = fail(engine, "Sauvegarde batterie trop volumineuse"); break; }
        bytes[i] = malloc(size);
        if (!bytes[i]) { ok = fail(engine, "Mémoire insuffisante"); break; }
        sizes[i] = size;
        int copied = slots ? engine->subsystems.battery_copy(engine->handle, i, bytes[i], size,
                                                            &generations[i])
                           : engine->api.battery_copy(engine->handle, bytes[i], size, &generations[i]);
        if (!copied) ok = sdk_error(engine);
    }
    for (unsigned i = 0; ok && i < count; ++i) {
        char path[4096];
        if (bytes[i]) ok = battery_path(engine, i, path) && write_file(engine, path, bytes[i], sizes[i]);
    }
    /* No generation is acknowledged until every dirty slot is durable. */
    for (unsigned i = 0; ok && i < count; ++i) {
        if (!bytes[i]) continue;
        int acknowledged = slots ? engine->subsystems.battery_acknowledge(engine->handle, i, generations[i])
                                 : engine->api.battery_acknowledge(engine->handle, generations[i]);
        if (!acknowledged) ok = sdk_error(engine);
    }
    for (unsigned i = 0; i < 2; ++i) free(bytes[i]);
    if (ok) success(engine);
    return ok;
}
bool cr_engine_reset(cr_engine *engine) {
    if (!ready(engine))
        return false;
    if (!cr_engine_can_reset(engine))
        return fail(engine, "Ce cœur ne prend pas en charge le redémarrage de cette session");
    if (!cr_engine_save_battery(engine))
        return false;
    if (!engine->api.reset || !engine->api.reset(engine->handle))
        return sdk_error(engine);
    engine->audio_count = 0;
    success(engine);
    return true;
}
static bool state_path(cr_engine *engine, char path[4096]) {
    if (engine->players <= 1)
        return data_path(engine, path, "states", "-" CR_CORE_VERSION ".state");
    if (engine->players != 2 || !cr_game_id_valid(engine->second_id) || !token(engine->subsystem_id))
        return fail(engine, "Identité de la session invalide");
    uint64_t hash = UINT64_C(14695981039346656037);
    for (const unsigned char *p = (const unsigned char *)engine->subsystem_id; *p; ++p)
        hash = (hash ^ *p) * UINT64_C(1099511628211);
    char suffix[160];
    int n = snprintf(suffix, sizeof(suffix), "-link-%016llx-%s-" CR_CORE_VERSION ".state",
                     (unsigned long long)hash, engine->second_id);
    if (n < 0 || (size_t)n >= sizeof(suffix)) return fail(engine, "Chemin de session trop long");
    return data_path(engine, path, "states", suffix);
}
bool cr_engine_save_state(cr_engine *engine) {
    if (!ready(engine))
        return false;
    if (!cr_engine_can_state(engine))
        return fail(engine, "Ce cœur ne prend pas en charge les états de sauvegarde");
    size_t size = engine->api.serialize_size(engine->handle);
    if (!size || size > MAX_STATE)
        return fail(engine, "État de sauvegarde invalide");
    uint8_t *bytes = malloc(size);
    if (!bytes)
        return fail(engine, "Mémoire insuffisante");
    bool copied = engine->api.serialize(engine->handle, bytes, size) != 0;
    if (!copied)
        sdk_error(engine);
    char path[4096];
    bool written = copied && state_path(engine, path) &&
                   write_file(engine, path, bytes, size);
    free(bytes);
    if (!written)
        return false;
    success(engine);
    return true;
}
bool cr_engine_load_state(cr_engine *engine) {
    if (!ready(engine))
        return false;
    if (!cr_engine_can_state(engine))
        return fail(engine, "Ce cœur ne prend pas en charge les états de sauvegarde");
    char path[4096];
    uint8_t *bytes = NULL;
    size_t size = 0;
    if (!state_path(engine, path) ||
        !read_file(engine, path, MAX_STATE, false, &bytes, &size))
        return false;
    bool loaded = engine->api.unserialize(engine->handle, bytes, size) != 0;
    free(bytes);
    if (!loaded)
        return sdk_error(engine);
    engine->audio_count = 0;
    success(engine);
    return true;
}
bool cr_engine_unload_game(cr_engine *engine) {
    if (!engine->loaded) {
        success(engine);
        return true;
    }
    if (!cr_engine_save_battery(engine))
        return false;
    engine->api.unload_game(engine->handle);
    free(engine->pixels);
    engine->pixels = NULL;
    /* The core still owns a host callback pointing at this heap context. */
    memset(engine->output, 0, sizeof(*engine->output));
    engine->loaded = false;
    engine->width = engine->height = engine->rate = 0;
    engine->fps = 0;
    engine->audio_count = 0;
    memset(engine->audio, 0, sizeof(engine->audio));
    engine->id[0] = engine->second_id[0] = engine->subsystem_id[0] = '\0';
    engine->players = 0;
    success(engine);
    return true;
}
bool cr_engine_close(cr_engine *engine) {
    if (!cr_engine_save_battery(engine))
        return false;
    release_handle(engine);
    success(engine);
    return true;
}
void cr_engine_destroy(cr_engine *engine) {
    if (!cr_engine_save_battery(engine))
        fprintf(stderr, "CorvusRetro : %s\n", engine->error);
    release_handle(engine);
}
