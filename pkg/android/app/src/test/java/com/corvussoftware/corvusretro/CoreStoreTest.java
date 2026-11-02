// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
package com.corvussoftware.corvusretro;

import android.content.SharedPreferences;
import java.io.File;
import java.io.FileOutputStream;
import java.lang.reflect.InvocationHandler;
import java.lang.reflect.Method;
import java.lang.reflect.Proxy;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.util.Arrays;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.Future;
import java.util.concurrent.TimeUnit;
import java.util.zip.ZipEntry;
import java.util.zip.ZipOutputStream;
import org.json.JSONArray;
import org.json.JSONObject;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.TemporaryFolder;
import static org.junit.Assert.*;

/** Exercise actual immutable-module storage, with synthetic ELF bytes and in-memory Android preferences. */
public final class CoreStoreTest {
    @Rule public TemporaryFolder temporary = new TemporaryFolder();
    private final Memory memory = new Memory();

    private static final class Memory implements InvocationHandler {
        final Map<String,Object> values = new HashMap<>();
        volatile boolean failNext;
        volatile Runnable committed;
        volatile CountDownLatch entered, release;
        final SharedPreferences preferences = (SharedPreferences)Proxy.newProxyInstance(
                SharedPreferences.class.getClassLoader(),new Class<?>[] {SharedPreferences.class},this);
        @Override public Object invoke(Object proxy, Method method, Object[] args) {
            String name = method.getName();
            if (name.equals("edit")) return Proxy.newProxyInstance(SharedPreferences.Editor.class.getClassLoader(),
                    new Class<?>[] {SharedPreferences.Editor.class},new Editor(this));
            synchronized (values) {
                if (name.equals("getAll")) return new HashMap<>(values);
                if (name.equals("contains")) return values.containsKey(args[0]);
                if (name.startsWith("get")) {
                    Object value=values.get(args[0]);
                    if (value==null) return args[1];
                    if (name.equals("getString")) return (String)value;
                    return value;
                }
            }
            if (name.equals("toString")) return "SyntheticPreferences";
            return null;
        }
    }
    private static final class Editor implements InvocationHandler {
        final Memory owner;
        final Map<String,Object> puts = new HashMap<>();
        final Set<String> removes = new HashSet<>();
        boolean clear;
        Editor(Memory owner) { this.owner=owner; }
        @Override public Object invoke(Object proxy, Method method, Object[] args) throws Exception {
            String name=method.getName();
            if (name.startsWith("put")) { puts.put((String)args[0],args[1]); removes.remove(args[0]); return proxy; }
            if (name.equals("remove")) { removes.add((String)args[0]); puts.remove(args[0]); return proxy; }
            if (name.equals("clear")) { clear=true; return proxy; }
            if (name.equals("commit") || name.equals("apply")) {
                boolean failed=owner.failNext; owner.failNext=false;
                synchronized (owner.values) {
                    if (clear) owner.values.clear();
                    for (String key:removes) owner.values.remove(key);
                    owner.values.putAll(puts);
                }
                // Android updates memory before the synchronous disk result is known.
                if (owner.committed!=null) owner.committed.run();
                CountDownLatch entered=owner.entered, release=owner.release;
                if (entered!=null && release!=null) {
                    entered.countDown();
                    if (!release.await(5,TimeUnit.SECONDS)) throw new AssertionError("synthetic commit timeout");
                }
                return name.equals("commit") ? !failed : null;
            }
            return proxy;
        }
    }
    private static final class Archive {
        final CoreRepository.Core core;
        final File zip;
        final byte[] elf;
        Archive(CoreRepository.Core core, File zip, byte[] elf) { this.core=core;this.zip=zip;this.elf=elf; }
    }
    private Archive archive(String id,String library,int marker) throws Exception {
        byte[] elf=new byte[64];elf[0]=0x7f;elf[1]='E';elf[2]='L';elf[3]='F';elf[4]=2;elf[5]=1;
        elf[16]=3;elf[18]=62;elf[32]=(byte)marker;
        File zip=temporary.newFile();
        try (ZipOutputStream output=new ZipOutputStream(new FileOutputStream(zip))) {
            output.putNextEntry(new ZipEntry(library));output.write(elf);output.closeEntry();
        }
        String name=id.startsWith("corvus.") ? "Corvus"+id.substring(7).toUpperCase(java.util.Locale.ROOT) : "Community core";
        JSONObject metadata=new JSONObject().put("id",id).put("name",name).put("description","Original test fixture")
                .put("version","1.0.0").put("abi","x86_64").put("api_version",0).put("library_name",library)
                .put("size",zip.length()).put("sha256",CoreStore.hash(zip))
                .put("archive_url","https://github.com/CorvusSoftware/CorvusRepository/releases/download/v1.0.0/Test.zip");
        CoreRepository.Core core=new CoreRepository.Core(metadata,id.startsWith("corvus.") ? "corvuscore.json" : "vendor.json");
        return new Archive(core,zip,elf);
    }
    private CoreStore store(File directory) { return new CoreStore(directory,memory.preferences); }
    private File current(File directory,String id) throws Exception {
        return new File(directory,new JSONObject(memory.preferences.getString(id,"")).getString("file"));
    }

    @Test public void removalKeepsOtherCoresGamesAndSavesAndCanReinstall() throws Exception {
        File directory=temporary.newFolder("cores");CoreStore store=store(directory);
        Archive first=archive("corvus.gb","libcorvus_gb.so",1), second=archive("corvus.gb","libcorvus_gb.so",2);
        Archive gbc=archive("corvus.gbc","libcorvus_gbc.so",3);
        store.install(first.core,first.zip);File old=current(directory,"corvus.gb");
        store.install(second.core,second.zip);File latest=current(directory,"corvus.gb");
        store.install(gbc.core,gbc.zip);File other=current(directory,"corvus.gbc");
        File save=temporary.newFile("Original.sav"), rom=temporary.newFile("Original.rom"), state=temporary.newFile("Original.state");
        for (File file:Arrays.asList(save,rom,state)) Files.write(file.toPath(),"Preserved".getBytes(StandardCharsets.UTF_8));
        assertTrue(store.canRemove("corvus.gb"));assertFalse(store.canRemove("../../other"));
        memory.committed=() -> { assertTrue(old.exists());assertTrue(latest.exists()); };
        assertTrue(store.remove("corvus.gb"));memory.committed=null;
        assertFalse(old.exists());assertFalse(latest.exists());assertTrue(other.exists());
        assertFalse(store.canRemove("corvus.gb"));assertFalse(memory.preferences.contains("corvus.gb"));
        assertArrayEquals(new String[] {other.getAbsolutePath()},store.modules());
        for (File file:Arrays.asList(save,rom,state)) assertEquals("Preserved",new String(Files.readAllBytes(file.toPath()),StandardCharsets.UTF_8));
        CoreStore restarted=store(directory);assertFalse(restarted.canRemove("corvus.gb"));
        restarted.install(second.core,second.zip);assertTrue(restarted.canRemove("corvus.gb"));assertEquals(2,restarted.modules().length);
    }

    @Test public void sharedLibraryFilenameRemainsUntilLastOwnerIsRemoved() throws Exception {
        File directory=temporary.newFolder("cores");CoreStore store=store(directory);
        Archive a=archive("vendor.a","libshared.so",4),b=archive("vendor.b","libshared.so",4);
        store.install(a.core,a.zip);File file=current(directory,"vendor.a");
        store.install(b.core,b.zip);assertEquals(file,current(directory,"vendor.b"));
        assertTrue(store.remove("vendor.a"));assertTrue(file.exists());assertTrue(store.canRemove("vendor.b"));
        assertArrayEquals(new String[] {file.getAbsolutePath()},store.modules());
        assertTrue(store.remove("vendor.b"));assertFalse(file.exists());
    }

    @Test public void removalDoesNotGuessLegacyOrphanOwnershipFromLibraryPrefix() throws Exception {
        File directory=temporary.newFolder("cores");CoreStore store=store(directory);
        Archive old=archive("corvus.gb","libcorvus_gb.so",5),latest=archive("corvus.gb","libcorvus_gb.so",6);
        store.install(old.core,old.zip);File orphan=current(directory,"corvus.gb");
        store.install(latest.core,latest.zip);File file=current(directory,"corvus.gb");
        JSONObject legacy=new JSONObject(memory.preferences.getString("corvus.gb",""));
        assertNotNull(legacy.remove("files"));
        memory.preferences.edit().putString("corvus.gb",legacy.toString()).commit();
        assertTrue(store.remove("corvus.gb"));assertFalse(file.exists());assertTrue(orphan.exists());
    }

    @Test public void symlinkCleanupNeverTouchesTheOutsideTarget() throws Exception {
        File directory=temporary.newFolder("cores");CoreStore store=store(directory);
        Archive a=archive("corvus.gb","libcorvus_gb.so",7);store.install(a.core,a.zip);
        File file=current(directory,"corvus.gb"),outside=temporary.newFile("outside");Files.write(outside.toPath(),new byte[] {11,22});
        assertTrue(file.delete());Files.createSymbolicLink(file.toPath(),outside.toPath());
        assertFalse(store.remove("corvus.gb"));assertArrayEquals(new byte[] {11,22},Files.readAllBytes(outside.toPath()));
        assertTrue(Files.isSymbolicLink(file.toPath()));assertEquals(0,store.modules().length);
    }

    @Test public void traversalMetadataIsRejectedWithoutDeletingAnything() throws Exception {
        File directory=temporary.newFolder("cores");CoreStore store=store(directory);
        Archive a=archive("corvus.gb","libcorvus_gb.so",8);store.install(a.core,a.zip);
        File file=current(directory,"corvus.gb"),outside=temporary.newFile("outside");
        JSONObject invalid=new JSONObject(memory.preferences.getString("corvus.gb","")).put("files",new JSONArray().put("../outside"));
        memory.preferences.edit().putString("corvus.gb",invalid.toString()).commit();
        assertFalse(store.canRemove("corvus.gb"));
        try { store.remove("corvus.gb");fail("invalid history must fail"); } catch (java.io.IOException expected) { }
        assertTrue(file.exists());assertTrue(outside.exists());assertTrue(memory.preferences.contains("corvus.gb"));
    }

    @Test public void failedPreferenceCommitKeepsModuleAndOriginalMetadata() throws Exception {
        File directory=temporary.newFolder("cores");CoreStore store=store(directory);
        Archive a=archive("corvus.gb","libcorvus_gb.so",9);store.install(a.core,a.zip);
        String previous=memory.preferences.getString("corvus.gb",null);File file=current(directory,"corvus.gb");
        memory.failNext=true;
        try { store.remove("corvus.gb");fail("failed durable removal must be reported"); } catch (java.io.IOException expected) { }
        assertEquals(previous,memory.preferences.getString("corvus.gb",null));assertTrue(file.exists());assertTrue(store.canRemove("corvus.gb"));
        assertArrayEquals(new String[] {file.getAbsolutePath()},store.modules());
    }

    @Test public void libraryRenameTracksBothVerifiedVersions() throws Exception {
        File directory=temporary.newFolder("cores");CoreStore store=store(directory);
        Archive a=archive("vendor.alpha","libold.so",12),b=archive("vendor.alpha","libnew.so",13);
        store.install(a.core,a.zip);File old=current(directory,"vendor.alpha");
        store.install(b.core,b.zip);File latest=current(directory,"vendor.alpha");
        assertTrue(store.remove("vendor.alpha"));assertFalse(old.exists());assertFalse(latest.exists());
    }

    @Test public void symlinkDirectoryIsRejectedBeforeMetadataRemoval() throws Exception {
        File directory=temporary.newFolder("cores");CoreStore store=store(directory);
        Archive a=archive("corvus.gb","libcorvus_gb.so",14);store.install(a.core,a.zip);
        File moved=new File(temporary.getRoot(),"moved-cores");assertTrue(directory.renameTo(moved));
        Files.createSymbolicLink(directory.toPath(),moved.toPath());
        try { store.remove("corvus.gb");fail("symlink directory must fail"); } catch (java.io.IOException expected) { }
        assertTrue(memory.preferences.contains("corvus.gb"));assertEquals(1,moved.list().length);
    }

    @Test public void invalidTypedMetadataCanBeReplacedAndFailedInstallRestoresItsType() throws Exception {
        File directory=temporary.newFolder("cores");CoreStore store=store(directory);
        Archive a=archive("corvus.gb","libcorvus_gb.so",15);
        memory.preferences.edit().putInt("corvus.gb",17).commit();
        assertFalse(store.canRemove("corvus.gb"));memory.failNext=true;
        try { store.install(a.core,a.zip);fail("failed install metadata must fail"); } catch (java.io.IOException expected) { }
        assertEquals(17,memory.preferences.getInt("corvus.gb",0));
        store.install(a.core,a.zip);assertTrue(store.canRemove("corvus.gb"));assertEquals(1,store.modules().length);
    }

    @Test public void versionHistoryIsBoundedWithoutCreatingUntrackedCommittedModules() throws Exception {
        File directory=temporary.newFolder("cores");CoreStore store=store(directory);
        for (int marker=0;marker<64;++marker) {
            Archive a=archive("corvus.gb","libcorvus_gb.so",marker);store.install(a.core,a.zip);
        }
        String previous=memory.preferences.getString("corvus.gb",null);
        assertEquals(64,new JSONObject(previous).getJSONArray("files").length());
        Archive overflow=archive("corvus.gb","libcorvus_gb.so",64);
        try { store.install(overflow.core,overflow.zip);fail("bounded history must reject another distinct version"); }
        catch (java.io.IOException expected) { }
        assertEquals(previous,memory.preferences.getString("corvus.gb",null));assertEquals(64,directory.list().length);
        assertTrue(store.remove("corvus.gb"));assertEquals(0,directory.list().length);
    }

    @Test public void menuCapabilityDoesNotWaitForInstallationIoLock() throws Exception {
        File directory=temporary.newFolder("cores");CoreStore store=store(directory);
        Archive a=archive("corvus.gb","libcorvus_gb.so",10),b=archive("corvus.gb","libcorvus_gb.so",11);
        store.install(a.core,a.zip);assertTrue(store.canRemove("corvus.gb"));
        memory.entered=new CountDownLatch(1);memory.release=new CountDownLatch(1);
        ExecutorService worker=Executors.newFixedThreadPool(2);
        try {
            Future<?> update=worker.submit(() -> { try { store.install(b.core,b.zip); } catch (Exception error) { throw new AssertionError(error); } });
            assertTrue(memory.entered.await(5,TimeUnit.SECONDS));
            Future<Boolean> capability=worker.submit(() -> store.canRemove("corvus.gb"));
            assertTrue(capability.get(500,TimeUnit.MILLISECONDS));
            memory.release.countDown();update.get(5,TimeUnit.SECONDS);
        } finally { memory.release.countDown();worker.shutdownNow(); }
    }
}
