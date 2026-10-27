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
static void release_handle(cr_engine *engine) {
    if (engine->handle && engine->api.destroy)
        engine->api.destroy(engine->handle);
    if (engine->module)
        dlclose(engine->module);
    free(engine->output);
    free(engine->pixels);
    engine->handle = NULL;
    engine->module = NULL;
    engine->output = NULL;
    engine->pixels = NULL;
    memset(&engine->api, 0, sizeof(engine->api));
    memset(&engine->achievements, 0, sizeof(engine->achievements));
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
    engine->output = calloc(1, sizeof(*engine->output));
    if (!engine->output) return fail(engine, "Mémoire insuffisante");
    corvus_host host = {.struct_size = sizeof(host), .userdata = engine->output,
                        .video_frame = on_video, .audio_frames = on_audio};
    engine->handle = engine->api.create(&host);
    if (!engine->handle) return sdk_error(engine);
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
bool cr_engine_open(cr_engine *engine, int system, const char *id, const char *path) {
    if (!valid_system(engine, system))
        return false;
    if (!cr_game_id_valid(id))
        return fail(engine, "Identifiant de contenu invalide");
    {
        const cr_core_descriptor *core = cr_registry_get(system);
        char expected[96];
        char hash[65];
        memcpy(hash, id, 64);
        hash[64] = '\0';
        if (!core || !cr_registry_content_id(expected, sizeof(expected), hash, core->id) ||
            strcmp(id, expected) != 0)
            return fail(engine, "Le contenu appartient à un autre cœur");
    }
    cr_engine candidate;
    cr_engine_init(&candidate, engine->root);
    memcpy(candidate.id, id, strlen(id) + 1);
    if (!create_handle(&candidate, system)) {
        fail(engine, candidate.error);
        release_handle(&candidate);
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
        corvus_av_info av = {.struct_size = sizeof(av)};
        if (!candidate.api.get_av_info(candidate.handle, &av))
            ok = sdk_error(&candidate);
        else {
            candidate.width = (int)av.width;
            candidate.height = (int)av.height;
            candidate.rate = (int)av.audio_sample_rate;
            candidate.fps = av.fps;
        }
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
        else {
            candidate.output->pixels = candidate.pixels;
            candidate.output->width = (uint32_t)candidate.width;
            candidate.output->height = (uint32_t)candidate.height;
            candidate.loaded = true;
        }
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
size_t cr_engine_read_achievement_memory(void *context, uint32_t address, uint8_t *buffer, size_t capacity) {
    cr_engine *engine = context;
    if (!engine || !engine->loaded || !engine->handle || !engine->achievements.read_memory ||
        !buffer || !capacity) return 0;
    size_t copied = engine->achievements.read_memory(engine->handle, address, buffer, capacity);
    return copied <= capacity ? copied : 0;
}
bool cr_engine_step(cr_engine *engine, unsigned mask) {
    if (!ready(engine)) return false;
    if (mask & ~1023u) return fail(engine, "Boutons invalides");
    static const uint32_t input_map[10] = {
        CORVUS_BUTTON_UP, CORVUS_BUTTON_DOWN, CORVUS_BUTTON_LEFT,
        CORVUS_BUTTON_RIGHT, CORVUS_BUTTON_A, CORVUS_BUTTON_B,
        CORVUS_BUTTON_START, CORVUS_BUTTON_SELECT, CORVUS_BUTTON_L, CORVUS_BUTTON_R
    };
    struct cr_engine_output *output = engine->output;
    output->got_video = false;
    output->failed = false;
    output->audio_count = 0;
    for (int button = 0; button < 10; ++button)
        engine->api.set_button(engine->handle, 0, input_map[button],
                               (mask & (1u << button)) != 0);
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
    if (!engine->handle || !engine->loaded || !engine->api.battery_dirty ||
        !engine->api.battery_dirty(engine->handle)) {
        success(engine);
        return true;
    }
    size_t size = engine->api.battery_size(engine->handle);
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
    bool copied = engine->api.battery_copy(engine->handle, bytes, size, &generation) != 0;
    if (!copied)
        sdk_error(engine);
    char path[4096];
    bool written =
        copied && data_path(engine, path, "saves", ".sav") && write_file(engine, path, bytes, size);
    free(bytes);
    if (!written)
        return false;
    if (!engine->api.battery_acknowledge(engine->handle, generation))
        return sdk_error(engine);
    success(engine);
    return true;
}
bool cr_engine_reset(cr_engine *engine) {
    if (!ready(engine) || !cr_engine_save_battery(engine))
        return false;
    if (!engine->api.reset || !engine->api.reset(engine->handle))
        return sdk_error(engine);
    engine->audio_count = 0;
    success(engine);
    return true;
}
bool cr_engine_save_state(cr_engine *engine) {
    if (!ready(engine))
        return false;
    if (!engine->api.serialize_size || !engine->api.serialize)
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
    if (!engine->api.unserialize)
        return fail(engine, "Ce cœur ne prend pas en charge les états de sauvegarde");
    char path[4096];
    uint8_t *bytes = NULL;
    size_t size = 0;
    if (!data_path(engine, path, "states", "-" CR_CORE_VERSION ".state") ||
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
    engine->id[0] = '\0';
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
