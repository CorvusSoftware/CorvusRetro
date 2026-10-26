// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
#define _POSIX_C_SOURCE 200809L
#include "menu_driver.h"
#include "core_registry.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void check(bool ok, const char *description) {
    if (!ok) {
        fprintf(stderr, "FAIL %s\n", description);
        exit(1);
    }
}

static int reloads;
static void refresh(void *ctx) { (void)ctx; reloads++; }

int main(void) {
    check(cr_registry_count() == 0, "core-free native registry is empty");
    check(cr_registry_get(0) == NULL && cr_registry_find("corvus.gb") == -1,
          "official engines are not installed by default");

    char dir[] = "/tmp/corvus-no-core-XXXXXX";
    check(mkdtemp(dir) != NULL, "temporary state directory");
    cr_ui ui;
    cr_platform platform = {.refresh_core_catalog = refresh};
    cr_ui_init(&ui, platform, dir);
    cr_ui_size(&ui, 384, 700);
    check(ui.page == CR_MAIN && !ui.engine.handle && !ui.engine.loaded,
          "base APK frontend starts without an active core");

    cr_ui_confirm(&ui);
    check(ui.page == CR_CORES, "load core opens the core screen");
    cr_ui_confirm(&ui);
    check(ui.page == CR_CORE_DOWNLOADS && reloads == 1,
          "core downloader remains available with no installed engines");
    cr_core_catalog empty = {0};
    cr_ui_catalog(&ui, &empty);
    check(!ui.catalog_loading && ui.core_catalog.count == 0,
          "an empty signed-release catalog is a supported state");
    cr_ui_back(&ui);
    cr_ui_back(&ui);
    check(ui.page == CR_MAIN, "back navigation works without installed cores");

    cr_ui_destroy(&ui);
    puts("PASS core-free CorvusRetro 0.13");
    return 0;
}
