// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
#ifndef CORVUS_ANDROID_BACKEND_H
#define CORVUS_ANDROID_BACKEND_H
#include "render.h"
#include "controller_ports.h"
#include <android_native_app_glue.h>
#include <jni.h>
typedef struct {
    struct android_app *app;
    JNIEnv *env;
    jclass cls;
    bool attached;
    jmethodID density, safe_area, atlas, advances, glyph, advance, games, root, credits, preference,
        set_preference, pick, finish, touch, appearance, quit, refresh_catalog, install_pack,
        installed_modules, install_pending, achievement_request, achievement_credentials,
        achievement_credentials_set, edit_text, achievement_image;
} cr_android;
bool cr_android_init(cr_android *, struct android_app *);
void cr_android_destroy(cr_android *);
cr_platform cr_android_services(cr_android *);
float cr_android_density(cr_android *);
void cr_android_root(cr_android *, char *, size_t);
bool cr_android_import(cr_import *);
bool cr_android_core_catalog(cr_core_catalog *);
bool cr_android_update_modules(void);
bool cr_android_core_status(char *, size_t, bool *);
bool cr_android_text_result(int *, char *, size_t, bool *);
bool cr_android_controller_devices(cr_controller_device devices[CR_CONTROLLER_DEVICE_CAP], size_t *count);
bool cr_android_clear_error(cr_android *);
#endif
