// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
package com.corvussoftware.corvusretro;

import java.io.File;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.LinkOption;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/** Strict names and P6 decoding for private native state previews. */
final class FrontendImageFiles {
    static final String GAME = "[0-9a-f]{64}-(?:[0-9]|[12][0-9]|3[01]|c[0-9a-f]{16})";
    private static final Pattern COVER = Pattern.compile("cover:("+GAME+")");
    private static final Pattern STATE = Pattern.compile("state:("+GAME+"):([1-9]|[12][0-9]|3[0-2]):[0-9a-f]{16}");
    static final int MAX_PPM_BYTES = 384*256*3+64;
    private FrontendImageFiles() { }
    static String coverGame(String key) {
        Matcher match=COVER.matcher(key==null ? "" : key);
        return match.matches() ? match.group(1) : null;
    }
    static boolean allowed(String key) {
        return "asset:home".equals(key) || coverGame(key)!=null || STATE.matcher(key==null ? "" : key).matches();
    }
    static File preview(File files, String key) throws IOException {
        Matcher match=STATE.matcher(key==null ? "" : key);
        if (!match.matches()) throw new IOException("Invalid state preview key");
        File base=new File(files.getCanonicalFile(),"states/library");
        File directory=new File(new File(base,match.group(1)),match.group(2)+".entry");
        File image=new File(directory,"preview.ppm");
        File canonical=image.getCanonicalFile();
        if (!canonical.getPath().startsWith(files.getCanonicalPath()+File.separator)
                || !canonical.equals(image.getAbsoluteFile())
                || !Files.isRegularFile(image.toPath(),LinkOption.NOFOLLOW_LINKS)
                || image.length()<=0 || image.length()>MAX_PPM_BYTES)
            throw new IOException("Private preview unavailable");
        return image;
    }
    static final class Pixels {
        final int width,height;
        final int[] argb;
        Pixels(int width,int height,int[] argb) { this.width=width;this.height=height;this.argb=argb; }
    }
    static Pixels ppm(byte[] data) throws IOException {
        if (data==null || data.length>MAX_PPM_BYTES) throw new IOException("Preview byte limit");
        int at=0;
        String[] tokens=new String[4];
        for (int field=0;field<4;field++) {
            while (at<data.length && whitespace(data[at])) at++;
            int begin=at;
            while (at<data.length && !whitespace(data[at]) && at<64) at++;
            if (at==begin || at>=64 || at>=data.length) throw new IOException("Malformed P6 header");
            tokens[field]=new String(data,begin,at-begin,java.nio.charset.StandardCharsets.US_ASCII);
        }
        if (!"P6".equals(tokens[0]) || !"255".equals(tokens[3])) throw new IOException("Unsupported preview format");
        int width,height;
        try {
            if (!tokens[1].matches("[1-9][0-9]{0,2}") || !tokens[2].matches("[1-9][0-9]{0,2}")) throw new NumberFormatException();
            width=Integer.parseInt(tokens[1]);height=Integer.parseInt(tokens[2]);
        } catch (NumberFormatException malformed) { throw new IOException("Malformed preview dimensions"); }
        if (width>384 || height>256) throw new IOException("Preview dimensions limit");
        if (data[at]=='\r' && at+1<data.length && data[at+1]=='\n') at+=2;
        else at++;
        if (data.length-at!=width*height*3) throw new IOException("Truncated or trailing preview data");
        int[] pixels=new int[width*height];
        for (int i=0;i<pixels.length;i++) {
            int r=data[at++]&255,g=data[at++]&255,b=data[at++]&255;
            pixels[i]=0xff000000 | r<<16 | g<<8 | b;
        }
        return new Pixels(width,height,pixels);
    }
    private static boolean whitespace(byte value) { return value==' ' || value=='\n' || value=='\r' || value=='\t'; }
}
