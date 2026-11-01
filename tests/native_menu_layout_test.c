// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
#include "menu_layout.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void check(bool ok, const char *message) {
    if (!ok) { fprintf(stderr, "FAIL menu layout: %s\n", message); exit(1); }
}
static bool near(float first, float second) { return fabsf(first - second) < .002f; }
static unsigned wrap_fallback(const char *text, float width, float font_size, float glyph_ratio,
                              char lines[CR_MENU_MAX_HINT_LINES][CR_MENU_HINT_BYTES]) {
    return cr_menu_wrap_hint(text, width, font_size, glyph_ratio, NULL, NULL, lines);
}
static float proportional_advance(void *context, uint32_t cp, float size) {
    unsigned *calls = context;
    ++*calls;
    float ratio = cp == 'W' || cp == 'M' ? .95f : cp == 'i' ? .25f :
                  cp == ' ' ? .3f : cp == 0x153 ? .85f : .55f;
    return size * ratio;
}
int main(void) {
    cr_menu_layout portrait = cr_menu_layout_make(384, 696, 1.25f, 480, 960, 200, 1.2f, .93f, .556f);
    check(near(portrait.header, 48.8f) && near(portrait.nav_size, 48.8f) && near(portrait.icon, 48.8f),
          "Android default reference icon and bars are physical 61px at 200dpi");
    check(near(portrait.title_font, 20.8f) && near(portrait.label_font, 16) && near(portrait.hint_font, 13.6f),
          "font sizes round reference fractions to physical pixels before converting to dp");
    check(near(portrait.margin, 16) && near(portrait.gap, 3.2f) && near(portrait.hint_padding, 7.2f),
          "margins and text gap follow the same reference unit");
    check(fabsf(portrait.padding - 26.0243f) < .01f && near(portrait.label_line, 19.2f),
          "row padding and actual font line ratio are separate quantities");
    check(!portrait.side_nav && near(portrait.body_top, portrait.header) &&
          near(portrait.body_bottom, 696 - portrait.nav_size) && near(portrait.content_width, 384),
          "portrait reserves bottom navigation without a duplicate system or footer bar");
    cr_menu_layout landscape = cr_menu_layout_make(696, 336, 1.25f, 960, 480, 200, 1.2f, .93f, .556f);
    check(landscape.side_nav && near(landscape.content_width, 696 - landscape.nav_size) &&
          near(landscape.body_bottom, 336) && near(landscape.header, portrait.header),
          "rotation moves navigation right while retaining physical size");
    cr_menu_layout doubled = cr_menu_layout_make(384, 696, 2.5f, 960, 1920, 400, 1.2f, .93f, .556f);
    check(fabsf(doubled.label_font - portrait.label_font) <= .5f &&
          fabsf(doubled.header - portrait.header) <= .5f,
          "equivalent display densities preserve interface scale in dp");
    float diagonal = hypotf(1920, 1080);
    const float inches[] = {20, 28, 40};
    for (unsigned i = 0; i < sizeof(inches) / sizeof(*inches); ++i) {
        float density_scale = diagonal / inches[i] / 96;
        float scale = i == 0 ? density_scale : i == 1 ? (density_scale + 1) / 2 : 1;
        float expected = floorf(212 * .75f * scale / 3 + .5f) / 2;
        cr_menu_layout large = cr_menu_layout_make(960, 540, 2, 1920, 1080,
            diagonal / inches[i] / .83f, 1.2f, .93f, .556f);
        check(near(large.header, expected), "large-screen density and pixel-scale interpolation");
    }
    cr_menu_layout fallback = cr_menu_layout_make(960, 540, 2, 1920, 1080, 0, 1.2f, .93f, .556f);
    check(near(fallback.header, 26.5f), "absent DPI uses the physical-resolution reference scale");
    for (unsigned i = 0; i < 3; ++i) {
        float invalid = i == 0 ? NAN : i == 1 ? INFINITY : -1;
        cr_menu_layout tiny = cr_menu_layout_make(invalid, invalid, invalid, invalid, invalid,
                                                   invalid, invalid, invalid, invalid);
        check(isfinite(tiny.header) && isfinite(tiny.padding) && tiny.body_bottom >= tiny.body_top &&
              tiny.content_width > 0 && tiny.label_font > 0, "invalid metrics stay finite and bounded");
    }
    char lines[CR_MENU_MAX_HINT_LINES][CR_MENU_HINT_BYTES];
    unsigned count = wrap_fallback("un cœur chargé", 8 * 10.f, 10, 1, lines);
    check(count == 2 && !strcmp(lines[0], "un cœur") && !strcmp(lines[1], "chargé"),
          "word wrapping counts UTF-8 characters rather than bytes");
    check(wrap_fallback("un cœur chargé", 80, 10, 1, NULL) == count,
          "count-only path and rendered lines share exact geometry");
    count = wrap_fallback("abcdefghijk", 40, 10, 1, lines);
    check(count == 3 && !strcmp(lines[0], "abcd") && !strcmp(lines[1], "efgh") && !strcmp(lines[2], "ijk"),
          "long tokens split within the available width");
    count = wrap_fallback("Jeu\n\nPrêt", 200, 10, 1, lines);
    check(count == 3 && !strcmp(lines[0], "Jeu") && !*lines[1] && !strcmp(lines[2], "Prêt"),
          "explicit blank lines are retained");
    count = wrap_fallback("漢字漢字", 30, 10, 1, lines);
    check(count == 4 && !strcmp(lines[0], "漢") && !strcmp(lines[1], "字"),
          "wide characters retain complete byte sequences and double-column widths");
    count = wrap_fallback("🪶🪶🪶", 40, 10, 1, lines);
    check(count == 2 && strlen(lines[0]) == 8 && strlen(lines[1]) == 4,
          "four-byte characters are never split");
    char long_hint[256]; memset(long_hint, 'a', 255); long_hint[255] = 0;
    check(wrap_fallback(long_hint, 1, 10, 1, lines) == CR_MENU_MAX_HINT_LINES,
          "extremely narrow windows are bounded independently of text length");
    for (unsigned i = 0; i < 127; ++i) { long_hint[i * 2] = (char)0xc3; long_hint[i * 2 + 1] = (char)0xa9; }
    long_hint[254] = 0;
    check(wrap_fallback(long_hint, 3000, 10, 1, lines) == 1 && strlen(lines[0]) == 254,
          "maximum multibyte hint remains bounded and complete");
    check(!wrap_fallback(NULL, 200, 10, 1, lines) && !wrap_fallback(" \t\r", 200, 10, 1, lines),
          "absent hints reserve no text lines");
    count = wrap_fallback("\300\257\200", 200, 10, 1, lines);
    check(count == 1 && !strcmp(lines[0], "???"), "invalid UTF-8 produces bounded replacement characters");
    unsigned calls = 0;
    count = cr_menu_wrap_hint("WWW MMM", 40, 10, .556f, proportional_advance, &calls, lines);
    check(count == 2 && !strcmp(lines[0], "WWW") && !strcmp(lines[1], "MMM"),
          "wide proportional capitals wrap before the renderer clips the end");
    check(cr_menu_wrap_hint("WWW MMM", 40, 10, .556f, proportional_advance, &calls, NULL) == count,
          "measured count-only layout and draw lines use identical glyph advances");
    count = cr_menu_wrap_hint("iiiiiiii", 20, 10, .556f, proportional_advance, &calls, lines);
    check(count == 1 && !strcmp(lines[0], "iiiiiiii"),
          "narrow proportional letters fit according to their measured advances");
    count = cr_menu_wrap_hint("cœur chargé", 32, 10, .556f, proportional_advance, &calls, lines);
    check(count == 3 && !strcmp(lines[0], "cœur") && !strcmp(lines[1], "charg") && !strcmp(lines[2], "é"),
          "French accents and the extended atlas ligature retain actual glyph widths");
    check(calls > 0, "cached glyph metrics are consulted for proportional layout");
    puts("PASS reference menu metrics, rotation, density, bounded UTF-8 wrapping and invalid inputs");
    return 0;
}
