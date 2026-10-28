// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
#include "android_backend.h"
#include "gl2.h"
#include "aaudio.h"
#include "menu_driver.h"
#include "task_achievements.h"
#include <android/keycodes.h>
#include <android/log.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
typedef struct {
    cr_android platform;
    cr_gl gl;
    cr_ui ui;
    cr_audio audio;
    cr_controller_ports controllers;
    bool controllers_running;
    bool window, resumed, focused, was_running, audio_warning, applied_awake;
    int applied_theme;
    double deadline, battery_time, last_draw;
} application;
static double now(void) {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec + t.tv_nsec / 1e9;
}
static void save(application *a) {
    if (!cr_engine_save_battery(&a->ui.engine))
        cr_ui_message(&a->ui, a->ui.engine.error);
}
static void controller_devices(application *a) {
    cr_controller_device devices[CR_CONTROLLER_DEVICE_CAP];
    size_t count;
    if (cr_android_controller_devices(devices, &count)) {
        unsigned removed = cr_controller_ports_sync(&a->controllers, devices, count);
        for (unsigned p = 0; p < CR_CONTROLLER_PORT_COUNT; ++p)
            if (removed & (1u << p)) cr_ui_release_player(&a->ui, p);
    }
}
static void controller_mode(application *a) {
    bool running = cr_ui_running(&a->ui);
    if (running != a->controllers_running) {
        cr_controller_ports_clear(&a->controllers);
        for (unsigned p = 0; p < CR_CONTROLLER_PORT_COUNT; ++p)
            cr_ui_release_player(&a->ui, p);
        a->controllers_running = running;
    }
}
static void controller_apply(application *a, unsigned port, unsigned previous) {
    unsigned current = cr_controller_mask(&a->controllers, port);
    cr_ui_player_key(&a->ui, port, previous & ~current, false);
    cr_ui_player_key(&a->ui, port, current & ~previous, true);
}
static bool controller_source(int source) {
    return (source & AINPUT_SOURCE_GAMEPAD) == AINPUT_SOURCE_GAMEPAD ||
           (source & AINPUT_SOURCE_JOYSTICK) == AINPUT_SOURCE_JOYSTICK;
}
static void command(struct android_app *app, int32_t cmd) {
    application *a = app->userData;
    switch (cmd) {
    case APP_CMD_INIT_WINDOW:
        if (app->window) {
            if (a->window)
                cr_gl_destroy(&a->gl);
            a->window = cr_gl_init(&a->gl, &a->platform);
            a->last_draw = now();
            if (!a->window) {
                cr_gl_destroy(&a->gl);
                __android_log_write(ANDROID_LOG_ERROR, "CorvusRetro",
                                    "Initialisation EGL impossible");
                ANativeActivity_finish(app->activity);
            }
        }
        break;
    case APP_CMD_TERM_WINDOW:
        if (a->window)
            cr_gl_destroy(&a->gl);
        a->window = false;
        cr_audio_pause(&a->audio);
        cr_ui_clear_input(&a->ui);
        cr_controller_ports_clear(&a->controllers);
        save(a);
        break;
    case APP_CMD_RESUME:
        if (a->ui.platform.core_install_pending) {
            bool pending = a->ui.platform.core_install_pending(a->ui.platform.context);
            if (pending && !a->ui.installing) {
                save(a);
                cr_ui_clear_input(&a->ui);
                cr_controller_ports_clear(&a->controllers);
            }
            a->ui.installing = pending;
        }
        a->last_draw = now();
        a->resumed = true;
        a->deadline = now();
        break;
    case APP_CMD_PAUSE:
        a->resumed = false;
        cr_audio_pause(&a->audio);
        cr_ui_clear_input(&a->ui);
        cr_controller_ports_clear(&a->controllers);
        save(a);
        break;
    case APP_CMD_GAINED_FOCUS:
        a->focused = true;
        a->deadline = now();
        break;
    case APP_CMD_LOST_FOCUS:
        a->focused = false;
        cr_audio_pause(&a->audio);
        cr_ui_clear_input(&a->ui);
        cr_controller_ports_clear(&a->controllers);
        save(a);
        break;
    case APP_CMD_CONFIG_CHANGED:
        cr_ui_clear_input(&a->ui);
        cr_controller_ports_clear(&a->controllers);
        break;
    default:
        break;
    }
}
static int button(int key) {
    switch (key) {
    case AKEYCODE_DPAD_UP:
        return 0;
    case AKEYCODE_DPAD_DOWN:
        return 1;
    case AKEYCODE_DPAD_LEFT:
        return 2;
    case AKEYCODE_DPAD_RIGHT:
        return 3;
    case AKEYCODE_BUTTON_A:
    case AKEYCODE_Z:
        return 4;
    case AKEYCODE_BUTTON_B:
    case AKEYCODE_X:
        return 5;
    case AKEYCODE_BUTTON_START:
    case AKEYCODE_ENTER:
        return 6;
    case AKEYCODE_BUTTON_SELECT:
    case AKEYCODE_SPACE:
        return 7;
    case AKEYCODE_BUTTON_L1:
        return 8;
    case AKEYCODE_BUTTON_R1:
        return 9;
    default:
        return -1;
    }
}
static int32_t input(struct android_app *app, AInputEvent *event) {
    application *a = app->userData;
    cr_ui *u = &a->ui;
    controller_devices(a);
    controller_mode(a);
    int source = AInputEvent_getSource(event);
    int port = cr_controller_port(&a->controllers, AInputEvent_getDeviceId(event));
    bool controller = cr_controller_input(&a->controllers, AInputEvent_getDeviceId(event),
                                          controller_source(source),
                                          (source & AINPUT_SOURCE_DPAD) == AINPUT_SOURCE_DPAD);
    if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_KEY) {
        if (a->ui.ra_editing) return 0;
        int key = AKeyEvent_getKeyCode(event), action = AKeyEvent_getAction(event);
        bool down = action == AKEY_EVENT_ACTION_DOWN;
        if (key == AKEYCODE_BACK) {
            if (!down)
                cr_ui_back(u);
            return 1;
        }
        if (cr_ui_running(u)) {
            int b = button(key);
            if (b >= 0) {
                if (controller) {
                    if (port >= 0) {
                        unsigned previous = cr_controller_mask(&a->controllers, (unsigned)port);
                        cr_controller_button(&a->controllers, (unsigned)port, 1u << b, down);
                        controller_apply(a, (unsigned)port, previous);
                    }
                } else cr_ui_key(u, 1u << b, down);
                return 1;
            }
        } else if (down) {
            if (key == AKEYCODE_DPAD_UP) {
                cr_ui_move(u, -1);
                return 1;
            }
            if (key == AKEYCODE_DPAD_DOWN) {
                cr_ui_move(u, 1);
                return 1;
            }
            if (key == AKEYCODE_DPAD_CENTER || key == AKEYCODE_ENTER || key == AKEYCODE_BUTTON_A) {
                if (AKeyEvent_getRepeatCount(event) == 0)
                    cr_ui_confirm(u);
                return 1;
            }
            if (key == AKEYCODE_BUTTON_B) {
                cr_ui_back(u);
                return 1;
            }
        }
        return button(key) >= 0 ? 1 : 0;
    }
    if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
        if (controller) {
            if (port < 0) return 1;
            unsigned previous = cr_controller_mask(&a->controllers, (unsigned)port);
            unsigned directions = 0;
            if ((AMotionEvent_getAction(event) & AMOTION_EVENT_ACTION_MASK) != AMOTION_EVENT_ACTION_CANCEL &&
                AMotionEvent_getPointerCount(event)) {
                float x = AMotionEvent_getAxisValue(event, AMOTION_EVENT_AXIS_X, 0);
                float y = AMotionEvent_getAxisValue(event, AMOTION_EVENT_AXIS_Y, 0);
                float hat_x = AMotionEvent_getAxisValue(event, AMOTION_EVENT_AXIS_HAT_X, 0);
                float hat_y = AMotionEvent_getAxisValue(event, AMOTION_EVENT_AXIS_HAT_Y, 0);
                if (y < -.5f || hat_y < -.5f) directions |= 1u << 0;
                if (y > .5f || hat_y > .5f) directions |= 1u << 1;
                if (x < -.5f || hat_x < -.5f) directions |= 1u << 2;
                if (x > .5f || hat_x > .5f) directions |= 1u << 3;
            }
            cr_controller_axes(&a->controllers, (unsigned)port, directions);
            if (cr_ui_running(u)) controller_apply(a, (unsigned)port, previous);
            else if (!u->ra_editing) {
                unsigned pressed = directions & ~previous;
                if (pressed & (1u << 0)) cr_ui_move(u, -1);
                if (pressed & (1u << 1)) cr_ui_move(u, 1);
            }
            return 1;
        }
        if (!(source & AINPUT_SOURCE_CLASS_POINTER)) return 0;
        int action = AMotionEvent_getAction(event), kind = action & AMOTION_EVENT_ACTION_MASK,
            index = (action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >>
                    AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
        if (kind == AMOTION_EVENT_ACTION_CANCEL) {
            cr_ui_clear_input(u);
            cr_controller_ports_clear(&a->controllers);
            return 1;
        }
        size_t count = AMotionEvent_getPointerCount(event);
        if (!a->window)
            return 0;
        for (size_t i = 0; i < count; i++) {
            int phase;
            if (kind == AMOTION_EVENT_ACTION_MOVE)
                phase = CR_TOUCH_MOVE;
            else if ((int)i == index && (kind == AMOTION_EVENT_ACTION_DOWN ||
                                         kind == AMOTION_EVENT_ACTION_POINTER_DOWN))
                phase = CR_TOUCH_DOWN;
            else if ((int)i == index &&
                     (kind == AMOTION_EVENT_ACTION_UP || kind == AMOTION_EVENT_ACTION_POINTER_UP))
                phase = CR_TOUCH_UP;
            else
                continue;
            float x = (AMotionEvent_getX(event, i) - a->gl.left) / a->gl.density,
                  y = (AMotionEvent_getY(event, i) - a->gl.top) / a->gl.density;
            cr_ui_touch_at(u, AMotionEvent_getPointerId(event, i), phase, x, y,
                           AMotionEvent_getEventTime(event) / 1000000000.0);
        }
        return 1;
    }
    return 0;
}
void android_main(struct android_app *app) {
    application *a = calloc(1, sizeof(*a));
    if (!a) {
        ANativeActivity_finish(app->activity);
        return;
    }
    app->userData = a;
    cr_controller_ports_init(&a->controllers);
    if (!cr_android_init(&a->platform, app)) {
        __android_log_write(ANDROID_LOG_ERROR, "CorvusRetro", "Services Android indisponibles");
        cr_android_destroy(&a->platform);
        ANativeActivity_finish(app->activity);
        free(a);
        return;
    }
    char root[4096];
    cr_android_root(&a->platform, root, sizeof(root));
    cr_ui_init(&a->ui, cr_android_services(&a->platform), root);
    cr_audio_init(&a->audio);
    app->onAppCmd = command;
    app->onInputEvent = input;
    a->last_draw = now();
    a->applied_theme = -1;
#ifndef NDEBUG
    unsigned last_players = ~0u, last_controlled = ~0u;
    int last_page = -1, last_core = -2, last_content = -1, last_quick = -1, last_theme = -1,
        last_active = -1, last_show_input = -1, last_installing = -1;
#endif
    while (!app->destroyRequested) {
        bool active = a->window && a->resumed && a->focused;
        double time = now();
        bool running = active && cr_ui_running(&a->ui);
        int wait = active ? 0 : -1;
        if (active && !running)
            wait = 16;
        else if (running && a->deadline > time)
            wait = (int)fmin(16, (a->deadline - time) * 1000);
        struct android_poll_source *source = NULL;
        int events;
        int ident = ALooper_pollOnce(wait, NULL, &events, (void **)&source);
        if (ident >= 0 && source)
            source->process(app, source);
        if (app->destroyRequested)
            break;
        controller_devices(a);
        controller_mode(a);
        cr_achievement_response response;
        while (cr_achievement_response_pop(&response)) {
            cr_cheevos_response(a->ui.achievements, response.id, response.status, response.body, response.size);
            cr_achievement_response_dispose(&response);
        }
        int text_field;
        char input_text[513];
        bool accepted;
        if (cr_android_text_result(&text_field, input_text, sizeof(input_text), &accepted)) {
            cr_ui_text_result(&a->ui, text_field, input_text, accepted);
            memset(input_text, 0, sizeof(input_text));
        }
        cr_import item;
        if (cr_android_import(&item))
            cr_ui_import(&a->ui, &item);
        cr_core_catalog core_catalog;
        if (cr_android_core_catalog(&core_catalog))
            cr_ui_catalog(&a->ui, &core_catalog);
        if (cr_android_update_modules())
            cr_ui_reload(&a->ui);
        char install_status[256];
        bool install_finished;
        if (cr_android_core_status(install_status, sizeof(install_status), &install_finished))
            cr_ui_install_status(&a->ui, install_status, install_finished);
        active = a->window && a->resumed && a->focused;
        running = active && cr_ui_running(&a->ui);
        time = now();
        bool awake = running && a->ui.awake;
        if (a->applied_theme != a->ui.theme || a->applied_awake != awake) {
            cr_platform services = a->ui.platform;
            if (services.appearance)
                services.appearance(services.context, (int)cr_theme_get(a->ui.theme)->system_bar,
                                    awake);
            a->applied_theme = a->ui.theme;
            a->applied_awake = awake;
        }
#ifndef NDEBUG
        if (last_page != a->ui.page || last_core != a->ui.engine.system ||
            last_content != a->ui.engine.loaded || last_quick != a->ui.quick ||
            last_theme != a->ui.theme || last_active != active || last_show_input != a->ui.show_input ||
            last_installing != a->ui.installing || last_players != a->ui.engine.players ||
            last_controlled != a->ui.input_player) {
            __android_log_print(ANDROID_LOG_DEBUG, "CorvusRetro",
                                "frontend page=%d core=%d content=%d quick=%d theme=%d active=%d show_input=%d installing=%d players=%u controlled=%u",
                                a->ui.page, a->ui.engine.system, a->ui.engine.loaded, a->ui.quick,
                                a->ui.theme, active, a->ui.show_input, a->ui.installing, a->ui.engine.players, a->ui.input_player);
            last_page = a->ui.page;
            last_core = a->ui.engine.system;
            last_content = a->ui.engine.loaded;
            last_quick = a->ui.quick;
            last_theme = a->ui.theme;
            last_active = active;
            last_show_input = a->ui.show_input;
            last_installing = a->ui.installing;
            last_players = a->ui.engine.players;
            last_controlled = a->ui.input_player;
        }
#endif
        if (running != a->was_running) {
            if (!running) {
                cr_audio_pause(&a->audio);
                save(a);
            }
            a->deadline = time;
            a->was_running = running;
        }
        if (running && time >= a->deadline) {
            unsigned masks[2];
            cr_ui_inputs(&a->ui, masks);
            if (cr_engine_step_players(&a->ui.engine, masks)) {
                cr_ui_achievement_frame(&a->ui);
                if (a->ui.audio) {
                    if (cr_audio_start(&a->audio, a->ui.engine.rate)) {
                        cr_audio_write(&a->audio, a->ui.engine.audio, a->ui.engine.audio_count);
                        a->audio_warning = false;
                    } else if (!a->audio_warning) {
                        cr_ui_message(&a->ui,
                                      "Sortie audio indisponible. Le jeu continue sans son.");
                        a->audio_warning = true;
                    }
                } else
                    cr_audio_pause(&a->audio);
            } else {
                cr_ui_message(&a->ui, a->ui.engine.error);
                a->ui.quick = true;
                cr_ui_clear_input(&a->ui);
                cr_controller_ports_clear(&a->controllers);
                cr_audio_pause(&a->audio);
            }
            a->deadline += 1.0 / a->ui.engine.fps;
            if (time - a->deadline > 2.0 / a->ui.engine.fps)
                a->deadline = time;
            if (time - a->battery_time > 5) {
                save(a);
                a->battery_time = time;
            }
        }
        if (a->window && a->resumed) {
            cr_gl_begin(&a->gl, cr_theme_get(a->ui.theme)->system_bar);
            if (fabsf(a->ui.width - a->gl.width) > .1f || fabsf(a->ui.height - a->gl.height) > .1f)
                cr_ui_size(&a->ui, a->gl.width, a->gl.height);
            cr_ui_update(&a->ui, time - a->last_draw);
            cr_painter painter = cr_gl_painter(&a->gl);
            cr_ui_draw(&a->ui, &painter);
            if (!cr_gl_end(&a->gl)) {
                cr_gl_destroy(&a->gl);
                a->window = false;
                cr_audio_pause(&a->audio);
            }
            a->last_draw = time;
        }
    }
    cr_audio_destroy(&a->audio);
    cr_ui_destroy(&a->ui);
    if (a->window)
        cr_gl_destroy(&a->gl);
    cr_android_destroy(&a->platform);
    app->userData = NULL;
    free(a);
}
