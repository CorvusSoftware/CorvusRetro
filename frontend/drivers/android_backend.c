// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
#include "android_backend.h"
#include "viewport.h"
#include "engine.h"
#include "core_registry.h"
#include "task_achievements.h"
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
static pthread_mutex_t save_mutex = PTHREAD_MUTEX_INITIALIZER;
static uint64_t save_request;
static cr_save_transfer_result save_result;
static bool save_ready, save_delivered;
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

static pthread_mutex_t text_mutex = PTHREAD_MUTEX_INITIALIZER;
static char text_value[513];
static int text_field;
static bool text_ready, text_accepted;
static pthread_mutex_t controller_mutex = PTHREAD_MUTEX_INITIALIZER;
static cr_controller_device controller_devices[CR_CONTROLLER_DEVICE_CAP];
static char controller_names[CR_CONTROLLER_DEVICE_CAP][128];
static size_t controller_count;
static bool controller_ready;
static void string_copy(JNIEnv *, jstring, char *, size_t);
static void wake_frontend(void) {
    pthread_mutex_lock(&import_mutex);
    if (import_looper) ALooper_wake(import_looper);
    pthread_mutex_unlock(&import_mutex);
}

JNIEXPORT void JNICALL Java_com_corvussoftware_corvusretro_MainActivity_controllerDevicesChanged(
    JNIEnv *env, jclass cls, jintArray devices, jlongArray generations, jobjectArray names) {
    (void)cls;
    if (!devices || !generations || !names) return;
    jsize count = (*env)->GetArrayLength(env, devices);
    if (count < 0 || count > CR_CONTROLLER_DEVICE_CAP ||
        (*env)->GetArrayLength(env, generations) != count ||
        (*env)->GetArrayLength(env, names) != count) return;
    jint next[CR_CONTROLLER_DEVICE_CAP];
    jlong epochs[CR_CONTROLLER_DEVICE_CAP];
    char descriptions[CR_CONTROLLER_DEVICE_CAP][128];
    if (count) (*env)->GetIntArrayRegion(env, devices, 0, count, next);
    if ((*env)->ExceptionCheck(env)) return;
    if (count) (*env)->GetLongArrayRegion(env, generations, 0, count, epochs);
    if ((*env)->ExceptionCheck(env)) return;
    for (jsize i = 0; i < count; ++i) {
        if (next[i] < 0 || epochs[i] <= 0) return;
        jstring name = (jstring)(*env)->GetObjectArrayElement(env, names, i);
        string_copy(env, name, descriptions[i], sizeof(descriptions[i]));
        if (name) (*env)->DeleteLocalRef(env, name);
        if ((*env)->ExceptionCheck(env)) return;
        if (!descriptions[i][0]) snprintf(descriptions[i], sizeof(descriptions[i]), "Manette");
    }
    pthread_mutex_lock(&controller_mutex);
    controller_count = (size_t)count;
    for (jsize i = 0; i < count; ++i) {
        controller_devices[i].id = next[i];
        controller_devices[i].generation = (uint64_t)epochs[i];
        memcpy(controller_names[i], descriptions[i], sizeof(controller_names[i]));
    }
    controller_ready = true;
    pthread_mutex_unlock(&controller_mutex);
    wake_frontend();
}
bool cr_android_controller_devices(cr_controller_device devices[CR_CONTROLLER_DEVICE_CAP],
                                   char names[CR_CONTROLLER_DEVICE_CAP][128], size_t *count) {
    pthread_mutex_lock(&controller_mutex);
    bool ready = controller_ready;
    if (ready) {
        *count = controller_count;
        memcpy(devices, controller_devices, controller_count * sizeof(*devices));
        memcpy(names, controller_names, controller_count * sizeof(*names));
        controller_ready = false;
    }
    pthread_mutex_unlock(&controller_mutex);
    return ready;
}

JNIEXPORT void JNICALL Java_com_corvussoftware_corvusretro_MainActivity_achievementImageReady(
    JNIEnv *env, jclass cls) {
    (void)env;
    (void)cls;
    wake_frontend();
}

JNIEXPORT void JNICALL Java_com_corvussoftware_corvusretro_MainActivity_achievementResponse(
    JNIEnv *env, jclass cls, jlong id, jint status, jbyteArray body) {
    (void)cls;
    jsize size = body ? (*env)->GetArrayLength(env, body) : 0;
    if (size > 4 * 1024 * 1024) {
        cr_achievement_response_push((uint64_t)id, -1, NULL, 0);
    } else {
        jbyte *bytes = size ? (*env)->GetByteArrayElements(env, body, NULL) : NULL;
        cr_achievement_response_push((uint64_t)id, bytes || !size ? status : -2, bytes,
                                     bytes ? (size_t)size : 0);
        if (bytes) (*env)->ReleaseByteArrayElements(env, body, bytes, JNI_ABORT);
    }
    wake_frontend();
}
bool cr_android_clear_error(cr_android *p) {
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
JNIEXPORT void JNICALL Java_com_corvussoftware_corvusretro_MainActivity_textInputResult(
    JNIEnv *env, jclass cls, jint field, jstring text, jboolean accepted) {
    (void)cls;
    pthread_mutex_lock(&text_mutex);
    string_copy(env, text, text_value, sizeof(text_value));
    text_field = field; text_accepted = accepted; text_ready = true;
    pthread_mutex_unlock(&text_mutex);
    wake_frontend();
}
bool cr_android_text_result(int *field, char *text, size_t size, bool *accepted) {
    pthread_mutex_lock(&text_mutex);
    bool ready = text_ready;
    if (ready) {
        *field = text_field; *accepted = text_accepted;
        snprintf(text, size, "%s", text_value);
        memset(text_value, 0, sizeof(text_value));
        text_ready = false;
    }
    pthread_mutex_unlock(&text_mutex);
    return ready;
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
JNIEXPORT void JNICALL Java_com_corvussoftware_corvusretro_MainActivity_saveTransferResult(
    JNIEnv *env, jclass cls, jlong request, jstring path, jstring error, jboolean cancelled) {
    (void)cls;
    if (request <= 0) return;
    cr_save_transfer_result result = {.request = (uint64_t)request, .cancelled = cancelled != JNI_FALSE};
    bool valid = (!path || (*env)->GetStringUTFLength(env, path) < (jsize)sizeof(result.path)) &&
                 (!error || (*env)->GetStringUTFLength(env, error) < (jsize)sizeof(result.error));
    if (valid) {
        string_copy(env, path, result.path, sizeof(result.path));
        string_copy(env, error, result.error, sizeof(result.error));
    } else snprintf(result.error, sizeof(result.error), "Réponse de sauvegarde trop longue");
    if ((*env)->ExceptionCheck(env)) return;
    pthread_mutex_lock(&save_mutex);
    if (save_request == (uint64_t)request && !save_delivered) {
        save_result = result; save_ready = save_delivered = true;
    }
    pthread_mutex_unlock(&save_mutex);
    wake_frontend();
}
bool cr_android_save_transfer_result(cr_save_transfer_result *out) {
    pthread_mutex_lock(&save_mutex);
    bool ready = save_ready;
    if (ready) { *out = save_result; save_ready = false; }
    pthread_mutex_unlock(&save_mutex);
    return ready;
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
    if (cr_android_clear_error(p) || !p->field)                                                               \
    return false
    METHOD(density, "density", "()F");
    METHOD(menu_metrics, "menuMetrics", "()[F");
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
    METHOD(pick_folder, "pickFolder", "(ILjava/lang/String;)V");
    METHOD(save_transfer, "saveTransfer", "(JILjava/lang/String;[B)V");
    METHOD(finish_save_transfer, "finishSaveTransfer", "(J)V");
    METHOD(finish, "finishImport", "(ZILjava/lang/String;Ljava/lang/String;)Ljava/lang/String;");
    METHOD(touch, "touchGame", "(Ljava/lang/String;)V");
    METHOD(appearance, "appearance", "(IZ)V");
    METHOD(quit, "quit", "()V");
    METHOD(refresh_catalog, "refreshCoreCatalog", "()V");
    METHOD(install_pack, "installCorePack", "(Ljava/lang/String;)V");
    METHOD(installed_modules, "installedCoreModules", "()[Ljava/lang/String;");
    METHOD(install_pending, "coreInstallPending", "()Z");
    METHOD(achievement_request, "achievementRequest", "(JLjava/lang/String;Ljava/lang/String;)V");
    METHOD(achievement_credentials, "achievementCredentials", "()[Ljava/lang/String;");
    METHOD(achievement_credentials_set, "achievementCredentialsSet", "(Ljava/lang/String;Ljava/lang/String;)V");
    METHOD(edit_text, "textInput", "(ILjava/lang/String;Z)V");
    METHOD(achievement_image, "achievementImage", "(Ljava/lang/String;)Landroid/graphics/Bitmap;");
#undef METHOD
    jobjectArray modules = (*p->env)->CallObjectMethod(p->env, app->activity->clazz,
                                                      p->installed_modules);
    if (cr_android_clear_error(p)) return false;
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
        if (cr_android_clear_error(p)) return false;
    }
    pthread_mutex_lock(&import_mutex);
    import_looper = app->looper;
    if (import_ready)
        ALooper_wake(import_looper);
    pthread_mutex_unlock(&import_mutex);
    pthread_mutex_lock(&controller_mutex);
    bool controllers_pending = controller_ready;
    pthread_mutex_unlock(&controller_mutex);
    if (controllers_pending) wake_frontend();
    return true;
}
void cr_android_destroy(cr_android *p) {
    pthread_mutex_lock(&save_mutex);
    save_request = 0; save_ready = save_delivered = false;
    memset(&save_result, 0, sizeof(save_result));
    pthread_mutex_unlock(&save_mutex);
    cr_achievement_tasks_clear();
    pthread_mutex_lock(&text_mutex);
    memset(text_value, 0, sizeof(text_value));
    text_ready = false;
    pthread_mutex_unlock(&text_mutex);
    pthread_mutex_lock(&import_mutex);
    import_looper = NULL;
    import_ready = false;
    pthread_mutex_unlock(&import_mutex);
    pthread_mutex_lock(&controller_mutex);
    controller_ready = false;
    controller_count = 0;
    pthread_mutex_unlock(&controller_mutex);
    if (p->env && p->cls)
        (*p->env)->DeleteGlobalRef(p->env, p->cls);
    if (p->attached)
        (*p->app->activity->vm)->DetachCurrentThread(p->app->activity->vm);
}
float cr_android_density(cr_android *p) {
    float n = (*p->env)->CallFloatMethod(p->env, p->app->activity->clazz, p->density);
    if (cr_android_clear_error(p) || n <= 0)
        n = 1;
    return n;
}
void cr_android_menu_metrics(cr_android *p, float values[4]) {
    static const float defaults[4] = {90, 1.2f, .93f, .556f};
    memcpy(values, defaults, sizeof(defaults));
    jfloatArray result = (*p->env)->CallObjectMethod(p->env, p->app->activity->clazz, p->menu_metrics);
    bool failed = cr_android_clear_error(p);
    if (!failed && result && (*p->env)->GetArrayLength(p->env, result) == 4) {
        float actual[4];
        (*p->env)->GetFloatArrayRegion(p->env, result, 0, 4, actual);
        if (!cr_android_clear_error(p))
            for (unsigned i = 0; i < 4; ++i)
                if (isfinite(actual[i]) && actual[i] > 0) values[i] = actual[i];
    }
    if (result) (*p->env)->DeleteLocalRef(p->env, result);
}
void cr_android_root(cr_android *p, char *out, size_t size) {
    jstring s = (*p->env)->CallObjectMethod(p->env, p->app->activity->clazz, p->root);
    string_copy(p->env, s, out, size);
    if (s)
        (*p->env)->DeleteLocalRef(p->env, s);
    cr_android_clear_error(p);
}
static int pref(void *ctx, const char *key, int fallback) {
    cr_android *p = ctx;
    jstring s = (*p->env)->NewStringUTF(p->env, key);
    int value =
        (*p->env)->CallIntMethod(p->env, p->app->activity->clazz, p->preference, s, fallback);
    (*p->env)->DeleteLocalRef(p->env, s);
    return cr_android_clear_error(p) ? fallback : value;
}
static void set_pref(void *ctx, const char *key, int value) {
    cr_android *p = ctx;
    jstring s = (*p->env)->NewStringUTF(p->env, key);
    (*p->env)->CallVoidMethod(p->env, p->app->activity->clazz, p->set_preference, s, value);
    (*p->env)->DeleteLocalRef(p->env, s);
    cr_android_clear_error(p);
}
static void pick(void *ctx, int system) {
    cr_android *p = ctx;
    (*p->env)->CallVoidMethod(p->env, p->app->activity->clazz, p->pick, system);
    cr_android_clear_error(p);
}
static void pick_folder(void *ctx, int system, const char *extensions) {
    cr_android *p = ctx;
    jstring allowed = (*p->env)->NewStringUTF(p->env, extensions ? extensions : "");
    if (!allowed) { cr_android_clear_error(p); return; }
    (*p->env)->CallVoidMethod(p->env, p->app->activity->clazz, p->pick_folder, system, allowed);
    (*p->env)->DeleteLocalRef(p->env, allowed);
    cr_android_clear_error(p);
}
static bool finish(void *ctx, bool accept, int system, const char *core_id,
                   const char *system_id, char *error, size_t cap) {
    cr_android *p = ctx;
    jstring core = (*p->env)->NewStringUTF(p->env, core_id ? core_id : "");
    jstring machine = (*p->env)->NewStringUTF(p->env, system_id ? system_id : "");
    if (!core || !machine || cr_android_clear_error(p)) {
        if (core) (*p->env)->DeleteLocalRef(p->env, core);
        if (machine) (*p->env)->DeleteLocalRef(p->env, machine);
        snprintf(error, cap, "Identité du cœur indisponible");
        return false;
    }
    jstring result = (*p->env)->CallObjectMethod(p->env, p->app->activity->clazz, p->finish,
                                                 (jboolean)accept, (jint)system, core, machine);
    (*p->env)->DeleteLocalRef(p->env, core);
    (*p->env)->DeleteLocalRef(p->env, machine);
    if (cr_android_clear_error(p)) {
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
    if (cr_android_clear_error(p) || !records)
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
    cr_android_clear_error(p);
    return result;
}
static void touch(void *ctx, const char *id) {
    cr_android *p = ctx;
    jstring s = (*p->env)->NewStringUTF(p->env, id);
    (*p->env)->CallVoidMethod(p->env, p->app->activity->clazz, p->touch, s);
    (*p->env)->DeleteLocalRef(p->env, s);
    cr_android_clear_error(p);
}
static char *credits(void *ctx) {
    cr_android *p = ctx;
    jstring s = (*p->env)->CallObjectMethod(p->env, p->app->activity->clazz, p->credits);
    if (cr_android_clear_error(p) || !s)
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
    cr_android_clear_error(p);
}
static void quit(void *ctx) {
    cr_android *p = ctx;
    (*p->env)->CallVoidMethod(p->env, p->app->activity->clazz, p->quit);
    cr_android_clear_error(p);
}
static void refresh_catalog(void *ctx) {
    cr_android *p = ctx;
    (*p->env)->CallVoidMethod(p->env, p->app->activity->clazz, p->refresh_catalog);
    cr_android_clear_error(p);
}
static void install_pack(void *ctx, const char *pack_id) {
    cr_android *p = ctx;
    jstring id = (*p->env)->NewStringUTF(p->env, pack_id);
    if (!id) { cr_android_clear_error(p); return; }
    (*p->env)->CallVoidMethod(p->env, p->app->activity->clazz, p->install_pack, id);
    (*p->env)->DeleteLocalRef(p->env, id);
    cr_android_clear_error(p);
}
static bool install_pending(void *ctx) {
    cr_android *p = ctx;
    jboolean pending = (*p->env)->CallBooleanMethod(p->env, p->app->activity->clazz,
                                                   p->install_pending);
    return cr_android_clear_error(p) || pending;
}
static void achievement_request(void *ctx, uint64_t id, const char *url, const char *body) {
    cr_android *p = ctx;
    if (!cr_achievement_request_track(id)) return;
    jstring address = (*p->env)->NewStringUTF(p->env, url);
    jstring post = (*p->env)->NewStringUTF(p->env, body ? body : "");
    if (address && post)
        (*p->env)->CallVoidMethod(p->env, p->app->activity->clazz, p->achievement_request, (jlong)id, address, post);
    if (cr_android_clear_error(p) || !address || !post)
        cr_achievement_response_push(id, -2, NULL, 0);
    if (address) (*p->env)->DeleteLocalRef(p->env, address);
    if (post) (*p->env)->DeleteLocalRef(p->env, post);
}
static void achievement_credentials(void *ctx, char *user, size_t user_size, char *token, size_t token_size) {
    cr_android *p = ctx;
    user[0] = token[0] = 0;
    jobjectArray values = (*p->env)->CallObjectMethod(p->env, p->app->activity->clazz, p->achievement_credentials);
    if (cr_android_clear_error(p) || !values) return;
    if ((*p->env)->GetArrayLength(p->env, values) == 2) {
        jstring username = (*p->env)->GetObjectArrayElement(p->env, values, 0);
        jstring key = (*p->env)->GetObjectArrayElement(p->env, values, 1);
        string_copy(p->env, username, user, user_size);
        string_copy(p->env, key, token, token_size);
        if (username) (*p->env)->DeleteLocalRef(p->env, username);
        if (key) (*p->env)->DeleteLocalRef(p->env, key);
    }
    (*p->env)->DeleteLocalRef(p->env, values);
}
static void save_achievement_credentials(void *ctx, const char *user, const char *token) {
    cr_android *p = ctx;
    jstring username = (*p->env)->NewStringUTF(p->env, user);
    jstring key = (*p->env)->NewStringUTF(p->env, token);
    if (username && key)
        (*p->env)->CallVoidMethod(p->env, p->app->activity->clazz, p->achievement_credentials_set, username, key);
    cr_android_clear_error(p);
    if (username) (*p->env)->DeleteLocalRef(p->env, username);
    if (key) (*p->env)->DeleteLocalRef(p->env, key);
}
static void edit_text(void *ctx, int field, const char *text, bool secret) {
    cr_android *p = ctx;
    jstring initial = (*p->env)->NewStringUTF(p->env, text);
    if (initial) {
        (*p->env)->CallVoidMethod(p->env, p->app->activity->clazz, p->edit_text, field, initial, (jboolean)secret);
        (*p->env)->DeleteLocalRef(p->env, initial);
    }
    cr_android_clear_error(p);
}
static bool save_transfer(void *context, uint64_t request, int operation, const char *title,
                          const uint8_t *bytes, size_t size) {
    cr_android *p = context;
    if (!request || request > INT64_MAX || size > 1024u * 1024u ||
        (operation != CR_SAVE_IMPORT && (!bytes || !size))) return false;
    jstring name = (*p->env)->NewStringUTF(p->env, title);
    jbyteArray data = operation != CR_SAVE_IMPORT ? (*p->env)->NewByteArray(p->env, (jsize)size) : NULL;
    if (!name || (operation != CR_SAVE_IMPORT && !data)) {
        if (name) (*p->env)->DeleteLocalRef(p->env, name);
        if (data) (*p->env)->DeleteLocalRef(p->env, data);
        cr_android_clear_error(p); return false;
    }
    if (data) (*p->env)->SetByteArrayRegion(p->env, data, 0, (jsize)size, (const jbyte *)bytes);
    bool ok = !cr_android_clear_error(p);
    if (ok) {
        pthread_mutex_lock(&save_mutex);
        save_request = request; save_ready = save_delivered = false;
        pthread_mutex_unlock(&save_mutex);
        (*p->env)->CallVoidMethod(p->env, p->app->activity->clazz, p->save_transfer,
                                 (jlong)request, (jint)operation, name, data);
        ok = !cr_android_clear_error(p);
    }
    (*p->env)->DeleteLocalRef(p->env, name);
    if (data) (*p->env)->DeleteLocalRef(p->env, data);
    return ok;
}
static void finish_save_transfer(void *context, uint64_t request) {
    cr_android *p = context;
    pthread_mutex_lock(&save_mutex);
    if (save_request == request) { save_request = 0; save_ready = save_delivered = false; }
    pthread_mutex_unlock(&save_mutex);
    (*p->env)->CallVoidMethod(p->env, p->app->activity->clazz, p->finish_save_transfer, (jlong)request);
    cr_android_clear_error(p);
}
static bool controller_info(void *context, unsigned port, char *name, size_t capacity) {
    cr_android *p = context;
    if (capacity) name[0] = 0;
    if (port >= CR_CONTROLLER_PORT_COUNT || !p->controller_connected[port]) return false;
    snprintf(name, capacity, "%s", p->controller_names[port]);
    return true;
}
cr_platform cr_android_services(cr_android *p) {
    return (cr_platform){p, pref, set_pref, pick, finish, games, touch, credits,
                         appearance, quit, refresh_catalog, install_pack, install_pending,
                         achievement_request, achievement_credentials, save_achievement_credentials, edit_text,
                         pick_folder, save_transfer, finish_save_transfer, controller_info};
}
