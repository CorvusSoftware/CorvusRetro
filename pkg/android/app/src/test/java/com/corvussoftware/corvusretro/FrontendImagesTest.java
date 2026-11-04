// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
package com.corvussoftware.corvusretro;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.TemporaryFolder;
import static org.junit.Assert.*;

public final class FrontendImagesTest {
    @Rule public TemporaryFolder temporary=new TemporaryFolder();
    private static final String GAME="a".repeat(64)+"-2";
    private interface Read { void run() throws IOException; }
    private static void rejects(Read read) throws IOException {
        try { read.run();fail("Untrusted image input accepted"); }
        catch (IOException expected) { }
    }
    private static byte[] ppm(String dimensions,byte[] pixels) throws IOException {
        ByteArrayOutputStream bytes=new ByteArrayOutputStream();
        bytes.write(("P6\n"+dimensions+"\n255\n").getBytes(StandardCharsets.US_ASCII));
        bytes.write(pixels);return bytes.toByteArray();
    }
    @Test public void nativePreviewKeepsRgbChannelsAndBinaryWhitespace() throws Exception {
        FrontendImageFiles.Pixels pixels=FrontendImageFiles.ppm(ppm("2 1",new byte[]{10,32,13,(byte)255,0,(byte)128}));
        assertEquals(2,pixels.width);assertEquals(1,pixels.height);
        assertArrayEquals(new int[]{0xff0a200d,0xffff0080},pixels.argb);
        rejects(()->FrontendImageFiles.ppm(ppm("2 1",new byte[5])));
        rejects(()->FrontendImageFiles.ppm(ppm("2 1",new byte[7])));
        rejects(()->FrontendImageFiles.ppm(ppm("385 1",new byte[385*3])));
        rejects(()->FrontendImageFiles.ppm(ppm("1 257",new byte[257*3])));
        rejects(()->FrontendImageFiles.ppm(ppm("-1 1",new byte[3])));
        rejects(()->FrontendImageFiles.ppm("P6\n1 1\n256\nabc".getBytes(StandardCharsets.US_ASCII)));
    }
    @Test public void keysAndPrivatePreviewPathsRejectTraversalAndSymlinks() throws Exception {
        String key="state:"+GAME+":1:0123456789abcdef";
        assertTrue(FrontendImageFiles.allowed(key));assertTrue(FrontendImageFiles.allowed("asset:home"));
        assertEquals(GAME,FrontendImageFiles.coverGame("cover:"+GAME));
        for (String invalid:new String[]{null,"asset:anything","cover:../state",key.replace(":1:",":0:"),
                key.replace(":1:",":33:"),key+"/../x",key.replace("0123456789abcdef","ABCDEF0123456789")})
            assertFalse(FrontendImageFiles.allowed(invalid));
        File root=temporary.newFolder("private");File entry=new File(root,"states/library/"+GAME+"/1.entry");
        assertTrue(entry.mkdirs());File image=new File(entry,"preview.ppm");
        Files.write(image.toPath(),ppm("1 1",new byte[]{1,2,3}));
        assertEquals(image.getCanonicalFile(),FrontendImageFiles.preview(root,key));
        assertTrue(image.delete());File other=temporary.newFile("external.ppm");
        Files.write(other.toPath(),ppm("1 1",new byte[]{1,2,3}));
        Files.createSymbolicLink(image.toPath(),other.toPath());
        rejects(()->FrontendImageFiles.preview(root,key));
    }
    @Test public void imeByteLimitsKeepCompleteUnicodeAndReplacementCapacity() {
        assertNull(Utf8Text.filter("Pokémon",0,7,"",0,0,127));
        String base="a".repeat(124),emoji="\ud83d\udc26";
        assertEquals("",Utf8Text.filter(emoji,0,2,base,base.length(),base.length(),127));
        assertNull(Utf8Text.filter(emoji,0,2,base,120,124,127));
        assertEquals("AÉ",Utf8Text.filter("AÉ🦅",0,4,"",0,0,4));
        assertEquals("ab",Utf8Text.filter("a\nb",0,3,"",0,0,95));
        assertEquals("",Utf8Text.filter("\ud83d",0,1,"",0,0,95));
    }
}
