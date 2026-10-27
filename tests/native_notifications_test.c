// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
#include "notifications.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks;
static void check(bool condition, const char *message) {
    ++checks;
    if (!condition) {
        fprintf(stderr, "FAIL %s\n", message);
        exit(1);
    }
}
static void queue_tests(void) {
    cr_notifications n = {0};
    char title[] = "Jeu original", url[] = "https://media.retroachievements.org/Images/1.png";
    cr_notifications_push(&n, CR_NOTIFICATION_GAME, 10, title, "2/10 succès", url);
    title[0] = 'X'; url[0] = 'X';
    check(strcmp(n.cards[0].title, "Jeu original") == 0 && n.cards[0].image_url[0] == 'h',
          "cards own their strings beyond the callback");
    cr_notifications_update(&n, 0);
    cr_notifications_update(&n, .8);
    cr_notifications_push(&n, CR_NOTIFICATION_GAME, 10, "Jeu identifié", "3/10 succès", "icon");
    check(n.count == 1 && strcmp(n.cards[0].title, "Jeu identifié") == 0,
          "late game information updates one launch card");
    cr_notifications_update(&n, 3.9);
    check(n.count == 1, "updated game card has a complete readable hold");
    cr_notifications_update(&n, .8);
    check(n.count == 0, "launch card expires after its hold and animations");
    cr_notifications_push(&n, CR_NOTIFICATION_UNLOCK, 1, "Premier succès", "10 points", "badge");
    cr_notifications_push(&n, CR_NOTIFICATION_GAME, 10, "Jeu", "", "");
    cr_notifications_push(&n, CR_NOTIFICATION_GAME, 10, "Jeu et icône", "", "icon");
    check(n.count == 2 && strcmp(n.cards[1].title, "Jeu et icône") == 0,
          "waiting game cards are coalesced without disturbing current unlock");
    cr_notifications_update(&n, 0);
    cr_notifications_update(&n, 20);
    check(n.count == 1 && n.cards[0].id == 10 && n.elapsed == 0,
          "long pauses do not consume unseen waiting notifications");
    cr_notifications_clear(&n);
    for (uint32_t id = 0; id < 20; ++id)
        cr_notifications_push(&n, CR_NOTIFICATION_UNLOCK, id, "Succès", "", "badge");
    check(n.count == CR_NOTIFICATION_CAPACITY && n.cards[0].id == 0 &&
          n.cards[n.count - 1].id == 19, "flood is bounded while preserving visible and newest cards");
    cr_notifications_update(&n, 0);
    uint32_t previous = n.cards[0].id;
    while (n.count) {
        cr_notifications_update(&n, 6);
        check(!n.count || n.cards[0].id > previous, "waiting notifications retain arrival order");
        if (n.count) previous = n.cards[0].id;
    }
    cr_notifications_update(&n, NAN);
    cr_notifications_update(&n, -1);
    cr_notifications_push(&n, -1, 0, "", "", "");
    check(n.count == 0 && n.elapsed == 0, "invalid updates and types leave empty state unchanged");
    char long_title[300];
    memset(long_title, 'a', sizeof(long_title));
    long_title[190] = (char)0xc3; long_title[191] = (char)0xa9; long_title[299] = 0;
    cr_notifications_push(&n, CR_NOTIFICATION_GAME, 1, long_title, NULL, NULL);
    check(strlen(n.cards[0].title) == 190 && !n.cards[0].detail[0] && !n.cards[0].image_url[0],
          "truncation preserves a complete UTF-8 prefix and accepts absent details");
}
static void progress_tests(void) {
    cr_notifications n = {0};
    cr_notifications_progress(&n, 4, "1/10", "locked", 10);
    cr_notifications_update(&n, 1.8);
    check(n.progress.visible && strcmp(n.progress.text, "1/10") == 0,
          "measured progress remains visible during its two-second window");
    cr_notifications_progress(&n, 4, "2/10", "locked", 20);
    cr_notifications_update(&n, 1.8);
    check(n.progress.visible && n.progress.percent == 20 && n.count == 0,
          "progress updates replace in place and refresh expiry without queueing cards");
    cr_notifications_hide_progress(&n, 3);
    check(n.progress.visible, "unrelated hide events do not hide current achievement");
    cr_notifications_hide_progress(&n, 4);
    check(!n.progress.visible && !n.progress.image_url[0], "matching hide clears the badge immediately");
    cr_notifications_progress(&n, 8, "50%", "badge", 50);
    cr_notifications_update(&n, 2.01);
    check(!n.progress.visible && n.progress_time == 0, "progress expires without another event");
    cr_notifications_progress(&n, 9, "unknown", "", NAN);
    check(n.progress.percent < 0, "missing measured percentage does not create a fake bar");
    cr_notifications_progress(&n, 9, "125%", "", 125);
    check(n.progress.percent == 100, "over-range percentages cannot overflow a progress bar");
    cr_notifications_push(&n, CR_NOTIFICATION_UNLOCK, 5, "Succès", "", "");
    cr_notifications_clear(&n);
    check(!n.count && !n.progress.visible && !n.elapsed && !n.progress_time,
          "unload clears queued cards and progress together");
}
typedef struct {
    float width, top, bottom;
    unsigned rectangles, texts, icons, images, clips;
    bool ready, measured_text, clipped;
    float clip_x, clip_y, clip_w, clip_h;
    float card_x, card_y, card_w, card_h, badge_x, badge_y, text_x, panel_x, panel_w;
} capture;
static void bounds(capture *c, float x, float y, float width, float height) {
    check(isfinite(x) && isfinite(y) && isfinite(width) && isfinite(height) && width >= 0 && height >= 0,
          "animation issues finite nonnegative primitives");
    if (c->clipped) {
        float right = fminf(x + width, c->clip_x + c->clip_w);
        float bottom = fminf(y + height, c->clip_y + c->clip_h);
        x = fmaxf(x, c->clip_x); y = fmaxf(y, c->clip_y);
        width = right - x; height = bottom - y;
        if (width <= 0 || height <= 0) return;
    }
    check(x >= -.01f && x + width <= c->width + .01f && y >= c->top - .01f &&
          y + height <= c->bottom + .01f, "visible notification pixels remain inside the safe area");
}
static void rectangle(void *context, float x, float y, float width, float height, uint32_t color) {
    capture *c = context;
    if ((color >> 24) == 247 && !c->card_w) {
        c->card_x = x; c->card_y = y; c->card_w = width; c->card_h = height;
    }
    bounds(c, x, y, width, height);
    ++c->rectangles;
}
static void text(void *context, const char *value, float x, float y, float size,
                 uint32_t color, float max_width) {
    (void)color;
    capture *c = context;
    bounds(c, x, y - size, max_width, size);
    if (!c->texts) c->text_x = x;
    ++c->texts;
    c->measured_text |= strcmp(value, "5/10") == 0;
}
static float measure(void *context, const char *value, float size) {
    (void)context;
    return (float)strlen(value) * size * .5f;
}
static void icon(void *context, int kind, float x, float y, float size, uint32_t color) {
    (void)color;
    capture *c = context;
    check(kind == CR_ICON_CROW, "missing images use the original Corvus fallback");
    bounds(c, x, y, size, size);
    ++c->icons;
}
static bool image(void *context, const char *url, float x, float y, float width, float height,
                  float opacity) {
    capture *c = context;
    check(url[0] && isfinite(opacity) && opacity >= 0 && opacity <= 1,
          "image request has a copied URL and bounded alpha");
    if (!c->images) { c->badge_x = x; c->badge_y = y; }
    bounds(c, x, y, width, height);
    ++c->images;
    return c->ready;
}
static void clip(void *context, float x, float y, float width, float height) {
    capture *c = context;
    check(isfinite(x) && isfinite(y) && isfinite(width) && isfinite(height) && width >= 0 && height >= 0,
          "clip geometry is valid throughout the animation");
    c->clipped = true; c->clip_x = x; c->clip_y = y; c->clip_w = width; c->clip_h = height;
    if (x > 0 && width < c->width) { c->panel_x = x; c->panel_w = width; }
    ++c->clips;
}
static cr_painter painter(capture *c) {
    return (cr_painter){.context = c, .rect = rectangle, .text = text, .measure = measure,
                        .icon = icon, .clip = clip, .image = image};
}
static void draw_tests(void) {
    cr_notifications n = {0};
    cr_notifications_push(&n, CR_NOTIFICATION_UNLOCK, 1, "Succès débloqué", "Nom long du succès - 10 points", "badge");
    cr_notifications_progress(&n, 2, "5/10", "locked", 50);
    cr_notifications_update(&n, 0);
    cr_notifications_update(&n, .8);
    capture c = {.width = 360, .bottom = 640};
    cr_painter p = painter(&c);
    cr_notifications_draw(&n, &p, 360, 640, cr_theme_get(CR_THEME_CORVUS));
    check(c.images == 2 && c.icons == 2 && c.measured_text, "both notifications display real measured text with image fallbacks");
    c = (capture){.width = 360, .bottom = 640, .ready = true};
    cr_notifications_draw(&n, &p, 360, 640, cr_theme_get(CR_THEME_CORVUS));
    check(c.images == 2 && c.icons == 0 && c.measured_text, "late images replace fallbacks without replacing notification state");
    check(c.clips > 0 && c.clip_x == 0 && c.clip_y == 0 && c.clip_w == 360 && c.clip_h == 640,
          "card text clipping restores the supplied region");
    const float sizes[][2] = {{240, 320}, {360, 640}, {720, 320}, {100, 100}, {320, 48}, {32, 24}};
    const double times[] = {.01, .1, .33, .495, .66, 2, 4.825, 4.99, 5.2, 5.319};
    for (int theme = 0; theme < CR_THEME_COUNT; ++theme)
        for (size_t i = 0; i < sizeof(sizes) / sizeof(sizes[0]); ++i)
            for (size_t j = 0; j < sizeof(times) / sizeof(times[0]); ++j) {
                n.elapsed = times[j];
                c = (capture){.width = sizes[i][0], .bottom = sizes[i][1], .ready = true};
                cr_notifications_draw(&n, &p, c.width, c.bottom, cr_theme_get(cr_theme_at(theme)));
                check(c.images >= 1, "entry, hold and exit remain drawable at phone and tiny dimensions");
            }
    n.elapsed = .8;
    c = (capture){.width = 720, .top = 80, .bottom = 200, .ready = true};
    cr_notifications_draw_area(&n, &p, c.width, c.top, c.bottom, cr_theme_get(CR_THEME_ASUS));
    check(c.images == 1 && !c.measured_text && fabs(n.progress_time - 1.2) < .001,
          "small control-free area prioritizes the card without changing progress expiry");
    n.count = 0;
    c = (capture){.width = 720, .top = 80, .bottom = 200, .ready = true};
    cr_notifications_draw_area(&n, &p, c.width, c.top, c.bottom, cr_theme_get(CR_THEME_ASUS));
    check(c.images == 1 && c.measured_text, "progress can occupy a small safe area when no card is visible");
    c = (capture){.width = 360, .bottom = 640};
    cr_notifications_draw_area(&n, &p, NAN, 0, 640, cr_theme_get(CR_THEME_CORVUS));
    cr_notifications_draw_area(&n, &p, 360, 100, 90, cr_theme_get(CR_THEME_CORVUS));
    check(!c.rectangles && !c.texts && !c.images, "invalid area produces no drawing");
}
static capture frame(cr_notifications *n, double time) {
    capture c = {.width = 360, .top = 20, .bottom = 640, .ready = true};
    cr_painter p = painter(&c);
    n->elapsed = time;
    cr_notifications_draw_area(n, &p, c.width, c.top, c.bottom, cr_theme_get(CR_THEME_CORVUS));
    return c;
}
static void animation_tests(void) {
    for (int kind = CR_NOTIFICATION_GAME; kind <= CR_NOTIFICATION_UNLOCK; ++kind) {
        cr_notifications n = {0};
        cr_notifications_push(&n, kind, 1, "Titre du jeu ou succès", "Informations de la notification", "image");
        cr_notifications_update(&n, 20);
        check(n.count == 1 && n.elapsed == 0, "slow loading cannot consume an unseen descent");
        capture falling = frame(&n, .1), arrived = frame(&n, .33);
        check(fabsf(falling.card_x + falling.card_w / 2 - 180) < .01f &&
              fabsf(falling.card_w - falling.card_h) < .01f && falling.badge_y < arrived.badge_y &&
              !falling.texts && !arrived.texts, "the centered icon descends alone before any title is shown");
        capture opening = frame(&n, .495), held = frame(&n, 1), late = frame(&n, 4.5);
        check(opening.card_w > arrived.card_w && opening.card_w < held.card_w &&
              fabsf(opening.card_x + opening.card_w / 2 - 180) < .01f,
              "horizontal opening keeps the entire growing popup centered");
        check(opening.texts == 2 && opening.text_x < opening.panel_x && opening.panel_w > 0,
              "the title slides behind the expanding panel clip");
        check(held.card_w == late.card_w && held.badge_y == late.badge_y && held.texts == 2,
              "the expanded title remains still during the readable hold");
        capture folding = frame(&n, 4.825), folded = frame(&n, 4.99), lifting = frame(&n, 5.155);
        check(folding.card_w < held.card_w && folding.card_w > arrived.card_w &&
              folding.badge_y == held.badge_y && fabsf(folding.card_x + folding.card_w / 2 - 180) < .01f,
              "the title folds back while the icon stays at its final height");
        check(!folded.texts && !lifting.texts && fabsf(folded.card_w - arrived.card_w) < .01f &&
              fabsf(lifting.card_x + lifting.card_w / 2 - 180) < .01f && lifting.badge_y < folded.badge_y,
              "only the centered icon rises after horizontal folding is complete");
        frame(&n, 5.3); cr_notifications_update(&n, .03);
        check(!n.count, "the next notification waits for the complete 5.32-second sequence");
    }
    cr_notifications n = {0};
    cr_notifications_push(&n, CR_NOTIFICATION_GAME, 1, "Jeu", "", "");
    cr_notifications_update(&n, 0); cr_notifications_update(&n, 1);
    cr_notifications_push(&n, CR_NOTIFICATION_GAME, 1, "Jeu identifié", "", "image");
    check(fabs(n.elapsed - .66) < .001 && n.count == 1, "late game metadata refreshes the hold without replaying descent");
    n.elapsed = 4.8;
    cr_notifications_push(&n, CR_NOTIFICATION_GAME, 1, "Jeu identifié", "", "image");
    check(n.elapsed == 4.8, "late images do not reverse an ongoing exit");
}
int main(void) {
    animation_tests();
    queue_tests();
    progress_tests();
    draw_tests();
    printf("PASS %u notification queue, progress, image and safe-area checks\n", checks);
    return 0;
}
