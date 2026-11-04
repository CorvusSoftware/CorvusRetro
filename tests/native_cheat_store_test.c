// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
#define _POSIX_C_SOURCE 200809L
#include "cheats/cheat_store.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#ifdef CR_CHEAT_TEST_FAILURES
static int fail_sync;
static int fail_allocation;
int __real_fsync(int fd);
int __wrap_fsync(int fd) {
    if (fail_sync && !--fail_sync) { errno = EIO; return -1; }
    return __real_fsync(fd);
}
void *__real_malloc(size_t size);
void *__real_calloc(size_t count, size_t size);
void *__real_realloc(void *pointer, size_t size);
void *__wrap_malloc(size_t size) {
    if (fail_allocation && !--fail_allocation) return NULL;
    return __real_malloc(size);
}
void *__wrap_calloc(size_t count, size_t size) {
    if (fail_allocation && !--fail_allocation) return NULL;
    return __real_calloc(count, size);
}
void *__wrap_realloc(void *pointer, size_t size) {
    if (fail_allocation && !--fail_allocation) return NULL;
    return __real_realloc(pointer, size);
}
#endif

static int failures;
static char error[256];

static void check(bool condition, const char *message) {
    if (!condition) {
        fprintf(stderr, "FAIL: %s (%s)\n", message, error);
        ++failures;
    }
}

static bool parse(cr_cheat_profile *profile, const char *text) {
    return cr_cheat_profile_parse(profile, text, strlen(text), error, sizeof(error));
}

static void unchanged_rejection(cr_cheat_profile *profile, const char *text, const char *message) {
    cr_cheat_entry *entries = profile->entries;
    check(!parse(profile, text), message);
    check(profile->entries == entries && profile->count == 1 &&
          !strcmp(profile->entries[0].code, "01234567 89AB"), "rejected import preserves destination");
}

static void test_roundtrip(void) {
    cr_cheat_profile source = {0}, decoded = {0};
    check(cr_cheat_profile_append(&source, "Étoile \"rare\" \\news\\", "01234567 89AB\r\nABCDEF12 3456\t7890", true,
          error, sizeof(error)), "append UTF-8, quotes and literal backslashes");
    check(cr_cheat_profile_append(&source, "", "", false, error, sizeof(error)), "disabled empty editor row is valid");
    check(!strcmp(source.entries[1].name, "Code 2"), "missing name uses numbered fallback");
    source.apply_after_load = true;
    source.apply_after_toggle = true;
    char *data = NULL;
    size_t size = 0;
    check(cr_cheat_profile_encode(&source, &data, &size, error, sizeof(error)), "encode profile");
    if (data) {
        check(strstr(data, "\\news") != NULL && strstr(data, "\\\\news") == NULL,
              "ordinary backslashes remain interoperable literal backslashes");
        check(cr_cheat_profile_parse(&decoded, data, size, error, sizeof(error)), "parse encoded profile");
        check(size == strlen(data), "encoded length excludes terminator");
        check(decoded.count == 2 && decoded.apply_after_load && decoded.apply_after_toggle,
              "roundtrip flags and count");
        if (decoded.count == 2) {
            check(!strcmp(source.entries[0].name, decoded.entries[0].name), "UTF-8 and quote/backslash name roundtrip");
            check(!strcmp(decoded.entries[0].code, "01234567 89AB  ABCDEF12 3456 7890"),
                  "multiline codes export with whitespace separators");
            check(decoded.entries[0].enabled && !decoded.entries[1].enabled && !decoded.entries[1].code[0],
                  "enabled and disabled empty codes roundtrip");
        }
    }
    free(data);
    check(cr_cheat_profile_clone(&decoded, &source, error, sizeof(error)), "clone profile");
    check(decoded.entries != source.entries && decoded.count == source.count, "clone owns independent entries");
    check(cr_cheat_profile_remove(&decoded, 0) && decoded.count == 1 &&
          !strcmp(decoded.entries[0].name, "Code 2"), "remove shifts remaining entries");
    check(!cr_cheat_profile_remove(&decoded, 1), "remove rejects invalid index");
    check(cr_cheat_profile_clone(&source, &source, error, sizeof(error)), "self clone");
    cr_cheat_profile_free(&source);
    cr_cheat_profile_free(&decoded);
}

static void test_parser(void) {
    cr_cheat_profile profile = {0};
    check(parse(&profile, "# synthetic\r\ncheat0_code = \"01234567 89AB\" # comment\r\n"
          "cheat0_enable = true\r\ncheat0_desc = \"\"\r\ncheats = 1\r\nunknown = metadata\r\n"),
          "CRLF, comments, ordering and missing engine handler");
    check(profile.count == 1 && !strcmp(profile.entries[0].name, "Code 1") &&
          !profile.apply_after_load && !profile.apply_after_toggle, "import defaults");
    unchanged_rejection(&profile, "cheats=1\ncheat0_handler=1\n", "memory handler explicitly rejected");
    check(strstr(error, "mémoire") != NULL, "memory handler error is actionable");
    unchanged_rejection(&profile, "cheats=1\ncheat0_handler=-1\n", "negative handler rejected");
    unchanged_rejection(&profile, "cheats=1\ncheat0_enable=true\n", "enabled empty code rejected");
    unchanged_rejection(&profile, "cheats=1\ncheat0_code=   \ncheat0_enable=true\n", "whitespace active code rejected");
    unchanged_rejection(&profile, "cheats=129\n", "excess count rejected");
    unchanged_rejection(&profile, "cheats=1\ncheat1_code=123\n", "index outside declared count rejected");
    unchanged_rejection(&profile, "cheats=128\ncheat128_code=123\n", "index outside hard limit rejected");
    unchanged_rejection(&profile, "cheats=0\ncheat999999999999999999999999999999999_code=123\n", "overflow index rejected");
    unchanged_rejection(&profile, "cheats=1\ncheat0_enable=maybe\n", "invalid boolean rejected");
    unchanged_rejection(&profile, "cheats=1\ncorvus_apply_after_toggle=maybe\n", "invalid profile flag rejected");
    unchanged_rejection(&profile, "cheats=1\ncorvus_apply_after_load=true\ncorvus_apply_after_load=false\n", "duplicate flag rejected");
    unchanged_rejection(&profile, "cheats=1\ncheats=1\n", "duplicate count rejected");
    unchanged_rejection(&profile, "cheats=1\ncheat0_desc=A\ncheat00_desc=B\n", "duplicate alias index field rejected");
    unchanged_rejection(&profile, "cheats=1\ncheat0_handler=0\ncheat0_handler=0\n", "duplicate handler rejected");
    unchanged_rejection(&profile, "cheat0_code=123\n", "missing count rejected");
    unchanged_rejection(&profile, "cheats=1\ncheat0_code=\"unterminated\n", "unterminated quote rejected");
    unchanged_rejection(&profile, "cheats=1\ncheat0_desc=\"name\" junk\n", "trailing quoted garbage rejected");
    unchanged_rejection(&profile, "cheats=1\ncheat0_desc=\"bad\tname\"\n", "control in description rejected");
    unchanged_rejection(&profile, "cheats=1\ncheat0_desc=\"\xc0\x80\"\n", "overlong UTF-8 rejected");
    unchanged_rejection(&profile, "cheats=1\ncheat0_desc=\"\xed\xa0\x80\"\n", "UTF-8 surrogate rejected");
    unchanged_rejection(&profile, "cheats=1\ncheat0_desc=\"\xf4\x90\x80\x80\"\n", "out of range UTF-8 rejected");
    const char nul[] = "cheats=0\n\0ignored";
    check(!cr_cheat_profile_parse(&profile, nul, sizeof(nul) - 1, error, sizeof(error)), "embedded NUL rejected");
    check(profile.count == 1, "NUL rejection preserves destination");
    check(!cr_cheat_profile_parse(&profile, "x", CR_CHEAT_FILE_MAX + 1u, error, sizeof(error)),
          "oversized input rejected before access");
    check(parse(&profile, "cheats=1\ncheat0_desc=\"\\news\"\n"), "literal backslash n accepted");
    check(!strcmp(profile.entries[0].name, "\\news"), "literal backslash n is not a newline escape");
    check(parse(&profile, "cheats=1\ncheat0_code=123\ncheat0_handler=0\ncheat0_enable=1\n"
          "corvus_apply_after_load=0\ncorvus_apply_after_toggle=1\n"), "explicit engine handler and numeric booleans");
    check(profile.entries[0].enabled && !profile.apply_after_load && profile.apply_after_toggle,
          "numeric booleans retain meaning");
    check(parse(&profile, "cheats=0\n"), "empty explicit list accepted");
    cr_cheat_profile_free(&profile);
}

static void test_bounds(void) {
    cr_cheat_profile profile = {0};
    char name[CR_CHEAT_NAME_SIZE + 1], code[CR_CHEAT_CODE_SIZE + 1];
    memset(name, 'n', sizeof(name)); name[sizeof(name) - 1] = '\0';
    memset(code, 'A', sizeof(code)); code[sizeof(code) - 1] = '\0';
    check(!cr_cheat_profile_append(&profile, name, "123", false, error, sizeof(error)), "name above byte limit rejected");
    check(!cr_cheat_profile_append(&profile, "test", code, false, error, sizeof(error)), "code above byte limit rejected");
    char input[CR_CHEAT_CODE_SIZE + 100];
    snprintf(input, sizeof(input), "cheats=1\ncheat0_desc=\"%s\"\n", name);
    check(!parse(&profile, input) && !profile.count, "import rejects name above byte limit");
    snprintf(input, sizeof(input), "cheats=1\ncheat0_code=\"%s\"\n", code);
    check(!parse(&profile, input) && !profile.count, "import rejects code above byte limit");
    name[CR_CHEAT_NAME_SIZE - 1] = '\0';
    code[CR_CHEAT_CODE_SIZE - 1] = '\0';
    for (size_t i = 0; i < CR_CHEAT_MAX_ENTRIES; ++i)
        check(cr_cheat_profile_append(&profile, name, code, true, error, sizeof(error)), "maximum bounded entries accepted");
    check(!cr_cheat_profile_append(&profile, "extra", "123", false, error, sizeof(error)), "entry beyond hard count rejected");
    char *data = NULL;
    size_t size = 0;
    cr_cheat_profile decoded = {0};
    check(cr_cheat_profile_encode(&profile, &data, &size, error, sizeof(error)) && size <= CR_CHEAT_FILE_MAX,
          "max profile export remains within file limit");
    if (data) check(cr_cheat_profile_parse(&decoded, data, size, error, sizeof(error)) &&
          decoded.count == CR_CHEAT_MAX_ENTRIES, "maximum lengths and count parse");
    free(data);
    cr_cheat_profile_free(&decoded);
    cr_cheat_profile_free(&profile);
}

static void write_file(const char *path, const char *data) {
    FILE *file = fopen(path, "wb");
    check(file != NULL, "open test file");
    if (file) { check(fwrite(data, 1, strlen(data), file) == strlen(data), "write test file"); fclose(file); }
}

static void test_files(void) {
    char directory[] = "/tmp/corvus-cheats-XXXXXX";
    check(mkdtemp(directory) != NULL, "create test private directory");
    char path[256], target_directory[256];
    snprintf(path, sizeof(path), "%s/profile.cht", directory);
    snprintf(target_directory, sizeof(target_directory), "%s/occupied", directory);
    cr_cheat_profile profile = {0}, loaded = {0};
    check(cr_cheat_profile_append(&profile, "Synthetic", "01234567 89AB", true, error, sizeof(error)), "file profile setup");
    bool found = true;
    check(cr_cheat_profile_load(&loaded, path, &found, error, sizeof(error)) && !found && !loaded.count,
          "missing file succeeds as empty");
    check(cr_cheat_profile_save(&profile, path, error, sizeof(error)), "save atomically");
    struct stat st;
    check(!stat(path, &st) && (st.st_mode & 0777) == 0600, "saved file permissions are private");
    check(cr_cheat_profile_load(&loaded, path, &found, error, sizeof(error)) && found && loaded.count == 1,
          "load saved file");
    write_file(path, "cheats=1\ncheat0_handler=1\n");
    cr_cheat_entry *old_entries = loaded.entries;
    check(!cr_cheat_profile_load(&loaded, path, &found, error, sizeof(error)) && found &&
          loaded.entries == old_entries && loaded.entries[0].enabled, "corrupt file keeps loaded list");
    check(!cr_cheat_profile_load(&loaded, directory, &found, error, sizeof(error)) && found &&
          loaded.entries == old_entries, "non-regular file rejected without mutation");
    check(!mkdir(target_directory, 0700), "create occupied rename destination");
    check(!cr_cheat_profile_save(&profile, target_directory, error, sizeof(error)), "failed rename reported");
    check(!stat(target_directory, &st) && S_ISDIR(st.st_mode), "failed save retains old destination");
    check(cr_cheat_profile_save(&profile, path, error, sizeof(error)), "restore valid saved profile");
#ifdef CR_CHEAT_TEST_FAILURES
    strcpy(profile.entries[0].name, "Changed");
    fail_sync = 1;
    check(!cr_cheat_profile_save(&profile, path, error, sizeof(error)), "injected file fsync failure reported");
    check(cr_cheat_profile_load(&loaded, path, &found, error, sizeof(error)) &&
          !strcmp(loaded.entries[0].name, "Synthetic"), "failed fsync retains previous file bytes");
    fail_sync = 2;
    check(!cr_cheat_profile_save(&profile, path, error, sizeof(error)) && strstr(error, "remplacé"),
          "directory fsync failure explicitly reports replacement durability uncertainty");
    check(cr_cheat_profile_load(&loaded, path, &found, error, sizeof(error)) &&
          !strcmp(loaded.entries[0].name, "Changed"), "replacement remains complete on directory fsync failure");
    fail_allocation = 1;
    check(!cr_cheat_profile_append(&loaded, "No memory", "123", false, error, sizeof(error)) && loaded.count == 1,
          "allocation failure preserves list on append");
    old_entries = loaded.entries;
    fail_allocation = 1;
    check(!parse(&loaded, "cheats=0\n") && loaded.entries == old_entries && loaded.count == 1,
          "allocation failure preserves list on import");
    fail_allocation = 1;
    check(!cr_cheat_profile_clone(&loaded, &profile, error, sizeof(error)) && loaded.entries == old_entries,
          "allocation failure preserves clone destination");
    fail_allocation = 1;
    check(!cr_cheat_profile_save(&profile, path, error, sizeof(error)), "allocation failure aborts save");
    check(cr_cheat_profile_load(&loaded, path, &found, error, sizeof(error)) && loaded.count == 1,
          "allocation failure leaves valid stored file");
#endif
    unlink(path);
    check(cr_cheat_profile_load(&loaded, path, &found, error, sizeof(error)) && !found && !loaded.count,
          "missing file replaces prior profile with empty defaults");
    rmdir(target_directory);
    check(!rmdir(directory), "no temporary files left after failed saves");
    cr_cheat_profile_free(&profile);
    cr_cheat_profile_free(&loaded);
}

int main(void) {
    cr_cheat_profile bom = {0}; char bom_error[256];
    const char bom_text[] = "\xef\xbb\xbf" "cheats = 0\n";
    check(cr_cheat_profile_parse(&bom,bom_text,sizeof(bom_text)-1,bom_error,sizeof(bom_error)) &&
          bom.count == 0,"UTF-8 BOM accepted before first field");
    cr_cheat_profile_free(&bom);
    test_roundtrip();
    test_parser();
    test_bounds();
    test_files();
    if (failures) return EXIT_FAILURE;
    puts("native_cheat_store_test: PASS");
    return EXIT_SUCCESS;
}
