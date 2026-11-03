// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
#define _POSIX_C_SOURCE 200809L
#include "menu_driver.h"
#include "play_layout.h"
#include "overlay.h"
#include "core_registry.h"
#include "interface_draw.h"
#include "interface_layout.h"
#include "state_store.h"
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdatomic.h>
#include <sys/stat.h>
#include <unistd.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static const char *core_name(int index) {
    const cr_core_descriptor *core = cr_registry_get(index);
    return core ? core->name : "Cœur indisponible";
}
static const char *system_name(int index) {
    const cr_core_descriptor *core = cr_registry_get(index);
    return core ? core->system_name : "Système indisponible";
}

enum {
    ACT_NONE,
    ACT_CORE_REMOVE_CANCEL,
    ACT_CORE_REMOVE_CONFIRM,
    ACT_CORE_REMOVE,
    ACT_CORES,
    ACT_UNLOAD,
    ACT_PICK,
    ACT_HISTORY,
    ACT_ABOUT,
    ACT_GAME,
    ACT_THEME,
    ACT_AUDIO,
    ACT_AWAKE,
    ACT_LICENSE,
    ACT_SELECT,
    ACT_SET_THEME,
    ACT_QUICK,
    ACT_PAGE,
    ACT_SHOW_INPUT,
    ACT_OPEN_DOWNLOADER,
    ACT_REFRESH_CATALOG,
    ACT_INSTALL_CORE_PACK,
    ACT_RA_ENABLE,
    ACT_RA_USER,
    ACT_RA_PASSWORD,
    ACT_RA_CONNECT,
    ACT_QUICK_ACTION,
    ACT_SAVE_STATE,
    ACT_LOAD_STATE,
    ACT_CORE_OPTION,
    ACT_OPTION_VALUE,
    ACT_SUBSYSTEM,
    ACT_SUBSYSTEM_SLOT,
    ACT_SUBSYSTEM_GAME,
    ACT_SUBSYSTEM_IMPORT,
    ACT_SUBSYSTEM_START,
    ACT_LIBRARY,
    ACT_FOLDER,
    ACT_SAVE_TRANSFER,
    ACT_SAVE_PLAYER,
    ACT_SAVE_COMMIT,
    ACT_SAVE_CANCEL,
    ACT_SETTING_VALUES,
    ACT_SETTING_VALUE,
    ACT_SETTING_TOGGLE,
    ACT_LAYOUT_RESET,
    ACT_BIND_CAPTURE,
    ACT_BIND_RESET,
    ACT_LOAD_AUTO_STATE,
    ACT_BACKUP_MENU,
    ACT_EXPORT_BACKUP,
    ACT_CORE_DETAILS,
    ACT_STATE_GAME,
    ACT_STATE_CREATE,
    ACT_STATE_ACTIONS,
    ACT_STATE_LOAD,
    ACT_STATE_RENAME,
    ACT_STATE_DUPLICATE,
    ACT_STATE_SHARE,
    ACT_STATE_DELETE_CONFIRM,
    ACT_STATE_DELETE
};
typedef cr_menu_row row;
enum { SET_VOLUME, SET_INTEGER, SET_FILTER, SET_OPACITY, SET_SIZE, SET_POSITION,
       SET_CARDS, SET_PROGRESS, SET_BACKUPS, SET_AUTO_STATE, SET_COUNT };
typedef struct {
    const char *key, *label;
    int minimum, maximum, step, fallback;
    size_t offset;
} setting_spec;
#define SETTING(key, label, field, min, max, step, fallback) \
    {key, label, min, max, step, fallback, offsetof(cr_ui, field)}
static const setting_spec settings[SET_COUNT] = {
    SETTING("audio_volume", "Volume du jeu", volume, 0, 100, 10, 100),
    SETTING("video_integer", "Présentation de l'image", video_integer, 0, 1, 1, 0),
    SETTING("video_smooth", "Filtrage", video_smooth, 0, 1, 1, 0),
    SETTING("overlay_opacity", "Intensité au repos", overlay_opacity, 10, 100, 10, 100),
    SETTING("overlay_size", "Taille des commandes", overlay_size, 100, 130, 10, 100),
    SETTING("overlay_position", "Position verticale", overlay_position, -15, 15, 5, 0),
    SETTING("notify_cards", "Cartes de jeu et de succès", notify_cards, 0, 1, 1, 1),
    SETTING("notify_progress", "Progression des succès", notify_progress, 0, 1, 1, 1),
    SETTING("backup_count", "Copies de sécurité", backup_count, 1, 3, 1, 1),
    SETTING("auto_state", "État automatique", auto_state, 0, 1, 1, 0)
};
#undef SETTING
static int *setting_value(cr_ui *u, int id) {
    return (int *)((char *)u + settings[id].offset);
}
static int setting_get(const cr_ui *u, int id) {
    return *(const int *)((const char *)u + settings[id].offset);
}
static void setting_label(int id, int value, char *out, size_t size) {
    if (id == SET_INTEGER) snprintf(out, size, "%s", value ? "Échelle entière" : "Ratio natif adapté");
    else if (id == SET_FILTER) snprintf(out, size, "%s", value ? "Interpolation" : "Pixels nets");
    else if (id == SET_BACKUPS) snprintf(out, size, "%d copie%s par joueur", value, value > 1 ? "s" : "");
    else if (id == SET_POSITION && !value) snprintf(out, size, "%s", "Automatique");
    else if (id == SET_POSITION) snprintf(out, size, "%+d %%", value);
    else snprintf(out, size, "%d %%", value);
}
static void apply_setting(cr_ui *u, int id, int value) {
    *setting_value(u, id) = value;
    if (u->platform.set_preference)
        u->platform.set_preference(u->platform.context, settings[id].key, value);
    if (id == SET_SIZE || id == SET_POSITION || id == SET_OPACITY) cr_ui_clear_input(u);
    if (id == SET_CARDS && !value) cr_notifications_hide_cards(&u->notifications);
    if (id == SET_PROGRESS && !value) cr_notifications_hide_progress(&u->notifications, 0);
    if (id == SET_BACKUPS || id == SET_AUTO_STATE)
        cr_engine_set_save_policy(&u->engine, (unsigned)u->backup_count, u->auto_state != 0);
}
static const char *binding_labels[10] = {"Haut", "Bas", "Gauche", "Droite", "A", "B", "Start", "Select", "L", "R"};
static const int quick_order[CR_QUICK_COUNT] = {
    CR_QUICK_RESUME, CR_QUICK_LOAD_STATE, CR_QUICK_SAVE_STATE, CR_QUICK_RESTART,
    CR_QUICK_CORE_OPTIONS, CR_QUICK_CLOSE, CR_QUICK_STATES,
    CR_QUICK_GAME_SAVES, CR_QUICK_CONTROLS, CR_QUICK_PLAYER,
    CR_QUICK_ACHIEVEMENTS, CR_QUICK_CORE_INFO, CR_QUICK_MAIN
};
int cr_ui_quick_row(const cr_ui *u, int action) {
    int row = 0;
    for (int i = 0; i < CR_QUICK_COUNT; ++i) {
        if (quick_order[i] == CR_QUICK_PLAYER && u->engine.players < 2) continue;
        if (quick_order[i] == action) return row;
        ++row;
    }
    return -1;
}
static int quick_at(const cr_ui *u, int row) {
    for (int i = 0; i < CR_QUICK_COUNT; ++i) {
        if (quick_order[i] == CR_QUICK_PLAYER && u->engine.players < 2) continue;
        if (!row--) return quick_order[i];
    }
    return CR_QUICK_RESUME;
}
static int setting_icon(int id) {
    static const int icons[SET_COUNT] = {CR_ICON_VOLUME, CR_ICON_DISPLAY, CR_ICON_VIDEO,
        CR_ICON_TOUCH, CR_ICON_TOUCH, CR_ICON_TOUCH, CR_ICON_NOTIFICATIONS,
        CR_ICON_NOTIFICATIONS, CR_ICON_BACKUP, CR_ICON_SAVES};
    return icons[id];
}
static void quick_action(cr_ui *u, int action);
static void quick_clamp(cr_ui *u);
static void finish_save_transfer(cr_ui *u);
static int target(const cr_ui *u, float x, float y);
static bool has_back(const cr_ui *u) {
    return u->quick || u->depth || (u->page != CR_MAIN && u->page != CR_LIBRARY &&
           u->page != CR_SAVES_LIBRARY && u->page != CR_CORES && u->page != CR_SETTINGS);
}
static void clear_link_selection(cr_ui *u) {
    u->subsystem_id[0] = 0;
    memset(u->link_ids, 0, sizeof(u->link_ids));
    memset(u->link_titles, 0, sizeof(u->link_titles));
    u->link_slot = 0;
}
static bool selected_option(const cr_ui *u, corvus_core_option *out) {
    for (size_t i = 0; i < cr_engine_option_count(&u->engine); ++i) {
        corvus_core_option option;
        if (cr_engine_option(&u->engine, i, &option) && !strcmp(option.key, u->option_key)) {
            *out = option;
            return true;
        }
    }
    return false;
}
static bool selected_subsystem(const cr_ui *u, corvus_subsystem_info *out) {
    for (size_t i = 0; i < cr_engine_subsystem_count(&u->engine); ++i) {
        corvus_subsystem_info subsystem;
        if (cr_engine_subsystem(&u->engine, i, &subsystem) &&
            !strcmp(subsystem.id, u->subsystem_id) && subsystem.game_count == 2 &&
            subsystem.num_players == 2) {
            *out = subsystem;
            return true;
        }
    }
    return false;
}
static bool link_game(const cr_ui *u, size_t index) {
    return u->engine.handle && index < u->game_count &&
           u->games[index].system == u->engine.system;
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
static int game_console(const cr_game *g) {
    static const char *ids[] = {"nintendo.game_boy", "nintendo.game_boy_color", "nintendo.game_boy_advance"};
    static const char *cores[] = {"corvus.gb", "corvus.gbc", "corvus.gba"};
    for (int i = 0; i < 3; ++i)
        if (!strcmp(g->system_id, ids[i]) || (!g->system_id[0] && !strcmp(g->core_id, cores[i]))) return i;
    return -1;
}
static int game_index(const cr_ui *u, const char *id) {
    for (size_t i = 0; i < u->game_count; ++i)
        if (!strcmp(u->games[i].id, id)) return (int)i;
    return -1;
}
static uint32_t folded_character(const char **at) {
    const unsigned char *s = (const unsigned char *)*at;
    uint32_t c = *s++;
    unsigned count = c >= 0xf0 ? 3 : c >= 0xe0 ? 2 : c >= 0xc0 ? 1 : 0;
    if (count) {
        c &= (1u << (6 - count)) - 1;
        for (unsigned i = 0; i < count && (*s & 0xc0) == 0x80; ++i) c = (c << 6) | (*s++ & 63);
    }
    *at = (const char *)s;
    return (c >= 'A' && c <= 'Z') || (c >= 0xc0 && c <= 0xde && c != 0xd7) ? c + 32 : c;
}
static bool matches_query(const char *title, const char *query) {
    if (!query[0]) return true;
    for (const char *start = title; *start; folded_character(&start)) {
        const char *a = start, *b = query;
        bool equal = true;
        while (*a && *b) if (folded_character(&a) != folded_character(&b)) { equal = false; break; }
        if (equal && !*b) return true;
    }
    return false;
}
static int library_game(const cr_ui *u, int row_index) {
    for (size_t i = 0; i < u->game_count; ++i)
        if ((u->library_filter < 0 || game_console(&u->games[i]) == u->library_filter) &&
            matches_query(u->games[i].title, u->library_query) && !row_index--) return (int)i;
    return -1;
}
static int library_count(const cr_ui *u) {
    int count = 0;
    for (size_t i = 0; i < u->game_count; ++i)
        if ((u->library_filter < 0 || game_console(&u->games[i]) == u->library_filter) &&
            matches_query(u->games[i].title, u->library_query)) ++count;
    return count;
}
static int detail_pack(const cr_ui *u) {
    for (size_t i = 0; i < u->core_catalog.count; ++i)
        if ((u->detail_pack_id[0] && !strcmp(u->core_catalog.packs[i].id, u->detail_pack_id)) ||
            (!u->detail_pack_id[0] && !strcmp(u->core_catalog.packs[i].core_id, u->detail_core_id))) return (int)i;
    return -1;
}
static int removable_core_at(const cr_ui *u, int row) {
    if (!u->platform.can_remove_core || !u->platform.remove_core) return -1;
    for (size_t i = 0; i < cr_registry_count(); ++i) {
        int index = cr_registry_index_at(i);
        const cr_core_descriptor *core = cr_registry_get(index);
        if (core && u->platform.can_remove_core(u->platform.context, core->id) && !row--) return index;
    }
    return -1;
}
static int removable_cores(const cr_ui *u) {
    int count = 0;
    if (!u->platform.can_remove_core || !u->platform.remove_core) return 0;
    for (size_t i = 0; i < cr_registry_count(); ++i) {
        const cr_core_descriptor *core = cr_registry_get(cr_registry_index_at(i));
        if (core && u->platform.can_remove_core(u->platform.context,core->id)) ++count;
    }
    return count;
}
static int rows(const cr_ui *u) {
    if (u->quick) return CR_QUICK_COUNT - (u->engine.players > 1 ? 0 : 1);
    if (u->page == CR_CORE_OPTIONS) return (int)cr_engine_option_count(&u->engine);
    if (u->page == CR_SUBSYSTEMS) return (int)cr_engine_subsystem_count(&u->engine);
    if (u->page == CR_CORE_OPTION_VALUES) {
        corvus_core_option option;
        return selected_option(u, &option) ? (int)option.value_count : 0;
    }
    if (u->page == CR_SUBSYSTEM_CONFIG) {
        corvus_subsystem_info subsystem;
        return selected_subsystem(u, &subsystem) ? 3 : 0;
    }
    if (u->page == CR_SUBSYSTEM_CONTENTS) {
        int count = 1;
        for (size_t i = 0; i < u->game_count; ++i) if (link_game(u, i)) ++count;
        return count;
    }
    if (u->page == CR_GAME_SAVES) return 4 + (u->engine.players > 1);
    if (u->page == CR_GAME_BACKUPS) return u->backup_count;
    if (u->page == CR_SAVE_CONFIRM) return 2;
    if (u->page == CR_SAVESTATES) return 3;
    if (u->page == CR_CONTENT) return 2;
    if (u->page == CR_LIBRARY) return 1 + library_count(u);
    if (u->page == CR_MAIN) return 5 + (u->engine.handle != NULL);
    if (u->page == CR_SAVES_LIBRARY) return 1 + (int)u->game_count;
    if (u->page == CR_STATE_LIST) return 1 + (int)u->saved_state_count +
        (u->engine.loaded && !strcmp(u->engine.id, u->states_game_id) ? 2 : 0);
    if (u->page == CR_STATE_ACTIONS) return 5;
    if (u->page == CR_STATE_DELETE_CONFIRM) return 2;
    if (u->page == CR_CORE_DETAILS) return 4;
    if (u->page == CR_LIBRARY_ACTIONS) return 4;
    if (u->page == CR_GAME_ACTIONS) return 3;
    if (u->page == CR_SYSTEM_SETTINGS) return 4;
    if (u->page == CR_LISTS)
        return 1;
    if (u->page == CR_CORES)
        return 1 + (int)cr_registry_count() + (removable_cores(u) > 0);
    if (u->page == CR_REMOVE_CORES) return removable_cores(u);
    if (u->page == CR_REMOVE_CORE_CONFIRM) return 2;
    if (u->page == CR_CORE_DOWNLOADS)
        return 1 + (int)u->core_catalog.count;
    if (u->page == CR_SETTINGS) return 7;
    if (u->page == CR_SETTING_VALUES) {
        const setting_spec *spec = &settings[u->editing_setting];
        return 1 + (spec->maximum - spec->minimum) / spec->step;
    }
    if (u->page == CR_NOTIFICATIONS_SETTINGS || u->page == CR_UI_SETTINGS ||
        u->page == CR_AUDIO_SETTINGS || u->page == CR_OSD_SETTINGS ||
        u->page == CR_SAVE_SETTINGS || u->page == CR_RA_SETTINGS) return 2;
    if (u->page == CR_OVERLAY_SETTINGS || u->page == CR_OVERLAY_LAYOUT ||
        u->page == CR_VIDEO_SETTINGS || u->page == CR_CONTROLLER_SETTINGS) return 3;
    if (u->page == CR_CONTROLLER_BINDINGS) return 11;
    if (u->page == CR_RA_ACCOUNT) return 4;
    if (u->page == CR_ACHIEVEMENTS) {
        size_t count = cr_cheevos_achievement_count(u->achievements);
        return 2 + (int)(count < 4096 ? count : 4096);
    }
    if (u->page == CR_INFORMATION)
        return 2 + (u->engine.handle ? 1 : 0) + (removable_cores(u) > 0);
    if (u->page == CR_THEMES)
        return CR_THEME_COUNT;
    if (u->page == CR_GAMES) {
        int n = 0;
        for (size_t i = 0; i < u->game_count; i++)
            if (u->filter < 0 ? u->games[i].played > 0 : u->games[i].system == u->filter)
                n++;
        return n;
    }
    return 0;
}
static void fill(row *r, const char *label, const char *hint, int icon, int action, int value) {
    memset(r, 0, sizeof(*r));
    r->console = -1;
    snprintf(r->label, sizeof(r->label), "%s", label);
    snprintf(r->hint, sizeof(r->hint), "%.*s", (int)sizeof(r->hint) - 1, hint ? hint : "");
    r->icon = icon;
    r->action = action;
    r->value = value;
}
static void game_row(row *r, const cr_ui *u, int index, int action) {
    if (index < 0 || (size_t)index >= u->game_count) return;
    const cr_game *g = &u->games[index];
    static const char *systems[] = {"Game Boy", "Game Boy Color", "Game Boy Advance"};
    int console = game_console(g);
    fill(r, g->title, console >= 0 ? systems[console] : g->system_id,
         CR_ICON_CONTENT, action, index);
    r->console = console;
    if (g->cover[0]) snprintf(r->image, sizeof(r->image), "cover:%s", g->id);
    if (action == ACT_GAME) {
        r->disabled = !u->engine.handle || g->system != u->engine.system;
        if (r->disabled) snprintf(r->hint, sizeof(r->hint), "%s", !u->engine.handle ?
            "Chargez un cœur pour jouer" : "Chargez le cœur associé à ce jeu");
    }
}
static row get_row(const cr_ui *u, int index) {
    row r;
    fill(&r, "", "", CR_ICON_FILE, ACT_NONE, 0);
    if (u->quick) {
        index = quick_at(u, index);
        static const char *labels[CR_QUICK_COUNT] = {
            "Reprendre le contenu", "Redémarrer le contenu", "Fermer le contenu",
            "Sauvegardes d’état", "Options du cœur", "Commandes", "Succès", "Informations sur le cœur",
            "Accueil", "Joueur contrôlé", "Sauvegardes du jeu", "Charger un état", "Sauvegarder un état"
        };
        static const char *hints[CR_QUICK_COUNT] = {
            "Retourner au jeu", "Relancer le contenu actuel", "Garder le cœur chargé",
            "Sauvegarder ou charger l’état du jeu", "Réglages proposés par le cœur", "Surimpression tactile",
            "Compte et succès du jeu", "Version et fonctions disponibles", "Retourner aux menus",
            "Choisir le joueur du tactile et du clavier", "Importer ou exporter la mémoire batterie (.sav / .srm)",
            "Choisir une sauvegarde de ce jeu", "Créer une nouvelle sauvegarde nommée"
        };
        static const int icons[CR_QUICK_COUNT] = {
            CR_ICON_PLAY, CR_ICON_RESTART, CR_ICON_CLOSE, CR_ICON_SAVES,
            CR_ICON_CORE, CR_ICON_INPUT, CR_ICON_ACHIEVEMENTS, CR_ICON_INFO,
            CR_ICON_HOME, CR_ICON_INPUT, CR_ICON_SAVES, CR_ICON_RESTORE, CR_ICON_SAVES
        };
        fill(&r, labels[index], hints[index], icons[index], ACT_QUICK_ACTION, index);
        r.disabled = (index == CR_QUICK_RESTART && !cr_engine_can_reset(&u->engine)) ||
                     ((index == CR_QUICK_STATES || index == CR_QUICK_LOAD_STATE || index == CR_QUICK_SAVE_STATE) &&
                      !cr_engine_can_state(&u->engine)) ||
                     (index == CR_QUICK_GAME_SAVES && (!u->platform.save_transfer ||
                      (!cr_engine_battery_size(&u->engine, 0) && !cr_engine_battery_size(&u->engine, 1))));
        if (index == CR_QUICK_PLAYER)
            snprintf(r.hint, sizeof(r.hint), "Joueur %u - toucher pour changer", u->input_player + 1);
        if (index == CR_QUICK_RESUME) snprintf(r.label, sizeof(r.label), "Reprendre");
        if (index == CR_QUICK_RESTART) snprintf(r.label, sizeof(r.label), "Redémarrer");
        if (index == CR_QUICK_CORE_OPTIONS) snprintf(r.label, sizeof(r.label), "Options du jeu");
        if (index == CR_QUICK_CLOSE) { snprintf(r.label, sizeof(r.label), "Quitter le jeu"); r.destructive = true; }
    } else if (u->page == CR_CORE_OPTIONS) {
        corvus_core_option option;
        if (cr_engine_option(&u->engine, (size_t)index, &option)) {
            uint32_t value = cr_engine_option_value(&u->engine, option.key);
            fill(&r, option.label, value < option.value_count ? option.values[value].label :
                 "Valeur indisponible", CR_ICON_CORE, ACT_CORE_OPTION, index);
        }
    } else if (u->page == CR_CORE_OPTION_VALUES) {
        corvus_core_option option;
        if (selected_option(u, &option) && (uint32_t)index < option.value_count) {
            fill(&r, option.values[index].label, (uint32_t)index == option.default_index ?
                 "Valeur par défaut" : (option.flags & CORVUS_OPTION_RESTART_REQUIRED ?
                 "Application au prochain redémarrage" : "Application immédiate"),
                 CR_ICON_CORE, ACT_OPTION_VALUE, index);
            r.checked = cr_engine_option_value(&u->engine, option.key) == (uint32_t)index;
        }
    } else if (u->page == CR_SUBSYSTEMS) {
        corvus_subsystem_info subsystem;
        if (cr_engine_subsystem(&u->engine, (size_t)index, &subsystem)) {
            fill(&r, subsystem.label, "Configurer les contenus et les joueurs",
                 CR_ICON_SUBSYSTEM, ACT_SUBSYSTEM, index);
            r.disabled = subsystem.game_count != 2 || subsystem.num_players != 2;
            if (r.disabled) snprintf(r.hint, sizeof(r.hint), "Configuration non prise en charge");
        }
    } else if (u->page == CR_SUBSYSTEM_CONFIG) {
        corvus_subsystem_info subsystem;
        if (selected_subsystem(u, &subsystem)) {
            if (index < 2)
                fill(&r, subsystem.games[index].label, u->link_ids[index][0] ? u->link_titles[index] :
                     "Choisir un contenu", CR_ICON_INPUT, ACT_SUBSYSTEM_SLOT, index);
            else {
                fill(&r, "Démarrer les contenus", "Lancer les deux consoles reliées",
                     CR_ICON_PLAY, ACT_SUBSYSTEM_START, 0);
                r.disabled = !u->link_ids[0][0] || !u->link_ids[1][0];
            }
        }
    } else if (u->page == CR_SUBSYSTEM_CONTENTS) {
        if (!index) {
            fill(&r, "Importer un contenu", "Ouvrir un fichier compatible avec le cœur",
                 CR_ICON_FOLDER, ACT_SUBSYSTEM_IMPORT, 0);
        } else {
            int n = 1;
            for (size_t i = 0; i < u->game_count; ++i)
                if (link_game(u, i) && n++ == index) {
                    fill(&r, u->games[i].title, system_name(u->engine.system), CR_ICON_CONTENT,
                         ACT_SUBSYSTEM_GAME, (int)i);
                    r.checked = !strcmp(u->link_ids[u->link_slot], u->games[i].id);
                    break;
                }
        }
    } else if (u->page == CR_GAME_SAVES) {
        if (u->engine.players > 1 && index == 0) {
            fill(&r, "Joueur de la sauvegarde", u->save_player ? "Joueur 2" : "Joueur 1",
                 CR_ICON_CONTENT, ACT_SAVE_PLAYER, 0);
        } else if (index - (u->engine.players > 1) == 3) {
            fill(&r, "Copies de secours", "Exporter une batterie avant import", CR_ICON_BACKUP, ACT_BACKUP_MENU, 0);
        } else {
            int operation = index - (u->engine.players > 1);
            static const char *labels[] = {"Importer une sauvegarde", "Exporter en .sav", "Exporter en .srm"};
            fill(&r, labels[operation], operation ? "Copie de la mémoire batterie actuelle" :
                 "Le contenu redémarrera après confirmation", CR_ICON_FILE, ACT_SAVE_TRANSFER, operation);
            r.disabled = u->save_request || !u->platform.save_transfer ||
                         !cr_engine_battery_size(&u->engine, u->save_player);
        }
    } else if (u->page == CR_GAME_BACKUPS) {
        char label[64]; snprintf(label, sizeof(label), "Copie de secours %d", index + 1);
        bool available = (u->backup_available & (1u << index)) != 0;
        fill(&r, label, available ? "Exporter en .sav" : "Absente ou illisible pour ce joueur",
             CR_ICON_BACKUP, ACT_EXPORT_BACKUP, index);
        r.disabled = !available;
    } else if (u->page == CR_SAVE_CONFIRM) {
        fill(&r, index ? "Annuler" : "Importer et redémarrer le contenu",
             index ? "Conserver la partie actuelle" : "Copie de secours conservée", CR_ICON_FILE,
             index ? ACT_SAVE_CANCEL : ACT_SAVE_COMMIT, 0);
    } else if (u->page == CR_SAVESTATES) {
        static const char *labels[] = {"Sauvegarder l’état", "Charger l’état", "Charger l'état automatique"};
        fill(&r, labels[index], index == 2 ? "Dernier état créé à la fermeture" :
             (index ? "Restaurer la sauvegarde de ce contenu" : "Enregistrer la position actuelle"),
             CR_ICON_FILE, index == 2 ? ACT_LOAD_AUTO_STATE : (index ? ACT_LOAD_STATE : ACT_SAVE_STATE), 0);
        if (index == 2) r.disabled = !u->auto_state_available;
    } else if (u->page == CR_MAIN) {
        if (!index) {
            int recent = -1;
            for (size_t i = 0; i < u->game_count; ++i)
                if (u->games[i].played && (recent < 0 || u->games[i].played > u->games[recent].played)) recent = (int)i;
            if (u->engine.loaded) fill(&r, "Continuer", u->title, CR_ICON_PLAY, ACT_QUICK, 0);
            else if (recent >= 0) {
                game_row(&r, u, recent, ACT_GAME);
                snprintf(r.hint, sizeof(r.hint), "%.220s", u->games[recent].title);
                snprintf(r.label, sizeof(r.label), "Continuer");
            } else fill(&r, u->engine.handle ? "Importer un jeu" : "Installer un cœur",
                        u->engine.handle ? "Construisez votre bibliothèque" : "Choisissez votre première console",
                        u->engine.handle ? CR_ICON_CONTENT : CR_ICON_DOWNLOAD,
                        u->engine.handle ? ACT_PICK : ACT_OPEN_DOWNLOADER, 0);
        } else if (index < 4) {
            static const char *labels[] = {"Bibliothèque", "Cœurs", "Saves"};
            static const int icons[] = {CR_ICON_INPUT, CR_ICON_CORE, CR_ICON_FOLDER};
            static const int pages[] = {CR_LIBRARY, CR_CORES, CR_SAVES_LIBRARY};
            fill(&r, labels[index - 1], "", icons[index - 1], ACT_PAGE, pages[index - 1]);
        } else if (index == 4) {
            fill(&r, u->engine.handle ? "Décharger le cœur" : "Charger un cœur",
                 u->engine.handle ? core_name(u->engine.system) : "Aucun cœur actif",
                 CR_ICON_CORE, u->engine.handle ? ACT_UNLOAD : ACT_CORES, 0);
            r.console = cr_registry_legacy_system(u->engine.system);
        } else fill(&r, "Sous-système", "Configurations du cœur actif", CR_ICON_SUBSYSTEM, ACT_PAGE, CR_SUBSYSTEMS);
    } else if (u->page == CR_LISTS) {
        fill(&r, "Historique", "Contenus récemment ouverts", CR_ICON_HISTORY, ACT_HISTORY, 0);
    } else if (u->page == CR_CONTENT) {
        if (!index) {
            fill(&r, "Importer un jeu", u->engine.handle ? "Choisir un fichier de jeu" :
                 "Chargez un cœur pour importer", CR_ICON_CONTENT, ACT_PICK, 0);
            r.disabled = !u->engine.handle || u->importing;
        } else
            fill(&r, "Bibliothèque", "Jeux importés et importation de dossiers",
                 CR_ICON_LIST, ACT_LIBRARY, 0);
    } else if (u->page == CR_LIBRARY) {
        if (!index) {
            fill(&r, "Importer et organiser", "Jeux, dossiers et historique", CR_ICON_FOLDER, ACT_PAGE, CR_LIBRARY_ACTIONS);
        } else game_row(&r, u, library_game(u, index - 1), ACT_GAME);
    } else if (u->page == CR_LIBRARY_ACTIONS) {
        if (!index) { fill(&r, "Importer un jeu", u->engine.handle ? "Ouvrir un fichier compatible" : "Chargez un cœur pour importer", CR_ICON_CONTENT, ACT_PICK, 0); r.disabled = !u->engine.handle || u->importing; }
        else if (index == 1) { fill(&r, "Importer un dossier", u->engine.handle ? "Ajouter plusieurs jeux" : "Chargez un cœur pour importer", CR_ICON_FOLDER, ACT_FOLDER, 0); r.disabled = !u->engine.handle || u->importing; }
        else if (index == 2) fill(&r, "Historique", "Retrouver les dernières parties", CR_ICON_HISTORY, ACT_HISTORY, 0);
        else fill(&r, u->library_list ? "Afficher en grille" : "Afficher en liste", "Présentation de la bibliothèque", CR_ICON_LIST, ACT_LIBRARY, 1);
    } else if (u->page == CR_GAME_ACTIONS) {
        int game = game_index(u, u->states_game_id);
        if (game >= 0) {
            if (!index) game_row(&r, u, game, ACT_GAME);
            else if (index == 1) {
                fill(&r, "États de sauvegarde", u->games[game].title, CR_ICON_SAVES, ACT_STATE_GAME, game);
            } else {
                int core = cr_registry_find(u->games[game].core_id);
                fill(&r, core >= 0 ? "Charger le cœur associé" : "Installer le cœur associé",
                     u->games[game].core_id, CR_ICON_CORE, core >= 0 ? ACT_SELECT : ACT_OPEN_DOWNLOADER, core);
            }
        }
    } else if (u->page == CR_SAVES_LIBRARY) {
        if (!index) {
            fill(&r, "Partie en cours", u->engine.loaded ? u->title : "Lancez un jeu pour créer un état", CR_ICON_SAVES, ACT_STATE_GAME, -1);
            r.disabled = !u->engine.loaded;
        } else game_row(&r, u, index - 1, ACT_STATE_GAME);
    } else if (u->page == CR_STATE_LIST) {
        if (!index) {
            fill(&r, "Nouvelle sauvegarde", "Enregistrer la position actuelle", CR_ICON_SAVES, ACT_STATE_CREATE, 0);
            r.disabled = !u->engine.loaded || strcmp(u->engine.id, u->states_game_id) || !cr_engine_can_state(&u->engine);
            if (r.disabled) snprintf(r.hint, sizeof(r.hint), "Lancez ce jeu avec un cœur compatible");
        } else if ((size_t)(index - 1) < u->saved_state_count) {
            const cr_state_entry *state = &u->saved_states[index - 1];
            char stamp[80]; time_t time = (time_t)(state->created_ms / 1000); struct tm local;
            if (localtime_r(&time, &local)) strftime(stamp, sizeof(stamp), "%d/%m/%Y · %H:%M", &local);
            else snprintf(stamp, sizeof(stamp), "État %u", state->slot);
            fill(&r, state->name, stamp, CR_ICON_SAVES, ACT_STATE_ACTIONS, index - 1);
            snprintf(r.image, sizeof(r.image), "%s", state->preview_key);
        } else if ((size_t)index == u->saved_state_count + 1) {
            fill(&r, "État manuel et automatique", "Retrouver les sauvegardes classiques", CR_ICON_RESTORE,
                 ACT_PAGE, CR_SAVESTATES);
            r.disabled = !cr_engine_can_state(&u->engine);
        } else {
            fill(&r, "Mémoire batterie (.sav / .srm)", "Importer, exporter et gérer les copies", CR_ICON_BACKUP,
                 ACT_PAGE, CR_GAME_SAVES);
            r.disabled = !u->platform.save_transfer || (!cr_engine_battery_size(&u->engine, 0) &&
                                                      !cr_engine_battery_size(&u->engine, 1));
        }
    } else if (u->page == CR_STATE_ACTIONS || u->page == CR_STATE_DELETE_CONFIRM) {
        bool available = u->selected_state >= 0 && (size_t)u->selected_state < u->saved_state_count;
        if (u->page == CR_STATE_DELETE_CONFIRM) {
            fill(&r, index ? "Annuler" : "Supprimer cet état", index ? "Conserver la sauvegarde" : "Cette action est définitive",
                 index ? CR_ICON_BACK : CR_ICON_CLOSE, index ? ACT_CORE_REMOVE_CANCEL : ACT_STATE_DELETE, 0);
            r.destructive = !index; r.disabled = !available;
        } else {
            static const char *labels[] = {"Charger", "Renommer", "Dupliquer", "Partager", "Supprimer"};
            static const int actions[] = {ACT_STATE_LOAD, ACT_STATE_RENAME, ACT_STATE_DUPLICATE, ACT_STATE_SHARE, ACT_STATE_DELETE_CONFIRM};
            static const int icons[] = {CR_ICON_PLAY, CR_ICON_FILE, CR_ICON_BACKUP, CR_ICON_FOLDER, CR_ICON_CLOSE};
            fill(&r, labels[index], "", icons[index], actions[index], 0);
            r.disabled = !available || (index == 3 && !u->platform.share_state);
            if (!index && available) r.disabled = !u->engine.handle ||
                u->engine.system != cr_registry_find(u->saved_states[u->selected_state].core_id);
            if (!index && r.disabled) snprintf(r.hint, sizeof(r.hint), "Chargez le cœur associé à cet état");
            r.destructive = index == 4;
        }
    } else if (u->page == CR_REMOVE_CORES) {
        int slot = removable_core_at(u,index);
        if (slot >= 0) fill(&r,core_name(slot),"Supprimer ce cœur téléchargé",CR_ICON_CLOSE,ACT_CORE_REMOVE_CONFIRM,slot);
    } else if (u->page == CR_REMOVE_CORE_CONFIRM) {
        char label[160];
        snprintf(label,sizeof(label),"Supprimer %.128s",u->remove_core_name);
        fill(&r,index ? "Annuler" : label,index ? "Revenir à la liste des cœurs" :
             "Jeux et sauvegardes conservés",index ? CR_ICON_BACK : CR_ICON_CLOSE,
             index ? ACT_CORE_REMOVE_CANCEL : ACT_CORE_REMOVE,0);
    } else if (u->page == CR_CORES) {
        if (index > (int)cr_registry_count()) {
            fill(&r,"Supprimer un cœur téléchargé","Conserver jeux et sauvegardes",CR_ICON_CLOSE,ACT_PAGE,CR_REMOVE_CORES);
        } else if (index == (int)cr_registry_count()) {
            fill(&r, "Téléchargeur de cœurs", "Catalogue officiel CorvusSoftware",
                 CR_ICON_DOWNLOAD, ACT_OPEN_DOWNLOADER, 0);
        } else {
            int slot = cr_registry_index_at((size_t)index);
            fill(&r, core_name(slot), system_name(slot), CR_ICON_CORE, ACT_SELECT, slot);
            r.checked = u->engine.handle && u->engine.system == slot;
            r.console = cr_registry_legacy_system(slot);
        }
    } else if (u->page == CR_CORE_DETAILS) {
        int pack_index = detail_pack(u), core_index = cr_registry_find(u->detail_core_id);
        const cr_core_descriptor *core = cr_registry_get(core_index);
        bool loaded = core && u->engine.handle && u->engine.system == core_index;
        if (!index) {
            fill(&r, core ? (loaded ? "Décharger le cœur" : "Charger le cœur") : "Installer",
                 u->installing ? "Installation en cours..." : (core ? "Disponible sur cet appareil" : "Télécharger le module vérifié"),
                 core ? CR_ICON_CORE : CR_ICON_DOWNLOAD,
                 core ? (loaded ? ACT_UNLOAD : ACT_SELECT) : ACT_INSTALL_CORE_PACK,
                 core ? core_index : pack_index);
            r.disabled = u->installing || (!core && pack_index < 0);
        } else if (index == 1) {
            fill(&r, core ? "Réinstaller" : "Actualiser", u->catalog_loading ? "Recherche en cours..." :
                 (u->core_catalog.error[0] ? u->core_catalog.error : "Catalogue des développeurs"),
                 CR_ICON_RESTART, core && pack_index >= 0 ? ACT_INSTALL_CORE_PACK : ACT_REFRESH_CATALOG, pack_index);
            r.disabled = u->installing || u->catalog_loading;
        } else if (index == 2) {
            fill(&r, "Désinstaller", "Jeux et sauvegardes conservés", CR_ICON_CLOSE, ACT_CORE_REMOVE_CONFIRM, core_index);
            r.disabled = !core || !u->platform.can_remove_core || !u->platform.remove_core ||
                !u->platform.can_remove_core(u->platform.context, core->id) || u->installing;
            r.destructive = true;
        } else fill(&r, "Bibliothèque", "Retrouver vos jeux importés", CR_ICON_LIST, ACT_LIBRARY, 0);
    } else if (u->page == CR_CORE_DOWNLOADS) {
        if (index == 0) {
            fill(&r, "Actualiser le catalogue",
                 u->catalog_loading ? "Récupération en cours..." :
                 (u->core_catalog.error[0] ? u->core_catalog.error :
                  "Rechercher les cœurs CorvusSoftware"),
                 CR_ICON_DOWNLOAD, ACT_REFRESH_CATALOG, 0);
        } else {
            const cr_core_pack *pack = &u->core_catalog.packs[index - 1];
            const cr_core_descriptor *installed = cr_registry_get(cr_registry_find(pack->core_id));
            const char *state = !installed ? "" : strcmp(installed->version, pack->version) == 0
                    ? " - Installé" : " - mise à jour";
            char hint[160];
            snprintf(hint, sizeof(hint), "Version %.56s - %.20s%s",
                     pack->version, pack->abi, state);
            fill(&r, pack->name, hint, CR_ICON_CORE, ACT_CORE_DETAILS, -index);
            r.console = !strcmp(pack->core_id, "corvus.gb") ? 0 : !strcmp(pack->core_id, "corvus.gbc") ? 1 :
                        !strcmp(pack->core_id, "corvus.gba") ? 2 : -1;
            r.checked = installed != NULL;
        }
    } else if (u->page == CR_SETTINGS) {
        const char *labels[] = {"Affichage", "Commandes", "Audio", "Système", "Sauvegardes", "RetroAchievements", "À propos"};
        const char *hints[] = {"Écran, filtrage et proportions", "Tactile, manettes et affectation", "Son et volume du jeu",
                              "Interface, thèmes et fichiers", "Copies et états automatiques", "Activation et compte", "Version, crédits et licences"};
        const int pages[] = {CR_VIDEO_SETTINGS, CR_OSD_SETTINGS, CR_AUDIO_SETTINGS, CR_SYSTEM_SETTINGS, CR_SAVE_SETTINGS, CR_RA_SETTINGS, CR_INFORMATION};
        const int icons[] = {CR_ICON_DISPLAY, CR_ICON_INPUT, CR_ICON_AUDIO, CR_ICON_SETTINGS, CR_ICON_SAVES, CR_ICON_ACHIEVEMENTS, CR_ICON_INFO};
        fill(&r, labels[index], hints[index], icons[index], ACT_PAGE, pages[index]);
    } else if (u->page == CR_SYSTEM_SETTINGS) {
        static const char *labels[] = {"Interface", "Sauvegardes", "Informations", "Importer du contenu"};
        static const char *hints[] = {"Thèmes et notifications", "Copies et états automatiques", "Application, cœur et licences", "Jeux et dossiers"};
        static const int pages[] = {CR_UI_SETTINGS, CR_SAVE_SETTINGS, CR_INFORMATION, CR_CONTENT};
        static const int icons[] = {CR_ICON_THEME, CR_ICON_SAVES, CR_ICON_INFO, CR_ICON_FOLDER};
        fill(&r, labels[index], hints[index], icons[index], ACT_PAGE, pages[index]);
    } else if (u->page == CR_RA_SETTINGS) {
        fill(&r, index ? "Compte" : "Activer RetroAchievements", index ? "Connexion et état du compte" :
             "Mode normal", CR_ICON_ACCOUNT, index ? ACT_PAGE : ACT_RA_ENABLE, CR_RA_ACCOUNT);
        if (!index) { r.toggle = true; r.checked = u->ra_enabled; }
    } else if (u->page == CR_ACHIEVEMENTS || u->page == CR_RA_ACCOUNT) {
        index += u->page == CR_RA_ACCOUNT ? 1 : 4;
        cr_cheevos_status status = {0};
        cr_cheevos_get_status(u->achievements, &status);
        if (index == 1) {
            const char *user = status.connected ? status.username : (u->ra_editing == 1 ? u->ra_preview : u->ra_username);
            fill(&r, "Identifiant", user[0] ? user : "Saisir le nom du compte", CR_ICON_ACCOUNT, status.connected || status.pending ? ACT_NONE : ACT_RA_USER, 0);
        } else if (index == 2) {
            fill(&r, "Mot de passe", u->ra_password[0] || (u->ra_editing == 2 && u->ra_preview[0])
                    ? "Saisi, jamais enregistré" : "Saisir avec le clavier Android", CR_ICON_ACCOUNT, status.connected || status.pending ? ACT_NONE : ACT_RA_PASSWORD, 0);
        } else if (index == 3) {
            fill(&r, status.connected ? "Se déconnecter" : "Se connecter", status.pending ? "Connexion en cours..." :
                    "Le jeton de session est chiffré sur cet appareil", CR_ICON_ACCOUNT, ACT_RA_CONNECT, 0);
        } else if (index == 4) {
            fill(&r, "État", u->engine.players > 1 ? "Succès indisponibles en liaison locale" :
                 (status.message[0] ? status.message : "Connectez votre compte pour activer les succès"), CR_ICON_INFO, ACT_NONE, 0);
        } else if (index == 5) {
            char hint[256];
            snprintf(hint, sizeof(hint), "%u / %u succès, %u points", status.unlocked, status.total, status.points);
            fill(&r, status.game_title[0] ? status.game_title : "Aucun jeu identifié", hint, CR_ICON_CONTENT, ACT_NONE, 0);
        } else {
            cr_cheevos_achievement achievement;
            if (cr_cheevos_get_achievement(u->achievements, (size_t)index - 6, &achievement)) {
                char hint[256];
                snprintf(hint, sizeof(hint), "%s, %u points - %.180s", achievement.unlocked ? "Déverrouillé" : "Verrouillé",
                         achievement.points, achievement.description);
                fill(&r, achievement.title, hint, CR_ICON_ACHIEVEMENTS, ACT_NONE, 0);
                r.checked = achievement.unlocked;
            }
        }
    } else if (u->page == CR_UI_SETTINGS) {
        fill(&r, index ? "Notifications" : "Thèmes", index ? "Cartes et progression des succès" : cr_theme_get(u->theme)->name,
             index ? CR_ICON_NOTIFICATIONS : CR_ICON_THEME, index ? ACT_PAGE : ACT_THEME, CR_NOTIFICATIONS_SETTINGS);
    } else if (u->page == CR_NOTIFICATIONS_SETTINGS) {
        int id = index ? SET_PROGRESS : SET_CARDS;
        fill(&r, settings[id].label, "Affichage dans le jeu", CR_ICON_NOTIFICATIONS, ACT_SETTING_TOGGLE, id);
        r.toggle = true; r.checked = setting_get(u, id) != 0;
    } else if (u->page == CR_VIDEO_SETTINGS && !index) {
        fill(&r, "Garder l’écran allumé", "Empêcher la veille pendant le jeu", CR_ICON_DISPLAY, ACT_AWAKE, 0);
        r.toggle = true; r.checked = u->awake;
    } else if (u->page == CR_AUDIO_SETTINGS && !index) {
        fill(&r, "Activer le son", "Sortie audio pendant le jeu", CR_ICON_AUDIO, ACT_AUDIO, 0);
        r.toggle = true; r.checked = u->audio;
    } else if (u->page == CR_OSD_SETTINGS) {
        fill(&r, index ? "Manettes" : "Commandes tactiles", index ? "Appareils et affectation des touches" :
             "Visibilité, opacité et disposition", index ? CR_ICON_INPUT : CR_ICON_TOUCH, ACT_PAGE,
             index ? CR_CONTROLLER_SETTINGS : CR_OVERLAY_SETTINGS);
    } else if (u->page == CR_OVERLAY_SETTINGS && !index) {
        fill(&r, "Afficher les commandes", "Commandes tactiles pendant le jeu", CR_ICON_TOUCH, ACT_SHOW_INPUT, 0);
        r.toggle = true; r.checked = u->show_input;
    } else if (u->page == CR_OVERLAY_SETTINGS && index == 2) {
        fill(&r, "Taille et position", "Ajuster ou rétablir la disposition", CR_ICON_TOUCH, ACT_PAGE, CR_OVERLAY_LAYOUT);
    } else if (u->page == CR_OVERLAY_LAYOUT && index == 2) {
        fill(&r, "Rétablir la disposition", "Taille et position automatiques", CR_ICON_RESTORE, ACT_LAYOUT_RESET, 0);
    } else if (u->page == CR_CONTROLLER_SETTINGS) {
        char name[192] = {0};
        if (index < 2) {
            char label[64]; snprintf(label, sizeof(label), "Port du joueur %d", index + 1);
            bool connected = u->platform.controller_info &&
                u->platform.controller_info(u->platform.context, (unsigned)index, name, sizeof(name));
            fill(&r, label, connected ? name : "Aucune manette connectée", CR_ICON_INPUT, ACT_NONE, 0);
        } else fill(&r, "Affectation des touches", "Profil commun aux manettes", CR_ICON_INPUT, ACT_PAGE, CR_CONTROLLER_BINDINGS);
    } else if (u->page == CR_CONTROLLER_BINDINGS) {
        if (index == 10) fill(&r, "Rétablir les touches", "Revenir au profil automatique", CR_ICON_RESTORE, ACT_BIND_RESET, 0);
        else {
            char name[96]; cr_controller_binding_name(u->controller_bindings[index], name, sizeof(name));
            fill(&r, binding_labels[index], u->controller_capture == index ? "Bouton de manette, ou Retour" : name,
                 CR_ICON_INPUT, ACT_BIND_CAPTURE, index);
        }
    } else if (u->page == CR_SAVE_SETTINGS && index == 1) {
        fill(&r, settings[SET_AUTO_STATE].label, "Créé à la fermeture si compatible", CR_ICON_SAVES, ACT_SETTING_TOGGLE, SET_AUTO_STATE);
        r.toggle = true; r.checked = u->auto_state != 0;
    } else if (u->page == CR_SETTING_VALUES) {
        const setting_spec *spec = &settings[u->editing_setting];
        int value = spec->minimum + index * spec->step;
        char label[96]; setting_label(u->editing_setting, value, label, sizeof(label));
        fill(&r, label, value == spec->fallback ? "Valeur par défaut" : "Appliquer ce réglage", setting_icon(u->editing_setting), ACT_SETTING_VALUE, value);
        r.checked = value == setting_get(u, u->editing_setting);
    } else if (u->page == CR_VIDEO_SETTINGS || u->page == CR_AUDIO_SETTINGS ||
               u->page == CR_OVERLAY_SETTINGS || u->page == CR_OVERLAY_LAYOUT || u->page == CR_SAVE_SETTINGS) {
        int id = u->page == CR_VIDEO_SETTINGS ? (index == 1 ? SET_INTEGER : SET_FILTER) :
                 u->page == CR_AUDIO_SETTINGS ? SET_VOLUME : u->page == CR_OVERLAY_SETTINGS ? SET_OPACITY :
                 u->page == CR_OVERLAY_LAYOUT ? (index ? SET_POSITION : SET_SIZE) : SET_BACKUPS;
        char value[96]; setting_label(id, setting_get(u, id), value, sizeof(value));
        fill(&r, settings[id].label, value, setting_icon(id), ACT_SETTING_VALUES, id);
    } else if (u->page == CR_INFORMATION) {
        if (index == 2 + (u->engine.handle ? 1 : 0))
            fill(&r,"Supprimer un cœur téléchargé","Conserver jeux et sauvegardes",CR_ICON_CLOSE,ACT_PAGE,CR_REMOVE_CORES);
        else if (index == 0)
            fill(&r, "À propos de CorvusRetro", "Application native de CorvusSoftware",
                 CR_ICON_INFO, ACT_ABOUT, 0);
        else if (u->engine.handle && index == 1)
            fill(&r, "Informations sur le cœur", core_name(u->engine.system), CR_ICON_CORE, ACT_PAGE,
                 CR_CORE_INFO);
        else
            fill(&r, "Licence et crédits", "AGPL-3.0-only · Code original", CR_ICON_FILE,
                 ACT_LICENSE, 0);
    } else if (u->page == CR_THEMES) {
        int id = cr_theme_at(index);
        const cr_theme *theme = cr_theme_get(id);
        fill(&r, theme->name, theme->description, CR_ICON_THEME, ACT_SET_THEME, id);
        r.checked = u->theme == id;
    } else if (u->page == CR_GAMES) {
        int n = 0;
        for (size_t i = 0; i < u->game_count; i++)
            if (u->filter < 0 ? u->games[i].played > 0 : u->games[i].system == u->filter) {
                if (n++ == index) {
                    fill(&r, u->games[i].title, core_name(u->games[i].system), CR_ICON_FILE, ACT_GAME,
                         (int)i);
                    r.disabled = !u->engine.handle || u->engine.system != u->games[i].system;
                    if (!u->engine.handle)
                        snprintf(r.hint, sizeof(r.hint), "Chargez un cœur avant de lancer ce jeu");
                    else if (u->engine.system != u->games[i].system)
                        snprintf(r.hint, sizeof(r.hint), "Chargez le cœur associé : %.180s", core_name(u->games[i].system));
                    break;
                }
            }
    }
    return r;
}
static void clamp_scroll(cr_ui *u) {
    float content = cr_interface_content_height(u, rows(u));
    if (u->page == CR_ABOUT || u->page == CR_LICENSE || u->page == CR_CORE_INFO ||
        ((u->page == CR_SUBSYSTEMS || u->page == CR_CORE_OPTIONS) && !rows(u)))
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
static void page(cr_ui *u, int p) {
    u->depth = 0;
    view(u, p);
}
static void push_page(cr_ui *u, int p) {
    if (u->depth >= sizeof(u->history) / sizeof(u->history[0]))
        return;
    u->history[u->depth++] = (cr_navigation){
        .page = u->page, .focus = u->focus, .filter = u->filter, .tab = u->tab,
        .scroll = u->scroll, .quick_scroll = u->quick_scroll, .quick = u->quick
    };
    view(u, p);
}
static bool pop_page(cr_ui *u) {
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
    int count = rows(u);
    u->focus = count > 0 ? (u->focus < count ? u->focus : count - 1) : 0;
    if (u->quick) quick_clamp(u);
    else clamp_scroll(u);
    return true;
}
static void refresh_saved_states(cr_ui *u) {
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
    refresh_saved_states(u);
    push_page(u, CR_STATE_LIST);
}
static void open_core_details(cr_ui *u, int index) {
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
    push_page(u, CR_CORE_DETAILS);
    if (!u->core_catalog.count && !u->catalog_loading && u->platform.refresh_core_catalog) {
        u->catalog_loading = true;
        u->platform.refresh_core_catalog(u->platform.context);
    }
}
void cr_ui_message(cr_ui *u, const char *s) {
    snprintf(u->message, sizeof(u->message), "%s", s ? s : "");
    u->message_time = 5;
}
void cr_ui_catalog(cr_ui *u, const cr_core_catalog *catalog) {
    if (!catalog) return;
    u->core_catalog = *catalog;
    u->catalog_loading = false;
    int count = rows(u);
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
            int game = game_index(u, u->engine.id);
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
static void achievement_sync(cr_ui *u, bool identify) {
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
    achievement_sync(u, false);
    if (u->ra_enabled && u->engine.players <= 1) cr_cheevos_frame(u->achievements);
}
void cr_ui_text_result(cr_ui *u, int field, const char *text, bool accepted) {
    if (u->ui_editing == 3 && (field == 3 || field == -3)) {
        snprintf(u->library_query, sizeof(u->library_query), "%.127s", field == 3 && !accepted ? u->library_original_query : text);
        u->scroll = 0; u->focus = library_count(u) ? 1 : 0;
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
                pop_page(u);
                refresh_saved_states(u);
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
    for (int i = 0; i < SET_COUNT; ++i) {
        const setting_spec *spec = &settings[i];
        int value = p.preference ? p.preference(p.context, spec->key, spec->fallback) : spec->fallback;
        *setting_value(u, i) = value < spec->minimum || value > spec->maximum ||
            (value - spec->minimum) % spec->step ? spec->fallback : value;
    }
    cr_engine_set_save_policy(&u->engine, (unsigned)u->backup_count, u->auto_state != 0);
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
    finish_save_transfer(u);
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
    achievement_sync(u, false);
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
                else if (!u->quick && p->target >= 0 && p->target < rows(u))
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
    refresh_saved_states(u);
    bool used[CR_STATE_STORE_LIMIT + 1] = {false};
    for (size_t i = 0; i < u->saved_state_count; ++i)
        if (u->saved_states[i].slot <= CR_STATE_STORE_LIMIT) used[u->saved_states[i].slot] = true;
    unsigned number = 1;
    while (number <= CR_STATE_STORE_LIMIT && used[number]) ++number;
    char name[96]; snprintf(name, sizeof(name), "État %u", number);
    cr_state_entry created;
    if (!result(u, cr_state_store_create(&u->engine, name, &created))) {
        refresh_saved_states(u);
        cr_ui_message(u, u->engine.error);
        return;
    }
    if (u->page != CR_STATE_LIST) push_page(u, CR_STATE_LIST);
    refresh_saved_states(u);
    for (size_t i = 0; i < u->saved_state_count; ++i)
        if (u->saved_states[i].slot == created.slot) u->focus = (int)i + 1;
    cr_ui_move(u, 0);
    cr_ui_message(u, "État sauvegardé");
}
static bool selected_state_entry(const cr_ui *u, cr_state_entry *entry) {
    if (u->selected_state < 0 || (size_t)u->selected_state >= u->saved_state_count) return false;
    *entry = u->saved_states[u->selected_state];
    return true;
}
static void load_named_state(cr_ui *u, const cr_state_entry *entry) {
    if (state_busy(u)) return;
    int core = cr_registry_find(entry->core_id);
    if (!u->engine.handle || u->engine.system != core) {
        cr_ui_message(u, "Chargez le cœur associé à cet état"); return;
    }
    int game = game_index(u, entry->game_id), second = game_index(u, entry->second_id);
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
        achievement_sync(u, false);
        snprintf(u->title, sizeof(u->title), "%s", title);
    }
    if (!result(u, cr_state_store_load(&u->engine, entry))) return;
    cr_cheevos_reset(u->achievements);
    achievement_sync(u, true);
    u->notifications_loaded = true;
    page(u, CR_PLAY);
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
    achievement_sync(u, true);
    u->notifications_loaded = true;
    snprintf(u->title, sizeof(u->title), "%s", title);
    page(u, CR_PLAY);
    if (u->platform.touch)
        u->platform.touch(u->platform.context, id);
    cr_ui_reload(u);
}
static void start_subsystem(cr_ui *u) {
    corvus_subsystem_info subsystem;
    if (!selected_subsystem(u, &subsystem) || !u->link_ids[0][0] || !u->link_ids[1][0]) {
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
    achievement_sync(u, false);
    u->notifications_loaded = true;
    snprintf(u->title, sizeof(u->title), "%.230s + %.230s", u->link_titles[0], u->link_titles[1]);
    page(u, CR_PLAY);
    if (u->platform.touch)
        for (unsigned i = 0; i < 2; ++i) u->platform.touch(u->platform.context, ids[i]);
    cr_ui_reload(u);
}
static void show_core_downloader(cr_ui *u) {
    push_page(u, CR_CORE_DOWNLOADS);
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
static atomic_uint_fast64_t save_nonce = ATOMIC_VAR_INIT(0);
static bool save_current(const cr_ui *u) {
    return u->save_request && u->engine.loaded && u->engine.handle == u->save_handle &&
        u->core_revision == u->save_revision && u->save_player < u->engine.players &&
        !strcmp(u->engine.id, u->save_id) && !strcmp(u->engine.second_id, u->save_second_id);
}
static void finish_save_transfer(cr_ui *u) {
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
        finish_save_transfer(u); cr_ui_message(u, "Service de sauvegarde indisponible");
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
        finish_save_transfer(u); return;
    }
    if (u->save_operation != CR_SAVE_IMPORT) {
        finish_save_transfer(u); cr_ui_message(u, "Sauvegarde exportée"); return;
    }
    char expected_path[4096];
    int n = snprintf(expected_path, sizeof(expected_path), "%s/save-transfer/%llu.sav",
                     u->engine.root, (unsigned long long)u->save_request);
    if (n <= 0 || n >= (int)sizeof(expected_path) || strcmp(item->path, expected_path)) {
        finish_save_transfer(u); cr_ui_message(u, "Fichier de sauvegarde invalide"); return;
    }
    snprintf(u->save_path, sizeof(u->save_path), "%s", item->path);
    push_page(u, CR_SAVE_CONFIRM);
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
static void activate(cr_ui *u, row r) {
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
        open_core_details(u, r.value);
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
            push_page(u, CR_STATE_ACTIONS);
        }
        break;
    case ACT_STATE_LOAD: {
        cr_state_entry entry;
        if (selected_state_entry(u, &entry)) load_named_state(u, &entry);
        break;
    }
    case ACT_STATE_RENAME: {
        cr_state_entry entry;
        if (u->platform.edit_text && selected_state_entry(u, &entry)) {
            snprintf(u->state_rename, sizeof(u->state_rename), "%s", entry.name);
            u->ui_editing = 4;
            u->platform.edit_text(u->platform.context, 4, u->state_rename, false);
        }
        break;
    }
    case ACT_STATE_DUPLICATE: {
        cr_state_entry entry, created; char error[256] = {0};
        if (selected_state_entry(u, &entry)) {
            bool ok = cr_state_store_duplicate(u->engine.root, &entry, &created, error, sizeof(error));
            pop_page(u); refresh_saved_states(u);
            cr_ui_message(u, ok ? "État dupliqué" : error);
        }
        break;
    }
    case ACT_STATE_SHARE: {
        cr_state_entry entry; char error[256] = {0}; uint8_t *bytes = NULL; size_t size = 0;
        if (u->platform.share_state && selected_state_entry(u, &entry)) {
            if (cr_state_store_read(u->engine.root, &entry, &bytes, &size, error, sizeof(error))) {
                u->platform.share_state(u->platform.context, entry.name, bytes, size);
                cr_ui_message(u, "Ouverture du partage...");
            } else cr_ui_message(u, error);
        }
        free(bytes);
        break;
    }
    case ACT_STATE_DELETE_CONFIRM:
        push_page(u, CR_STATE_DELETE_CONFIRM);
        break;
    case ACT_STATE_DELETE: {
        cr_state_entry entry; char error[256] = {0};
        if (selected_state_entry(u, &entry)) {
            bool ok = cr_state_store_delete(u->engine.root, &entry, error, sizeof(error));
            pop_page(u); pop_page(u); refresh_saved_states(u);
            cr_ui_message(u, ok ? "État supprimé" : error);
        }
        break;
    }
    case ACT_CORE_REMOVE_CANCEL:
        pop_page(u); break;
    case ACT_CORE_REMOVE_CONFIRM: {
        const cr_core_descriptor *core = cr_registry_get(r.value);
        if (!core || !u->platform.can_remove_core || !u->platform.remove_core ||
            !u->platform.can_remove_core(u->platform.context,core->id)) break;
        snprintf(u->remove_core_id,sizeof(u->remove_core_id),"%s",core->id);
        snprintf(u->remove_core_name,sizeof(u->remove_core_name),"%s",core->name);
        push_page(u,CR_REMOVE_CORE_CONFIRM); break;
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
        page(u,CR_CORES);
        u->platform.remove_core(u->platform.context,u->remove_core_id);
        cr_ui_message(u,"Suppression du cœur..."); break;
    }
    case ACT_SETTING_VALUES: {
        u->editing_setting = r.value;
        push_page(u, CR_SETTING_VALUES);
        const setting_spec *spec = &settings[r.value];
        u->focus = (setting_get(u, r.value) - spec->minimum) / spec->step;
        cr_ui_move(u, 0);
        break;
    }
    case ACT_SETTING_VALUE:
        apply_setting(u, u->editing_setting, r.value);
        pop_page(u);
        break;
    case ACT_SETTING_TOGGLE:
        apply_setting(u, r.value, !setting_get(u, r.value));
        break;
    case ACT_LAYOUT_RESET:
        apply_setting(u, SET_SIZE, 100); apply_setting(u, SET_POSITION, 0);
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
            cr_cheevos_reset(u->achievements); page(u, CR_PLAY);
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
        push_page(u, CR_GAME_BACKUPS);
        break;
    case ACT_EXPORT_BACKUP:
        if (!r.disabled) begin_save_transfer(u, CR_SAVE_EXPORT_SAV, r.value);
        break;
    case ACT_SAVE_CANCEL:
        finish_save_transfer(u); pop_page(u); cr_ui_message(u, "Importation annulée");
        break;
    case ACT_SAVE_COMMIT: {
        bool ok = import_save_file(u);
        finish_save_transfer(u);
        if (ok) {
            ++u->core_revision; cr_cheevos_reset(u->achievements);
            achievement_sync(u, true); page(u, CR_PLAY);
            cr_ui_message(u, "Sauvegarde importée : contenu redémarré");
        } else pop_page(u);
        break;
    }
    case ACT_CORE_OPTION: {
        corvus_core_option option;
        if (cr_engine_option(&u->engine, (size_t)r.value, &option)) {
            snprintf(u->option_key, sizeof(u->option_key), "%s", option.key);
            push_page(u, CR_CORE_OPTION_VALUES);
            u->focus = (int)cr_engine_option_value(&u->engine, option.key);
            if (u->focus < 0 || (uint32_t)u->focus >= option.value_count) u->focus = 0;
            cr_ui_move(u, 0);
        }
        break;
    }
    case ACT_OPTION_VALUE: {
        corvus_core_option option;
        if (selected_option(u, &option) &&
            result(u, cr_engine_set_option(&u->engine, option.key, (uint32_t)r.value))) {
            bool restart = (option.flags & CORVUS_OPTION_RESTART_REQUIRED) != 0;
            pop_page(u);
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
            push_page(u, CR_SUBSYSTEM_CONFIG);
        }
        break;
    }
    case ACT_SUBSYSTEM_SLOT:
        if (r.value >= 0 && r.value < 2) {
            u->link_slot = r.value;
            push_page(u, CR_SUBSYSTEM_CONTENTS);
        }
        break;
    case ACT_SUBSYSTEM_GAME:
        if (r.value >= 0 && link_game(u, (size_t)r.value)) {
            const cr_game *game = &u->games[r.value];
            snprintf(u->link_ids[u->link_slot], sizeof(u->link_ids[0]), "%s", game->id);
            snprintf(u->link_titles[u->link_slot], sizeof(u->link_titles[0]), "%s", game->title);
            pop_page(u);
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
        quick_action(u, r.value);
        break;
    case ACT_SAVE_STATE:
    case ACT_LOAD_STATE: {
        bool load = r.action == ACT_LOAD_STATE;
        if (result(u, load ? cr_engine_load_state(&u->engine) : cr_engine_save_state(&u->engine))) {
            if (load) cr_cheevos_reset(u->achievements);
            page(u, CR_PLAY);
            cr_ui_message(u, load ? "État chargé" : "État sauvegardé");
        }
        break;
    }
    case ACT_RA_ENABLE:
        u->ra_enabled = !u->ra_enabled;
        if (u->platform.set_preference) u->platform.set_preference(u->platform.context, "achievements_enabled", u->ra_enabled);
        if (!u->ra_enabled) cr_cheevos_logout(u->achievements);
        achievement_sync(u, true);
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
            achievement_sync(u, true);
        }
        if (u->platform.set_preference) u->platform.set_preference(u->platform.context, "achievements_enabled", u->ra_enabled);
        break;
    }
    case ACT_CORES:
        push_page(u, CR_CORES);
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
            page(u, CR_MAIN);
        }
        break;
    case ACT_LIBRARY:
        if (r.value == 1 && u->page == CR_LIBRARY_ACTIONS) {
            u->library_list = !u->library_list;
            pop_page(u); u->scroll = 0; u->focus = library_count(u) ? 1 : 0;
        } else push_page(u, CR_LIBRARY);
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
        push_page(u, CR_GAMES);
        u->filter = -1;
        break;
    case ACT_ABOUT:
        push_page(u, CR_ABOUT);
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
            page(u, CR_MAIN);
        }
        break;
    case ACT_THEME:
        push_page(u, CR_THEMES);
        break;
    case ACT_SET_THEME:
        u->theme = cr_theme_valid(r.value);
        if (u->platform.set_preference)
            u->platform.set_preference(u->platform.context, "theme", u->theme);
        if (!pop_page(u))
            page(u, CR_UI_SETTINGS);
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
        push_page(u, CR_LICENSE);
        break;
    case ACT_QUICK:
        page(u, CR_PLAY);
        u->quick = true;
        break;
    case ACT_PAGE:
        if (r.value == CR_SAVESTATES) u->auto_state_available = cr_engine_has_auto_state(&u->engine);
        if (r.value == CR_GAME_SAVES) u->save_player = 0;
        push_page(u, r.value);
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
        pop_page(u);
        return;
    }
    char path[4256];
    snprintf(path, sizeof(path), "%s/roms/%s.rom", u->engine.root, resolved.id);
    open_game(u, resolved.system, resolved.id, resolved.title, path);
}
static void quick_action(cr_ui *u, int action) {
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
        page(u, CR_PLAY);
        break;
    case CR_QUICK_RESTART:
        if (result(u, cr_engine_reset(&u->engine))) {
            page(u, CR_PLAY);
            cr_cheevos_reset(u->achievements);
            cr_ui_message(u, "Contenu redémarré");
        }
        break;
    case CR_QUICK_CLOSE:
        if (result(u, cr_engine_unload_game(&u->engine))) {
            achievement_sync(u, false);
            u->title[0] = 0;
            u->input_player = 0;
            page(u, CR_MAIN);
        }
        break;
    case CR_QUICK_GAME_SAVES:
        if (u->platform.save_transfer && (cr_engine_battery_size(&u->engine, 0) ||
                                        cr_engine_battery_size(&u->engine, 1))) {
            u->save_player = 0; push_page(u, CR_GAME_SAVES);
        }
        break;
    case CR_QUICK_STATES:
        u->auto_state_available = cr_engine_has_auto_state(&u->engine);
        push_page(u, CR_SAVESTATES); break;
    case CR_QUICK_CORE_OPTIONS: push_page(u, CR_CORE_OPTIONS); break;
    case CR_QUICK_CONTROLS: push_page(u, CR_OVERLAY_SETTINGS); break;
    case CR_QUICK_ACHIEVEMENTS: push_page(u, CR_ACHIEVEMENTS); break;
    case CR_QUICK_CORE_INFO: push_page(u, CR_CORE_INFO); break;
    case CR_QUICK_MAIN: page(u, CR_MAIN); break;
    case CR_QUICK_PLAYER:
        if (u->engine.players > 1) u->input_player = u->input_player == 0 ? 1 : 0;
        break;
    default: break;
    }
    cr_ui_clear_input(u);
}
void cr_ui_back(cr_ui *u) {
    if (u->page == CR_LIBRARY && u->library_search) {
        u->ui_editing = 0;
        if (u->platform.edit_text) u->platform.edit_text(u->platform.context, 0, "", false);
        u->library_search = false;
        u->library_query[0] = 0;
        u->scroll = 0; u->focus = library_count(u) ? 1 : 0;
        return;
    }
    if (u->controller_capture >= 0) {
        u->controller_capture = -1;
        cr_ui_clear_input(u); cr_ui_message(u, "Affectation annulée"); return;
    }
    cr_ui_clear_input(u);
    if (u->save_request) {
        finish_save_transfer(u);
        cr_ui_message(u, "Transfert annulé");
    }
    if (u->page == CR_PLAY) {
        u->quick = !u->quick;
        u->focus = 0;
        u->quick_scroll = 0;
        return;
    }
    if (pop_page(u))
        return;
    if (u->page == CR_GAMES) {
        page(u, CR_LISTS);
        u->focus = u->filter < 0 ? 0 : u->filter + 1;
        return;
    }
    if (u->page == CR_THEMES || (u->page >= CR_UI_SETTINGS && u->page <= CR_OVERLAY_SETTINGS)) {
        page(u, CR_SETTINGS);
        return;
    }
    if (u->page != CR_MAIN) {
        page(u, CR_MAIN);
        return;
    }
    if (u->engine.loaded) {
        if (u->import_folder) {
            cr_ui_message(u, "Importation du dossier en cours");
            return;
        }
        page(u, CR_PLAY);
        return;
    }
    if (u->platform.quit)
        u->platform.quit(u->platform.context);
}
void cr_ui_confirm(cr_ui *u) {
    if (u->quick && u->focus >= 0 && u->focus < rows(u))
        quick_action(u, get_row(u, u->focus).value);
    else if (u->page != CR_PLAY && u->focus >= 0 && u->focus < rows(u))
        activate(u, get_row(u, u->focus));
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
    int count = rows(u);
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
        cr_interface_content_height(u, rows(u)) - fmaxf(1, body_bottom(u) - cr_interface_content_top(u))));
}
static int target(const cr_ui *u, float x, float y) {
    if (x < 0 || y < 0 || x >= u->width || y >= u->height)
        return -1;
    if (u->page == CR_PLAY && !u->quick) {
        cr_play_layout layout = play_layout(u);
        return cr_area_contains(layout.menu, x, y) ? -3 :
               cr_area_contains(layout.hide, x, y) ? -5 : -1;
    }
    return cr_interface_target(u, x, y, rows(u), has_back(u));
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
            else if (!u->quick && t < rows(u)) {
                u->focus = t;
                if (event_time - p->started >= 1) {
                    row r = get_row(u, t);
                    if (!r.checked && (r.action == ACT_AUDIO || r.action == ACT_AWAKE ||
                                       r.action == ACT_SHOW_INPUT))
                        activate(u, r);
                }
            }
            return;
        }
        if (t >= 10000) {
            quick_action(u, get_row(u, t - 10000).value);
            return;
        }
        if (t == -5) {
            row r = {0};
            r.action = ACT_SHOW_INPUT;
            activate(u, r);
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
            if (index >= rows(u)) return;
            row item = get_row(u, index);
            if (u->page == CR_CORES && item.action == ACT_SELECT) open_core_details(u, item.value);
            else if (u->page == CR_LIBRARY && item.action == ACT_GAME && item.value >= 0 && (size_t)item.value < u->game_count) {
                snprintf(u->states_game_id, sizeof(u->states_game_id), "%s", u->games[item.value].id);
                push_page(u, CR_GAME_ACTIONS);
            } else if (item.action == ACT_STATE_ACTIONS || item.action == ACT_STATE_GAME) activate(u, item);
            return;
        }
        if (t == -200 && u->page == CR_LIBRARY) {
            u->library_search = true;
            if (u->ui_editing != 3) snprintf(u->library_original_query, sizeof(u->library_original_query), "%s", u->library_query);
            u->ui_editing = 3;
            if (u->platform.edit_text) u->platform.edit_text(u->platform.context, 3, u->library_query, false);
            return;
        }
        if (t == -201 && u->page == CR_LIBRARY) { push_page(u, CR_LIBRARY_ACTIONS); return; }
        if (t == -202 && u->page == CR_LIBRARY) {
            u->ui_editing = 0;
            if (u->platform.edit_text) u->platform.edit_text(u->platform.context, 0, "", false);
            u->library_query[0] = 0; u->library_search = false; u->scroll = 0;
            return;
        }
        if (t <= -210 && t >= -213 && u->page == CR_LIBRARY) {
            u->library_filter = -t - 211; u->scroll = 0; u->focus = library_count(u) ? 1 : 0;
            return;
        }
        if (t == -300) { push_page(u, CR_SETTINGS); return; }
        if (t <= -100 && t >= -104) {
            static const int pages[] = {CR_MAIN, CR_LIBRARY, CR_SAVES_LIBRARY, CR_CORES, CR_SETTINGS};
            cr_ui_reload(u);
            page(u, pages[-t - 100]);
            return;
        }
        if (t >= 0) {
            u->focus = t;
            activate(u, get_row(u, t));
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
static row get_visual_row(const cr_ui *u, int index) {
    if (index >= 0) return get_row(u, index);
    row r; fill(&r, "", "", CR_ICON_CONTENT, ACT_NONE, 0);
    if (u->page == CR_CORE_DETAILS) {
        int pack_index = detail_pack(u), core_index = cr_registry_find(u->detail_core_id);
        const cr_core_descriptor *core = cr_registry_get(core_index);
        const cr_core_pack *pack = pack_index >= 0 ? &u->core_catalog.packs[pack_index] : NULL;
        snprintf(r.label, sizeof(r.label), "%s", core ? core->name : pack ? pack->name : "Cœur indisponible");
        const char *system = core ? core->system_name : pack ? pack->system_name : "";
        const char *version = core ? core->version : pack ? pack->version : "?";
        if (pack && pack->size) snprintf(r.hint, sizeof(r.hint), "%.95s\nVersion %.55s · %.1f Mo\n%.60s",
            system, version, pack->size / 1048576.0, pack->description);
        else snprintf(r.hint, sizeof(r.hint), "%.110s\nVersion %.55s", system, version);
        r.checked = core != NULL;
        r.console = !strcmp(u->detail_core_id, "corvus.gb") ? 0 : !strcmp(u->detail_core_id, "corvus.gbc") ? 1 :
                    !strcmp(u->detail_core_id, "corvus.gba") ? 2 : -1;
    } else if (u->page == CR_STATE_ACTIONS || u->page == CR_STATE_DELETE_CONFIRM) {
        cr_state_entry entry;
        r.value = 1 + (int)u->saved_state_count +
            (u->engine.loaded && !strcmp(u->engine.id, u->states_game_id) ? 2 : 0);
        if (selected_state_entry(u, &entry)) {
            snprintf(r.label, sizeof(r.label), "%s", u->ui_editing == 4 ? u->state_rename : entry.name);
            time_t stamp = (time_t)(entry.created_ms / 1000); struct tm local;
            if (localtime_r(&stamp, &local)) strftime(r.hint, sizeof(r.hint), "%d/%m/%Y · %H:%M", &local);
            snprintf(r.image, sizeof(r.image), "%s", entry.preview_key);
        }
    } else if (u->page == CR_STATE_LIST || u->page == CR_GAME_ACTIONS) {
        int game = game_index(u, u->states_game_id);
        if (game >= 0) game_row(&r, u, game, ACT_NONE);
        else snprintf(r.label, sizeof(r.label), "Sauvegardes");
        if (u->page == CR_STATE_LIST) snprintf(r.hint, sizeof(r.hint), "%zu état%s enregistré%s", u->saved_state_count,
            u->saved_state_count > 1 ? "s" : "", u->saved_state_count > 1 ? "s" : "");
    }
    return r;
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
            title = selected_option(u, &option) ? option.label : "Options du cœur";
            break;
        }
        case CR_SUBSYSTEM_CONFIG: {
            corvus_subsystem_info subsystem;
            title = selected_subsystem(u, &subsystem) ? subsystem.label : "Sous-systèmes";
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
            title = u->filter < 0 ? "Historique" : system_name(u->filter);
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
            title = settings[u->editing_setting].label;
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
        int count = rows(u);
        cr_interface_draw(u, p, title, has_back(u), count, get_visual_row);
        if (p->clip) p->clip(p->context, 0, 56, w, fmaxf(1, body_bottom(u) - 56));
        if (u->page == CR_ABOUT)
            u->text_height = paragraphs(
                p,
                "CorvusRetro 0.22.2\n\nFrontend original en C, rendu OpenGL ES et audio "
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
