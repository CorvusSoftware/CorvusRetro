// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
#include "frontend.h"
#include "play_layout.h"
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *cores[] = {"CorvusGB", "CorvusGBC", "CorvusGBA"};
static const char *systems[] = {"Game Boy", "Game Boy Color", "Game Boy Advance"};
static const char *quick_labels[] = {"Reprendre le contenu", "Redémarrer le contenu",
                                     "Sauvegarder l’état",   "Charger l’état",
                                     "Menu principal",       "Fermer le contenu"};
enum {
    ACT_NONE,
    ACT_CORES,
    ACT_UNLOAD,
    ACT_PICK,
    ACT_HISTORY,
    ACT_ABOUT,
    ACT_SYSTEM,
    ACT_GAME,
    ACT_THEME,
    ACT_AUDIO,
    ACT_AWAKE,
    ACT_LICENSE,
    ACT_SELECT,
    ACT_SET_THEME,
    ACT_RESUME
};
typedef struct {
    char label[512], hint[256];
    int icon, action, value;
    bool toggle, checked;
} row;
static float quick_height(const cr_ui *u);
static void quick_clamp(cr_ui *u);
static float body_bottom(const cr_ui *u) {
    return u->height - 100;
}
static int rows(const cr_ui *u) {
    if (u->page == CR_MAIN)
        return 4 + (u->engine.handle ? 1 : 0) + (u->engine.loaded ? 1 : 0);
    if (u->page == CR_LISTS || u->page == CR_CORES)
        return 3;
    if (u->page == CR_SETTINGS)
        return 5;
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
    if (u->page == CR_MAIN) {
        int n = 0;
        if (u->engine.loaded) {
            if (index == n++)
                fill(&r, "Reprendre le contenu", u->title, CR_ICON_MENU, ACT_RESUME, 0);
        }
        if (index == n++)
            fill(&r, "Charger un cœur", u->engine.handle ? cores[u->engine.system] : "Aucun cœur",
                 CR_ICON_CORE, ACT_CORES, 0);
        if (u->engine.handle) {
            if (index == n++)
                fill(&r, "Décharger le cœur", "Libérer le moteur", CR_ICON_UNLOAD, ACT_UNLOAD, 0);
        }
        if (index == n++)
            fill(&r, "Charger du contenu",
                 u->importing ? "Import du contenu…" : "Ouvrir un fichier .gb, .gbc ou .gba",
                 CR_ICON_CONTENT, ACT_PICK, 0);
        if (index == n++)
            fill(&r, "Historique", "Retrouver les contenus déjà ouverts", CR_ICON_HISTORY,
                 ACT_HISTORY, 0);
        if (index == n)
            fill(&r, "Informations", "CorvusRetro et les cœurs CorvusCore", CR_ICON_INFO, ACT_ABOUT,
                 0);
    } else if (u->page == CR_LISTS) {
        int n = 0;
        for (size_t i = 0; i < u->game_count; i++)
            if (u->games[i].system == index)
                n++;
        char hint[64];
        snprintf(hint, sizeof(hint), "%d contenu(s)", n);
        fill(&r, systems[index], hint, CR_ICON_LIST, ACT_SYSTEM, index);
    } else if (u->page == CR_CORES) {
        fill(&r, cores[index], systems[index], CR_ICON_CORE, ACT_SELECT, index);
        if (index == 2)
            snprintf(r.hint, sizeof(r.hint), "%s · Expérimental", systems[index]);
        r.checked = u->engine.handle && u->engine.system == index;
    } else if (u->page == CR_SETTINGS) {
        switch (index) {
        case 0:
            fill(&r, "Thème de l’interface", cr_theme_get(u->theme)->name, CR_ICON_THEME, ACT_THEME,
                 0);
            break;
        case 1:
            fill(&r, "Audio", "Activer le son pendant le jeu", CR_ICON_SETTINGS, ACT_AUDIO, 0);
            r.toggle = true;
            r.checked = u->audio;
            break;
        case 2:
            fill(&r, "Garder l’écran allumé", "Empêcher la veille pendant le jeu", CR_ICON_SETTINGS,
                 ACT_AWAKE, 0);
            r.toggle = true;
            r.checked = u->awake;
            break;
        case 3:
            fill(&r, "À propos de CorvusRetro", "Application native de CorvusSoftware",
                 CR_ICON_INFO, ACT_ABOUT, 0);
            break;
        case 4:
            fill(&r, "Licence et crédits", "AGPL-3.0-only · Code original", CR_ICON_FILE,
                 ACT_LICENSE, 0);
            break;
        }
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
                    fill(&r, u->games[i].title, cores[u->games[i].system], CR_ICON_FILE, ACT_GAME,
                         (int)i);
                    break;
                }
            }
    }
    return r;
}
static void clamp_scroll(cr_ui *u) {
    float content = (float)rows(u) * 76;
    if (u->page == CR_ABOUT || u->page == CR_LICENSE)
        content = fmaxf(400, u->text_height);
    float max = fmaxf(0, content - fmaxf(1, body_bottom(u) - 64));
    u->scroll = fminf(max, fmaxf(0, u->scroll));
}
static void page(cr_ui *u, int p) {
    u->page = p;
    u->scroll = 0;
    u->quick_scroll = 0;
    u->text_height = 0;
    u->focus = 0;
    u->quick = false;
    cr_ui_clear_input(u);
}
void cr_ui_message(cr_ui *u, const char *s) {
    snprintf(u->message, sizeof(u->message), "%s", s ? s : "");
    u->message_time = 5;
}
void cr_ui_reload(cr_ui *u) {
    size_t count = 0;
    cr_game *games = u->platform.games ? u->platform.games(u->platform.context, &count) : NULL;
    free(u->games);
    u->games = games;
    u->game_count = games ? count : 0;
    clamp_scroll(u);
}
void cr_ui_init(cr_ui *u, cr_platform p, const char *root) {
    memset(u, 0, sizeof(*u));
    u->platform = p;
    u->width = 384;
    u->height = 700;
    u->filter = -1;
    u->theme = CR_THEME_CORVUS;
    cr_engine_init(&u->engine, root);
    if (p.preference) {
        u->theme = cr_theme_valid(p.preference(p.context, "theme", CR_THEME_CORVUS));
        u->audio = p.preference(p.context, "audio", 1) != 0;
        u->awake = p.preference(p.context, "keep_awake", 1) != 0;
    } else {
        u->audio = true;
        u->awake = true;
    }
    cr_ui_reload(u);
}
void cr_ui_destroy(cr_ui *u) {
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
    return u->page == CR_PLAY && !u->quick && u->engine.loaded;
}
void cr_ui_update(cr_ui *u, double seconds) {
    u->message_time = fmax(0, u->message_time - seconds);
}
static bool result(cr_ui *u, bool ok) {
    if (!ok)
        cr_ui_message(u, u->engine.error);
    return ok;
}
static void open_game(cr_ui *u, int system, const char *id, const char *title, const char *path) {
    if (!result(u, cr_engine_open(&u->engine, system, id, path)))
        return;
    u->return_page = u->page == CR_PLAY ? CR_MAIN : u->page;
    snprintf(u->title, sizeof(u->title), "%s", title);
    page(u, CR_PLAY);
    if (u->platform.touch)
        u->platform.touch(u->platform.context, id);
    cr_ui_reload(u);
}
static void activate(cr_ui *u, row r) {
    switch (r.action) {
    case ACT_CORES:
        page(u, CR_CORES);
        break;
    case ACT_UNLOAD:
        if (result(u, cr_engine_close(&u->engine))) {
            u->title[0] = 0;
            page(u, CR_MAIN);
        }
        break;
    case ACT_PICK:
        if (!u->importing && u->platform.pick) {
            u->importing = true;
            u->platform.pick(u->platform.context, u->engine.handle ? u->engine.system : 0);
        }
        break;
    case ACT_HISTORY:
        u->filter = -1;
        page(u, CR_GAMES);
        break;
    case ACT_ABOUT:
        u->detail_parent = u->page;
        page(u, CR_ABOUT);
        break;
    case ACT_SYSTEM:
        u->filter = r.value;
        page(u, CR_GAMES);
        break;
    case ACT_GAME: {
        cr_game *g = &u->games[r.value];
        char path[4200];
        snprintf(path, sizeof(path), "%s/roms/%s.rom", u->engine.root, g->id);
        char title[512], id[68];
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
        page(u, CR_THEMES);
        break;
    case ACT_SET_THEME:
        u->theme = cr_theme_valid(r.value);
        if (u->platform.set_preference)
            u->platform.set_preference(u->platform.context, "theme", u->theme);
        page(u, CR_SETTINGS);
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
        u->detail_parent = u->page;
        if (!u->license && u->platform.credits)
            u->license = u->platform.credits(u->platform.context);
        page(u, CR_LICENSE);
        break;
    case ACT_RESUME:
        page(u, CR_PLAY);
        break;
    default:
        break;
    }
}
int cr_content_system(const char *title, int selected) {
    const char *extension = strrchr(title, '.');
    char suffix[8] = {0};
    if (extension && strlen(extension) < sizeof(suffix)) {
        for (size_t i = 0; extension[i]; i++)
            suffix[i] = (char)tolower((unsigned char)extension[i]);
    }
    if (strcmp(suffix, ".gbc") == 0)
        return 1;
    if (strcmp(suffix, ".gba") == 0)
        return 2;
    if (strcmp(suffix, ".gb") == 0 && selected == 2)
        return 0;
    return selected >= 0 && selected <= 2 ? selected : 0;
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
    bool id_ok = cr_game_id_valid(item->id);
    if (id_ok)
        resolved.id[65] = (char)('0' + resolved.system);
    bool valid = id_ok && cr_engine_check(&u->engine, resolved.system, resolved.path);
    char error[512] = {0};
    if (!valid)
        snprintf(error, sizeof(error), "%s",
                 u->engine.error[0] ? u->engine.error : "Contenu incompatible");
    bool committed = u->platform.finish_import &&
                     u->platform.finish_import(u->platform.context, valid, resolved.system, error,
                                               sizeof(error));
    if (!valid || !committed) {
        cr_ui_message(u, error);
        return;
    }
    cr_ui_reload(u);
    char path[4200];
    snprintf(path, sizeof(path), "%s/roms/%s.rom", u->engine.root, resolved.id);
    open_game(u, resolved.system, resolved.id, resolved.title, path);
}
static void quick_action(cr_ui *u, int i) {
    if (i == 0)
        u->quick = false;
    if (i == 1 && result(u, cr_engine_reset(&u->engine))) {
        u->quick = false;
        cr_ui_message(u, "Contenu redémarré");
    }
    if (i == 2 && result(u, cr_engine_save_state(&u->engine))) {
        u->quick = false;
        cr_ui_message(u, "État sauvegardé");
    }
    if (i == 3 && result(u, cr_engine_load_state(&u->engine))) {
        u->quick = false;
        cr_ui_message(u, "État chargé");
    }
    if (i == 4)
        page(u, CR_MAIN);
    if (i == 5 && result(u, cr_engine_close(&u->engine))) {
        u->title[0] = 0;
        page(u, u->return_page == CR_PLAY ? CR_MAIN : u->return_page);
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
    if (u->page == CR_ABOUT || u->page == CR_LICENSE) {
        page(u, u->detail_parent);
        return;
    }
    if (u->page == CR_GAMES) {
        page(u, u->filter < 0 ? CR_MAIN : CR_LISTS);
        return;
    }
    if (u->page == CR_THEMES) {
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
    int count = u->quick ? 6 : rows(u);
    if (count <= 0)
        return;
    u->focus = (u->focus + direction + count) % count;
    if (u->quick) {
        float top = u->focus * 56;
        float view = fmaxf(1, quick_height(u) - 56);
        if (top < u->quick_scroll)
            u->quick_scroll = top;
        if (top + 56 > u->quick_scroll + view)
            u->quick_scroll = top + 56 - view;
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
        u->keys |= mask;
        u->pulses |= mask;
    } else
        u->keys &= ~mask;
}
static cr_play_layout play_layout(const cr_ui *u) {
    return cr_play_layout_make(u->width, u->height, u->engine.system, u->engine.width, u->engine.height);
}
static unsigned button_at(const cr_ui *u, float x, float y) {
    cr_play_layout layout = play_layout(u);
    return cr_play_hit(&layout, x, y);
}
static float quick_height(const cr_ui *u) {
    return fminf(392, u->height - 16);
}
static void quick_clamp(cr_ui *u) {
    u->quick_scroll = fmaxf(0, fminf(u->quick_scroll, 336 - (quick_height(u) - 56)));
}
static int target(const cr_ui *u, float x, float y) {
    if (u->quick) {
        float w = fminf(360, u->width - 24), left = (u->width - w) / 2,
              top = (u->height - quick_height(u)) / 2;
        if (x < left || x > left + w || y < top || y > top + quick_height(u))
            return -4;
        if (y < top + 56)
            return -1;
        return 10000 + (int)((y - top - 56 + u->quick_scroll) / 56);
    }
    if (u->page == CR_PLAY) {
        cr_play_layout layout = play_layout(u);
        return cr_area_contains(layout.menu, x, y) ? -3 : -1;
    }
    if (y < 56)
        return x < 56 && u->page != CR_MAIN ? -2 : -1;
    if (y >= u->height - 72)
        return -100 - (int)fminf(2, fmaxf(0, floorf(x / u->width * 3)));
    if (y < 64 || y >= body_bottom(u))
        return -1;
    int i = (int)((y - 64 + u->scroll) / 76);
    return i < rows(u) ? i : -1;
}
void cr_ui_touch(cr_ui *u, int id, int phase, float x, float y) {
    if (id < 0 || id >= 32)
        return;
    if (phase == CR_TOUCH_CANCEL) {
        cr_ui_clear_input(u);
        return;
    }
    cr_pointer *p = &u->pointers[id];
    if (phase == CR_TOUCH_DOWN) {
        *p = (cr_pointer){.active = true, .x = x, .y = y, .last_y = y, .target = target(u, x, y)};
        if (cr_ui_running(u)) {
            p->mask = button_at(u, x, y);
            u->pulses |= p->mask;
        }
        return;
    }
    if (!p->active)
        return;
    if (phase == CR_TOUCH_MOVE) {
        if (fabsf(x - p->x) > 8 || fabsf(y - p->y) > 8)
            p->dragged = true;
        if (cr_ui_running(u)) {
            if (p->target == -3)
                return;
            unsigned mask = button_at(u, x, y);
            u->pulses |= mask & ~p->mask;
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
        p->last_y = y;
        return;
    }
    if (phase == CR_TOUCH_UP) {
        int t = p->target;
        bool click = !p->dragged && target(u, x, y) == t;
        p->active = false;
        p->mask = 0;
        if (!click)
            return;
        if (t >= 10000) {
            quick_action(u, t - 10000);
            return;
        }
        if (t == -4) {
            u->quick = false;
            cr_ui_clear_input(u);
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
    if (u->page == CR_PLAY) {
        rect(p, 0, 0, w, h, 0xff000000);
        cr_play_layout layout = play_layout(u);
        if (u->engine.loaded)
            p->game(p->context, u->engine.pixels, u->engine.width, u->engine.height,
                    layout.video.x, layout.video.y, layout.video.w, layout.video.h);
        if (layout.portrait)
            rect(p, layout.panel.x, layout.panel.y, layout.panel.w, layout.panel.h, bg);
        unsigned input = u->keys;
        bool menu_down = false;
        for (int i = 0; i < 32; i++)
            if (u->pointers[i].active) {
                input |= u->pointers[i].mask;
                menu_down |= u->pointers[i].target == -3 && !u->pointers[i].dragged;
            }
        uint32_t idle = layout.portrait ? theme->control : (theme->control & 0xffffff) | 0xb0000000;
        float cell = layout.dpad.w / 3;
        rect(p, layout.dpad.x + cell, layout.dpad.y + cell, cell, cell, idle);
        for (int i = 0; i < layout.count; i++) {
            const cr_control *b = &layout.controls[i];
            cr_area a = b->area;
            uint32_t c = input & b->mask ? accent : idle;
            if (i < 4)
                rect(p, a.x, a.y, a.w, a.h, c);
            else if (a.w == a.h)
                p->circle(p->context, a.x + a.w / 2, a.y + a.h / 2, a.h / 2, c);
            else {
                rect(p, a.x + a.h / 2, a.y, a.w - a.h, a.h, c);
                p->circle(p->context, a.x + a.h / 2, a.y + a.h / 2, a.h / 2, c);
                p->circle(p->context, a.x + a.w - a.h / 2, a.y + a.h / 2, a.h / 2, c);
            }
            float font = (strlen(b->label) > 3 ? 9 : i < 4 ? 18 : 23) * layout.scale;
            center(p, b->label, a.x + a.w / 2, a.y + a.h / 2 + font * .32f, font,
                   input & b->mask ? theme->control_pressed_text : theme->control_text);
        }
        cr_area menu = layout.menu;
        p->circle(p->context, menu.x + menu.w / 2, menu.y + menu.h / 2, menu.w / 2,
                  menu_down ? accent : idle);
        float icon = 24 * layout.scale;
        p->icon(p->context, CR_ICON_MENU, menu.x + (menu.w - icon) / 2,
                menu.y + (menu.h - icon) / 2, icon,
                menu_down ? theme->control_pressed_text : theme->control_text);
    } else {
        const char *title = "Menu principal";
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
        case CR_GAMES:
            title = u->filter < 0 ? "Historique" : systems[u->filter];
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
        default:
            break;
        }
        rect(p, 0, 0, w, 56, header);
        if (u->page != CR_MAIN)
            p->icon(p->context, CR_ICON_BACK, 12, 16, 24, theme->header_text);
        text(p, title, u->page == CR_MAIN ? 20 : 56, 35, 19, theme->header_text, w - 100);
        p->icon(p->context, CR_ICON_CROW, w - 44, 16, 24, theme->header_icon);
        if (p->clip)
            p->clip(p->context, 0, 56, w, fmaxf(1, body_bottom(u) - 56));
        int count = rows(u);
        for (int i = 0; i < count; i++) {
            float y = 64 + i * 76 - u->scroll;
            if (y + 76 < 56 || y > body_bottom(u))
                continue;
            row r = get_row(u, i);
            if (i == u->focus)
                rect(p, 0, y, w, 76, focus);
            p->icon(p->context, r.icon, 20, y + 22, 32, theme->icon);
            text(p, r.label, 66, y + 29, 16, fg, w - 110);
            text(p, r.hint, 66, y + 53, 13, muted, w - 110);
            if (r.toggle) {
                rect(p, w - 55, y + 31, 31, 14, r.checked ? accent : muted);
                p->circle(p->context, w - (r.checked ? 26 : 48), y + 38, 10,
                          r.checked ? accent : fg);
            } else if (r.checked)
                text(p, "✓", w - 36, y + 41, 20, accent, 28);
            else
                p->icon(p->context, CR_ICON_NEXT, w - 36, y + 29, 18, muted);
            rect(p, 66, y + 75, w - 66, 1, line);
        }
        if (u->page == CR_GAMES && !count) {
            text(p,
                 u->filter < 0 ? "Aucun contenu dans l’historique."
                               : "Aucun contenu dans cette liste.",
                 20, 106, 15, fg, w - 40);
            text(p, "Utilisez Charger du contenu.", 20, 137, 13, muted, w - 40);
        }
        if (u->page == CR_ABOUT)
            u->text_height =
                paragraphs(p,
                           "CorvusRetro 0.8.0\n\nFrontend original en C, rendu OpenGL ES et audio "
                           "AAudio.\n\nCorvusSoftware\nCœurs CorvusCore 0.14.0 : Game Boy, Game "
                           "Boy Color, Game Boy Advance (expérimental).\n\nNavigation inspirée de "
                           "MaterialUI. Thèmes CorvusRetro, ASUS, NVIDIA Shield et "
                           "sombre.\n\nCopyright (C) 2026 "
                           "CorvusSoftware\nSPDX-License-Identifier: AGPL-3.0-only",
                           20, 92, w - 40, 24, fg, u->scroll);
        if (u->page == CR_LICENSE)
            u->text_height = paragraphs(p, u->license ? u->license : "Licence indisponible", 20, 92,
                                        w - 40, 23, fg, u->scroll);
        if (p->clip)
            p->clip(p->context, 0, 0, w, h);
        // Cover scrolled content before drawing the fixed header and navigation.
        rect(p, 0, 0, w, 56, header);
        if (u->page != CR_MAIN)
            p->icon(p->context, CR_ICON_BACK, 12, 16, 24, theme->header_text);
        text(p, title, u->page == CR_MAIN ? 20 : 56, 35, 19, theme->header_text, w - 100);
        p->icon(p->context, CR_ICON_CROW, w - 44, 16, 24, theme->header_icon);
        rect(p, 0, body_bottom(u), w, 100, bg);
        char footer[160];
        if (u->engine.handle)
            snprintf(footer, sizeof(footer), "Cœur chargé : %s · %s", cores[u->engine.system],
                     CR_CORE_VERSION);
        else
            snprintf(footer, sizeof(footer), "Aucun cœur");
        text(p, footer, 20, h - 84, 11, muted, w - 40);
        rect(p, 0, h - 72, w, 72, nav);
        rect(p, 0, h - 72, w, 1, line);
        const char *labels[] = {"Menu principal", "Listes de lecture", "Réglages"};
        int icons[] = {CR_ICON_CROW, CR_ICON_LIST, CR_ICON_SETTINGS};
        int selected = u->page == CR_LISTS || u->page == CR_GAMES       ? 1
                       : u->page == CR_SETTINGS || u->page == CR_THEMES ? 2
                                                                        : 0;
        for (int i = 0; i < 3; i++) {
            float x = w * (i + .5f) / 3;
            uint32_t c = i == selected ? theme->navigation_active : theme->navigation_passive;
            p->icon(p->context, icons[i], x - 12, h - 61, 24, c);
            center(p, labels[i], x, h - 17, 10, c);
        }
    }
    if (u->quick) {
        rect(p, 0, 0, w, h, 0xb0000000);
        float qh = quick_height(u), qw = fminf(360, w - 24), x = (w - qw) / 2, y = (h - qh) / 2;
        rect(p, x, y, qw, qh, theme->surface);
        if (p->clip)
            p->clip(p->context, x, y + 56, qw, qh - 56);
        for (int i = 0; i < 6; i++) {
            float top = y + 56 + i * 56 - u->quick_scroll;
            if (top + 56 < y + 56 || top > y + qh)
                continue;
            if (i == u->focus)
                rect(p, x, top, qw, 56, focus);
            text(p, quick_labels[i], x + 20, top + 35, 16, fg, qw - 40);
        }
        if (p->clip)
            p->clip(p->context, 0, 0, w, h);
        rect(p, x, y, qw, 56, header);
        text(p, "Menu rapide", x + 20, y + 35, 20, theme->header_text, qw - 40);
    }
    if (u->message_time > 0 && u->message[0]) {
        float y = u->page == CR_PLAY ? h - 108 : h - 142;
        rect(p, 12, y, w - 24, 52, 0xf0333333);
        text(p, u->message, 24, y + 32, 13, 0xffffffff, w - 48);
    }
}
