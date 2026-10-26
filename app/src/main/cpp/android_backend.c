// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
#include "android_backend.h"
#include "viewport.h"
#include "engine.h"
#include "core_registry.h"
#include <android/bitmap.h>
#include <android/log.h>
#include <math.h>
#include <pthread.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static pthread_mutex_t import_mutex = PTHREAD_MUTEX_INITIALIZER;
static cr_import import_value;
static bool import_ready;
static ALooper *import_looper;
static pthread_mutex_t core_mutex = PTHREAD_MUTEX_INITIALIZER;
static char pending_modules[CR_MAX_CORES][4096];
static size_t pending_module_count;
static bool modules_ready;
static cr_core_catalog pending_catalog;
static bool catalog_ready;
static char core_status[256];
static bool core_status_ready;
static bool core_status_finished;

static void wake_frontend(void) {
    pthread_mutex_lock(&import_mutex);
    if (import_looper) ALooper_wake(import_looper);
    pthread_mutex_unlock(&import_mutex);
}

static bool clear_error(cr_android *p) {
    if (!(*p->env)->ExceptionCheck(p->env))
        return false;
    (*p->env)->ExceptionClear(p->env);
    __android_log_write(ANDROID_LOG_ERROR, "CorvusRetro", "Erreur de service Android");
    return true;
}
static void string_copy(JNIEnv *env, jstring value, char *out, size_t cap) {
    if (!cap)
        return;
    out[0] = 0;
    if (!value)
        return;
    const jchar *src = (*env)->GetStringChars(env, value, NULL);
    if (!src)
        return;
    jsize length = (*env)->GetStringLength(env, value);
    size_t at = 0;
    for (jsize i = 0; i < length; i++) {
        uint32_t c = src[i];
        if (c >= 0xd800 && c <= 0xdbff && i + 1 < length && src[i + 1] >= 0xdc00 &&
            src[i + 1] <= 0xdfff) {
            c = 0x10000 + ((c - 0xd800) << 10) + (src[++i] - 0xdc00);
        } else if (c >= 0xd800 && c <= 0xdfff)
            c = '?';
        if (!c)
            c = '?';
        unsigned n = c < 0x80 ? 1 : c < 0x800 ? 2 : c < 0x10000 ? 3 : 4;
        if (at + n >= cap)
            break;
        if (n == 1)
            out[at++] = (char)c;
        else {
            if (n == 2)
                out[at++] = (char)(0xc0 | (c >> 6));
            else if (n == 3)
                out[at++] = (char)(0xe0 | (c >> 12));
            else
                out[at++] = (char)(0xf0 | (c >> 18));
            if (n >= 4)
                out[at++] = (char)(0x80 | ((c >> 12) & 63));
            if (n >= 3)
                out[at++] = (char)(0x80 | ((c >> 6) & 63));
            out[at++] = (char)(0x80 | (c & 63));
        }
    }
    out[at] = 0;
    (*env)->ReleaseStringChars(env, value, src);
}
JNIEXPORT void JNICALL Java_com_corvussoftware_corvusretro_MainActivity_coreCatalogUpdated(
    JNIEnv *env, jclass cls, jobjectArray rows, jstring error) {
    (void)cls;
    cr_core_catalog next = {0};
    string_copy(env, error, next.error, sizeof(next.error));
    jsize size = rows ? (*env)->GetArrayLength(env, rows) : 0;
    if (size < 0 || size > CR_MAX_CORE_PACKS * 5 || size % 5 != 0) {
        snprintf(next.error, sizeof(next.error), "Catalogue incompatible");
    } else {
        for (jsize i = 0; i < size; i += 5) {
            cr_core_pack *pack = &next.packs[next.count];
            for (int f = 0; f < 5; ++f) {
                jstring field = (*env)->GetObjectArrayElement(env, rows, i + f);
                if (f == 0) string_copy(env, field, pack->id, sizeof(pack->id));
                if (f == 1) string_copy(env, field, pack->name, sizeof(pack->name));
                if (f == 2) string_copy(env, field, pack->version, sizeof(pack->version));
                if (f == 3) string_copy(env, field, pack->core_id, sizeof(pack->core_id));
                if (f == 4) string_copy(env, field, pack->abi, sizeof(pack->abi));
                if (field) (*env)->DeleteLocalRef(env, field);
            }
            if (!pack->id[0] || !pack->name[0] || !pack->version[0] || !pack->core_id[0]) {
                next.count = 0;
                snprintf(next.error, sizeof(next.error), "Metadonnees du catalogue invalides");
                break;
            }
            next.count++;
        }
    }
    pthread_mutex_lock(&core_mutex);
    pending_catalog = next;
    catalog_ready = true;
    pthread_mutex_unlock(&core_mutex);
    wake_frontend();
}
JNIEXPORT void JNICALL Java_com_corvussoftware_corvusretro_MainActivity_coreModulesUpdated(
    JNIEnv *env, jclass cls, jobjectArray paths) {
    (void)cls;
    if (!paths) return;
    jsize count = (*env)->GetArrayLength(env, paths);
    if (count < 0 || count > CR_MAX_CORES) return;
    pthread_mutex_lock(&core_mutex);
    pending_module_count = 0;
    for (jsize i = 0; i < count; ++i) {
        jstring path = (*env)->GetObjectArrayElement(env, paths, i);
        if (path && (*env)->GetStringUTFLength(env, path) < 4096) {
            string_copy(env, path, pending_modules[pending_module_count], 4096);
            ++pending_module_count;
        }
        if (path) (*env)->DeleteLocalRef(env, path);
    }
    modules_ready = true;
    pthread_mutex_unlock(&core_mutex);
    wake_frontend();
}
bool cr_android_update_modules(void) {
    // Only this frontend-thread consumer may negotiate CorvusAPI or mutate the registry.
    pthread_mutex_lock(&core_mutex);
    bool changed = modules_ready;
    if (changed) {
        for (size_t i = 0; i < pending_module_count; ++i) {
            const char *path = pending_modules[i];
            cr_registry_add_installed_modules(&path, 1);
        }
        modules_ready = false;
    }
    pthread_mutex_unlock(&core_mutex);
    return changed;
}
JNIEXPORT void JNICALL Java_com_corvussoftware_corvusretro_MainActivity_coreInstallStatus(
    JNIEnv *env, jclass cls, jstring value, jboolean finished) {
    (void)cls;
    char message[256];
    string_copy(env, value, message, sizeof(message));
    pthread_mutex_lock(&core_mutex);
    snprintf(core_status, sizeof(core_status), "%s", message);
    core_status_ready = true;
    core_status_finished = finished;
    pthread_mutex_unlock(&core_mutex);
    wake_frontend();
}
bool cr_android_core_catalog(cr_core_catalog *out) {
    pthread_mutex_lock(&core_mutex);
    bool ready = catalog_ready;
    if (ready) {
        *out = pending_catalog;
        catalog_ready = false;
    }
    pthread_mutex_unlock(&core_mutex);
    return ready;
}
bool cr_android_core_status(char *out, size_t cap, bool *finished) {
    pthread_mutex_lock(&core_mutex);
    bool ready = core_status_ready;
    if (ready) {
        snprintf(out, cap, "%s", core_status);
        *finished = core_status_finished;
        core_status_ready = false;
    }
    pthread_mutex_unlock(&core_mutex);
    return ready;
}
JNIEXPORT void JNICALL Java_com_corvussoftware_corvusretro_MainActivity_imported(
    JNIEnv *env, jclass cls, jstring id, jstring title, jint system, jstring path, jstring error) {
    (void)cls;
    cr_import item;
    memset(&item, 0, sizeof(item));
    item.system = system;
    string_copy(env, id, item.id, sizeof(item.id));
    string_copy(env, title, item.title, sizeof(item.title));
    string_copy(env, path, item.path, sizeof(item.path));
    string_copy(env, error, item.error, sizeof(item.error));
    pthread_mutex_lock(&import_mutex);
    import_value = item;
    import_ready = true;
    if (import_looper)
        ALooper_wake(import_looper);
    pthread_mutex_unlock(&import_mutex);
}
bool cr_android_import(cr_import *out) {
    pthread_mutex_lock(&import_mutex);
    bool ready = import_ready;
    if (ready) {
        *out = import_value;
        import_ready = false;
    }
    pthread_mutex_unlock(&import_mutex);
    return ready;
}
bool cr_android_init(cr_android *p, struct android_app *app) {
    memset(p, 0, sizeof(*p));
    p->app = app;
    JavaVM *vm = app->activity->vm;
    if ((*vm)->GetEnv(vm, (void **)&p->env, JNI_VERSION_1_6) != JNI_OK) {
        if ((*vm)->AttachCurrentThread(vm, &p->env, NULL) != JNI_OK)
            return false;
        p->attached = true;
    }
    jclass local = (*p->env)->GetObjectClass(p->env, app->activity->clazz);
    p->cls = (*p->env)->NewGlobalRef(p->env, local);
    (*p->env)->DeleteLocalRef(p->env, local);
#define METHOD(field, name, sig)                                                                   \
    p->field = (*p->env)->GetMethodID(p->env, p->cls, name, sig);                                  \
    if (clear_error(p) || !p->field)                                                               \
    return false
    METHOD(density, "density", "()F");
    METHOD(safe_area, "safeWindowArea", "()[I");
    METHOD(atlas, "atlas", "()Landroid/graphics/Bitmap;");
    METHOD(advances, "advances", "()[F");
    METHOD(glyph, "glyph", "(I)Landroid/graphics/Bitmap;");
    METHOD(advance, "advance", "(I)F");
    METHOD(games, "games", "()[Ljava/lang/String;");
    METHOD(root, "filesRoot", "()Ljava/lang/String;");
    METHOD(credits, "credits", "()Ljava/lang/String;");
    METHOD(preference, "preference", "(Ljava/lang/String;I)I");
    METHOD(set_preference, "preferenceSet", "(Ljava/lang/String;I)V");
    METHOD(pick, "pickContent", "(I)V");
    METHOD(finish, "finishImport", "(ZILjava/lang/String;Ljava/lang/String;)Ljava/lang/String;");
    METHOD(touch, "touchGame", "(Ljava/lang/String;)V");
    METHOD(appearance, "appearance", "(IZ)V");
    METHOD(quit, "quit", "()V");
    METHOD(refresh_catalog, "refreshCoreCatalog", "()V");
    METHOD(install_pack, "installCorePack", "(Ljava/lang/String;)V");
    METHOD(installed_modules, "installedCoreModules", "()[Ljava/lang/String;");
    METHOD(install_pending, "coreInstallPending", "()Z");
#undef METHOD
    jobjectArray modules = (*p->env)->CallObjectMethod(p->env, app->activity->clazz,
                                                      p->installed_modules);
    if (clear_error(p)) return false;
    if (modules) {
        jsize count = (*p->env)->GetArrayLength(p->env, modules);
        if (count >= 0 && count <= CR_MAX_CORES) {
            for (jsize i = 0; i < count; ++i) {
                jstring module = (*p->env)->GetObjectArrayElement(p->env, modules, i);
                char path[4096];
                if (module && (*p->env)->GetStringUTFLength(p->env, module) < (jsize)sizeof(path)) {
                    string_copy(p->env, module, path, sizeof(path));
                    const char *entry = path;
                    cr_registry_add_installed_modules(&entry, 1);
                }
                if (module) (*p->env)->DeleteLocalRef(p->env, module);
            }
        }
        (*p->env)->DeleteLocalRef(p->env, modules);
        if (clear_error(p)) return false;
    }
    pthread_mutex_lock(&import_mutex);
    import_looper = app->looper;
    if (import_ready)
        ALooper_wake(import_looper);
    pthread_mutex_unlock(&import_mutex);
    return true;
}
void cr_android_destroy(cr_android *p) {
    pthread_mutex_lock(&import_mutex);
    import_looper = NULL;
    import_ready = false;
    pthread_mutex_unlock(&import_mutex);
    if (p->env && p->cls)
        (*p->env)->DeleteGlobalRef(p->env, p->cls);
    if (p->attached)
        (*p->app->activity->vm)->DetachCurrentThread(p->app->activity->vm);
}
float cr_android_density(cr_android *p) {
    float n = (*p->env)->CallFloatMethod(p->env, p->app->activity->clazz, p->density);
    if (clear_error(p) || n <= 0)
        n = 1;
    return n;
}
void cr_android_root(cr_android *p, char *out, size_t size) {
    jstring s = (*p->env)->CallObjectMethod(p->env, p->app->activity->clazz, p->root);
    string_copy(p->env, s, out, size);
    if (s)
        (*p->env)->DeleteLocalRef(p->env, s);
    clear_error(p);
}
static int pref(void *ctx, const char *key, int fallback) {
    cr_android *p = ctx;
    jstring s = (*p->env)->NewStringUTF(p->env, key);
    int value =
        (*p->env)->CallIntMethod(p->env, p->app->activity->clazz, p->preference, s, fallback);
    (*p->env)->DeleteLocalRef(p->env, s);
    return clear_error(p) ? fallback : value;
}
static void set_pref(void *ctx, const char *key, int value) {
    cr_android *p = ctx;
    jstring s = (*p->env)->NewStringUTF(p->env, key);
    (*p->env)->CallVoidMethod(p->env, p->app->activity->clazz, p->set_preference, s, value);
    (*p->env)->DeleteLocalRef(p->env, s);
    clear_error(p);
}
static void pick(void *ctx, int system) {
    cr_android *p = ctx;
    (*p->env)->CallVoidMethod(p->env, p->app->activity->clazz, p->pick, system);
    clear_error(p);
}
static bool finish(void *ctx, bool accept, int system, const char *core_id,
                   const char *system_id, char *error, size_t cap) {
    cr_android *p = ctx;
    jstring core = (*p->env)->NewStringUTF(p->env, core_id ? core_id : "");
    jstring machine = (*p->env)->NewStringUTF(p->env, system_id ? system_id : "");
    if (!core || !machine || clear_error(p)) {
        if (core) (*p->env)->DeleteLocalRef(p->env, core);
        if (machine) (*p->env)->DeleteLocalRef(p->env, machine);
        snprintf(error, cap, "Identité du cœur indisponible");
        return false;
    }
    jstring result = (*p->env)->CallObjectMethod(p->env, p->app->activity->clazz, p->finish,
                                                 (jboolean)accept, (jint)system, core, machine);
    (*p->env)->DeleteLocalRef(p->env, core);
    (*p->env)->DeleteLocalRef(p->env, machine);
    if (clear_error(p)) {
        snprintf(error, cap, "Import Android indisponible");
        return false;
    }
    if (result) {
        string_copy(p->env, result, error, cap);
        (*p->env)->DeleteLocalRef(p->env, result);
        return false;
    }
    return true;
}
static cr_game *games(void *ctx, size_t *count) {
    cr_android *p = ctx;
    *count = 0;
    jobjectArray records = (*p->env)->CallObjectMethod(p->env, p->app->activity->clazz, p->games);
    if (clear_error(p) || !records)
        return NULL;
    jsize length = (*p->env)->GetArrayLength(p->env, records);
    if (length < 0 || length % 6 != 0 || length / 6 > 16384) {
        (*p->env)->DeleteLocalRef(p->env, records);
        return NULL;
    }
    cr_game *result = calloc((size_t)length / 6, sizeof(*result));
    if (length && !result) {
        (*p->env)->DeleteLocalRef(p->env, records);
        return NULL;
    }
    for (jsize i = 0; i + 5 < length; i += 6) {
        cr_game game;
        memset(&game, 0, sizeof(game));
        char number[64];
        for (int f = 0; f < 6; f++) {
            jstring s = (*p->env)->GetObjectArrayElement(p->env, records, i + f);
            if (f == 0)
                string_copy(p->env, s, game.id, sizeof(game.id));
            if (f == 1)
                string_copy(p->env, s, game.title, sizeof(game.title));
            if (f == 2 || f == 3) {
                string_copy(p->env, s, number, sizeof(number));
                if (f == 2) game.system = (int)strtol(number, NULL, 10);
                else game.played = strtoll(number, NULL, 10);
            }
            if (f == 4) string_copy(p->env, s, game.core_id, sizeof(game.core_id));
            if (f == 5) string_copy(p->env, s, game.system_id, sizeof(game.system_id));
            if (s)
                (*p->env)->DeleteLocalRef(p->env, s);
        }
        if (cr_game_id_valid(game.id)) {
            /* Do not discard a title when its original core is not installed. */
            game.system = cr_registry_find(game.core_id);
            result[(*count)++] = game;
        }
    }
    (*p->env)->DeleteLocalRef(p->env, records);
    clear_error(p);
    return result;
}
static void touch(void *ctx, const char *id) {
    cr_android *p = ctx;
    jstring s = (*p->env)->NewStringUTF(p->env, id);
    (*p->env)->CallVoidMethod(p->env, p->app->activity->clazz, p->touch, s);
    (*p->env)->DeleteLocalRef(p->env, s);
    clear_error(p);
}
static char *credits(void *ctx) {
    cr_android *p = ctx;
    jstring s = (*p->env)->CallObjectMethod(p->env, p->app->activity->clazz, p->credits);
    if (clear_error(p) || !s)
        return NULL;
    jsize len = (*p->env)->GetStringLength(p->env, s);
    char *out = len < 524288 ? malloc((size_t)len * 4 + 1) : NULL;
    if (out)
        string_copy(p->env, s, out, (size_t)len * 4 + 1);
    (*p->env)->DeleteLocalRef(p->env, s);
    return out;
}
static void appearance(void *ctx, int theme, bool awake) {
    cr_android *p = ctx;
    (*p->env)->CallVoidMethod(p->env, p->app->activity->clazz, p->appearance, theme,
                              (jboolean)awake);
    clear_error(p);
}
static void quit(void *ctx) {
    cr_android *p = ctx;
    (*p->env)->CallVoidMethod(p->env, p->app->activity->clazz, p->quit);
    clear_error(p);
}
static void refresh_catalog(void *ctx) {
    cr_android *p = ctx;
    (*p->env)->CallVoidMethod(p->env, p->app->activity->clazz, p->refresh_catalog);
    clear_error(p);
}
static void install_pack(void *ctx, const char *pack_id) {
    cr_android *p = ctx;
    jstring id = (*p->env)->NewStringUTF(p->env, pack_id);
    if (!id) { clear_error(p); return; }
    (*p->env)->CallVoidMethod(p->env, p->app->activity->clazz, p->install_pack, id);
    (*p->env)->DeleteLocalRef(p->env, id);
    clear_error(p);
}
static bool install_pending(void *ctx) {
    cr_android *p = ctx;
    jboolean pending = (*p->env)->CallBooleanMethod(p->env, p->app->activity->clazz,
                                                   p->install_pending);
    return clear_error(p) || pending;
}
cr_platform cr_android_services(cr_android *p) {
    return (cr_platform){p, pref, set_pref, pick, finish, games, touch, credits,
                         appearance, quit, refresh_catalog, install_pack, install_pending};
}

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
    if (clear_error(p) || !bitmap)
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
    if (clear_error(p) || !advances || (*p->env)->GetArrayLength(p->env, advances) != 233)
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
    if (clear_error(p))
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
void cr_gl_begin(cr_gl *g, uint32_t system_bar) {
    eglQuerySurface(g->display, g->surface, EGL_WIDTH, &g->full_width);
    eglQuerySurface(g->display, g->surface, EGL_HEIGHT, &g->full_height);
    cr_android *p = g->platform;
    ARect content = p->app->contentRect;
    jint area[6];
    jintArray snapshot = (*p->env)->CallObjectMethod(p->env, p->app->activity->clazz, p->safe_area);
    bool valid = !clear_error(p) && snapshot && (*p->env)->GetArrayLength(p->env, snapshot) == 6;
    if (valid) {
        (*p->env)->GetIntArrayRegion(p->env, snapshot, 0, 6, area);
        valid = !clear_error(p);
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
    return (cr_painter){g, solid, circle, draw_text, measure, icon, clip, game};
}
