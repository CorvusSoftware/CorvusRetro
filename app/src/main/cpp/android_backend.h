// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
#ifndef CORVUS_ANDROID_BACKEND_H
#define CORVUS_ANDROID_BACKEND_H
#include "render.h"
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <android_native_app_glue.h>
#include <jni.h>
typedef struct {
    struct android_app *app;
    JNIEnv *env;
    jclass cls;
    bool attached;
    jmethodID density, safe_area, atlas, advances, glyph, advance, games, root, credits, preference,
        set_preference, pick, finish, touch, appearance, quit, refresh_catalog, install_pack,
        installed_modules, install_pending;
} cr_android;
typedef struct {
    float x, y, u, v, r, g, b, a;
} cr_vertex;
typedef struct {
    cr_android *platform;
    EGLDisplay display;
    EGLSurface surface;
    EGLContext context;
    GLuint program, atlas, game, vbo;
    GLint dimensions, mode;
    int full_width, full_height, left, top, pixel_width, pixel_height, game_width, game_height;
    float width, height, density;
    cr_vertex vertices[12288];
    size_t count;
    float advances[480];
    uint32_t codepoints[480];
    uint64_t stamps[480], stamp;
} cr_gl;
bool cr_android_init(cr_android *, struct android_app *);
void cr_android_destroy(cr_android *);
cr_platform cr_android_services(cr_android *);
float cr_android_density(cr_android *);
void cr_android_root(cr_android *, char *, size_t);
bool cr_android_import(cr_import *);
bool cr_android_core_catalog(cr_core_catalog *);
bool cr_android_core_status(char *, size_t, bool *);
bool cr_gl_init(cr_gl *, cr_android *);
void cr_gl_destroy(cr_gl *);
void cr_gl_begin(cr_gl *, uint32_t system_bar);
bool cr_gl_end(cr_gl *);
cr_painter cr_gl_painter(cr_gl *);
#endif
