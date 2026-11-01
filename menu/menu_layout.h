// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
#ifndef CORVUS_MENU_LAYOUT_H
#define CORVUS_MENU_LAYOUT_H
#include <stdbool.h>
#include <stdint.h>
#define CR_MENU_MAX_HINT_LINES 32
#define CR_MENU_HINT_BYTES 256
typedef float (*cr_menu_glyph_advance)(void *context, uint32_t codepoint, float font_size);
typedef struct {
    float width, height, content_width, body_top, body_bottom;
    float header, nav_size, margin, icon;
    float label_font, hint_font, title_font, label_line, hint_line;
    float ascent_ratio, glyph_ratio, padding, gap, hint_padding;
    cr_menu_glyph_advance glyph_advance;
    void *glyph_context;
    bool side_nav;
} cr_menu_layout;
cr_menu_layout cr_menu_layout_make(float width, float height, float density,
    float screen_pixel_width, float screen_pixel_height, float dpi,
    float line_ratio, float ascent_ratio, float glyph_ratio);
/* NULL lines counts only. Both paths use the same bounded UTF-8 word wrapping. */
unsigned cr_menu_wrap_hint(const char *text, float width, float font_size, float glyph_ratio,
                           cr_menu_glyph_advance advance, void *context,
                           char lines[CR_MENU_MAX_HINT_LINES][CR_MENU_HINT_BYTES]);
#endif
