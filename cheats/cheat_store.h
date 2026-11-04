// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
#ifndef CR_CHEAT_STORE_H
#define CR_CHEAT_STORE_H

#include <stdbool.h>
#include <stddef.h>

#define CR_CHEAT_MAX_ENTRIES 128u
#define CR_CHEAT_NAME_SIZE 128u
#define CR_CHEAT_CODE_SIZE 4097u
#define CR_CHEAT_FILE_MAX (2u * 1024u * 1024u)

typedef struct cr_cheat_entry {
    char name[CR_CHEAT_NAME_SIZE];
    char code[CR_CHEAT_CODE_SIZE];
    bool enabled;
} cr_cheat_entry;

typedef struct cr_cheat_profile {
    cr_cheat_entry *entries;
    size_t count;
    bool apply_after_load;
    bool apply_after_toggle;
} cr_cheat_profile;

void cr_cheat_profile_init(cr_cheat_profile *profile);
void cr_cheat_profile_free(cr_cheat_profile *profile);
bool cr_cheat_profile_clone(cr_cheat_profile *destination,
      const cr_cheat_profile *source, char *error, size_t error_size);
bool cr_cheat_profile_append(cr_cheat_profile *profile, const char *name,
      const char *code, bool enabled, char *error, size_t error_size);
bool cr_cheat_profile_remove(cr_cheat_profile *profile, size_t index);

/* Initialize destinations first. Errors leave them unchanged. Input excludes NUL.
 * Unknown metadata is ignored; indexed engine entries require handler zero.
 * Names reject ASCII controls. Encode normalizes code CR/LF/TAB to spaces for
 * .cht interoperability, and uses literal backslashes except before quotes or
 * the closing quote. It allocates a NUL-terminated buffer (free it); size
 * excludes that NUL. */
bool cr_cheat_profile_parse(cr_cheat_profile *destination, const char *data,
      size_t size, char *error, size_t error_size);
bool cr_cheat_profile_encode(const cr_cheat_profile *profile, char **data,
      size_t *size, char *error, size_t error_size);

/* Owner-thread actions only: these functions perform synchronous private-file IO.
 * ENOENT loads an empty profile successfully and sets found=false.
 * Save uses a mode-0600 sibling temporary, fsync, rename and directory fsync.
 * If the final directory fsync fails, replacement occurred but durability is
 * uncertain, and save reports failure with that distinction in its error. */
bool cr_cheat_profile_load(cr_cheat_profile *destination, const char *path,
      bool *found, char *error, size_t error_size);
bool cr_cheat_profile_save(const cr_cheat_profile *profile, const char *path,
      char *error, size_t error_size);

#endif
