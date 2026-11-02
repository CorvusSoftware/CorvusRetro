// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
#define _POSIX_C_SOURCE 200809L
#include "menu_driver.h"
#include "core_registry.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
typedef struct { bool removable[3]; unsigned calls; char id[96]; } fixture;
static void check(bool ok,const char *message) {
    if (!ok) { fprintf(stderr,"FAIL removal: %s\n",message); exit(1); }
}
static bool can_remove(void *context,const char *id) {
    fixture *f=context;
    int slot=cr_registry_find(id);
    return slot>=0 && slot<3 && f->removable[slot];
}
static void remove_core(void *context,const char *id) {
    fixture *f=context;
    check(can_remove(context,id),"only an installed downloadable ID reaches the service");
    ++f->calls; snprintf(f->id,sizeof(f->id),"%s",id);
    f->removable[cr_registry_find(id)]=false;
}
static void choose(cr_ui *u,int row) { u->focus=row; cr_ui_move(u,0); cr_ui_confirm(u); }
static void manager(cr_ui *u) {
    u->page=CR_CORES; u->quick=false; u->depth=0;
    choose(u,(int)cr_registry_count()+1);
    check(u->page==CR_REMOVE_CORES,"visible core-list entry opens downloaded-core manager");
}
static void original_gba(const char *path) {
    uint8_t bytes[512]={0};
    const uint32_t words[]={0xe59f0008,0xe3a0105a,0xe5c01000,0xeafffffe,0x0e000000};
    for (size_t i=0;i<sizeof(words)/sizeof(*words);++i)
        for (unsigned b=0;b<4;++b) bytes[i*4+b]=(uint8_t)(words[i]>>(b*8));
    bytes[0xb2]=0x96; memcpy(bytes+0x100,"SRAM_V",6);
    FILE *file=fopen(path,"wb");
    check(file && fwrite(bytes,1,sizeof(bytes),file)==sizeof(bytes) && !fclose(file),"original GBA battery fixture");
}
int main(void) {
    char root[]="/tmp/corvus-remove-XXXXXX",rom[4096],saves[4096],saved_directory[4096],battery[4096];
    check(mkdtemp(root)!=NULL,"private temporary storage");
    snprintf(rom,sizeof(rom),"%s/original.gba",root); original_gba(rom);
    fixture f={{true,true,true},0,{0}}; cr_ui u;
    cr_ui_init(&u,(cr_platform){.context=&f,.can_remove_core=can_remove,.remove_core=remove_core},root);
    const char *id="dddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd-2";
    check(cr_engine_open(&u.engine,2,id,rom) && cr_engine_step(&u.engine,0),"load original GBA game");
    void *handle=u.engine.handle;
    u.page=CR_INFORMATION; choose(&u,3);
    check(u.page==CR_REMOVE_CORES && u.engine.handle==handle,"core manager is reachable while a game is loaded");
    choose(&u,2);
    check(u.page==CR_REMOVE_CORE_CONFIRM && !f.calls,"removal requires a separate confirmation");
    choose(&u,1);
    check(u.page==CR_REMOVE_CORES && u.focus==2 && u.engine.handle==handle && !f.calls,
          "cancel restores the selected manager row and keeps the game intact");
    choose(&u,2); u.importing=true; choose(&u,0);
    check(!f.calls && u.engine.handle==handle,"pending game import blocks destructive core operation");
    u.importing=false; u.save_request=7; choose(&u,0);
    check(!f.calls && u.engine.handle==handle,"pending save transfer blocks core removal");
    u.save_request=0; cr_ui_back(&u); choose(&u,0); choose(&u,0);
    check(f.calls==1 && !strcmp(f.id,"corvus.gb") && u.engine.handle==handle && u.engine.loaded &&
          cr_engine_step(&u.engine,0),"deleting another core leaves the current engine and input state usable");
    cr_ui_install_status(&u,"Cœur supprimé",true);
    manager(&u); choose(&u,1);
    check(u.page==CR_REMOVE_CORE_CONFIRM && !strcmp(u.remove_core_id,"corvus.gba"),
          "remaining rows retain permanent core identities");
    snprintf(saves,sizeof(saves),"%s/saves",root);
    snprintf(saved_directory,sizeof(saved_directory),"%s/saves-retained",root);
    check(!rename(saves,saved_directory),"retain existing battery directory");
    FILE *block=fopen(saves,"wb"); check(block && !fclose(block),"obstruct battery storage");
    choose(&u,0);
    check(f.calls==1 && u.engine.handle==handle && u.engine.loaded && !u.installing,
          "failed battery write preserves running engine and never starts file deletion");
    check(!unlink(saves) && !rename(saved_directory,saves),"restore battery directory"); choose(&u,0);
    check(f.calls==2 && !strcmp(f.id,"corvus.gba") && !u.engine.handle && u.installing,
          "confirmed loaded core is safely closed before the removal service starts");
    snprintf(battery,sizeof(battery),"%s/saves/%s.sav",root,id);
    struct stat st;
    check(!stat(battery,&st) && st.st_size==32768 && !access(rom,F_OK),
          "raw battery and original game survive downloaded-core removal");
    cr_ui_install_status(&u,"Cœur supprimé",true);
    check(!u.installing,"terminal removal status releases the operation barrier");
    cr_ui_destroy(&u);
    char options[4096]; snprintf(options,sizeof(options),"%s/options",root);
    check(!rmdir(options),"clean empty core options directory");
    check(!unlink(battery) && !rmdir(saves) && !unlink(rom) && !rmdir(root),"clean synthetic fixture");
    puts("PASS native core deletion confirmation, identity, active-engine and battery barriers");
}
