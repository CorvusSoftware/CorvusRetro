// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
package com.corvussoftware.corvusretro;

import android.app.NativeActivity;
import android.content.Intent;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Insets;
import android.graphics.Paint;
import android.graphics.Typeface;
import android.graphics.drawable.Drawable;
import android.hardware.input.InputManager;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.util.SparseLongArray;
import android.view.DisplayCutout;
import android.view.InputDevice;
import android.view.View;
import android.view.WindowInsets;
import android.view.WindowManager;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.nio.charset.StandardCharsets;
import java.util.List;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

/** Platform services only. Menus, layout, input and emulation run in native C. */
public final class MainActivity extends NativeActivity {
    static { System.loadLibrary("corvusretro"); }
    private static final int OPEN_CONTENT = 10;
    private static final int[] EXTRA_GLYPHS = {0x153, 0x152, 0x2190, 0x2191, 0x2192, 0x2193, 0x2026, 0x2018, 0x2019};
    private static final int[] ICONS = {R.drawable.ic_core, R.drawable.ic_corvus, R.drawable.ic_content,
            R.drawable.ic_library, R.drawable.ic_settings, R.drawable.ic_history, R.drawable.ic_info,
            R.drawable.ic_unload, R.drawable.ic_theme, R.drawable.ic_menu, R.drawable.ic_next,
            R.drawable.ic_back, R.drawable.ic_file, R.drawable.ic_home};
    private final ExecutorService io = Executors.newSingleThreadExecutor();
    private final Object importLock = new Object();
    private GameLibrary library;
    private CoreRepository coreRepository;
    private RetroAchievementsService achievements;
    private RetroAchievementsImages achievementImages;
    private NativeTextInput textInput;
    private GameLibrary.Pending pending;
    private volatile boolean destroyed;
    private boolean pickerOpen;
    private boolean copying;
    private int pickerSystem;
    // Immutable snapshot published by the UI thread, read by the native render thread.
    private volatile int[] windowArea;
    private InputManager inputManager;
    private final SparseLongArray controllerGenerations = new SparseLongArray();
    private long controllerGeneration;
    private final InputManager.InputDeviceListener controllerListener = new InputManager.InputDeviceListener() {
        @Override public void onInputDeviceAdded(int deviceId) {
            controllerGenerations.delete(deviceId);
            publishControllerDevices();
        }
        @Override public void onInputDeviceRemoved(int deviceId) {
            controllerGenerations.delete(deviceId);
            publishControllerDevices();
        }
        @Override public void onInputDeviceChanged(int deviceId) {
            controllerGenerations.delete(deviceId);
            publishControllerDevices();
        }
    };

    private static native void imported(String id, String title, int system, String path, String error);
    static native void achievementResponse(long id, int status, byte[] body);
    static native void achievementImageReady();
    static native void textInputResult(int field, String text, boolean accepted);
    static native void coreCatalogUpdated(String[] packages, String error);
    static native void coreModulesUpdated(String[] paths);
    static native void coreInstallStatus(String message, boolean finished);
    private static native void controllerDevicesChanged(int[] deviceIds, long[] generations);

    private void publishControllerDevices() {
        if (destroyed || inputManager == null) return;
        // Bound the service snapshot; devices outside this bound remain unassigned.
        int[] controllers = new int[64];
        long[] generations = new long[64];
        int count = 0;
        for (int id : inputManager.getInputDeviceIds()) {
            InputDevice device = inputManager.getInputDevice(id);
            if (device == null || (!device.supportsSource(InputDevice.SOURCE_GAMEPAD)
                    && !device.supportsSource(InputDevice.SOURCE_JOYSTICK))) continue;
            if (count == controllers.length) break;
            long generation = controllerGenerations.get(id);
            if (generation == 0) {
                generation = ++controllerGeneration;
                controllerGenerations.put(id, generation);
            }
            controllers[count] = id;
            generations[count++] = generation;
        }
        controllerDevicesChanged(Arrays.copyOf(controllers, count), Arrays.copyOf(generations, count));
    }

    @Override public void onCreate(Bundle state) {
        // NativeActivity starts android_main in super.onCreate. Services must already exist.
        library = new GameLibrary(this);
        coreRepository = new CoreRepository(this, io);
        achievements = new RetroAchievementsService(this);
        achievementImages = new RetroAchievementsImages(this, MainActivity::achievementImageReady);
        textInput = new NativeTextInput(this);
        inputManager = getSystemService(InputManager.class);
        if (inputManager != null) {
            inputManager.registerInputDeviceListener(controllerListener, new Handler(Looper.getMainLooper()));
            publishControllerDevices();
        }
        if (state != null) {
            pickerOpen = state.getBoolean("picker", false);
            pickerSystem = state.getInt("picker_system", 0);
        }
        super.onCreate(state);
        View decor = getWindow().getDecorView();
        // Use the same full-window coordinate space on Android 8 through Android 15+.
        // The C renderer reserves the system-provided safe area inside that surface.
        decor.setSystemUiVisibility(decor.getSystemUiVisibility()
                | View.SYSTEM_UI_FLAG_LAYOUT_STABLE | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION);
        if (Build.VERSION.SDK_INT >= 28) {
            WindowManager.LayoutParams attributes = getWindow().getAttributes();
            attributes.layoutInDisplayCutoutMode = WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_SHORT_EDGES;
            getWindow().setAttributes(attributes);
        }
        decor.setOnApplyWindowInsetsListener((view, insets) -> {
            publishWindowArea(view, insets);
            return insets;
        });
        decor.requestApplyInsets();
    }
    @Override public void onGlobalLayout() {
        super.onGlobalLayout();
        View decor = getWindow().getDecorView();
        WindowInsets insets = decor.getRootWindowInsets();
        if (insets != null) publishWindowArea(decor, insets);
    }
    private void publishWindowArea(View decor, WindowInsets insets) {
        int left, top, right, bottom;
        if (Build.VERSION.SDK_INT >= 30) {
            Insets safe = insets.getInsetsIgnoringVisibility(WindowInsets.Type.systemBars() | WindowInsets.Type.displayCutout());
            left = safe.left; top = safe.top; right = safe.right; bottom = safe.bottom;
        } else {
            left = insets.getStableInsetLeft(); top = insets.getStableInsetTop();
            right = insets.getStableInsetRight(); bottom = insets.getStableInsetBottom();
            DisplayCutout cutout = Build.VERSION.SDK_INT >= 28 ? insets.getDisplayCutout() : null;
            if (Build.VERSION.SDK_INT >= 28 && cutout != null) {
                left = Math.max(left, cutout.getSafeInsetLeft()); top = Math.max(top, cutout.getSafeInsetTop());
                right = Math.max(right, cutout.getSafeInsetRight()); bottom = Math.max(bottom, cutout.getSafeInsetBottom());
            }
        }
        int width = decor.getWidth(), height = decor.getHeight();
        if (width > left + right && height > top + bottom)
            windowArea = new int[] {width, height, left, top, right, bottom};
    }
    public int[] safeWindowArea() { return windowArea; }
    @Override protected void onSaveInstanceState(Bundle state) {
        state.putBoolean("picker", pickerOpen); state.putInt("picker_system", pickerSystem);
        super.onSaveInstanceState(state);
    }
    @Override protected void onResume() {
        super.onResume();
        achievements.resumeAwards();
    }
    @Override protected void onDestroy() {
        if (inputManager != null) inputManager.unregisterInputDeviceListener(controllerListener);
        synchronized (importLock) {
            destroyed = true;
            if (pending != null) { pending.discard(); pending = null; }
        }
        textInput.cancel();
        achievements.close();
        achievementImages.close();
        io.shutdown();
        super.onDestroy();
    }
    boolean isAlive() { return !destroyed; }
    public void refreshCoreCatalog() { coreRepository.refresh(); }
    public void installCorePack(String packId) { coreRepository.install(packId); }
    public boolean coreInstallPending() { return CoreRepository.downloading(); }
    public String[] installedCoreModules() {
        ArrayList<String> modules = new ArrayList<>(Arrays.asList(coreRepository.installedModules()));
        try {
            PackageInfo installed = getPackageManager().getPackageInfo(getPackageName(), 0);
            ApplicationInfo app = installed.applicationInfo;
            if (app == null || app.nativeLibraryDir == null || installed.splitNames == null)
                return modules.toArray(new String[0]);
            List<String> splits = Arrays.asList(installed.splitNames);
            File directory = new File(app.nativeLibraryDir).getCanonicalFile();
            String[] ids = {"gb", "gbc", "gba"};
            for (String id : ids) {
                if (!splits.contains("core" + id)) continue;
                boolean downloaded = false;
                for (String path : modules)
                    if (new File(path).getName().startsWith("libcorvus_" + id + "-")) downloaded = true;
                if (downloaded) continue;
                File library = new File(directory, "libcorvus_" + id + ".so").getCanonicalFile();
                if (directory.equals(library.getParentFile()) && library.isFile() && !library.canWrite())
                    modules.add(library.getAbsolutePath());
            }
        } catch (IOException | PackageManager.NameNotFoundException error) {
            android.util.Log.w("CorvusRetro", "Installed core discovery failed", error);
        }
        return modules.toArray(new String[0]);
    }
    public Bitmap achievementImage(String url) { return achievementImages.get(url); }
    public void achievementRequest(long id, String url, String post) { achievements.request(id, url, post); }
    public String[] achievementCredentials() { return achievements.credentials(); }
    public void achievementCredentialsSet(String user, String token) { achievements.saveCredentials(user, token); }
    public void textInput(int field, String initial, boolean secret) { textInput.open(field, initial, secret); }
    public float density() { return getResources().getDisplayMetrics().density; }
    public String filesRoot() { return getFilesDir().getAbsolutePath(); }
    public String[] games() {
        List<GameLibrary.Game> games = library.games();
        String[] records = new String[games.size() * 6];
        for (int i = 0; i < games.size(); i++) {
            GameLibrary.Game game = games.get(i);
            records[i * 6] = game.id;
            records[i * 6 + 1] = game.title;
            records[i * 6 + 2] = Integer.toString(game.system);
            records[i * 6 + 3] = Long.toString(game.played);
            records[i * 6 + 4] = game.coreId;
            records[i * 6 + 5] = game.systemId;
        }
        return records;
    }
    public int preference(String key, int fallback) {
        try {
            if ("theme".equals(key)) {
                String value = library.preferences.getString(key, "corvus");
                if ("shield".equals(value)) return 0;
                if ("dark".equals(value)) {
                    library.preferences.edit().putString(key, "corvus").apply();
                    return 2;
                }
                if ("corvus".equals(value)) return 2;
                if ("asus".equals(value)) return 3;
                return fallback;
            }
            if ("audio".equals(key) || "keep_awake".equals(key)) return library.preferences.getBoolean(key, fallback != 0) ? 1 : 0;
            return library.preferences.getInt(key, fallback);
        } catch (ClassCastException error) { return fallback; }
    }
    public void preferenceSet(String key, int value) {
        if ("theme".equals(key)) {
            String selected = value == 0 ? "shield" : value == 3 ? "asus" : "corvus";
            library.preferences.edit().putString(key, selected).apply();
        }
        else if ("audio".equals(key) || "keep_awake".equals(key)) library.preferences.edit().putBoolean(key, value != 0).apply();
        else library.preferences.edit().putInt(key, value).apply();
    }
    public void pickContent(int system) {
        runOnUiThread(() -> {
            synchronized (importLock) { if (destroyed || pickerOpen || copying || pending != null) return; }
            Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT).setType("*/*").addCategory(Intent.CATEGORY_OPENABLE);
            try {
                pickerSystem = system; pickerOpen = true;
                startActivityForResult(intent, OPEN_CONTENT);
            } catch (android.content.ActivityNotFoundException error) {
                pickerOpen = false;
                notifyError("Sélecteur de documents indisponible");
            }
        });
    }
    @Override protected void onActivityResult(int request, int result, Intent data) {
        super.onActivityResult(request, result, data);
        if (request != OPEN_CONTENT) return;
        pickerOpen = false;
        if (result != RESULT_OK || data == null || data.getData() == null) {
            synchronized (importLock) { if (!destroyed) imported("", "", -1, "", ""); }
            return;
        }
        android.net.Uri uri = data.getData();
        int system = pickerSystem;
        synchronized (importLock) { if (destroyed || copying) return; copying = true; }
        io.execute(() -> {
            try {
                GameLibrary.Pending copied = library.prepare(uri, system);
                synchronized (importLock) {
                    copying = false;
                    if (destroyed) { copied.discard(); return; }
                    if (pending != null) { copied.discard(); return; }
                    pending = copied;
                    imported(copied.game.id, copied.game.title, copied.game.system, copied.temporary.getAbsolutePath(), "");
                }
            } catch (Exception | OutOfMemoryError error) {
                synchronized (importLock) { copying = false; }
                notifyError(error.getMessage() == null ? "Import impossible" : error.getMessage());
            }
        });
    }
    private void notifyError(String message) {
        synchronized (importLock) { if (!destroyed) imported("", "", -1, "", message); }
    }
    public String finishImport(boolean accept, int system, String coreId, String systemId) {
        synchronized (importLock) {
            GameLibrary.Pending ready = pending;
            pending = null;
            if (ready == null) return accept ? "Import interrompu" : null;
            if (!accept || destroyed) { ready.discard(); return accept ? "Import interrompu" : null; }
            if (system < 0 || system >= 32) { ready.discard(); return "Cœur inconnu"; }
            try { library.commit(ready, system, coreId, systemId); return null; }
            catch (IOException error) { return error.getMessage() == null ? "Import impossible" : error.getMessage(); }
        }
    }
    public void touchGame(String id) {
        try { library.touch(id); }
        catch (IOException error) { android.util.Log.w("CorvusRetro", "Historique non enregistré", error); }
    }
    private String asset(String name) throws IOException {
        try (InputStream input = getAssets().open(name)) {
            ByteArrayOutputStream bytes = new ByteArrayOutputStream(); byte[] buffer = new byte[4096]; int count;
            while ((count = input.read(buffer)) != -1) bytes.write(buffer, 0, count);
            return bytes.toString(StandardCharsets.UTF_8.name());
        }
    }
    public String credits() {
        try { return asset("LICENSE.txt") + "\n\n" + asset("CORVUSCORE-NOTICE.txt") + "\n\n" + asset("CORVUSCORE-COPYRIGHT.txt") + "\n\n" + asset("CORVUSAPI-LICENSE.txt") + "\n\n" + asset("ANDROID-NATIVE-GLUE-NOTICE.txt") + "\n\nRetroAchievements - rcheevos (MIT)\n" + asset("RCHEEVOS-LICENSE.txt"); }
        catch (IOException error) { return "Licence indisponible. Consultez LICENSE dans le dépôt."; }
    }
    public void appearance(int color, boolean keepAwake) {
        runOnUiThread(() -> {
            if (destroyed) return;
            getWindow().setStatusBarColor(color); getWindow().setNavigationBarColor(color);
            if (keepAwake) getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
            else getWindow().clearFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        });
    }
    public void quit() { runOnUiThread(() -> { if (!destroyed) finish(); }); }

    private static Paint font() {
        Paint paint = new Paint(Paint.ANTI_ALIAS_FLAG);
        paint.setColor(Color.WHITE); paint.setTextSize(40);
        paint.setTypeface(Typeface.create("sans-serif", Typeface.NORMAL));
        return paint;
    }
    private static String character(int codepoint) {
        return new String(Character.toChars(Character.isValidCodePoint(codepoint) ? codepoint : 0xfffd));
    }
    private static int codepoint(int slot) { return slot < 224 ? slot + 32 : EXTRA_GLYPHS[slot - 224]; }
    public Bitmap atlas() {
        Bitmap atlas = Bitmap.createBitmap(1024, 2048, Bitmap.Config.ARGB_8888);
        Canvas canvas = new Canvas(atlas);
        Paint paint = font();
        for (int slot = 0; slot < 233; slot++)
            canvas.drawText(character(codepoint(slot)), slot % 16 * 64, slot / 16 * 64 + 46, paint);
        for (int i = 0; i < ICONS.length; i++) {
            int slot = 480 + i, x = slot % 16 * 64, y = slot / 16 * 64;
            Drawable icon = getResources().getDrawable(ICONS[i], getTheme()).mutate();
            icon.setTint(Color.WHITE); icon.setBounds(x + 8, y + 8, x + 56, y + 56); icon.draw(canvas);
        }
        canvas.drawRect(960, 1984, 964, 1988, paint);
        return atlas;
    }
    public float[] advances() {
        Paint paint = font(); float[] result = new float[233];
        for (int i = 0; i < result.length; i++) result[i] = paint.measureText(character(codepoint(i)));
        return result;
    }
    public Bitmap glyph(int codepoint) {
        Bitmap bitmap = Bitmap.createBitmap(64, 64, Bitmap.Config.ARGB_8888);
        new Canvas(bitmap).drawText(character(codepoint), 0, 46, font());
        return bitmap;
    }
    public float advance(int codepoint) { return font().measureText(character(codepoint)); }
}
