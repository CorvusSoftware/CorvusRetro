// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
#define _POSIX_C_SOURCE 200809L
#include "menu_cbs.h"
#include "menu_internal.h"
#include "menu_setting.h"
#include "core_registry.h"
#include "state_store.h"
#include <errno.h>
#include <fcntl.h>
#include <stdatomic.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef cr_menu_row row;
static atomic_uint_fast64_t save_nonce = ATOMIC_VAR_INIT(0);
static void clear_link_selection(cr_ui *u) {
    u->subsystem_id[0] = 0;
    memset(u->link_ids, 0, sizeof(u->link_ids));
    memset(u->link_titles, 0, sizeof(u->link_titles));
    u->link_slot = 0;
}
void cr_menu_refresh_saved_states(cr_ui *u) {
    u->saved_state_count = 0;
    u->selected_state = -1;
    if (!cr_game_id_valid(u->states_game_id)) return;
    if (!u->saved_states) u->saved_states = calloc(CR_STATE_STORE_LIMIT, sizeof(*u->saved_states));
    if (!u->saved_states) { cr_ui_message(u, "Mémoire insuffisante"); return; }
    char error[256] = {0};
    u->saved_state_count = cr_state_store_list(u->engine.root, u->states_game_id,
        u->saved_states, CR_STATE_STORE_LIMIT, error, sizeof(error));
    if (error[0]) cr_ui_message(u, error);
}
static void open_state_list(cr_ui *u, const char *game_id) {
    if (!cr_game_id_valid(game_id)) return;
    memmove(u->states_game_id, game_id, strlen(game_id) + 1);
    cr_menu_refresh_saved_states(u);
    cr_menu_push_page(u, CR_STATE_LIST);
}
void cr_menu_open_core_details(cr_ui *u, int index) {
    u->detail_core_id[0] = u->detail_pack_id[0] = 0;
    if (index >= 0) {
        const cr_core_descriptor *core = cr_registry_get(index);
        if (!core) return;
        snprintf(u->detail_core_id, sizeof(u->detail_core_id), "%s", core->id);
    } else {
        size_t pack_index = (size_t)(-index - 1);
        if (pack_index >= u->core_catalog.count) return;
        const cr_core_pack *pack = &u->core_catalog.packs[pack_index];
        snprintf(u->detail_core_id, sizeof(u->detail_core_id), "%s", pack->core_id);
        snprintf(u->detail_pack_id, sizeof(u->detail_pack_id), "%s", pack->id);
    }
    cr_menu_push_page(u, CR_CORE_DETAILS);
    if (!u->core_catalog.count && !u->catalog_loading && u->platform.refresh_core_catalog) {
        u->catalog_loading = true;
        u->platform.refresh_core_catalog(u->platform.context);
    }
}
static bool result(cr_ui *u, bool ok) {
    if (!ok)
        cr_ui_message(u, u->engine.error);
    return ok;
}
static bool state_busy(cr_ui *u) {
    if (!u->importing && !u->save_request && !u->installing) return false;
    cr_ui_message(u, "Terminez l'opération en cours avant de gérer les états");
    return true;
}
static void create_named_state(cr_ui *u) {
    if (state_busy(u) || !u->engine.loaded || !cr_engine_can_state(&u->engine)) return;
    snprintf(u->states_game_id, sizeof(u->states_game_id), "%s", u->engine.id);
    cr_menu_refresh_saved_states(u);
    bool used[CR_STATE_STORE_LIMIT + 1] = {false};
    for (size_t i = 0; i < u->saved_state_count; ++i)
        if (u->saved_states[i].slot <= CR_STATE_STORE_LIMIT) used[u->saved_states[i].slot] = true;
    unsigned number = 1;
    while (number <= CR_STATE_STORE_LIMIT && used[number]) ++number;
    char name[96]; snprintf(name, sizeof(name), "État %u", number);
    cr_state_entry created;
    if (!result(u, cr_state_store_create(&u->engine, name, &created))) {
        cr_menu_refresh_saved_states(u);
        cr_ui_message(u, u->engine.error);
        return;
    }
    if (u->page != CR_STATE_LIST) cr_menu_push_page(u, CR_STATE_LIST);
    cr_menu_refresh_saved_states(u);
    for (size_t i = 0; i < u->saved_state_count; ++i)
        if (u->saved_states[i].slot == created.slot) u->focus = (int)i + 1;
    cr_ui_move(u, 0);
    cr_ui_message(u, "État sauvegardé");
}
static void load_named_state(cr_ui *u, const cr_state_entry *entry) {
    if (state_busy(u)) return;
    int core = cr_registry_find(entry->core_id);
    if (!u->engine.handle || u->engine.system != core) {
        cr_ui_message(u, "Chargez le cœur associé à cet état"); return;
    }
    int game = cr_menu_game_index(u, entry->game_id), second = cr_menu_game_index(u, entry->second_id);
    if (game < 0 || (entry->players == 2 && second < 0)) {
        cr_ui_message(u, "Le contenu associé à cet état est introuvable"); return;
    }
    char title[512];
    if (entry->players == 2) snprintf(title, sizeof(title), "%.230s + %.230s", u->games[game].title, u->games[second].title);
    else snprintf(title, sizeof(title), "%s", u->games[game].title);
    bool same = u->engine.loaded && !strcmp(u->engine.id, entry->game_id) &&
        u->engine.players == entry->players && !strcmp(u->engine.second_id, entry->second_id) &&
        !strcmp(u->engine.subsystem_id, entry->subsystem_id);
    corvus_core_info current = {.struct_size = sizeof(current)};
    const cr_core_descriptor *next = cr_registry_get(core);
    if (!cr_engine_core_info(&u->engine, &current) ||
        strcmp(entry->core_version, same ? current.version : next ? next->version : "")) {
        cr_ui_message(u, "Cet état exige la version du cœur avec laquelle il a été créé"); return;
    }
    uint8_t *verified = NULL; size_t verified_size = 0; char error[256] = {0};
    bool valid = cr_state_store_read(u->engine.root, entry, &verified, &verified_size, error, sizeof(error));
    free(verified);
    if (!valid) { cr_ui_message(u, error); return; }
    if (!same) {
        char paths[2][4256];
        snprintf(paths[0], sizeof(paths[0]), "%s/roms/%s.rom", u->engine.root, entry->game_id);
        snprintf(paths[1], sizeof(paths[1]), "%s/roms/%s.rom", u->engine.root, entry->second_id);
        bool opened;
        if (entry->players == 2) {
            const char *ids[] = {entry->game_id, entry->second_id}, *files[] = {paths[0], paths[1]};
            opened = cr_engine_open_subsystem(&u->engine, core, entry->subsystem_id, ids, files);
        } else opened = cr_engine_open(&u->engine, core, entry->game_id, paths[0]);
        if (!result(u, opened)) return;
        ++u->core_revision;
        u->input_player = 0;
        cr_notifications_clear(&u->notifications);
        cr_menu_achievement_sync(u, false);
        snprintf(u->title, sizeof(u->title), "%s", title);
    }
    if (!result(u, cr_state_store_load(&u->engine, entry))) return;
    cr_cheevos_reset(u->achievements);
    cr_menu_achievement_sync(u, true);
    u->notifications_loaded = true;
    cr_menu_page(u, CR_PLAY);
    if (u->platform.touch) u->platform.touch(u->platform.context, entry->game_id);
    cr_ui_reload(u);
    cr_ui_message(u, "État chargé");
}
static void open_game(cr_ui *u, int system, const char *id, const char *title, const char *path) {
    if (!u->engine.handle) {
        cr_ui_message(u, "Chargez un cœur avant de lancer un jeu");
        return;
    }
    if (u->engine.system != system) {
        cr_ui_message(u, "Chargez le cœur associé à ce contenu");
        return;
    }
    if (!result(u, cr_engine_open(&u->engine, system, id, path)))
        return;
    ++u->core_revision;
    u->input_player = 0;
    cr_notifications_clear(&u->notifications);
    cr_menu_achievement_sync(u, true);
    u->notifications_loaded = true;
    snprintf(u->title, sizeof(u->title), "%s", title);
    cr_menu_page(u, CR_PLAY);
    if (u->platform.touch)
        u->platform.touch(u->platform.context, id);
    cr_ui_reload(u);
}
static void start_subsystem(cr_ui *u) {
    corvus_subsystem_info subsystem;
    if (!cr_menu_selected_subsystem(u, &subsystem) || !u->link_ids[0][0] || !u->link_ids[1][0]) {
        cr_ui_message(u, "Choisissez les deux contenus avant de démarrer");
        return;
    }
    char files[2][4256];
    const char *ids[2] = {u->link_ids[0], u->link_ids[1]};
    const char *paths[2] = {files[0], files[1]};
    for (unsigned i = 0; i < 2; ++i)
        snprintf(files[i], sizeof(files[i]), "%s/roms/%s.rom", u->engine.root, ids[i]);
    if (!result(u, cr_engine_open_subsystem(&u->engine, u->engine.system,
                                          subsystem.id, ids, paths))) return;
    ++u->core_revision;
    u->input_player = 0;
    cr_notifications_clear(&u->notifications);
    cr_menu_achievement_sync(u, false);
    u->notifications_loaded = true;
    snprintf(u->title, sizeof(u->title), "%.230s + %.230s", u->link_titles[0], u->link_titles[1]);
    cr_menu_page(u, CR_PLAY);
    if (u->platform.touch)
        for (unsigned i = 0; i < 2; ++i) u->platform.touch(u->platform.context, ids[i]);
    cr_ui_reload(u);
}
static void show_core_downloader(cr_ui *u) {
    cr_menu_push_page(u, CR_CORE_DOWNLOADS);
    u->catalog_loading = true;
    u->core_catalog.count = 0;
    u->core_catalog.error[0] = '\0';
    if (u->platform.refresh_core_catalog)
        u->platform.refresh_core_catalog(u->platform.context);
    else {
        u->catalog_loading = false;
        cr_ui_message(u, "Catalogue Android indisponible");
    }
}
static bool save_current(const cr_ui *u) {
    return u->save_request && u->engine.loaded && u->engine.handle == u->save_handle &&
        u->core_revision == u->save_revision && u->save_player < u->engine.players &&
        !strcmp(u->engine.id, u->save_id) && !strcmp(u->engine.second_id, u->save_second_id);
}
void cr_menu_finish_save_transfer(cr_ui *u) {
    uint64_t request = u->save_request;
    u->save_request = 0; u->save_path[0] = 0; u->save_handle = NULL;
    if (request && u->platform.finish_save_transfer)
        u->platform.finish_save_transfer(u->platform.context, request);
}
static void begin_save_transfer(cr_ui *u, int operation, int backup) {
    if (u->save_request || u->importing || u->installing || !u->platform.save_transfer ||
        !cr_engine_battery_size(&u->engine, u->save_player)) return;
    uint8_t *bytes = NULL; size_t size = 0;
    if (operation != CR_SAVE_IMPORT &&
        !result(u, backup >= 0 ? cr_engine_export_backup(&u->engine, u->save_player, (unsigned)backup, &bytes, &size) :
                cr_engine_export_battery(&u->engine, u->save_player, &bytes, &size))) return;
    u->save_request = atomic_fetch_add(&save_nonce, 1) + 1;
    u->save_revision = u->core_revision; u->save_handle = u->engine.handle;
    u->save_operation = operation;
    snprintf(u->save_id, sizeof(u->save_id), "%s", u->engine.id);
    snprintf(u->save_second_id, sizeof(u->save_second_id), "%s", u->engine.second_id);
    char title[600];
    snprintf(title, sizeof(title), "%s%s", u->title[0] ? u->title : u->engine.id,
             u->save_player ? " - Joueur 2" : "");
    if (backup >= 0) {
        size_t used = strlen(title);
        snprintf(title + used, sizeof(title) - used, " - copie %d", backup + 1);
    }
    bool started = u->platform.save_transfer(u->platform.context, u->save_request,
                                            operation, title, bytes, size);
    free(bytes);
    if (!started) {
        cr_menu_finish_save_transfer(u); cr_ui_message(u, "Service de sauvegarde indisponible");
    }
    cr_ui_clear_input(u);
}
void cr_ui_save_transfer_result(cr_ui *u, const cr_save_transfer_result *item) {
    if (!item || !item->request) return;
    if (item->request != u->save_request) {
        if (u->platform.finish_save_transfer)
            u->platform.finish_save_transfer(u->platform.context, item->request);
        return;
    }
    if (u->save_path[0]) return;
    if (!save_current(u) || item->cancelled || item->error[0]) {
        if (item->error[0]) cr_ui_message(u, item->error);
        else cr_ui_message(u, item->cancelled ? "Transfert annulé" : "Transfert expiré : contenu modifié");
        cr_menu_finish_save_transfer(u); return;
    }
    if (u->save_operation != CR_SAVE_IMPORT) {
        cr_menu_finish_save_transfer(u); cr_ui_message(u, "Sauvegarde exportée"); return;
    }
    char expected_path[4096];
    int n = snprintf(expected_path, sizeof(expected_path), "%s/save-transfer/%llu.sav",
                     u->engine.root, (unsigned long long)u->save_request);
    if (n <= 0 || n >= (int)sizeof(expected_path) || strcmp(item->path, expected_path)) {
        cr_menu_finish_save_transfer(u); cr_ui_message(u, "Fichier de sauvegarde invalide"); return;
    }
    snprintf(u->save_path, sizeof(u->save_path), "%s", item->path);
    cr_menu_push_page(u, CR_SAVE_CONFIRM);
}
static bool import_save_file(cr_ui *u) {
    if (!save_current(u)) { cr_ui_message(u, "Transfert expiré : contenu modifié"); return false; }
    char directory_path[4096];
    int n = snprintf(directory_path, sizeof(directory_path), "%s/save-transfer", u->engine.root);
    int directory_fd = n > 0 && n < (int)sizeof(directory_path)
        ? open(directory_path, O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_DIRECTORY) : -1;
    char name[64]; snprintf(name, sizeof(name), "%llu.sav", (unsigned long long)u->save_request);
    int fd = directory_fd >= 0 ? openat(directory_fd, name, O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK) : -1;
    if (directory_fd >= 0) close(directory_fd);
    struct stat status;
    size_t expected = cr_engine_battery_size(&u->engine, u->save_player);
    if (fd < 0 || fstat(fd, &status) || !S_ISREG(status.st_mode) || !expected ||
        status.st_size != (off_t)expected || expected > 1024u * 1024u) {
        if (fd >= 0) close(fd);
        cr_ui_message(u, "Taille ou fichier incompatible avec la sauvegarde du jeu"); return false;
    }
    uint8_t *bytes = malloc(expected);
    bool ok = bytes != NULL;
    size_t offset = 0;
    while (ok && offset < expected) {
        ssize_t count = read(fd, bytes + offset, expected - offset);
        if (count < 0 && errno == EINTR) continue;
        if (count <= 0) { ok = false; break; }
        offset += (size_t)count;
    }
    uint8_t extra;
    if (ok && read(fd, &extra, 1) != 0) ok = false;
    if (close(fd) != 0) ok = false;
    if (ok) ok = result(u, cr_engine_import_battery(&u->engine, u->save_player, bytes, expected));
    else cr_ui_message(u, "Lecture de la sauvegarde impossible");
    free(bytes);
    return ok;
}
void cr_menu_activate(cr_ui *u, row r) {
    if (u->controller_capture >= 0) return;
    if ((r.action == ACT_CORE_REMOVE || r.action == ACT_CORE_REMOVE_CONFIRM) &&
        (u->save_request || u->importing || u->installing)) {
        cr_ui_message(u,"Terminez l’opération en cours avant de supprimer un cœur"); return;
    }
    if (u->save_request && (r.action == ACT_SELECT || r.action == ACT_UNLOAD ||
        r.action == ACT_GAME || r.action == ACT_PICK || r.action == ACT_FOLDER ||
        r.action == ACT_INSTALL_CORE_PACK || r.action == ACT_OPTION_VALUE ||
        r.action == ACT_SUBSYSTEM_START || r.action == ACT_SUBSYSTEM_IMPORT ||
        r.action == ACT_SAVE_STATE || r.action == ACT_LOAD_STATE || r.action == ACT_LOAD_AUTO_STATE || r.action == ACT_SAVE_PLAYER)) {
        cr_ui_message(u, "Transfert de sauvegarde en cours"); return;
    }
    if (r.disabled && r.action == ACT_QUICK_ACTION) return;
    if (u->import_folder && (r.action == ACT_SELECT || r.action == ACT_UNLOAD ||
                             r.action == ACT_GAME || r.action == ACT_PICK ||
                             r.action == ACT_QUICK)) {
        cr_ui_message(u, "Importation du dossier en cours");
        return;
    }
    if (u->installing && (r.action == ACT_INSTALL_CORE_PACK || r.action == ACT_GAME ||
                          r.action == ACT_SELECT || r.action == ACT_PICK ||
                          r.action == ACT_UNLOAD || r.action == ACT_SAVE_STATE ||
                          r.action == ACT_LOAD_STATE || r.action == ACT_OPTION_VALUE ||
                          r.action == ACT_SUBSYSTEM_START || r.action == ACT_SUBSYSTEM_IMPORT)) {
        cr_ui_message(u, "Téléchargement du cœur en cours");
        return;
    }
    if (r.disabled && r.action != ACT_GAME) {
        if (r.hint[0]) cr_ui_message(u, r.hint);
        return;
    }
    switch (r.action) {
    case ACT_CORE_DETAILS:
        cr_menu_open_core_details(u, r.value);
        break;
    case ACT_STATE_GAME:
        if (r.value < 0) { if (u->engine.loaded) open_state_list(u, u->engine.id); }
        else if ((size_t)r.value < u->game_count) open_state_list(u, u->games[r.value].id);
        break;
    case ACT_STATE_CREATE:
        create_named_state(u);
        break;
    case ACT_STATE_ACTIONS:
        if (r.value >= 0 && (size_t)r.value < u->saved_state_count) {
            u->selected_state = r.value;
            cr_menu_push_page(u, CR_STATE_ACTIONS);
        }
        break;
    case ACT_STATE_LOAD: {
        cr_state_entry entry;
        if (cr_menu_selected_state_entry(u, &entry)) load_named_state(u, &entry);
        break;
    }
    case ACT_STATE_RENAME: {
        cr_state_entry entry;
        if (u->platform.edit_text && cr_menu_selected_state_entry(u, &entry)) {
            snprintf(u->state_rename, sizeof(u->state_rename), "%s", entry.name);
            u->ui_editing = 4;
            u->platform.edit_text(u->platform.context, 4, u->state_rename, false);
        }
        break;
    }
    case ACT_STATE_DUPLICATE: {
        cr_state_entry entry, created; char error[256] = {0};
        if (cr_menu_selected_state_entry(u, &entry)) {
            bool ok = cr_state_store_duplicate(u->engine.root, &entry, &created, error, sizeof(error));
            cr_menu_pop_page(u); cr_menu_refresh_saved_states(u);
            cr_ui_message(u, ok ? "État dupliqué" : error);
        }
        break;
    }
    case ACT_STATE_SHARE: {
        cr_state_entry entry; char error[256] = {0}; uint8_t *bytes = NULL; size_t size = 0;
        if (u->platform.share_state && cr_menu_selected_state_entry(u, &entry)) {
            if (cr_state_store_read(u->engine.root, &entry, &bytes, &size, error, sizeof(error))) {
                u->platform.share_state(u->platform.context, entry.name, bytes, size);
                cr_ui_message(u, "Ouverture du partage...");
            } else cr_ui_message(u, error);
        }
        free(bytes);
        break;
    }
    case ACT_STATE_DELETE_CONFIRM:
        cr_menu_push_page(u, CR_STATE_DELETE_CONFIRM);
        break;
    case ACT_STATE_DELETE: {
        cr_state_entry entry; char error[256] = {0};
        if (cr_menu_selected_state_entry(u, &entry)) {
            bool ok = cr_state_store_delete(u->engine.root, &entry, error, sizeof(error));
            cr_menu_pop_page(u); cr_menu_pop_page(u); cr_menu_refresh_saved_states(u);
            cr_ui_message(u, ok ? "État supprimé" : error);
        }
        break;
    }
    case ACT_CORE_REMOVE_CANCEL:
        cr_menu_pop_page(u); break;
    case ACT_CORE_REMOVE_CONFIRM: {
        const cr_core_descriptor *core = cr_registry_get(r.value);
        if (!core || !u->platform.can_remove_core || !u->platform.remove_core ||
            !u->platform.can_remove_core(u->platform.context,core->id)) break;
        snprintf(u->remove_core_id,sizeof(u->remove_core_id),"%s",core->id);
        snprintf(u->remove_core_name,sizeof(u->remove_core_name),"%s",core->name);
        cr_menu_push_page(u,CR_REMOVE_CORE_CONFIRM); break;
    }
    case ACT_CORE_REMOVE: {
        int slot = cr_registry_find(u->remove_core_id);
        if (slot < 0 || !u->platform.can_remove_core || !u->platform.remove_core ||
            !u->platform.can_remove_core(u->platform.context,u->remove_core_id)) break;
        if (u->engine.handle && u->engine.system == slot) {
            if (!result(u,cr_engine_close(&u->engine))) break;
            ++u->core_revision; clear_link_selection(u);
            u->option_key[0]=0; u->input_player=0; u->title[0]=0;
            cr_cheevos_unload_game(u->achievements); cr_notifications_clear(&u->notifications);
        }
        u->installing=true;
        cr_ui_clear_input(u);
        cr_menu_page(u,CR_CORES);
        u->platform.remove_core(u->platform.context,u->remove_core_id);
        cr_ui_message(u,"Suppression du cœur..."); break;
    }
    case ACT_SETTING_VALUES: {
        u->editing_setting = r.value;
        cr_menu_push_page(u, CR_SETTING_VALUES);
        const cr_menu_setting_spec *spec = &cr_menu_settings[r.value];
        u->focus = (cr_menu_setting_get(u, r.value) - spec->minimum) / spec->step;
        cr_ui_move(u, 0);
        break;
    }
    case ACT_SETTING_VALUE:
        cr_menu_setting_apply(u, u->editing_setting, r.value);
        cr_menu_pop_page(u);
        break;
    case ACT_SETTING_TOGGLE:
        cr_menu_setting_apply(u, r.value, !cr_menu_setting_get(u, r.value));
        break;
    case ACT_LAYOUT_RESET:
        cr_menu_setting_apply(u, SET_SIZE, 100); cr_menu_setting_apply(u, SET_POSITION, 0);
        cr_ui_message(u, "Disposition rétablie");
        break;
    case ACT_BIND_CAPTURE: {
        char name[192];
        if (!u->platform.controller_info ||
            (!u->platform.controller_info(u->platform.context, 0, name, sizeof(name)) &&
             !u->platform.controller_info(u->platform.context, 1, name, sizeof(name)))) {
            cr_ui_message(u, "Connectez une manette pour affecter ses touches"); break;
        }
        cr_ui_clear_input(u); u->controller_capture = r.value;
        cr_ui_message(u, "Appuyez sur un bouton de la manette");
        break;
    }
    case ACT_BIND_RESET:
        for (unsigned i = 0; i < 10; ++i) {
            char key[48]; snprintf(key, sizeof(key), "controller_bind_%u", i);
            u->controller_bindings[i] = cr_controller_binding_default(i);
            if (u->platform.set_preference) u->platform.set_preference(u->platform.context, key, u->controller_bindings[i]);
        }
        cr_ui_clear_input(u); cr_ui_message(u, "Touches rétablies");
        break;
    case ACT_LOAD_AUTO_STATE:
        if (u->auto_state_available && result(u, cr_engine_load_auto_state(&u->engine))) {
            cr_cheevos_reset(u->achievements); cr_menu_page(u, CR_PLAY);
            cr_ui_message(u, "État automatique chargé");
        }
        break;
    case ACT_SAVE_PLAYER:
        u->save_player = u->save_player ? 0 : 1;
        break;
    case ACT_SAVE_TRANSFER:
        if (!r.disabled) begin_save_transfer(u, r.value, -1);
        break;
    case ACT_BACKUP_MENU:
        u->backup_available = 0;
        for (unsigned i = 0; i < (unsigned)u->backup_count; ++i) {
            uint8_t *bytes = NULL; size_t size = 0;
            if (cr_engine_export_backup(&u->engine, u->save_player, i, &bytes, &size))
                u->backup_available |= 1u << i;
            free(bytes);
        }
        cr_menu_push_page(u, CR_GAME_BACKUPS);
        break;
    case ACT_EXPORT_BACKUP:
        if (!r.disabled) begin_save_transfer(u, CR_SAVE_EXPORT_SAV, r.value);
        break;
    case ACT_SAVE_CANCEL:
        cr_menu_finish_save_transfer(u); cr_menu_pop_page(u); cr_ui_message(u, "Importation annulée");
        break;
    case ACT_SAVE_COMMIT: {
        bool ok = import_save_file(u);
        cr_menu_finish_save_transfer(u);
        if (ok) {
            ++u->core_revision; cr_cheevos_reset(u->achievements);
            cr_menu_achievement_sync(u, true); cr_menu_page(u, CR_PLAY);
            cr_ui_message(u, "Sauvegarde importée : contenu redémarré");
        } else cr_menu_pop_page(u);
        break;
    }
    case ACT_CORE_OPTION: {
        corvus_core_option option;
        if (cr_engine_option(&u->engine, (size_t)r.value, &option)) {
            snprintf(u->option_key, sizeof(u->option_key), "%s", option.key);
            cr_menu_push_page(u, CR_CORE_OPTION_VALUES);
            u->focus = (int)cr_engine_option_value(&u->engine, option.key);
            if (u->focus < 0 || (uint32_t)u->focus >= option.value_count) u->focus = 0;
            cr_ui_move(u, 0);
        }
        break;
    }
    case ACT_OPTION_VALUE: {
        corvus_core_option option;
        if (cr_menu_selected_option(u, &option) &&
            result(u, cr_engine_set_option(&u->engine, option.key, (uint32_t)r.value))) {
            bool restart = (option.flags & CORVUS_OPTION_RESTART_REQUIRED) != 0;
            cr_menu_pop_page(u);
            cr_ui_message(u, restart ? "Option enregistrée pour le prochain redémarrage" :
                                      "Option du cœur appliquée");
        }
        break;
    }
    case ACT_SUBSYSTEM: {
        corvus_subsystem_info subsystem;
        if (!r.disabled && cr_engine_subsystem(&u->engine, (size_t)r.value, &subsystem)) {
            if (strcmp(u->subsystem_id, subsystem.id)) clear_link_selection(u);
            snprintf(u->subsystem_id, sizeof(u->subsystem_id), "%s", subsystem.id);
            cr_menu_push_page(u, CR_SUBSYSTEM_CONFIG);
        }
        break;
    }
    case ACT_SUBSYSTEM_SLOT:
        if (r.value >= 0 && r.value < 2) {
            u->link_slot = r.value;
            cr_menu_push_page(u, CR_SUBSYSTEM_CONTENTS);
        }
        break;
    case ACT_SUBSYSTEM_GAME:
        if (r.value >= 0 && cr_menu_link_game(u, (size_t)r.value)) {
            const cr_game *game = &u->games[r.value];
            snprintf(u->link_ids[u->link_slot], sizeof(u->link_ids[0]), "%s", game->id);
            snprintf(u->link_titles[u->link_slot], sizeof(u->link_titles[0]), "%s", game->title);
            cr_menu_pop_page(u);
        }
        break;
    case ACT_SUBSYSTEM_IMPORT:
        if (!u->importing && u->engine.handle && u->platform.pick) {
            u->importing = true;
            u->import_link = true;
            u->import_slot = u->link_slot;
            u->import_revision = u->core_revision;
            snprintf(u->import_subsystem_id, sizeof(u->import_subsystem_id), "%s", u->subsystem_id);
            u->platform.pick(u->platform.context, u->engine.system);
        }
        break;
    case ACT_SUBSYSTEM_START:
        if (u->importing) cr_ui_message(u, "Terminez l’import avant de démarrer les contenus");
        else start_subsystem(u);
        break;
    case ACT_QUICK_ACTION:
        cr_menu_quick_action(u, r.value);
        break;
    case ACT_SAVE_STATE:
    case ACT_LOAD_STATE: {
        bool load = r.action == ACT_LOAD_STATE;
        if (result(u, load ? cr_engine_load_state(&u->engine) : cr_engine_save_state(&u->engine))) {
            if (load) cr_cheevos_reset(u->achievements);
            cr_menu_page(u, CR_PLAY);
            cr_ui_message(u, load ? "État chargé" : "État sauvegardé");
        }
        break;
    }
    case ACT_RA_ENABLE:
        u->ra_enabled = !u->ra_enabled;
        if (u->platform.set_preference) u->platform.set_preference(u->platform.context, "achievements_enabled", u->ra_enabled);
        if (!u->ra_enabled) cr_cheevos_logout(u->achievements);
        cr_menu_achievement_sync(u, true);
        break;
    case ACT_RA_USER:
    case ACT_RA_PASSWORD:
        if (u->platform.edit_text) {
            int field = r.action == ACT_RA_USER ? 1 : 2;
            if (u->ra_editing == field) break;
            if (u->ra_editing) cr_ui_text_result(u, u->ra_editing, u->ra_preview, true);
            u->ra_editing = field;
            snprintf(u->ra_preview, sizeof(u->ra_preview), "%s", u->ra_editing == 1 ? u->ra_username : "");
            u->platform.edit_text(u->platform.context, u->ra_editing, u->ra_preview, u->ra_editing == 2);
        }
        break;
    case ACT_RA_CONNECT: {
        cr_cheevos_status status = {0};
        cr_cheevos_get_status(u->achievements, &status);
        if (status.pending && !status.connected) { cr_ui_message(u, "Connexion en cours..."); break; }
        if (status.connected) {
            cr_cheevos_logout(u->achievements);
            u->ra_enabled = false;
        } else {
            u->ra_enabled = cr_cheevos_login(u->achievements, u->ra_username, u->ra_password);
            memset(u->ra_password, 0, sizeof(u->ra_password));
            u->ra_game_id[0] = 0;
            cr_menu_achievement_sync(u, true);
        }
        if (u->platform.set_preference) u->platform.set_preference(u->platform.context, "achievements_enabled", u->ra_enabled);
        break;
    }
    case ACT_CORES:
        cr_menu_push_page(u, CR_CORES);
        break;
    case ACT_OPEN_DOWNLOADER:
        show_core_downloader(u);
        break;
    case ACT_REFRESH_CATALOG:
        if (u->platform.refresh_core_catalog && !u->catalog_loading) {
            u->catalog_loading = true;
            u->platform.refresh_core_catalog(u->platform.context);
        }
        break;
    case ACT_INSTALL_CORE_PACK:
        if (u->importing) {
            cr_ui_message(u, "Terminez l’import du contenu avant d’installer un cœur");
            break;
        }
        if (r.value >= 0 && (size_t)r.value < u->core_catalog.count &&
            u->platform.install_core_pack) {
            if (!result(u, cr_engine_save_battery(&u->engine))) break;
            u->installing = true;
            cr_ui_clear_input(u);
            u->platform.install_core_pack(u->platform.context,
                                          u->core_catalog.packs[r.value].id);
            cr_ui_message(u, "Téléchargement du cœur...");
        }
        break;
    case ACT_UNLOAD:
        if (result(u, cr_engine_close(&u->engine))) {
            ++u->core_revision;
            clear_link_selection(u);
            u->option_key[0] = 0;
            u->input_player = 0;
            u->title[0] = 0;
            cr_menu_page(u, CR_MAIN);
        }
        break;
    case ACT_LIBRARY:
        if (r.value == 1 && u->page == CR_LIBRARY_ACTIONS) {
            u->library_list = !u->library_list;
            cr_menu_pop_page(u); u->scroll = 0; u->focus = cr_menu_library_count(u) ? 1 : 0;
        } else cr_menu_push_page(u, CR_LIBRARY);
        break;
    case ACT_FOLDER:
        if (!u->engine.handle) {
            cr_ui_message(u, "Chargez un cœur avant d'importer un dossier");
            break;
        }
        if (!u->importing && u->platform.pick_folder) {
            const cr_core_descriptor *core = cr_registry_get(u->engine.system);
            if (!core) break;
            u->importing = true;
            u->import_folder = true;
            u->import_link = false;
            u->import_revision = u->core_revision;
            u->platform.pick_folder(u->platform.context, u->engine.system, core->extensions);
        }
        break;
    case ACT_PICK:
        if (!u->engine.handle) {
            cr_ui_message(u, "Chargez un cœur avant de charger du contenu");
            break;
        }
        if (!u->importing && u->platform.pick) {
            u->importing = true;
            u->import_link = false;
            u->import_revision = u->core_revision;
            u->platform.pick(u->platform.context, u->engine.system);
        }
        break;
    case ACT_HISTORY:
        cr_menu_push_page(u, CR_GAMES);
        u->filter = -1;
        break;
    case ACT_ABOUT:
        cr_menu_push_page(u, CR_ABOUT);
        break;
    case ACT_GAME: {
        if (r.value < 0 || (size_t)r.value >= u->game_count) break;
        cr_game *g = &u->games[r.value];
        if (g->system < 0) {
            cr_ui_message(u, "Le cœur associé à ce jeu n'est plus installé");
            break;
        }
        char path[4256];
        snprintf(path, sizeof(path), "%s/roms/%s.rom", u->engine.root, g->id);
        char title[512], id[96];
        snprintf(title, sizeof(title), "%s", g->title);
        snprintf(id, sizeof(id), "%s", g->id);
        open_game(u, g->system, id, title, path);
        break;
    }
    case ACT_SELECT:
        if (result(u, cr_engine_select(&u->engine, r.value))) {
            ++u->core_revision;
            clear_link_selection(u);
            u->option_key[0] = 0;
            u->input_player = 0;
            u->title[0] = 0;
            cr_menu_page(u, CR_MAIN);
        }
        break;
    case ACT_THEME:
        cr_menu_push_page(u, CR_THEMES);
        break;
    case ACT_SET_THEME:
        u->theme = cr_theme_valid(r.value);
        if (u->platform.set_preference)
            u->platform.set_preference(u->platform.context, "theme", u->theme);
        if (!cr_menu_pop_page(u))
            cr_menu_page(u, CR_UI_SETTINGS);
        break;
    case ACT_AUDIO:
        u->audio = !u->audio;
        if (u->platform.set_preference)
            u->platform.set_preference(u->platform.context, "audio", u->audio);
        break;
    case ACT_AWAKE:
        u->awake = !u->awake;
        if (u->platform.set_preference)
            u->platform.set_preference(u->platform.context, "keep_awake", u->awake);
        break;
    case ACT_LICENSE:
        if (!u->license && u->platform.credits)
            u->license = u->platform.credits(u->platform.context);
        cr_menu_push_page(u, CR_LICENSE);
        break;
    case ACT_QUICK:
        cr_menu_page(u, CR_PLAY);
        u->quick = true;
        break;
    case ACT_PAGE:
        if (r.value == CR_SAVESTATES) u->auto_state_available = cr_engine_has_auto_state(&u->engine);
        if (r.value == CR_GAME_SAVES) u->save_player = 0;
        cr_menu_push_page(u, r.value);
        break;
    case ACT_SHOW_INPUT:
        cr_ui_clear_input(u);
        u->show_input = !u->show_input;
        if (u->platform.set_preference)
            u->platform.set_preference(u->platform.context, "show_input", u->show_input);
        break;
    default:
        break;
    }
}
int cr_content_system(const char *title, int selected) {
    const char *extension = title ? strrchr(title, '.') : NULL;
    return cr_registry_choose(extension, selected);
}
void cr_ui_import(cr_ui *u, const cr_import *item) {
    bool link = u->import_link;
    bool folder = u->import_folder;
    if (!folder) {
        u->import_link = false;
        u->importing = false;
    }
    if (item->system < 0) {
        if (folder) {
            u->import_folder = false;
            u->importing = false;
            cr_ui_reload(u);
        }
        if (item->error[0])
            cr_ui_message(u, item->error);
        return;
    }
    cr_import resolved = *item;
    resolved.system = cr_content_system(item->title, item->system);
    const cr_core_descriptor *core = cr_registry_get(resolved.system);
    bool id_ok = cr_game_id_valid(item->id) && core != NULL;
    if (id_ok) {
        char hash[65];
        memcpy(hash, item->id, 64);
        hash[64] = '\0';
        id_ok = cr_registry_content_id(resolved.id, sizeof(resolved.id), hash, core->id);
    }
    char error[512] = {0};
    bool current = u->import_revision == u->core_revision && (!link ||
        (u->import_slot >= 0 && u->import_slot < 2 && u->link_slot == u->import_slot &&
         u->page == CR_SUBSYSTEM_CONTENTS && !strcmp(u->subsystem_id, u->import_subsystem_id)));
    bool selected = current && u->engine.handle && u->engine.system == resolved.system;
    bool valid = selected && id_ok && cr_engine_check(&u->engine, resolved.system, resolved.path);
    if (!current)
        snprintf(error, sizeof(error), "La sélection du cœur ou du sous-système a changé");
    else if (!u->engine.handle)
        snprintf(error, sizeof(error), "Chargez un cœur avant de lancer un jeu");
    else if (!selected)
        snprintf(error, sizeof(error), "Chargez le cœur associé à ce contenu");
    else if (!valid)
        snprintf(error, sizeof(error), "%s",
                 u->engine.error[0] ? u->engine.error : "Contenu incompatible");
    bool committed = u->platform.finish_import &&
                     u->platform.finish_import(u->platform.context, valid, resolved.system,
                                               core ? core->id : "", core ? core->system_id : "",
                                               error, sizeof(error));
    if (!valid || !committed) {
        if (!folder) cr_ui_message(u, error);
        return;
    }
    cr_ui_reload(u);
    if (folder) return;
    if (link) {
        snprintf(u->link_ids[u->import_slot], sizeof(u->link_ids[0]), "%s", resolved.id);
        snprintf(u->link_titles[u->import_slot], sizeof(u->link_titles[0]), "%s", resolved.title);
        cr_menu_pop_page(u);
        return;
    }
    char path[4256];
    snprintf(path, sizeof(path), "%s/roms/%s.rom", u->engine.root, resolved.id);
    open_game(u, resolved.system, resolved.id, resolved.title, path);
}
void cr_menu_quick_action(cr_ui *u, int action) {
    if (action < 0 || action >= CR_QUICK_COUNT ||
        (action == CR_QUICK_RESTART && !cr_engine_can_reset(&u->engine)) ||
        ((action == CR_QUICK_STATES || action == CR_QUICK_LOAD_STATE || action == CR_QUICK_SAVE_STATE) &&
         !cr_engine_can_state(&u->engine))) return;
    if ((action == CR_QUICK_LOAD_STATE || action == CR_QUICK_SAVE_STATE) && state_busy(u)) return;
    if (u->save_request && (action == CR_QUICK_RESTART || action == CR_QUICK_CLOSE ||
                           action == CR_QUICK_STATES || action == CR_QUICK_GAME_SAVES)) {
        cr_ui_message(u, "Transfert de sauvegarde en cours"); return;
    }
    if (u->installing && (action == CR_QUICK_RESTART || action == CR_QUICK_CLOSE ||
                          action == CR_QUICK_STATES)) {
        cr_ui_message(u, "Téléchargement du cœur en cours");
        return;
    }
    switch (action) {
    case CR_QUICK_LOAD_STATE:
        open_state_list(u, u->engine.id);
        break;
    case CR_QUICK_SAVE_STATE:
        create_named_state(u);
        break;
    case CR_QUICK_RESUME:
        cr_menu_page(u, CR_PLAY);
        break;
    case CR_QUICK_RESTART:
        if (result(u, cr_engine_reset(&u->engine))) {
            cr_menu_page(u, CR_PLAY);
            cr_cheevos_reset(u->achievements);
            cr_ui_message(u, "Contenu redémarré");
        }
        break;
    case CR_QUICK_CLOSE:
        if (result(u, cr_engine_unload_game(&u->engine))) {
            cr_menu_achievement_sync(u, false);
            u->title[0] = 0;
            u->input_player = 0;
            cr_menu_page(u, CR_MAIN);
        }
        break;
    case CR_QUICK_GAME_SAVES:
        if (u->platform.save_transfer && (cr_engine_battery_size(&u->engine, 0) ||
                                        cr_engine_battery_size(&u->engine, 1))) {
            u->save_player = 0; cr_menu_push_page(u, CR_GAME_SAVES);
        }
        break;
    case CR_QUICK_STATES:
        u->auto_state_available = cr_engine_has_auto_state(&u->engine);
        cr_menu_push_page(u, CR_SAVESTATES); break;
    case CR_QUICK_CORE_OPTIONS: cr_menu_push_page(u, CR_CORE_OPTIONS); break;
    case CR_QUICK_CONTROLS: cr_menu_push_page(u, CR_OVERLAY_SETTINGS); break;
    case CR_QUICK_ACHIEVEMENTS: cr_menu_push_page(u, CR_ACHIEVEMENTS); break;
    case CR_QUICK_CORE_INFO: cr_menu_push_page(u, CR_CORE_INFO); break;
    case CR_QUICK_MAIN: cr_menu_page(u, CR_MAIN); break;
    case CR_QUICK_PLAYER:
        if (u->engine.players > 1) u->input_player = u->input_player == 0 ? 1 : 0;
        break;
    default: break;
    }
    cr_ui_clear_input(u);
}
