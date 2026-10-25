// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
#include <jni.h>
#include "corvus_core.h"

/* Called exclusively on Android's UI thread. No background native resources. */
static cr_session session;
static uint32_t pixels[CR_WIDTH * CR_HEIGHT];

JNIEXPORT void JNICALL
Java_com_corvussoftware_corvusretro_NativeBridge_reset(JNIEnv *env, jclass type)
{
    (void)env; (void)type;
    cr_reset(&session);
}

JNIEXPORT void JNICALL
Java_com_corvussoftware_corvusretro_NativeBridge_frame(
    JNIEnv *env, jclass type, jintArray output, jint input, jboolean paused)
{
    (void)type;
    if (!output || (*env)->GetArrayLength(env, output) < CR_WIDTH * CR_HEIGHT) {
        jclass error = (*env)->FindClass(env, "java/lang/IllegalArgumentException");
        if (error) (*env)->ThrowNew(env, error, "Frame buffer must contain 49152 pixels");
        return;
    }
    session.paused = paused == JNI_TRUE;
    cr_step(&session, (unsigned)input);
    cr_render(&session, pixels, CR_WIDTH * CR_HEIGHT);
    (*env)->SetIntArrayRegion(env, output, 0, CR_WIDTH * CR_HEIGHT, (const jint *)pixels);
}
