// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
#include "menu_setting.h"
#include <stdio.h>

#define SETTING(key, label, field, min, max, step, fallback) \
    {key, label, min, max, step, fallback, offsetof(cr_ui, field)}
const cr_menu_setting_spec cr_menu_settings[SET_COUNT] = {
    SETTING("audio_volume", "Volume du jeu", volume, 0, 100, 10, 100),
    SETTING("video_integer", "Présentation de l'image", video_integer, 0, 1, 1, 0),
    SETTING("video_smooth", "Filtrage", video_smooth, 0, 1, 1, 0),
    SETTING("overlay_opacity", "Intensité au repos", overlay_opacity, 10, 100, 10, 100),
    SETTING("overlay_size", "Taille des commandes", overlay_size, 100, 130, 10, 100),
    SETTING("overlay_position", "Position verticale", overlay_position, -15, 15, 5, 0),
    SETTING("notify_cards", "Cartes de jeu et de succès", notify_cards, 0, 1, 1, 1),
    SETTING("notify_progress", "Progression des succès", notify_progress, 0, 1, 1, 1),
    SETTING("backup_count", "Copies de sécurité", backup_count, 1, 3, 1, 1),
    SETTING("auto_state", "État automatique", auto_state, 0, 1, 1, 0)
};
#undef SETTING
static int *setting_value(cr_ui *u, int id) {
    return (int *)((char *)u + cr_menu_settings[id].offset);
}
int cr_menu_setting_get(const cr_ui *u, int id) {
    return *(const int *)((const char *)u + cr_menu_settings[id].offset);
}
void cr_menu_setting_label(int id, int value, char *out, size_t size) {
    if (id == SET_INTEGER) snprintf(out, size, "%s", value ? "Échelle entière" : "Ratio natif adapté");
    else if (id == SET_FILTER) snprintf(out, size, "%s", value ? "Interpolation" : "Pixels nets");
    else if (id == SET_BACKUPS) snprintf(out, size, "%d copie%s par joueur", value, value > 1 ? "s" : "");
    else if (id == SET_POSITION && !value) snprintf(out, size, "%s", "Automatique");
    else if (id == SET_POSITION) snprintf(out, size, "%+d %%", value);
    else snprintf(out, size, "%d %%", value);
}
void cr_menu_setting_apply(cr_ui *u, int id, int value) {
    *setting_value(u, id) = value;
    if (u->platform.set_preference)
        u->platform.set_preference(u->platform.context, cr_menu_settings[id].key, value);
    if (id == SET_SIZE || id == SET_POSITION || id == SET_OPACITY) cr_ui_clear_input(u);
    if (id == SET_CARDS && !value) cr_notifications_hide_cards(&u->notifications);
    if (id == SET_PROGRESS && !value) cr_notifications_hide_progress(&u->notifications, 0);
    if (id == SET_BACKUPS || id == SET_AUTO_STATE)
        cr_engine_set_save_policy(&u->engine, (unsigned)u->backup_count, u->auto_state != 0);
}
int cr_menu_setting_icon(int id) {
    static const int icons[SET_COUNT] = {CR_ICON_VOLUME, CR_ICON_DISPLAY, CR_ICON_FILTER,
        CR_ICON_SLIDERS, CR_ICON_SLIDERS, CR_ICON_SLIDERS, CR_ICON_NOTIFICATIONS,
        CR_ICON_NOTIFICATIONS, CR_ICON_BACKUP, CR_ICON_SAVES};
    return icons[id];
}
void cr_menu_settings_load(cr_ui *u) {
    cr_platform p = u->platform;
    for (int i = 0; i < SET_COUNT; ++i) {
        const cr_menu_setting_spec *spec = &cr_menu_settings[i];
        int value = p.preference ? p.preference(p.context, spec->key, spec->fallback) : spec->fallback;
        *setting_value(u, i) = value < spec->minimum || value > spec->maximum ||
            (value - spec->minimum) % spec->step ? spec->fallback : value;
    }
    cr_engine_set_save_policy(&u->engine, (unsigned)u->backup_count, u->auto_state != 0);
}
