// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
#include "interface_draw.h"
#include "interface_layout.h"
#include "widgets.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

static void text(const cr_painter *p, const char *s, float x, float y, float size, uint32_t c, float max) {
    if (p->text && s && max > 0) p->text(p->context, s, x, y, size, c, max);
}
static void icon(const cr_painter *p, int id, float x, float y, float size, uint32_t color) {
    if (p->icon) p->icon(p->context, id, x, y, size, color);
}
static void clip(const cr_painter *p, float x, float y, float w, float h) {
    if (p->clip) p->clip(p->context, x, y, fmaxf(1, w), fmaxf(1, h));
}
static void close_icon(const cr_painter *p, float x, float y, uint32_t color) {
    icon(p, CR_ICON_CLOSE, x - 9, y - 9, 18, color);
}
static void overflow(const cr_painter *p, float x, float y, uint32_t color) {
    icon(p, CR_ICON_MORE, x - 10, y - 10, 20, color);
}
static void search_icon(const cr_painter *p, float x, float y, uint32_t color) {
    icon(p, CR_ICON_SEARCH, x - 11, y - 11, 22, color);
}
static void tick(const cr_painter *p, float x, float y, uint32_t color) {
    icon(p, CR_ICON_CHECK, x - 10, y - 10, 20, color);
}
static bool image(const cr_painter *p, const char *key, cr_menu_rect r) {
    return p->image && key && key[0] && p->image(p->context, key, r.x, r.y, r.w, r.h, 1);
}
static void thumbnail(const cr_painter *p, const cr_menu_row *row, cr_menu_rect r, const cr_theme *theme) {
    cr_widget_round(p, r, 10, cr_widget_mix(theme->surface, theme->accent, .16f));
    if (!image(p, row->image, (cr_menu_rect){r.x + 2, r.y + 2, r.w - 4, r.h - 4})) {
        if (row->console >= 0) cr_widget_console(p, (cr_menu_rect){r.x + 6, r.y + 5, r.w - 12, r.h - 10}, row->console, theme);
        else icon(p, row->icon, r.x + r.w / 2 - 14, r.y + r.h / 2 - 14, 28, theme->icon);
    }
}
static float pressing(const cr_ui *u, int target) {
    return u->feedback_target == target ? fminf(1, fmaxf(0, u->feedback_alpha)) : 0;
}
static float selected(const cr_ui *u, int row) {
    if (row == u->focus) return 1;
    return pressing(u, u->quick ? 10000 + row : row);
}
static void toggle(const cr_painter *p, const cr_theme *theme, float x, float y, bool checked) {
    cr_menu_rect track = {x, y, 48, 27};
    uint32_t off = cr_widget_mix(theme->surface, theme->muted, .38f);
    cr_widget_gradient(p, track, 13.5f,
        checked ? cr_widget_mix(theme->accent, 0xffffffff, .20f) : off,
        checked ? cr_widget_mix(theme->accent, theme->focus, .20f) : off);
    if (p->circle) {
        float center = x + (checked ? 34.5f : 13.5f);
        p->circle(p->context, center, y + 14.5f, 10, 0x33000000);
        p->circle(p->context, center, y + 13.5f, 10, checked ? 0xffffffff : cr_widget_mix(theme->text, 0xffffffff, .7f));
    }
}
static void navigation(const cr_ui *u, const cr_painter *p, const cr_theme *theme) {
    if (p->rect) p->rect(p->context, 0, u->height - 64, u->width, 64, theme->navigation);
    cr_widget_line(p, 0, u->height - 64, u->width, u->height - 64, 1,
                   cr_widget_mix(theme->navigation, theme->divider, .5f));
    static const char *labels[] = {"Accueil", "Jeux", "Saves", "Cœurs", "Réglages"};
    static const int icons[] = {CR_ICON_HOME, CR_ICON_GRID, CR_ICON_FOLDER, CR_ICON_CORE, CR_ICON_SETTINGS};
    for (int i = 0; i < 5; ++i) {
        cr_menu_rect area = cr_interface_navigation(u, i);
        float center = area.x + area.w / 2;
        bool active = i == u->tab;
        float level = active ? 1 : pressing(u, -100 - i);
        if (level > 0) {
            float width = fminf(54, area.w - 6);
            cr_widget_gradient(p, (cr_menu_rect){center - width / 2, area.y + 5, width, 54}, 15,
                cr_widget_mix(theme->navigation, u->theme == CR_THEME_ASUS ? 0xff373737 : cr_widget_mix(theme->focus, theme->accent, .36f), level),
                cr_widget_mix(theme->navigation, u->theme == CR_THEME_ASUS ? 0xff151515 : theme->focus, level));
        }
        uint32_t color = cr_widget_mix(theme->navigation_passive, theme->navigation_active, level);
        icon(p, icons[i], center - 11, area.y + 12, 22, color);
        cr_widget_center(p, labels[i], center, area.y + 52, 10.5f, color, area.w - 4);
    }
}
static void header(const cr_ui *u, const cr_painter *p, const cr_theme *theme, const char *title, bool back) {
    if (p->rect) p->rect(p->context, 0, 0, u->width, 56, theme->header);
    if (u->page == CR_MAIN) {
        icon(p, CR_ICON_CROW, 16, 7, 40, theme->header_icon);
        text(p, "Corvus", 64, 35, 21, theme->header_text, u->width - 168);
        float width = p->measure ? p->measure(p->context, "Corvus", 21) : 70;
        text(p, "Retro", 64 + width, 35, 21,
             u->theme == CR_THEME_ASUS ? theme->header_text : theme->accent, u->width - 120 - width);
        return;
    }
    if (back) {
        if (p->circle && pressing(u, -2) > 0)
            p->circle(p->context, 26, 28, 20, cr_widget_mix(theme->header, theme->focus, pressing(u, -2)));
        icon(p, CR_ICON_BACK, 14, 16, 24, theme->header_text);
    }
    text(p, title, back ? 52 : 18, 35, 20, theme->header_text,
         u->width - (back ? 64 : 36) - (u->page == CR_LIBRARY ? 80 : u->page == CR_CHEAT_TEXT ? 52 : 0));
    if (u->page == CR_CHEAT_TEXT) {
        if (p->circle && pressing(u, -302) > 0)
            p->circle(p->context, u->width - 27, 28, 20, theme->focus);
        icon(p, CR_ICON_CHECK, u->width - 39, 16, 24, theme->header_text);
    }
    if (u->page == CR_LIBRARY) {
        uint32_t bg = cr_widget_mix(theme->header, theme->focus, pressing(u, -200));
        if (p->circle && pressing(u, -200) > 0) p->circle(p->context, u->width - 72, 28, 20, bg);
        search_icon(p, u->width - 72, 28, theme->header_text);
        if (p->circle && pressing(u, -201) > 0)
            p->circle(p->context, u->width - 27, 28, 20, cr_widget_mix(theme->header, theme->focus, pressing(u, -201)));
        overflow(p, u->width - 27, 28, theme->header_text);
    }
}
static void mountain(const cr_painter *p, float x, float top, float base, float half, uint32_t color) {
    if (!p->rect || base <= top) return;
    for (float y = top; y < base; y += 3) {
        float width = half * (y - top + 3) / (base - top);
        p->rect(p->context, x - width, y, width * 2, fminf(3, base - y), color);
    }
}
static void hero(const cr_ui *u, const cr_painter *p, const cr_theme *theme) {
    cr_menu_rect r = cr_interface_home_hero(u);
    bool compact = u->height < 500 || u->width > u->height;
    float height = compact ? r.h : r.h - 86;
    cr_widget_card(p, r, theme, false);
    cr_widget_gradient(p, (cr_menu_rect){r.x + 1, r.y + 1, r.w - 2, r.h - 2}, 14,
                       cr_widget_mix(theme->background, theme->accent, .25f), theme->background);
    float art_top = fmaxf(56, r.y + 3), art_bottom = fminf(cr_interface_bottom(u), r.y + height - 3);
    if (art_bottom > art_top) {
        clip(p, r.x + 3, art_top, r.w - 6, art_bottom - art_top);
        float width = compact ? r.w - 4 : fmaxf(r.w - 4, (height - 4) * 512 / 224);
        cr_menu_rect art = compact ? (cr_menu_rect){r.x + 2, r.y + 2, r.w - 4, r.h - 4}
            : (cr_menu_rect){r.x + (r.w - width) / 2, r.y + height - 2 - width * 224 / 512,
                             width, width * 224 / 512};
        if (!image(p, "asset:home", art)) {
            for (int i = 0; i < 14; ++i) {
                float x = r.x + 16 + fmodf(i * 47.0f, fmaxf(1, r.w - 30));
                float y = r.y + 12 + fmodf(i * 19.0f, height * .4f);
                if (p->circle) p->circle(p->context, x, y, i % 3 ? .8f : 1.2f,
                                         cr_widget_mix(theme->accent, 0xffffffff, .6f));
            }
            mountain(p, r.x + r.w * .28f, r.y + height * .3f, r.y + height - 4, r.w * .36f,
                     cr_widget_mix(theme->background, theme->accent, .29f));
            mountain(p, r.x + r.w * .56f, r.y + height * .42f, r.y + height - 4, r.w * .4f,
                     cr_widget_mix(theme->background, theme->accent, .17f));
            mountain(p, r.x + r.w * .14f, r.y + height * .58f, r.y + height - 4, r.w * .21f, theme->background);
            cr_widget_console(p, (cr_menu_rect){r.x + r.w * .68f, r.y + 11, r.w * .26f, height * .63f}, 1, theme);
        }
    }
    clip(p, 0, 56, u->width, cr_interface_bottom(u) - 56);
    cr_widget_gradient(p, (cr_menu_rect){r.x + 2, r.y + height * (compact ? .48f : .72f),
                       r.w - 4, height * (compact ? .5f : .28f)}, 0,
                       theme->background & 0x00ffffff, theme->background);
    text(p, "Bienvenue !", r.x + 14, r.y + r.h - (compact ? 36 : 62), 23, theme->text, r.w - 28);
    if (compact) text(p, "Retrouvez vos jeux et vos sauvegardes.", r.x + 14, r.y + r.h - 14,
                      12, theme->muted, r.w - 28);
    else {
        text(p, "Reprenez votre dernière partie", r.x + 14, r.y + r.h - 38, 12, theme->muted, r.w - 28);
        text(p, "ou explorez votre bibliothèque.", r.x + 14, r.y + r.h - 20, 12, theme->muted, r.w - 28);
    }
}
static void home_row(const cr_ui *u, const cr_painter *p, const cr_theme *theme, cr_menu_row row, int index) {
    cr_menu_rect r = cr_interface_row_bounds(u, index);
    cr_widget_card(p, r, theme, selected(u, index));
    uint32_t fg = row.disabled ? theme->muted : theme->text;
    float center = r.y + r.h / 2;
    if (index >= 1 && index <= 3) {
        icon(p, row.icon, r.x + r.w / 2 - 16, center - 28, 32, theme->icon);
        cr_widget_center(p, row.label, r.x + r.w / 2, center + 27, 12, fg, r.w - 8);
        return;
    }
    float thumb = fminf(index == 0 ? 80 : 56, r.h - 16), left = thumb + 22;
    thumbnail(p, &row, (cr_menu_rect){r.x + 10, center - thumb / 2, thumb, thumb}, theme);
    text(p, row.label, r.x + left, center - 8, 16, fg, r.w - left - 50);
    text(p, row.hint, r.x + left, center + 14, 13, theme->muted, r.w - left - 50);
    if (index == 0) {
        if (p->circle) p->circle(p->context, r.x + r.w - 28, center, 20, row.disabled ? theme->divider : theme->accent);
        icon(p, CR_ICON_PLAY, r.x + r.w - 38, center - 10, 20, row.disabled ? theme->muted : 0xffffffff);
    } else icon(p, CR_ICON_NEXT, r.x + r.w - 34, center - 10, 20, theme->muted);
}
static void library_controls(const cr_ui *u, const cr_painter *p, const cr_theme *theme) {
    if (p->rect) p->rect(p->context, 0, 56, u->width, cr_interface_content_top(u) - 56, theme->background);
    static const char *labels[] = {"Tous", "GB", "GBC", "GBA"};
    for (int i = 0; i < 4; ++i) {
        cr_menu_rect r = cr_interface_chip(u, i);
        bool active = u->library_filter + 1 == i;
        cr_widget_gradient(p, r, 15,
            active ? cr_widget_mix(theme->accent, 0xffffffff, .24f) : cr_widget_mix(theme->surface, theme->accent, .1f + pressing(u, -210 - i) * .3f),
            active ? theme->accent : cr_widget_mix(theme->surface, theme->accent, pressing(u, -210 - i) * .3f));
        cr_widget_center(p, labels[i], r.x + r.w / 2, r.y + 20, 12,
                          active ? (u->theme == CR_THEME_ASUS ? 0xffffffff : 0xff160a2a) : theme->text, r.w - 8);
    }
    if (u->library_search || u->library_query[0]) {
        cr_menu_rect r = cr_interface_search(u);
        cr_widget_round(p, r, 12, cr_widget_mix(theme->accent, theme->text, .15f));
        cr_widget_round(p, (cr_menu_rect){r.x + 1, r.y + 1, r.w - 2, r.h - 2}, 11, theme->surface);
        search_icon(p, r.x + 19, r.y + 22, theme->text);
        text(p, u->library_query[0] ? u->library_query : "Rechercher un jeu...", r.x + 37, r.y + 28,
             14, u->library_query[0] ? theme->text : theme->muted, r.w - 78);
        close_icon(p, r.x + r.w - 21, r.y + 22, cr_widget_mix(theme->muted, theme->text, pressing(u, -202)));
    }
}
static const char *console_label(int console) {
    static const char *labels[] = {"GB", "GBC", "GBA"};
    return console >= 0 && console < 3 ? labels[console] : "Jeu";
}
static void game_card(const cr_ui *u, const cr_painter *p, const cr_theme *theme, cr_menu_row row, int index) {
    cr_menu_rect r = cr_interface_row_bounds(u, index);
    cr_widget_card(p, r, theme, selected(u, index));
    if (u->library_list || u->library_query[0]) {
        thumbnail(p, &row, (cr_menu_rect){r.x + 8, r.y + 8, 56, 56}, theme);
        text(p, row.label, r.x + 76, r.y + 29, 15, theme->text, r.w - 120);
        text(p, row.hint, r.x + 76, r.y + 51, 12, theme->muted, r.w - 120);
        overflow(p, r.x + r.w - 22, r.y + r.h / 2, theme->text);
        return;
    }
    cr_menu_rect art = {r.x + 5, r.y + 5, r.w - 10, r.h - 51};
    cr_widget_gradient(p, art, 10, cr_widget_mix(theme->surface, theme->accent, .3f), theme->surface);
    if (!image(p, row.image, art))
        cr_widget_console(p, (cr_menu_rect){art.x + art.w * .12f, art.y + art.h * .12f,
                                          art.w * .76f, art.h * .76f}, row.console, theme);
    cr_widget_round(p, (cr_menu_rect){r.x + 10, r.y + 10, 35, 19}, 6, 0xd6111026);
    text(p, console_label(row.console), r.x + 16, r.y + 24, 10, 0xffffffff, 30);
    if (p->circle) p->circle(p->context, r.x + r.w - 22, r.y + 22, 12,
        cr_widget_mix(0xa8111026, theme->accent, pressing(u, -5000 - index)));
    overflow(p, r.x + r.w - 22, r.y + 22, 0xffffffff);
    text(p, row.label, r.x + 11, r.y + r.h - 25, 14, theme->text, r.w - 22);
    text(p, row.hint, r.x + 11, r.y + r.h - 9, 10, theme->muted, r.w - 22);
}
static bool has_overflow(const cr_ui *u, const cr_menu_row *row, int index) {
    return (u->page == CR_CORES && row->icon == CR_ICON_CORE) ||
           (u->page == CR_STATE_LIST && index > 0 && (size_t)index <= u->saved_state_count);
}
static void primary(const cr_ui *u, const cr_painter *p, const cr_theme *theme, cr_menu_row row, int index) {
    cr_menu_rect r = cr_interface_row_bounds(u, index);
    uint32_t color = row.disabled ? theme->divider : cr_widget_mix(theme->accent,
        cr_widget_mix(theme->accent, theme->focus, .25f), selected(u, index));
    cr_widget_round(p, r, 16, cr_widget_mix(color, 0xffffffff, .35f));
    cr_widget_gradient(p, (cr_menu_rect){r.x + 1, r.y + 1, r.w - 2, r.h - 2}, 15,
                       cr_widget_mix(color, 0xffffffff, .25f), color);
    uint32_t fg = row.disabled ? theme->muted : u->theme == CR_THEME_ASUS ? 0xffffffff : 0xff180b2c;
    float width = p->measure ? p->measure(p->context, row.label, 14) : strlen(row.label) * 7;
    width = fminf(width, r.w - 60);
    float start = r.x + (r.w - width - 28) / 2;
    icon(p, row.icon, start, r.y + r.h / 2 - 10, 20, fg);
    text(p, row.label, start + 28, r.y + r.h / 2 + 5, 14, fg, r.w - (start - r.x) - 36);
}
static void regular_row(const cr_ui *u, const cr_painter *p, const cr_theme *theme, cr_menu_row row, int index) {
    if ((u->page == CR_STATE_LIST && index == 0) ||
        ((u->page == CR_CORES || u->page == CR_CORE_DETAILS) && row.icon == CR_ICON_DOWNLOAD)) {
        primary(u, p, theme, row, index); return;
    }
    cr_menu_rect r = cr_interface_row_bounds(u, index);
    cr_widget_card(p, r, theme, fmaxf(selected(u, index), row.checked ? 1 : 0));
    bool photo = row.image[0] || row.console >= 0;
    if (photo) thumbnail(p, &row, (cr_menu_rect){r.x + 8, r.y + 8, 52, 52}, theme);
    else {
        cr_widget_round(p, (cr_menu_rect){r.x + 10, r.y + 14, 40, 40}, 13,
                         cr_widget_mix(theme->surface, theme->accent, .1f));
        icon(p, row.icon, r.x + 17, r.y + 21, 26, row.disabled ? theme->muted : theme->icon);
    }
    float left = r.x + (photo ? 72 : 62);
    bool dots = has_overflow(u, &row, index);
    float right = row.toggle ? 68 : dots && row.checked ? 90 : 48;
    uint32_t fg = row.destructive ? (u->theme == CR_THEME_ASUS ? 0xffba1742 : 0xfff56b93)
                  : row.disabled ? theme->muted : theme->text;
    text(p, row.label, left, r.y + 28, 16, fg, fmaxf(1, r.x + r.w - right - left));
    text(p, row.hint, left, r.y + 50, 13, theme->muted, fmaxf(1, r.x + r.w - right - left));
    if (row.toggle) toggle(p, theme, r.x + r.w - 60, r.y + 21, row.checked);
    else if (row.checked) {
        float x = r.x + r.w - (dots ? 62 : 26);
        tick(p, x, r.y + r.h / 2, theme->accent);
    }
    if (dots) {
        float level = pressing(u, -5000 - index);
        if (p->circle && level > 0) p->circle(p->context, r.x + r.w - 22, r.y + r.h / 2, 14,
            cr_widget_mix(theme->surface, theme->accent, level * .35f));
        overflow(p, r.x + r.w - 22, r.y + r.h / 2, theme->muted);
    }
    else if (!row.toggle && !row.checked && row.action)
        icon(p, CR_ICON_NEXT, r.x + r.w - 32, r.y + 25, 18, theme->muted);
}
static void metadata(const cr_ui *u, const cr_painter *p, const cr_theme *theme, cr_menu_row row) {
    if (u->page == CR_STATE_LIST) {
        cr_menu_rect r = {16, 64 - u->scroll, u->width - 32, 78};
        cr_widget_card(p, r, theme, false);
        thumbnail(p, &row, (cr_menu_rect){r.x + 8, r.y + 9, 60, 60}, theme);
        text(p, row.label, r.x + 80, r.y + 32, 16, theme->text, r.w - 94);
        text(p, row.hint, r.x + 80, r.y + 55, 13, theme->muted, r.w - 94);
    } else if (u->page == CR_CORE_DETAILS) {
        char lines[3][256] = {{0}};
        const char *start = row.hint;
        for (int i = 0; i < 3 && *start; ++i) {
            const char *end = strchr(start, '\n');
            size_t length = end ? (size_t)(end - start) : strlen(start);
            if (length >= sizeof(lines[i])) length = sizeof(lines[i]) - 1;
            memcpy(lines[i], start, length);
            if (!end) break;
            start = end + 1;
        }
        float top = 70 - u->scroll;
        if (u->height < 500) {
            cr_widget_console(p, (cr_menu_rect){24, top + 4, 120, 88}, row.console, theme);
            text(p, row.label, 164, top + 29, 22, theme->text, u->width - 180);
            for (int i = 0; i < 3; ++i)
                text(p, lines[i], 164, top + 51 + i * 16, 12, theme->muted, u->width - 180);
            if (row.checked) {
                uint32_t success = u->theme == CR_THEME_ASUS ? 0xff147b42 : 0xff51ef91;
                text(p, "Déjà installé", 164, top + 101, 12, success, u->width - 180);
            }
            return;
        }
        cr_widget_console(p, (cr_menu_rect){u->width / 2 - 82, top + 12, 164, 96}, row.console, theme);
        cr_widget_center(p, row.label, u->width / 2, top + 142, 24, theme->text, u->width - 32);
        for (int i = 0; i < 3; ++i)
            cr_widget_center(p, lines[i], u->width / 2, top + 164 + i * 16, 12, theme->muted, u->width - 32);
        if (row.checked) {
            cr_menu_rect status = {24, top + 201, u->width - 48, 26};
            uint32_t success = u->theme == CR_THEME_ASUS ? 0xff147b42 : 0xff51ef91;
            cr_widget_round(p, status, 17, success);
            cr_widget_round(p, (cr_menu_rect){status.x + 1, status.y + 1, status.w - 2, status.h - 2}, 16,
                             cr_widget_mix(theme->background, success, .10f));
            cr_widget_center(p, "Déjà installé", u->width / 2, status.y + 18, 12, success, status.w - 20);
        }
    }
}
static void empty(const cr_ui *u, const cr_painter *p, const cr_theme *theme, int count) {
    const char *title = NULL, *hint = NULL;
    if (u->page == CR_LIBRARY && count <= 1) {
        title = u->library_query[0] ? "Aucun jeu ne correspond" : "Votre bibliothèque vous attend";
        hint = u->library_query[0] ? "Essayez un autre titre ou une autre console." : "Importez vos jeux depuis le menu en haut.";
    } else if (u->page == CR_SAVES_LIBRARY && u->game_count == 0 && !u->engine.loaded) {
        title = "Vos sauvegardes, au même endroit";
        hint = "Importez un jeu pour retrouver ses états ici.";
    } else if (u->page == CR_STATE_LIST && count <= 1) {
        title = "Aucun état enregistré";
        hint = "Créez un état pendant une partie.";
    } else if (u->page == CR_CORE_DOWNLOADS && count <= 1) {
        title = u->catalog_loading ? "Recherche des cœurs..." : "Aucun cœur disponible";
        hint = u->catalog_loading ? "Le catalogue est en cours de chargement." : "Actualisez le catalogue pour réessayer.";
    }
    if (!title) return;
    float y = cr_interface_content_top(u) + (u->page == CR_STATE_LIST ? 110 :
        u->page == CR_SAVES_LIBRARY ? 190 : 120) - u->scroll;
    int symbol = u->page == CR_CORE_DOWNLOADS ? CR_ICON_CORE :
                 u->page == CR_LIBRARY ? (u->library_query[0] ? CR_ICON_SEARCH : CR_ICON_GRID) : CR_ICON_SAVES;
    icon(p, symbol, u->width / 2 - 32, y - 70, 64, theme->icon);
    cr_widget_center(p, title, u->width / 2, y + 28, 16, theme->text, u->width - 32);
    cr_widget_center(p, hint, u->width / 2, y + 54, 12, theme->muted, u->width - 32);
}
static void panel(const cr_ui *u, const cr_painter *p, const cr_theme *theme, const char *title,
                  int count, cr_menu_row_reader reader) {
    if (p->rect) p->rect(p->context, 0, 0, u->width, u->height, 0x99020212);
    cr_menu_rect r = cr_interface_panel(u);
    cr_widget_card(p, r, theme, true);
    text(p, title, r.x + 18, r.y + 35, 19, theme->text, r.w - 72);
    close_icon(p, r.x + r.w - 25, r.y + 28, theme->muted);
    if (!u->quick) {
        cr_menu_row meta = reader(u, -1);
        if (u->height < 500) {
            thumbnail(p, &meta, (cr_menu_rect){r.x + 18, r.y + 61, 100, 64}, theme);
            text(p, meta.label, r.x + 134, r.y + 84, 15, theme->text, r.w - 152);
            text(p, meta.hint, r.x + 134, r.y + 107, 12, theme->muted, r.w - 152);
        } else {
            cr_menu_rect preview = {r.x + r.w / 2 - 74, r.y + 59, 148, 94};
            thumbnail(p, &meta, preview, theme);
            cr_widget_center(p, meta.label, r.x + r.w / 2, r.y + 177, 15, theme->text, r.w - 36);
            cr_widget_center(p, meta.hint, r.x + r.w / 2, r.y + 198, 12, theme->muted, r.w - 36);
        }
    }
    float top = cr_interface_content_top(u), bottom = cr_interface_bottom(u);
    clip(p, r.x + 8, top, r.w - 16, bottom - top);
    for (int i = 0; i < count; ++i) {
        cr_menu_rect row = cr_interface_row_bounds(u, i);
        if (row.y + row.h <= top) continue;
        if (row.y >= bottom) break;
        cr_menu_row value = reader(u, i);
        bool danger = value.destructive || value.icon == CR_ICON_CLOSE;
        uint32_t color = danger ? (u->theme == CR_THEME_ASUS ? 0xffba1742 : 0xfff56b93)
                         : value.disabled ? theme->muted : theme->text;
        uint32_t surface = cr_widget_mix(cr_widget_mix(theme->surface, theme->background, .18f),
                                         theme->focus, selected(u, i));
        cr_widget_round(p, row, 9, surface);
        icon(p, value.icon, row.x + 11, row.y + row.h / 2 - 10, 20, color);
        text(p, value.label, row.x + 42, row.y + row.h / 2 + 5, 14, color, row.w - 56);
    }
    clip(p, 0, 0, u->width, u->height);
}
void cr_interface_draw(cr_ui *u, const cr_painter *p, const char *title, bool back,
                       int count, cr_menu_row_reader reader) {
    if (!p || !reader) return;
    const cr_theme *theme = cr_theme_get(u->theme);
    if (u->quick) { panel(u, p, theme, title, count, reader); return; }
    if (u->page == CR_STATE_ACTIONS) {
        cr_ui background = *u;
        background.page = CR_STATE_LIST;
        background.scroll = u->depth ? u->history[u->depth - 1].scroll : 0;
        background.focus = -1;
        cr_menu_row meta = reader(u, -1);
        cr_interface_draw(&background, p, "Saves", true, meta.value, reader);
        panel(u, p, theme, title, count, reader);
        return;
    }
    cr_widget_gradient(p, (cr_menu_rect){0, 0, u->width, u->height}, 0,
                       cr_widget_mix(theme->background, theme->accent, u->theme == CR_THEME_ASUS ? 0 : .025f), theme->background);
    float top = u->page == CR_LIBRARY ? cr_interface_content_top(u) : 56;
    clip(p, 0, top, u->width, cr_interface_bottom(u) - top);
    if (u->page == CR_MAIN) hero(u, p, theme);
    if (u->page == CR_STATE_LIST || u->page == CR_CORE_DETAILS) metadata(u, p, theme, reader(u, -1));
    int first = u->page == CR_LIBRARY ? 1 : 0;
    for (int i = first; i < count; ++i) {
        cr_menu_rect r = cr_interface_row_bounds(u, i);
        if (r.y + r.h <= top) continue;
        if (r.y >= cr_interface_bottom(u)) break;
        cr_menu_row row = reader(u, i);
        if (u->page == CR_MAIN) home_row(u, p, theme, row, i);
        else if (u->page == CR_LIBRARY) game_card(u, p, theme, row, i);
        else regular_row(u, p, theme, row, i);
    }
    empty(u, p, theme, count);
    clip(p, 0, 0, u->width, u->height);
    header(u, p, theme, title, back);
    if (u->page == CR_LIBRARY) library_controls(u, p, theme);
    if (u->page != CR_CORE_DETAILS) navigation(u, p, theme);
}
