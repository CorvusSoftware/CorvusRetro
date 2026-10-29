// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
package com.corvussoftware.corvusretro;

import java.io.IOException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.TreeSet;
import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

/** Discover reviewed provider feeds from the GitHub catalog directory. */
final class CoreCatalogDiscovery {
    static final String DIRECTORY =
            "https://api.github.com/repos/CorvusSoftware/CorvusRepository/contents/catalog/v1?ref=main";
    static final String RAW =
            "https://raw.githubusercontent.com/CorvusSoftware/CorvusRepository/main/catalog/v1/";
    static final String FALLBACK = RAW + "corvuscore.json";
    static final int MAX_CATALOGS = 16;
    static final int MAX_TOTAL_CORES = 32;

    private CoreCatalogDiscovery() { }

    static List<String> sources(String payload) throws JSONException, IOException {
        JSONArray files = new JSONArray(payload);
        if (files.length() == 0 || files.length() > 128)
            throw new IOException("Liste des catalogues invalide");
        TreeSet<String> names = new TreeSet<>();
        for (int i = 0; i < files.length(); i++) {
            JSONObject item = files.getJSONObject(i);
            if (!"file".equals(item.optString("type"))) continue;
            String name = item.getString("name");
            if (!name.endsWith(".json")) continue;
            if (!name.matches("[a-z][a-z0-9_-]{0,39}\\.json") ||
                    !item.getString("path").equals("catalog/v1/" + name))
                throw new IOException("Fichier de catalogue non fiable");
            Object size = item.get("size");
            if (!(size instanceof Integer || size instanceof Long) ||
                    ((Number) size).longValue() <= 0 ||
                    ((Number) size).longValue() > 96 * 1024)
                throw new IOException("Taille de catalogue non fiable");
            if (!RAW.concat(name).equals(item.getString("download_url")) ||
                    !names.add(name) || names.size() > MAX_CATALOGS)
                throw new IOException("URL ou doublon de catalogue non fiable");
        }
        if (!names.contains("corvuscore.json"))
            throw new IOException("Catalogue CorvusCore absent");
        ArrayList<String> sources = new ArrayList<>();
        for (String name : names) sources.add(RAW + name);
        return sources;
    }

    static final class Collector {
        private final Map<String, CoreRepository.Core> entries = new LinkedHashMap<>();
        private final ArrayList<String> records = new ArrayList<>();

        void add(CoreRepository.Catalog catalog) throws IOException {
            if (entries.size() + catalog.cores.size() > MAX_TOTAL_CORES)
                throw new IOException("Trop de cœurs");
            for (String key : catalog.cores.keySet())
                if (entries.containsKey(key)) throw new IOException("Identifiant de cœur dupliqué");
            entries.putAll(catalog.cores);
            records.addAll(Arrays.asList(catalog.records));
        }

        CoreRepository.Catalog result() throws IOException {
            if (entries.isEmpty()) throw new IOException("Aucun cœur compatible disponible");
            return new CoreRepository.Catalog(new LinkedHashMap<>(entries),
                    records.toArray(new String[0]), "");
        }
    }
}
