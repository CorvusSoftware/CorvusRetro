// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
package com.corvussoftware.corvusretro;

import java.io.IOException;
import org.json.JSONArray;
import org.json.JSONObject;
import org.junit.Test;
import static org.junit.Assert.*;

/** Exercise the production parser, including the official-provider origin gate. */
public final class CoreCatalogTest {
    private JSONObject core(String system, String abi, String repository) throws Exception {
        String name = "Corvus" + system.toUpperCase(java.util.Locale.ROOT);
        return new JSONObject().put("id", "corvus." + system).put("name", name)
                .put("description", "Console").put("version", "0.15.0")
                .put("abi", abi).put("api_version", 0).put("library_name", "libcorvus_" + system + ".so")
                .put("size", 1930063).put("sha256", "257477d8c89c72f79f5909cb78d58db66b3b784fdc767f5f4b6459ed97f427ca")
                .put("archive_url", "https://github.com/CorvusSoftware/" + repository
                        + "/releases/download/v0.15.1/" + name + "-0.15.0-android-" + abi + ".zip");
    }

    private String feed(JSONArray entries) throws Exception {
        return new JSONObject().put("format", 3).put("cores", entries).toString();
    }

    @Test public void officialCentralCatalogListsThreeCoresForEachArchitecture() throws Exception {
        JSONArray entries = new JSONArray();
        for (String abi : new String[] {"arm64-v8a", "x86_64"})
            for (String system : new String[] {"gb", "gbc", "gba"})
                entries.put(core(system, abi, "CorvusRepository"));
        for (String abi : new String[] {"arm64-v8a", "x86_64"}) {
            CoreRepository.Catalog catalog = CoreRepository.parseCatalog(feed(entries), abi);
            assertEquals(3, catalog.cores.size());
            assertEquals(15, catalog.records.length);
            for (String system : new String[] {"gb", "gbc", "gba"})
                assertEquals(abi, catalog.cores.get("corvus." + system).abi);
        }
    }

    @Test public void legacyOfficialReleaseRemainsAccepted() throws Exception {
        assertEquals(1, CoreRepository.parseCatalog(feed(new JSONArray().put(
                core("gb", "x86_64", "CorvusRetro"))), "x86_64").cores.size());
    }

    @Test public void foreignOriginsStillFail() throws Exception {
        JSONObject entry = core("gb", "x86_64", "CorvusRepository");
        for (String url : new String[] {
                entry.getString("archive_url").replace("CorvusSoftware/", "OtherOwner/"),
                entry.getString("archive_url").replace("github.com/", "github.com.evil.example/"),
                entry.getString("archive_url") + "?redirect=evil",
                entry.getString("archive_url").replace("CorvusRepository/", "OtherRepository/")}) {
            entry.put("archive_url", url);
            try {
                CoreRepository.parseCatalog(feed(new JSONArray().put(entry)), "x86_64");
                fail("Foreign origin accepted");
            } catch (IOException expected) { }
        }
    }

    @Test public void versionedDiscoveryKeepsItsOwnPathBoundary() throws Exception {
        JSONObject item = new JSONObject().put("type", "file").put("name", "corvuscore.json")
                .put("path", "catalog/v1/corvuscore.json").put("size", 2791)
                .put("download_url", CoreCatalogDiscovery.FALLBACK);
        assertEquals(CoreCatalogDiscovery.FALLBACK,
                CoreCatalogDiscovery.sources(new JSONArray().put(item).toString()).get(0));
        item.put("path", "catalog/corvuscore.json");
        try {
            CoreCatalogDiscovery.sources(new JSONArray().put(item).toString());
            fail("Legacy directory accepted as the versioned feed");
        } catch (IOException expected) { }
    }
}
