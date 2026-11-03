// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
package com.corvussoftware.corvusretro;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.LinkOption;
import java.text.Normalizer;
import java.util.Arrays;
import java.util.Comparator;
import java.util.UUID;
import java.util.function.BooleanSupplier;

/** Private, bounded data exports. Tokens never resolve outside this cache directory. */
final class StateShareFiles {
    static final int MAX_BYTES=16*1024*1024;
    private static final int MAX_FILES=4;
    private StateShareFiles() { }
    static String name(String title) {
        String text=Normalizer.normalize(title==null ? "" : title,Normalizer.Form.NFC)
                .replaceAll("[^\\p{L}\\p{N} _.()-]","_").trim();
        if (text.endsWith(".state")) text=text.substring(0,text.length()-6);
        if (text.isEmpty() || text.equals(".") || text.equals("..")) text="CorvusRetro";
        int end=Math.min(text.length(),72);
        if (end<text.length() && Character.isHighSurrogate(text.charAt(end-1))) end--;
        return text.substring(0,end)+".state";
    }
    private static boolean valid(String token) {
        return token!=null && token.length()<=120
                && token.matches("[0-9a-f]{32}-[\\p{L}\\p{N} _.()-]{1,78}\\.state");
    }
    private static File directory(File cache) throws IOException {
        File base=cache.getCanonicalFile();
        File directory=new File(base,"state-share");
        if ((!directory.isDirectory() && !directory.mkdirs())
                || !directory.getCanonicalFile().equals(directory)) throw new IOException("Share cache unavailable");
        return directory;
    }
    static synchronized File stage(File cache,String title,byte[] bytes,BooleanSupplier active) throws IOException {
        if (bytes==null || bytes.length==0 || bytes.length>MAX_BYTES || !active.getAsBoolean())
            throw new IOException("Invalid state export");
        File directory=directory(cache);
        File[] previous=directory.listFiles((folder,file)->valid(file));
        if (previous==null) throw new IOException("Share cache unavailable");
        Arrays.sort(previous,Comparator.comparingLong(File::lastModified));
        for (int i=0;i<=previous.length-MAX_FILES;i++)
            if (!previous[i].delete()) throw new IOException("Previous state export unavailable");
        File file=new File(directory,UUID.randomUUID().toString().replace("-","")+"-"+name(title));
        if (!file.createNewFile()) throw new IOException("State export collision");
        boolean complete=false;
        try {
            try (FileOutputStream output=new FileOutputStream(file)) {
                for (int at=0;at<bytes.length;at+=32768) {
                    if (!active.getAsBoolean()) throw new IOException("State export cancelled");
                    output.write(bytes,at,Math.min(32768,bytes.length-at));
                }
                output.getFD().sync();
            }
            if (!file.setReadOnly()) throw new IOException("State export protection unavailable");
            complete=true;return file;
        } finally { if (!complete) file.delete(); }
    }
    static File resolve(File cache,String token) throws IOException {
        if (!valid(token)) throw new IOException("Invalid state share token");
        File directory=directory(cache),file=new File(directory,token);
        if (!file.getCanonicalFile().equals(file) || !Files.isRegularFile(file.toPath(),LinkOption.NOFOLLOW_LINKS)
                || file.length()<=0 || file.length()>MAX_BYTES) throw new IOException("State share unavailable");
        return file;
    }
    static String displayName(String token) throws IOException {
        if (!valid(token)) throw new IOException("Invalid state share token");
        return token.substring(33);
    }
}
