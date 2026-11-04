// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
#include "interface_layout.h"
#include "core_registry.h"
#include <math.h>

typedef struct {
    float hero, resume, shortcut, core, hero_gap, shortcut_gap, core_gap;
} home_metrics;
static home_metrics home(const cr_ui *u) {
    if (u->height < 500 || u->width > u->height)
        return (home_metrics){112, 82, 78, 78, 10, 12, 12};
    /* Fit the five-card composition between the header and bottom navigation. */
    float scale = fminf(1.25f, fmaxf(.8f, (u->height - 144) / 552));
    return (home_metrics){218 * scale, 106 * scale, 92 * scale, 92 * scale,
                          16 * scale, 14 * scale, 14 * scale};
}
cr_menu_rect cr_interface_home_hero(const cr_ui *u) {
    return (cr_menu_rect){16, 64 - u->scroll, fmaxf(1, u->width - 32), home(u).hero};
}
static bool library_list(const cr_ui *u) { return u->library_list || u->library_query[0]; }
static unsigned columns(const cr_ui *u) { return u->width >= 700 ? 4 : 2; }
static float grid_width(const cr_ui *u) {
    return fmaxf(1, (u->width - 32 - (columns(u) - 1) * 12) / columns(u));
}
static bool contains(cr_menu_rect a, float x, float y) {
    return a.w > 0 && a.h > 0 && x >= a.x && y >= a.y && x < a.x + a.w && y < a.y + a.h;
}
cr_menu_rect cr_interface_panel(const cr_ui *u) {
    float width = fmaxf(1, fminf(u->width - 24, u->quick ? 460 : 440));
    float height = fmaxf(1, fminf(u->height - 32, u->quick ? 428 : 510));
    return (cr_menu_rect){(u->width - width) / 2, (u->height - height) / 2, width, height};
}
cr_menu_rect cr_interface_search(const cr_ui *u) {
    return (cr_menu_rect){16, 104, fmaxf(1, u->width - 32), 44};
}
cr_menu_rect cr_interface_chip(const cr_ui *u, int chip) {
    float width = fmaxf(1, (u->width - 32 - 24) / 4);
    return (cr_menu_rect){16 + chip * (width + 8), 64, width, 30};
}
cr_menu_rect cr_interface_navigation(const cr_ui *u, int tab) {
    return (cr_menu_rect){u->width * tab / 5, u->height - 64, u->width / 5, 64};
}
float cr_interface_bottom(const cr_ui *u) {
    if (u->quick || u->page == CR_STATE_ACTIONS) {
        cr_menu_rect panel = cr_interface_panel(u);
        return panel.y + panel.h - 14;
    }
    return fmaxf(57, u->height - (u->page == CR_CORE_DETAILS ? 20 : 76));
}
float cr_interface_content_top(const cr_ui *u) {
    if (u->quick) return cr_interface_panel(u).y + 62;
    if (u->page == CR_STATE_ACTIONS) return cr_interface_panel(u).y + (u->height < 500 ? 145 : 214);
    if (u->page == CR_CORE_DETAILS) return u->height < 500 ? 186 : 300;
    if (u->page == CR_STATE_LIST) return 154;
    if (u->page == CR_LIBRARY) return u->library_search || u->library_query[0] ? 162 : 108;
    return 64;
}
cr_menu_rect cr_interface_row_bounds(const cr_ui *u, int row) {
    if (row < 0) return (cr_menu_rect){0};
    float scroll = u->quick ? u->quick_scroll : u->scroll;
    if (u->quick || u->page == CR_STATE_ACTIONS) {
        cr_menu_rect panel = cr_interface_panel(u);
        float step = u->quick ? 49 : 46;
        return (cr_menu_rect){panel.x + 12, cr_interface_content_top(u) + row * step - scroll,
                              fmaxf(1, panel.w - 24), step - 3};
    }
    if (u->page == CR_MAIN) {
        home_metrics m = home(u);
        float top = 64 + m.hero + m.hero_gap - scroll;
        if (row == 0) return (cr_menu_rect){16, top, fmaxf(1, u->width - 32), m.resume};
        if (row >= 1 && row <= 3) {
            float width = fmaxf(1, (u->width - 52) / 3);
            return (cr_menu_rect){16 + (row - 1) * (width + 10),
                top + m.resume + m.shortcut_gap, width, m.shortcut};
        }
        float core_top = top + m.resume + m.shortcut_gap + m.shortcut + m.core_gap;
        return (cr_menu_rect){16, row == 4 ? core_top : core_top + m.core + 10 + (row - 5) * 78,
                              fmaxf(1, u->width - 32), row == 4 ? m.core : 68};
    }
    if (u->page == CR_LIBRARY) {
        if (row == 0) return (cr_menu_rect){u->width - 48, 6, 44, 44};
        if (!library_list(u)) {
            float width = grid_width(u), height = width * 1.26f;
            return (cr_menu_rect){16 + ((row - 1) % columns(u)) * (width + 12),
                cr_interface_content_top(u) + ((row - 1) / columns(u)) * (height + 12) - scroll,
                width, height};
        }
        return (cr_menu_rect){16, cr_interface_content_top(u) + (row - 1) * 82 - scroll,
                              fmaxf(1, u->width - 32), 72};
    }
    if (u->page == CR_STATE_LIST) {
        if (row == 0) return (cr_menu_rect){16, 154 - scroll, fmaxf(1, u->width - 32), 48};
        return (cr_menu_rect){16, 214 + (row - 1) * 78 - scroll, fmaxf(1, u->width - 32), 68};
    }
    return (cr_menu_rect){16, cr_interface_content_top(u) + row * 78 - scroll,
                          fmaxf(1, u->width - 32), 68};
}
float cr_interface_content_height(const cr_ui *u, int count) {
    if (count <= 0) return 0;
    if (u->quick) return count * 49.0f;
    if (u->page == CR_STATE_ACTIONS) return count * 46.0f;
    if (u->page == CR_MAIN) {
        cr_menu_rect last = cr_interface_row_bounds(u, count > 5 ? count - 1 : 4);
        return last.y + u->scroll + last.h - 64;
    }
    if (u->page == CR_LIBRARY && !library_list(u)) {
        unsigned games = count > 0 ? (unsigned)count - 1 : 0;
        return ((games + columns(u) - 1) / columns(u)) * (grid_width(u) * 1.26f + 12);
    }
    if (u->page == CR_LIBRARY) return fmaxf(0, count - 1) * 82.0f;
    if (u->page == CR_STATE_LIST) return 60 + fmaxf(0, count - 1) * 78.0f;
    return count * 78.0f;
}
int cr_interface_target(const cr_ui *u, float x, float y, int count, bool back) {
    if (x < 0 || y < 0 || x >= u->width || y >= u->height) return -1;
    if (u->quick || u->page == CR_STATE_ACTIONS) {
        cr_menu_rect panel = cr_interface_panel(u);
        if (!contains(panel, x, y)) return -2;
        if (x >= panel.x + panel.w - 48 && y < panel.y + 52) return -2;
        if (y < cr_interface_content_top(u) || y >= cr_interface_bottom(u)) return -1;
        float step = u->quick ? 49 : 46;
        int row = (int)((y - cr_interface_content_top(u) +
                         (u->quick ? u->quick_scroll : u->scroll)) / step);
        if (row >= 0 && row < count && contains(cr_interface_row_bounds(u, row), x, y))
            return u->quick ? 10000 + row : row;
        return -1;
    }
    if (y < 56) {
        if (back && x < 52) return -2;
        if (u->page == CR_CHEAT_TEXT && x >= u->width - 52) return -302;
        if (u->page == CR_LIBRARY) {
            if (x >= u->width - 48) return -201;
            if (x >= u->width - 96) return -200;
        }
        return -1;
    }
    if (u->page != CR_CORE_DETAILS && y >= u->height - 64)
        return -100 - (int)fminf(4, floorf(x / u->width * 5));
    if (u->page == CR_LIBRARY) {
        for (int i = 0; i < 4; ++i) if (contains(cr_interface_chip(u, i), x, y)) return -210 - i;
        if (u->library_search || u->library_query[0]) {
            cr_menu_rect search = cr_interface_search(u);
            if (contains(search, x, y)) return x >= search.x + search.w - 44 ? -202 : -200;
        }
        if (y < cr_interface_content_top(u)) return -1;
    }
    if (y < 56 || y >= cr_interface_bottom(u)) return -1;
    int first = u->page == CR_LIBRARY ? 1 : 0;
    for (int row = first; row < count; ++row) {
        cr_menu_rect r = cr_interface_row_bounds(u, row);
        if (r.y > cr_interface_bottom(u)) break;
        if (!contains(r, x, y)) continue;
        bool overflow = (u->page == CR_CORES && row < (int)cr_registry_count()) ||
                        u->page == CR_LIBRARY || (u->page == CR_STATE_LIST && row > 0 &&
                        (size_t)row <= u->saved_state_count);
        if (overflow && x >= r.x + r.w - 44 &&
            (u->page != CR_LIBRARY || library_list(u) || y < r.y + 44)) return -5000 - row;
        return row;
    }
    return -1;
}
