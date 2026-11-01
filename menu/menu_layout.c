// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
#include "menu_layout.h"
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

static float positive(float value, float fallback) {
    return isfinite(value) && value > 0 ? value : fallback;
}
static float physical_size(float pixels, float density) {
    return fmaxf(1, floorf(pixels + .5f)) / density;
}
cr_menu_layout cr_menu_layout_make(float width, float height, float density,
    float screen_width, float screen_height, float dpi,
    float line_ratio, float ascent_ratio, float glyph_ratio) {
    cr_menu_layout layout = {0};
    density = fminf(8, fmaxf(.25f, positive(density, 1)));
    width = fminf(32768, positive(width, 1));
    height = fminf(32768, positive(height, 1));
    screen_width = fminf(65536, positive(screen_width, width * density));
    screen_height = fminf(65536, positive(screen_height, height * density));
    float diagonal = hypotf(screen_width, screen_height);
    float pixel_scale = diagonal / hypotf(1920, 1080);
    float scale = pixel_scale;
    if (isfinite(dpi) && dpi > 0) {
        float corrected_dpi = dpi * .83f;
        float inches = diagonal / corrected_dpi;
        float density_scale = corrected_dpi / 96;
        if (inches < 12) {
            float fraction = inches / 12;
            scale = pixel_scale * (1 - fraction) + density_scale * fraction;
        } else if (inches <= 24) scale = density_scale;
        else {
            float fraction = (fminf(inches, 32) - 24) / 8;
            scale = density_scale * (1 - fraction) + pixel_scale * fraction;
        }
    }
    float unit = 212 * .75f * scale;
    line_ratio = fminf(2, fmaxf(1, positive(line_ratio, 1.2f)));
    ascent_ratio = fminf(1.5f, fmaxf(.5f, positive(ascent_ratio, .93f)));
    glyph_ratio = fminf(1.5f, fmaxf(.25f, positive(glyph_ratio, .556f)));
    layout.width = width; layout.height = height;
    layout.side_nav = width > height;
    layout.header = layout.nav_size = layout.icon = physical_size(unit / 3, density);
    float maximum_bar = fminf(width, height) * .45f;
    layout.header = fminf(layout.header, maximum_bar);
    layout.nav_size = fminf(layout.nav_size, maximum_bar);
    layout.icon = fminf(layout.icon, maximum_bar);
    layout.margin = physical_size(unit / 9, density);
    layout.title_font = physical_size(unit / 7, density);
    layout.label_font = physical_size(unit / 9, density);
    layout.hint_font = physical_size(unit / 11, density);
    layout.label_line = layout.label_font * line_ratio;
    layout.hint_line = layout.hint_font * line_ratio;
    layout.padding = unit * (1.f / 10 + 1.f / 13) / density;
    layout.gap = physical_size(unit / 42, density);
    layout.hint_padding = physical_size(unit / 20, density);
    layout.ascent_ratio = ascent_ratio; layout.glyph_ratio = glyph_ratio;
    layout.content_width = width - (layout.side_nav ? layout.nav_size : 0);
    layout.body_top = layout.header;
    layout.body_bottom = height - (layout.side_nav ? 0 : layout.nav_size);
    return layout;
}

typedef struct { unsigned length, columns; uint32_t codepoint; bool valid; } rune;
static rune decode(const char *text, size_t available) {
    unsigned char first = (unsigned char)text[0];
    rune result = {.length = 1, .columns = 1, .codepoint = first, .valid = true};
    if (first < 128) return result;
    unsigned length = first >= 0xc2 && first <= 0xdf ? 2 :
                      first >= 0xe0 && first <= 0xef ? 3 :
                      first >= 0xf0 && first <= 0xf4 ? 4 : 0;
    uint32_t cp = length ? first & (0x7f >> length) : 0;
    if (!length || length > available) { result.valid = false; return result; }
    for (unsigned i = 1; i < length; ++i) {
        unsigned char next = (unsigned char)text[i];
        if ((next & 0xc0) != 0x80) { result.valid = false; return result; }
        cp = (cp << 6) | (next & 63);
    }
    if ((length == 3 && cp < 0x800) || (length == 4 && cp < 0x10000) ||
        (cp >= 0xd800 && cp <= 0xdfff) || cp > 0x10ffff) {
        result.valid = false; return result;
    }
    result.length = length; result.codepoint = cp;
    result.columns = cp >= 0x1100 && (cp <= 0x115f || cp == 0x2329 || cp == 0x232a ||
        (cp >= 0x2e80 && cp <= 0xa4cf) || (cp >= 0xac00 && cp <= 0xd7a3) ||
        (cp >= 0xf900 && cp <= 0xfaff) || (cp >= 0xfe10 && cp <= 0xfe6f) ||
        (cp >= 0xff01 && cp <= 0xff60) || (cp >= 0x1f000)) ? 2 : 1;
    return result;
}
typedef struct {
    char current[CR_MENU_HINT_BYTES];
    unsigned used, count;
    float width, maximum, font_size, glyph_ratio;
    cr_menu_glyph_advance advance;
    void *context;
    char (*lines)[CR_MENU_HINT_BYTES];
} wrapper;
static float glyph_width(const wrapper *wrap, rune cp) {
    float width = wrap->advance ? wrap->advance(wrap->context,
        cp.valid ? cp.codepoint : '?', wrap->font_size) : -1;
    return isfinite(width) && width >= 0 ? width : cp.columns * wrap->font_size * wrap->glyph_ratio;
}
static bool flush_line(wrapper *wrap) {
    if (wrap->count >= CR_MENU_MAX_HINT_LINES) return false;
    if (wrap->lines) {
        memcpy(wrap->lines[wrap->count], wrap->current, wrap->used);
        wrap->lines[wrap->count][wrap->used] = 0;
    }
    ++wrap->count; wrap->used = 0; wrap->width = 0;
    return wrap->count < CR_MENU_MAX_HINT_LINES;
}
static bool append_word(wrapper *wrap, const char *word, size_t bytes, float width) {
    float space = glyph_width(wrap, decode(" ", 1));
    if (wrap->used && (width > wrap->maximum || wrap->width + space + width > wrap->maximum ||
                          wrap->used + 1 + bytes >= CR_MENU_HINT_BYTES)) {
        if (!flush_line(wrap)) return false;
    } else if (wrap->used) {
        wrap->current[wrap->used++] = ' '; wrap->width += space;
    }
    for (size_t at = 0; at < bytes;) {
        rune cp = decode(word + at, bytes - at);
        float advance = glyph_width(wrap, cp);
        if (wrap->used && (wrap->width + advance > wrap->maximum ||
                          wrap->used + cp.length >= CR_MENU_HINT_BYTES))
            if (!flush_line(wrap)) return false;
        memcpy(wrap->current + wrap->used, word + at, cp.length);
        wrap->used += cp.length; wrap->width += advance; at += cp.length;
    }
    return true;
}
unsigned cr_menu_wrap_hint(const char *text, float width, float font_size, float glyph_ratio,
                           cr_menu_glyph_advance advance, void *context,
                           char lines[CR_MENU_MAX_HINT_LINES][CR_MENU_HINT_BYTES]) {
    if (!text || !*text) return 0;
    width = positive(width, 1);
    font_size = positive(font_size, 13);
    glyph_ratio = positive(glyph_ratio, .556f);
    wrapper wrap = {.maximum = width, .font_size = font_size, .glyph_ratio = glyph_ratio,
                    .advance = advance, .context = context, .lines = lines};
    size_t length = 0;
    while (length < CR_MENU_HINT_BYTES - 1 && text[length]) ++length;
    bool trailing_newline = false;
    for (size_t at = 0; at < length;) {
        if (text[at] == '\n') {
            if (!flush_line(&wrap)) return wrap.count;
            trailing_newline = true; ++at; continue;
        }
        if (text[at] == ' ' || text[at] == '\t' || text[at] == '\r') { ++at; continue; }
        char word[CR_MENU_HINT_BYTES]; size_t used = 0; float word_width = 0;
        while (at < length && text[at] != ' ' && text[at] != '\t' && text[at] != '\n' && text[at] != '\r') {
            rune cp = decode(text + at, length - at);
            if (cp.valid) { memcpy(word + used, text + at, cp.length); used += cp.length; }
            else word[used++] = '?';
            word_width += glyph_width(&wrap, cp); at += cp.length;
        }
        if (!append_word(&wrap, word, used, word_width)) return wrap.count;
        trailing_newline = false;
    }
    if (wrap.used || trailing_newline) flush_line(&wrap);
    return wrap.count;
}
