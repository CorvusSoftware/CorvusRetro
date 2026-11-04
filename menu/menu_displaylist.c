// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
#define _POSIX_C_SOURCE 200809L
#include "menu_entries.h"
#include "menu_cbs.h"
#include "menu_setting.h"
#include "core_registry.h"
#include "state_store.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

typedef cr_menu_row row;
static const char *binding_labels[10] = {"Haut", "Bas", "Gauche", "Droite", "A", "B", "Start", "Select", "L", "R"};
static const int quick_order[CR_QUICK_COUNT] = {
    CR_QUICK_RESUME, CR_QUICK_LOAD_STATE, CR_QUICK_SAVE_STATE, CR_QUICK_RESTART,
    CR_QUICK_CORE_OPTIONS, CR_QUICK_CLOSE, CR_QUICK_STATES,
    CR_QUICK_GAME_SAVES, CR_QUICK_CONTROLS, CR_QUICK_PLAYER,
    CR_QUICK_ACHIEVEMENTS, CR_QUICK_CORE_INFO, CR_QUICK_MAIN
};
static const char *core_name(int index) {
    const cr_core_descriptor *core = cr_registry_get(index);
    return core ? core->name : "Cœur indisponible";
}
const char *cr_menu_system_name(int index) {
    const cr_core_descriptor *core = cr_registry_get(index);
    return core ? core->system_name : "Système indisponible";
}
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
bool cr_menu_selected_option(const cr_ui *u, corvus_core_option *out) {
    for (size_t i = 0; i < cr_engine_option_count(&u->engine); ++i) {
        corvus_core_option option;
        if (cr_engine_option(&u->engine, i, &option) && !strcmp(option.key, u->option_key)) {
            *out = option;
            return true;
        }
    }
    return false;
}
bool cr_menu_selected_subsystem(const cr_ui *u, corvus_subsystem_info *out) {
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
bool cr_menu_link_game(const cr_ui *u, size_t index) {
    return u->engine.handle && index < u->game_count &&
           u->games[index].system == u->engine.system;
}
static int game_console(const cr_game *g) {
    static const char *ids[] = {"nintendo.game_boy", "nintendo.game_boy_color", "nintendo.game_boy_advance"};
    static const char *cores[] = {"corvus.gb", "corvus.gbc", "corvus.gba"};
    for (int i = 0; i < 3; ++i)
        if (!strcmp(g->system_id, ids[i]) || (!g->system_id[0] && !strcmp(g->core_id, cores[i]))) return i;
    return -1;
}
int cr_menu_game_index(const cr_ui *u, const char *id) {
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
int cr_menu_library_count(const cr_ui *u) {
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
int cr_menu_entry_count(const cr_ui *u) {
    if (u->quick) return CR_QUICK_COUNT - (u->engine.players > 1 ? 0 : 1);
    if (u->page == CR_CORE_OPTIONS) return (int)cr_engine_option_count(&u->engine);
    if (u->page == CR_SUBSYSTEMS) return (int)cr_engine_subsystem_count(&u->engine);
    if (u->page == CR_CORE_OPTION_VALUES) {
        corvus_core_option option;
        return cr_menu_selected_option(u, &option) ? (int)option.value_count : 0;
    }
    if (u->page == CR_SUBSYSTEM_CONFIG) {
        corvus_subsystem_info subsystem;
        return cr_menu_selected_subsystem(u, &subsystem) ? 3 : 0;
    }
    if (u->page == CR_SUBSYSTEM_CONTENTS) {
        int count = 1;
        for (size_t i = 0; i < u->game_count; ++i) if (cr_menu_link_game(u, i)) ++count;
        return count;
    }
    if (u->page == CR_GAME_SAVES) return 4 + (u->engine.players > 1);
    if (u->page == CR_GAME_BACKUPS) return u->backup_count;
    if (u->page == CR_SAVE_CONFIRM) return 2;
    if (u->page == CR_SAVESTATES) return 3;
    if (u->page == CR_CONTENT) return 2;
    if (u->page == CR_LIBRARY) return 1 + cr_menu_library_count(u);
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
        const cr_menu_setting_spec *spec = &cr_menu_settings[u->editing_setting];
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
row cr_menu_entry_get(const cr_ui *u, int index) {
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
            CR_ICON_PLAY, CR_ICON_RESTART, CR_ICON_EXIT, CR_ICON_SAVES,
            CR_ICON_SLIDERS, CR_ICON_INPUT, CR_ICON_ACHIEVEMENTS, CR_ICON_INFO,
            CR_ICON_HOME, CR_ICON_INPUT, CR_ICON_SAVES, CR_ICON_IMPORT, CR_ICON_SAVES
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
                 "Valeur indisponible", CR_ICON_SLIDERS, ACT_CORE_OPTION, index);
        }
    } else if (u->page == CR_CORE_OPTION_VALUES) {
        corvus_core_option option;
        if (cr_menu_selected_option(u, &option) && (uint32_t)index < option.value_count) {
            fill(&r, option.values[index].label, (uint32_t)index == option.default_index ?
                 "Valeur par défaut" : (option.flags & CORVUS_OPTION_RESTART_REQUIRED ?
                 "Application au prochain redémarrage" : "Application immédiate"),
                 CR_ICON_SLIDERS, ACT_OPTION_VALUE, index);
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
        if (cr_menu_selected_subsystem(u, &subsystem)) {
            if (index < 2)
                fill(&r, subsystem.games[index].label, u->link_ids[index][0] ? u->link_titles[index] :
                     "Choisir un contenu", CR_ICON_CONTENT, ACT_SUBSYSTEM_SLOT, index);
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
                if (cr_menu_link_game(u, i) && n++ == index) {
                    fill(&r, u->games[i].title, cr_menu_system_name(u->engine.system), CR_ICON_CONTENT,
                         ACT_SUBSYSTEM_GAME, (int)i);
                    r.checked = !strcmp(u->link_ids[u->link_slot], u->games[i].id);
                    break;
                }
        }
    } else if (u->page == CR_GAME_SAVES) {
        if (u->engine.players > 1 && index == 0) {
            fill(&r, "Joueur de la sauvegarde", u->save_player ? "Joueur 2" : "Joueur 1",
                 CR_ICON_ACCOUNT, ACT_SAVE_PLAYER, 0);
        } else if (index - (u->engine.players > 1) == 3) {
            fill(&r, "Copies de secours", "Exporter une batterie avant import", CR_ICON_BACKUP, ACT_BACKUP_MENU, 0);
        } else {
            int operation = index - (u->engine.players > 1);
            static const char *labels[] = {"Importer une sauvegarde", "Exporter en .sav", "Exporter en .srm"};
            fill(&r, labels[operation], operation ? "Copie de la mémoire batterie actuelle" :
                 "Le contenu redémarrera après confirmation", operation ? CR_ICON_EXPORT : CR_ICON_IMPORT, ACT_SAVE_TRANSFER, operation);
            r.disabled = u->save_request || !u->platform.save_transfer ||
                         !cr_engine_battery_size(&u->engine, u->save_player);
        }
    } else if (u->page == CR_GAME_BACKUPS) {
        char label[64]; snprintf(label, sizeof(label), "Copie de secours %d", index + 1);
        bool available = (u->backup_available & (1u << index)) != 0;
        fill(&r, label, available ? "Exporter en .sav" : "Absente ou illisible pour ce joueur",
             CR_ICON_EXPORT, ACT_EXPORT_BACKUP, index);
        r.disabled = !available;
    } else if (u->page == CR_SAVE_CONFIRM) {
        fill(&r, index ? "Annuler" : "Importer et redémarrer le contenu",
             index ? "Conserver la partie actuelle" : "Copie de secours conservée", index ? CR_ICON_BACK : CR_ICON_IMPORT,
             index ? ACT_SAVE_CANCEL : ACT_SAVE_COMMIT, 0);
    } else if (u->page == CR_SAVESTATES) {
        static const char *labels[] = {"Sauvegarder l’état", "Charger l’état", "Charger l'état automatique"};
        fill(&r, labels[index], index == 2 ? "Dernier état créé à la fermeture" :
             (index ? "Restaurer la sauvegarde de ce contenu" : "Enregistrer la position actuelle"),
             index == 0 ? CR_ICON_SAVES : index == 1 ? CR_ICON_IMPORT : CR_ICON_RESTORE, index == 2 ? ACT_LOAD_AUTO_STATE : (index ? ACT_LOAD_STATE : ACT_SAVE_STATE), 0);
        if (index == 2) r.disabled = !u->auto_state_available;
    } else if (u->page == CR_MAIN) {
        if (!index) {
            int recent = -1;
            for (size_t i = 0; i < u->game_count; ++i)
                if (u->games[i].played && (recent < 0 || u->games[i].played > u->games[recent].played)) recent = (int)i;
            if (u->engine.loaded) {
                int current = cr_menu_game_index(u, u->engine.id);
                if (current >= 0) game_row(&r, u, current, ACT_QUICK);
                else {
                    fill(&r, "", "", CR_ICON_CONTENT, ACT_QUICK, 0);
                    r.console = cr_registry_legacy_system(u->engine.system);
                }
                snprintf(r.label, sizeof(r.label), "Continuer");
                snprintf(r.hint, sizeof(r.hint), "%.220s", u->title);
            }
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
                 u->engine.handle ? CR_ICON_UNLOAD : CR_ICON_CORE, u->engine.handle ? ACT_UNLOAD : ACT_CORES, 0);
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
                 CR_ICON_GRID, ACT_LIBRARY, 0);
    } else if (u->page == CR_LIBRARY) {
        if (!index) {
            fill(&r, "Importer et organiser", "Jeux, dossiers et historique", CR_ICON_FOLDER, ACT_PAGE, CR_LIBRARY_ACTIONS);
        } else game_row(&r, u, library_game(u, index - 1), ACT_GAME);
    } else if (u->page == CR_LIBRARY_ACTIONS) {
        if (!index) { fill(&r, "Importer un jeu", u->engine.handle ? "Ouvrir un fichier compatible" : "Chargez un cœur pour importer", CR_ICON_CONTENT, ACT_PICK, 0); r.disabled = !u->engine.handle || u->importing; }
        else if (index == 1) { fill(&r, "Importer un dossier", u->engine.handle ? "Ajouter plusieurs jeux" : "Chargez un cœur pour importer", CR_ICON_FOLDER, ACT_FOLDER, 0); r.disabled = !u->engine.handle || u->importing; }
        else if (index == 2) fill(&r, "Historique", "Retrouver les dernières parties", CR_ICON_HISTORY, ACT_HISTORY, 0);
        else fill(&r, u->library_list ? "Afficher en grille" : "Afficher en liste", "Présentation de la bibliothèque", u->library_list ? CR_ICON_GRID : CR_ICON_LIST, ACT_LIBRARY, 1);
    } else if (u->page == CR_GAME_ACTIONS) {
        int game = cr_menu_game_index(u, u->states_game_id);
        if (game >= 0) {
            if (!index) game_row(&r, u, game, ACT_GAME);
            else if (index == 1) {
                fill(&r, "États de sauvegarde", u->games[game].title, CR_ICON_SAVES, ACT_STATE_GAME, game);
            } else {
                int core = cr_registry_find(u->games[game].core_id);
                fill(&r, core >= 0 ? "Charger le cœur associé" : "Installer le cœur associé",
                     u->games[game].core_id, core >= 0 ? CR_ICON_CORE : CR_ICON_DOWNLOAD, core >= 0 ? ACT_SELECT : ACT_OPEN_DOWNLOADER, core);
            }
        }
    } else if (u->page == CR_SAVES_LIBRARY) {
        if (!index) {
            fill(&r, "Partie en cours", u->engine.loaded ? u->title : "Lancez un jeu pour créer un état", CR_ICON_SAVES, ACT_STATE_GAME, -1);
            r.disabled = !u->engine.loaded;
        } else game_row(&r, u, index - 1, ACT_STATE_GAME);
    } else if (u->page == CR_STATE_LIST) {
        if (!index) {
            fill(&r, "Nouvelle sauvegarde", "Enregistrer la position actuelle", CR_ICON_ADD, ACT_STATE_CREATE, 0);
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
                 index ? CR_ICON_BACK : CR_ICON_TRASH, index ? ACT_CORE_REMOVE_CANCEL : ACT_STATE_DELETE, 0);
            r.destructive = !index; r.disabled = !available;
        } else {
            static const char *labels[] = {"Charger", "Renommer", "Dupliquer", "Partager", "Supprimer"};
            static const int actions[] = {ACT_STATE_LOAD, ACT_STATE_RENAME, ACT_STATE_DUPLICATE, ACT_STATE_SHARE, ACT_STATE_DELETE_CONFIRM};
            static const int icons[] = {CR_ICON_IMPORT, CR_ICON_RENAME, CR_ICON_DUPLICATE, CR_ICON_SHARE, CR_ICON_TRASH};
            fill(&r, labels[index], "", icons[index], actions[index], 0);
            r.disabled = !available || (index == 3 && !u->platform.share_state);
            if (!index && available) r.disabled = !u->engine.handle ||
                u->engine.system != cr_registry_find(u->saved_states[u->selected_state].core_id);
            if (!index && r.disabled) snprintf(r.hint, sizeof(r.hint), "Chargez le cœur associé à cet état");
            r.destructive = index == 4;
        }
    } else if (u->page == CR_REMOVE_CORES) {
        int slot = removable_core_at(u,index);
        if (slot >= 0) fill(&r,core_name(slot),"Supprimer ce cœur téléchargé",CR_ICON_TRASH,ACT_CORE_REMOVE_CONFIRM,slot);
    } else if (u->page == CR_REMOVE_CORE_CONFIRM) {
        char label[160];
        snprintf(label,sizeof(label),"Supprimer %.128s",u->remove_core_name);
        fill(&r,index ? "Annuler" : label,index ? "Revenir à la liste des cœurs" :
             "Jeux et sauvegardes conservés",index ? CR_ICON_BACK : CR_ICON_TRASH,
             index ? ACT_CORE_REMOVE_CANCEL : ACT_CORE_REMOVE,0);
    } else if (u->page == CR_CORES) {
        if (index > (int)cr_registry_count()) {
            fill(&r,"Supprimer un cœur téléchargé","Conserver jeux et sauvegardes",CR_ICON_TRASH,ACT_PAGE,CR_REMOVE_CORES);
        } else if (index == (int)cr_registry_count()) {
            fill(&r, "Téléchargeur de cœurs", "Catalogue officiel CorvusSoftware",
                 CR_ICON_DOWNLOAD, ACT_OPEN_DOWNLOADER, 0);
        } else {
            int slot = cr_registry_index_at((size_t)index);
            fill(&r, core_name(slot), cr_menu_system_name(slot), CR_ICON_CORE, ACT_SELECT, slot);
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
                 core && pack_index >= 0 ? CR_ICON_DOWNLOAD : CR_ICON_RESTART, core && pack_index >= 0 ? ACT_INSTALL_CORE_PACK : ACT_REFRESH_CATALOG, pack_index);
            r.disabled = u->installing || u->catalog_loading;
        } else if (index == 2) {
            fill(&r, "Désinstaller", "Jeux et sauvegardes conservés", CR_ICON_TRASH, ACT_CORE_REMOVE_CONFIRM, core_index);
            r.disabled = !core || !u->platform.can_remove_core || !u->platform.remove_core ||
                !u->platform.can_remove_core(u->platform.context, core->id) || u->installing;
            r.destructive = true;
        } else fill(&r, "Bibliothèque", "Retrouver vos jeux importés", CR_ICON_GRID, ACT_LIBRARY, 0);
    } else if (u->page == CR_CORE_DOWNLOADS) {
        if (index == 0) {
            fill(&r, "Actualiser le catalogue",
                 u->catalog_loading ? "Récupération en cours..." :
                 (u->core_catalog.error[0] ? u->core_catalog.error :
                  "Rechercher les cœurs CorvusSoftware"),
                 CR_ICON_RESTART, ACT_REFRESH_CATALOG, 0);
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
        const int icons[] = {CR_ICON_DISPLAY, CR_ICON_INPUT, CR_ICON_AUDIO, CR_ICON_SYSTEM, CR_ICON_SAVES, CR_ICON_ACHIEVEMENTS, CR_ICON_INFO};
        fill(&r, labels[index], hints[index], icons[index], ACT_PAGE, pages[index]);
    } else if (u->page == CR_SYSTEM_SETTINGS) {
        static const char *labels[] = {"Interface", "Sauvegardes", "Informations", "Importer du contenu"};
        static const char *hints[] = {"Thèmes et notifications", "Copies et états automatiques", "Application, cœur et licences", "Jeux et dossiers"};
        static const int pages[] = {CR_UI_SETTINGS, CR_SAVE_SETTINGS, CR_INFORMATION, CR_CONTENT};
        static const int icons[] = {CR_ICON_DISPLAY, CR_ICON_SAVES, CR_ICON_INFO, CR_ICON_IMPORT};
        fill(&r, labels[index], hints[index], icons[index], ACT_PAGE, pages[index]);
    } else if (u->page == CR_RA_SETTINGS) {
        fill(&r, index ? "Compte" : "Activer RetroAchievements", index ? "Connexion et état du compte" :
             "Mode normal", index ? CR_ICON_ACCOUNT : CR_ICON_ACHIEVEMENTS, index ? ACT_PAGE : ACT_RA_ENABLE, CR_RA_ACCOUNT);
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
                    ? "Saisi, jamais enregistré" : "Saisir avec le clavier Android", CR_ICON_LOCK, status.connected || status.pending ? ACT_NONE : ACT_RA_PASSWORD, 0);
        } else if (index == 3) {
            fill(&r, status.connected ? "Se déconnecter" : "Se connecter", status.pending ? "Connexion en cours..." :
                    "Le jeton de session est chiffré sur cet appareil", status.connected ? CR_ICON_EXIT : CR_ICON_ACCOUNT, ACT_RA_CONNECT, 0);
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
        fill(&r, cr_menu_settings[id].label, "Affichage dans le jeu", CR_ICON_NOTIFICATIONS, ACT_SETTING_TOGGLE, id);
        r.toggle = true; r.checked = cr_menu_setting_get(u, id) != 0;
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
        fill(&r, "Rétablir la disposition", "Taille et position automatiques", CR_ICON_RESTART, ACT_LAYOUT_RESET, 0);
    } else if (u->page == CR_CONTROLLER_SETTINGS) {
        char name[192] = {0};
        if (index < 2) {
            char label[64]; snprintf(label, sizeof(label), "Port du joueur %d", index + 1);
            bool connected = u->platform.controller_info &&
                u->platform.controller_info(u->platform.context, (unsigned)index, name, sizeof(name));
            fill(&r, label, connected ? name : "Aucune manette connectée", CR_ICON_INPUT, ACT_NONE, 0);
        } else fill(&r, "Affectation des touches", "Profil commun aux manettes", CR_ICON_INPUT, ACT_PAGE, CR_CONTROLLER_BINDINGS);
    } else if (u->page == CR_CONTROLLER_BINDINGS) {
        if (index == 10) fill(&r, "Rétablir les touches", "Revenir au profil automatique", CR_ICON_RESTART, ACT_BIND_RESET, 0);
        else {
            char name[96]; cr_controller_binding_name(u->controller_bindings[index], name, sizeof(name));
            fill(&r, binding_labels[index], u->controller_capture == index ? "Bouton de manette, ou Retour" : name,
                 CR_ICON_INPUT, ACT_BIND_CAPTURE, index);
        }
    } else if (u->page == CR_SAVE_SETTINGS && index == 1) {
        fill(&r, cr_menu_settings[SET_AUTO_STATE].label, "Créé à la fermeture si compatible", CR_ICON_SAVES, ACT_SETTING_TOGGLE, SET_AUTO_STATE);
        r.toggle = true; r.checked = u->auto_state != 0;
    } else if (u->page == CR_SETTING_VALUES) {
        const cr_menu_setting_spec *spec = &cr_menu_settings[u->editing_setting];
        int value = spec->minimum + index * spec->step;
        char label[96]; cr_menu_setting_label(u->editing_setting, value, label, sizeof(label));
        fill(&r, label, value == spec->fallback ? "Valeur par défaut" : "Appliquer ce réglage", cr_menu_setting_icon(u->editing_setting), ACT_SETTING_VALUE, value);
        r.checked = value == cr_menu_setting_get(u, u->editing_setting);
    } else if (u->page == CR_VIDEO_SETTINGS || u->page == CR_AUDIO_SETTINGS ||
               u->page == CR_OVERLAY_SETTINGS || u->page == CR_OVERLAY_LAYOUT || u->page == CR_SAVE_SETTINGS) {
        int id = u->page == CR_VIDEO_SETTINGS ? (index == 1 ? SET_INTEGER : SET_FILTER) :
                 u->page == CR_AUDIO_SETTINGS ? SET_VOLUME : u->page == CR_OVERLAY_SETTINGS ? SET_OPACITY :
                 u->page == CR_OVERLAY_LAYOUT ? (index ? SET_POSITION : SET_SIZE) : SET_BACKUPS;
        char value[96]; cr_menu_setting_label(id, cr_menu_setting_get(u, id), value, sizeof(value));
        fill(&r, cr_menu_settings[id].label, value, cr_menu_setting_icon(id), ACT_SETTING_VALUES, id);
    } else if (u->page == CR_INFORMATION) {
        if (index == 2 + (u->engine.handle ? 1 : 0))
            fill(&r,"Supprimer un cœur téléchargé","Conserver jeux et sauvegardes",CR_ICON_TRASH,ACT_PAGE,CR_REMOVE_CORES);
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
                    fill(&r, u->games[i].title, core_name(u->games[i].system), CR_ICON_CONTENT, ACT_GAME,
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
bool cr_menu_selected_state_entry(const cr_ui *u, cr_state_entry *entry) {
    if (u->selected_state < 0 || (size_t)u->selected_state >= u->saved_state_count) return false;
    *entry = u->saved_states[u->selected_state];
    return true;
}
row cr_menu_entry_visual(const cr_ui *u, int index) {
    if (index >= 0) return cr_menu_entry_get(u, index);
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
        if (cr_menu_selected_state_entry(u, &entry)) {
            snprintf(r.label, sizeof(r.label), "%s", u->ui_editing == 4 ? u->state_rename : entry.name);
            time_t stamp = (time_t)(entry.created_ms / 1000); struct tm local;
            if (localtime_r(&stamp, &local)) strftime(r.hint, sizeof(r.hint), "%d/%m/%Y · %H:%M", &local);
            snprintf(r.image, sizeof(r.image), "%s", entry.preview_key);
        }
    } else if (u->page == CR_STATE_LIST || u->page == CR_GAME_ACTIONS) {
        int game = cr_menu_game_index(u, u->states_game_id);
        if (game >= 0) game_row(&r, u, game, ACT_NONE);
        else snprintf(r.label, sizeof(r.label), "Sauvegardes");
        if (u->page == CR_STATE_LIST) snprintf(r.hint, sizeof(r.hint), "%zu état%s enregistré%s", u->saved_state_count,
            u->saved_state_count > 1 ? "s" : "", u->saved_state_count > 1 ? "s" : "");
    }
    return r;
}
