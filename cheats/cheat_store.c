// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
#define _POSIX_C_SOURCE 200809L
#include "cheat_store.h"

#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static bool fail(char *error, size_t size, const char *format, ...) {
    if (error && size) {
        va_list args;
        va_start(args, format);
        vsnprintf(error, size, format, args);
        va_end(args);
    }
    return false;
}

static void clear_error(char *error, size_t size) {
    if (error && size) error[0] = '\0';
}

static bool whitespace(unsigned char c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

static bool utf8(const char *data, size_t size) {
    size_t i = 0;
    while (i < size) {
        unsigned char c = (unsigned char)data[i++];
        uint32_t value;
        size_t remaining;
        if (!c) return false;
        if (c < 0x80) continue;
        if (c >= 0xc2 && c <= 0xdf) { value = c & 31u; remaining = 1; }
        else if (c >= 0xe0 && c <= 0xef) { value = c & 15u; remaining = 2; }
        else if (c >= 0xf0 && c <= 0xf4) { value = c & 7u; remaining = 3; }
        else return false;
        if (remaining > size - i) return false;
        size_t length = remaining;
        while (remaining--) {
            c = (unsigned char)data[i++];
            if ((c & 0xc0u) != 0x80u) return false;
            value = (value << 6) | (c & 63u);
        }
        if ((length == 1 && value < 0x80) ||
            (length == 2 && value < 0x800) ||
            (length == 3 && value < 0x10000) ||
            (value >= 0xd800 && value <= 0xdfff) || value > 0x10ffff)
            return false;
    }
    return true;
}

static bool has_code(const char *code) {
    while (*code) {
        if (!whitespace((unsigned char)*code++)) return true;
    }
    return false;
}

static bool valid_controls(const char *text, bool code) {
    for (; *text; ++text) {
        unsigned char c = (unsigned char)*text;
        if ((c < 32 || c == 127) && !(code && whitespace(c))) return false;
    }
    return true;
}

static bool valid_profile(const cr_cheat_profile *profile,
      char *error, size_t error_size) {
    if (!profile || profile->count > CR_CHEAT_MAX_ENTRIES ||
        (profile->count && !profile->entries))
        return fail(error, error_size, "Profil de triche invalide");
    for (size_t i = 0; i < profile->count; ++i) {
        const cr_cheat_entry *entry = &profile->entries[i];
        size_t name_size = strnlen(entry->name, sizeof(entry->name));
        size_t code_size = strnlen(entry->code, sizeof(entry->code));
        if (name_size == sizeof(entry->name) || code_size == sizeof(entry->code) ||
            !utf8(entry->name, name_size) || !utf8(entry->code, code_size) ||
            !valid_controls(entry->name, false) || !valid_controls(entry->code, true) ||
            (entry->enabled && !has_code(entry->code)))
            return fail(error, error_size, "Entrée de triche %zu invalide", i + 1);
    }
    return true;
}

void cr_cheat_profile_init(cr_cheat_profile *profile) {
    if (profile) memset(profile, 0, sizeof(*profile));
}

void cr_cheat_profile_free(cr_cheat_profile *profile) {
    if (profile) {
        free(profile->entries);
        cr_cheat_profile_init(profile);
    }
}

static void replace(cr_cheat_profile *destination, cr_cheat_profile *source) {
    cr_cheat_profile_free(destination);
    *destination = *source;
    cr_cheat_profile_init(source);
}

bool cr_cheat_profile_clone(cr_cheat_profile *destination,
      const cr_cheat_profile *source, char *error, size_t error_size) {
    clear_error(error, error_size);
    if (!destination || !valid_profile(source, error, error_size)) return false;
    if (destination == source) return true;
    cr_cheat_profile copy = *source;
    copy.entries = NULL;
    if (copy.count) {
        copy.entries = malloc(copy.count * sizeof(*copy.entries));
        if (!copy.entries) return fail(error, error_size, "Mémoire insuffisante");
        memcpy(copy.entries, source->entries, copy.count * sizeof(*copy.entries));
    }
    replace(destination, &copy);
    return true;
}

bool cr_cheat_profile_append(cr_cheat_profile *profile, const char *name,
      const char *code, bool enabled, char *error, size_t error_size) {
    clear_error(error, error_size);
    if (!valid_profile(profile, error, error_size)) return false;
    if (!name || !code || profile->count == CR_CHEAT_MAX_ENTRIES)
        return fail(error, error_size, "Entrée invalide ou liste de triche pleine");
    cr_cheat_entry entry = {0};
    size_t name_size = strnlen(name, sizeof(entry.name));
    size_t code_size = strnlen(code, sizeof(entry.code));
    if (name_size >= sizeof(entry.name) || code_size >= sizeof(entry.code) ||
        !utf8(name, name_size) || !utf8(code, code_size) ||
        !valid_controls(name, false) || !valid_controls(code, true) ||
        (enabled && !has_code(code)))
        return fail(error, error_size, "Nom ou code de triche invalide");
    if (name_size) memcpy(entry.name, name, name_size + 1);
    else snprintf(entry.name, sizeof(entry.name), "Code %zu", profile->count + 1);
    memcpy(entry.code, code, code_size + 1);
    entry.enabled = enabled;
    cr_cheat_entry *entries = realloc(profile->entries,
          (profile->count + 1) * sizeof(*entries));
    if (!entries) return fail(error, error_size, "Mémoire insuffisante");
    profile->entries = entries;
    profile->entries[profile->count++] = entry;
    return true;
}

bool cr_cheat_profile_remove(cr_cheat_profile *profile, size_t index) {
    if (!profile || index >= profile->count || !profile->entries) return false;
    memmove(profile->entries + index, profile->entries + index + 1,
          (profile->count - index - 1) * sizeof(*profile->entries));
    --profile->count;
    return true;
}

static bool number(const char *value, size_t *result) {
    size_t n = 0;
    if (!*value) return false;
    for (; *value; ++value) {
        if (*value < '0' || *value > '9' || n > (SIZE_MAX - 9) / 10)
            return false;
        n = n * 10 + (size_t)(*value - '0');
    }
    *result = n;
    return true;
}

static bool boolean(const char *value, bool *result) {
    if (!strcmp(value, "true") || !strcmp(value, "1")) *result = true;
    else if (!strcmp(value, "false") || !strcmp(value, "0")) *result = false;
    else return false;
    return true;
}

static bool value_decode(const char *start, const char *end,
      char *value, size_t capacity) {
    size_t count = 0;
    while (start < end && whitespace((unsigned char)*start)) ++start;
    if (start < end && *start == '"') {
        bool closed = false;
        ++start;
        while (start < end) {
            char c = *start++;
            if (c == '"') { closed = true; break; }
            if (c == '\\') {
                size_t slashes = 1;
                while (start < end && *start == '\\') { ++slashes; ++start; }
                bool before_quote = start < end && *start == '"';
                size_t literal = before_quote ? slashes / 2 : slashes;
                if (literal >= capacity - count) return false;
                memset(value + count, '\\', literal);
                count += literal;
                if (before_quote && (slashes & 1u)) {
                    if (count + 1 >= capacity) return false;
                    value[count++] = *start++;
                }
                continue;
            }
            if (count + 1 >= capacity) return false;
            value[count++] = c;
        }
        if (!closed) return false;
        while (start < end && whitespace((unsigned char)*start)) ++start;
        if (start < end && *start != '#' && *start != ';') return false;
    } else {
        const char *finish = start;
        while (finish < end && *finish != '#' && *finish != ';') ++finish;
        while (finish > start && whitespace((unsigned char)finish[-1])) --finish;
        count = (size_t)(finish - start);
        if (count >= capacity) return false;
        memcpy(value, start, count);
    }
    value[count] = '\0';
    return true;
}

bool cr_cheat_profile_parse(cr_cheat_profile *destination, const char *data,
      size_t size, char *error, size_t error_size) {
    clear_error(error, error_size);
    if (!destination || (!data && size) || size > CR_CHEAT_FILE_MAX ||
        !utf8(data, size)) return fail(error, error_size, "Taille ou encodage UTF-8 du fichier de triche invalide");
    cr_cheat_profile parsed = {0};
    unsigned char seen[CR_CHEAT_MAX_ENTRIES] = {0};
    unsigned globals = 0;
    size_t line = 0;
    parsed.entries = calloc(CR_CHEAT_MAX_ENTRIES, sizeof(*parsed.entries));
    if (!parsed.entries) return fail(error, error_size, "Mémoire insuffisante");
    const char *cursor = data;
    if (size >= 3 && !memcmp(data, "\xef\xbb\xbf", 3)) cursor += 3;
    const char *limit = data ? data + size : data;
    while (cursor != limit) {
        const char *end = memchr(cursor, '\n', (size_t)(limit - cursor));
        if (!end) end = limit;
        const char *next = end == limit ? limit : end + 1;
        ++line;
        while (cursor < end && whitespace((unsigned char)*cursor)) ++cursor;
        if (cursor == end || *cursor == '#' || *cursor == ';') { cursor = next; continue; }
        const char *key_end = cursor;
        while (key_end < end && !whitespace((unsigned char)*key_end) && *key_end != '=') ++key_end;
        size_t key_size = (size_t)(key_end - cursor);
        char key[128], value[CR_CHEAT_CODE_SIZE];
        if (!key_size || key_size >= sizeof(key)) goto invalid;
        memcpy(key, cursor, key_size); key[key_size] = '\0';
        cursor = key_end;
        while (cursor < end && whitespace((unsigned char)*cursor)) ++cursor;
        if (cursor == end || *cursor++ != '=' || !value_decode(cursor, end, value, sizeof(value))) goto invalid;
        if (!strcmp(key, "cheats")) {
            if ((globals & 1u) || !number(value, &parsed.count) || parsed.count > CR_CHEAT_MAX_ENTRIES) goto invalid;
            globals |= 1u;
        } else if (!strcmp(key, "corvus_apply_after_load") || !strcmp(key, "corvus_apply_after_toggle")) {
            unsigned bit = !strcmp(key, "corvus_apply_after_load") ? 2u : 4u;
            bool *flag = bit == 2u ? &parsed.apply_after_load : &parsed.apply_after_toggle;
            if ((globals & bit) || !boolean(value, flag)) goto invalid;
            globals |= bit;
        } else if (!strncmp(key, "cheat", 5)) {
            char *index_end = key + 5;
            while (*index_end >= '0' && *index_end <= '9') ++index_end;
            if (index_end == key + 5 || *index_end != '_') goto invalid;
            *index_end = '\0';
            size_t index;
            if (!number(key + 5, &index) || index >= CR_CHEAT_MAX_ENTRIES) goto invalid;
            const char *field = index_end + 1;
            unsigned bit = !strcmp(field, "desc") ? 1u : !strcmp(field, "code") ? 2u :
                  !strcmp(field, "enable") ? 4u : !strcmp(field, "handler") ? 8u : 0u;
            if (seen[index] & bit) goto invalid;
            seen[index] |= (unsigned char)(bit | 16u);
            cr_cheat_entry *entry = &parsed.entries[index];
            if (bit == 1u) {
                if (strlen(value) >= sizeof(entry->name)) goto invalid;
                strcpy(entry->name, value);
            } else if (bit == 2u) strcpy(entry->code, value);
            else if (bit == 4u) { if (!boolean(value, &entry->enabled)) goto invalid; }
            else if (bit == 8u) {
                size_t handler;
                if (!number(value, &handler)) goto invalid;
                if (handler) {
                    fail(error, error_size, "Ligne %zu : moteur de triche mémoire non pris en charge", line);
                    goto cleanup;
                }
            }
        }
        cursor = next;
    }
    if (!(globals & 1u)) goto invalid;
    for (size_t i = 0; i < CR_CHEAT_MAX_ENTRIES; ++i) {
        if (i >= parsed.count && seen[i]) goto invalid;
        if (i < parsed.count && !parsed.entries[i].name[0])
            snprintf(parsed.entries[i].name, sizeof(parsed.entries[i].name), "Code %zu", i + 1);
    }
    if (!valid_profile(&parsed, error, error_size)) goto cleanup;
    replace(destination, &parsed);
    return true;
invalid:
    fail(error, error_size, "Champ de triche invalide ou répété à la ligne %zu", line);
cleanup:
    cr_cheat_profile_free(&parsed);
    return false;
}

static void quoted(char **cursor, const char *value, bool code) {
    *(*cursor)++ = '"';
    while (*value) {
        char c = *value++;
        if (c == '\\') {
            size_t slashes = 1;
            while (*value == '\\') { ++slashes; ++value; }
            if (!*value || *value == '"') slashes *= 2;
            memset(*cursor, '\\', slashes);
            *cursor += slashes;
            continue;
        }
        if (c == '"') {
            *(*cursor)++ = '\\';
        }
        if (code && (c == '\n' || c == '\r' || c == '\t')) c = ' ';
        *(*cursor)++ = c;
    }
    *(*cursor)++ = '"';
    *(*cursor)++ = '\n';
}

bool cr_cheat_profile_encode(const cr_cheat_profile *profile, char **data,
      size_t *size, char *error, size_t error_size) {
    clear_error(error, error_size);
    if (!data || !size || !valid_profile(profile, error, error_size)) return false;
    size_t capacity = 256 + profile->count *
          (2 * (CR_CHEAT_NAME_SIZE + CR_CHEAT_CODE_SIZE) + 256);
    char *buffer = malloc(capacity);
    if (!buffer) return fail(error, error_size, "Mémoire insuffisante");
    char *cursor = buffer;
    cursor += sprintf(cursor, "cheats = %zu\ncorvus_apply_after_load = %s\ncorvus_apply_after_toggle = %s\n",
          profile->count, profile->apply_after_load ? "true" : "false",
          profile->apply_after_toggle ? "true" : "false");
    for (size_t i = 0; i < profile->count; ++i) {
        const cr_cheat_entry *entry = &profile->entries[i];
        cursor += sprintf(cursor, "cheat%zu_desc = ", i);
        if (entry->name[0]) quoted(&cursor, entry->name, false);
        else {
            char fallback[32];
            snprintf(fallback, sizeof(fallback), "Code %zu", i + 1);
            quoted(&cursor, fallback, false);
        }
        cursor += sprintf(cursor, "cheat%zu_code = ", i);
        quoted(&cursor, entry->code, true);
        cursor += sprintf(cursor, "cheat%zu_enable = %s\ncheat%zu_handler = 0\n",
              i, entry->enabled ? "true" : "false", i);
    }
    *cursor = '\0';
    *data = buffer;
    *size = (size_t)(cursor - buffer);
    return true;
}

bool cr_cheat_profile_load(cr_cheat_profile *destination, const char *path,
      bool *found, char *error, size_t error_size) {
    clear_error(error, error_size);
    if (found) *found = false;
    if (!destination || !path || !*path) return fail(error, error_size, "Chemin de triche invalide");
    int fd = open(path, O_RDONLY | O_CLOEXEC | O_NONBLOCK);
    if (fd < 0) {
        if (errno == ENOENT) { cr_cheat_profile_free(destination); return true; }
        return fail(error, error_size, "Impossible d'ouvrir le fichier de triche : %s", strerror(errno));
    }
    if (found) *found = true;
    struct stat st;
    if (fstat(fd, &st) || !S_ISREG(st.st_mode) || st.st_size < 0 ||
        (uintmax_t)st.st_size > CR_CHEAT_FILE_MAX) {
        close(fd);
        return fail(error, error_size, "Type ou taille du fichier de triche invalide");
    }
    char *buffer = malloc(CR_CHEAT_FILE_MAX + 1u);
    if (!buffer) { close(fd); return fail(error, error_size, "Mémoire insuffisante"); }
    size_t size = 0;
    bool ok = true;
    while (size <= CR_CHEAT_FILE_MAX) {
        ssize_t n = read(fd, buffer + size, CR_CHEAT_FILE_MAX + 1u - size);
        if (n < 0 && errno == EINTR) continue;
        if (n < 0) { ok = fail(error, error_size, "Impossible de lire le fichier de triche : %s", strerror(errno)); break; }
        if (!n) break;
        size += (size_t)n;
        if (size > CR_CHEAT_FILE_MAX) { ok = fail(error, error_size, "Fichier de triche trop grand"); break; }
    }
    if (close(fd) && ok) ok = fail(error, error_size, "Impossible de fermer le fichier de triche : %s", strerror(errno));
    if (ok) ok = cr_cheat_profile_parse(destination, buffer, size, error, error_size);
    free(buffer);
    return ok;
}

bool cr_cheat_profile_save(const cr_cheat_profile *profile, const char *path,
      char *error, size_t error_size) {
    clear_error(error, error_size);
    if (!path || !*path) return fail(error, error_size, "Chemin de triche invalide");
    char *data = NULL;
    size_t size = 0;
    if (!cr_cheat_profile_encode(profile, &data, &size, error, error_size)) return false;
    size_t path_size = strlen(path);
    if (path_size > SIZE_MAX - 12) { free(data); return fail(error, error_size, "Chemin de triche trop long"); }
    char *temporary = malloc(path_size + 12);
    char *directory = strdup(path);
    if (!temporary || !directory) {
        free(data); free(temporary); free(directory);
        return fail(error, error_size, "Mémoire insuffisante");
    }
    snprintf(temporary, path_size + 12, "%s.tmp.XXXXXX", path);
    char *slash = strrchr(directory, '/');
    if (!slash) strcpy(directory, ".");
    else if (slash == directory) slash[1] = '\0';
    else *slash = '\0';
    int directory_fd = open(directory, O_RDONLY | O_CLOEXEC | O_DIRECTORY);
    int fd = -1;
    bool ok = false;
    bool renamed = false;
    bool temporary_created = false;
    if (directory_fd < 0) goto io_error;
    fd = mkstemp(temporary);
    if (fd < 0) goto io_error;
    temporary_created = true;
    if (fchmod(fd, S_IRUSR | S_IWUSR)) goto io_error;
    size_t written = 0;
    while (written < size) {
        ssize_t n = write(fd, data + written, size - written);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) { if (!n) errno = EIO; goto io_error; }
        written += (size_t)n;
    }
    if (fsync(fd)) goto io_error;
    if (close(fd)) { fd = -1; goto io_error; }
    fd = -1;
    if (rename(temporary, path)) goto io_error;
    renamed = true;
    if (fsync(directory_fd)) {
        fail(error, error_size, "Fichier de triche remplacé, synchronisation du dossier échouée : %s", strerror(errno));
        goto cleanup;
    }
    ok = true;
    goto cleanup;
io_error:
    fail(error, error_size, "Impossible d'enregistrer le fichier de triche : %s", strerror(errno));
cleanup:
    if (fd >= 0) close(fd);
    if (directory_fd >= 0) close(directory_fd);
    if (temporary_created && !renamed) unlink(temporary);
    free(data); free(temporary); free(directory);
    return ok;
}
