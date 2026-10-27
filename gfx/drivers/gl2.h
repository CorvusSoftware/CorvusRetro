// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
#ifndef CORVUS_GL2_H
#define CORVUS_GL2_H
#include "android_backend.h"
#include <EGL/egl.h>
#include <GLES2/gl2.h>
typedef struct {
    float x, y, u, v, r, g, b, a;
} cr_vertex;
typedef struct {
    char url[512];
    GLuint texture;
    int width, height;
    uint64_t stamp;
} cr_gl_image;
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
    cr_gl_image images[16];
} cr_gl;
bool cr_gl_init(cr_gl *, cr_android *);
void cr_gl_destroy(cr_gl *);
void cr_gl_begin(cr_gl *, uint32_t system_bar);
bool cr_gl_end(cr_gl *);
cr_painter cr_gl_painter(cr_gl *);
#endif
