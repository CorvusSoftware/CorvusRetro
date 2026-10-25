// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
#define _POSIX_C_SOURCE 200809L
#include "engine.h"
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#define MAX_ROM (32u * 1024u * 1024u)
#define MAX_BATTERY (1024u * 1024u)
#define MAX_STATE (16u * 1024u * 1024u)

static bool fail(cr_engine *engine, const char *message) {
    snprintf(engine->error, sizeof(engine->error), "%s", message ? message : "Erreur du moteur");
    return false;
}
static bool sdk_error(cr_engine *engine) {
    return fail(engine, corvuscore_last_error());
}
static void success(cr_engine *engine) {
    engine->error[0] = '\0';
}
static bool valid_system(cr_engine *engine, int system) {
    return (system >= CORVUSCORE_GB && system <= CORVUSCORE_GBA) || fail(engine, "Cœur inconnu");
}
static bool ready(cr_engine *engine) {
    return (engine->handle && engine->loaded) || fail(engine, "Aucun contenu chargé");
}
bool cr_game_id_valid(const char *id) {
    if (!id || strlen(id) != 66 || id[64] != '-' || id[65] < '0' || id[65] > '2')
        return false;
    for (size_t i = 0; i < 64; ++i)
        if (!((id[i] >= '0' && id[i] <= '9') || (id[i] >= 'a' && id[i] <= 'f')))
            return false;
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
static void release_handle(cr_engine *engine) {
    if (engine->handle)
        corvuscore_destroy(engine->handle);
    free(engine->pixels);
    engine->handle = NULL;
    engine->pixels = NULL;
    engine->loaded = false;
    engine->system = -1;
    engine->width = engine->height = engine->rate = 0;
    engine->fps = 0;
    engine->audio_count = 0;
    engine->id[0] = '\0';
}
static bool create_handle(cr_engine *engine, int system) {
    if (!valid_system(engine, system))
        return false;
    if (corvuscore_api_version() != CORVUSCORE_API_VERSION)
        return fail(engine, "Version de l'API CorvusCore incompatible");
    engine->handle = corvuscore_create(system);
    if (!engine->handle)
        return sdk_error(engine);
    engine->system = system;
    return true;
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
    int loaded = corvuscore_load(engine->handle, data, size, battery, battery_size);
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
bool cr_engine_select(cr_engine *engine, int system) {
    if (!valid_system(engine, system))
        return false;
    if (engine->handle && !engine->loaded && engine->system == system) {
        success(engine);
        return true;
    }
    cr_engine candidate;
    cr_engine_init(&candidate, engine->root);
    if (!create_handle(&candidate, system)) {
        fail(engine, candidate.error);
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
bool cr_engine_open(cr_engine *engine, int system, const char *id, const char *path) {
    if (!valid_system(engine, system))
        return false;
    if (!cr_game_id_valid(id) || id[65] != '0' + system)
        return fail(engine, "Identifiant de contenu invalide");
    cr_engine candidate;
    cr_engine_init(&candidate, engine->root);
    memcpy(candidate.id, id, strlen(id) + 1);
    if (!create_handle(&candidate, system)) {
        fail(engine, candidate.error);
        return false;
    }
    /* Reopening the current game must load its latest in-memory battery. */
    if (!cr_engine_save_battery(engine)) {
        release_handle(&candidate);
        return false;
    }
    char battery_path[4096];
    uint8_t *battery = NULL;
    size_t battery_size = 0;
    bool ok = data_path(&candidate, battery_path, "saves", ".sav") &&
              read_file(&candidate, battery_path, MAX_BATTERY, true, &battery, &battery_size) &&
              mapped_load(&candidate, path, battery, battery_size);
    free(battery);
    if (ok) {
        candidate.width = corvuscore_video_width(candidate.handle);
        candidate.height = corvuscore_video_height(candidate.handle);
        candidate.rate = corvuscore_audio_rate(candidate.handle);
        candidate.fps = corvuscore_video_fps(candidate.handle);
        if (candidate.width <= 0 || candidate.height <= 0 || candidate.width > 1024 ||
            candidate.height > 1024 || candidate.rate <= 0 || !isfinite(candidate.fps) ||
            candidate.fps <= 0)
            ok = fail(&candidate, "Format vidéo ou audio invalide");
    }
    if (ok) {
        candidate.pixels =
            calloc((size_t)candidate.width * (size_t)candidate.height, sizeof(*candidate.pixels));
        if (!candidate.pixels)
            ok = fail(&candidate, "Mémoire insuffisante");
        else
            candidate.loaded = true;
    }
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
bool cr_engine_step(cr_engine *engine, unsigned mask) {
    if (!ready(engine))
        return false;
    if (mask & ~1023u)
        return fail(engine, "Boutons invalides");
    for (int button = 0; button < 10; ++button)
        if (!corvuscore_set_button(engine->handle, button, (mask & (1u << button)) != 0))
            return sdk_error(engine);
    if (!corvuscore_run_frame(engine->handle, engine->pixels,
                              (size_t)engine->width * (size_t)engine->height))
        return sdk_error(engine);
    engine->audio_count = corvuscore_read_audio(engine->handle, engine->audio,
                                                sizeof(engine->audio) / sizeof(engine->audio[0]));
    if (engine->audio_count > sizeof(engine->audio) / sizeof(engine->audio[0]) ||
        engine->audio_count % 2 != 0)
        return fail(engine, "Format audio invalide");
    success(engine);
    return true;
}
bool cr_engine_save_battery(cr_engine *engine) {
    if (!engine->handle || !engine->loaded || !corvuscore_battery_dirty(engine->handle)) {
        success(engine);
        return true;
    }
    size_t size = corvuscore_battery_size(engine->handle);
    if (!size) {
        success(engine);
        return true;
    }
    if (size > MAX_BATTERY)
        return fail(engine, "Sauvegarde batterie trop volumineuse");
    uint8_t *bytes = malloc(size);
    if (!bytes)
        return fail(engine, "Mémoire insuffisante");
    uint64_t generation = 0;
    bool copied = corvuscore_copy_battery_generation(engine->handle, bytes, size, &generation) != 0;
    if (!copied)
        sdk_error(engine);
    char path[4096];
    bool written =
        copied && data_path(engine, path, "saves", ".sav") && write_file(engine, path, bytes, size);
    free(bytes);
    if (!written)
        return false;
    if (!corvuscore_acknowledge_battery(engine->handle, generation))
        return sdk_error(engine);
    success(engine);
    return true;
}
bool cr_engine_reset(cr_engine *engine) {
    if (!ready(engine) || !cr_engine_save_battery(engine))
        return false;
    if (!corvuscore_reset(engine->handle))
        return sdk_error(engine);
    engine->audio_count = 0;
    success(engine);
    return true;
}
bool cr_engine_save_state(cr_engine *engine) {
    if (!ready(engine))
        return false;
    size_t size = corvuscore_state_size(engine->handle);
    if (!size || size > MAX_STATE)
        return fail(engine, "État de sauvegarde invalide");
    uint8_t *bytes = malloc(size);
    if (!bytes)
        return fail(engine, "Mémoire insuffisante");
    bool copied = corvuscore_save_state(engine->handle, bytes, size) != 0;
    if (!copied)
        sdk_error(engine);
    char path[4096];
    bool written = copied && data_path(engine, path, "states", "-" CR_CORE_VERSION ".state") &&
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
    char path[4096];
    uint8_t *bytes = NULL;
    size_t size = 0;
    if (!data_path(engine, path, "states", "-" CR_CORE_VERSION ".state") ||
        !read_file(engine, path, MAX_STATE, false, &bytes, &size))
        return false;
    bool loaded = corvuscore_load_state(engine->handle, bytes, size) != 0;
    free(bytes);
    if (!loaded)
        return sdk_error(engine);
    engine->audio_count = 0;
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
