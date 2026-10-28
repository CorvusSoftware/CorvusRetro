// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
#include "controller_ports.h"
#include <string.h>
void cr_controller_ports_init(cr_controller_ports *ports) {
    memset(ports, 0, sizeof(*ports));
    for (unsigned p = 0; p < CR_CONTROLLER_PORT_COUNT; ++p)
        ports->devices[p] = -1;
}
int cr_controller_port(const cr_controller_ports *ports, int device) {
    if (device < 0) return -1;
    for (unsigned p = 0; p < CR_CONTROLLER_PORT_COUNT; ++p)
        if (ports->devices[p] == device) return (int)p;
    return -1;
}
bool cr_controller_input(const cr_controller_ports *ports, int device, bool gamepad_source,
                         bool dpad_source) {
    if (gamepad_source) return true;
    if (!dpad_source || device < 0) return false;
    for (size_t i = 0; i < ports->connected_count; ++i)
        if (ports->connected[i] == device) return true;
    for (size_t i = 0; i < ports->retired_count; ++i)
        if (ports->retired[i] == device) return true;
    return false;
}
unsigned cr_controller_ports_sync(cr_controller_ports *ports, const cr_controller_device *devices, size_t count) {
    unsigned removed = 0;
    if (count > CR_CONTROLLER_DEVICE_CAP) count = CR_CONTROLLER_DEVICE_CAP;
    for (size_t i = 0; i < ports->connected_count; ++i) {
        bool found = false;
        for (size_t j = 0; j < count; ++j)
            if (devices[j].id == ports->connected[i]) found = true;
        if (!found) {
            ports->retired[ports->retired_next] = ports->connected[i];
            ports->retired_next = (ports->retired_next + 1) % CR_CONTROLLER_DEVICE_CAP;
            if (ports->retired_count < CR_CONTROLLER_DEVICE_CAP) ++ports->retired_count;
        }
    }
    ports->connected_count = count;
    for (size_t i = 0; i < count; ++i) ports->connected[i] = devices[i].id;
    for (unsigned p = 0; p < CR_CONTROLLER_PORT_COUNT; ++p) {
        if (ports->devices[p] < 0) continue;
        bool found = false;
        for (size_t i = 0; i < count; ++i)
            if (devices[i].id == ports->devices[p] &&
                devices[i].generation == ports->generations[p]) found = true;
        if (!found) {
            removed |= 1u << p;
            ports->devices[p] = -1;
            ports->buttons[p] = ports->axes[p] = 0;
        }
    }
    for (size_t i = 0; i < count; ++i) {
        if (devices[i].id < 0 || cr_controller_port(ports, devices[i].id) >= 0) continue;
        for (unsigned p = 0; p < CR_CONTROLLER_PORT_COUNT; ++p) {
            if (ports->devices[p] >= 0) continue;
            ports->devices[p] = devices[i].id;
            ports->generations[p] = devices[i].generation;
            break;
        }
    }
    return removed;
}
unsigned cr_controller_mask(const cr_controller_ports *ports, unsigned port) {
    return port < CR_CONTROLLER_PORT_COUNT ? ports->buttons[port] | ports->axes[port] : 0;
}
void cr_controller_button(cr_controller_ports *ports, unsigned port, unsigned mask, bool down) {
    if (port >= CR_CONTROLLER_PORT_COUNT) return;
    if (down) ports->buttons[port] |= mask;
    else ports->buttons[port] &= ~mask;
}
void cr_controller_axes(cr_controller_ports *ports, unsigned port, unsigned mask) {
    if (port < CR_CONTROLLER_PORT_COUNT) ports->axes[port] = mask & 15u;
}
void cr_controller_ports_clear(cr_controller_ports *ports) {
    memset(ports->buttons, 0, sizeof(ports->buttons));
    memset(ports->axes, 0, sizeof(ports->axes));
}
