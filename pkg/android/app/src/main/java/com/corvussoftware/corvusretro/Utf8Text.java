// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
package com.corvussoftware.corvusretro;

import java.nio.charset.StandardCharsets;

/** Byte-aware IME limits matching the native UTF-8 buffers. */
final class Utf8Text {
    private Utf8Text() { }
    static CharSequence filter(CharSequence source, int start, int end, CharSequence destination,
                               int dstart, int dend, int maximum) {
        int occupied = (destination.subSequence(0,dstart).toString()
                + destination.subSequence(dend,destination.length())).getBytes(StandardCharsets.UTF_8).length;
        int remaining = Math.max(0,maximum-occupied);
        StringBuilder accepted = new StringBuilder();
        for (int at=start;at<end;) {
            char first=source.charAt(at);
            int cp=first, count=1;
            if (Character.isHighSurrogate(first)) {
                if (at+1>=end || !Character.isLowSurrogate(source.charAt(at+1))) { at++; continue; }
                cp=Character.toCodePoint(first,source.charAt(at+1)); count=2;
            } else if (Character.isLowSurrogate(first)) { at++; continue; }
            at+=count;
            if (Character.isISOControl(cp)) continue;
            int bytes=cp<0x80 ? 1 : cp<0x800 ? 2 : cp<0x10000 ? 3 : 4;
            if (bytes>remaining) break;
            accepted.appendCodePoint(cp); remaining-=bytes;
        }
        String text=accepted.toString();
        return text.contentEquals(source.subSequence(start,end)) ? null : text;
    }
}
