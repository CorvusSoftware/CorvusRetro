// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
package com.corvussoftware.corvusretro;

import android.app.Instrumentation;
import android.content.Context;
import android.content.SharedPreferences;
import android.content.pm.ProviderInfo;
import android.database.Cursor;
import android.graphics.Bitmap;
import android.net.Uri;
import android.os.Bundle;
import android.os.ParcelFileDescriptor;
import android.provider.OpenableColumns;
import java.io.File;
import java.io.InputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.security.MessageDigest;
import java.util.Locale;
import java.util.UUID;
import java.util.concurrent.atomic.AtomicInteger;
import org.json.JSONArray;
import org.json.JSONObject;

/** Real Android service boundaries, with original generated data and no network requests. */
public final class FrontendServicesInstrumentation extends Instrumentation {
    private int checks;
    private void check(boolean condition,String message) {
        if (!condition) throw new AssertionError(message);
        checks++;
    }
    @Override public void onCreate(Bundle arguments) { super.onCreate(arguments);start(); }
    @Override public void onStart() {
        Bundle result=new Bundle();
        try {
            runServices();result.putString("stream","PASS "+checks+" Android frontend service checks\n");
            finish(0,result);
        } catch (Throwable failure) {
            result.putString("stream","FAIL "+failure+"\n"+android.util.Log.getStackTraceString(failure));
            finish(1,result);
        }
    }
    private Bitmap awaitImage(FrontendImages images,String key) throws Exception {
        long deadline=android.os.SystemClock.elapsedRealtime()+10000;
        Bitmap bitmap;
        while ((bitmap=images.get(key))==null && android.os.SystemClock.elapsedRealtime()<deadline) Thread.sleep(20);
        check(bitmap!=null,"asynchronous private image becomes available");return bitmap;
    }
    private void runServices() throws Exception {
        Context context=getTargetContext();GameLibrary library=new GameLibrary(context);
        SharedPreferences preferences=library.preferences;
        String oldGames=preferences.getString("games","[]");
        byte[] hash=MessageDigest.getInstance("SHA-256").digest(UUID.randomUUID().toString().getBytes(StandardCharsets.US_ASCII));
        StringBuilder key=new StringBuilder();for (byte value:hash) key.append(String.format(Locale.ROOT,"%02x",value&255));
        String id=key+"-2",coverKey="game_cover_"+id;
        File previewFolder=new File(context.getFilesDir(),"states/library/"+id+"/32.entry");
        File preview=new File(previewFolder,"preview.ppm"),exported=null;
        AtomicInteger published=new AtomicInteger();
        FrontendImages images=new FrontendImages(context,library,published::incrementAndGet);
        try {
            JSONArray records=new JSONArray(oldGames);
            int originalCount=records.length();
            records.put(new JSONObject().put("id",id).put("title","Original services fixture")
                    .put("system",2).put("played",1234).put("coreId","corvus.gba")
                    .put("systemId","nintendo.game_boy_advance"));
            check(preferences.edit().putString("games",records.toString()).commit(),"temporary private library fixture");
            String url="https://media.retroachievements.org/Images/123456.png";
            library.cover(id,url);
            check(url.equals(library.cover(id)),"optional game artwork persists independently of the library index");
            library.touch(id);GameLibrary.Game game=library.find(id);
            check(game!=null && game.played>1234 && game.cover.equals(url)
                    && game.coreId.equals("corvus.gba"),"touch preserves permanent identity and optional artwork");
            check(new JSONArray(preferences.getString("games","[]")).length()==originalCount+1,
                    "existing library records stay present");
            String beforeRejected=preferences.getString("games","[]");
            boolean rejected=false;
            try { library.cover(id,"https://foreign.example/cover.png"); }
            catch (IOException expected) { rejected=true; }
            check(rejected && library.cover(id).equals(url)
                    && preferences.getString("games","[]").equals(beforeRejected),"foreign image leaves identity and metadata unchanged");
            check(preferences.edit().putInt(coverKey,42).commit(),"inject malformed optional metadata");
            check(library.cover(id).isEmpty() && library.find(id)!=null,"malformed optional artwork does not discard the game");
            check(preferences.edit().remove(coverKey).commit(),"remove optional metadata before image test");
            Bitmap hero=awaitImage(images,"asset:home");
            check(hero.getWidth()==512 && hero.getHeight()==224 && hero.getConfig()==Bitmap.Config.ARGB_8888,
                    "original home art is large RGBA artwork compatible with the native texture bound");
            check(hero.getPixel(12,12)!=hero.getPixel(400,180),"home artwork contains actual original illustration pixels");
            check(previewFolder.mkdirs(),"private preview folder");
            try (FileOutputStream out=new FileOutputStream(preview)) {
                out.write("P6\n2 1\n255\n".getBytes(StandardCharsets.US_ASCII));
                out.write(new byte[]{10,32,13,(byte)255,0,(byte)128});
            }
            Bitmap frame=awaitImage(images,"state:"+id+":32:0123456789abcdef");
            check(frame.getWidth()==2 && frame.getHeight()==1 && frame.getPixel(0,0)==0xff0a200d
                    && frame.getPixel(1,0)==0xffff0080,"native P6 preview retains channel order and binary whitespace");
            check(images.get("state:../outside:32:0123456789abcdef")==null,"image service refuses traversal keys before IO");
            check(published.get()>=2,"completed images wake their native consumer");
            byte[] snapshot={3,4,5,6,(byte)255};
            exported=StateShareFiles.stage(context.getCacheDir(),"État original",snapshot,()->true);
            Uri uri=new Uri.Builder().scheme("content").authority(context.getPackageName()+".state-share")
                    .appendPath(exported.getName()).build();
            ProviderInfo provider=context.getPackageManager().resolveContentProvider(uri.getAuthority(),0);
            check(provider!=null && !provider.exported && provider.grantUriPermissions,
                    "state provider is private and supports explicit URI grants");
            ParcelFileDescriptor descriptor=context.getContentResolver().openFileDescriptor(uri,"r");
            check(descriptor!=null,"read-only state URI opens");
            try (InputStream in=new ParcelFileDescriptor.AutoCloseInputStream(descriptor)) {
                for (byte value:snapshot) check(in.read()==(value&255),"state URI delivers exact snapshot bytes");
                check(in.read()==-1,"state URI has no trailing bytes");
            }
            try (Cursor cursor=context.getContentResolver().query(uri,null,null,null,null)) {
                check(cursor!=null && cursor.moveToFirst(),"share metadata query succeeds");
                check(cursor.getString(cursor.getColumnIndexOrThrow(OpenableColumns.DISPLAY_NAME)).equals("État original.state")
                        && cursor.getLong(cursor.getColumnIndexOrThrow(OpenableColumns.SIZE))==snapshot.length,
                        "share metadata reports user-facing name and exact size");
            }
            for (String mode:new String[]{"w","rw","wa"}) {
                rejected=false;
                try (ParcelFileDescriptor ignored=context.getContentResolver().openFileDescriptor(uri,mode)) { }
                catch (IOException | SecurityException expected) { rejected=true; }
                check(rejected,"state provider refuses writable mode "+mode);
            }
            rejected=false;
            Uri traversal=uri.buildUpon().appendPath("..").appendPath("elsewhere").build();
            try (ParcelFileDescriptor ignored=context.getContentResolver().openFileDescriptor(traversal,"r")) { }
            catch (IOException | SecurityException expected) { rejected=true; }
            check(rejected,"provider rejects URI path traversal");
            images.close();check(images.get("asset:home")==null,"closed Activity image service retires cached results");
        } finally {
            images.close();preferences.edit().putString("games",oldGames).remove(coverKey).commit();
            if (exported!=null) exported.delete();
            preview.delete();previewFolder.delete();previewFolder.getParentFile().delete();
        }
    }
}
