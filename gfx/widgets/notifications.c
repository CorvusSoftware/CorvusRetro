// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
#include "notifications.h"
#include <math.h>
#include <string.h>

#define CR_CARD_ANIMATION .33
#define CR_CARD_HOLD 4.0
#define CR_PROGRESS_HOLD 2.0

static void copy_text(char *destination, size_t capacity, const char *source) {
    size_t size = 0;
    if (!source) source = "";
    while (source[size] && size + 1 < capacity) ++size;
    if (source[size])
        while (size && ((unsigned char)source[size] & 0xc0) == 0x80) --size;
    memcpy(destination, source, size);
    destination[size] = 0;
}
void cr_notifications_push(cr_notifications *n, int kind, uint32_t id, const char *title,
                           const char *detail, const char *image_url) {
    if (!n || (kind != CR_NOTIFICATION_GAME && kind != CR_NOTIFICATION_UNLOCK)) return;
    cr_notification_card *card = NULL;
    if (kind == CR_NOTIFICATION_GAME)
        for (size_t i = 0; i < n->count; ++i)
            if (n->cards[i].kind == kind && n->cards[i].id == id) {
                card = &n->cards[i];
                if (!i && n->elapsed > CR_CARD_ANIMATION) n->elapsed = CR_CARD_ANIMATION;
                break;
            }
    if (!card) {
        if (n->count == CR_NOTIFICATION_CAPACITY) {
            /* Preserve the visible card while keeping the most recent waiting events. */
            memmove(&n->cards[1], &n->cards[2], (n->count - 2) * sizeof(n->cards[0]));
            --n->count;
        }
        card = &n->cards[n->count++];
    }
    card->kind = kind;
    card->id = id;
    copy_text(card->title, sizeof(card->title), title);
    copy_text(card->detail, sizeof(card->detail), detail);
    copy_text(card->image_url, sizeof(card->image_url), image_url);
}
void cr_notifications_progress(cr_notifications *n, uint32_t id, const char *text,
                               const char *image_url, float percent) {
    if (!n) return;
    n->progress.id = id;
    n->progress.visible = true;
    n->progress.percent = isfinite(percent) ? fmaxf(0, fminf(100, percent)) : -1;
    copy_text(n->progress.text, sizeof(n->progress.text), text);
    copy_text(n->progress.image_url, sizeof(n->progress.image_url), image_url);
    n->progress_time = CR_PROGRESS_HOLD;
}
void cr_notifications_hide_progress(cr_notifications *n, uint32_t id) {
    if (n && (!id || id == n->progress.id)) {
        memset(&n->progress, 0, sizeof(n->progress));
        n->progress_time = 0;
    }
}
void cr_notifications_clear(cr_notifications *n) {
    if (n) memset(n, 0, sizeof(*n));
}
void cr_notifications_update(cr_notifications *n, double seconds) {
    if (!n || !isfinite(seconds) || seconds <= 0) return;
    if (n->count) {
        n->elapsed += seconds;
        if (n->elapsed >= CR_CARD_HOLD + CR_CARD_ANIMATION * 2) {
            --n->count;
            memmove(n->cards, &n->cards[1], n->count * sizeof(n->cards[0]));
            memset(&n->cards[n->count], 0, sizeof(n->cards[0]));
            n->elapsed = 0;
        }
    }
    n->progress_time = fmax(0, n->progress_time - seconds);
    if (!n->progress_time) cr_notifications_hide_progress(n, 0);
}
static uint32_t alpha(uint32_t color, float opacity) {
    unsigned value = (unsigned)((color >> 24) * fmaxf(0, fminf(1, opacity)) + .5f);
    return (color & 0xffffffu) | (value << 24);
}
static float measure(const cr_painter *p, const char *text, float size) {
    float width = p->measure ? p->measure(p->context, text, size) : (float)strlen(text) * size * .5f;
    return isfinite(width) ? fmaxf(0, width) : 0;
}
static void label(const cr_painter *p, const char *text, float x, float y, float size,
                  uint32_t color, float width) {
    if (p->text && width > 0) p->text(p->context, text, x, y, size, color, width);
}
static void badge(const cr_painter *p, const cr_theme *theme, const char *url, float x,
                  float y, float size, float opacity) {
    p->rect(p->context, x, y, size, size, alpha(theme->background, opacity));
    if (url[0] && p->image && p->image(p->context, url, x, y, size, size, opacity)) return;
    if (p->icon)
        p->icon(p->context, CR_ICON_CROW, x + size * .15f, y + size * .15f,
                size * .7f, alpha(theme->accent, opacity));
}
static float card_width(const cr_notification_card *c, const cr_painter *p, float scale,
                         float available) {
    float text = fmaxf(measure(p, c->title, 16 * scale), measure(p, c->detail, 13 * scale));
    return fminf(available, fminf(420 * scale, fmaxf(236 * scale, 96 * scale + text)));
}
void cr_notifications_draw_region(const cr_notifications *n, const cr_painter *p, float left,
                                  float top, float right, float bottom, const cr_theme *theme) {
    if (!n || !p || !p->rect || !theme || !isfinite(left) || !isfinite(right) || !isfinite(top) ||
        !isfinite(bottom) || left < 0 || right <= left || top < 0 || bottom <= top) return;
    float width = right - left, height = bottom - top;
    float scale = fminf(1, fminf(width / 320, height / 104));
    float margin = 12 * scale, card_height = 80 * scale;
    float available = width - margin * 2;
    bool card_visible = false;
    if (n->count) {
        float fade = (float)fmin(1, fmin(n->elapsed / CR_CARD_ANIMATION,
                    (CR_CARD_HOLD + CR_CARD_ANIMATION * 2 - n->elapsed) / CR_CARD_ANIMATION));
        float opacity = fmaxf(0, fade);
        if (opacity > 0) {
            float eased = 1 - (1 - opacity) * (1 - opacity);
            const cr_notification_card *c = &n->cards[0];
            float w = card_width(c, p, scale, available);
            float x = left + width - margin - w + margin * (1 - eased);
            float y = top + margin;
            p->rect(p->context, x, y + 2 * scale, w, card_height,
                    alpha(0x40000000, opacity));
            p->rect(p->context, x, y, w, card_height, alpha(theme->surface, opacity * .97f));
            p->rect(p->context, x, y, 3 * scale, card_height, alpha(theme->accent, opacity));
            badge(p, theme, c->image_url, x + 8 * scale, y + 8 * scale, 64 * scale, opacity);
            float tx = x + 84 * scale, max = w - 92 * scale;
            label(p, c->title, tx, y + 30 * scale, 16 * scale, alpha(theme->text, opacity), max);
            label(p, c->detail, tx, y + 53 * scale, 13 * scale, alpha(theme->muted, opacity), max);
            card_visible = true;
        }
    }
    if (n->progress.visible && n->progress_time > 0) {
        float h = 44 * scale;
        float y = bottom - margin - h;
        if (card_visible && y < top + margin + card_height + margin) return;
        float w = fminf(available, fmaxf(116 * scale,
                  58 * scale + measure(p, n->progress.text, 14 * scale)));
        float x = left + width - margin - w;
        float opacity = (float)fmin(1, n->progress_time / .2);
        p->rect(p->context, x, y, w, h, alpha(theme->surface, opacity * .97f));
        badge(p, theme, n->progress.image_url, x + 6 * scale, y + 6 * scale, 32 * scale, opacity);
        float tx = x + 48 * scale, max = w - 54 * scale;
        label(p, n->progress.text, tx, y + 26 * scale, 14 * scale,
              alpha(theme->text, opacity), max);
        if (n->progress.percent >= 0 && max > 0) {
            p->rect(p->context, tx, y + h - 5 * scale, max, 2 * scale,
                    alpha(theme->divider, opacity));
            if (n->progress.percent > 0)
                p->rect(p->context, tx, y + h - 5 * scale, max * n->progress.percent / 100,
                        2 * scale, alpha(theme->accent, opacity));
        }
    }
}
void cr_notifications_draw_area(const cr_notifications *n, const cr_painter *p, float width,
                                float top, float bottom, const cr_theme *theme) {
    cr_notifications_draw_region(n, p, 0, top, width, bottom, theme);
}
void cr_notifications_draw(const cr_notifications *n, const cr_painter *p, float width,
                           float height, const cr_theme *theme) {
    cr_notifications_draw_area(n, p, width, 0, height, theme);
}
