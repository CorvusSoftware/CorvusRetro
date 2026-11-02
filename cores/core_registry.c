// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
#include "core_registry.h"
#include <corvusapi.h>
#include "cr_bundled_cores.h"
#include <ctype.h>
#include <dlfcn.h>
#include <stdio.h>
#include <string.h>

static cr_core_descriptor registry[CR_MAX_CORES];
static size_t registry_size;
static bool present[CR_MAX_CORES], installed[CR_MAX_CORES];
static bool initialized;

static bool copy_string(char *dest, size_t capacity, const char *source) {
    if (!source) return false;
    size_t length = 0;
    while (length < capacity && source[length]) ++length;
    if (!length || length >= capacity) return false;
    memcpy(dest, source, length + 1);
    return true;
}

static bool valid_core_id(const char *id) {
    for (const unsigned char *p = (const unsigned char *)id; *p; ++p)
        if (!isalnum(*p) && *p != '.' && *p != '_' && *p != '-') return false;
    return id[0] != '\0';
}

static void set_system_id(cr_core_descriptor *core) {
    if (strcmp(core->id, "corvus.gb") == 0)
        snprintf(core->system_id, sizeof(core->system_id), "nintendo.game_boy");
    else if (strcmp(core->id, "corvus.gbc") == 0)
        snprintf(core->system_id, sizeof(core->system_id), "nintendo.game_boy_color");
    else if (strcmp(core->id, "corvus.gba") == 0)
        snprintf(core->system_id, sizeof(core->system_id), "nintendo.game_boy_advance");
    else
        snprintf(core->system_id, sizeof(core->system_id), "%s", core->id);
}

static void register_module(const char *name, bool downloaded) {
    if (!name) return;
    char path[4096];
#if defined(__ANDROID__)
    int n = snprintf(path, sizeof(path), "%s", name);
#else
    int n = snprintf(path, sizeof(path), "%s%s", name[0] == '/' ? "" : "./", name);
#endif
    if (n <= 0 || (size_t)n >= sizeof(path)) return;
    void *module = dlopen(path, RTLD_NOW | RTLD_LOCAL);
    if (!module) return;

    void *symbol = dlsym(module, CORVUS_CORE_ENTRYPOINT);
    corvus_get_api_fn entry = NULL;
    if (symbol) memcpy(&entry, &symbol, sizeof(entry));

    corvus_core_api api = {.struct_size = sizeof(api)};
    corvus_core_info info = {.struct_size = sizeof(info)};
    const bool negotiated = entry && entry(CORVUS_API_VERSION, &api);
    const bool has_state = api.serialize_size && api.serialize && api.unserialize;
    const bool any_state = api.serialize_size || api.serialize || api.unserialize;
    const bool has_battery = api.battery_size && api.battery_dirty &&
                             api.battery_copy && api.battery_acknowledge;
    const bool any_battery = api.battery_size || api.battery_dirty ||
                             api.battery_copy || api.battery_acknowledge;
    if (negotiated && api.api_version == CORVUS_API_VERSION &&
        api.struct_size >= sizeof(api) && has_state == any_state &&
        has_battery == any_battery && api.get_info && api.create &&
        api.destroy && api.load_game && api.unload_game && api.get_av_info &&
        api.set_button && api.run_frame && api.get_info(&info) &&
        info.struct_size >= sizeof(info)) {
        cr_core_descriptor descriptor = {0};
        if (copy_string(descriptor.module, sizeof(descriptor.module), name) &&
            copy_string(descriptor.id, sizeof(descriptor.id), info.id) &&
            valid_core_id(descriptor.id) &&
            copy_string(descriptor.name, sizeof(descriptor.name), info.name) &&
            copy_string(descriptor.version, sizeof(descriptor.version), info.version) &&
            copy_string(descriptor.system_name, sizeof(descriptor.system_name),
                        info.system_name) &&
            copy_string(descriptor.extensions, sizeof(descriptor.extensions),
                        info.extensions)) {
            set_system_id(&descriptor);
            descriptor.max_players = info.max_players;
            descriptor.state_support = has_state;
            descriptor.battery_support = has_battery;
            size_t slot = registry_size;
            for (size_t i = 0; i < registry_size; ++i)
                if (!strcmp(registry[i].id, descriptor.id)) { slot = i; break; }
            if (slot == registry_size)
                for (size_t i = 0; i < registry_size; ++i)
                    if (!present[i]) { slot = i; break; }
            if (slot < CR_MAX_CORES) {
                registry[slot] = descriptor;
                present[slot] = true; installed[slot] = downloaded;
                if (slot == registry_size) ++registry_size;
            }
        }
    }
    dlclose(module);
}

static void discover(void) {
    if (initialized) return;
    initialized = true;
    for (size_t i = 0; i < sizeof(cr_bundled_core_modules) /
                            sizeof(cr_bundled_core_modules[0]); ++i)
        register_module(cr_bundled_core_modules[i], false);
}

size_t cr_registry_count(void) {
    discover();
    size_t count = 0;
    for (size_t i = 0; i < registry_size; ++i) count += present[i];
    return count;
}
int cr_registry_index_at(size_t row) {
    discover();
    for (size_t i = 0; i < registry_size; ++i)
        if (present[i] && !row--) return (int)i;
    return -1;
}
void cr_registry_sync_installed_modules(const char *const *paths, size_t count, int keep) {
    discover();
    if (count > CR_MAX_CORES || (count && !paths)) return;
    for (size_t i = 0; i < registry_size; ++i) {
        if (!present[i] || !installed[i] || (int)i == keep) continue;
        bool found = false;
        for (size_t j = 0; j < count; ++j)
            if (paths[j] && !strcmp(registry[i].module, paths[j])) { found = true; break; }
        if (!found) present[i] = false;
    }
    cr_registry_add_installed_modules(paths, count);
}

void cr_registry_add_installed_modules(const char *const *paths, size_t count) {
    discover();
    if (!paths || count > CR_MAX_CORES) return;
    for (size_t i = 0; i < count; ++i)
        if (paths[i] && paths[i][0] == '/') register_module(paths[i], true);
}

const cr_core_descriptor *cr_registry_get(int index) {
    discover();
    return index >= 0 && (size_t)index < registry_size && present[index] ? &registry[index] : NULL;
}

int cr_registry_find(const char *core_id) {
    if (!core_id || !*core_id) return -1;
    discover();
    for (size_t i = 0; i < registry_size; ++i)
        if (present[i] && strcmp(registry[i].id, core_id) == 0) return (int)i;
    return -1;
}

static int legacy_id(const char *core_id) {
    if (!core_id) return -1;
    const char *const official[] = {"corvus.gb", "corvus.gbc", "corvus.gba"};
    for (int i = 0; i < 3; ++i)
        if (strcmp(core_id, official[i]) == 0) return i;
    return -1;
}

int cr_registry_legacy_system(int index) {
    const cr_core_descriptor *core = cr_registry_get(index);
    return core ? legacy_id(core->id) : -1;
}

bool cr_registry_content_id(char *out, size_t cap, const char *sha256,
                            const char *core_id) {
    if (!out || !cap || !sha256 || !core_id || !*core_id) return false;
    if (strlen(sha256) != 64) return false;
    for (size_t i = 0; i < 64; ++i)
        if (!((sha256[i] >= '0' && sha256[i] <= '9') ||
              (sha256[i] >= 'a' && sha256[i] <= 'f'))) return false;
    int legacy = legacy_id(core_id);
    if (legacy >= 0) {
        int n = snprintf(out, cap, "%s-%d", sha256, legacy);
        return n == 66 && (size_t)n < cap;
    }
    if (cap < 83) return false;
    /* FNV-1a here is a deterministic filename namespace, NOT a security hash.
     * The ROM identity remains the full SHA-256 digest above. */
    uint64_t suffix = UINT64_C(14695981039346656037);
    for (const unsigned char *p = (const unsigned char *)core_id; *p; ++p) {
        suffix ^= *p;
        suffix *= UINT64_C(1099511628211);
    }
    int n = snprintf(out, cap, "%s-c%016llx", sha256,
                     (unsigned long long)suffix);
    return n == 82 && (size_t)n < cap;
}

bool cr_registry_accepts(int index, const char *extension) {
    const cr_core_descriptor *core = cr_registry_get(index);
    if (!core || !extension || !extension[0]) return false;
    if (extension[0] == '.') ++extension;
    size_t length = strlen(extension);
    if (!length || length > 32) return false;
    const char *start = core->extensions;
    while (*start) {
        const char *end = strchr(start, '|');
        if (!end) end = start + strlen(start);
        if ((size_t)(end - start) == length) {
            size_t i = 0;
            while (i < length &&
                   tolower((unsigned char)start[i]) ==
                   tolower((unsigned char)extension[i])) ++i;
            if (i == length) return true;
        }
        if (!*end) break;
        start = end + 1;
    }
    return false;
}

int cr_registry_choose(const char *extension, int selected) {
    int count = (int)cr_registry_count();
    if (count == 0) return -1;
    if (cr_registry_accepts(selected, extension)) return selected;
    for (int row = 0; row < count; ++row) {
        int index = cr_registry_index_at((size_t)row);
        if (cr_registry_accepts(index, extension)) return index;
    }
    return cr_registry_get(selected) ? selected : cr_registry_index_at(0);
}
