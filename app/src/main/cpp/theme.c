// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
#include "theme.h"
static const cr_theme themes[CR_THEME_ASUS + 1] = {
    [CR_THEME_SHIELD] = {.name = "NVIDIA Shield",
                         .description = "Noir et vert",
                         .background = 0xff121212,
                         .header = 0xff1f1f1f,
                         .surface = 0xff1f1f1f,
                         .navigation = 0xff242424,
                         .text = 0xffdedede,
                         .muted = 0xff999999,
                         .accent = 0xff7ab547,
                         .icon = 0xff7ab547,
                         .divider = 0xff498515,
                         .focus = 0xff255d00,
                         .header_text = 0xffdedede,
                         .header_hint = 0xff999999,
                         .header_icon = 0xff7ab547,
                         .navigation_active = 0xff7ab547,
                         .navigation_passive = 0xff558b2f,
                         .control = 0xcc263320,
                         .control_text = 0xffdedede,
                         .control_pressed_text = 0xff101a08,
                         .system_bar = 0xff000000},
    [CR_THEME_CORVUS] = {.name = "CorvusRetro",
                         .description = "Bleu nuit et violet du corbeau",
                         .background = 0xff090d14,
                         .header = 0xff111827,
                         .surface = 0xff141b2c,
                         .navigation = 0xff141e30,
                         .text = 0xffeff1fa,
                         .muted = 0xffa8b2cd,
                         .accent = 0xffb69aff,
                         .icon = 0xff9a9dff,
                         .divider = 0xff303b54,
                         .focus = 0xff292343,
                         .header_text = 0xffeff1fa,
                         .header_hint = 0xffa8b2cd,
                         .header_icon = 0xffb69aff,
                         .navigation_active = 0xffb69aff,
                         .navigation_passive = 0xff8d9cbd,
                         .control = 0xdd1d273b,
                         .control_text = 0xffeff1fa,
                         .control_pressed_text = 0xff111322,
                         .system_bar = 0xff090d14},
    [CR_THEME_ASUS] = {.name = "ASUS",
                       .description = "Noir et blanc du logotype",
                       .background = 0xffffffff,
                       .header = 0xff000000,
                       .surface = 0xffffffff,
                       .navigation = 0xff000000,
                       .text = 0xff111111,
                       .muted = 0xff515151,
                       .accent = 0xff000000,
                       .icon = 0xff000000,
                       .divider = 0xffd0d0d0,
                       .focus = 0xffededed,
                       .header_text = 0xffffffff,
                       .header_hint = 0xffdddddd,
                       .header_icon = 0xffffffff,
                       .navigation_active = 0xffffffff,
                       .navigation_passive = 0xffc8c8c8,
                       .control = 0xdd161616,
                       .control_text = 0xffffffff,
                       .control_pressed_text = 0xffffffff,
                       .system_bar = 0xff000000}};
int cr_theme_valid(int id) {
    return id == CR_THEME_SHIELD || id == CR_THEME_CORVUS || id == CR_THEME_ASUS ? id
                                                                                 : CR_THEME_CORVUS;
}
int cr_theme_at(int index) {
    static const int order[CR_THEME_COUNT] = {CR_THEME_CORVUS, CR_THEME_SHIELD, CR_THEME_ASUS};
    return index >= 0 && index < CR_THEME_COUNT ? order[index] : CR_THEME_CORVUS;
}
const cr_theme *cr_theme_get(int id) {
    return &themes[cr_theme_valid(id)];
}
