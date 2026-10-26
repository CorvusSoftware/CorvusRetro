// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
package com.corvussoftware.corvusretro;

import android.app.Activity;
import android.app.Instrumentation;
import android.content.Context;
import android.content.ContextWrapper;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Bundle;
import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.net.URL;
import java.net.URLConnection;
import java.net.URLStreamHandler;
import java.security.cert.Certificate;
import java.util.Arrays;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.zip.ZipEntry;
import java.util.zip.ZipOutputStream;
import javax.net.ssl.HttpsURLConnection;
import org.json.JSONArray;
import org.json.JSONObject;

/** Real compiled cores with a deterministic HTTPS transport fixture. */
public final class CoreRepositoryInstrumentation extends Instrumentation {
    private int checks;
    private File fixtures;
    private boolean truncated;
    private String requestedCore = "";
    private volatile java.util.concurrent.CountDownLatch transferStarted;
    private volatile java.util.concurrent.CountDownLatch transferRelease;
    private void check(boolean condition, String message) {
        if (!condition) throw new AssertionError(message);
        checks++;
    }
    private File archive(String id) {
        return new File(fixtures, "Corvus" + id.toUpperCase(java.util.Locale.ROOT) + "-0.14.1-android-x86_64.zip");
    }
    private JSONObject core(String id) throws Exception {
        File file = archive(id);
        return new JSONObject().put("id", "corvus." + id).put("name", "Corvus" + id.toUpperCase(java.util.Locale.ROOT))
                .put("description", "Official core test fixture").put("version", "0.14.1")
                .put("abi", "x86_64").put("sha256", CoreStore.hash(file)).put("size", file.length())
                .put("archive_url", "https://github.com/CorvusSoftware/CorvusRetro/releases/download/v0.13.0/" + file.getName())
                .put("library_name", "libcorvus_" + id + ".so").put("api_version", 0);
    }
    private JSONObject item(String name) throws Exception {
        return new JSONObject().put("type", "file").put("name", name)
                .put("path", "catalog/" + name).put("size", 4096)
                .put("download_url", CoreCatalogDiscovery.RAW + name);
    }
    private String listing() throws Exception {
        return new JSONArray().put(item("corvuscore.json")).put(item("demo.json")).toString();
    }
    private JSONObject community() throws Exception {
        return new JSONObject(core("gb").toString())
                .put("id", "demo.handheld").put("name", "Demo")
                .put("library_name", "libdemo_handheld.so");
    }
    private String feed(JSONObject... entries) throws Exception {
        JSONArray array = new JSONArray();
        for (JSONObject entry : entries) array.put(entry);
        return new JSONObject().put("format", 3).put("cores", array).toString();
    }
    private void reject(JSONObject entry) throws Exception {
        try {
            new CoreRepository.Core(entry);
            throw new AssertionError("invalid catalog entry accepted");
        } catch (IOException | org.json.JSONException expected) { checks++; }
    }
    private void catalogs() throws Exception {
        check(CoreRepository.parseCatalog(feed(), "x86_64").cores.isEmpty(), "empty catalog");
        CoreRepository.Catalog catalog = CoreRepository.parseCatalog(feed(core("gb"), core("gba")
                .put("abi", "arm64-v8a")), "x86_64");
        check(catalog.cores.size() == 1 && catalog.cores.containsKey("corvus.gb"), "ABI filter");
        check(Arrays.equals(catalog.records, new String[] {"corvus.gb", "CorvusGB", "0.14.1", "corvus.gb", "x86_64"}), "JNI fields");
        try {
            CoreRepository.parseCatalog(feed(core("gb"), core("gb")), "x86_64");
            throw new AssertionError("duplicate core accepted");
        } catch (IOException expected) { checks++; }
        reject(core("gb").put("api_version", 1));
        reject(core("gb").put("api_version", true));
        reject(core("gb").put("size", 12.0));
        reject(core("gb").put("name", 1));
        reject(core("gb").put("library_name", "../libcorvus_gb.so"));
        reject(core("gb").put("archive_url", "https://example.com/core.zip"));
        reject(core("gb").put("base_version_code", 13));
    }
    private void discovery() throws Exception {
        java.util.List<String> list = CoreCatalogDiscovery.sources(listing());
        check(list.size() == 2 && list.get(0).endsWith("/corvuscore.json") &&
                list.get(1).endsWith("/demo.json"), "automatic sorted discovery");
        CoreRepository.Catalog extra =
                CoreRepository.parseCatalog(feed(community()), "x86_64", "demo.json");
        check(extra.cores.containsKey("demo.handheld"), "namespaced provider");
        CoreCatalogDiscovery.Collector joined = new CoreCatalogDiscovery.Collector();
        joined.add(CoreRepository.parseCatalog(feed(core("gb")), "x86_64"));
        joined.add(extra);
        check(joined.result().cores.size() == 2, "multiple provider feeds combined");
        try {
            joined.add(CoreRepository.parseCatalog(feed(core("gb")), "x86_64"));
            throw new AssertionError("duplicate core accepted across feeds");
        } catch (IOException expected) { checks++; }
        try {
            CoreRepository.parseCatalog(feed(community().put("id", "corvus.gb")),
                    "x86_64", "demo.json");
            throw new AssertionError("provider ID spoofing accepted");
        } catch (IOException expected) { checks++; }
        try {
            CoreRepository.parseCatalog(feed(community().put("library_name", "../evil.so")),
                    "x86_64", "demo.json");
            throw new AssertionError("unsafe module name accepted");
        } catch (IOException expected) { checks++; }
        try {
            CoreRepository.parseCatalog(feed(community().put("archive_url",
                    "https://github.com/OtherOrg/Bad/releases/download/v1.0.0/bad.zip")),
                    "x86_64", "demo.json");
            throw new AssertionError("untrusted archive source accepted");
        } catch (IOException expected) { checks++; }
        try {
            CoreCatalogDiscovery.sources(new JSONArray().put(item("corvuscore.json"))
                    .put(item("../evil.json")).toString());
            throw new AssertionError("traversal in catalog file accepted");
        } catch (IOException expected) { checks++; }
        try {
            CoreCatalogDiscovery.sources(new JSONArray().put(item("corvuscore.json"))
                    .put(item("demo.json").put("download_url", "https://evil.example/demo.json"))
                    .toString());
            throw new AssertionError("malicious catalog URL accepted");
        } catch (IOException expected) { checks++; }
    }
    private void store() throws Exception {
        Context target = getTargetContext();
        File root = new File(target.getCacheDir(), "native-core-store-test");
        check(root.isDirectory() || root.mkdirs(), "isolated module storage");
        Context context = new ContextWrapper(target) {
            @Override public File getFilesDir() { return root; }
            @Override public SharedPreferences getSharedPreferences(String key, int mode) {
                return super.getSharedPreferences("test_" + key, mode);
            }
        };
        context.getSharedPreferences("native_cores", Context.MODE_PRIVATE).edit().clear().commit();
        CoreStore store = new CoreStore(context);
        check(store.modules().length == 0, "zero downloaded cores");
        CoreRepository.Core gba = new CoreRepository.Core(core("gba"));
        store.install(gba, archive("gba"));
        String first = store.modules()[0];
        check(!new File(first).canWrite(), "installed library is read-only");
        System.load(first);
        check(true, "actual native module loads on the current Android target");
        store.install(new CoreRepository.Core(core("gb")), archive("gb"));
        check(store.modules().length == 2 && Arrays.asList(store.modules()).contains(first), "adding GB preserves GBA");
        check(new CoreStore(context).modules().length == 2, "modules survive service recreation");
        store.install(gba, archive("gba"));
        check(store.modules().length == 2, "same-version download is idempotent");
        try {
            store.install(new CoreRepository.Core(core("gb").put("sha256", "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa")), archive("gb"));
            throw new AssertionError("bad hash accepted");
        } catch (IOException expected) { checks++; }
        try {
            store.install(new CoreRepository.Core(core("gb").put("abi", "arm64-v8a")), archive("gb"));
            throw new AssertionError("wrong ELF machine accepted");
        } catch (IOException expected) { checks++; }
        File unsafe = new File(root, "traversal.zip");
        try (ZipOutputStream zip = new ZipOutputStream(new FileOutputStream(unsafe))) {
            zip.putNextEntry(new ZipEntry("../libcorvus_gb.so")); zip.write(new byte[32]); zip.closeEntry();
        }
        try {
            store.install(new CoreRepository.Core(core("gb").put("sha256", CoreStore.hash(unsafe)).put("size", unsafe.length())), unsafe);
            throw new AssertionError("archive path traversal accepted");
        } catch (IOException expected) { checks++; }
        check(store.modules().length == 2, "failed downloads preserve installed modules");
        File altered = new File(first);
        check(altered.setWritable(true), "create altered fixture");
        check(store.modules().length == 1, "writable executable rejected on discovery");
        check(altered.setReadOnly(), "restore fixture permissions");
    }
    private void transport() {
        URL.setURLStreamHandlerFactory(protocol -> "https".equals(protocol) ? new URLStreamHandler() {
            @Override protected URLConnection openConnection(URL url) {
                return new HttpsURLConnection(url) {
                    @Override public int getResponseCode() { return 200; }
                    @Override public InputStream getInputStream() throws IOException {
                        try {
                            if ("api.github.com".equals(url.getHost()))
                                return new ByteArrayInputStream(listing().getBytes(java.nio.charset.StandardCharsets.UTF_8));
                            if (url.toString().endsWith("/demo.json"))
                                return new ByteArrayInputStream(feed(community()).getBytes(java.nio.charset.StandardCharsets.UTF_8));
                            if (url.toString().endsWith("/corvuscore.json"))
                                return new ByteArrayInputStream(feed(core("gb"), core("gbc"), core("gba")).getBytes(java.nio.charset.StandardCharsets.UTF_8));
                            requestedCore = new File(url.getPath()).getName();
                            if (requestedCore.startsWith("CorvusGB-") && transferStarted != null) {
                                transferStarted.countDown();
                                if (!transferRelease.await(20, java.util.concurrent.TimeUnit.SECONDS))
                                    throw new IOException("fixture transfer timeout");
                            }
                            ByteArrayOutputStream bytes = new ByteArrayOutputStream();
                            try (FileInputStream input = new FileInputStream(new File(fixtures, requestedCore))) {
                                byte[] buffer = new byte[65536];
                                int count;
                                while ((count = input.read(buffer)) != -1) bytes.write(buffer, 0, count);
                            }
                            byte[] payload = bytes.toByteArray();
                            return new ByteArrayInputStream(truncated ? Arrays.copyOf(payload, payload.length / 2) : payload);
                        } catch (Exception error) { throw new IOException(error); }
                    }
                    @Override public void disconnect() { }
                    @Override public boolean usingProxy() { return false; }
                    @Override public void connect() { }
                    @Override public String getCipherSuite() { return "fixture"; }
                    @Override public Certificate[] getLocalCertificates() { return null; }
                    @Override public Certificate[] getServerCertificates() { return null; }
                };
            }
        } : null);
    }
    private void downloader() throws Exception {
        transport();
        MainActivity activity = (MainActivity) startActivitySync(new Intent(getTargetContext(), MainActivity.class)
                .addFlags(Intent.FLAG_ACTIVITY_NEW_TASK));
        ExecutorService executor = Executors.newSingleThreadExecutor();
        try {
            CoreRepository repository = new CoreRepository(activity, executor);
            repository.refresh();
            executor.submit(() -> { }).get();
            for (String id : new String[] {"gba", "gb", "gbc"}) {
                if ("gb".equals(id)) {
                    transferStarted = new java.util.concurrent.CountDownLatch(1);
                    transferRelease = new java.util.concurrent.CountDownLatch(1);
                }
                repository.install("corvus." + id);
                if ("gb".equals(id)) {
                    check(transferStarted.await(10, java.util.concurrent.TimeUnit.SECONDS), "download running before recreation");
                    ActivityMonitor monitor = addMonitor(MainActivity.class.getName(), null, false);
                    MainActivity old = activity;
                    runOnMainSync(old::recreate);
                    activity = (MainActivity) monitor.waitForActivityWithTimeout(15000);
                    removeMonitor(monitor);
                    check(activity != null && activity != old && activity.coreInstallPending(), "recreated activity retains pending download");
                    transferRelease.countDown();
                }
                executor.submit(() -> { }).get();
                if ("gb".equals(id)) {
                    check(activity.installedCoreModules().length == 2, "recreated activity sees the new module after completion");
                    transferStarted = null;
                    repository = new CoreRepository(activity, executor);
                    repository.refresh();
                    executor.submit(() -> { }).get();
                }
                check(!CoreRepository.downloading(), "download completes without an installer session");
                check(!requestedCore.isEmpty(), "production downloader requested the archive");
            }
            check(repository.installedModules().length == 3, "all three modules downloaded individually");
            android.os.SystemClock.sleep(400);
            sendKeyDownUpSync(android.view.KeyEvent.KEYCODE_DPAD_CENTER);
            android.os.SystemClock.sleep(400);
            android.graphics.Bitmap screen = getUiAutomation().takeScreenshot();
            check(screen != null, "core list captured in the same running activity after downloads");
            try (FileOutputStream output = new FileOutputStream(new File(activity.getFilesDir(), "test-immediate-core-list.png"))) {
                screen.compress(android.graphics.Bitmap.CompressFormat.PNG, 100, output);
            }
            screen.recycle();
            sendKeyDownUpSync(android.view.KeyEvent.KEYCODE_DPAD_CENTER);
            android.os.SystemClock.sleep(400);
            screen = getUiAutomation().takeScreenshot();
            try (FileOutputStream output = new FileOutputStream(new File(activity.getFilesDir(), "test-immediate-loaded-core.png"))) {
                screen.compress(android.graphics.Bitmap.CompressFormat.PNG, 100, output);
            }
            screen.recycle();
            String[] before = repository.installedModules();
            truncated = true;
            repository.install("corvus.gb");
            executor.submit(() -> { }).get();
            check(Arrays.equals(before, repository.installedModules()), "truncated download retains existing modules");
            check(activity.getPackageManager().getPackageInstaller().getMySessions().isEmpty(), "no Android installation session");
            check(new CoreStore(activity).modules().length == 3, "modules persist for next launch");
        } finally {
            executor.shutdown();
            if (transferRelease != null) transferRelease.countDown();
            MainActivity last = activity;
            if (last != null) runOnMainSync(last::finish);
        }
    }
    @Override public void onCreate(Bundle arguments) {
        super.onCreate(arguments);
        fixtures = new File(arguments.getString("fixture_dir", ""));
        start();
    }
    @Override public void onStart() {
        Bundle result = new Bundle();
        try {
            catalogs(); discovery(); store(); downloader();
            result.putString("stream", "PASS " + checks + " native core catalog/store/downloader checks\n");
            finish(Activity.RESULT_OK, result);
        } catch (Throwable failure) {
            result.putString("stream", "FAIL " + failure + "\n");
            finish(Activity.RESULT_CANCELED, result);
        }
    }
}
