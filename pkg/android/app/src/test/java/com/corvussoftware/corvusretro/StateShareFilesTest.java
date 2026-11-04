// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
package com.corvussoftware.corvusretro;

import java.io.File;
import java.io.IOException;
import java.nio.file.Files;
import java.util.concurrent.atomic.AtomicInteger;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.TemporaryFolder;
import static org.junit.Assert.*;

public final class StateShareFilesTest {
    @Rule public TemporaryFolder temporary=new TemporaryFolder();
    private interface Write { void run() throws IOException; }
    private static void rejects(Write write) throws IOException {
        try { write.run();fail("Unsafe state share accepted"); }
        catch (IOException expected) { }
    }
    @Test public void exportedBytesAreImmutablePrivateAndCacheIsBounded() throws Exception {
        File cache=temporary.newFolder("cache");File latest=null;
        byte[] bytes={1,7,9,(byte)255};
        for (int i=0;i<8;i++) latest=StateShareFiles.stage(cache,"Émeraude / .. état "+i,bytes,()->true);
        assertNotNull(latest);assertArrayEquals(bytes,Files.readAllBytes(latest.toPath()));
        assertEquals(4,new File(cache,"state-share").list().length);
        assertEquals(latest,StateShareFiles.resolve(cache,latest.getName()));
        assertTrue(StateShareFiles.displayName(latest.getName()).startsWith("Émeraude _ .. état"));
        for (String name:new String[]{null,"",latest.getName()+"/../x","../"+latest.getName(),"x.state"})
            rejects(()->StateShareFiles.resolve(cache,name));
    }
    @Test public void invalidOrCancelledExportsNeverLeavePartialPayloads() throws Exception {
        File cache=temporary.newFolder("cache");
        rejects(()->StateShareFiles.stage(cache,"state",new byte[0],()->true));
        rejects(()->StateShareFiles.stage(cache,"state",new byte[StateShareFiles.MAX_BYTES+1],()->true));
        rejects(()->StateShareFiles.stage(cache,"state",new byte[1],()->false));
        AtomicInteger steps=new AtomicInteger();
        rejects(()->StateShareFiles.stage(cache,"state",new byte[128*1024],()->steps.incrementAndGet()<3));
        assertEquals(0,new File(cache,"state-share").list().length);
    }
    @Test public void shareResolutionRefusesSymlinkPayloads() throws Exception {
        File cache=temporary.newFolder("cache");
        File exported=StateShareFiles.stage(cache,"state",new byte[]{42},()->true);
        assertTrue(exported.delete());File foreign=temporary.newFile("other.state");
        Files.write(foreign.toPath(),new byte[]{43});Files.createSymbolicLink(exported.toPath(),foreign.toPath());
        rejects(()->StateShareFiles.resolve(cache,exported.getName()));
    }
}
