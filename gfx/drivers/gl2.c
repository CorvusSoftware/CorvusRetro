// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
#include "gl2.h"
#include "viewport.h"
#include "engine.h"
#include <android/bitmap.h>
#include <android/log.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static GLuint shader(GLenum type, const char *source) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &source, NULL);
    glCompileShader(s);
    GLint ok = 0;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        glDeleteShader(s);
        return 0;
    }
    return s;
}
static bool bitmap_upload(cr_android *p, GLuint texture, jobject bitmap, int x, int y, bool whole) {
    if (cr_android_clear_error(p) || !bitmap)
        return false;
    AndroidBitmapInfo info;
    void *pixels = NULL;
    if (AndroidBitmap_getInfo(p->env, bitmap, &info) != ANDROID_BITMAP_RESULT_SUCCESS ||
        info.format != ANDROID_BITMAP_FORMAT_RGBA_8888 || info.stride != info.width * 4 ||
        AndroidBitmap_lockPixels(p->env, bitmap, &pixels) != ANDROID_BITMAP_RESULT_SUCCESS)
        return false;
    glBindTexture(GL_TEXTURE_2D, texture);
    if (whole)
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (GLsizei)info.width, (GLsizei)info.height, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    else
        glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, (GLsizei)info.width, (GLsizei)info.height, GL_RGBA,
                        GL_UNSIGNED_BYTE, pixels);
    AndroidBitmap_unlockPixels(p->env, bitmap);
    return glGetError() == GL_NO_ERROR;
}
bool cr_gl_init(cr_gl *g, cr_android *platform) {
    memset(g, 0, sizeof(*g));
    g->platform = platform;
    g->display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (g->display == EGL_NO_DISPLAY || !eglInitialize(g->display, NULL, NULL))
        return false;
    EGLint attrs[] = {EGL_RENDERABLE_TYPE,
                      EGL_OPENGL_ES2_BIT,
                      EGL_SURFACE_TYPE,
                      EGL_WINDOW_BIT,
                      EGL_RED_SIZE,
                      8,
                      EGL_GREEN_SIZE,
                      8,
                      EGL_BLUE_SIZE,
                      8,
                      EGL_ALPHA_SIZE,
                      8,
                      EGL_NONE};
    EGLConfig config;
    EGLint n = 0;
    if (!eglChooseConfig(g->display, attrs, &config, 1, &n) || !n)
        return false;
    EGLint format;
    eglGetConfigAttrib(g->display, config, EGL_NATIVE_VISUAL_ID, &format);
    ANativeWindow_setBuffersGeometry(platform->app->window, 0, 0, format);
    EGLint ca[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};
    g->context = eglCreateContext(g->display, config, EGL_NO_CONTEXT, ca);
    g->surface = eglCreateWindowSurface(g->display, config, platform->app->window, NULL);
    if (g->context == EGL_NO_CONTEXT || g->surface == EGL_NO_SURFACE ||
        !eglMakeCurrent(g->display, g->surface, g->surface, g->context))
        return false;
    const char *vs = "attribute vec2 position;attribute vec2 uv;attribute vec4 color;uniform vec2 "
                     "dimensions;varying vec2 v_uv;varying vec4 v_color;void "
                     "main(){gl_Position=vec4(position.x/dimensions.x*2.0-1.0,1.0-position.y/"
                     "dimensions.y*2.0,0.0,1.0);v_uv=uv;v_color=color;}";
    const char *fs = "precision mediump float;uniform sampler2D tex;uniform float "
                     "swap_channels;varying vec2 v_uv;varying vec4 v_color;void main(){vec4 "
                     "t=texture2D(tex,v_uv);if(swap_channels>0.5)t=t.bgra;gl_FragColor=t*vec4(v_"
                     "color.rgb*v_color.a,v_color.a);}";
    GLuint v = shader(GL_VERTEX_SHADER, vs), f = shader(GL_FRAGMENT_SHADER, fs);
    if (!v || !f)
        return false;
    g->program = glCreateProgram();
    glAttachShader(g->program, v);
    glAttachShader(g->program, f);
    glBindAttribLocation(g->program, 0, "position");
    glBindAttribLocation(g->program, 1, "uv");
    glBindAttribLocation(g->program, 2, "color");
    glLinkProgram(g->program);
    glDeleteShader(v);
    glDeleteShader(f);
    GLint linked;
    glGetProgramiv(g->program, GL_LINK_STATUS, &linked);
    if (!linked)
        return false;
    g->dimensions = glGetUniformLocation(g->program, "dimensions");
    g->mode = glGetUniformLocation(g->program, "swap_channels");
    glGenBuffers(1, &g->vbo);
    glGenTextures(1, &g->atlas);
    glGenTextures(1, &g->game);
    cr_android *p = platform;
    jobject bitmap = (*p->env)->CallObjectMethod(p->env, p->app->activity->clazz, p->atlas);
    bool ok = bitmap_upload(p, g->atlas, bitmap, 0, 0, true);
    if (bitmap)
        (*p->env)->DeleteLocalRef(p->env, bitmap);
    if (!ok)
        return false;
    jfloatArray advances =
        (*p->env)->CallObjectMethod(p->env, p->app->activity->clazz, p->advances);
    if (cr_android_clear_error(p) || !advances || (*p->env)->GetArrayLength(p->env, advances) != 233)
        return false;
    (*p->env)->GetFloatArrayRegion(p->env, advances, 0, 233, g->advances);
    (*p->env)->DeleteLocalRef(p->env, advances);
    for (int i = 0; i < 224; i++)
        g->codepoints[i] = (uint32_t)i + 32;
    const uint32_t extras[] = {0x153,  0x152,  0x2190, 0x2191, 0x2192,
                               0x2193, 0x2026, 0x2018, 0x2019};
    for (int i = 0; i < 9; i++)
        g->codepoints[224 + i] = extras[i];
    for (int i = 0; i < 2; i++) {
        glBindTexture(GL_TEXTURE_2D, i ? g->game : g->atlas);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, i ? GL_NEAREST : GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, i ? GL_NEAREST : GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    eglSwapInterval(g->display, 1);
    return true;
}
void cr_gl_destroy(cr_gl *g) {
    if (g->display != EGL_NO_DISPLAY && g->display) {
        if (g->context && g->surface)
            eglMakeCurrent(g->display, g->surface, g->surface, g->context);
        if (g->atlas)
            glDeleteTextures(1, &g->atlas);
        if (g->game)
            glDeleteTextures(1, &g->game);
        for (size_t i = 0; i < sizeof(g->images) / sizeof(g->images[0]); ++i)
            if (g->images[i].texture)
                glDeleteTextures(1, &g->images[i].texture);
        if (g->vbo)
            glDeleteBuffers(1, &g->vbo);
        if (g->program)
            glDeleteProgram(g->program);
        eglMakeCurrent(g->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (g->surface)
            eglDestroySurface(g->display, g->surface);
        if (g->context)
            eglDestroyContext(g->display, g->context);
        eglTerminate(g->display);
    }
    memset(g, 0, sizeof(*g));
}
static void flush(cr_gl *g) {
    if (!g->count)
        return;
    glBindBuffer(GL_ARRAY_BUFFER, g->vbo);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(g->count * sizeof(cr_vertex)), g->vertices,
                 GL_STREAM_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(cr_vertex),
                          (void *)offsetof(cr_vertex, x));
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(cr_vertex),
                          (void *)offsetof(cr_vertex, u));
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(cr_vertex),
                          (void *)offsetof(cr_vertex, r));
    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)g->count);
    g->count = 0;
}
static void vertex(cr_gl *g, float x, float y, float u, float v, uint32_t color) {
    g->vertices[g->count++] = (cr_vertex){x,
                                          y,
                                          u,
                                          v,
                                          ((color >> 16) & 255) / 255.f,
                                          ((color >> 8) & 255) / 255.f,
                                          (color & 255) / 255.f,
                                          ((color >> 24) & 255) / 255.f};
}
static void quad(cr_gl *g, float x, float y, float w, float h, float u, float v, float uw, float vh,
                 uint32_t c) {
    if (g->count + 6 > sizeof(g->vertices) / sizeof(g->vertices[0]))
        flush(g);
    vertex(g, x, y, u, v, c);
    vertex(g, x + w, y, u + uw, v, c);
    vertex(g, x, y + h, u, v + vh, c);
    vertex(g, x, y + h, u, v + vh, c);
    vertex(g, x + w, y, u + uw, v, c);
    vertex(g, x + w, y + h, u + uw, v + vh, c);
}
static void solid(void *ctx, float x, float y, float w, float h, uint32_t c) {
    quad(ctx, x, y, w, h, 962.f / 1024, 1986.f / 2048, 0, 0, c);
}
static void line(void *ctx, float x1, float y1, float x2, float y2, float width, uint32_t color) {
    float length = hypotf(x2 - x1, y2 - y1);
    if (!isfinite(length) || !isfinite(width) || length <= 0 || width <= 0) return;
    cr_gl *g = ctx;
    if (g->count + 6 > sizeof(g->vertices) / sizeof(g->vertices[0])) flush(g);
    float dx = (y1 - y2) * width / (2 * length);
    float dy = (x2 - x1) * width / (2 * length);
    const float u = 962.f / 1024, v = 1986.f / 2048;
    vertex(g, x1 + dx, y1 + dy, u, v, color);
    vertex(g, x2 + dx, y2 + dy, u, v, color);
    vertex(g, x1 - dx, y1 - dy, u, v, color);
    vertex(g, x1 - dx, y1 - dy, u, v, color);
    vertex(g, x2 + dx, y2 + dy, u, v, color);
    vertex(g, x2 - dx, y2 - dy, u, v, color);
}
static void circle(void *ctx, float x, float y, float radius, uint32_t c) {
    cr_gl *g = ctx;
    for (int i = 0; i < 32; i++) {
        if (g->count + 3 > sizeof(g->vertices) / sizeof(g->vertices[0]))
            flush(g);
        float a = i * 6.2831853f / 32, b = (i + 1) * 6.2831853f / 32;
        vertex(g, x, y, 962.f / 1024, 1986.f / 2048, c);
        vertex(g, x + cosf(a) * radius, y + sinf(a) * radius, 962.f / 1024, 1986.f / 2048, c);
        vertex(g, x + cosf(b) * radius, y + sinf(b) * radius, 962.f / 1024, 1986.f / 2048, c);
    }
}
static uint32_t next_utf8(const char **text) {
    const unsigned char *s = (const unsigned char *)*text;
    uint32_t c = *s++;
    if (c >= 0xc2 && c <= 0xf4) {
        int n = c < 0xe0 ? 1 : c < 0xf0 ? 2 : 3;
        c &= (1u << (6 - n)) - 1;
        for (int i = 0; i < n; i++) {
            if (!*s || (*s & 0xc0) != 0x80) {
                *text = (const char *)s;
                return '?';
            }
            c = (c << 6) | (*s++ & 63);
        }
        if (c > 0x10ffff || (c >= 0xd800 && c <= 0xdfff))
            c = '?';
    } else if (c >= 0x80)
        c = '?';
    *text = (const char *)s;
    return c;
}
static int glyph_index(cr_gl *g, uint32_t cp) {
    if (cp >= 32 && cp <= 255)
        return (int)cp - 32;
    for (int i = 224; i < 480; i++)
        if (g->codepoints[i] == cp) {
            g->stamps[i] = ++g->stamp;
            return i;
        }
    int slot = 233;
    for (int i = 233; i < 480; i++)
        if (!g->codepoints[i]) {
            slot = i;
            break;
        } else if (g->stamps[i] < g->stamps[slot])
            slot = i;
    flush(g);
    cr_android *p = g->platform;
    jobject bitmap =
        (*p->env)->CallObjectMethod(p->env, p->app->activity->clazz, p->glyph, (jint)cp);
    bool ok = bitmap_upload(p, g->atlas, bitmap, (slot % 16) * 64, (slot / 16) * 64, false);
    if (bitmap)
        (*p->env)->DeleteLocalRef(p->env, bitmap);
    if (!ok)
        return '?' - 32;
    float advance =
        (*p->env)->CallFloatMethod(p->env, p->app->activity->clazz, p->advance, (jint)cp);
    if (cr_android_clear_error(p))
        return '?' - 32;
    g->codepoints[slot] = cp;
    g->advances[slot] = advance;
    g->stamps[slot] = ++g->stamp;
    return slot;
}
static float measure(void *ctx, const char *text, float size) {
    cr_gl *g = ctx;
    float w = 0;
    while (*text) {
        uint32_t cp = next_utf8(&text);
        if (cp == '\n')
            break;
        int slot = glyph_index(g, cp);
        w += g->advances[slot] * size / 40;
    }
    return w;
}
static void draw_text(void *ctx, const char *text, float x, float baseline, float size,
                      uint32_t color, float max) {
    cr_gl *g = ctx;
    float end = x + max;
    while (*text) {
        uint32_t cp = next_utf8(&text);
        if (cp == '\n')
            break;
        int i = glyph_index(g, cp);
        float advance = g->advances[i] * size / 40;
        if (x + advance > end)
            break;
        quad(g, x, baseline - 46 * size / 40, 64 * size / 40, 64 * size / 40,
             ((i % 16) * 64 + .5f) / 1024, ((i / 16) * 64 + .5f) / 2048, 63.f / 1024, 63.f / 2048,
             color);
        x += advance;
    }
}
static void icon(void *ctx, int index, float x, float y, float size, uint32_t color) {
    int slot = 480 + index;
    quad(ctx, x, y, size, size, (slot % 16 * 64 + 8) / 1024.f, (slot / 16 * 64 + 8) / 2048.f,
         48.f / 1024, 48.f / 2048, color);
}
static void clip(void *ctx, float x, float y, float w, float h) {
    cr_gl *g = ctx;
    flush(g);
    int left = g->left + (int)floorf(x * g->density),
        bottom = g->full_height - g->top - (int)ceilf((y + h) * g->density);
    glEnable(GL_SCISSOR_TEST);
    glScissor(left, bottom, (int)ceilf(w * g->density), (int)ceilf(h * g->density));
}
static void game(void *ctx, const uint32_t *pixels, int width, int height, float x, float y,
                 float w, float h) {
    cr_gl *g = ctx;
    flush(g);
    glBindTexture(GL_TEXTURE_2D, g->game);
    if (g->game_width != width || g->game_height != height) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                     pixels);
        g->game_width = width;
        g->game_height = height;
    } else
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    glUniform1f(g->mode, 1);
    quad(g, x, y, w, h, 0, 0, 1, 1, 0xffffffff);
    flush(g);
    glBindTexture(GL_TEXTURE_2D, g->atlas);
    glUniform1f(g->mode, 0);
}
static cr_gl_image *image_texture(cr_gl *g, const char *url) {
    cr_gl_image *oldest = &g->images[0];
    for (size_t i = 0; i < sizeof(g->images) / sizeof(g->images[0]); ++i) {
        cr_gl_image *image = &g->images[i];
        if (image->texture && strcmp(image->url, url) == 0) {
            image->stamp = ++g->stamp;
            return image;
        }
        if (image->stamp < oldest->stamp)
            oldest = image;
    }
    cr_android *p = g->platform;
    jstring address = (*p->env)->NewStringUTF(p->env, url);
    if (!address) {
        cr_android_clear_error(p);
        return NULL;
    }
    jobject bitmap = (*p->env)->CallObjectMethod(p->env, p->app->activity->clazz,
                                                p->achievement_image, address);
    (*p->env)->DeleteLocalRef(p->env, address);
    bool error = cr_android_clear_error(p);
    AndroidBitmapInfo info;
    if (error || !bitmap || AndroidBitmap_getInfo(p->env, bitmap, &info) != ANDROID_BITMAP_RESULT_SUCCESS ||
        info.format != ANDROID_BITMAP_FORMAT_RGBA_8888 || !info.width || !info.height ||
        info.width > 96 || info.height > 96 || info.stride != info.width * 4) {
        if (bitmap) (*p->env)->DeleteLocalRef(p->env, bitmap);
        return NULL;
    }
    flush(g);
    GLuint texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    bool uploaded = texture && bitmap_upload(p, texture, bitmap, 0, 0, true);
    (*p->env)->DeleteLocalRef(p->env, bitmap);
    glBindTexture(GL_TEXTURE_2D, g->atlas);
    glUniform1f(g->mode, 0);
    if (!uploaded) {
        if (texture) glDeleteTextures(1, &texture);
        return NULL;
    }
    if (oldest->texture) glDeleteTextures(1, &oldest->texture);
    snprintf(oldest->url, sizeof(oldest->url), "%s", url);
    oldest->texture = texture;
    oldest->width = (int)info.width;
    oldest->height = (int)info.height;
    oldest->stamp = ++g->stamp;
    return oldest;
}
static bool image_ready(void *ctx, const char *url) {
    return url && url[0] && strlen(url) < sizeof(((cr_gl_image *)0)->url) &&
           image_texture(ctx, url) != NULL;
}
static bool image(void *ctx, const char *url, float x, float y, float w, float h, float alpha) {
    if (!url || !url[0] || strlen(url) >= sizeof(((cr_gl_image *)0)->url) ||
        !isfinite(x) || !isfinite(y) || !isfinite(w) || !isfinite(h) || !isfinite(alpha) ||
        w <= 0 || h <= 0)
        return false;
    cr_gl *g = ctx;
    cr_gl_image *entry = image_texture(g, url);
    if (!entry) return false;
    float scale = fminf(w / entry->width, h / entry->height);
    float width = entry->width * scale, height = entry->height * scale;
    uint32_t opacity = (uint32_t)(fminf(1, fmaxf(0, alpha)) * 255 + .5f);
    flush(g);
    glBindTexture(GL_TEXTURE_2D, entry->texture);
    glUniform1f(g->mode, 0);
    quad(g, x + (w - width) / 2, y + (h - height) / 2, width, height,
         0, 0, 1, 1, (opacity << 24) | 0xffffff);
    flush(g);
    glBindTexture(GL_TEXTURE_2D, g->atlas);
    glUniform1f(g->mode, 0);
    return true;
}
void cr_gl_begin(cr_gl *g, uint32_t system_bar) {
    eglQuerySurface(g->display, g->surface, EGL_WIDTH, &g->full_width);
    eglQuerySurface(g->display, g->surface, EGL_HEIGHT, &g->full_height);
    cr_android *p = g->platform;
    ARect content = p->app->contentRect;
    jint area[6];
    jintArray snapshot = (*p->env)->CallObjectMethod(p->env, p->app->activity->clazz, p->safe_area);
    bool valid = !cr_android_clear_error(p) && snapshot && (*p->env)->GetArrayLength(p->env, snapshot) == 6;
    if (valid) {
        (*p->env)->GetIntArrayRegion(p->env, snapshot, 0, 6, area);
        valid = !cr_android_clear_error(p);
    }
    if (snapshot)
        (*p->env)->DeleteLocalRef(p->env, snapshot);
    cr_rect r = cr_viewport(g->full_width, g->full_height, valid ? area : NULL,
                            (cr_rect){content.left, content.top, content.right, content.bottom});
#ifndef NDEBUG
    if (g->left != r.left || g->top != r.top || g->pixel_width != r.right - r.left ||
        g->pixel_height != r.bottom - r.top)
        __android_log_print(ANDROID_LOG_DEBUG, "CorvusRetro", "viewport surface=%dx%d safe=%d,%d,%d,%d",
                            g->full_width, g->full_height, r.left, r.top, r.right, r.bottom);
#endif
    g->left = r.left;
    g->top = r.top;
    g->pixel_width = r.right - r.left;
    g->pixel_height = r.bottom - r.top;
    g->density = cr_android_density(g->platform);
    g->width = g->pixel_width / g->density;
    g->height = g->pixel_height / g->density;
    glDisable(GL_SCISSOR_TEST);
    glClearColor(((system_bar >> 16) & 255) / 255.f, ((system_bar >> 8) & 255) / 255.f,
                 (system_bar & 255) / 255.f, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    glViewport(g->left, g->full_height - r.bottom, g->pixel_width, g->pixel_height);
    glUseProgram(g->program);
    glUniform2f(g->dimensions, g->width, g->height);
    glUniform1f(g->mode, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, g->atlas);
    for (int i = 0; i < 3; i++)
        glEnableVertexAttribArray((GLuint)i);
    g->count = 0;
}
bool cr_gl_end(cr_gl *g) {
    flush(g);
    return eglSwapBuffers(g->display, g->surface) == EGL_TRUE;
}
cr_painter cr_gl_painter(cr_gl *g) {
    return (cr_painter){g, solid, circle, draw_text, measure, icon, clip, game, image, image_ready, line};
}
