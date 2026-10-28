// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
#include "menu_driver.h"
#include "play_layout.h"
#include "overlay.h"
#include "core_registry.h"
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
    ACT_LOAD_STATE
};
typedef struct {
    char label[512], hint[256];
    int icon, action, value;
    bool toggle, checked, disabled;
} row;
static void quick_action(cr_ui *u, int action);
static void quick_clamp(cr_ui *u);
static int target(const cr_ui *u, float x, float y);
static bool has_back(const cr_ui *u) {
    return u->quick || u->depth || (u->page != CR_MAIN && u->page != CR_LISTS && u->page != CR_SETTINGS);
}
static void press_controls(cr_ui *u, unsigned mask) {
    /* Keep brief input visible for a rendered frame, independently of the engine latch. */
    u->feedback_pulses |= mask;
    for (int i = 0; i < 10; i++)
        if (mask & (1u << i))
            u->control_feedback[i] = 1;
}
static float body_bottom(const cr_ui *u) {
    return u->height - 100;
}
static int rows(const cr_ui *u) {
    if (u->quick) return CR_QUICK_COUNT;
    if (u->page == CR_SAVESTATES) return 2;
    if (u->page == CR_MAIN)
        return 3 + (u->engine.loaded ? 1 : 0) + (u->engine.handle ? 1 : 0);
    if (u->page == CR_LISTS)
        return 1;
    if (u->page == CR_CORES)
        return 1 + (int)cr_registry_count();
    if (u->page == CR_CORE_DOWNLOADS)
        return 1 + (int)u->core_catalog.count;
    if (u->page == CR_SETTINGS)
        return 5;
    if (u->page == CR_ACHIEVEMENTS) {
        size_t count = cr_cheevos_achievement_count(u->achievements);
        return 6 + (int)(count < 4096 ? count : 4096);
    }
    if (u->page == CR_INFORMATION)
        return 2 + (u->engine.handle ? 1 : 0);
    if (u->page >= CR_UI_SETTINGS && u->page <= CR_OVERLAY_SETTINGS)
        return 1;
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
    snprintf(r->label, sizeof(r->label), "%s", label);
    snprintf(r->hint, sizeof(r->hint), "%.*s", (int)sizeof(r->hint) - 1, hint ? hint : "");
    r->icon = icon;
    r->action = action;
    r->value = value;
}
static row get_row(const cr_ui *u, int index) {
    row r;
    fill(&r, "", "", CR_ICON_FILE, ACT_NONE, 0);
    if (u->quick) {
        static const char *labels[CR_QUICK_COUNT] = {
            "Reprendre le contenu", "Redémarrer le contenu", "Fermer le contenu",
            "Sauvegardes d’état", "Options du cœur", "Commandes", "Succès", "Informations sur le cœur",
            "Menu principal"
        };
        static const char *hints[CR_QUICK_COUNT] = {
            "Retourner au jeu", "Relancer le contenu actuel", "Garder le cœur chargé",
            "Sauvegarder ou charger l’état du jeu", "Réglages proposés par le cœur", "Surimpression tactile",
            "Compte et succès du jeu", "Version et fonctions disponibles", "Retourner aux menus"
        };
        static const int icons[CR_QUICK_COUNT] = {
            CR_ICON_CONTENT, CR_ICON_CONTENT, CR_ICON_UNLOAD, CR_ICON_FILE,
            CR_ICON_CORE, CR_ICON_SETTINGS, CR_ICON_INFO, CR_ICON_CORE, CR_ICON_HOME
        };
        fill(&r, labels[index], hints[index], icons[index], ACT_QUICK_ACTION, index);
        r.disabled = (index == CR_QUICK_RESTART && !u->engine.api.reset) ||
                     (index == CR_QUICK_STATES && (!u->engine.api.serialize_size ||
                       !u->engine.api.serialize || !u->engine.api.unserialize));
    } else if (u->page == CR_SAVESTATES) {
        fill(&r, index ? "Charger l’état" : "Sauvegarder l’état",
             index ? "Restaurer la sauvegarde de ce contenu" : "Enregistrer la position actuelle",
             CR_ICON_FILE, index ? ACT_LOAD_STATE : ACT_SAVE_STATE, 0);
    } else if (u->page == CR_MAIN) {
        int n = 0;
        if (u->engine.loaded) {
            if (index == n++)
                fill(&r, "Menu rapide", u->title, CR_ICON_MENU, ACT_QUICK, 0);
        }
        if (index == n++)
            fill(&r, u->engine.handle ? "Décharger le cœur" : "Charger un cœur",
                 u->engine.handle ? core_name(u->engine.system) : "Aucun cœur",
                 u->engine.handle ? CR_ICON_UNLOAD : CR_ICON_CORE,
                 u->engine.handle ? ACT_UNLOAD : ACT_CORES, 0);
        if (index == n++)
            fill(&r, "Charger du contenu",
                 u->importing ? "Import du contenu…" :
                 (!u->engine.handle ? "Chargez un cœur avant le contenu" :
                  "Ouvrir un fichier de jeu compatible"),
                 CR_ICON_CONTENT, ACT_PICK, 0);
        if (index == n - 1) r.disabled = !u->engine.handle;
        if (u->engine.handle && index == n++)
            fill(&r, "Configuration du sous-système", "Sous-systèmes du cœur chargé",
                 CR_ICON_CORE, ACT_PAGE, CR_SUBSYSTEMS);
        if (index == n)
            fill(&r, "Informations", "Application, cœur et licences", CR_ICON_INFO, ACT_PAGE,
                 CR_INFORMATION);
    } else if (u->page == CR_LISTS) {
        fill(&r, "Historique", "Contenus récemment ouverts", CR_ICON_HISTORY, ACT_HISTORY, 0);
    } else if (u->page == CR_CORES) {
        if (index == (int)cr_registry_count()) {
            fill(&r, "Téléchargeur de cœurs", "Catalogue officiel CorvusSoftware",
                 CR_ICON_CORE, ACT_OPEN_DOWNLOADER, 0);
        } else {
            fill(&r, core_name(index), system_name(index), CR_ICON_CORE, ACT_SELECT, index);
            if (strcmp(cr_registry_get(index)->id, "corvus.gba") == 0)
                snprintf(r.hint, sizeof(r.hint), "%s · Expérimental", system_name(index));
            r.checked = u->engine.handle && u->engine.system == index;
        }
    } else if (u->page == CR_CORE_DOWNLOADS) {
        if (index == 0) {
            fill(&r, "Actualiser le catalogue",
                 u->catalog_loading ? "Récupération en cours..." :
                 (u->core_catalog.error[0] ? u->core_catalog.error :
                  "Rechercher les cœurs CorvusSoftware"),
                 CR_ICON_CORE, ACT_REFRESH_CATALOG, 0);
        } else {
            const cr_core_pack *pack = &u->core_catalog.packs[index - 1];
            const cr_core_descriptor *installed = cr_registry_get(cr_registry_find(pack->core_id));
            const char *state = !installed ? "" : strcmp(installed->version, pack->version) == 0
                    ? " - Installé" : " - mise à jour";
            char hint[160];
            snprintf(hint, sizeof(hint), "Version %.56s - %.20s%s",
                     pack->version, pack->abi, state);
            fill(&r, pack->name, hint, CR_ICON_CORE, ACT_INSTALL_CORE_PACK, index - 1);
        }
    } else if (u->page == CR_SETTINGS) {
        const char *labels[] = {"Interface utilisateur", "Vidéo", "Audio", "Affichage à l’écran", "RetroAchievements"};
        const char *hints[] = {"Apparence des menus", "Affichage pendant le jeu", "Sortie audio",
                               "Surimpression tactile", "Compte et succès des jeux"};
        const int pages[] = {CR_UI_SETTINGS, CR_VIDEO_SETTINGS, CR_AUDIO_SETTINGS, CR_OSD_SETTINGS, CR_ACHIEVEMENTS};
        fill(&r, labels[index], hints[index], CR_ICON_SETTINGS, ACT_PAGE, pages[index]);
    } else if (u->page == CR_ACHIEVEMENTS) {
        cr_cheevos_status status = {0};
        cr_cheevos_get_status(u->achievements, &status);
        if (index == 0) {
            fill(&r, "Activer RetroAchievements", "Mode normal, avec sauvegardes d’état", CR_ICON_SETTINGS, ACT_RA_ENABLE, 0);
            r.toggle = true; r.checked = u->ra_enabled;
        } else if (index == 1) {
            const char *user = status.connected ? status.username : (u->ra_editing == 1 ? u->ra_preview : u->ra_username);
            fill(&r, "Identifiant", user[0] ? user : "Saisir le nom du compte", CR_ICON_INFO, status.connected || status.pending ? ACT_NONE : ACT_RA_USER, 0);
        } else if (index == 2) {
            fill(&r, "Mot de passe", u->ra_password[0] || (u->ra_editing == 2 && u->ra_preview[0])
                    ? "Saisi, jamais enregistré" : "Saisir avec le clavier Android", CR_ICON_INFO, status.connected || status.pending ? ACT_NONE : ACT_RA_PASSWORD, 0);
        } else if (index == 3) {
            fill(&r, status.connected ? "Se déconnecter" : "Se connecter", status.pending ? "Connexion en cours..." :
                    "Le jeton de session est chiffré sur cet appareil", CR_ICON_INFO, ACT_RA_CONNECT, 0);
        } else if (index == 4) {
            fill(&r, "État", status.message[0] ? status.message : "Connectez votre compte pour activer les succès", CR_ICON_INFO, ACT_NONE, 0);
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
                fill(&r, achievement.title, hint, CR_ICON_INFO, ACT_NONE, 0);
                r.checked = achievement.unlocked;
            }
        }
    } else if (u->page == CR_UI_SETTINGS) {
        fill(&r, "Thème de l’interface", cr_theme_get(u->theme)->name, CR_ICON_THEME, ACT_THEME, 0);
    } else if (u->page == CR_VIDEO_SETTINGS) {
        fill(&r, "Garder l’écran allumé", "Empêcher la veille pendant le jeu", CR_ICON_SETTINGS,
             ACT_AWAKE, 0);
        r.toggle = true;
        r.checked = u->awake;
    } else if (u->page == CR_AUDIO_SETTINGS) {
        fill(&r, "Activer le son", "Sortie audio pendant le jeu", CR_ICON_SETTINGS, ACT_AUDIO, 0);
        r.toggle = true;
        r.checked = u->audio;
    } else if (u->page == CR_OSD_SETTINGS) {
        fill(&r, "Surimpression tactile", "Retour visuel des commandes", CR_ICON_SETTINGS, ACT_PAGE,
             CR_OVERLAY_SETTINGS);
    } else if (u->page == CR_OVERLAY_SETTINGS) {
        fill(&r, "Afficher les commandes", "Afficher les commandes tactiles pendant le jeu", CR_ICON_SETTINGS,
             ACT_SHOW_INPUT, 0);
        r.toggle = true;
        r.checked = u->show_input;
    } else if (u->page == CR_INFORMATION) {
        if (index == 0)
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
    float content = (float)rows(u) * 76;
    if (u->page == CR_ABOUT || u->page == CR_LICENSE || u->page == CR_CORE_INFO ||
        u->page == CR_SUBSYSTEMS || u->page == CR_CORE_OPTIONS)
        content = fmaxf(400, u->text_height);
    float max = fmaxf(0, content - fmaxf(1, body_bottom(u) - 64));
    u->scroll = fminf(max, fmaxf(0, u->scroll));
}
static void view(cr_ui *u, int p) {
    u->page = p;
    u->tab =
        p == CR_PLAY                     ? u->tab
        : p == CR_LISTS || p == CR_GAMES ? 1
        : p == CR_SETTINGS || p == CR_THEMES || p == CR_ACHIEVEMENTS || (p >= CR_UI_SETTINGS && p <= CR_OVERLAY_SETTINGS)
            ? 2
            : 0;
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
        snprintf(detail, sizeof(detail), "%u/%u succès - Mode normal", event->unlocked, event->total);
        cr_notifications_push(&u->notifications, CR_NOTIFICATION_GAME, 0,
                              event->title, detail, event->image_url);
        break;
    case CR_CHEEVOS_UNLOCKED:
        snprintf(detail, sizeof(detail), "%s", event->title);
        char title[96];
        snprintf(title, sizeof(title), "Succès débloqué - %u points", event->points);
        cr_notifications_push(&u->notifications, CR_NOTIFICATION_UNLOCK, event->id,
                              title, detail, event->image_url);
        break;
    case CR_CHEEVOS_PROGRESS:
        cr_notifications_progress(&u->notifications, event->id, event->detail,
                                  event->image_url, event->percent);
        break;
    case CR_CHEEVOS_PROGRESS_HIDE:
        cr_notifications_hide_progress(&u->notifications, event->id);
        break;
    case CR_CHEEVOS_COMPLETED:
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
    if (!u->ra_enabled || !u->engine.loaded) {
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
    if (u->ra_enabled) cr_cheevos_frame(u->achievements);
}
void cr_ui_text_result(cr_ui *u, int field, const char *text, bool accepted) {
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
    u->theme = CR_THEME_CORVUS;
    u->menu_pointer = u->feedback_target = -1;
    u->show_input = true;
    cr_engine_init(&u->engine, root);
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
    cr_cheevos_destroy(u->achievements);
    u->achievements = NULL;
    memset(u->ra_password, 0, sizeof(u->ra_password));
    memset(u->ra_preview, 0, sizeof(u->ra_preview));
    cr_engine_destroy(&u->engine);
    free(u->games);
    free(u->license);
    u->games = NULL;
    u->license = NULL;
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
    u->feedback_pulses = 0;
    u->menu_pointer = u->feedback_target = -1;
    u->feedback_alpha = 0;
    memset(u->control_feedback, 0, sizeof(u->control_feedback));
}
unsigned cr_ui_input(cr_ui *u) {
    unsigned bits = u->keys | u->pulses;
    for (int i = 0; i < 32; i++)
        if (u->pointers[i].active)
            bits |= u->pointers[i].mask;
    u->pulses = 0;
    return bits;
}
bool cr_ui_running(const cr_ui *u) {
    return u->page == CR_PLAY && !u->quick && u->engine.loaded && !u->installing;
}
void cr_ui_update(cr_ui *u, double seconds) {
    achievement_sync(u, false);
    cr_cheevos_idle(u->achievements);
    float dt = (float)fmax(0, seconds);
    u->time += dt;
    cr_notifications_update(&u->notifications, dt);
    u->message_time = fmax(0, u->message_time - dt);
    unsigned held = u->keys | u->feedback_pulses;
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
    cr_notifications_clear(&u->notifications);
    achievement_sync(u, true);
    u->notifications_loaded = true;
    snprintf(u->title, sizeof(u->title), "%s", title);
    page(u, CR_PLAY);
    if (u->platform.touch)
        u->platform.touch(u->platform.context, id);
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
static void activate(cr_ui *u, row r) {
    if (r.disabled && r.action == ACT_QUICK_ACTION) return;
    if (u->installing && (r.action == ACT_INSTALL_CORE_PACK || r.action == ACT_GAME ||
                          r.action == ACT_SELECT || r.action == ACT_PICK ||
                          r.action == ACT_UNLOAD || r.action == ACT_SAVE_STATE ||
                          r.action == ACT_LOAD_STATE)) {
        cr_ui_message(u, "Téléchargement du cœur en cours");
        return;
    }
    switch (r.action) {
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
            u->title[0] = 0;
            page(u, CR_MAIN);
        }
        break;
    case ACT_PICK:
        if (!u->engine.handle) {
            cr_ui_message(u, "Chargez un cœur avant de charger du contenu");
            break;
        }
        if (!u->importing && u->platform.pick) {
            u->importing = true;
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
    u->importing = false;
    if (item->system < 0) {
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
    bool selected = u->engine.handle && u->engine.system == resolved.system;
    bool valid = selected && id_ok && cr_engine_check(&u->engine, resolved.system, resolved.path);
    if (!u->engine.handle)
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
        cr_ui_message(u, error);
        return;
    }
    cr_ui_reload(u);
    char path[4256];
    snprintf(path, sizeof(path), "%s/roms/%s.rom", u->engine.root, resolved.id);
    open_game(u, resolved.system, resolved.id, resolved.title, path);
}
static void quick_action(cr_ui *u, int action) {
    if (action < 0 || action >= CR_QUICK_COUNT || get_row(u, action).disabled) return;
    if (u->installing && (action == CR_QUICK_RESTART || action == CR_QUICK_CLOSE ||
                          action == CR_QUICK_STATES)) {
        cr_ui_message(u, "Téléchargement du cœur en cours");
        return;
    }
    switch (action) {
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
            page(u, CR_MAIN);
        }
        break;
    case CR_QUICK_STATES: push_page(u, CR_SAVESTATES); break;
    case CR_QUICK_CORE_OPTIONS: push_page(u, CR_CORE_OPTIONS); break;
    case CR_QUICK_CONTROLS: push_page(u, CR_OVERLAY_SETTINGS); break;
    case CR_QUICK_ACHIEVEMENTS: push_page(u, CR_ACHIEVEMENTS); break;
    case CR_QUICK_CORE_INFO: push_page(u, CR_CORE_INFO); break;
    case CR_QUICK_MAIN: page(u, CR_MAIN); break;
    default: break;
    }
    cr_ui_clear_input(u);
}
void cr_ui_back(cr_ui *u) {
    cr_ui_clear_input(u);
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
        page(u, CR_PLAY);
        return;
    }
    if (u->platform.quit)
        u->platform.quit(u->platform.context);
}
void cr_ui_confirm(cr_ui *u) {
    if (u->quick)
        quick_action(u, u->focus);
    else if (u->page != CR_PLAY && u->focus >= 0 && u->focus < rows(u))
        activate(u, get_row(u, u->focus));
}
void cr_ui_move(cr_ui *u, int direction) {
    int count = rows(u);
    if (count <= 0)
        return;
    u->focus = (u->focus + direction + count) % count;
    if (u->quick) {
        float top = u->focus * 76;
        float view = fmaxf(1, body_bottom(u) - 64);
        if (top < u->quick_scroll)
            u->quick_scroll = top;
        if (top + 76 > u->quick_scroll + view)
            u->quick_scroll = top + 76 - view;
        quick_clamp(u);
    } else {
        float top = u->focus * 76;
        float view = fmaxf(1, body_bottom(u) - 64);
        if (top < u->scroll)
            u->scroll = top;
        if (top + 76 > u->scroll + view)
            u->scroll = top + 76 - view;
        clamp_scroll(u);
    }
}
void cr_ui_key(cr_ui *u, unsigned mask, bool down) {
    if (down) {
        press_controls(u, mask);
        u->keys |= mask;
        u->pulses |= mask;
    } else
        u->keys &= ~mask;
}
static cr_play_layout play_layout(const cr_ui *u) {
    return cr_play_layout_make(u->width, u->height,
                               cr_registry_legacy_system(u->engine.system), u->engine.width,
                               u->engine.height);
}
static unsigned button_at(const cr_ui *u, float x, float y) {
    if (!u->show_input) return 0;
    cr_play_layout layout = play_layout(u);
    return cr_play_hit(&layout, x, y);
}
static void quick_clamp(cr_ui *u) {
    u->quick_scroll = fmaxf(0, fminf(u->quick_scroll,
                       CR_QUICK_COUNT * 76 - fmaxf(1, body_bottom(u) - 64)));
}
static int target(const cr_ui *u, float x, float y) {
    if (x < 0 || y < 0 || x >= u->width || y >= u->height)
        return -1;
    if (u->page == CR_PLAY && !u->quick) {
        cr_play_layout layout = play_layout(u);
        return cr_area_contains(layout.menu, x, y) ? -3 :
               cr_area_contains(layout.hide, x, y) ? -5 : -1;
    }
    if (y < 56)
        return x < 56 && has_back(u) ? -2 : -1;
    if (y >= u->height - 72)
        return -100 - (int)fminf(2, fmaxf(0, floorf(x / u->width * 3)));
    if (y < 64 || y >= body_bottom(u))
        return -1;
    int i = (int)((y - 64 + (u->quick ? u->quick_scroll : u->scroll)) / 76);
    return i < rows(u) ? (u->quick ? 10000 + i : i) : -1;
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
        *p = (cr_pointer){.active = true,
                          .x = x,
                          .y = y,
                          .last_x = x,
                          .last_y = y,
                          .target = target(u, x, y),
                          .started = event_time};
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
        if (p->dragged && u->quick && p->target >= 10000) {
            u->quick_scroll += p->last_y - y;
            quick_clamp(u);
        }
        if (p->dragged && p->target >= -1 && !u->quick && u->page != CR_PLAY) {
            u->scroll += p->last_y - y;
            clamp_scroll(u);
        }
        p->last_x = x;
        p->last_y = y;
        return;
    }
    if (phase == CR_TOUCH_UP) {
        int t = p->target;
        bool click = !p->dragged && target(u, x, y) == t;
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
            quick_action(u, t - 10000);
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
        if (t <= -100) {
            page(u, -t - 100);
            return;
        }
        if (t >= 0) {
            u->focus = t;
            activate(u, get_row(u, t));
        }
    }
}

static uint32_t alpha_color(uint32_t color, float alpha) {
    return (color & 0xffffff) | ((uint32_t)lroundf(fmaxf(0, fminf(1, alpha)) * 255) << 24);
}
static void rect(const cr_painter *p, float x, float y, float w, float h, uint32_t c) {
    if (w > 0 && h > 0)
        p->rect(p->context, x, y, w, h, c);
}
static void toggle(const cr_painter *p, const cr_theme *theme, float x, float y, bool checked) {
    uint32_t track = checked ? theme->accent : theme->divider;
    p->circle(p->context, x + 14, y + 14, 14, track);
    rect(p, x + 14, y, 20, 28, track);
    p->circle(p->context, x + 34, y + 14, 14, track);
    p->circle(p->context, x + (checked ? 34 : 14), y + 14, 10,
              checked ? 0xffffffff : theme->muted);
}
static void text(const cr_painter *p, const char *s, float x, float y, float size, uint32_t c,
                 float max) {
    p->text(p->context, s, x, y, size, c, max);
}
static float width(const cr_painter *p, const char *s, float size) {
    return p->measure(p->context, s, size);
}
static void center(const cr_painter *p, const char *s, float x, float y, float size, uint32_t c) {
    text(p, s, x - width(p, s, size) / 2, y, size, c, 10000);
}
static float paragraphs(const cr_painter *p, const char *s, float x, float top, float w, float line,
                        uint32_t color, float scroll) {
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
            if (y > 50 && y < 10000)
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
    if (at)
        text(p, buffer, x, y, 14, color, w);
    return y + scroll - top + line;
}
void cr_ui_draw(cr_ui *u, const cr_painter *p) {
    const cr_theme *theme = cr_theme_get(u->theme);
    uint32_t bg = theme->background, header = theme->header, nav = theme->navigation;
    uint32_t fg = theme->text, muted = theme->muted, accent = theme->accent;
    uint32_t line = theme->divider, focus = theme->focus;
    float w = u->width, h = u->height;
    rect(p, 0, 0, w, h, bg);
    if (u->page == CR_PLAY && !u->quick) {
        rect(p, 0, 0, w, h, 0xff000000);
        cr_play_layout layout = play_layout(u);
        if (u->engine.loaded)
            p->game(p->context, u->engine.pixels, u->engine.width, u->engine.height, layout.video.x,
                    layout.video.y, layout.video.w, layout.video.h);
        unsigned input = u->keys;
        bool menu_down = false, hide_down = false;
        for (int i = 0; i < 32; i++)
            if (u->pointers[i].active) {
                input |= u->pointers[i].mask;
                menu_down |= u->pointers[i].target == -3 && !u->pointers[i].dragged;
                hide_down |= u->pointers[i].target == -5 && !u->pointers[i].dragged;
            }
        cr_overlay_draw(&layout, p, input, u->control_feedback, u->show_input, menu_down, hide_down);
    } else {
        const char *title = u->quick ? "Menu rapide" : "Menu principal";
        switch (u->page) {
        case CR_LISTS:
            title = "Listes de lecture";
            break;
        case CR_SETTINGS:
            title = "Réglages";
            break;
        case CR_CORES:
            title = "Charger un cœur";
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
        case CR_ACHIEVEMENTS:
            title = "RetroAchievements";
            break;
        case CR_CORE_DOWNLOADS:
            title = "Téléchargeur de cœurs";
            break;
        case CR_GAMES:
            title = u->filter < 0 ? "Historique" : system_name(u->filter);
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
            title = "Vidéo";
            break;
        case CR_AUDIO_SETTINGS:
            title = "Audio";
            break;
        case CR_OSD_SETTINGS:
            title = "Affichage à l’écran";
            break;
        case CR_OVERLAY_SETTINGS:
            title = "Surimpression tactile";
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
        if (p->clip)
            p->clip(p->context, 0, 56, w, fmaxf(1, body_bottom(u) - 56));
        int count = rows(u);
        for (int i = 0; i < count; i++) {
            float y = 64 + i * 76 - (u->quick ? u->quick_scroll : u->scroll);
            if (y + 76 < 56 || y > body_bottom(u))
                continue;
            row r = get_row(u, i);
            if (i == u->focus)
                rect(p, 0, y, w, 76, focus);
            else if (u->feedback_target == (u->quick ? 10000 + i : i) && u->feedback_alpha > 0)
                rect(p, 0, y, w, 76, alpha_color(focus, u->feedback_alpha));
            p->icon(p->context, r.icon, 20, y + 22, 32, r.disabled ? muted : theme->icon);
            float text_width = w - (r.toggle ? 150 : 110);
            text(p, r.label, 66, y + 29, 16, r.disabled ? muted : fg, text_width);
            text(p, r.hint, 66, y + 53, 13, muted, text_width);
            if (r.toggle) {
                toggle(p, theme, w - 72, y + 24, r.checked);
            } else if (r.checked)
                text(p, "✓", w - 36, y + 41, 20, accent, 28);
            else
                p->icon(p->context, CR_ICON_NEXT, w - 36, y + 29, 18, muted);
            rect(p, 66, y + 75, w - 66, 1, line);
        }
        if (u->page == CR_CORE_DOWNLOADS && u->core_catalog.count == 0) {
            text(p, u->catalog_loading ? "Recherche des cœurs officiels..." :
                    "Aucun cœur disponible pour cette version.",
                 20, 236, 14, fg, w - 40);
        }
        if (u->page == CR_GAMES && !count) {
            text(p,
                 u->filter < 0 ? "Aucun contenu dans l’historique."
                               : "Aucun contenu dans cette liste.",
                 20, 106, 15, fg, w - 40);
            text(p, "Utilisez Charger du contenu.", 20, 137, 13, muted, w - 40);
        }
        if (u->page == CR_ABOUT)
            u->text_height = paragraphs(
                p,
                "CorvusRetro 0.17.1\n\nFrontend original en C, rendu OpenGL ES et audio "
                "AAudio.\n\nCorvusSoftware\nCœurs officiels CorvusCore 0.14.1 : Game Boy, Game "
                "Boy Color, Game Boy Advance (expérimental).\n\nNavigation inspirée de "
                "MaterialUI. Thèmes CorvusRetro, ASUS et NVIDIA Shield.\n\nCopyright (C) 2026 "
                "CorvusSoftware\nSPDX-License-Identifier: AGPL-3.0-only",
                20, 92, w - 40, 24, fg, u->scroll);
        if (u->page == CR_SUBSYSTEMS || u->page == CR_CORE_OPTIONS) {
            const char *empty = !u->engine.handle ? "Aucun cœur chargé." :
                u->page == CR_SUBSYSTEMS ?
                    "Aucun sous-système disponible avec ce cœur.\n\n"
                    "La configuration de plusieurs jeux n’est pas prise en charge par cette version." :
                    "Aucune option de cœur disponible.\n\n"
                    "Ce cœur ne fournit aucun réglage modifiable dans cette version.";
            u->text_height = paragraphs(p, empty, 20, 92, w - 40, 24, fg, u->scroll);
        }
        if (u->page == CR_CORE_INFO) {
            char info[2048];
            if (u->engine.handle) {
                const cr_core_descriptor *desc = cr_registry_get(u->engine.system);
                snprintf(info, sizeof(info),
                         "%s\n\nSystème : %s\nVersion : %s\nCorvusAPI : %u\nExtensions : %s\n"
                         "États de sauvegarde : %s\nBatterie : %s\nContenu : %s",
                         core_name(u->engine.system), system_name(u->engine.system),
                         desc ? desc->version : "?", CORVUS_API_VERSION,
                         desc ? desc->extensions : "?",
                         desc && desc->state_support ? "Oui" : "Non",
                         desc && desc->battery_support ? "Oui" : "Non",
                         u->engine.loaded ? u->title : "Aucun contenu");
            }
            else
                snprintf(info, sizeof(info), "Aucun cœur chargé");
            u->text_height = paragraphs(p, info, 20, 92, w - 40, 24, fg, u->scroll);
        }
        if (u->page == CR_LICENSE)
            u->text_height = paragraphs(p, u->license ? u->license : "Licence indisponible", 20, 92,
                                        w - 40, 23, fg, u->scroll);
        if (p->clip)
            p->clip(p->context, 0, 0, w, h);
        // Cover scrolled content before drawing the fixed header and navigation.
        rect(p, 0, 0, w, 56, header);
        if (has_back(u) && u->feedback_target == -2 && u->feedback_alpha > 0)
            p->circle(p->context, 28, 28, 22, alpha_color(focus, u->feedback_alpha));
        if (has_back(u))
            p->icon(p->context, CR_ICON_BACK, 12, 16, 24, theme->header_text);
        text(p, title, has_back(u) ? 56 : 20, 35, 19, theme->header_text, w - 100);
        p->icon(p->context, CR_ICON_CROW, w - 44, 16, 24, theme->header_icon);
        rect(p, 0, body_bottom(u), w, 100, bg);
        char footer[256];
        if (u->engine.handle)
            snprintf(footer, sizeof(footer), "Cœur chargé : %s · %s", core_name(u->engine.system),
                     cr_registry_get(u->engine.system)->version);
        else
            snprintf(footer, sizeof(footer), "Aucun cœur");
        text(p, footer, 20, h - 84, 11, muted, w - 40);
        rect(p, 0, h - 72, w, 72, nav);
        rect(p, 0, h - 72, w, 1, line);
        const char *labels[] = {"Menu principal", "Listes de lecture", "Réglages"};
        int icons[] = {CR_ICON_HOME, CR_ICON_LIST, CR_ICON_SETTINGS};
        int selected = u->tab;
        for (int i = 0; i < 3; i++) {
            float x = w * (i + .5f) / 3;
            uint32_t c = i == selected ? theme->navigation_active : theme->navigation_passive;
            if (u->feedback_target == -100 - i && u->feedback_alpha > 0)
                rect(p, i * w / 3, h - 72, w / 3, 72, alpha_color(focus, u->feedback_alpha));
            p->icon(p->context, icons[i], x - 12, h - 61, 24, c);
            center(p, labels[i], x, h - 17, 10, c);
        }
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
