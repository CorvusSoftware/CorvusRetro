// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
#define _POSIX_C_SOURCE 200809L
#include "menu_cheat.h"
#include "menu_cbs.h"
#include "core_registry.h"
#include "cheats/cheat_store.h"
#include "interface_layout.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void check(bool ok, const char *why, const cr_ui *u) {
    if (ok) return;
    fprintf(stderr, "FAIL %s: %s / %s\n", why, u ? u->message : "", u ? u->engine.error : ""); exit(1);
}
typedef struct {
    uint64_t request, finished;
    int field;
    bool exporting;
    char text[4097];
    uint8_t *bytes; size_t size;
    unsigned submits;
} fixture;
static bool file(void *ctx, uint64_t request, bool exporting, const char *title,
                 const uint8_t *bytes, size_t size) {
    fixture *f = ctx; (void)title;
    f->request = request; f->exporting = exporting;
    free(f->bytes); f->bytes = NULL; f->size = size;
    if (size) { f->bytes = malloc(size); check(f->bytes != NULL, "snapshot allocation", NULL); memcpy(f->bytes, bytes, size); }
    return true;
}
static void finish(void *ctx, uint64_t request) { ((fixture *)ctx)->finished = request; }
static void edit(void *ctx, int field, const char *text, bool secret) {
    fixture *f = ctx; check(!secret, "cheat input is not password", NULL);
    f->field = field; snprintf(f->text, sizeof(f->text), "%s", text);
}
static void submit(void *ctx) { ++((fixture *)ctx)->submits; }
static void action(cr_ui *u, int wanted) {
    for (int i = 0; i < cr_menu_entry_count(u); ++i) {
        cr_menu_row r = cr_menu_entry_get(u, i);
        if (r.action == wanted) { check(!r.disabled, "action enabled", u); u->focus = i; cr_ui_confirm(u); return; }
    }
    check(false, "action exists", u);
}
static cr_menu_row find(const cr_ui *u, int wanted) {
    for (int i = 0; i < cr_menu_entry_count(u); ++i) {
        cr_menu_row r = cr_menu_entry_get(u, i); if (r.action == wanted) return r;
    }
    return (cr_menu_row){0};
}
static void quick(cr_ui *u) {
    u->page = CR_PLAY; u->quick = true; u->depth = 0;
    u->focus = cr_ui_quick_row(u, CR_QUICK_CHEATS); cr_ui_confirm(u);
    check(u->page == CR_CHEATS && !u->quick, "quick menu opens cheats", u);
}
static void write_file(const char *path, const void *data, size_t size) {
    FILE *f = fopen(path, "wb"); check(f && fwrite(data, 1, size, f) == size && !fclose(f), "fixture file", NULL);
}
static void rom(const char *path) {
    uint8_t data[32768] = {0};
    data[0x100] = 0xc3; data[0x101] = 0x50; data[0x102] = 1;
    const uint8_t loop[] = {0x18,0xfe}; memcpy(data + 0x150, loop, sizeof(loop));
    write_file(path, data, sizeof(data));
}
static void imported(cr_ui *u, fixture *f, const char *text) {
    action(u, ACT_CHEAT_IMPORT);
    cr_cheat_file_result r = {.request = f->request, .data = (uint8_t *)text, .size = strlen(text)};
    cr_ui_cheat_file_result(u, &r);
    check(f->finished == r.request, "import acknowledged", u);
}
static const char valid[] = "cheats = 1\ncheat0_desc = \"PV test\"\ncheat0_code = \"014200C0\"\ncheat0_enable = true\ncheat0_handler = 0\n";
int main(void) {
    char root[] = "/tmp/corvus-menu-cheats-XXXXXX";
    check(mkdtemp(root) != NULL, "private root", NULL);
    fixture f = {0}; cr_ui u;
    cr_ui_init(&u, (cr_platform){.context=&f,.edit_text=edit,.submit_text=submit,
                  .cheat_file=file,.finish_cheat_file=finish}, root);
    cr_ui_size(&u, 384, 700);
    cr_menu_cheats_open(&u); check(!u.cheats && u.page == CR_MAIN, "no game is safe", &u);
    int core = cr_registry_find("corvus.gb"); check(core >= 0, "GB provider", &u);
    char id[96], source[4096]; memset(id,'a',64); strcpy(id+64,"-0");
    snprintf(source,sizeof(source),"%s/test.gb",root); rom(source);
    check(cr_engine_select(&u.engine,core) && cr_engine_open(&u.engine,core,id,source), "open synthetic game", &u);
    cr_menu_cheats_loaded(&u);
    corvus_cheats_api supported = u.engine.cheats_api;
    memset(&u.engine.cheats_api,0,sizeof(u.engine.cheats_api)); quick(&u);
    check(cr_menu_entry_count(&u)==2 && !find(&u,ACT_CHEAT_ADD).action &&
          find(&u,ACT_CHEAT_UPDATE_CORE).action==ACT_CHEAT_UPDATE_CORE,"older core offers update without fake cheats",&u);
    u.engine.cheats_api=supported; quick(&u);
    check(cr_menu_entry_count(&u) == 7, "empty cheat menu", &u);
    action(&u,ACT_CHEAT_ADD);
    check(u.page == CR_CHEAT_TEXT && f.field == 6, "new code opens multiline editor", &u);
    cr_ui_text_result(&u,-6,"014200C0",true);
    check(u.page == CR_CHEAT_TEXT && !cr_engine_cheat_count(&u.engine,0), "preview cannot apply", &u);
    cr_ui_touch(&u,0,0,360,28); cr_ui_touch(&u,0,2,360,28);
    check(f.submits == 1, "header asks OS to submit current ticket", &u);
    cr_ui_text_result(&u,6,"014200C0",true);
    check(u.page == CR_CHEAT_ENTRY, "valid code saved", &u);
    action(&u,ACT_CHEAT_ENABLE);
    check(find(&u,ACT_CHEAT_ENABLE).checked && !cr_engine_cheat_count(&u.engine,0), "toggle stages default", &u);
    action(&u,ACT_CHEAT_NAME); cr_ui_text_result(&u,5,"",true);
    check(!strcmp(find(&u,ACT_CHEAT_NAME).hint,"Code 1"),"empty name keeps readable generated label",&u);
    action(&u,ACT_CHEAT_NAME); cr_ui_text_result(&u,5,"PV émeraude",true);
    action(&u,ACT_CHEAT_CODE); cr_ui_text_result(&u,6,"invalid",true);
    check(u.page == CR_CHEAT_TEXT && f.field == 6, "invalid code keeps editor", &u);
    cr_ui_back(&u); check(u.page == CR_CHEAT_ENTRY, "cancel returns to entry", &u);
    cr_ui_back(&u); action(&u,ACT_CHEAT_APPLY);
    check(cr_engine_cheat_count(&u.engine,0) == 1, "apply enables actual core list", &u);
    check(cr_engine_step(&u.engine,0), "run patched frame", &u);
    uint8_t byte = 0;
    check(cr_engine_read_achievement_memory(&u.engine,0xc000,&byte,1)==1 && byte==0x42, "menu changes real engine memory", &u);
    action(&u,ACT_CHEAT_EXPORT); check(f.exporting && f.size, "export snapshot", &u);
    cr_cheat_profile parsed={0}; char error[256];
    check(cr_cheat_profile_parse(&parsed,(char *)f.bytes,f.size,error,sizeof(error)) &&
          parsed.count==1 && !strcmp(parsed.entries[0].name,"PV émeraude") && parsed.entries[0].enabled, "export preserves code/name/status", &u);
    cr_cheat_profile_free(&parsed);
    cr_cheat_file_result result={.request=f.request}; cr_ui_cheat_file_result(&u,&result);
    imported(&u,&f,"cheats = 1\ncheat0_code = \"broken\"\ncheat0_enable = true\n");
    check(cr_engine_cheat_count(&u.engine,0)==1 && cr_menu_entry_count(&u)==8,"invalid import preserves list and active core",&u);
    imported(&u,&f,"cheats = 1\ncheat0_handler = 1\ncheat0_code = \"014200C0\"\n");
    check(cr_menu_entry_count(&u)==8,"frontend-memory handler refused",&u);
    action(&u,ACT_CHEAT_IMPORT); uint64_t old=f.request;
    cr_ui_back(&u); check(u.quick && f.finished==old,"back cancels pending request and restores quick menu",&u);
    quick(&u); action(&u,ACT_CHEAT_IMPORT); check(f.request!=old,"request IDs unique",&u);
    result=(cr_cheat_file_result){.request=old,.data=(uint8_t *)"cheats = 0\n",.size=11}; cr_ui_cheat_file_result(&u,&result);
    check(find(&u,ACT_CHEAT_IMPORT).disabled,"stale result cannot complete newer transfer",&u);
    result=(cr_cheat_file_result){.request=f.request,.cancelled=true}; cr_ui_cheat_file_result(&u,&result);
    action(&u,ACT_CHEAT_SETTINGS); action(&u,ACT_CHEAT_AUTO_LOAD); action(&u,ACT_CHEAT_AUTO_TOGGLE); cr_ui_back(&u);
    action(&u,ACT_CHEAT_ENTRY); action(&u,ACT_CHEAT_ENABLE);
    check(!cr_engine_cheat_count(&u.engine,0),"automatic toggle clears actual active list",&u);
    action(&u,ACT_CHEAT_ENABLE); cr_ui_back(&u);
    check(cr_engine_cheat_count(&u.engine,0)==1,"automatic toggle enables",&u);
    check(cr_engine_open(&u.engine,core,id,source),"new game resets provider list",&u); cr_menu_cheats_loaded(&u); quick(&u);
    check(cr_engine_cheat_count(&u.engine,0)==1,"explicit auto-load persists",&u);
    corvus_subsystem_info subsystem={.struct_size=sizeof(subsystem)};
    check(cr_engine_subsystem(&u.engine,0,&subsystem),"link available",&u);
    const char *ids[]={id,id}, *paths[]={source,source};
    check(cr_engine_open_subsystem(&u.engine,core,subsystem.id,ids,paths),"same ROM in both slots",&u);
    cr_menu_cheats_loaded(&u); quick(&u);
    check(cr_engine_cheat_count(&u.engine,0)==1 && !cr_engine_cheat_count(&u.engine,1),"same ROM P2 profile isolated",&u);
    action(&u,ACT_CHEAT_PLAYER); imported(&u,&f,valid); action(&u,ACT_CHEAT_APPLY);
    check(cr_engine_cheat_count(&u.engine,0)==1 && cr_engine_cheat_count(&u.engine,1)==1,"both player lists active",&u);
    action(&u,ACT_CHEAT_CLEAR); action(&u,ACT_CHEAT_BACK);
    check(cr_engine_cheat_count(&u.engine,1)==1,"clear requires confirmation",&u);
    action(&u,ACT_CHEAT_CLEAR); action(&u,ACT_CHEAT_CLEAR_OK);
    check(cr_engine_cheat_count(&u.engine,0)==1 && !cr_engine_cheat_count(&u.engine,1),"clear only selected player",&u);
    action(&u,ACT_CHEAT_PLAYER); action(&u,ACT_CHEAT_ENTRY); action(&u,ACT_CHEAT_DELETE); action(&u,ACT_CHEAT_DELETE_OK);
    check(u.page==CR_CHEATS && !cr_engine_cheat_count(&u.engine,0),"delete with auto-apply returns to list",&u);
    imported(&u,&f,valid); action(&u,ACT_CHEAT_APPLY);
    cr_ui_back(&u); check(u.quick,"back to quick",&u);
    u.focus=cr_ui_quick_row(&u,CR_QUICK_CLOSE); cr_ui_confirm(&u);
    check(!u.cheats && !u.engine.loaded && u.engine.handle,"close frees cheats but keeps core",&u);
    char profile[4256]; snprintf(profile,sizeof(profile),"%s/cheats/%s.cht",root,id);
    write_file(profile,"corrupt",7);
    check(cr_engine_open(&u.engine,core,id,source),"reopen with corrupt private list",&u);
    cr_menu_cheats_loaded(&u); quick(&u);
    check(find(&u,ACT_CHEAT_ADD).disabled && !cr_engine_cheat_count(&u.engine,0),"corrupt private list protected",&u);
    imported(&u,&f,valid); check(!find(&u,ACT_CHEAT_ADD).disabled,"explicit import repairs corrupt list",&u);
    action(&u,ACT_CHEAT_IMPORT); uint64_t abandoned=f.request;
    cr_menu_cheats_loaded(&u);
    check(f.finished==abandoned,"game replacement cancels previous transfer",&u);
    result=(cr_cheat_file_result){.request=abandoned,.data=(uint8_t *)"cheats = 0\n",.size=11};
    cr_ui_cheat_file_result(&u,&result); quick(&u);
    check(cr_menu_entry_count(&u)==8,"late completion cannot replace reloaded profile",&u);
    cr_ui_destroy(&u); free(f.bytes);
    puts("native menu cheats passed"); return 0;
}
