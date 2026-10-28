// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
#include "controller_ports.h"
#undef NDEBUG
#include <assert.h>
#include <stdio.h>

int main(void) {
    cr_controller_ports ports;
    cr_controller_ports_init(&ports);
    cr_controller_device devices[] = {{17, 1}, {42, 2}, {99, 3}};
    assert(cr_controller_ports_sync(&ports, devices, 3) == 0);
    assert(cr_controller_port(&ports, 17) == 0);
    assert(cr_controller_port(&ports, 42) == 1);
    assert(cr_controller_port(&ports, 99) == -1);
    assert(cr_controller_port(&ports, -1) == -1);
    // An unassigned third controller's D-pad must never become selected-player keyboard input.
    assert(cr_controller_input(&ports, 99, false, true));
    assert(!cr_controller_input(&ports, 7, false, true));
    assert(!cr_controller_input(&ports, 99, false, false));
    assert(cr_controller_input(&ports, 100, true, false));
    cr_controller_button(&ports, 0, 1u << 4, true);
    cr_controller_button(&ports, 1, 1u << 5, true);
    cr_controller_device reordered[] = {{99, 3}, {42, 2}, {17, 1}};
    assert(cr_controller_ports_sync(&ports, reordered, 3) == 0);
    assert(cr_controller_port(&ports, 17) == 0);
    assert(cr_controller_port(&ports, 42) == 1);
    assert(cr_controller_mask(&ports, 0) == 1u << 4);
    assert(cr_controller_mask(&ports, 1) == 1u << 5);

    // Disconnecting player one must preserve player two's held buttons and port.
    cr_controller_device remaining[] = {{42, 2}, {99, 3}};
    assert(cr_controller_ports_sync(&ports, remaining, 2) == 1u);
    assert(cr_controller_port(&ports, 99) == 0);
    assert(cr_controller_port(&ports, 42) == 1);
    assert(cr_controller_mask(&ports, 0) == 0);
    assert(cr_controller_mask(&ports, 1) == 1u << 5);

    // Controller identity survives removal for already-queued D-pad events.
    assert(cr_controller_port(&ports, 17) == -1);
    assert(cr_controller_input(&ports, 17, false, true));
    assert(!cr_controller_input(&ports, 7, false, true));

    // Coalesced remove/add snapshots still release the reconnected device only.
    remaining[0].generation = 4;
    assert(cr_controller_ports_sync(&ports, remaining, 2) == 2u);
    assert(cr_controller_mask(&ports, 1) == 0);
    assert(cr_controller_port(&ports, 99) == 0);
    assert(cr_controller_port(&ports, 42) == 1);

    // A D-pad key and an axis can hold the same direction independently.
    cr_controller_button(&ports, 0, 1u, true);
    cr_controller_axes(&ports, 0, 1u | 4u);
    cr_controller_button(&ports, 0, 1u, false);
    assert(cr_controller_mask(&ports, 0) == (1u | 4u));
    cr_controller_button(&ports, 0, 4u, true);
    cr_controller_axes(&ports, 0, 0);
    assert(cr_controller_mask(&ports, 0) == 4u);

    cr_controller_ports_clear(&ports);
    assert(cr_controller_mask(&ports, 0) == 0);
    assert(cr_controller_mask(&ports, 1) == 0);
    assert(cr_controller_port(&ports, 99) == 0);
    assert(cr_controller_port(&ports, 42) == 1);
    assert(cr_controller_ports_sync(&ports, NULL, 0) == 3u);
    assert(cr_controller_port(&ports, 99) == -1);
    assert(cr_controller_port(&ports, 42) == -1);
    cr_controller_button(&ports, 2, ~0u, true);
    cr_controller_axes(&ports, 2, ~0u);
    assert(cr_controller_mask(&ports, 2) == 0);
    puts("controller ports: stable routing, disconnect isolation and axis/button overlap passed");
    return 0;
}
