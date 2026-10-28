// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
#ifndef CORVUS_CONTROLLER_PORTS_H
#define CORVUS_CONTROLLER_PORTS_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#define CR_CONTROLLER_PORT_COUNT 2
#define CR_CONTROLLER_DEVICE_CAP 64
typedef struct {
    int id;
    uint64_t generation;
} cr_controller_device;
typedef struct {
    int devices[CR_CONTROLLER_PORT_COUNT];
    uint64_t generations[CR_CONTROLLER_PORT_COUNT];
    unsigned buttons[CR_CONTROLLER_PORT_COUNT], axes[CR_CONTROLLER_PORT_COUNT];
    int connected[CR_CONTROLLER_DEVICE_CAP], retired[CR_CONTROLLER_DEVICE_CAP];
    size_t connected_count, retired_count, retired_next;
} cr_controller_ports;
void cr_controller_ports_init(cr_controller_ports *);
/* Returns the ports whose previous device disappeared, including reused ports. */
unsigned cr_controller_ports_sync(cr_controller_ports *, const cr_controller_device *, size_t);
int cr_controller_port(const cr_controller_ports *, int device);
/* D-pad events retain controller identity even without a player port. */
bool cr_controller_input(const cr_controller_ports *, int device, bool gamepad_source,
                         bool dpad_source);
unsigned cr_controller_mask(const cr_controller_ports *, unsigned port);
void cr_controller_button(cr_controller_ports *, unsigned port, unsigned mask, bool down);
void cr_controller_axes(cr_controller_ports *, unsigned port, unsigned mask);
/* Lifecycle/menu transitions release state without changing device assignments. */
void cr_controller_ports_clear(cr_controller_ports *);
#endif
