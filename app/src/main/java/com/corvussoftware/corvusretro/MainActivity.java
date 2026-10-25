// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
package com.corvussoftware.corvusretro;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Intent;
import android.os.Bundle;
import android.view.Gravity;
import android.view.View;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.Switch;
import android.widget.TextView;
import android.widget.Toast;
import java.io.ByteArrayOutputStream;
import java.io.InputStream;
import java.nio.charset.StandardCharsets;
import java.util.List;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

/** Original Android menus following MaterialUI's list/tab navigation model. */
public final class MainActivity extends Activity {
    private static final int OPEN_CONTENT = 10;
    private GameLibrary library;
    private final ExecutorService io = Executors.newSingleThreadExecutor();
    private LinearLayout body;
    private LinearLayout navigation;
    private TextView heading;
    private TextView footer;
    private ImageButton back;
    private int page;
    private int filter = -1;
    private boolean history;
    private boolean importing;
    private boolean pickerOpen;
    private boolean coreBusy;
    private final Runnable coreChanged = () -> { if (body != null) render(); };
    private boolean resumed;
    private String pendingGame;
    private String pendingError;
    private int dp(int value) { return Ui.dp(this, value); }
    private int selectedCore() {
        int loaded = CoreManager.loaded();
        return loaded >= 0 ? loaded : CoreCatalog.valid(library.preferences.getInt("selected_core", 0));
    }

    @Override public void onCreate(Bundle state) {
        Ui.applyTheme(this);
        super.onCreate(state);
        library = new GameLibrary(this);
        if (state != null) {
            page = state.getInt("page", 0); filter = state.getInt("filter", -1);
            history = state.getBoolean("history", false); pickerOpen = state.getBoolean("picker", false);
            pendingGame = state.getString("pending_game"); pendingError = state.getString("pending_error");
        }
        LinearLayout root = Ui.column(this);
        root.setBackgroundColor(Ui.BG);
        LinearLayout toolbar = Ui.row(this);
        toolbar.setPadding(dp(8), dp(4), dp(16), dp(4));
        toolbar.setBackgroundColor(Ui.HEADER);
        back = new ImageButton(this);
        back.setImageResource(R.drawable.ic_back);
        back.setImageTintList(android.content.res.ColorStateList.valueOf(Ui.TEXT));
        back.setBackground(Ui.ripple(this, Ui.SURFACE, 0));
        back.setContentDescription(getString(R.string.back_to_menu));
        back.setOnClickListener(view -> navigateBack());
        toolbar.addView(back, new LinearLayout.LayoutParams(dp(48), dp(48)));
        heading = Ui.text(this, R.string.main_menu, 19, Ui.TEXT, true);
        toolbar.addView(heading, new LinearLayout.LayoutParams(0, -2, 1));
        ImageView brand = Ui.icon(this, R.drawable.ic_corvus, Ui.ACCENT);
        toolbar.addView(brand, new LinearLayout.LayoutParams(dp(28), dp(28)));
        root.addView(toolbar);
        ScrollView scroll = new ScrollView(this);
        scroll.setFillViewport(true);
        body = Ui.column(this);
        scroll.addView(body);
        root.addView(scroll, new LinearLayout.LayoutParams(-1, 0, 1));
        footer = Ui.text(this, R.string.app_name, 11, Ui.MUTED, false);
        footer.setPadding(dp(20), dp(8), dp(20), dp(8));
        root.addView(footer);
        View divider = new View(this); divider.setBackgroundColor(Ui.BORDER);
        root.addView(divider, new LinearLayout.LayoutParams(-1, dp(1)));
        navigation = Ui.row(this); navigation.setBackgroundColor(Ui.NAV);
        root.addView(navigation);
        Ui.insets(this, root);
        render();
    }
    @Override protected void onStart() { super.onStart(); CoreManager.observe(coreChanged); }
    @Override protected void onStop() { CoreManager.forget(coreChanged); super.onStop(); }
    @Override protected void onResume() {
        super.onResume(); resumed = true;
        if (body != null) render();
        if (pendingGame != null) {
            GameLibrary.Game game = library.find(pendingGame); pendingGame = null;
            if (game != null) play(game);
        }
        if (pendingError != null) { String message = pendingError; pendingError = null; showImportError(message); }
    }
    @Override protected void onPause() { resumed = false; super.onPause(); }
    @Override protected void onSaveInstanceState(Bundle state) {
        state.putInt("page", page); state.putInt("filter", filter);
        state.putBoolean("history", history); state.putBoolean("picker", pickerOpen);
        state.putString("pending_game", pendingGame); state.putString("pending_error", pendingError);
        super.onSaveInstanceState(state);
    }
    @Override protected void onDestroy() { io.shutdown(); super.onDestroy(); }
    private void navigateBack() {
        if (history) page = 0;
        history = false; filter = -1; render();
    }
    @Override public void onBackPressed() {
        if (page == 1 && (history || filter >= 0)) navigateBack();
        else if (page != 0) { page = 0; render(); }
        else super.onBackPressed();
    }
    private void render() {
        int loaded = CoreManager.loaded();
        body.removeAllViews(); navigation.removeAllViews();
        back.setVisibility(page == 1 && (history || filter >= 0) ? View.VISIBLE : View.GONE);
        heading.setPadding(back.getVisibility() == View.GONE ? dp(12) : 0, 0, 0, 0);
        heading.setText(page == 0 ? getString(R.string.main_menu) : page == 2 ? getString(R.string.settings)
                : history ? getString(R.string.history) : filter >= 0 ? CoreCatalog.SYSTEMS[filter] : getString(R.string.playlists));
        footer.setText(loaded < 0 ? getString(R.string.no_core)
                : getString(R.string.core_loaded, CoreCatalog.NAMES[loaded], CoreCatalog.VERSION));
        Ui.space(body, 8);
        if (importing || coreBusy) {
            TextView loading = Ui.text(this, importing ? R.string.importing : R.string.core_working, 14, Ui.ACCENT, false);
            loading.setPadding(dp(20), dp(12), dp(20), dp(12)); body.addView(loading);
        }
        if (page == 0) mainMenu(loaded); else if (page == 1) playlists(); else settings();
        nav(R.string.main_menu, R.drawable.ic_corvus, 0);
        nav(R.string.playlists, R.drawable.ic_library, 1);
        nav(R.string.settings, R.drawable.ic_settings, 2);
    }
    private void mainMenu(int loaded) {
        View choose = row(getString(R.string.load_core), loaded < 0 ? getString(R.string.no_core)
                : CoreCatalog.label(this, loaded), R.drawable.ic_core, this::chooseCore);
        enable(choose, !coreBusy && !importing && !pickerOpen);
        if (loaded >= 0) {
            View unload = row(getString(R.string.unload_core), getString(R.string.unload_core_hint), R.drawable.ic_unload, this::unloadCore);
            enable(unload, !coreBusy && !importing && !pickerOpen);
        }
        View load = row(getString(R.string.load_content), getString(R.string.load_content_hint), R.drawable.ic_content, this::pickContent);
        enable(load, !coreBusy && !importing && !pickerOpen);
        row(getString(R.string.history), getString(R.string.history_hint), R.drawable.ic_history, () -> {
            page = 1; history = true; filter = -1; render();
        });
        row(getString(R.string.information), getString(R.string.information_hint), R.drawable.ic_info, this::showAbout);
    }
    private void enable(View view, boolean value) { view.setEnabled(value); view.setAlpha(value ? 1 : .5f); }
    private void chooseCore() {
        if (coreBusy || importing || pickerOpen) return;
        String[] labels = new String[3];
        for (int i = 0; i < 3; i++) labels[i] = CoreCatalog.label(this, i);
        new AlertDialog.Builder(this).setTitle(R.string.load_core)
                .setSingleChoiceItems(labels, CoreManager.loaded(), (dialog, which) -> {
                    dialog.dismiss(); coreBusy = true; render();
                    CoreManager.load(which, error -> {
                        if (isFinishing() || isDestroyed()) return;
                        coreBusy = false;
                        if (error == null) library.preferences.edit().putInt("selected_core", which).apply();
                        else showImportError(error);
                        render();
                    });
                }).setNegativeButton(R.string.close, null).show();
    }
    private void unloadCore() {
        if (coreBusy || importing || pickerOpen) return;
        coreBusy = true; render();
        CoreManager.unload(error -> {
            if (isFinishing() || isDestroyed()) return;
            coreBusy = false;
            if (error != null) showImportError(error);
            render();
        });
    }
    private void pickContent() {
        if (coreBusy || importing || pickerOpen) return;
        Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT).setType("*/*").addCategory(Intent.CATEGORY_OPENABLE);
        try { pickerOpen = true; startActivityForResult(intent, OPEN_CONTENT); }
        catch (android.content.ActivityNotFoundException error) {
            pickerOpen = false;
            Toast.makeText(this, R.string.import_error, Toast.LENGTH_LONG).show();
        }
    }
    @Override protected void onActivityResult(int request, int result, Intent data) {
        super.onActivityResult(request, result, data);
        if (request != OPEN_CONTENT) return;
        pickerOpen = false;
        if (result != RESULT_OK || data == null || data.getData() == null) { render(); return; }
        android.net.Uri uri = data.getData();
        int core = selectedCore();
        importing = true; render();
        io.execute(() -> {
            try {
                GameLibrary.Game game = library.importContent(uri, core);
                runOnUiThread(() -> {
                    if (isFinishing() || isDestroyed()) return;
                    importing = false;
                    library.preferences.edit().putInt("selected_core", game.system).apply();
                    render();
                    if (resumed) play(game); else pendingGame = game.id;
                });
            } catch (Exception | OutOfMemoryError error) {
                runOnUiThread(() -> {
                    if (isFinishing() || isDestroyed()) return;
                    importing = false; render();
                    if (resumed) showImportError(error.getMessage()); else pendingError = error.getMessage();
                });
            }
        });
    }
    private void play(GameLibrary.Game game) {
        startActivity(new Intent(this, PlayerActivity.class).putExtra("game", game.id));
    }
    private void showImportError(String message) {
        new AlertDialog.Builder(this).setTitle(R.string.import_error).setMessage(message)
                .setPositiveButton(R.string.close, null).show();
    }
    private void playlists() {
        List<GameLibrary.Game> games = library.games();
        if (!history && filter < 0) {
            for (int i = 0; i < 3; i++) {
                int system = i; int count = 0;
                for (GameLibrary.Game game : games) if (game.system == system) count++;
                row(CoreCatalog.SYSTEMS[i], getString(R.string.content_count, count), R.drawable.ic_core, () -> { filter = system; render(); });
            }
            return;
        }
        boolean empty = true;
        for (GameLibrary.Game game : games) {
            if (history ? game.played == 0 : game.system != filter) continue;
            empty = false;
            row(game.title, CoreCatalog.NAMES[game.system], R.drawable.ic_file, () -> play(game));
        }
        if (empty) {
            TextView message = Ui.text(this, history ? R.string.empty_history : R.string.empty_playlist, 16, Ui.TEXT, false);
            message.setPadding(dp(20), dp(24), dp(20), dp(12)); body.addView(message);
            TextView hint = Ui.text(this, R.string.add_content_hint, 14, Ui.MUTED, false);
            hint.setPadding(dp(20), 0, dp(20), dp(20)); body.addView(hint);
        }
    }
    private void settings() {
        View theme = row(getString(R.string.theme), getString(Ui.isShield(this) ? R.string.theme_shield : R.string.theme_dark),
                R.drawable.ic_theme, this::chooseTheme);
        enable(theme, !importing && !coreBusy);
        toggle(R.string.audio_setting, R.string.audio_setting_hint, "audio");
        toggle(R.string.keep_awake, R.string.keep_awake_hint, "keep_awake");
        row(getString(R.string.about), getString(R.string.about_caption), R.drawable.ic_info, this::showAbout);
        row(getString(R.string.license), getString(R.string.license_caption), R.drawable.ic_file, this::showLicense);
    }
    private void chooseTheme() {
        if (importing || coreBusy) return;
        String[] labels = {getString(R.string.theme_shield), getString(R.string.theme_dark)};
        new AlertDialog.Builder(this).setTitle(R.string.theme)
                .setSingleChoiceItems(labels, Ui.isShield(this) ? 0 : 1, (dialog, which) -> {
                    library.preferences.edit().putString("theme", which == 0 ? "shield" : "dark").apply();
                    dialog.dismiss(); recreate();
                }).setNegativeButton(R.string.close, null).show();
    }
    private void toggle(int title, int subtitle, String key) {
        LinearLayout item = baseRow();
        LinearLayout text = Ui.column(this);
        text.addView(Ui.text(this, title, 16, Ui.TEXT, false)); Ui.space(text, 6);
        text.addView(Ui.text(this, subtitle, 13, Ui.MUTED, false));
        item.addView(text, new LinearLayout.LayoutParams(0, -2, 1));
        Switch toggle = new Switch(this);
        toggle.setContentDescription(getString(title)); toggle.setMinHeight(dp(48));
        toggle.setChecked(library.preferences.getBoolean(key, true));
        toggle.setOnCheckedChangeListener((button, checked) -> library.preferences.edit().putBoolean(key, checked).apply());
        item.addView(toggle); body.addView(item); divider();
    }
    private LinearLayout baseRow() {
        LinearLayout item = Ui.row(this);
        item.setPadding(dp(20), dp(16), dp(20), dp(16)); item.setMinimumHeight(dp(76));
        return item;
    }
    private View row(String title, String subtitle, int icon, Runnable action) {
        LinearLayout item = baseRow(); item.setBackground(Ui.listBackground(this));
        ImageView image = Ui.icon(this, icon, Ui.ICON);
        LinearLayout.LayoutParams imageParams = new LinearLayout.LayoutParams(dp(32), dp(32)); imageParams.rightMargin = dp(14);
        item.addView(image, imageParams);
        LinearLayout copy = Ui.column(this);
        TextView label = Ui.text(this, R.string.app_name, 16, Ui.TEXT, false); label.setText(title); copy.addView(label);
        Ui.space(copy, 6);
        TextView hint = Ui.text(this, R.string.app_name, 13, Ui.MUTED, false); hint.setText(subtitle); copy.addView(hint);
        copy.setImportantForAccessibility(View.IMPORTANT_FOR_ACCESSIBILITY_NO_HIDE_DESCENDANTS);
        item.addView(copy, new LinearLayout.LayoutParams(0, -2, 1));
        ImageView arrow = Ui.icon(this, R.drawable.ic_next, Ui.MUTED);
        item.addView(arrow, new LinearLayout.LayoutParams(dp(18), dp(18)));
        item.setContentDescription(title + ". " + subtitle); item.setFocusable(true);
        item.setOnClickListener(view -> action.run()); body.addView(item); divider();
        return item;
    }
    private void divider() {
        View line = new View(this); line.setBackgroundColor(Ui.BORDER);
        LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(-1, dp(1)); params.leftMargin = dp(66);
        body.addView(line, params);
    }
    private void nav(int title, int icon, int destination) {
        boolean selected = page == destination;
        LinearLayout item = Ui.column(this); item.setGravity(Gravity.CENTER);
        item.setPadding(dp(4), dp(12), dp(4), dp(12)); item.setBackground(Ui.ripple(this, Ui.NAV, 0));
        item.addView(Ui.icon(this, icon, selected ? Ui.ACCENT : Ui.PASSIVE), new LinearLayout.LayoutParams(dp(22), dp(22)));
        Ui.space(item, 6);
        TextView label = Ui.text(this, title, 10, selected ? Ui.ACCENT : Ui.PASSIVE, true);
        label.setGravity(Gravity.CENTER); label.setImportantForAccessibility(View.IMPORTANT_FOR_ACCESSIBILITY_NO);
        item.addView(label); item.setSelected(selected); item.setFocusable(true);
        item.setContentDescription(selected ? getString(R.string.selected_tab, getString(title)) : getString(title));
        item.setOnClickListener(view -> { page = destination; filter = -1; history = false; render(); ((ScrollView)body.getParent()).scrollTo(0, 0); });
        item.setMinimumHeight(dp(64));
        navigation.addView(item, new LinearLayout.LayoutParams(0, -1, 1));
    }
    private void showAbout() {
        new AlertDialog.Builder(this).setTitle(R.string.app_name).setMessage(R.string.about_body).setPositiveButton(R.string.close, null).show();
    }
    private String asset(String name) throws java.io.IOException {
        try (InputStream stream = getAssets().open(name)) {
            ByteArrayOutputStream bytes = new ByteArrayOutputStream(); byte[] buffer = new byte[4096]; int count;
            while ((count = stream.read(buffer)) != -1) bytes.write(buffer, 0, count);
            return bytes.toString(StandardCharsets.UTF_8.name());
        }
    }
    private void showLicense() {
        String license;
        try { license = asset("LICENSE.txt") + "\n\n" + asset("CORVUSCORE-NOTICE.txt") + "\n\n" + asset("CORVUSCORE-COPYRIGHT.txt"); }
        catch (java.io.IOException error) { license = getString(R.string.license_unavailable); }
        new AlertDialog.Builder(this).setTitle(R.string.license).setMessage(getString(R.string.license_notice, license))
                .setPositiveButton(R.string.close, null).show();
    }
}
