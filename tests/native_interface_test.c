// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
#define _POSIX_C_SOURCE 200809L
#define _XOPEN_SOURCE 700
#include "menu_driver.h"
#include "interface_layout.h"
#include "core_registry.h"
#include "state_store.h"
#include <ftw.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

enum { GAME_A, GAME_B, GAME_GB, GAME_GBC, GAME_COUNT };
typedef struct {
    cr_game records[GAME_COUNT];
    int picks, refreshes, installs, edit_field;
    char installed[96], shared_name[96];
    uint8_t *shared;
    size_t shared_size;
} fixture;
static unsigned checks;
static cr_ui *current;
static void check(bool ok, const char *message) {
    if (!ok) {
        fprintf(stderr, "FAIL interface: %s (page=%d, message=%s, engine=%s)\n", message,
                current ? current->page : -1, current ? current->message : "",
                current ? current->engine.error : "");
        exit(1);
    }
    ++checks;
}
static cr_game *games(void *context, size_t *count) {
    fixture *f = context;
    cr_game *copy = malloc(sizeof(f->records));
    check(copy != NULL, "allocate an owned library snapshot");
    memcpy(copy, f->records, sizeof(f->records)); *count = GAME_COUNT;
    return copy;
}
static void touch(void *context, const char *id) {
    fixture *f = context;
    for (size_t i = 0; i < GAME_COUNT; ++i)
        if (!strcmp(f->records[i].id, id)) ++f->records[i].played;
}
static void pick(void *context, int system) { (void)system; ++((fixture *)context)->picks; }
static void edit(void *context, int field, const char *value, bool secret) {
    (void)value;
    check(!secret, "library search and state names are ordinary text");
    ((fixture *)context)->edit_field = field;
}
static void refresh(void *context) { ++((fixture *)context)->refreshes; }
static void install(void *context, const char *id) {
    fixture *f = context; ++f->installs; snprintf(f->installed, sizeof(f->installed), "%s", id);
}
static void share(void *context, const char *name, const uint8_t *bytes, size_t size) {
    fixture *f = context;
    free(f->shared); f->shared = malloc(size);
    check(f->shared && size && size <= CR_STATE_STORE_MAX_BYTES, "share receives a bounded real snapshot");
    memcpy(f->shared, bytes, size); f->shared_size = size;
    snprintf(f->shared_name, sizeof(f->shared_name), "%s", name);
}
typedef struct { char text[16384]; } capture;
static void rect(void *c,float x,float y,float w,float h,uint32_t color) {
    (void)c;(void)x;(void)y;(void)w;(void)h;(void)color;
}
static void circle(void *c,float x,float y,float radius,uint32_t color) {
    (void)c;(void)x;(void)y;(void)radius;(void)color;
}
static void text(void *context,const char *value,float x,float y,float size,uint32_t color,float max) {
    (void)x;(void)y;(void)size;(void)color;(void)max;
    capture *c = context; size_t used = strlen(c->text);
    snprintf(c->text + used, sizeof(c->text) - used, "%s\n", value);
}
static float measure(void *c,const char *value,float size) { (void)c;return strlen(value)*size*.5f; }
static void icon(void *c,int kind,float x,float y,float size,uint32_t color) {
    (void)c;(void)kind;(void)x;(void)y;(void)size;(void)color;
}
static void video(void *c,const uint32_t *pixels,int width,int height,float x,float y,float w,float h) {
    (void)c;(void)pixels;(void)width;(void)height;(void)x;(void)y;(void)w;(void)h;
}
static void line(void *c,float x,float y,float x2,float y2,float width,uint32_t color) {
    (void)c;(void)x;(void)y;(void)x2;(void)y2;(void)width;(void)color;
}
static capture draw(cr_ui *u) {
    capture c = {{0}};
    cr_painter painter = {.context=&c,.rect=rect,.circle=circle,.text=text,.measure=measure,
                         .icon=icon,.game=video,.line=line};
    cr_ui_draw(u,&painter);return c;
}
static void tap(cr_ui *u,float x,float y) {
    static double time = 1;
    cr_ui_touch_at(u,0,CR_TOUCH_DOWN,x,y,time);
    cr_ui_touch_at(u,0,CR_TOUCH_UP,x,y,time+.05);
    time += .1;
}
static void area(cr_ui *u,cr_menu_rect r) { tap(u,r.x+r.w/2,r.y+r.h/2); }
static void tab(cr_ui *u,int index) { area(u,cr_interface_navigation(u,index)); }
static void row(cr_ui *u,int index) {
    u->focus=index;cr_ui_move(u,0);area(u,cr_interface_row_bounds(u,index));
}
static void quick(cr_ui *u,int action) {
    check(u->page==CR_PLAY && u->quick,"quick action starts in the actual paused panel");
    int index=cr_ui_quick_row(u,action);check(index>=0,"quick action is reachable");row(u,index);
}
static void home(cr_ui *u) {
    if (u->page==CR_PLAY) { if (!u->quick) cr_ui_back(u);quick(u,CR_QUICK_MAIN); }
    else tab(u,0);
    check(u->page==CR_MAIN,"home navigation succeeds");
}
static void state_slot(cr_ui *u,unsigned slot) {
    check(u->page==CR_STATE_LIST,"state selection starts in its real list");
    for (size_t i=0;i<u->saved_state_count;++i) if (u->saved_states[i].slot==slot) {
        row(u,(int)i+1);check(u->page==CR_STATE_ACTIONS,"state card opens its action panel");return;
    }
    check(false,"requested real state remains listed");
}
static void browse_a(cr_ui *u,unsigned slot) {
    home(u);tab(u,2);check(u->page==CR_SAVES_LIBRARY,"Saves tab opens collection");
    row(u,1);check(u->page==CR_STATE_LIST,"Saves game card opens its own states");state_slot(u,slot);
}
static void write_bytes(const char *path,const void *bytes,size_t size) {
    FILE *file=fopen(path,"wb");check(file && fwrite(bytes,1,size,file)==size && !fclose(file),"write original fixture bytes");
}
static void rom(const char *path,unsigned seed) {
    uint8_t bytes[512]={0};
    const uint32_t code[]={0xe59f0010,0xe3a01000|seed,0xe2811001,0xe5c01000,
                           0xeafffffc,0xeafffffe,0x0e000000};
    for (size_t i=0;i<sizeof(code)/sizeof(*code);++i)
        for (unsigned b=0;b<4;++b) bytes[i*4+b]=(uint8_t)(code[i]>>(b*8));
    bytes[0xb2]=0x96;memcpy(bytes+0x100,"SRAM_V",6);write_bytes(path,bytes,sizeof(bytes));
}
static int (*original_info)(corvus_core_info *);
static int older_info(corvus_core_info *info) {
    int ok=original_info(info);if(ok) info->version="0.previous-test";return ok;
}
static void same_session(cr_ui *u,void *handle,const char *id,const uint8_t *expected,size_t size) {
    check(u->engine.handle==handle && u->engine.loaded && u->engine.players==1 &&
          !strcmp(u->engine.id,id),"invalid state selection preserves the current game handle and identity");
    uint8_t *actual=NULL;size_t actual_size=0;
    check(cr_engine_state_copy(&u->engine,&actual,&actual_size) && actual_size==size &&
          !memcmp(actual,expected,size),"invalid state does not alter the current machine");free(actual);
}
static int cleanup(const char *path,const struct stat *status,int type,struct FTW *walk) {
    (void)status;(void)type;(void)walk;return remove(path);
}
int main(void) {
    char root[]="/tmp/corvus-interface-XXXXXX",path[4096];fixture f={0};
    check(mkdtemp(root)!=NULL,"private integration fixture");
    int gba=cr_registry_find("corvus.gba"),gb=cr_registry_find("corvus.gb"),gbc=cr_registry_find("corvus.gbc");
    check(gba>=0 && gb>=0 && gbc>=0,"host regression catalog contains the three official test modules");
    const char *titles[]={"Pokémon Émeraude","Partie de contrôle","Rêve en poche","Cristal"};
    const char *core_ids[]={"corvus.gba","corvus.gba","corvus.gb","corvus.gbc"};
    const char *system_ids[]={"nintendo.game_boy_advance","nintendo.game_boy_advance","nintendo.game_boy","nintendo.game_boy_color"};
    int systems[]={gba,gba,gb,-1};
    for(size_t i=0;i<GAME_COUNT;++i) {
        memset(f.records[i].id,(int)('a'+i),64);
        snprintf(f.records[i].id+64,sizeof(f.records[i].id)-64,"-%d",i<2?2:i==GAME_GB?0:1);
        snprintf(f.records[i].title,sizeof(f.records[i].title),"%s",titles[i]);
        snprintf(f.records[i].core_id,sizeof(f.records[i].core_id),"%s",core_ids[i]);
        snprintf(f.records[i].system_id,sizeof(f.records[i].system_id),"%s",system_ids[i]);
        f.records[i].system=systems[i];f.records[i].played=i<2?100-(int64_t)i:0;
    }
    snprintf(path,sizeof(path),"%s/roms",root);check(!mkdir(path,0700),"private ROM directory");
    for(unsigned i=0;i<2;++i) {snprintf(path,sizeof(path),"%s/roms/%s.rom",root,f.records[i].id);rom(path,17+i*25);}
    cr_ui u;current=&u;
    cr_ui_init(&u,(cr_platform){.context=&f,.games=games,.touch=touch,.pick=pick,.edit_text=edit,
        .refresh_core_catalog=refresh,.install_core_pack=install,.share_state=share},root);
    cr_ui_size(&u,384,720);
    const int pages[]={CR_MAIN,CR_LIBRARY,CR_SAVES_LIBRARY,CR_CORES,CR_SETTINGS};
    for(int i=0;i<5;++i) {tab(&u,i);check(u.page==pages[i] && u.tab==i,"all five visible tabs navigate to their actual roots");}
    tab(&u,1);row(&u,1);check(!u.engine.loaded && strstr(u.message,"cœur"),"game card cannot launch without a selected core");
    row(&u,0);row(&u,0);check(!f.picks && !u.importing,"file import remains blocked without a core");cr_ui_back(&u);
    check(u.page==CR_LIBRARY,"back restores the real library");
    area(&u,cr_interface_chip(&u,1));capture c=draw(&u);
    check(strstr(c.text,titles[GAME_GB]) && !strstr(c.text,titles[GAME_A]),"GB chip filters by permanent system identity");
    area(&u,cr_interface_chip(&u,2));c=draw(&u);
    check(strstr(c.text,titles[GAME_GBC]) && !strstr(c.text,titles[GAME_A]),"GBC game remains filterable after its module disappears");
    area(&u,cr_interface_chip(&u,3));c=draw(&u);
    check(strstr(c.text,titles[GAME_A]) && strstr(c.text,titles[GAME_B]) && !strstr(c.text,titles[GAME_GBC]),"GBA chip shows only real GBA games");
    area(&u,cr_interface_chip(&u,0));tap(&u,u.width-72,28);
    check(f.edit_field==3 && u.ui_editing==3,"search header opens the native keyboard bridge");
    cr_ui_text_result(&u,-3,"POKÉMON ÉMERAUDE",true);c=draw(&u);
    check(strstr(c.text,titles[GAME_A]) && !strstr(c.text,titles[GAME_B]),"search folds accented Latin capitals and filters real titles");
    cr_ui_text_result(&u,-3,"aucune correspondance",true);c=draw(&u);
    check(strstr(c.text,"Aucun jeu ne correspond") && !strstr(c.text,titles[GAME_A]),"unmatched query renders a real empty result");
    cr_ui_text_result(&u,3,"",false);check(!u.library_query[0] && !u.ui_editing,"cancel restores the prior search");
    cr_ui_text_result(&u,-3,"late",true);check(!u.library_query[0],"a preview after editing ended is ignored");
    cr_menu_rect search=cr_interface_search(&u);tap(&u,search.x+search.w-22,search.y+22);
    check(!u.library_search && !u.library_query[0],"clear closes the native search field");
    tab(&u,3);row(&u,(int)cr_registry_count());check(u.page==CR_CORE_DOWNLOADS && f.refreshes==1,"core manager opens the actual catalog service");
    cr_core_catalog catalog={.count=1};cr_core_pack *pack=&catalog.packs[0];
    snprintf(pack->id,sizeof(pack->id),"test.gba");snprintf(pack->core_id,sizeof(pack->core_id),"corvus.gba");
    snprintf(pack->name,sizeof(pack->name),"CorvusGBA test");snprintf(pack->version,sizeof(pack->version),"1.5.0");
    snprintf(pack->abi,sizeof(pack->abi),"x86_64");snprintf(pack->description,sizeof(pack->description),"Archive de test vérifiée");
    snprintf(pack->system_name,sizeof(pack->system_name),"Game Boy Advance");pack->size=65536;
    cr_ui_catalog(&u,&catalog);row(&u,1);check(u.page==CR_CORE_DETAILS,"catalog item opens its actual detail screen");
    c=draw(&u);check(strstr(c.text,"Game Boy Advance") && strstr(c.text,"Archive de test vérifiée"),"core detail displays provider metadata");
    row(&u,1);check(f.installs==1 && !strcmp(f.installed,pack->id),"reinstall uses the verified installer identifier");
    cr_ui_install_status(&u,"Test terminé",true);cr_ui_back(&u);tab(&u,3);
    int gba_row=-1;for(size_t i=0;i<cr_registry_count();++i) if(cr_registry_index_at(i)==gba) gba_row=(int)i;
    check(gba_row>=0,"GBA is reachable in the real core manager");row(&u,gba_row);
    check(u.engine.handle && !u.engine.loaded && u.page==CR_MAIN,"explicit core selection loads no game");
    row(&u,0);check(cr_ui_running(&u) && !strcmp(u.engine.id,f.records[GAME_A].id),"continue launches the latest real game with the selected core");
    check(cr_engine_step(&u.engine,0),"original ARM fixture renders its first frame");
    cr_ui_back(&u);quick(&u,CR_QUICK_SAVE_STATE);
    check(u.page==CR_STATE_LIST && u.saved_state_count==1,"quick save creates an actual named entry");
    cr_state_entry first=u.saved_states[0];uint8_t *saved=NULL;size_t saved_size=0;char error[256];
    check(cr_state_store_read(root,&first,&saved,&saved_size,error,sizeof(error)),"UI-created entry contains real SDK bytes");
    state_slot(&u,first.slot);row(&u,1);check(f.edit_field==4 && u.ui_editing==4,"rename action opens its own keyboard field");
    cr_ui_text_result(&u,-4,"Avant le pont",true);cr_ui_text_result(&u,4,"Avant le pont",true);
    check(u.page==CR_STATE_LIST && u.saved_state_count==1 && !strcmp(u.saved_states[0].name,"Avant le pont"),"accepted rename updates the real collection");
    first=u.saved_states[0];state_slot(&u,first.slot);row(&u,2);
    check(u.page==CR_STATE_LIST && u.saved_state_count==2,"duplicate creates another actual entry");
    cr_state_entry duplicate=u.saved_states[0];check(duplicate.slot!=first.slot,"duplicate owns a distinct slot");
    state_slot(&u,duplicate.slot);row(&u,3);
    check(f.shared_size==saved_size && !memcmp(f.shared,saved,saved_size) && strstr(f.shared_name,"copie"),"share action passes the exact duplicated SDK snapshot");
    row(&u,4);check(u.page==CR_STATE_DELETE_CONFIRM,"delete requires the visible confirmation");row(&u,1);
    check(u.page==CR_STATE_ACTIONS && u.saved_state_count==2,"cancel leaves both states intact");
    row(&u,4);row(&u,0);check(u.page==CR_STATE_LIST && u.saved_state_count==1,"confirmed deletion refreshes the actual collection");
    cr_ui_back(&u);check(u.page==CR_PLAY && u.quick,"back returns to the paused game panel");
    void *retained=u.engine.handle;quick(&u,CR_QUICK_CLOSE);
    check(u.page==CR_MAIN && u.engine.handle==retained && !u.engine.loaded,"closing the game keeps the selected core");
    browse_a(&u,first.slot);row(&u,0);
    check(cr_ui_running(&u) && !strcmp(u.engine.id,first.game_id),"Saves restores a state after the game's session was closed");
    uint8_t *actual=NULL;size_t actual_size=0;
    check(cr_engine_state_copy(&u.engine,&actual,&actual_size) && actual_size==saved_size && !memcmp(actual,saved,saved_size),"restored UI session matches the saved machine exactly");free(actual);
    original_info=u.engine.api.get_info;u.engine.api.get_info=older_info;cr_state_entry older;
    bool made_old=cr_state_store_create(&u.engine,"Ancienne version",&older);u.engine.api.get_info=original_info;
    check(made_old,"create a genuine prior-version metadata fixture");
    home(&u);tab(&u,1);row(&u,2);check(cr_ui_running(&u) && !strcmp(u.engine.id,f.records[GAME_B].id),"second original game is now the active session");
    check(cr_engine_step(&u.engine,0),"advance the second game independently");
    void *game_b=u.engine.handle;uint8_t *before=NULL;size_t before_size=0;
    check(cr_engine_state_copy(&u.engine,&before,&before_size),"capture second game's live machine before rejected loads");
    browse_a(&u,older.slot);row(&u,0);same_session(&u,game_b,f.records[GAME_B].id,before,before_size);
    cr_ui_back(&u);state_slot(&u,first.slot);
    snprintf(path,sizeof(path),"%s/states/library/%s/%u.entry/state",root,first.game_id,first.slot);
    uint8_t *bad=malloc(saved_size);check(bad!=NULL,"allocate corruption fixture");memcpy(bad,saved,saved_size);bad[0]^=0x80;
    write_bytes(path,bad,saved_size);free(bad);row(&u,0);same_session(&u,game_b,f.records[GAME_B].id,before,before_size);
    check(!unlink(path),"remove only the selected test snapshot");row(&u,0);same_session(&u,game_b,f.records[GAME_B].id,before,before_size);
    write_bytes(path,saved,saved_size);row(&u,0);
    check(cr_ui_running(&u) && !strcmp(u.engine.id,first.game_id),"valid state remains loadable after storage is repaired");
    free(before);free(saved);free(f.shared);cr_ui_destroy(&u);current=NULL;
    check(!nftw(root,cleanup,16,FTW_DEPTH|FTW_PHYS),"remove only owned original test data");
    printf("PASS %u native interface navigation, search, state-action and preservation checks\n",checks);
    return 0;
}
