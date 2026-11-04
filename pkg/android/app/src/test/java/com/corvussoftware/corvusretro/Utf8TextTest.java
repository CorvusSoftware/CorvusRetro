// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
package com.corvussoftware.corvusretro;

import org.junit.Test;
import static org.junit.Assert.*;

public final class Utf8TextTest {
    @Test public void codeAllowsDeviceLineSeparators() {
        String code="32000000 0042\n32000001\t0033\r\n";
        assertNull(Utf8Text.filter(code,0,code.length(),"",0,0,4096,true));
    }
    @Test public void oversizedPasteNeverTurnsIntoAnotherValidCode() {
        String text="32000000 0042\n32000001 0033";
        assertEquals("",Utf8Text.filter(text,0,text.length(),"",0,0,13,true));
        assertEquals("",Utf8Text.filter("1",0,1,"a".repeat(4096),4096,4096,4096,true));
    }
    @Test public void invalidCodeCharactersAreRejectedWhole() {
        for(String code:new String[]{"32000000\u0000 0042","32000000\ud800 0042","32000000\udc00 0042"})
            assertEquals("",Utf8Text.filter(code,0,code.length(),"",0,0,4096,true));
    }
    @Test public void rejectedReplacementPreservesSelectedCode() {
        String original="123-456-789";
        assertEquals("-789",Utf8Text.filter("A".repeat(4097),0,4097,original,7,11,4096,true).toString());
        assertEquals("-789",Utf8Text.filter("\u0000",0,1,original,7,11,4096,true).toString());
        assertEquals("-789",Utf8Text.filter("\ud800",0,1,original,7,11,4096,true).toString());
    }
    @Test public void namesKeepExistingUnicodeByteLimit() {
        assertNull(Utf8Text.filter("PV 🐦",0,5,"",0,0,127));
        assertEquals("é",Utf8Text.filter("é🐦",0,3,"",0,0,5));
        assertEquals("PV",Utf8Text.filter("P\nV",0,3,"",0,0,127));
    }
    @Test public void replacementAndDeletionRespectRemainingBytes() {
        assertNull(Utf8Text.filter("AB",0,2,"XYZZ",0,2,4,true));
        assertNull(Utf8Text.filter("",0,0,"32000000 0042",0,13,4096,true));
    }
}
