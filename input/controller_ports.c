// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
#include "controller_ports.h"
#include <stdio.h>
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
int cr_controller_binding_default(unsigned button) {
    static const int keys[10] = {19, 20, 21, 22, 96, 97, 108, 109, 102, 103};
    return button < 10 ? keys[button] : -1;
}
bool cr_controller_binding_valid(int key) {
    return (key >= 19 && key <= 22) || (key >= 96 && key <= 110) ||
        (key >= 188 && key <= 203) || key == 54 || key == 52 || key == 66 || key == 62;
}
void cr_controller_bindings_sanitize(int bindings[10]) {
    bool valid = true;
    for (unsigned i = 0; i < 10; ++i) {
        if (!cr_controller_binding_valid(bindings[i])) valid = false;
        for (unsigned j = 0; j < i; ++j) if (bindings[j] == bindings[i]) valid = false;
    }
    if (!valid)
        for (unsigned i = 0; i < 10; ++i) bindings[i] = cr_controller_binding_default(i);
}
int cr_controller_binding_button(const int bindings[10], int key) {
    if (!cr_controller_binding_valid(key)) return -1;
    for (unsigned i = 0; i < 10; ++i) if (bindings[i] == key) return (int)i;
    /* Retain the former keyboard-style aliases unless explicitly reassigned. */
    static const int aliases[4] = {54, 52, 66, 62};
    for (unsigned i = 0; i < 4; ++i)
        if (key == aliases[i] && bindings[i + 4] == cr_controller_binding_default(i + 4))
            return (int)i + 4;
    return -1;
}
bool cr_controller_bindings_assign(int bindings[10], unsigned button, int key) {
    if (button >= 10 || !cr_controller_binding_valid(key)) return false;
    for (unsigned i = 0; i < 10; ++i)
        if (i != button && bindings[i] == key) return false;
    bindings[button] = key;
    return true;
}
void cr_controller_binding_name(int key, char *name, size_t capacity) {
    const char *label = NULL;
    switch (key) {
    case 19: label = "Croix haut"; break;
    case 20: label = "Croix bas"; break;
    case 21: label = "Croix gauche"; break;
    case 22: label = "Croix droite"; break;
    case 96: label = "Bouton A"; break;
    case 97: label = "Bouton B"; break;
    case 98: label = "Bouton C"; break;
    case 99: label = "Bouton X"; break;
    case 100: label = "Bouton Y"; break;
    case 101: label = "Bouton Z"; break;
    case 102: label = "L1"; break;
    case 103: label = "R1"; break;
    case 104: label = "L2"; break;
    case 105: label = "R2"; break;
    case 106: label = "L3"; break;
    case 107: label = "R3"; break;
    case 108: label = "Start"; break;
    case 109: label = "Select"; break;
    case 110: label = "Mode"; break;
    case 54: label = "Touche Z"; break;
    case 52: label = "Touche X"; break;
    case 66: label = "Entrée"; break;
    case 62: label = "Espace"; break;
    default: break;
    }
    if (label) snprintf(name, capacity, "%s", label);
    else if (key >= 188 && key <= 203) snprintf(name, capacity, "Bouton %d", key - 187);
    else snprintf(name, capacity, "Non définie");
}
