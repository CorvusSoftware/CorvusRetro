# CorvusRetro - CorvusSoftware
# Copyright (C) 2026 CorvusSoftware
# SPDX-License-Identifier: AGPL-3.0-only
LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := corvusretro
LOCAL_SRC_FILES := bridge.c ../../../../core/src/corvus_core.c
LOCAL_C_INCLUDES := $(LOCAL_PATH)/../../../../core/include
LOCAL_CFLAGS := -std=c11 -Wall -Wextra -Werror
LOCAL_LDFLAGS := -Wl,-z,max-page-size=16384
include $(BUILD_SHARED_LIBRARY)
