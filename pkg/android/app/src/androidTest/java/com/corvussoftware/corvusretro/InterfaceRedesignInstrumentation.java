// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
package com.corvussoftware.corvusretro;

import android.app.Activity;
import android.app.Instrumentation;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.graphics.Bitmap;
import android.os.Bundle;
import android.os.SystemClock;
import android.view.KeyEvent;
import android.view.MotionEvent;
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
import java.nio.charset.StandardCharsets;
import java.security.MessageDigest;
import java.security.cert.Certificate;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.concurrent.Callable;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import javax.net.ssl.HttpsURLConnection;
import org.json.JSONArray;
import org.json.JSONObject;

/** Real native navigation with original homebrew and a deterministic official-origin fixture transport. */
public final class InterfaceRedesignInstrumentation extends Instrumentation {
    private static final String[] CORE_IDS = {"corvus.gb", "corvus.gbc", "corvus.gba"};
    private static final String[] CORE_NAMES = {"CorvusGB", "CorvusGBC", "CorvusGBA"};
    private static final String[] SYSTEMS = {"nintendo.game_boy", "nintendo.game_boy_color", "nintendo.game_boy_advance"};
    private final Map<String, byte[]> responses = new HashMap<>();
    private final List<String> fixtureIds = new ArrayList<>();
    private final Map<String, Object> oldPreferences = new HashMap<>();
    private volatile int archiveRequests;
    private int checks;
    private boolean landscape;
    private File fixtures;
    private String oldGames;
    private float originalWidth, originalHeight;
    private MainActivity activity;

    private void check(boolean value, String message) {
        if (!value) throw new AssertionError(message);
        checks++;
    }
    private void waitFor(Callable<Boolean> condition, String description) throws Exception {
        long end = SystemClock.uptimeMillis() + 30000;
        while (!condition.call() && SystemClock.uptimeMillis() < end) SystemClock.sleep(100);
        check(condition.call(), description);
    }
    private String shell(String command) throws Exception {
        try (android.os.ParcelFileDescriptor fd = getUiAutomation().executeShellCommand(command);
             InputStream in = new android.os.ParcelFileDescriptor.AutoCloseInputStream(fd);
             ByteArrayOutputStream output = new ByteArrayOutputStream()) {
            byte[] block = new byte[8192]; int n;
            while ((n = in.read(block)) != -1) {
                check(output.size() + n <= 1024 * 1024, "bounded Android shell response");
                output.write(block, 0, n);
            }
            return output.toString(StandardCharsets.UTF_8.name());
        }
    }
    private String stateLine() throws Exception {
        String result = "";
        String text = shell("logcat -d --pid=" + android.os.Process.myPid() + " -s CorvusRetro:D '*:S'");
        for (String line : text.split("\n")) if (line.contains("frontend page=")) result = line;
        return result;
    }
    private int number(String key) throws Exception {
        Matcher match = Pattern.compile("(?:^| )" + key + "=(-?\\d+)").matcher(stateLine());
        return match.find() ? Integer.parseInt(match.group(1)) : Integer.MIN_VALUE;
    }
    private void state(int page, int core, int content, int quick) throws Exception {
        waitFor(() -> {
            String current = stateLine();
            return current.contains("frontend page=" + page + " core=" + core + " content=" + content + " quick=" + quick)
                    && current.contains(" active=1 ");
        }, "native navigation " + page + "/" + core + "/" + content + "/" + quick);
    }
    private void page(int page) throws Exception { waitFor(() -> number("page") == page, "native page " + page); }
    private float width() { int[] a = activity.safeWindowArea(); return (a[0] - a[2] - a[4]) / activity.density(); }
    private float height() { int[] a = activity.safeWindowArea(); return (a[1] - a[3] - a[5]) / activity.density(); }
    private void tap(float x, float y) throws Exception {
        waitFor(activity::hasWindowFocus, "native Activity owns touch window");
        int[] area = activity.safeWindowArea(); float density = activity.density();
        long time = SystemClock.uptimeMillis();
        sendPointerSync(MotionEvent.obtain(time, time, MotionEvent.ACTION_DOWN,
                area[2] + x * density, area[3] + y * density, 0));
        sendPointerSync(MotionEvent.obtain(time, time + 16, MotionEvent.ACTION_UP,
                area[2] + x * density, area[3] + y * density, 0));
        SystemClock.sleep(350);
    }
    private void normalViewport() throws Exception {
        waitFor(()->Math.abs(width()-originalWidth)<2 && Math.abs(height()-originalHeight)<2,"IME restores the native safe viewport");
    }
    private void tab(int index, int expectedPage) throws Exception {
        normalViewport();
        tap(width() * (index + .5f) / 5, height() - 30); page(expectedPage);
    }
    private void key(int key) { sendKeyDownUpSync(key); SystemClock.sleep(100); }
    private void choose(int index) throws Exception {
        int focus = number("focus");
        check(focus >= 0 && focus < 4096, "native focus is observable before keyboard selection");
        int difference = index - focus;
        for (int n = 0; n < Math.abs(difference); ++n)
            key(difference < 0 ? KeyEvent.KEYCODE_DPAD_UP : KeyEvent.KEYCODE_DPAD_DOWN);
        waitFor(() -> number("focus") == index, "native keyboard focus reaches row " + index);
        key(KeyEvent.KEYCODE_DPAD_CENTER);
    }
    private void back(int expected) throws Exception { key(KeyEvent.KEYCODE_BACK); page(expected); }
    private void firstLibraryCard() throws Exception {
        int columns = width() >= 700 ? 4 : 2;
        float tile = (width() - 32 - (columns - 1) * 12) / columns;
        tap(16 + tile / 2, Math.min(108 + tile * 1.26f / 2, height() - 88));
    }
    private void shot(String name) throws Exception {
        SystemClock.sleep(500);
        Bitmap bitmap = getUiAutomation().takeScreenshot();
        check(bitmap != null && bitmap.getWidth() > 300 && bitmap.getHeight() > 300, "screenshot " + name);
        try (FileOutputStream out = new FileOutputStream(new File(getTargetContext().getFilesDir(),
                "test-interface-" + (landscape ? "landscape-" : "portrait-") + name + ".png"))) {
            check(bitmap.compress(Bitmap.CompressFormat.PNG, 100, out), "encode " + name);
        } finally { bitmap.recycle(); }
    }
    private boolean greenPreview() {
        Bitmap b = getUiAutomation().takeScreenshot();
        if (b == null) return false;
        int found = 0;
        try {
            for (int y = 0; y < b.getHeight(); y += 3) for (int x = 0; x < b.getWidth(); x += 3) {
                int p = b.getPixel(x,y);
                if (((p >> 8) & 255) > 100 && ((p >> 16) & 255) < 70 && (p & 255) < 70) ++found;
            }
        } finally { b.recycle(); }
        return found > 60;
    }
    private void imeAction(boolean accepted) throws Exception {
        java.lang.reflect.Field bridge=MainActivity.class.getDeclaredField("textInput");bridge.setAccessible(true);
        Object owner=bridge.get(activity);
        java.lang.reflect.Field field=NativeTextInput.class.getDeclaredField("editor");field.setAccessible(true);
        android.view.View editor=(android.view.View)field.get(owner);
        check(editor!=null,"the Android IME owns its current native-rendered editor");
        runOnMainSync(()-> {
            if(accepted) {
                android.view.inputmethod.InputConnection connection=editor.onCreateInputConnection(new android.view.inputmethod.EditorInfo());
                check(connection!=null&&connection.performEditorAction(android.view.inputmethod.EditorInfo.IME_ACTION_DONE),
                        "real Android IME DONE callback is consumed");
            } else check(editor.onKeyPreIme(KeyEvent.KEYCODE_BACK,new KeyEvent(KeyEvent.ACTION_UP,KeyEvent.KEYCODE_BACK)),
                    "real Android pre-IME BACK callback is consumed");
        });
    }
    private void imeSetText(String text) throws Exception {
        java.lang.reflect.Field bridge=MainActivity.class.getDeclaredField("textInput");bridge.setAccessible(true);
        Object owner=bridge.get(activity);
        java.lang.reflect.Field field=NativeTextInput.class.getDeclaredField("editor");field.setAccessible(true);
        android.view.View editor=(android.view.View)field.get(owner);
        check(editor!=null,"the current IME editor exists for composing text");
        runOnMainSync(()-> {
            android.view.inputmethod.InputConnection connection=editor.onCreateInputConnection(new android.view.inputmethod.EditorInfo());
            check(connection instanceof android.view.inputmethod.BaseInputConnection,"native text bridge exposes a real editable input connection");
            int length=((android.view.inputmethod.BaseInputConnection)connection).getEditable().length();
            check(connection.setSelection(0,length)&&connection.commitText(text,1),"real IME replaces the selected native-rendered text");
        });
    }
    private long ticket() throws Exception {
        java.lang.reflect.Field f = MainActivity.class.getDeclaredField("textInputTicket");
        f.setAccessible(true); return f.getLong(null);
    }
    private void editing(int field, int length) throws Exception {
        waitFor(() -> number("editing") == field && (length < 0 || number("search_length") == length),
                "native IME field/UTF-8 length " + field + "/" + length);
    }
    private void staysEditing(int field, int length) throws Exception {
        long end = SystemClock.uptimeMillis() + 1400;
        do {
            check(number("editing") == field && (length < 0 || number("search_length") == length),
                    "stale IME result does not alter the current native editor");
            SystemClock.sleep(100);
        } while (SystemClock.uptimeMillis() < end);
    }
    private byte[] bytes(File file) throws Exception {
        try (InputStream in = new FileInputStream(file); ByteArrayOutputStream out = new ByteArrayOutputStream()) {
            byte[] block = new byte[8192]; int n;
            while ((n=in.read(block))!=-1) out.write(block,0,n);
            return out.toByteArray();
        }
    }
    private void prepare() throws Exception {
        Context context = getTargetContext(); SharedPreferences p = context.getSharedPreferences("corvus",Context.MODE_PRIVATE);
        oldGames = p.getString("games","[]");
        String[] keys = {"theme","audio","show_input","keep_awake","achievements_enabled","auto_state","video_integer","video_smooth","audio_volume"};
        for (String key : keys) oldPreferences.put(key,p.getAll().get(key));
        check(p.edit().putString("theme","corvus").putBoolean("audio",true).putInt("show_input",1)
                .putBoolean("keep_awake",true).putInt("achievements_enabled",0).putInt("auto_state",1)
                .putInt("video_integer",0).putInt("video_smooth",0).putInt("audio_volume",100).commit(), "native settings fixture");
        JSONArray cores = new JSONArray();
        for (int system=0;system<3;++system) {
            File archive = new File(fixtures,CORE_NAMES[system]+"-0.15.0-android-x86_64.zip");
            check(archive.isFile(),"official core fixture archive " + system);
            String address = "https://github.com/CorvusSoftware/CorvusRetro/releases/download/v0.16.0/" + archive.getName();
            JSONObject metadata = new JSONObject().put("id",CORE_IDS[system]).put("name",CORE_NAMES[system])
                    .put("description","Module officiel pour qualification de l'interface").put("version","0.15.0")
                    .put("abi","x86_64").put("api_version",0).put("library_name","libcorvus_"+CORE_IDS[system].substring(7)+".so")
                    .put("size",archive.length()).put("sha256",CoreStore.hash(archive)).put("archive_url",address);
            new CoreStore(context).install(new CoreRepository.Core(metadata),archive);
            cores.put(metadata); responses.put(address,bytes(archive));
            byte[] rom = new byte[system==2?1024:32768];
            if (system==2) {
                int[] words={0xe3a00301,0xe59f1028,0xe1c010b0,0xe3a00406,0xe3a01e3e,0xe3a02c96,0xe0c010b2,
                        0xe2522001,0x1afffffc,0xe3a0040e,0xe3a0105a,0xe5c01000,0xeafffffe,0x00000403};
                for(int b=0;b<4;++b)rom[b]=(byte)(0xea00003e>>>(b*8));
                for(int i=0;i<words.length;++i)for(int b=0;b<4;++b)rom[0x100+i*4+b]=(byte)(words[i]>>>(b*8));
                rom[0xb2]=(byte)0x96;System.arraycopy("SRAM_V".getBytes(StandardCharsets.US_ASCII),0,rom,0x200,6);
            } else {
                rom[0x100]=(byte)0xc3;rom[0x101]=0x50;rom[0x102]=1;rom[0x150]=0x18;rom[0x151]=(byte)0xfe;
                rom[0x143]=system==1?(byte)0x80:0;rom[0x147]=3;rom[0x149]=2;
                int checksum=0;for(int i=0x134;i<=0x14c;++i)checksum=(checksum-(rom[i]&255)-1)&255;rom[0x14d]=(byte)checksum;
            }
            System.arraycopy("CORVUS INTERFACE".getBytes(StandardCharsets.US_ASCII),0,rom,0x240,16);
            byte[] marker=java.util.UUID.randomUUID().toString().getBytes(StandardCharsets.US_ASCII);
            System.arraycopy(marker,0,rom,0x280,marker.length);
            File directory=new File(context.getFilesDir(),"roms");check(directory.isDirectory()||directory.mkdirs(),"private ROM directory");
            File temporary=File.createTempFile("interface-",".part",directory);
            try(FileOutputStream out=new FileOutputStream(temporary)){out.write(rom);out.getFD().sync();}
            StringBuilder hash=new StringBuilder();for(byte value:MessageDigest.getInstance("SHA-256").digest(rom))hash.append(String.format(Locale.ROOT,"%02x",value&255));
            String id=hash+"-"+system;fixtureIds.add(id);
            String title=new String[]{"Interface GB","Interface Cristal","Interface Émeraude"}[system];
            GameLibrary library=new GameLibrary(context);
            library.commit(new GameLibrary.Pending(new GameLibrary.Game(id,title,system,0,CORE_IDS[system],SYSTEMS[system]),temporary),
                    system,CORE_IDS[system],SYSTEMS[system]);library.touch(id);
        }
        byte[] catalog = new JSONObject().put("format",3).put("cores",cores).toString().getBytes(StandardCharsets.UTF_8);
        responses.put(CoreCatalogDiscovery.FALLBACK,catalog);
        responses.put(CoreCatalogDiscovery.DIRECTORY,new JSONArray().put(new JSONObject().put("type","file").put("name","corvuscore.json")
                .put("path","catalog/v1/corvuscore.json").put("size",catalog.length).put("download_url",CoreCatalogDiscovery.FALLBACK))
                .toString().getBytes(StandardCharsets.UTF_8));
        URL.setURLStreamHandlerFactory(protocol -> "https".equals(protocol) ? new URLStreamHandler() {
            @Override protected URLConnection openConnection(URL url) throws IOException {
                byte[] body=responses.get(url.toString());if(body==null)throw new IOException("Unexpected fixture URL");
                if(url.toString().endsWith(".zip"))archiveRequests++;
                return new HttpsURLConnection(url) {
                    @Override public void disconnect() { }
                    @Override public boolean usingProxy(){return false;}
                    @Override public void connect(){ }
                    @Override public String getCipherSuite(){return "TLS_FIXTURE";}
                    @Override public Certificate[] getLocalCertificates(){return null;}
                    @Override public Certificate[] getServerCertificates(){return new Certificate[0];}
                    @Override public int getResponseCode(){return 200;}
                    @Override public InputStream getInputStream(){return new ByteArrayInputStream(body);}
                    @Override public long getContentLengthLong(){return body.length;}
                };
            }
        } : null);
    }
    private File collection(){return new File(getTargetContext().getFilesDir(),"states/library/"+fixtureIds.get(2));}
    private File[] entries(){File[] files=collection().listFiles(f->f.isDirectory()&&f.getName().matches("[1-9][0-9]?\\.entry"));return files==null?new File[0]:files;}
    private boolean named(String value) throws Exception {
        for(File entry:entries())if(new String(bytes(new File(entry,"meta")),StandardCharsets.UTF_8).contains(value))return true;
        return false;
    }
    private void coreOverflow(int row) throws Exception {
        int focus=number("focus");for(int n=focus;n<row;++n)key(KeyEvent.KEYCODE_DPAD_DOWN);
        for(int n=focus;n>row;--n)key(KeyEvent.KEYCODE_DPAD_UP);
        waitFor(()->number("focus")==row,"focus core detail target");
        float y=Math.min(64+row*78,height()-76-68)+34;
        tap(width()-35,y);page(43);
    }
    private void runFlow() throws Exception {
        prepare();Context context=getTargetContext();
        activity=(MainActivity)startActivitySync(new Intent(context,MainActivity.class).addFlags(Intent.FLAG_ACTIVITY_NEW_TASK));
        runOnMainSync(()->activity.setRequestedOrientation(landscape?android.content.pm.ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE:
                android.content.pm.ActivityInfo.SCREEN_ORIENTATION_PORTRAIT));
        waitFor(()->activity.safeWindowArea()!=null && (width()>height())==landscape,"safe requested orientation");
        originalWidth=width();originalHeight=height();
        state(0,-1,0,0);shot("01-home");
        tab(1,25);shot("02-library-grid");
        tap(width()-74,28);editing(3,0);
        sendStringSync("Interface");editing(3,9);key(KeyEvent.KEYCODE_ENTER);editing(0,9);normalViewport();shot("03-library-search");
        tap(width()-37,126);tap(width()-74,28);editing(3,0);long old=ticket();
        tap(width()-37,126);tap(width()-74,28);editing(3,0);long current=ticket();check(current!=old,"search reopening creates a fresh generation");
        MainActivity.textInputResult(-3,"Interface",true,current);editing(3,9);
        MainActivity.textInputResult(3,"",false,old);staysEditing(3,9);
        MainActivity.textInputResult(-3,"obsolete",true,old);staysEditing(3,9);
        imeSetText("Interface");imeAction(true);editing(0,9);normalViewport();tap(width()-37,126);
        tab(2,39);shot("07-saves-browser");
        tab(3,3);shot("04-cores");coreOverflow(2);shot("06-core-installed");back(3);
        choose(3);page(16);waitFor(()->!stateLine().contains("installing=1"),"catalogue available");shot("05-core-catalogue");back(3);
        choose(2);state(0,2,0,0);
        tab(1,25);float chipWidth=(width()-56)/4;tap(16+(chipWidth+8)+chipWidth/2,79);page(25);
        firstLibraryCard();state(25,2,0,0);
        check(number("content")==0,"GB filter cannot launch with a GBA core");
        float chip=(width()-56)/4;tap(16+3*(chip+8)+chip/2,79);
        firstLibraryCard();state(8,2,1,0);waitFor(this::greenPreview,"original ARM game renders actual green framebuffer");shot("11-preserved-game");
        key(KeyEvent.KEYCODE_BACK);state(8,2,1,1);shot("12-quick-menu");
        choose(2);page(40);waitFor(()->entries().length>=1,"native new state is committed");shot("07-state-list");
        choose(1);page(41);waitFor(this::greenPreview,"state action preview displays original game pixels");shot("08-state-actions");
        choose(1);editing(4,-1);long oldRename=ticket();
        key(KeyEvent.KEYCODE_BACK);editing(0,-1);page(41);choose(1);editing(4,-1);long newRename=ticket();
        check(newRename!=oldRename,"rename reopening creates a fresh generation");
        MainActivity.textInputResult(4,"Obsolete rename",true,oldRename);staysEditing(4,-1);
        imeSetText("Étape interface");imeAction(true);page(40);normalViewport();waitFor(()->named("Étape interface"),"native rename reaches durable metadata");
        choose(1);page(41);choose(2);page(40);waitFor(()->entries().length>=2,"duplicate creates an independent state");
        choose(1);page(41);choose(2);page(40);waitFor(()->entries().length>=3,"three displayed states correspond to real files");shot("07-three-real-states");
        choose(1);page(41);
        ActivityMonitor monitor=addMonitor(new IntentFilter(Intent.ACTION_CHOOSER),new ActivityResult(Activity.RESULT_CANCELED,null),true);
        try {choose(3);waitFor(()->monitor.getHits()>0,"share opens the Android chooser");} finally {removeMonitor(monitor);}
        choose(4);page(42);shot("08-delete-confirmation");choose(1);page(41);check(entries().length>=3,"cancel deletion preserves states");
        choose(4);page(42);choose(0);page(40);waitFor(()->entries().length==2,"confirmed deletion removes only selected state");
        choose(1);page(41);choose(0);state(8,2,1,0);waitFor(this::greenPreview,"named state restores actual game framebuffer");
        key(KeyEvent.KEYCODE_BACK);state(8,2,1,1);choose(1);page(40);
        choose(3);page(18);choose(0);state(8,2,1,0);
        File states=new File(context.getFilesDir(),"states");File[] manual=states.listFiles(f->f.getName().startsWith(fixtureIds.get(2)+"-")&&f.getName().endsWith(".state")&&!f.getName().endsWith(".auto.state"));
        check(manual!=null&&manual.length==1,"legacy manual save stays available beside named states");
        key(KeyEvent.KEYCODE_BACK);state(8,2,1,1);choose(5);state(0,2,0,0);
        tab(4,2);shot("09-settings");choose(0);page(10);shot("10-display");back(2);
        choose(2);page(11);choose(1);page(29);choose(5);page(11);
        check(context.getSharedPreferences("corvus",Context.MODE_PRIVATE).getInt("audio_volume",-1)==50,"audio sub-setting persists the selected real volume");back(2);
        choose(3);page(45);choose(0);page(9);choose(0);page(7);choose(2);page(9);tab(0,0);shot("home-asus");
        tab(4,2);choose(3);page(45);choose(0);page(9);choose(0);page(7);choose(1);page(9);tab(0,0);shot("home-shield");
        tab(4,2);choose(3);page(45);choose(0);page(9);choose(0);page(7);choose(0);page(9);tab(0,0);
        tab(3,3);coreOverflow(2);choose(2);page(38);choose(0);state(3,-1,0,0);
        waitFor(()->!new CoreStore(context).canRemove("corvus.gba")&&number("installing")==0,"core uninstall is confirmed without deleting games");
        check(new GameLibrary(context).find(fixtureIds.get(2))!=null&&entries().length==2,"core removal preserves real games and named states");
        choose(2);page(16);choose(3);page(43);shot("05-core-not-installed");
        int requests=archiveRequests;choose(0);waitFor(()->new CoreStore(context).canRemove("corvus.gba")&&number("installing")==0,"individual verified archive install completes");
        check(archiveRequests>requests,"installer consumes the deterministic official-origin archive bytes");shot("06-core-reinstalled");
        requests=archiveRequests;final int beforeReinstall=requests;choose(1);
        waitFor(()->archiveRequests>beforeReinstall&&number("installing")==0,"explicit reinstall uses the existing verified installer");
        check(entries().length==2&&new GameLibrary(context).find(fixtureIds.get(2))!=null,"install and reinstall retain stable game and state identities");
    }
    private void restore() throws Exception {
        if(activity!=null) {runOnMainSync(activity::finish);waitFor(activity::isDestroyed,"native Activity is fully destroyed before fixture cleanup");}
        SharedPreferences.Editor edit=getTargetContext().getSharedPreferences("corvus",Context.MODE_PRIVATE).edit();
        if(oldGames!=null)edit.putString("games",oldGames);
        for(Map.Entry<String,Object> item:oldPreferences.entrySet()) {
            edit.remove(item.getKey());Object value=item.getValue();
            if(value instanceof String)edit.putString(item.getKey(),(String)value);
            else if(value instanceof Boolean)edit.putBoolean(item.getKey(),(Boolean)value);
            else if(value instanceof Integer)edit.putInt(item.getKey(),(Integer)value);
        }
        check(edit.commit(),"restore original game index and only preferences changed by qualification");
        File files=getTargetContext().getFilesDir();
        for(String id:fixtureIds) {
            deleteOwned(new File(files,"roms/"+id+".rom"));
            deleteOwned(new File(files,"states/library/"+id));
            for(String area:new String[]{"saves","states"}) {
                File[] owned=new File(files,area).listFiles(f->f.getName().startsWith(id+".")||f.getName().startsWith(id+"-"));
                if(owned!=null)for(File file:owned)deleteOwned(file);
            }
        }
    }
    private void deleteOwned(File file) throws Exception {
        if(!file.exists())return;
        if(file.isDirectory()&&!java.nio.file.Files.isSymbolicLink(file.toPath())) {
            File[] children=file.listFiles();check(children!=null,"owned fixture directory is readable");
            for(File child:children)deleteOwned(child);
        }
        check(file.delete(),"remove only this run's isolated fixture bytes");
    }
    @Override public void onCreate(Bundle args) {
        super.onCreate(args);landscape="landscape".equals(args.getString("orientation","portrait"));
        fixtures=new File(args.getString("fixture_dir",""));start();
    }
    @Override public void onStart() {
        Bundle result=new Bundle();
        try {runFlow();restore();result.putString("stream","PASS "+checks+" native interface Android checks "+(landscape?"landscape":"portrait")+"\n");finish(Activity.RESULT_OK,result);}
        catch(Throwable failure) {
            try {if(activity!=null)shot("failure");} catch(Throwable capture){failure.addSuppressed(capture);}
            try {restore();} catch(Throwable cleanup){failure.addSuppressed(cleanup);}
            result.putString("stream","FAIL "+failure+"\n"+android.util.Log.getStackTraceString(failure));finish(Activity.RESULT_CANCELED,result);
        }
    }
}
