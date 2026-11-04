// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
#define _POSIX_C_SOURCE 200809L
#include "menu_cheat.h"
#include "menu_cbs.h"
#include "menu_internal.h"
#include "cheats/cheat_store.h"
#include <errno.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

struct cr_menu_cheats {
    cr_cheat_profile profiles[2];
    char ids[2][96], notice[256], edit[CR_CHEAT_CODE_SIZE];
    bool writable[2], dirty[2], exporting;
    unsigned player, request_player;
    int selected;
    uint64_t request;
};
static atomic_uint_fast64_t file_nonce = ATOMIC_VAR_INIT(0);

bool cr_menu_cheat_page(int page) {
    return page >= CR_CHEATS && page <= CR_CHEAT_TEXT;
}
static uint32_t formats(const cr_ui *u) {
    return cr_engine_cheat_capabilities(&u->engine, u->cheats ? u->cheats->player : 0);
}
static bool path(const cr_ui *u, unsigned player, char *out, size_t size) {
    if (!u->cheats || player > 1 || !cr_game_id_valid(u->cheats->ids[player])) return false;
    int n = snprintf(out, size, "%s/cheats/%s%s.cht", u->engine.root,
                     u->cheats->ids[player], player ? ".p2" : "");
    return n > 0 && (size_t)n < size;
}
static bool directory(const cr_ui *u) {
    char folder[4128];
    int n = snprintf(folder, sizeof(folder), "%s/cheats", u->engine.root);
    if (n < 0 || (size_t)n >= sizeof(folder)) return false;
    if (mkdir(folder, 0700) && errno != EEXIST) return false;
    struct stat st;
    return !lstat(folder, &st) && S_ISDIR(st.st_mode);
}
static void display_copy(char *out, size_t cap, const char *text) {
    size_t n = strlen(text);
    if (n >= cap) {
        n = cap - 1;
        while (n && ((unsigned char)text[n] & 0xc0) == 0x80) --n;
    }
    memmove(out, text, n); out[n] = 0;
}
static void message(cr_ui *u, const char *text) {
    if (u->cheats) display_copy(u->cheats->notice, sizeof(u->cheats->notice), text);
    cr_ui_message(u, text);
}
static bool apply(cr_ui *u) {
    struct cr_menu_cheats *s = u->cheats;
    if (!s || !s->writable[s->player] || !formats(u)) return false;
    const cr_cheat_profile *p = &s->profiles[s->player];
    const char *codes[CR_CHEAT_MAX_ENTRIES];
    size_t count = 0;
    for (size_t i = 0; i < p->count; ++i) if (p->entries[i].enabled) codes[count++] = p->entries[i].code;
    if (!cr_engine_replace_cheats(&u->engine, s->player, count ? codes : NULL, count)) {
        s->dirty[s->player] = true; message(u, u->engine.error); return false;
    }
    s->dirty[s->player] = false;
    message(u, count ? "Codes de triche appliqués" : "Tous les codes de ce joueur sont désactivés");
    return true;
}
void cr_menu_cheats_cancel_file(cr_ui *u) {
    if (!u->cheats || !u->cheats->request) return;
    uint64_t request = u->cheats->request; u->cheats->request = 0;
    if (u->platform.finish_cheat_file) u->platform.finish_cheat_file(u->platform.context, request);
}
void cr_menu_cheats_destroy(cr_ui *u) {
    if (!u->cheats) return;
    cr_menu_cheats_cancel_file(u);
    for (unsigned p = 0; p < 2; ++p) cr_cheat_profile_free(&u->cheats->profiles[p]);
    free(u->cheats); u->cheats = NULL;
}
void cr_menu_cheats_loaded(cr_ui *u) {
    cr_menu_cheats_destroy(u);
    if (!u->engine.loaded || !cr_game_id_valid(u->engine.id)) return;
    struct cr_menu_cheats *s = calloc(1, sizeof(*s));
    if (!s) { cr_ui_message(u, "Mémoire insuffisante pour les codes de triche"); return; }
    u->cheats = s; s->selected = -1;
    snprintf(s->ids[0], sizeof(s->ids[0]), "%s", u->engine.id);
    snprintf(s->ids[1], sizeof(s->ids[1]), "%s", u->engine.second_id);
    for (unsigned player = 0; player < u->engine.players && player < 2; ++player) {
        char file[4256], error[256] = {0}; bool found = false;
        if (!path(u, player, file, sizeof(file))) {
            message(u, "Impossible d'ouvrir le dossier des codes de triche"); continue;
        }
        s->writable[player] = cr_cheat_profile_load(&s->profiles[player], file, &found, error, sizeof(error));
        if (!s->writable[player]) { message(u, error); continue; }
        for (size_t i = 0; i < s->profiles[player].count; ++i)
            if (s->profiles[player].entries[i].enabled) s->dirty[player] = true;
        s->player = player;
        if (s->profiles[player].apply_after_load && formats(u)) apply(u);
    }
    s->player = 0;
}
void cr_menu_cheats_open(cr_ui *u) {
    if (!u->engine.loaded) { cr_ui_message(u, "Lancez un jeu pour gérer ses codes de triche"); return; }
    if (!u->cheats) cr_menu_cheats_loaded(u);
    if (u->cheats) cr_menu_push_page(u, CR_CHEATS);
}
static bool save(cr_ui *u, cr_cheat_profile *next, bool changed) {
    struct cr_menu_cheats *s = u->cheats;
    char file[4256], error[256] = {0};
    if (!path(u, s->player, file, sizeof(file)) || !directory(u) ||
        !cr_cheat_profile_save(next, file, error, sizeof(error))) {
        message(u, error[0] ? error : "Impossible d'enregistrer la liste de triche"); return false;
    }
    cr_cheat_profile_free(&s->profiles[s->player]);
    s->profiles[s->player] = *next; memset(next, 0, sizeof(*next));
    s->writable[s->player] = true; s->dirty[s->player] |= changed;
    return true;
}
static void saved(cr_ui *u, bool changed) {
    if (changed && u->cheats->profiles[u->cheats->player].apply_after_toggle) apply(u);
    else message(u, changed ? "Liste enregistrée ; appliquez les modifications" : "Réglage enregistré");
}
static bool copy(cr_ui *u, cr_cheat_profile *next) {
    char error[256] = {0};
    if (cr_cheat_profile_clone(next, &u->cheats->profiles[u->cheats->player], error, sizeof(error))) return true;
    message(u, error); return false;
}
static bool selected(const cr_ui *u) {
    return u->cheats && u->cheats->selected >= 0 &&
        (size_t)u->cheats->selected < u->cheats->profiles[u->cheats->player].count;
}
static void entry(cr_menu_row *r, const char *name, const char *hint, int icon, int action, int value) {
    memset(r, 0, sizeof(*r)); r->console = -1;
    snprintf(r->label, sizeof(r->label), "%s", name);
    display_copy(r->hint, sizeof(r->hint), hint ? hint : "");
    r->icon = icon; r->action = action; r->value = value;
}
int cr_menu_cheat_count(const cr_ui *u) {
    if (!cr_menu_cheat_page(u->page)) return -1;
    if (!u->cheats) return 0;
    if (u->page == CR_CHEAT_TEXT) return 0;
    if (u->page == CR_CHEAT_SETTINGS || u->page == CR_CHEAT_DELETE || u->page == CR_CHEAT_CLEAR) return 2;
    if (u->page == CR_CHEAT_ENTRY) return selected(u) ? 4 : 0;
    if (!formats(u)) return u->engine.players > 1 ? 3 : 2;
    return 7 + (u->engine.players > 1) + (int)u->cheats->profiles[u->cheats->player].count;
}
cr_menu_row cr_menu_cheat_entry(const cr_ui *u, int index) {
    cr_menu_row r; entry(&r, "", "", CR_ICON_CHEATS, ACT_NONE, 0);
    if (!u->cheats || index < 0 || index >= cr_menu_cheat_count(u)) return r;
    const struct cr_menu_cheats *s = u->cheats;
    const cr_cheat_profile *p = &s->profiles[s->player];
    if (u->page == CR_CHEAT_TEXT) return r;
    if (u->page == CR_CHEAT_SETTINGS) {
        entry(&r, index ? "Appliquer après modification" : "Appliquer au chargement",
              index ? "Appliquer les changements de la liste immédiatement" : "Reprendre les codes sélectionnés à la prochaine partie",
              CR_ICON_CHEATS, index ? ACT_CHEAT_AUTO_TOGGLE : ACT_CHEAT_AUTO_LOAD, 0);
        r.toggle = true; r.checked = index ? p->apply_after_toggle : p->apply_after_load;
    } else if (u->page == CR_CHEAT_DELETE || u->page == CR_CHEAT_CLEAR) {
        entry(&r, index ? "Annuler" : u->page == CR_CHEAT_DELETE ? "Supprimer ce code" : "Effacer toute la liste",
              index ? "Conserver les codes actuels" : "Cette modification sera enregistrée",
              index ? CR_ICON_BACK : CR_ICON_TRASH,
              index ? ACT_CHEAT_BACK : u->page == CR_CHEAT_DELETE ? ACT_CHEAT_DELETE_OK : ACT_CHEAT_CLEAR_OK, 0);
        r.destructive = !index;
    } else if (u->page == CR_CHEAT_ENTRY) {
        const cr_cheat_entry *c = &p->entries[s->selected];
        static const char *names[] = {"Activer ce code", "Nom", "Code", "Supprimer"};
        static const int actions[] = {ACT_CHEAT_ENABLE, ACT_CHEAT_NAME, ACT_CHEAT_CODE, ACT_CHEAT_DELETE};
        static const int icons[] = {CR_ICON_CHEATS, CR_ICON_RENAME, CR_ICON_CHEATS, CR_ICON_TRASH};
        entry(&r, names[index], index == 1 ? c->name : index == 2 ? c->code :
              index == 0 ? (s->dirty[s->player] ? "La liste contient des changements à appliquer" : "État de la sélection") : "Retirer ce code de la liste",
              icons[index], actions[index], 0);
        if (!index) { r.toggle = true; r.checked = c->enabled; }
        r.destructive = index == 3;
    } else if (!formats(u)) {
        if (!index) entry(&r, "Triche indisponible", "Ce cœur ne fournit pas encore cette fonction pour ce joueur",
                          CR_ICON_INFO, ACT_NONE, 0);
        else if (index == 1) entry(&r, "Mettre à jour le cœur", "Installez la mise à jour puis rechargez le cœur",
                                  CR_ICON_DOWNLOAD, ACT_CHEAT_UPDATE_CORE, 0);
        else entry(&r, "Joueur de la liste", s->player ? "Joueur 2" : "Joueur 1", CR_ICON_ACCOUNT, ACT_CHEAT_PLAYER, 0);
    } else if (!index) {
        char hint[256] = {0}; uint32_t f = formats(u);
        if (s->notice[0]) snprintf(hint, sizeof(hint), "%s", s->notice);
        else snprintf(hint, sizeof(hint), "%s", f & (CORVUS_CHEAT_FORMAT_GB_GAME_GENIE | CORVUS_CHEAT_FORMAT_GB_GAME_SHARK)
            ? "Game Genie et GameShark" : f & CORVUS_CHEAT_FORMAT_GBA_CODEBREAKER_RAW
            ? "CodeBreaker brut, GameShark et Action Replay ; variantes vérifiées par le cœur" : "Format propre au cœur");
        entry(&r, "Formats et état", hint, CR_ICON_INFO, ACT_NONE, 0);
    } else if (index < 7) {
        static const char *names[] = {"", "Appliquer les modifications", "Ajouter un code",
            "Remplacer depuis un .cht", "Exporter la liste .cht", "Réglages des codes", "Effacer la liste"};
        static const int actions[] = {ACT_NONE, ACT_CHEAT_APPLY, ACT_CHEAT_ADD, ACT_CHEAT_IMPORT,
            ACT_CHEAT_EXPORT, ACT_CHEAT_SETTINGS, ACT_CHEAT_CLEAR};
        static const int icons[] = {CR_ICON_INFO, CR_ICON_CHECK, CR_ICON_ADD, CR_ICON_IMPORT,
            CR_ICON_EXPORT, CR_ICON_SETTINGS, CR_ICON_TRASH};
        char hint[256] = {0};
        if (index == 1) snprintf(hint, sizeof(hint), s->dirty[s->player] ? "Modifications en attente ; %zu code(s) actuellement actif(s)" : "%zu code(s) actif(s)", cr_engine_cheat_count(&u->engine, s->player));
        else if (index == 3) snprintf(hint, sizeof(hint), "Charger une liste de codes gérés par le cœur");
        else if (index == 6) snprintf(hint, sizeof(hint), "Confirmation avant suppression de tous les codes");
        entry(&r, names[index], hint, icons[index], actions[index], 0);
        if (index == 2) r.disabled = p->count >= CR_CHEAT_MAX_ENTRIES || !u->platform.edit_text || !u->platform.submit_text;
        if (index == 3 || index == 4) r.disabled = !u->platform.cheat_file;
        if (!s->writable[s->player] && index != 3 && index != 6) r.disabled = true;
        r.destructive = index == 6;
    } else if (index == 7 && u->engine.players > 1) {
        entry(&r, "Joueur de la liste", s->player ? "Joueur 2" : "Joueur 1", CR_ICON_ACCOUNT, ACT_CHEAT_PLAYER, 0);
    } else {
        size_t which = (size_t)index - 7 - (u->engine.players > 1);
        if (which < p->count) {
            const cr_cheat_entry *c = &p->entries[which];
            entry(&r, c->name, c->enabled ? (s->dirty[s->player] ? "Activé dans la liste, modifications à appliquer" : "Activé") : (s->dirty[s->player] ? "Désactivé dans la liste, modifications à appliquer" : "Désactivé"),
                  CR_ICON_CHEATS, ACT_CHEAT_ENTRY, (int)which);
        }
    }
    if (s->request) r.disabled = true;
    return r;
}
static void edit(cr_ui *u, int field) {
    if (!selected(u) || !u->platform.edit_text || !u->platform.submit_text) return;
    const cr_cheat_entry *c = &u->cheats->profiles[u->cheats->player].entries[u->cheats->selected];
    snprintf(u->cheats->edit, sizeof(u->cheats->edit), "%s", field == 5 ? c->name : c->code);
    cr_menu_push_page(u, CR_CHEAT_TEXT);
    if (u->page != CR_CHEAT_TEXT) return;
    u->ui_editing = field;
    u->platform.edit_text(u->platform.context, field, u->cheats->edit, false);
}
bool cr_menu_cheats_cancel_edit(cr_ui *u) {
    if (u->ui_editing != 5 && u->ui_editing != 6) return false;
    u->ui_editing = 0;
    if (u->platform.edit_text) u->platform.edit_text(u->platform.context, 0, "", false);
    if (u->page == CR_CHEAT_TEXT) cr_menu_pop_page(u);
    return true;
}
const char *cr_menu_cheats_preview(const cr_ui *u) { return u->cheats ? u->cheats->edit : ""; }
bool cr_menu_cheats_text_result(cr_ui *u, int field, const char *text, bool accepted) {
    if (field != 5 && field != -5 && field != 6 && field != -6) return false;
    int number = field < 0 ? -field : field;
    if ((number != 5 && number != 6) || u->ui_editing != number || u->page != CR_CHEAT_TEXT || !selected(u)) return false;
    if (field > 0 && !accepted) { cr_menu_cheats_cancel_edit(u); return true; }
    size_t limit = number == 5 ? CR_CHEAT_NAME_SIZE : CR_CHEAT_CODE_SIZE;
    if (!text || strnlen(text, limit) >= limit) {
        message(u, "Texte trop long");
        if (field > 0) u->platform.edit_text(u->platform.context, number, u->cheats->edit, false);
        return true;
    }
    memmove(u->cheats->edit, text, strlen(text) + 1);
    if (field < 0) return true;
    cr_cheat_profile next = {0}; bool ok = copy(u, &next);
    if (ok) {
        cr_cheat_entry *c = &next.entries[u->cheats->selected];
        if (number == 5) {
            if (text[0]) memcpy(c->name, text, strlen(text) + 1);
            else snprintf(c->name, sizeof(c->name), "Code %d", u->cheats->selected + 1);
        }
        else {
            snprintf(c->code, sizeof(c->code), "%s", text);
            if ((c->enabled || c->code[0]) && !cr_engine_validate_cheat(&u->engine, u->cheats->player, c->code)) {
                message(u, u->engine.error); ok = false;
            }
        }
    }
    if (ok) ok = save(u, &next, number == 6);
    cr_cheat_profile_free(&next);
    if (ok) { cr_menu_cheats_cancel_edit(u); saved(u, number == 6); }
    else u->platform.edit_text(u->platform.context, number, u->cheats->edit, false);
    return true;
}
static void file_request(cr_ui *u, bool exporting) {
    struct cr_menu_cheats *s = u->cheats; char *bytes = NULL; size_t size = 0; char error[256] = {0};
    if (!u->platform.cheat_file || s->request) return;
    if (exporting && !cr_cheat_profile_encode(&s->profiles[s->player], &bytes, &size, error, sizeof(error))) {
        message(u, error); return;
    }
    uint64_t request = atomic_fetch_add(&file_nonce, 1) + 1;
    if (!request || request > INT64_MAX) { free(bytes); message(u, "Transfert indisponible"); return; }
    s->request = request; s->request_player = s->player; s->exporting = exporting;
    bool started = u->platform.cheat_file(u->platform.context, request, exporting, u->title, (const uint8_t *)bytes, size);
    free(bytes);
    if (!started) { cr_menu_cheats_cancel_file(u); message(u, "Impossible d'ouvrir le sélecteur de fichiers"); }
}
void cr_ui_cheat_file_result(cr_ui *u, const cr_cheat_file_result *result) {
    struct cr_menu_cheats *s = u->cheats;
    if (!s || !result || !result->request || s->request != result->request || s->player != s->request_player ||
        !u->engine.loaded || strcmp(s->ids[0], u->engine.id) || strcmp(s->ids[1], u->engine.second_id)) return;
    bool exporting = s->exporting; cr_menu_cheats_cancel_file(u);
    if (result->cancelled) return;
    if (result->error[0]) { message(u, result->error); return; }
    if (exporting) { message(u, "Liste de triche exportée"); return; }
    cr_cheat_profile next = {0}; char error[256] = {0};
    bool ok = cr_cheat_profile_parse(&next, (const char *)result->data, result->size, error, sizeof(error));
    if (!ok) message(u, error);
    for (size_t i = 0; ok && i < next.count; ++i) {
        if (next.entries[i].code[0] && !cr_engine_validate_cheat(&u->engine, s->player, next.entries[i].code)) {
            message(u, u->engine.error); ok = false;
        }
    }
    if (ok) ok = save(u, &next, true);
    cr_cheat_profile_free(&next);
    if (ok) { s->selected = -1; u->focus = 0; u->scroll = 0; saved(u, true); }
}
bool cr_menu_cheat_activate(cr_ui *u, cr_menu_row r) {
    if (r.action < ACT_CHEAT_APPLY || r.action > ACT_CHEAT_BACK) return false;
    if (r.disabled || !u->cheats || !u->engine.loaded || u->cheats->request || u->installing || u->save_request) return true;
    struct cr_menu_cheats *s = u->cheats;
    if (r.action == ACT_CHEAT_BACK) { cr_menu_pop_page(u); return true; }
    if (r.action == ACT_CHEAT_UPDATE_CORE) { cr_menu_open_core_details(u, u->engine.system); return true; }
    if (r.action == ACT_CHEAT_PLAYER) { s->player = s->player ? 0 : 1; s->selected = -1; u->focus = 0; u->scroll = 0; s->notice[0] = 0; return true; }
    if (!formats(u)) return true;
    if (r.action == ACT_CHEAT_APPLY) { apply(u); return true; }
    if (r.action == ACT_CHEAT_SETTINGS) { cr_menu_push_page(u, CR_CHEAT_SETTINGS); return true; }
    if (r.action == ACT_CHEAT_IMPORT || r.action == ACT_CHEAT_EXPORT) { file_request(u, r.action == ACT_CHEAT_EXPORT); return true; }
    if (r.action == ACT_CHEAT_ENTRY) {
        if (r.value >= 0 && (size_t)r.value < s->profiles[s->player].count) { s->selected = r.value; cr_menu_push_page(u, CR_CHEAT_ENTRY); }
        return true;
    }
    if (r.action == ACT_CHEAT_CLEAR) { cr_menu_push_page(u, CR_CHEAT_CLEAR); return true; }
    if (!s->writable[s->player] && r.action != ACT_CHEAT_CLEAR_OK) return true;
    if (r.action == ACT_CHEAT_NAME || r.action == ACT_CHEAT_CODE) { edit(u, r.action == ACT_CHEAT_NAME ? 5 : 6); return true; }
    if (r.action == ACT_CHEAT_DELETE) { if (selected(u)) cr_menu_push_page(u, CR_CHEAT_DELETE); return true; }
    cr_cheat_profile next = {0}; bool changed = false;
    if (r.action == ACT_CHEAT_CLEAR_OK) {
        next.apply_after_load = s->profiles[s->player].apply_after_load;
        next.apply_after_toggle = s->profiles[s->player].apply_after_toggle;
        if (save(u, &next, true)) { cr_menu_pop_page(u); s->selected = -1; apply(u); }
        cr_cheat_profile_free(&next); return true;
    }
    if (!copy(u, &next)) return true;
    bool ok = true;
    switch (r.action) {
    case ACT_CHEAT_ADD: {
        char name[64], error[256] = {0}; snprintf(name, sizeof(name), "Code %zu", next.count + 1);
        ok = cr_cheat_profile_append(&next, name, "", false, error, sizeof(error));
        if (!ok) message(u, error);
        break;
    }
    case ACT_CHEAT_ENABLE:
        if (!selected(u)) ok = false;
        else {
            cr_cheat_entry *c = &next.entries[s->selected];
            if (!c->enabled && !cr_engine_validate_cheat(&u->engine, s->player, c->code)) { message(u, u->engine.error); ok = false; }
            else { c->enabled = !c->enabled; changed = true; }
        }
        break;
    case ACT_CHEAT_DELETE_OK:
        ok = selected(u) && cr_cheat_profile_remove(&next, (size_t)s->selected); changed = true; break;
    case ACT_CHEAT_AUTO_LOAD: next.apply_after_load = !next.apply_after_load; break;
    case ACT_CHEAT_AUTO_TOGGLE: next.apply_after_toggle = !next.apply_after_toggle; break;
    default: ok = false; break;
    }
    if (ok) ok = save(u, &next, changed);
    cr_cheat_profile_free(&next);
    if (!ok) return true;
    if (r.action == ACT_CHEAT_ADD) { s->selected = (int)s->profiles[s->player].count - 1; cr_menu_push_page(u, CR_CHEAT_ENTRY); edit(u, 6); }
    else if (r.action == ACT_CHEAT_DELETE_OK) { cr_menu_pop_page(u); cr_menu_pop_page(u); s->selected = -1; saved(u, true); }
    else saved(u, changed);
    return true;
}
