// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
#define _POSIX_C_SOURCE 200809L
#include "cheevos.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct {
    cr_cheevos *client;
    uint8_t ram[32];
    uint64_t held_id;
    unsigned calls, awards, notices, credential_saves;
    bool hold, fail, refuse_login, measured;
    unsigned games, unlocks, progresses, hides, clears, completions;
    cr_cheevos_notification game, unlock, progress;
    char user[96], token[128];
} fixture;
static unsigned checks;
static void check(bool value, const char *description) {
    if (!value) { fprintf(stderr, "FAIL %s\n", description); exit(1); }
    ++checks;
}
static size_t memory(void *context, uint32_t address, uint8_t *out, size_t size) {
    fixture *f = context;
    if (address >= sizeof(f->ram)) return 0;
    if (size > sizeof(f->ram) - address) size = sizeof(f->ram) - address;
    memcpy(out, f->ram + address, size);
    return size;
}
static const char login_response[] = "{\"Success\":true,\"User\":\"CorvusTest\",\"Token\":\"SyntheticSessionToken\",\"Score\":0,\"SoftcoreScore\":0,\"Messages\":0}";
static const char game_response[] =
    "{\"Success\":true,\"GameId\":731,\"Title\":\"Corvus Original Test\",\"ConsoleId\":4,"
    "\"ImageIconUrl\":\"https://media.retroachievements.org/Images/1.png\","
    "\"RichPresenceGameId\":731,\"RichPresencePatch\":\"\",\"Sets\":[{"
    "\"AchievementSetId\":731,\"GameId\":731,\"Title\":null,\"Type\":\"core\","
    "\"ImageIconUrl\":\"https://media.retroachievements.org/Images/1.png\",\"Achievements\":[{"
    "\"ID\":9001,\"Title\":\"Memory transition\",\"Description\":\"Observe a real byte change\","
    "\"Flags\":3,\"Points\":5,\"MemAddr\":\"%s\",\"Author\":\"CorvusSoftware\","
    "\"BadgeName\":\"00001\",\"Created\":1700000000,\"Modified\":1700000000}],\"Leaderboards\":[]}]}";
static void request(void *context, uint64_t id, const char *url, const char *post) {
    fixture *f = context;
    ++f->calls;
    check(strncmp(url, "https://retroachievements.org/", 30) == 0, "HTTPS official API origin");
    check(post != NULL, "official API uses POST body");
    if (f->hold) { f->held_id = id; return; }
    if (f->fail) { cr_cheevos_response(f->client, id, 503, "", 0); return; }
    const char *body = NULL;
    char definition[2048];
    if (strstr(post, "r=login2")) body = f->refuse_login ? "{\"Success\":false,\"Error\":\"Invalid credentials\"}" : login_response;
    else if (strstr(post, "r=achievementsets")) {
        snprintf(definition, sizeof(definition), game_response, f->measured ? "M:0xH0001=10" : "0xH0001=7");
        body = definition;
    }
    else if (strstr(post, "r=startsession")) {
        check(strstr(post, "&h=0") != NULL, "only normal mode is advertised");
        body = "{\"Success\":true,\"Unlocks\":[],\"HardcoreUnlocks\":[]}";
    } else if (strstr(post, "r=awardachievement")) {
        check(strstr(post, "&h=0") != NULL, "normal mode awards only");
        ++f->awards;
        body = "{\"Success\":true,\"AchievementID\":9001,\"Score\":0,\"SoftcoreScore\":5,\"AchievementsRemaining\":0}";
    } else if (strstr(post, "r=ping")) body = "{\"Success\":true}";
    check(body != NULL, "recognized official request");
    cr_cheevos_response(f->client, id, 200, body, strlen(body));
}
static void save(void *context, const char *user, const char *token) {
    fixture *f = context;
    ++f->credential_saves;
    snprintf(f->user, sizeof(f->user), "%s", user);
    snprintf(f->token, sizeof(f->token), "%s", token);
}
static void notice(void *context, const char *text) {
    fixture *f = context;
    ++f->notices;
    check(strstr(text, "SyntheticSessionToken") == NULL, "notices do not expose credentials");
}
static void notification(void *context, const cr_cheevos_notification *event) {
    fixture *f = context;
    check(strstr(event->image_url, "SyntheticSessionToken") == NULL, "public image metadata has no credentials");
    switch (event->type) {
    case CR_CHEEVOS_GAME_READY: f->games++; f->game = *event; break;
    case CR_CHEEVOS_UNLOCKED: f->unlocks++; f->unlock = *event; break;
    case CR_CHEEVOS_PROGRESS: f->progresses++; f->progress = *event; break;
    case CR_CHEEVOS_PROGRESS_HIDE: f->hides++; break;
    case CR_CHEEVOS_COMPLETED: f->completions++; break;
    case CR_CHEEVOS_CLEAR: f->clears++; break;
    }
}
static cr_cheevos *create(fixture *f) {
    f->client = cr_cheevos_create((cr_cheevos_host){.context = f, .request = request, .save_credentials = save, .notice = notice, .notification = notification});
    check(f->client != NULL, "client creation");
    return f->client;
}
static void pump(cr_cheevos *c) { for (unsigned i = 0; i < 8; ++i) cr_cheevos_idle(c); }
static cr_cheevos_status status(cr_cheevos *c) { cr_cheevos_status s; cr_cheevos_get_status(c, &s); return s; }
int main(void) {
    char path[] = "/tmp/corvus-cheevos-XXXXXX";
    int descriptor = mkstemp(path);
    check(descriptor >= 0, "original ROM fixture");
    uint8_t rom[512] = {0};
    memcpy(rom + 0x134, "CORVUS TEST", 11);
    check(write(descriptor, rom, sizeof(rom)) == (ssize_t)sizeof(rom) && close(descriptor) == 0, "write original ROM");
    fixture f = {0}; cr_cheevos *c = create(&f);
    check(!cr_cheevos_login(c, "", "secret"), "empty credentials rejected");
    check(cr_cheevos_load_game(c, path, 4, memory, &f), "ROM hash deferred before login");
    check(!status(c).game_loaded, "game is not invented before server confirmation");
    check(cr_cheevos_login(c, "CorvusTest", "SyntheticPassword"), "official password login starts");
    pump(c);
    check(status(c).connected && status(c).game_loaded, "login and real response parsing load game");
    check(strcmp(f.user, "CorvusTest") == 0 && strcmp(f.token, "SyntheticSessionToken") == 0,
          "only the server token is persisted");
    check(status(c).total == 1 && status(c).unlocked == 0 && f.awards == 0, "server achievement initially locked");
    cr_cheevos_achievement row;
    check(cr_cheevos_achievement_count(c) == 1 && cr_cheevos_get_achievement(c, 0, &row) &&
          strcmp(row.title, "Memory transition") == 0 && row.points == 5 && !row.unlocked,
          "achievement list exposes actual server definitions");
    for (unsigned i = 0; i < 8; ++i) cr_cheevos_frame(c);
    check(f.awards == 0, "unchanged RAM cannot unlock an achievement");
    size_t progress_size = cr_cheevos_progress_size(c);
    uint8_t *progress = malloc(progress_size);
    check(progress && progress_size && cr_cheevos_progress_save(c, progress, progress_size), "evaluation progress serialized");
    check(cr_cheevos_progress_restore(c, progress, progress_size), "saved progress restores in normal mode");
    free(progress);
    f.ram[1] = 7;
    cr_cheevos_frame(c); pump(c);
    check(f.awards == 1 && status(c).unlocked == 1 && f.unlocks == 1, "real RAM transition unlocks and submits once");
    cr_cheevos_frame(c); pump(c);
    check(f.awards == 1, "unlock is not submitted twice");
    check(f.completions == 1, "all achievements completed uses the same typed notification channel");
    check(!cr_cheevos_progress_restore(c, NULL, 0), "missing save-state progress resets evaluation safely");
    cr_cheevos_unload_game(c);
    check(!status(c).game_loaded && cr_cheevos_achievement_count(c) == 0, "unload clears previous game's achievements");
    check(!cr_cheevos_load_game(c, path, 4, NULL, NULL) && !status(c).memory_supported &&
          strstr(status(c).message, "sans accès mémoire"), "unsupported core is honest and never supplies fake bytes");
    cr_cheevos_logout(c);
    check(!status(c).connected && !f.token[0], "logout clears the saved token");
    check(cr_cheevos_login_token(c, "CorvusTest", "SyntheticSessionToken"), "token login starts");
    pump(c); check(status(c).connected, "token login succeeds");
    cr_cheevos_destroy(c);

    fixture old = {.hold = true}; c = create(&old);
    check(cr_cheevos_login(c, "CorvusTest", "SyntheticPassword"), "pending login starts"); pump(c);
    uint64_t stale = old.held_id;
    check(stale != 0 && status(c).pending, "transport request remains asynchronous");
    cr_cheevos_destroy(c);
    fixture replacement = {0}; c = create(&replacement);
    cr_cheevos_response(c, stale, 200, login_response, strlen(login_response));
    check(!status(c).connected, "completion from destroyed client cannot log in its replacement");
    replacement.refuse_login = true;
    check(cr_cheevos_login(c, "CorvusTest", "SyntheticPassword"), "failed login request starts"); pump(c);
    check(!status(c).connected && !status(c).pending, "invalid credentials do not leave pending state");
    replacement.refuse_login = false;
    check(cr_cheevos_login_token(c, "CorvusTest", "SyntheticSessionToken"), "retry after failure starts"); pump(c);
    check(status(c).connected, "retry after invalid credentials succeeds");
    cr_cheevos_destroy(c);

    fixture offline = {0}; c = create(&offline);
    check(cr_cheevos_login_token(c, "CorvusTest", "SyntheticSessionToken"), "offline fixture login starts"); pump(c);
    check(cr_cheevos_load_game(c, path, 4, memory, &offline), "offline fixture game starts"); pump(c);
    cr_cheevos_frame(c); offline.fail = true; offline.ram[1] = 7; cr_cheevos_frame(c); pump(c);
    check(strstr(status(c).message, "interrompue") != NULL, "network failure reports deferred synchronization");
    cr_cheevos_destroy(c);
    fixture measured = {.measured = true}; c = create(&measured);
    check(cr_cheevos_login_token(c, "CorvusTest", "SyntheticSessionToken"), "measured fixture authenticates"); pump(c);
    check(cr_cheevos_load_game(c, path, 4, memory, &measured), "measured fixture game starts"); pump(c);
    check(measured.games == 1 && measured.game.id == 731 && measured.game.total == 1 &&
          measured.game.unlocked == 0 && strcmp(measured.game.title, "Corvus Original Test") == 0 &&
          strcmp(measured.game.image_url, "https://media.retroachievements.org/Images/1.png") == 0,
          "confirmed game emits real title, summary and icon metadata");
    cr_cheevos_frame(c); measured.ram[1] = 4; cr_cheevos_frame(c);
    check(measured.progresses > 0 && strcmp(measured.progress.detail, "4/10") == 0 &&
          measured.progress.percent > 39.9f && measured.progress.percent < 40.1f &&
          strstr(measured.progress.image_url, "/Badge/00001_lock.png"),
          "measured RAM emits progress and the locked badge URL");
    unsigned updates = measured.progresses;
    cr_cheevos_frame(c);
    check(measured.progresses == updates, "unchanged RAM does not flood progress notifications");
    measured.ram[1] = 5; cr_cheevos_frame(c);
    check(measured.progresses > updates && strcmp(measured.progress.detail, "5/10") == 0,
          "progress changes update the existing indicator");
    unsigned hides = measured.hides;
    cr_cheevos_reset(c);
    check(measured.hides > hides, "reset explicitly hides stale measured progress");
    measured.ram[1] = 0; cr_cheevos_frame(c);
    measured.ram[1] = 10; cr_cheevos_frame(c); pump(c);
    check(measured.unlocks == 1 && measured.unlock.id == 9001 && measured.unlock.points == 5 &&
          strcmp(measured.unlock.title, "Memory transition") == 0 &&
          strstr(measured.unlock.image_url, "/Badge/00001.png") && !strstr(measured.unlock.image_url, "_lock"),
          "unlock emits one colored badge and actual achievement metadata");
    unsigned clears = measured.clears;
    cr_cheevos_unload_game(c);
    check(measured.clears > clears, "unload clears game notifications");
    measured.hold = true;
    check(cr_cheevos_load_game(c, path, 4, memory, &measured), "late game request starts"); pump(c);
    uint64_t late_game = measured.held_id;
    unsigned games = measured.games;
    cr_cheevos_unload_game(c);
    char late_definition[2048];
    snprintf(late_definition, sizeof(late_definition), game_response, "M:0xH0001=10");
    cr_cheevos_response(c, late_game, 200, late_definition, strlen(late_definition));
    check(measured.games == games && !status(c).game_loaded, "late old-game response cannot resurrect a popup");
    cr_cheevos_destroy(c);
    check(unlink(path) == 0, "remove original fixture");
    printf("PASS %u native RetroAchievements checks\n", checks);
    return 0;
}
