// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
#define _POSIX_C_SOURCE 200809L
#include "menu_driver.h"
#include "menu_entries.h"
#include "menu_cbs.h"
#include "menu_setting.h"
#include "menu_internal.h"
#include "play_layout.h"
#include "overlay.h"
#include "core_registry.h"
#include "interface_draw.h"
#include "interface_layout.h"
#include "state_store.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef cr_menu_row row;

static void quick_clamp(cr_ui *u);
static int target(const cr_ui *u, float x, float y);
static bool has_back(const cr_ui *u) {
    return u->quick || u->depth || (u->page != CR_MAIN && u->page != CR_LIBRARY &&
           u->page != CR_SAVES_LIBRARY && u->page != CR_CORES && u->page != CR_SETTINGS);
}

static void press_controls(cr_ui *u, unsigned mask) {
    /* Keep brief input visible for a rendered frame, independently of the engine latch. */
    u->feedback_pulses |= mask;
    for (int i = 0; i < 10; i++)
        if (mask & (1u << i))
            u->control_feedback[i] = 1;
}
static float body_bottom(const cr_ui *u) {
    return cr_interface_bottom(u);
}

static void clamp_scroll(cr_ui *u) {
    float content = cr_interface_content_height(u, cr_menu_entry_count(u));
    if (u->page == CR_ABOUT || u->page == CR_LICENSE || u->page == CR_CORE_INFO ||
        ((u->page == CR_SUBSYSTEMS || u->page == CR_CORE_OPTIONS) && !cr_menu_entry_count(u)))
        content = fmaxf(400, u->text_height);
    float max = fmaxf(0, content - fmaxf(1, body_bottom(u) - cr_interface_content_top(u)));
    u->scroll = fminf(max, fmaxf(0, u->scroll));
}
static void view(cr_ui *u, int p) {
    if (u->ui_editing || u->ra_editing) {
        u->ui_editing = u->ra_editing = 0;
        memset(u->ra_preview, 0, sizeof(u->ra_preview));
        if (u->platform.edit_text) u->platform.edit_text(u->platform.context, 0, "", false);
    }
    u->controller_capture = -1;
    u->page = p;
    if (p == CR_MAIN) u->tab = 0;
    else if (p == CR_LIBRARY || p == CR_LIBRARY_ACTIONS || p == CR_GAME_ACTIONS || p == CR_CONTENT || p == CR_GAMES || p == CR_LISTS) u->tab = 1;
    else if (p == CR_SAVES_LIBRARY || p == CR_STATE_LIST || p == CR_STATE_ACTIONS || p == CR_STATE_DELETE_CONFIRM) u->tab = 2;
    else if (p == CR_CORES || p == CR_CORE_DOWNLOADS || p == CR_CORE_DETAILS || p == CR_REMOVE_CORES || p == CR_REMOVE_CORE_CONFIRM) u->tab = 3;
    else if (p == CR_SETTINGS || p == CR_SYSTEM_SETTINGS || p == CR_THEMES || p == CR_INFORMATION ||
             p == CR_ABOUT || p == CR_LICENSE || (p >= CR_UI_SETTINGS && p <= CR_OVERLAY_SETTINGS) ||
             (p >= CR_NOTIFICATIONS_SETTINGS && p <= CR_RA_ACCOUNT)) u->tab = 4;
    u->scroll = 0;
    u->quick_scroll = 0;
    u->text_height = 0;
    u->focus = 0;
    u->quick = false;
    cr_ui_clear_input(u);
}
void cr_menu_page(cr_ui *u, int p) {
    u->depth = 0;
    view(u, p);
}
void cr_menu_push_page(cr_ui *u, int p) {
    if (u->depth >= sizeof(u->history) / sizeof(u->history[0]))
        return;
    u->history[u->depth++] = (cr_navigation){
        .page = u->page, .focus = u->focus, .filter = u->filter, .tab = u->tab,
        .scroll = u->scroll, .quick_scroll = u->quick_scroll, .quick = u->quick
    };
    view(u, p);
}
bool cr_menu_pop_page(cr_ui *u) {
    if (!u->depth)
        return false;
    cr_navigation previous = u->history[--u->depth];
    view(u, previous.page);
    u->focus = previous.focus;
    u->filter = previous.filter;
    u->tab = previous.tab;
    u->scroll = previous.scroll;
    u->quick = previous.quick;
    u->quick_scroll = previous.quick_scroll;
    int count = cr_menu_entry_count(u);
    u->focus = count > 0 ? (u->focus < count ? u->focus : count - 1) : 0;
    if (u->quick) quick_clamp(u);
    else clamp_scroll(u);
    return true;
}

void cr_ui_message(cr_ui *u, const char *s) {
    snprintf(u->message, sizeof(u->message), "%s", s ? s : "");
    u->message_time = 5;
}
void cr_ui_catalog(cr_ui *u, const cr_core_catalog *catalog) {
    if (!catalog) return;
    u->core_catalog = *catalog;
    u->catalog_loading = false;
    int count = cr_menu_entry_count(u);
    if (count > 0 && u->focus >= count) u->focus = count - 1;
    clamp_scroll(u);
    if (catalog->error[0]) cr_ui_message(u, catalog->error);
    else if (!catalog->count) cr_ui_message(u, "Aucun cœur publié pour cet appareil");
}
void cr_ui_install_status(cr_ui *u, const char *message, bool finished) {
    if (finished) u->installing = false;
    cr_ui_message(u, message);
}
void cr_ui_reload(cr_ui *u) {
    size_t count = 0;
    cr_game *games = u->platform.games ? u->platform.games(u->platform.context, &count) : NULL;
    free(u->games);
    u->games = games;
    u->game_count = games ? count : 0;
    clamp_scroll(u);
}
static void achievement_request(void *context, uint64_t id, const char *url, const char *post) {
    cr_ui *u = context;
    if (u->platform.achievement_request)
        u->platform.achievement_request(u->platform.context, id, url, post);
}
static void achievement_credentials(void *context, const char *user, const char *token) {
    cr_ui *u = context;
    if (u->platform.save_achievement_credentials)
        u->platform.save_achievement_credentials(u->platform.context, user, token);
}
static void achievement_notice(void *context, const char *message) { cr_ui_message(context, message); }
static void achievement_notification(void *context, const cr_cheevos_notification *event) {
    cr_ui *u = context;
    char detail[256];
    switch (event->type) {
    case CR_CHEEVOS_GAME_READY:
        if (u->engine.loaded && event->image_url[0] && u->platform.game_image) {
            u->platform.game_image(u->platform.context, u->engine.id, event->image_url);
            int game = cr_menu_game_index(u, u->engine.id);
            if (game >= 0) snprintf(u->games[game].cover, sizeof(u->games[game].cover), "%.255s", event->image_url);
        }
        if (!u->notify_cards) break;
        snprintf(detail, sizeof(detail), "%u/%u succès - Mode normal", event->unlocked, event->total);
        cr_notifications_push(&u->notifications, CR_NOTIFICATION_GAME, 0,
                              event->title, detail, event->image_url);
        break;
    case CR_CHEEVOS_UNLOCKED:
        if (!u->notify_cards) break;
        if (u->audio && u->volume > 0 && u->platform.achievement_sound)
            u->platform.achievement_sound(u->platform.context, u->volume);
        snprintf(detail, sizeof(detail), "%s", event->title);
        char title[96];
        snprintf(title, sizeof(title), "Succès débloqué - %u points", event->points);
        cr_notifications_push(&u->notifications, CR_NOTIFICATION_UNLOCK, event->id,
                              title, detail, event->image_url);
        break;
    case CR_CHEEVOS_PROGRESS:
        if (!u->notify_progress) break;
        cr_notifications_progress(&u->notifications, event->id, event->detail,
                                  event->image_url, event->percent);
        break;
    case CR_CHEEVOS_PROGRESS_HIDE:
        cr_notifications_hide_progress(&u->notifications, event->id);
        break;
    case CR_CHEEVOS_COMPLETED:
        if (!u->notify_cards) break;
        cr_notifications_push(&u->notifications, CR_NOTIFICATION_UNLOCK, 0,
                              "Tous les succès débloqués", event->title, event->image_url);
        break;
    case CR_CHEEVOS_CLEAR:
        cr_notifications_clear(&u->notifications);
        break;
    }
}
static size_t achievement_memory(void *context, uint32_t address, uint8_t *bytes, size_t size) {
    cr_ui *u = context;
    if (!u->engine.loaded || strcmp(u->engine.id, u->ra_game_id) != 0 || u->engine.handle != u->ra_handle) return 0;
    return cr_engine_read_achievement_memory(&u->engine, address, bytes, size);
}
void cr_menu_achievement_sync(cr_ui *u, bool identify) {
    if (!u->engine.loaded && u->notifications_loaded) {
        cr_notifications_clear(&u->notifications);
        u->notifications_loaded = false;
    }
    if (!u->achievements) return;
    if (!u->ra_enabled || !u->engine.loaded || u->engine.players > 1) {
        if (u->ra_game_id[0]) cr_cheevos_unload_game(u->achievements);
        u->ra_game_id[0] = 0; u->ra_handle = NULL;
        return;
    }
    if (strcmp(u->engine.id, u->ra_game_id) == 0 && u->ra_handle == u->engine.handle) return;
    if (!identify) return;
    snprintf(u->ra_game_id, sizeof(u->ra_game_id), "%s", u->engine.id);
    u->ra_handle = u->engine.handle;
    char path[4352];
    snprintf(path, sizeof(path), "%s/roms/%s.rom", u->engine.root, u->engine.id);
    uint32_t console = u->engine.achievements.console_id;
    if (!console) {
        const uint32_t ids[] = {4, 6, 5};
        int system = cr_registry_legacy_system(u->engine.system);
        if (system >= 0 && system < 3) console = ids[system];
    }
    cr_cheevos_load_game(u->achievements, path, console,
                        u->engine.achievements.read_memory ? achievement_memory : NULL, u);
}
void cr_ui_achievement_frame(cr_ui *u) {
    cr_menu_achievement_sync(u, false);
    if (u->ra_enabled && u->engine.players <= 1) cr_cheevos_frame(u->achievements);
}
void cr_ui_text_result(cr_ui *u, int field, const char *text, bool accepted) {
    if (u->ui_editing == 3 && (field == 3 || field == -3)) {
        snprintf(u->library_query, sizeof(u->library_query), "%.127s", field == 3 && !accepted ? u->library_original_query : text);
        u->scroll = 0; u->focus = cr_menu_library_count(u) ? 1 : 0;
        if (field == 3) u->ui_editing = 0;
        return;
    }
    if (u->ui_editing == 4 && (field == 4 || field == -4)) {
        snprintf(u->state_rename, sizeof(u->state_rename), "%.95s", text);
        if (field == 4) {
            u->ui_editing = 0;
            if (accepted && u->selected_state >= 0 && (size_t)u->selected_state < u->saved_state_count) {
                char error[256] = {0};
                bool ok = cr_state_store_rename(u->engine.root, &u->saved_states[u->selected_state],
                                                u->state_rename, error, sizeof(error));
                cr_menu_pop_page(u);
                cr_menu_refresh_saved_states(u);
                cr_ui_message(u, ok ? "État renommé" : error);
            }
        }
        return;
    }
    if (field < 0 && -field == u->ra_editing) {
        snprintf(u->ra_preview, sizeof(u->ra_preview), "%s", text);
        return;
    }
    if (field != u->ra_editing || !field) return;
    if (accepted && field == 1) snprintf(u->ra_username, sizeof(u->ra_username), "%.95s", text);
    if (accepted && field == 2) snprintf(u->ra_password, sizeof(u->ra_password), "%s", text);
    memset(u->ra_preview, 0, sizeof(u->ra_preview));
    u->ra_editing = 0;
}
void cr_ui_init(cr_ui *u, cr_platform p, const char *root) {
    memset(u, 0, sizeof(*u));
    u->platform = p;
    u->installing = p.core_install_pending && p.core_install_pending(p.context);
    u->width = 384;
    u->height = 700;
    u->filter = -1;
    u->library_filter = -1;
    u->selected_state = -1;
    u->theme = CR_THEME_CORVUS;
    u->menu_pointer = u->feedback_target = -1;
    u->show_input = true;
    cr_engine_init(&u->engine, root);
    u->pixel_density = 1;
    u->controller_capture = -1;
    cr_menu_settings_load(u);
    for (unsigned i = 0; i < 10; ++i) {
        char key[48]; snprintf(key, sizeof(key), "controller_bind_%u", i);
        int value = cr_controller_binding_default(i);
        u->controller_bindings[i] = p.preference ? p.preference(p.context, key, value) : value;
    }
    cr_controller_bindings_sanitize(u->controller_bindings);
    if (p.preference) {
        u->theme = cr_theme_valid(p.preference(p.context, "theme", CR_THEME_CORVUS));
        u->audio = p.preference(p.context, "audio", 1) != 0;
        u->awake = p.preference(p.context, "keep_awake", 1) != 0;
        u->show_input = p.preference(p.context, "show_input", 1) != 0;
    } else {
        u->audio = true;
        u->awake = true;
    }
    if (p.achievement_request) {
        u->achievements = cr_cheevos_create((cr_cheevos_host){.context = u, .request = achievement_request,
                .save_credentials = achievement_credentials, .notice = achievement_notice,
                .notification = achievement_notification});
        u->ra_enabled = p.preference && p.preference(p.context, "achievements_enabled", 0) != 0;
        if (p.achievement_credentials) {
            char token[257] = {0};
            p.achievement_credentials(p.context, u->ra_username, sizeof(u->ra_username), token, sizeof(token));
            if (u->ra_enabled && token[0]) cr_cheevos_login_token(u->achievements, u->ra_username, token);
            memset(token, 0, sizeof(token));
        }
    }
    cr_ui_reload(u);
}
void cr_ui_destroy(cr_ui *u) {
    cr_menu_finish_save_transfer(u);
    cr_cheevos_destroy(u->achievements);
    u->achievements = NULL;
    memset(u->ra_password, 0, sizeof(u->ra_password));
    memset(u->ra_preview, 0, sizeof(u->ra_preview));
    cr_engine_destroy(&u->engine);
    free(u->games);
    free(u->license);
    free(u->saved_states);
    u->games = NULL;
    u->license = NULL;
    u->saved_states = NULL;
}
void cr_ui_size(cr_ui *u, float w, float h) {
    u->width = fmaxf(1, w);
    u->height = fmaxf(1, h);
    cr_ui_clear_input(u);
    clamp_scroll(u);
    if (u->quick)
        cr_ui_move(u, 0);
}
void cr_ui_clear_input(cr_ui *u) {
    memset(u->pointers, 0, sizeof(u->pointers));
    u->keys = 0;
    u->pulses = 0;
    memset(u->hardware_keys, 0, sizeof(u->hardware_keys));
    memset(u->hardware_pulses, 0, sizeof(u->hardware_pulses));
    u->feedback_pulses = 0;
    u->menu_pointer = u->feedback_target = -1;
    u->feedback_alpha = 0;
    u->scroll_velocity = 0;
    memset(u->control_feedback, 0, sizeof(u->control_feedback));
}
unsigned cr_ui_input(cr_ui *u) {
    unsigned masks[2];
    cr_ui_inputs(u, masks);
    return masks[0];
}
void cr_ui_inputs(cr_ui *u, unsigned masks[2]) {
    unsigned bits = u->keys | u->pulses;
    for (int i = 0; i < 32; i++)
        if (u->pointers[i].active)
            bits |= u->pointers[i].mask;
    u->pulses = 0;
    for (unsigned player = 0; player < 2; ++player) {
        masks[player] = u->hardware_keys[player] | u->hardware_pulses[player];
        u->hardware_pulses[player] = 0;
    }
    unsigned player = u->engine.players > 1 && u->input_player == 1 ? 1 : 0;
    masks[player] |= bits;
}
bool cr_ui_running(const cr_ui *u) {
    return u->page == CR_PLAY && !u->quick && u->engine.loaded && !u->installing && !u->save_request;
}
void cr_ui_update(cr_ui *u, double seconds) {
    cr_menu_achievement_sync(u, false);
    cr_cheevos_idle(u->achievements);
    float dt = (float)fmax(0, seconds);
    u->time += dt;
    cr_notifications_update(&u->notifications, dt);
    u->message_time = fmax(0, u->message_time - dt);
    if (u->scroll_velocity != 0 && u->menu_pointer < 0 && !cr_ui_running(u)) {
        float speed = u->scroll_velocity;
        float decay = expf(-8.f * dt);
        float distance = speed * (1.f - decay) / 8.f;
        float before = u->quick ? u->quick_scroll : u->scroll;
        if (u->quick) {
            u->quick_scroll += distance;
            quick_clamp(u);
        } else {
            u->scroll += distance;
            clamp_scroll(u);
        }
        float moved = (u->quick ? u->quick_scroll : u->scroll) - before;
        u->scroll_velocity = fabsf(moved - distance) > .5f || fabsf(speed * decay) < 25.f
                                ? 0 : speed * decay;
    }
    unsigned held = u->keys | u->feedback_pulses | u->hardware_keys[u->input_player == 1 ? 1 : 0];
    bool feedback = false;
    for (int i = 0; i < 32; i++) {
        cr_pointer *p = &u->pointers[i];
        if (!p->active)
            continue;
        p->held += dt;
        held |= p->mask;
        if (!cr_ui_running(u) && !p->dragged && p->target != -1 &&
            p->target == u->feedback_target && target(u, p->last_x, p->last_y) == p->target) {
            feedback = true;
            if (p->held >= .2f) {
                if (u->quick && p->target >= 10000 && p->target < 10000 + CR_QUICK_COUNT)
                    u->focus = p->target - 10000;
                else if (!u->quick && p->target >= 0 && p->target < cr_menu_entry_count(u))
                    u->focus = p->target;
            }
        }
    }
    u->feedback_alpha =
        feedback ? fminf(1, u->feedback_alpha + dt / .2f) : fmaxf(0, u->feedback_alpha - dt / .12f);
    for (int i = 0; i < 10; i++)
        u->control_feedback[i] = held & (1u << i) ? 1 : fmaxf(0, u->control_feedback[i] - dt / .1f);
    u->feedback_pulses = 0;
}


void cr_ui_back(cr_ui *u) {
    if (u->page == CR_LIBRARY && u->library_search) {
        u->ui_editing = 0;
        if (u->platform.edit_text) u->platform.edit_text(u->platform.context, 0, "", false);
        u->library_search = false;
        u->library_query[0] = 0;
        u->scroll = 0; u->focus = cr_menu_library_count(u) ? 1 : 0;
        return;
    }
    if (u->controller_capture >= 0) {
        u->controller_capture = -1;
        cr_ui_clear_input(u); cr_ui_message(u, "Affectation annulée"); return;
    }
    cr_ui_clear_input(u);
    if (u->save_request) {
        cr_menu_finish_save_transfer(u);
        cr_ui_message(u, "Transfert annulé");
    }
    if (u->page == CR_PLAY) {
        u->quick = !u->quick;
        u->focus = 0;
        u->quick_scroll = 0;
        return;
    }
    if (cr_menu_pop_page(u))
        return;
    if (u->page == CR_GAMES) {
        cr_menu_page(u, CR_LISTS);
        u->focus = u->filter < 0 ? 0 : u->filter + 1;
        return;
    }
    if (u->page == CR_THEMES || (u->page >= CR_UI_SETTINGS && u->page <= CR_OVERLAY_SETTINGS)) {
        cr_menu_page(u, CR_SETTINGS);
        return;
    }
    if (u->page != CR_MAIN) {
        cr_menu_page(u, CR_MAIN);
        return;
    }
    if (u->engine.loaded) {
        if (u->import_folder) {
            cr_ui_message(u, "Importation du dossier en cours");
            return;
        }
        cr_menu_page(u, CR_PLAY);
        return;
    }
    if (u->platform.quit)
        u->platform.quit(u->platform.context);
}
void cr_ui_confirm(cr_ui *u) {
    if (u->quick && u->focus >= 0 && u->focus < cr_menu_entry_count(u))
        cr_menu_quick_action(u, cr_menu_entry_get(u, u->focus).value);
    else if (u->page != CR_PLAY && u->focus >= 0 && u->focus < cr_menu_entry_count(u))
        cr_menu_activate(u, cr_menu_entry_get(u, u->focus));
}
bool cr_ui_controller_key(cr_ui *u, int key) {
    if (u->controller_capture < 0 || u->controller_capture >= 10) return false;
    unsigned button = (unsigned)u->controller_capture;
    if (!cr_controller_bindings_assign(u->controller_bindings, button, key)) {
        cr_ui_message(u, "Touche réservée ou déjà affectée"); return true;
    }
    char pref[48]; snprintf(pref, sizeof(pref), "controller_bind_%u", button);
    if (u->platform.set_preference)
        u->platform.set_preference(u->platform.context, pref, key);
    u->controller_capture = -1;
    cr_ui_clear_input(u); cr_ui_message(u, "Touche affectée");
    return true;
}
void cr_ui_move(cr_ui *u, int direction) {
    int count = cr_menu_entry_count(u);
    if (count <= 0)
        return;
    u->focus = (u->focus + direction + count) % count;
    cr_menu_rect bounds = cr_interface_row_bounds(u, u->focus);
    float top = cr_interface_content_top(u), bottom = body_bottom(u);
    float *scroll = u->quick ? &u->quick_scroll : &u->scroll;
    if (bounds.y < top) *scroll += bounds.y - top;
    else if (bounds.y + bounds.h > bottom) *scroll += bounds.y + bounds.h - bottom;
    if (u->quick) quick_clamp(u); else clamp_scroll(u);
}
void cr_ui_key(cr_ui *u, unsigned mask, bool down) {
    if (down) {
        press_controls(u, mask);
        u->keys |= mask;
        u->pulses |= mask;
    } else
        u->keys &= ~mask;
}
void cr_ui_player_key(cr_ui *u, unsigned player, unsigned mask, bool down) {
    if (player >= 2 || (mask & ~1023u)) return;
    if (down) {
        u->hardware_keys[player] |= mask;
        u->hardware_pulses[player] |= mask;
        if (player == u->input_player) press_controls(u, mask);
    } else u->hardware_keys[player] &= ~mask;
}
void cr_ui_release_player(cr_ui *u, unsigned player) {
    if (player < 2) u->hardware_keys[player] = u->hardware_pulses[player] = 0;
}
static cr_play_layout play_layout(const cr_ui *u) {
    cr_play_layout layout = cr_play_layout_configured(u->width, u->height,
                               cr_registry_legacy_system(u->engine.system), u->engine.width,
                               u->engine.height, u->overlay_size, u->overlay_position);
    if (u->video_integer)
        layout.video = cr_play_video_integer(layout.video, u->engine.width, u->engine.height, u->pixel_density);
    return layout;
}
static unsigned button_at(const cr_ui *u, float x, float y) {
    if (!u->show_input) return 0;
    cr_play_layout layout = play_layout(u);
    return cr_play_hit(&layout, x, y);
}
static void quick_clamp(cr_ui *u) {
    u->quick_scroll = fmaxf(0, fminf(u->quick_scroll,
        cr_interface_content_height(u, cr_menu_entry_count(u)) - fmaxf(1, body_bottom(u) - cr_interface_content_top(u))));
}
static int target(const cr_ui *u, float x, float y) {
    if (x < 0 || y < 0 || x >= u->width || y >= u->height)
        return -1;
    if (u->page == CR_PLAY && !u->quick) {
        cr_play_layout layout = play_layout(u);
        return cr_area_contains(layout.menu, x, y) ? -3 :
               cr_area_contains(layout.hide, x, y) ? -5 : -1;
    }
    return cr_interface_target(u, x, y, cr_menu_entry_count(u), has_back(u));
}
void cr_ui_touch(cr_ui *u, int id, int phase, float x, float y) {
    cr_ui_touch_at(u, id, phase, x, y, u->time);
}
void cr_ui_touch_at(cr_ui *u, int id, int phase, float x, float y, double event_time) {
    if (id < 0 || id >= 32)
        return;
    if (phase == CR_TOUCH_CANCEL) {
        cr_ui_clear_input(u);
        return;
    }
    cr_pointer *p = &u->pointers[id];
    if (phase == CR_TOUCH_DOWN) {
        if (!cr_ui_running(u) && u->menu_pointer >= 0)
            return;
        u->scroll_velocity = 0;
        *p = (cr_pointer){.active = true,
                          .x = x,
                          .y = y,
                          .last_x = x,
                          .last_y = y,
                          .target = target(u, x, y),
                          .started = event_time,
                          .last_event = event_time};
        if (cr_ui_running(u)) {
            p->mask = button_at(u, x, y);
            u->pulses |= p->mask;
            press_controls(u, p->mask);
        } else {
            u->menu_pointer = id;
            u->feedback_target = p->target;
            u->feedback_alpha = 0;
        }
        return;
    }
    if (!p->active)
        return;
    if (phase == CR_TOUCH_MOVE) {
        if (fabsf(x - p->x) > 8 || fabsf(y - p->y) > 8)
            p->dragged = true;
        if (cr_ui_running(u)) {
            p->last_x = x;
            p->last_y = y;
            if (p->target == -3 || p->target == -5)
                return;
            unsigned mask = button_at(u, x, y);
            u->pulses |= mask & ~p->mask;
            press_controls(u, mask);
            p->mask = mask;
            return;
        }
        bool scrolling = p->dragged && ((u->quick && p->target >= 10000) ||
                         (!u->quick && p->target >= -1 && u->page != CR_PLAY));
        if (scrolling) {
            float shift = p->last_y - y;
            double elapsed = event_time - p->last_event;
            p->velocity = elapsed > 0 && elapsed <= .15
                            ? fmaxf(-4000, fminf(4000, shift / (float)elapsed)) : 0;
            if (u->quick) {
                u->quick_scroll += shift;
                quick_clamp(u);
            } else {
                u->scroll += shift;
                clamp_scroll(u);
            }
        }
        p->last_x = x;
        p->last_y = y;
        p->last_event = event_time;
        return;
    }
    if (phase == CR_TOUCH_UP) {
        int t = p->target;
        bool click = !p->dragged && target(u, x, y) == t;
        if (p->dragged && event_time >= p->last_event &&
            event_time - p->last_event <= .12 &&
            ((u->quick && t >= 10000) || (!u->quick && t >= -1 && u->page != CR_PLAY)))
            u->scroll_velocity = p->velocity;
        p->active = false;
        p->mask = 0;
        if (u->menu_pointer == id)
            u->menu_pointer = -1;
        if (!click)
            return;
        if (event_time - p->started >= .2 && (t >= 0)) {
            if (u->quick && t >= 10000 && t < 10000 + CR_QUICK_COUNT)
                u->focus = t - 10000;
            else if (!u->quick && t < cr_menu_entry_count(u)) {
                u->focus = t;
                if (event_time - p->started >= 1) {
                    row r = cr_menu_entry_get(u, t);
                    if (!r.checked && (r.action == ACT_AUDIO || r.action == ACT_AWAKE ||
                                       r.action == ACT_SHOW_INPUT))
                        cr_menu_activate(u, r);
                }
            }
            return;
        }
        if (t >= 10000) {
            cr_menu_quick_action(u, cr_menu_entry_get(u, t - 10000).value);
            return;
        }
        if (t == -5) {
            row r = {0};
            r.action = ACT_SHOW_INPUT;
            cr_menu_activate(u, r);
            return;
        }
        if (t == -3) {
            u->quick = true;
            u->focus = 0;
            u->quick_scroll = 0;
            cr_ui_clear_input(u);
            return;
        }
        if (t == -2) {
            cr_ui_back(u);
            return;
        }
        if (t <= -5000) {
            int index = -t - 5000;
            if (index >= cr_menu_entry_count(u)) return;
            row item = cr_menu_entry_get(u, index);
            if (u->page == CR_CORES && item.action == ACT_SELECT) cr_menu_open_core_details(u, item.value);
            else if (u->page == CR_LIBRARY && item.action == ACT_GAME && item.value >= 0 && (size_t)item.value < u->game_count) {
                snprintf(u->states_game_id, sizeof(u->states_game_id), "%s", u->games[item.value].id);
                cr_menu_push_page(u, CR_GAME_ACTIONS);
            } else if (item.action == ACT_STATE_ACTIONS || item.action == ACT_STATE_GAME) cr_menu_activate(u, item);
            return;
        }
        if (t == -200 && u->page == CR_LIBRARY) {
            u->library_search = true;
            if (u->ui_editing != 3) snprintf(u->library_original_query, sizeof(u->library_original_query), "%s", u->library_query);
            u->ui_editing = 3;
            if (u->platform.edit_text) u->platform.edit_text(u->platform.context, 3, u->library_query, false);
            return;
        }
        if (t == -201 && u->page == CR_LIBRARY) { cr_menu_push_page(u, CR_LIBRARY_ACTIONS); return; }
        if (t == -202 && u->page == CR_LIBRARY) {
            u->ui_editing = 0;
            if (u->platform.edit_text) u->platform.edit_text(u->platform.context, 0, "", false);
            u->library_query[0] = 0; u->library_search = false; u->scroll = 0;
            return;
        }
        if (t <= -210 && t >= -213 && u->page == CR_LIBRARY) {
            u->library_filter = -t - 211; u->scroll = 0; u->focus = cr_menu_library_count(u) ? 1 : 0;
            return;
        }
        if (t == -300) { cr_menu_push_page(u, CR_SETTINGS); return; }
        if (t <= -100 && t >= -104) {
            static const int pages[] = {CR_MAIN, CR_LIBRARY, CR_SAVES_LIBRARY, CR_CORES, CR_SETTINGS};
            cr_ui_reload(u);
            cr_menu_page(u, pages[-t - 100]);
            return;
        }
        if (t >= 0) {
            u->focus = t;
            cr_menu_activate(u, cr_menu_entry_get(u, t));
        }
    }
}

static void rect(const cr_painter *p, float x, float y, float w, float h, uint32_t c) {
    if (w > 0 && h > 0)
        p->rect(p->context, x, y, w, h, c);
}
static void text(const cr_painter *p, const char *s, float x, float y, float size, uint32_t c,
                 float max) {
    p->text(p->context, s, x, y, size, c, max);
}
static float width(const cr_painter *p, const char *s, float size) {
    return p->measure(p->context, s, size);
}
static float paragraphs(const cr_painter *p, const char *s, float x, float top, float w, float line,
                        uint32_t color, float scroll, float bottom) {
    char buffer[1024];
    size_t at = 0;
    float y = top - scroll;
    const char *word = s;
    while (*word) {
        const char *end = word;
        while (*end && *end != ' ' && *end != '\n')
            end++;
        size_t len = (size_t)(end - word);
        char next[1024];
        size_t part = len < sizeof(next) - 1 ? len : sizeof(next) - 1;
        memcpy(next, word, part);
        next[part] = 0;
        if (at && width(p, buffer, 14) + width(p, next, 14) > w) {
            if (y >= 56 && y <= bottom + line)
                text(p, buffer, x, y, 14, color, w);
            y += line;
            at = 0;
            buffer[0] = 0;
        }
        if (len >= sizeof(buffer) - at - 2)
            len = sizeof(buffer) - at - 2;
        memcpy(buffer + at, word, len);
        at += len;
        buffer[at] = 0;
        if (*end == '\n') {
            if (y >= 56 && y <= bottom + line)
                text(p, buffer, x, y, 14, color, w);
            y += line;
            at = 0;
            buffer[0] = 0;
        } else if (*end == ' ') {
            buffer[at++] = ' ';
            buffer[at] = 0;
        }
        word = *end ? end + 1 : end;
    }
    if (at && y >= 56 && y <= bottom + line)
        text(p, buffer, x, y, 14, color, w);
    return y + scroll - top + line;
}

void cr_ui_draw(cr_ui *u, const cr_painter *p) {
    const cr_theme *theme = cr_theme_get(u->theme);
    uint32_t bg = theme->background, fg = theme->text;
    float w = u->width, h = u->height;
    rect(p, 0, 0, w, h, bg);
    if (u->page == CR_PLAY && !u->quick) {
        rect(p, 0, 0, w, h, 0xff000000);
        cr_play_layout layout = play_layout(u);
        if (u->engine.loaded)
            p->game(p->context, u->engine.pixels, u->engine.width, u->engine.height, layout.video.x,
                    layout.video.y, layout.video.w, layout.video.h);
        unsigned input = u->keys | u->hardware_keys[u->input_player == 1 ? 1 : 0];
        bool menu_down = false, hide_down = false;
        for (int i = 0; i < 32; i++)
            if (u->pointers[i].active) {
                input |= u->pointers[i].mask;
                menu_down |= u->pointers[i].target == -3 && !u->pointers[i].dragged;
                hide_down |= u->pointers[i].target == -5 && !u->pointers[i].dragged;
            }
        cr_overlay_draw_configured(&layout, p, input, u->control_feedback, u->show_input, menu_down, hide_down, u->overlay_opacity);
    } else {
        const char *title = u->quick ? "Menu rapide" : "Menu principal";
        switch (u->page) {
        case CR_LISTS:
            title = "Listes de lecture";
            break;
        case CR_SETTINGS:
            title = "Réglages";
            break;
        case CR_SAVES_LIBRARY: title = "Saves"; break;
        case CR_STATE_LIST: title = "Saves"; break;
        case CR_STATE_ACTIONS: title = "État de sauvegarde"; break;
        case CR_STATE_DELETE_CONFIRM: title = "Supprimer l'état ?"; break;
        case CR_CORE_DETAILS: title = "Installer un cœur"; break;
        case CR_LIBRARY_ACTIONS: title = "Bibliothèque"; break;
        case CR_GAME_ACTIONS: title = "Options du jeu"; break;
        case CR_SYSTEM_SETTINGS: title = "Système"; break;
        case CR_REMOVE_CORES:
            title = "Cœurs téléchargés";
            break;
        case CR_REMOVE_CORE_CONFIRM:
            title = "Supprimer le cœur ?";
            break;
        case CR_CORES:
            title = "Cœurs";
            break;
        case CR_GAME_SAVES:
            title = "Sauvegardes du jeu";
            break;
        case CR_GAME_BACKUPS:
            title = "Copies de secours";
            break;
        case CR_SAVE_CONFIRM:
            title = "Confirmer l’importation";
            break;
        case CR_SAVESTATES:
            title = "Sauvegardes d’état";
            break;
        case CR_SUBSYSTEMS:
            title = "Sous-systèmes";
            break;
        case CR_CORE_OPTIONS:
            title = "Options du cœur";
            break;
        case CR_CORE_OPTION_VALUES: {
            corvus_core_option option;
            title = cr_menu_selected_option(u, &option) ? option.label : "Options du cœur";
            break;
        }
        case CR_SUBSYSTEM_CONFIG: {
            corvus_subsystem_info subsystem;
            title = cr_menu_selected_subsystem(u, &subsystem) ? subsystem.label : "Sous-systèmes";
            break;
        }
        case CR_SUBSYSTEM_CONTENTS:
            title = u->link_slot ? "Contenu du joueur 2" : "Contenu du joueur 1";
            break;
        case CR_ACHIEVEMENTS:
            title = "RetroAchievements";
            break;
        case CR_CORE_DOWNLOADS:
            title = "Téléchargeur de cœurs";
            break;
        case CR_GAMES:
            title = u->filter < 0 ? "Historique" : cr_menu_system_name(u->filter);
            break;
        case CR_CONTENT:
            title = "Charger du contenu";
            break;
        case CR_LIBRARY:
            title = "Bibliothèque";
            break;
        case CR_ABOUT:
            title = "CorvusRetro";
            break;
        case CR_LICENSE:
            title = "Licence et crédits";
            break;
        case CR_THEMES:
            title = "Thème de l’interface";
            break;
        case CR_UI_SETTINGS:
            title = "Interface utilisateur";
            break;
        case CR_VIDEO_SETTINGS:
            title = "Affichage";
            break;
        case CR_AUDIO_SETTINGS:
            title = "Audio";
            break;
        case CR_OSD_SETTINGS:
            title = "Commandes";
            break;
        case CR_OVERLAY_SETTINGS:
            title = "Commandes tactiles";
            break;
        case CR_NOTIFICATIONS_SETTINGS:
            title = "Notifications";
            break;
        case CR_SETTING_VALUES:
            title = cr_menu_settings[u->editing_setting].label;
            break;
        case CR_OVERLAY_LAYOUT:
            title = "Taille et position";
            break;
        case CR_CONTROLLER_SETTINGS:
            title = "Manettes";
            break;
        case CR_CONTROLLER_BINDINGS:
            title = "Affectation des touches";
            break;
        case CR_SAVE_SETTINGS:
            title = "Sauvegardes";
            break;
        case CR_RA_SETTINGS:
            title = "RetroAchievements";
            break;
        case CR_RA_ACCOUNT:
            title = "Compte RetroAchievements";
            break;
        case CR_INFORMATION:
            title = "Informations";
            break;
        case CR_CORE_INFO:
            title = "Informations sur le cœur";
            break;
        default:
            break;
        }
        if (u->quick && u->engine.loaded) {
            cr_play_layout layout = play_layout(u);
            rect(p, 0, 0, w, h, 0xff000000);
            if (p->game) p->game(p->context, u->engine.pixels, u->engine.width, u->engine.height,
                layout.video.x, layout.video.y, layout.video.w, layout.video.h);
        }
        int count = cr_menu_entry_count(u);
        cr_interface_draw(u, p, title, has_back(u), count, cr_menu_entry_visual);
        if (p->clip) p->clip(p->context, 0, 56, w, fmaxf(1, body_bottom(u) - 56));
        if (u->page == CR_ABOUT)
            u->text_height = paragraphs(
                p,
                "CorvusRetro 0.23.0\n\nFrontend original en C, rendu OpenGL ES et audio "
                "AAudio.\n\nCorvusSoftware\nCœurs officiels CorvusCore 0.15.0 : Game Boy, Game "
                "Boy Color, Game Boy Advance.\n\nInterface CorvusRetro. "
                "Thèmes CorvusRetro, ASUS et NVIDIA Shield.\n\nCopyright (C) 2026 "
                "CorvusSoftware\nSPDX-License-Identifier: AGPL-3.0-only",
                20, 92, w - 40, 24, fg, u->scroll, body_bottom(u));
        if ((u->page == CR_SUBSYSTEMS || u->page == CR_CORE_OPTIONS) && !count) {
            const char *empty = !u->engine.handle ? "Aucun cœur chargé." :
                u->page == CR_SUBSYSTEMS ?
                    "Aucun sous-système disponible avec ce cœur.\n\n"
                    "Ce module ne déclare pas de configuration de plusieurs contenus." :
                    "Aucune option de cœur disponible.\n\n"
                    "Ce module ne déclare pas de réglage modifiable.";
            u->text_height = paragraphs(p, empty, 20, 92, w - 40, 24, fg, u->scroll, body_bottom(u));
        }
        if (u->page == CR_CORE_INFO) {
            char info[2048];
            if (u->engine.handle) {
                corvus_core_info desc = {.struct_size = sizeof(desc)};
                bool known = cr_engine_core_info(&u->engine, &desc);
                snprintf(info, sizeof(info),
                         "%s\n\nSystème : %s\nVersion : %s\nCorvusAPI : %u\nExtensions : %s\n"
                         "États de sauvegarde : %s\nBatterie : %s\nJoueurs actifs : %u\nContenu : %s",
                         known ? desc.name : "?", known ? desc.system_name : "?",
                         known ? desc.version : "?", CORVUS_API_VERSION,
                         known ? desc.extensions : "?",
                         (u->engine.loaded ? cr_engine_can_state(&u->engine) : u->engine.api.serialize_size &&
                          u->engine.api.serialize && u->engine.api.unserialize) ? "Oui" : "Non",
                         (u->engine.players > 1 ? u->engine.subsystems.battery_size != NULL :
                          u->engine.api.battery_size != NULL) ? "Oui" : "Non",
                         u->engine.players,
                         u->engine.loaded ? u->title : "Aucun contenu");
            }
            else
                snprintf(info, sizeof(info), "Aucun cœur chargé");
            u->text_height = paragraphs(p, info, 20, 92, w - 40, 24, fg, u->scroll, body_bottom(u));
        }
        if (u->page == CR_LICENSE)
            u->text_height = paragraphs(p, u->license ? u->license : "Licence indisponible", 20, 92,
                                        w - 40, 23, fg, u->scroll, body_bottom(u));
        if (p->clip)
            p->clip(p->context, 0, 0, w, h);
    }
    if (u->message_time > 0 && u->message[0]) {
        float y = u->page == CR_PLAY ? h - 108 : h - 142;
        rect(p, 12, y, w - 24, 52, 0xf0333333);
        text(p, u->message, 24, y + 32, 13, 0xffffffff, w - 48);
    }
    float popup_left = 0, popup_right = w, popup_top = 56, popup_bottom = h - 72;
    if (u->page == CR_PLAY && !u->quick) {
        cr_play_layout layout = play_layout(u);
        popup_top = 0;
        popup_bottom = layout.portrait ? layout.panel.y : h;
        if (!layout.portrait) {
            /* The centered video sits between both control gutters, including menu/hide. */
            popup_left = layout.video.x;
            popup_right = layout.video.x + layout.video.w;
        }
    }
    if (u->message_time > 0)
        popup_bottom = fminf(popup_bottom, h - (u->page == CR_PLAY ? 112 : 146));
    cr_notifications_prepare(&u->notifications, p);
    cr_notifications_draw_region(&u->notifications, p, popup_left, popup_top, popup_right, popup_bottom, theme);
    if (u->notifications.count && p->clip) p->clip(p->context, 0, 0, w, h);
}
