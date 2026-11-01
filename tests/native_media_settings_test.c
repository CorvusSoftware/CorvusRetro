// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
#include "play_layout.h"
#include "overlay.h"
#include "volume.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

static void check(bool ok, const char *message) {
    if (!ok) { fprintf(stderr, "FAIL %s\n", message); exit(1); }
}
static bool inside(cr_area a, float w, float h) {
    return isfinite(a.x) && isfinite(a.y) && a.x >= -.01f && a.y >= -.01f &&
           a.w > 0 && a.h > 0 && a.x + a.w <= w + .01f && a.y + a.h <= h + .01f;
}
static bool overlaps(cr_area a, cr_area b) {
    return a.x < b.x + b.w - .01f && b.x < a.x + a.w - .01f &&
           a.y < b.y + b.h - .01f && b.y < a.y + a.h - .01f;
}
static void layout_settings(void) {
    const float dimensions[][2] = {{320,480},{360,640},{384,696},{600,960},{720,320},
                                   {900,450},{320,240},{100,60},{60,100},{1920,1080}};
    for (int system = 0; system < 3; ++system)
        for (unsigned n = 0; n < sizeof(dimensions) / sizeof(dimensions[0]); ++n)
            for (int size = 100; size <= 130; size += 10)
                for (int vertical = -15; vertical <= 15; vertical += 15) {
                    float w = dimensions[n][0], h = dimensions[n][1];
                    cr_play_layout p = cr_play_layout_configured(w,h,system,0,0,size,vertical);
                    check(inside(p.menu,w,h) && inside(p.hide,w,h) && inside(p.dpad,w,h),
                          "configured menu, hide and dpad stay inside safe viewport");
                    check(inside(p.video,w,h), "configured video stays inside viewport");
                    check(!overlaps(p.video,p.dpad), "configured cross never covers video");
                    for (int i = 0; i < p.count; ++i) {
                        cr_control b = p.controls[i];
                        check(inside(b.area,w,h), "every configured button remains usable inside viewport");
                        check(!overlaps(p.video,b.area), "configured button does not cover video");
                        float x = b.area.x + b.area.w / 2, y = b.area.y + b.area.h / 2;
                        check((cr_play_hit(&p,x,y) & b.mask) == b.mask,
                              "configured visual center activates its actual button");
                        if (w >= 320 && h >= 480)
                            check(b.area.w >= 48 && b.area.h >= 48,
                                  "phone targets retain at least48dp at every allowed size");
                    }
                }
    cr_play_layout normal = cr_play_layout_make(600,960,2,0,0);
    cr_play_layout larger = cr_play_layout_configured(600,960,2,0,0,130,0);
    cr_play_layout raised = cr_play_layout_configured(600,960,2,0,0,100,-15);
    cr_play_layout lowered = cr_play_layout_configured(600,960,2,0,0,100,15);
    check(larger.dpad.w > normal.dpad.w && larger.controls[4].area.w > normal.controls[4].area.w,
          "size setting enlarges real touch and drawing geometry when space is available");
    check(raised.dpad.y < normal.dpad.y && raised.menu.y < normal.menu.y,
          "portrait vertical setting raises controls and their centered menu");
    check(lowered.dpad.y > normal.dpad.y, "portrait vertical setting lowers controls within bounds");
    check(raised.panel.y < normal.panel.y && lowered.panel.y > normal.panel.y &&
          raised.video.h <= normal.video.h && lowered.video.h >= normal.video.h,
          "portrait video reservation follows actual control position");
    cr_play_layout landscape = cr_play_layout_make(900,450,2,0,0);
    raised = cr_play_layout_configured(900,450,2,0,0,100,-15);
    lowered = cr_play_layout_configured(900,450,2,0,0,100,15);
    check(raised.dpad.y < landscape.dpad.y && lowered.dpad.y > landscape.dpad.y,
          "landscape primary controls move between shoulders and utility row");
    check(raised.menu.y == landscape.menu.y && lowered.hide.y == landscape.hide.y,
          "landscape menu and overlay toggle remain anchored and accessible");
}
static void integer_scaling(void) {
    cr_area box = {21.2f,10.4f,330,297};
    cr_area integer = cr_play_video_integer(box,160,144,1.25f);
    check(fabsf(integer.w * 1.25f - 320) < .001f && fabsf(integer.h * 1.25f - 288) < .001f,
          "integer mode uses a whole physical pixel multiple even at fractional density");
    check(fabsf(integer.x * 1.25f - roundf(integer.x * 1.25f)) < .001f &&
          fabsf(integer.y * 1.25f - roundf(integer.y * 1.25f)) < .001f,
          "integer image origin is aligned to physical pixel boundaries");
    check(integer.x >= box.x && integer.y >= box.y && integer.x + integer.w <= box.x + box.w &&
          integer.y + integer.h <= box.y + box.h, "integer video stays inside reserved control gutters");
    box = (cr_area){0,0,100,70}; integer = cr_play_video_integer(box,240,160,1.25f);
    check(integer.w == box.w && integer.h == box.h,
          "small windows preserve native fit when no whole source multiple fits");
    integer = cr_play_video_integer(box,240,160,NAN);
    check(integer.w == box.w && integer.h == box.h, "invalid density preserves safe native fit");
}
typedef struct { unsigned first, last, count; } paint;
static void line(void *context, float x1, float y1, float x2, float y2, float width, uint32_t color) {
    (void)x1; (void)y1; (void)x2; (void)y2; (void)width;
    paint *p = context;
    if (!p->count) p->first = color >> 24;
    p->last = color >> 24; ++p->count;
}
static void opacity_settings(void) {
    cr_play_layout layout = cr_play_layout_make(384,696,0,0,0);
    float feedback[10] = {0}; paint capture = {0};
    cr_painter painter = {.context=&capture,.line=line};
    cr_overlay_draw(&layout,&painter,0,feedback,true,false,false);
    check(capture.first == 128 && capture.last == 160, "default resting opacity remains compatible");
    unsigned count = capture.count;
    capture = (paint){0}; cr_overlay_draw_configured(&layout,&painter,0,feedback,true,false,false,50);
    check(capture.first == 64 && capture.count == count,
          "opacity setting changes resting controls without changing their geometry");
    check(capture.last == 160, "menu and hide stay visible independently of game button opacity");
    capture = (paint){0}; cr_overlay_draw_configured(&layout,&painter,1,feedback,true,false,false,0);
    check(capture.first == 255 && capture.last == 160, "pressed feedback stays bright at low resting opacity");
}
static void audio_settings(void) {
    check(cr_audio_gain(32767,100) == 32767 && cr_audio_gain(-32768,100) == -32768,
          "full game volume preserves exact signed PCM endpoints");
    check(cr_audio_gain(30000,50) == 15000 && cr_audio_gain(-30000,50) == -15000,
          "half volume scales both positive and negative samples equally");
    check(cr_audio_gain(32767,0) == 0 && cr_audio_gain(-32768,0) == 0, "zero gain silences game PCM");
    check(cr_audio_gain(-32768,50) == -16384 && cr_audio_gain(32767,50) == 16383,
          "PCM arithmetic remains within int16 bounds");
    check(cr_audio_gain(20000,150) == 20000 && cr_audio_gain(-20000,-2) == 0,
          "invalid gains are bounded without amplification or wraparound");
}
int main(void) {
    layout_settings(); integer_scaling(); opacity_settings(); audio_settings();
    puts("PASS media settings: configured touch layout, integer video, opacity and PCM gain");
    return 0;
}
