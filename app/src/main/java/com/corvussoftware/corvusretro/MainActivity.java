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
import android.widget.Button;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;
import java.io.ByteArrayOutputStream;
import java.io.InputStream;
import java.nio.charset.StandardCharsets;

public final class MainActivity extends Activity {
    private int page;
    private LinearLayout body;
    private LinearLayout navigation;

    @Override public void onCreate(Bundle state) {
        super.onCreate(state);
        page = state == null ? 0 : state.getInt("page", 0);
        LinearLayout root = Ui.column(this);
        root.setBackgroundColor(Ui.BG);
        ScrollView scroll = new ScrollView(this);
        scroll.setFillViewport(true);
        scroll.setClipToPadding(false);
        body = Ui.column(this);
        body.setPadding(dp(24), dp(24), dp(24), dp(24));
        scroll.addView(body);
        root.addView(scroll, new LinearLayout.LayoutParams(-1, 0, 1));
        View divider = new View(this);
        divider.setBackgroundColor(Ui.BORDER);
        root.addView(divider, new LinearLayout.LayoutParams(-1, dp(1)));
        navigation = Ui.row(this);
        navigation.setPadding(dp(12), dp(8), dp(12), dp(8));
        root.addView(navigation);
        showPage(page);
        Ui.insets(this, root);
    }
    private int dp(int value) { return Ui.dp(this, value); }
    @Override protected void onSaveInstanceState(Bundle state) {
        state.putInt("page", page);
        super.onSaveInstanceState(state);
    }
    private void showPage(int selected) {
        page = selected;
        body.removeAllViews();
        navigation.removeAllViews();
        LinearLayout brand = Ui.row(this);
        ImageView logo = Ui.icon(this, R.drawable.ic_corvus, Ui.ACCENT);
        logo.setPadding(dp(8), dp(8), dp(8), dp(8));
        logo.setBackground(Ui.shape(this, 0xff263224, 12, false));
        brand.addView(logo, new LinearLayout.LayoutParams(dp(40), dp(40)));
        TextView name = Ui.text(this, R.string.app_name, 20, Ui.TEXT, true);
        LinearLayout.LayoutParams nameParams = new LinearLayout.LayoutParams(-2, -2);
        nameParams.leftMargin = dp(12);
        brand.addView(name, nameParams);
        body.addView(brand);
        Ui.space(body, 30);
        body.addView(Ui.text(this, page == 2 ? R.string.settings : R.string.library, 30, Ui.TEXT, true));
        Ui.space(body, 8);
        body.addView(Ui.text(this, page == 2 ? R.string.settings_subtitle : R.string.library_subtitle, 14, Ui.MUTED, false));
        Ui.space(body, 24);
        if (page == 2) settings(); else library();
        navItem(R.string.library, R.drawable.ic_library, 0);
        navItem(R.string.lab, R.drawable.ic_play, 1);
        navItem(R.string.settings, R.drawable.ic_settings, 2);
    }
    private void library() {
        LinearLayout hero = Ui.card(this);
        hero.setBackground(Ui.shape(this, 0xff1c2a20, 24, false));
        TextView badge = Ui.badge(this, R.string.original);
        hero.addView(badge, new LinearLayout.LayoutParams(-2, -2));
        LinearLayout feature = Ui.row(this);
        LinearLayout copy = Ui.column(this);
        copy.addView(Ui.text(this, R.string.hero_title, 26, Ui.TEXT, true));
        Ui.space(copy, 12);
        copy.addView(Ui.text(this, R.string.hero_caption, 13, 0xffadbcaa, false));
        feature.addView(copy, new LinearLayout.LayoutParams(0, -2, 1));
        feature.addView(new PocketArtView(this), new LinearLayout.LayoutParams(dp(116), dp(160)));
        hero.addView(feature);
        Ui.space(hero, 12);
        Button launch = Ui.button(this, R.string.launch_demo, true);
        launch.setOnClickListener(view -> launchDemo());
        hero.addView(launch, new LinearLayout.LayoutParams(-1, -2));
        body.addView(hero);
        Ui.space(body, 28);
        LinearLayout heading = Ui.row(this);
        heading.addView(Ui.text(this, R.string.collection, 18, Ui.TEXT, true), new LinearLayout.LayoutParams(0, -2, 1));
        heading.addView(Ui.text(this, R.string.game_count, 12, Ui.MUTED, false));
        body.addView(heading);
        Ui.space(body, 14);
        LinearLayout empty = Ui.card(this);
        empty.setGravity(Gravity.CENTER_HORIZONTAL);
        ImageView collection = Ui.icon(this, R.drawable.ic_library, Ui.MUTED);
        empty.addView(collection, new LinearLayout.LayoutParams(dp(28), dp(28)));
        Ui.space(empty, 14);
        empty.addView(Ui.text(this, R.string.empty_title, 18, Ui.TEXT, true));
        Ui.space(empty, 8);
        TextView caption = Ui.text(this, R.string.empty_caption, 13, Ui.MUTED, false);
        caption.setGravity(Gravity.CENTER);
        empty.addView(caption);
        body.addView(empty);
        Ui.space(body, 12);
        TextView note = Ui.text(this, R.string.availability, 12, Ui.MUTED, false);
        note.setGravity(Gravity.CENTER);
        body.addView(note);
        Ui.space(body, 24);
        TextView local = Ui.text(this, R.string.local_caption, 10, Ui.MUTED, true);
        local.setLetterSpacing(.1f);
        local.setGravity(Gravity.CENTER);
        body.addView(local);
    }
    private void settings() {
        body.addView(Ui.text(this, R.string.appearance, 11, Ui.MUTED, true));
        Ui.space(body, 12);
        LinearLayout appearance = Ui.row(this);
        appearance.setPadding(dp(20), dp(20), dp(20), dp(20));
        appearance.setBackground(Ui.shape(this, Ui.SURFACE, 20, true));
        appearance.addView(Ui.text(this, R.string.theme, 16, Ui.TEXT, true), new LinearLayout.LayoutParams(0, -2, 1));
        appearance.addView(Ui.badge(this, R.string.dark_theme));
        body.addView(appearance);
        Ui.space(body, 28);
        body.addView(Ui.text(this, R.string.about_section, 11, Ui.MUTED, true));
        Ui.space(body, 12);
        settingsRow(R.string.about, R.string.about_caption, () -> new AlertDialog.Builder(this)
                .setTitle(R.string.app_name).setMessage(R.string.about_body)
                .setPositiveButton(R.string.close, null).show());
        Ui.space(body, 12);
        settingsRow(R.string.license, R.string.license_caption, this::showLicense);
        Ui.space(body, 30);
        TextView version = Ui.text(this, R.string.version, 10, Ui.MUTED, true);
        version.setLetterSpacing(.08f);
        body.addView(version);
    }
    private void settingsRow(int title, int subtitle, Runnable action) {
        LinearLayout card = Ui.card(this);
        card.setBackground(Ui.ripple(this, Ui.SURFACE, 20));
        card.addView(Ui.text(this, title, 16, Ui.TEXT, true));
        Ui.space(card, 7);
        card.addView(Ui.text(this, subtitle, 13, Ui.MUTED, false));
        card.setContentDescription(getString(title) + ". " + getString(subtitle));
        card.setFocusable(true);
        card.setOnClickListener(view -> action.run());
        body.addView(card);
    }
    private void navItem(int title, int icon, int destination) {
        boolean selected = page == destination;
        LinearLayout item = Ui.column(this);
        item.setGravity(Gravity.CENTER);
        item.setPadding(dp(4), dp(8), dp(4), dp(8));
        item.setBackground(Ui.ripple(this, selected ? 0xff202c22 : Ui.BG, 16));
        item.addView(Ui.icon(this, icon, selected ? Ui.ACCENT : Ui.MUTED), new LinearLayout.LayoutParams(dp(22), dp(22)));
        Ui.space(item, 6);
        TextView label = Ui.text(this, title, 11, selected ? Ui.ACCENT : Ui.MUTED, true);
        label.setImportantForAccessibility(View.IMPORTANT_FOR_ACCESSIBILITY_NO);
        item.addView(label);
        item.setContentDescription(selected ? getString(R.string.selected_tab, getString(title)) : getString(title));
        item.setSelected(selected);
        item.setFocusable(true);
        item.setOnClickListener(view -> {
            if (destination == 1) launchDemo();
            else {
                showPage(destination);
                ((ScrollView)body.getParent()).scrollTo(0, 0);
            }
        });
        LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(0, -2, 1);
        params.setMargins(dp(3), 0, dp(3), 0);
        navigation.addView(item, params);
    }
    private void launchDemo() { startActivity(new Intent(this, GameActivity.class)); }
    private void showLicense() {
        String license;
        try (InputStream stream = getAssets().open("LICENSE.txt")) {
            ByteArrayOutputStream bytes = new ByteArrayOutputStream();
            byte[] buffer = new byte[4096];
            int count;
            while ((count = stream.read(buffer)) != -1) bytes.write(buffer, 0, count);
            license = bytes.toString(StandardCharsets.UTF_8.name());
        } catch (java.io.IOException error) {
            license = getString(R.string.license_unavailable);
        }
        new AlertDialog.Builder(this).setTitle(R.string.license)
                .setMessage(getString(R.string.license_notice, license))
                .setPositiveButton(R.string.close, null).show();
    }
}
