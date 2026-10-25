// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
package com.corvussoftware.corvusretro;

import android.content.Context;

final class CoreCatalog {
    static final String VERSION = "0.14.0";
    static final String[] NAMES = {"CorvusGB", "CorvusGBC", "CorvusGBA"};
    static final String[] SYSTEMS = {"Game Boy", "Game Boy Color", "Game Boy Advance"};
    private CoreCatalog() {}
    static int valid(int system) { return system >= 0 && system < 3 ? system : 0; }
    static String label(Context context, int system) {
        return NAMES[system] + " : " + SYSTEMS[system] + (system == 2 ? " · " + context.getString(R.string.experimental) : "");
    }
}
