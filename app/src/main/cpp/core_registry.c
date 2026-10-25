// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
#include "core_registry.h"
#include <corvus/api.h>
#include "cr_bundled_cores.h"
#include <ctype.h>
#include <dlfcn.h>
#include <stdio.h>
#include <string.h>

static cr_core_descriptor registry[CR_MAX_CORES];
static size_t registry_size;
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

static void register_module(const char *name) {
    if (!name || registry_size == CR_MAX_CORES) return;
    char path[256];
#if defined(__ANDROID__)
    int n = snprintf(path, sizeof(path), "%s", name);
#else
    int n = snprintf(path, sizeof(path), "./%s", name);
#endif
    if (n <= 0 || (size_t)n >= sizeof(path)) return;
    void *module = dlopen(path, RTLD_NOW | RTLD_LOCAL);
    if (!module) return;

    void *symbol = dlsym(module, CORVUS_CORE_ENTRYPOINT);
    corvus_get_api_fn entry = NULL;
    if (symbol) memcpy(&entry, &symbol, sizeof(entry));

    corvus_core_api api = {.struct_size = sizeof(api)};
    corvus_core_info info = {.struct_size = sizeof(info)};
    if (entry && entry(CORVUS_API_VERSION, &api) &&
        api.api_version == CORVUS_API_VERSION && api.struct_size >= sizeof(api) &&
        api.get_info && api.create && api.destroy && api.load_game && api.unload_game &&
        api.get_av_info && api.set_button && api.run_frame && api.get_info(&info) &&
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
            bool unique = true;
            for (size_t i = 0; i < registry_size; ++i)
                if (strcmp(registry[i].id, descriptor.id) == 0)
                    unique = false;
            if (unique) {
                descriptor.max_players = info.max_players;
                descriptor.state_support = api.serialize_size && api.serialize && api.unserialize;
                descriptor.battery_support = api.battery_size && api.battery_dirty &&
                                             api.battery_copy && api.battery_acknowledge;
                registry[registry_size++] = descriptor;
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
        register_module(cr_bundled_core_modules[i]);
}

size_t cr_registry_count(void) {
    discover();
    return registry_size;
}

const cr_core_descriptor *cr_registry_get(int index) {
    discover();
    return index >= 0 && (size_t)index < registry_size ? &registry[index] : NULL;
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
    for (int i = 0; i < count; ++i)
        if (cr_registry_accepts(i, extension)) return i;
    return cr_registry_get(selected) ? selected : 0;
}
