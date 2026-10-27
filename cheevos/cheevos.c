// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
#define _POSIX_C_SOURCE 200809L
#include "cheevos.h"
#include <rc_client.h>
#include <rc_hash.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define CR_RA_REQUESTS 32
#define CR_RA_RESPONSE_LIMIT (4u * 1024u * 1024u)

typedef struct {
    uint64_t id;
    rc_client_server_callback_t callback;
    void *userdata;
    char *url, *post;
    bool dispatched, authentication;
} cr_ra_request;
struct cr_cheevos {
    rc_client_t *client;
    cr_cheevos_host host;
    rc_client_async_handle_t *login, *load;
    cr_ra_request requests[CR_RA_REQUESTS];
    cr_cheevos_read_memory memory;
    void *memory_context;
    char hash[33], message[256];
    bool selected, closing;
    uint64_t login_completions, load_completions;
};
static atomic_uint_fast64_t next_request = ATOMIC_VAR_INIT(1);
static const char cancelled[] = "{\"Success\":false,\"Error\":\"Operation cancelled\",\"Code\":\"invalid_state\"}";

static void message(cr_cheevos *c, const char *text) {
    snprintf(c->message, sizeof(c->message), "%s", text);
}
static void notify(cr_cheevos *c, const cr_cheevos_notification *notification) {
    if (!c->closing && c->host.notification)
        c->host.notification(c->host.context, notification);
}
static void erase_free(char *text) {
    if (!text) return;
    volatile unsigned char *p = (volatile unsigned char *)text;
    size_t count = strlen(text);
    while (count--) *p++ = 0;
    free(text);
}
static char *duplicate(const char *text, size_t limit) {
    if (!text) return NULL;
    size_t size = strlen(text);
    if (size > limit) return NULL;
    char *result = malloc(size + 1);
    if (result) memcpy(result, text, size + 1);
    return result;
}
static void fail_request(rc_client_server_callback_t callback, void *userdata) {
    rc_api_server_response_t response = {cancelled, sizeof(cancelled) - 1, 400};
    callback(&response, userdata);
}
static uint32_t read_memory(uint32_t address, uint8_t *bytes, uint32_t size, rc_client_t *client) {
    cr_cheevos *c = rc_client_get_userdata(client);
    if (!c || c->closing || !c->memory || !bytes || !size) return 0;
    size_t result = c->memory(c->memory_context, address, bytes, size);
    return result <= size ? (uint32_t)result : 0;
}
static rc_clock_t milliseconds(const rc_client_t *client) {
    const cr_cheevos *c = rc_client_get_userdata(client);
    if (c && c->closing) return UINT64_MAX / 4;
    struct timespec now;
    if (clock_gettime(CLOCK_MONOTONIC, &now) != 0) return 0;
    return (rc_clock_t)now.tv_sec * 1000 + (rc_clock_t)now.tv_nsec / 1000000;
}
static void server_call(const rc_api_request_t *request, rc_client_server_callback_t callback,
                        void *userdata, rc_client_t *client) {
    cr_cheevos *c = rc_client_get_userdata(client);
    if (!c || c->closing || !c->host.request || !request->url ||
        strncmp(request->url, "https://retroachievements.org/", 30) != 0) {
        fail_request(callback, userdata);
        return;
    }
    for (size_t i = 0; i < CR_RA_REQUESTS; ++i) {
        cr_ra_request *slot = &c->requests[i];
        if (slot->id) continue;
        char *url = duplicate(request->url, 1024);
        char *post = request->post_data ? duplicate(request->post_data, 8192) : NULL;
        if (!url || (request->post_data && !post)) {
            erase_free(url); erase_free(post);
            fail_request(callback, userdata);
            return;
        }
        *slot = (cr_ra_request){.id = atomic_fetch_add(&next_request, 1), .callback = callback,
                               .userdata = userdata, .url = url, .post = post,
                               .authentication = post && strncmp(post, "r=login2&", 9) == 0};
        return;
    }
    message(c, "Trop de requêtes RetroAchievements en cours");
    fail_request(callback, userdata);
}
static void dispatch(cr_cheevos *c) {
    for (size_t i = 0; i < CR_RA_REQUESTS; ++i) {
        cr_ra_request *slot = &c->requests[i];
        if (!slot->id || slot->dispatched) continue;
        uint64_t id = slot->id;
        char *url = slot->url, *post = slot->post;
        slot->dispatched = true;
        slot->url = slot->post = NULL;
        c->host.request(c->host.context, id, url, post);
        erase_free(url); erase_free(post);
    }
}
static void game_loaded(int result, const char *error, rc_client_t *client, void *userdata) {
    (void)error;
    cr_cheevos *c = userdata;
    ++c->load_completions;
    c->load = NULL;
    if (c->closing) return;
    const rc_client_game_t *game = rc_client_get_game_info(client);
    if (result == RC_OK && game) {
        message(c, rc_client_has_achievements(client) ? "Succès du jeu prêts, mode normal" : "Ce jeu ne propose pas de succès");
        rc_client_user_game_summary_t summary;
        rc_client_get_user_game_summary(client, &summary);
        cr_cheevos_notification notification = {.type = CR_CHEEVOS_GAME_READY, .id = game->id,
            .unlocked = summary.num_unlocked_achievements, .total = summary.num_core_achievements};
        snprintf(notification.title, sizeof(notification.title), "%s", game->title);
        if (rc_client_game_get_image_url(game, notification.image_url, sizeof(notification.image_url)) != RC_OK)
            notification.image_url[0] = 0;
        notify(c, &notification);
    } else
        message(c, result == RC_NO_GAME_LOADED ? "Jeu non reconnu par RetroAchievements" : "Succès du jeu indisponibles");
}
static void begin_game(cr_cheevos *c) {
    if (!c->closing && c->hash[0] && c->memory && rc_client_get_user_info(c->client)) {
        message(c, "Identification du jeu...");
        uint64_t before = c->load_completions;
        rc_client_async_handle_t *handle = rc_client_begin_load_game(c->client, c->hash, game_loaded, c);
        if (c->load_completions == before) c->load = handle;
    }
}
static void logged_in(int result, const char *error, rc_client_t *client, void *userdata) {
    (void)error;
    cr_cheevos *c = userdata;
    ++c->login_completions;
    c->login = NULL;
    if (c->closing) return;
    const rc_client_user_t *user = rc_client_get_user_info(client);
    if (result != RC_OK || !user) {
        message(c, "Connexion RetroAchievements impossible");
        if (c->host.save_credentials) c->host.save_credentials(c->host.context, "", "");
        return;
    }
    if (c->host.save_credentials) c->host.save_credentials(c->host.context, user->username, user->token);
    message(c, "Compte RetroAchievements connecté");
    begin_game(c);
}
static void event(const rc_client_event_t *event, rc_client_t *client) {
    cr_cheevos *c = rc_client_get_userdata(client);
    if (!c || c->closing) return;
    char text[384];
    cr_cheevos_notification notification = {0};
    switch (event->type) {
    case RC_CLIENT_EVENT_ACHIEVEMENT_TRIGGERED:
        if (event->achievement) {
            notification.type = CR_CHEEVOS_UNLOCKED;
            notification.id = event->achievement->id;
            notification.points = event->achievement->points;
            snprintf(notification.title, sizeof(notification.title), "%s", event->achievement->title);
            snprintf(notification.detail, sizeof(notification.detail), "%s", event->achievement->description);
            if (rc_client_achievement_get_image_url(event->achievement, RC_CLIENT_ACHIEVEMENT_STATE_UNLOCKED,
                    notification.image_url, sizeof(notification.image_url)) != RC_OK)
                notification.image_url[0] = 0;
            notify(c, &notification);
            if (!c->host.notification && c->host.notice) {
                snprintf(text, sizeof(text), "Succès débloqué : %s", event->achievement->title);
                c->host.notice(c->host.context, text);
            }
        }
        break;
    case RC_CLIENT_EVENT_ACHIEVEMENT_PROGRESS_INDICATOR_SHOW:
    case RC_CLIENT_EVENT_ACHIEVEMENT_PROGRESS_INDICATOR_UPDATE:
        if (event->achievement) {
            notification.type = CR_CHEEVOS_PROGRESS;
            notification.id = event->achievement->id;
            notification.percent = event->achievement->measured_percent;
            snprintf(notification.title, sizeof(notification.title), "%s", event->achievement->title);
            snprintf(notification.detail, sizeof(notification.detail), "%s", event->achievement->measured_progress);
            if (rc_client_achievement_get_image_url(event->achievement, RC_CLIENT_ACHIEVEMENT_STATE_ACTIVE,
                    notification.image_url, sizeof(notification.image_url)) != RC_OK)
                notification.image_url[0] = 0;
            notify(c, &notification);
        }
        break;
    case RC_CLIENT_EVENT_ACHIEVEMENT_PROGRESS_INDICATOR_HIDE:
        notification.type = CR_CHEEVOS_PROGRESS_HIDE;
        notify(c, &notification);
        break;
    case RC_CLIENT_EVENT_DISCONNECTED:
        message(c, "Connexion interrompue, envoi des succès différé");
        break;
    case RC_CLIENT_EVENT_RECONNECTED:
        message(c, "Succès synchronisés");
        break;
    case RC_CLIENT_EVENT_SERVER_ERROR:
        message(c, "Une requête RetroAchievements a échoué");
        break;
    case RC_CLIENT_EVENT_GAME_COMPLETED:
        notification.type = CR_CHEEVOS_COMPLETED;
        const rc_client_game_t *game = rc_client_get_game_info(client);
        if (game) {
            notification.id = game->id;
            snprintf(notification.title, sizeof(notification.title), "%s", game->title);
            if (rc_client_game_get_image_url(game, notification.image_url, sizeof(notification.image_url)) != RC_OK)
                notification.image_url[0] = 0;
        }
        notify(c, &notification);
        if (!c->host.notification && c->host.notice)
            c->host.notice(c->host.context, "Tous les succès du jeu sont débloqués");
        break;
    default:
        break;
    }
}
cr_cheevos *cr_cheevos_create(cr_cheevos_host host) {
    cr_cheevos *c = calloc(1, sizeof(*c));
    if (!c) return NULL;
    c->host = host;
    c->client = rc_client_create(read_memory, server_call);
    if (!c->client) { free(c); return NULL; }
    rc_client_set_userdata(c->client, c);
    rc_client_set_hardcore_enabled(c->client, 0);
    rc_client_set_allow_background_memory_reads(c->client, 0);
    rc_client_set_event_handler(c->client, event);
    rc_client_set_get_time_millisecs_function(c->client, milliseconds);
    message(c, "Connectez votre compte RetroAchievements");
    return c;
}
static void cancel(cr_cheevos *c, bool authentication) {
    c->closing = true;
    if (authentication && c->login) rc_client_abort_async(c->client, c->login);
    if (c->load) rc_client_abort_async(c->client, c->load);
    if (authentication) c->login = NULL;
    c->load = NULL;
    /* Retire deferred UI events while the game is still attached to rc_client. */
    rc_client_reset(c->client);
    rc_client_unload_game(c->client);
    for (size_t i = 0; i < CR_RA_REQUESTS; ++i) {
        cr_ra_request request = c->requests[i];
        if (!authentication && request.authentication) continue;
        c->requests[i] = (cr_ra_request){0};
        if (request.id) {
            erase_free(request.url); erase_free(request.post);
            fail_request(request.callback, request.userdata);
        }
    }
    // Due retries receive a terminal cancellation while their client is still valid.
    rc_client_idle(c->client);
    c->closing = false;
    notify(c, &(cr_cheevos_notification){.type = CR_CHEEVOS_CLEAR});
}
void cr_cheevos_destroy(cr_cheevos *c) {
    if (!c) return;
    cancel(c, true);
    rc_client_destroy(c->client);
    free(c);
}
static bool valid_credentials(const char *user, const char *secret, size_t limit) {
    if (!user || !secret || !*user || !*secret || strlen(user) > 64 || strlen(secret) > limit) return false;
    for (const unsigned char *p = (const unsigned char *)user; *p; ++p)
        if (*p <= 32 || *p == 127) return false;
    return true;
}
static bool login(cr_cheevos *c, const char *user, const char *secret, bool token) {
    if (!c) return false;
    if (!valid_credentials(user, secret, token ? 512 : 256)) {
        message(c, "Identifiants RetroAchievements invalides");
        return false;
    }
    cancel(c, true);
    rc_client_logout(c->client);
    message(c, "Connexion à RetroAchievements...");
    uint64_t before = c->login_completions;
    rc_client_async_handle_t *handle = token ? rc_client_begin_login_with_token(c->client, user, secret, logged_in, c)
                                            : rc_client_begin_login_with_password(c->client, user, secret, logged_in, c);
    if (c->login_completions == before) c->login = handle;
    return c->login != NULL || rc_client_get_user_info(c->client) != NULL;
}
bool cr_cheevos_login(cr_cheevos *c, const char *user, const char *password) { return login(c, user, password, false); }
bool cr_cheevos_login_token(cr_cheevos *c, const char *user, const char *token) { return login(c, user, token, true); }
void cr_cheevos_logout(cr_cheevos *c) {
    if (!c) return;
    cancel(c, true);
    rc_client_logout(c->client);
    if (c->host.save_credentials) c->host.save_credentials(c->host.context, "", "");
    message(c, "Compte RetroAchievements déconnecté");
}
void cr_cheevos_unload_game(cr_cheevos *c) {
    if (!c) return;
    // A late award callback must not refer to a different (or freed) game.
    cancel(c, false);
    c->memory = NULL;
    c->memory_context = NULL;
    c->selected = false;
    c->hash[0] = 0;
    message(c, "Ouvrez un jeu pour consulter ses succès");
}
bool cr_cheevos_load_game(cr_cheevos *c, const char *path, uint32_t console,
                           cr_cheevos_read_memory memory, void *context) {
    if (!c) return false;
    cr_cheevos_unload_game(c);
    c->selected = true;
    if (!memory) { message(c, "Cœur sans accès mémoire RetroAchievements"); return false; }
    c->memory = memory;
    c->memory_context = context;
    if (!path || (console != 4 && console != 5 && console != 6) ||
        !rc_hash_generate_from_file(c->hash, console, path)) {
        message(c, "Identification RetroAchievements impossible");
        return false;
    }
    message(c, "Connectez votre compte pour activer les succès");
    begin_game(c);
    return true;
}
void cr_cheevos_response(cr_cheevos *c, uint64_t id, int status, const char *body, size_t size) {
    if (!c || !id) return;
    for (size_t i = 0; i < CR_RA_REQUESTS; ++i) {
        cr_ra_request request = c->requests[i];
        if (request.id != id) continue;
        c->requests[i] = (cr_ra_request){0};
        erase_free(request.url); erase_free(request.post);
        if (!body || size > CR_RA_RESPONSE_LIMIT) {
            rc_api_server_response_t response = {NULL, 0, RC_API_SERVER_RESPONSE_RETRYABLE_CLIENT_ERROR};
            request.callback(&response, request.userdata);
        } else {
            rc_api_server_response_t response = {body, size, status};
            request.callback(&response, request.userdata);
        }
        return;
    }
}
void cr_cheevos_frame(cr_cheevos *c) {
    if (!c) return;
    if (c->memory && rc_client_is_game_loaded(c->client)) rc_client_do_frame(c->client);
    else rc_client_idle(c->client);
    dispatch(c);
}
void cr_cheevos_idle(cr_cheevos *c) { if (c) { rc_client_idle(c->client); dispatch(c); } }
void cr_cheevos_reset(cr_cheevos *c) {
    if (c) {
        rc_client_reset(c->client);
        notify(c, &(cr_cheevos_notification){.type = CR_CHEEVOS_PROGRESS_HIDE});
    }
}
size_t cr_cheevos_progress_size(cr_cheevos *c) { return c ? rc_client_progress_size(c->client) : 0; }
bool cr_cheevos_progress_save(cr_cheevos *c, void *bytes, size_t size) {
    return c && bytes && rc_client_serialize_progress_sized(c->client, bytes, size) == RC_OK;
}
bool cr_cheevos_progress_restore(cr_cheevos *c, const void *bytes, size_t size) {
    if (!c) return false;
    if (bytes && rc_client_deserialize_progress_sized(c->client, bytes, size) == RC_OK) return true;
    rc_client_reset(c->client);
    return false;
}
void cr_cheevos_get_status(cr_cheevos *c, cr_cheevos_status *out) {
    if (!out) return;
    memset(out, 0, sizeof(*out));
    if (!c) { snprintf(out->message, sizeof(out->message), "RetroAchievements indisponible"); return; }
    const rc_client_user_t *user = rc_client_get_user_info(c->client);
    const rc_client_game_t *game = rc_client_get_game_info(c->client);
    out->connected = user != NULL;
    out->pending = c->login || c->load;
    out->game_loaded = game != NULL;
    out->memory_supported = c->memory != NULL;
    snprintf(out->username, sizeof(out->username), "%s", user ? user->display_name : "");
    snprintf(out->game_title, sizeof(out->game_title), "%s", game ? game->title : "");
    snprintf(out->message, sizeof(out->message), "%s", c->selected && !c->memory
             ? "Cœur sans accès mémoire RetroAchievements" : c->message);
    if (game) {
        rc_client_user_game_summary_t summary;
        rc_client_get_user_game_summary(c->client, &summary);
        out->unlocked = summary.num_unlocked_achievements;
        out->total = summary.num_core_achievements;
        out->points = summary.points_unlocked;
    }
}
size_t cr_cheevos_achievement_count(cr_cheevos *c) {
    if (!c || !rc_client_is_game_loaded(c->client)) return 0;
    rc_client_achievement_list_t *list = rc_client_create_achievement_list(c->client, RC_CLIENT_ACHIEVEMENT_CATEGORY_CORE,
                                                                       RC_CLIENT_ACHIEVEMENT_LIST_GROUPING_LOCK_STATE);
    if (!list) return 0;
    size_t count = 0;
    for (uint32_t i = 0; i < list->num_buckets; ++i) count += list->buckets[i].num_achievements;
    rc_client_destroy_achievement_list(list);
    return count;
}
bool cr_cheevos_get_achievement(cr_cheevos *c, size_t index, cr_cheevos_achievement *out) {
    if (!c || !out || !rc_client_is_game_loaded(c->client)) return false;
    rc_client_achievement_list_t *list = rc_client_create_achievement_list(c->client, RC_CLIENT_ACHIEVEMENT_CATEGORY_CORE,
                                                                       RC_CLIENT_ACHIEVEMENT_LIST_GROUPING_LOCK_STATE);
    if (!list) return false;
    const rc_client_achievement_t *achievement = NULL;
    for (uint32_t i = 0; i < list->num_buckets; ++i) {
        if (index < list->buckets[i].num_achievements) { achievement = list->buckets[i].achievements[index]; break; }
        index -= list->buckets[i].num_achievements;
    }
    if (achievement) {
        *out = (cr_cheevos_achievement){.id = achievement->id, .points = achievement->points,
                                     .unlocked = achievement->unlocked != 0};
        snprintf(out->title, sizeof(out->title), "%s", achievement->title);
        snprintf(out->description, sizeof(out->description), "%s", achievement->description);
    }
    rc_client_destroy_achievement_list(list);
    return achievement != NULL;
}
