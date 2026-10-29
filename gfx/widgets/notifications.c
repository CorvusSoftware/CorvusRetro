// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
#include "notifications.h"
#include <math.h>
#include <string.h>

#define CR_CARD_ANIMATION .33
#define CR_CARD_HOLD 4.0
#define CR_PROGRESS_HOLD 2.0
#define CR_CARD_DURATION (CR_CARD_HOLD + CR_CARD_ANIMATION * 4)
#define CR_IMAGE_WAIT 3.0

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
                if (!i && n->elapsed >= CR_CARD_ANIMATION * 2 &&
                    n->elapsed < CR_CARD_ANIMATION * 2 + CR_CARD_HOLD)
                    n->elapsed = CR_CARD_ANIMATION * 2;
                break;
            }
    if (!card) {
        if (!n->count) {
            n->card_started = false;
            n->image_wait_started = false;
            n->image_wait = 0;
        }
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
    /* Artwork stays fixed once the visible animation has started. */
    if (card != &n->cards[0] || !n->card_started) {
        copy_text(card->image_url, sizeof(card->image_url), image_url);
        card->image_pending = card->image_url[0] != 0;
    }
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
void cr_notifications_prepare(cr_notifications *n, const cr_painter *p) {
    if (!n || !n->count || !p) return;
    cr_notification_card *card = &n->cards[0];
    if (!card->image_pending) return;
    n->image_wait_started = true;
    if (p->image_ready && p->image_ready(p->context, card->image_url)) {
        card->image_pending = false;
        n->image_wait = 0;
    } else if (n->image_wait >= CR_IMAGE_WAIT) {
        /* Commit to one fallback if artwork fails; never swap it mid-animation. */
        card->image_pending = false;
        card->image_url[0] = 0;
    }
}
void cr_notifications_update(cr_notifications *n, double seconds) {
    if (!n || !isfinite(seconds) || seconds < 0) return;
    if (n->count && n->cards[0].image_pending) {
        if (n->image_wait_started) n->image_wait += seconds;
    } else if (n->count) {
        /* Loading time before the first presentation must not skip the descent. */
        if (!n->card_started) n->card_started = true;
        else n->elapsed += seconds;
        if (n->elapsed >= CR_CARD_DURATION) {
            --n->count;
            memmove(n->cards, &n->cards[1], n->count * sizeof(n->cards[0]));
            memset(&n->cards[n->count], 0, sizeof(n->cards[0]));
            n->elapsed = 0;
            n->image_wait = 0;
            n->card_started = false;
            n->image_wait_started = false;
        }
    }
    n->progress_time = fmax(0, n->progress_time - seconds);
    if (!n->progress_time) cr_notifications_hide_progress(n, 0);
}
static float ease_out(double phase) {
    float t = (float)fmax(0, fmin(1, phase));
    return 1 - (1 - t) * (1 - t);
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
                  float y, float size, float opacity, bool live_fallback) {
    p->rect(p->context, x, y, size, size, alpha(theme->background, opacity));
    if (url[0]) {
        if (p->image && p->image(p->context, url, x, y, size, size, opacity)) return;
        if (!live_fallback) return;
    }
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
    if (n->count && !n->cards[0].image_pending) {
        double time = n->elapsed;
        float slide = ease_out(time / CR_CARD_ANIMATION);
        float unfold = ease_out((time - CR_CARD_ANIMATION) / CR_CARD_ANIMATION);
        if (time >= CR_CARD_ANIMATION * 2 + CR_CARD_HOLD)
            unfold = 1 - ease_out((time - CR_CARD_ANIMATION * 2 - CR_CARD_HOLD) / CR_CARD_ANIMATION);
        if (time >= CR_CARD_ANIMATION * 3 + CR_CARD_HOLD)
            slide = 1 - ease_out((time - CR_CARD_ANIMATION * 3 - CR_CARD_HOLD) / CR_CARD_ANIMATION);
        if (slide > 0 && time < CR_CARD_DURATION) {
            const cr_notification_card *c = &n->cards[0];
            float w = card_width(c, p, scale, available);
            float panel = w - card_height, shown = card_height + panel * unfold;
            float x = left + (width - shown) / 2;
            float y = top - card_height + (margin + card_height) * slide;
            if (p->clip) p->clip(p->context, left, top, width, height);
            p->rect(p->context, x, y + 2 * scale, shown, card_height, 0x40000000);
            p->rect(p->context, x, y, shown, card_height, alpha(theme->surface, .97f));
            p->rect(p->context, x, y, 3 * scale, card_height, theme->accent);
            badge(p, theme, c->image_url, x + 8 * scale, y + 8 * scale, 64 * scale, 1, false);
            if (unfold > 0 && (p->clip || unfold == 1)) {
                if (p->clip)
                    p->clip(p->context, x + card_height, fmaxf(top, y), panel * unfold,
                            fminf(bottom, y + card_height) - fmaxf(top, y));
                float tx = x + 84 * scale - panel * (1 - unfold) / 2;
                float max = w - 92 * scale;
                label(p, c->title, tx, y + 30 * scale, 16 * scale, theme->text, max);
                label(p, c->detail, tx, y + 53 * scale, 13 * scale, theme->muted, max);
            }
            if (p->clip) p->clip(p->context, left, top, width, height);
            card_visible = true;
        }
    }
    if (n->progress.visible && n->progress_time > 0) {
        float h = 44 * scale;
        float y = bottom - margin - h;
        if (card_visible && y < top + margin + card_height + margin) return;
        /* A pending progress image must not produce a Corvus crow placeholder. */
        bool image_ready = n->progress.image_url[0] && p->image && p->image_ready &&
                           p->image_ready(p->context, n->progress.image_url);
        float inset = image_ready ? 58 * scale : 16 * scale;
        float w = fminf(available, fmaxf((image_ready ? 116 : 70) * scale,
                  inset + measure(p, n->progress.text, 14 * scale)));
        float x = left + width - margin - w;
        float opacity = (float)fmin(1, n->progress_time / .2);
        p->rect(p->context, x, y, w, h, alpha(theme->surface, opacity * .97f));
        if (image_ready)
            badge(p, theme, n->progress.image_url, x + 6 * scale, y + 6 * scale,
                  32 * scale, opacity, false);
        float tx = x + (image_ready ? 48 : 10) * scale;
        float max = w - (image_ready ? 54 : 16) * scale;
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
