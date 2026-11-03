// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
#define _POSIX_C_SOURCE 200809L
#define _XOPEN_SOURCE 700
#include "engine.h"
#include "core_registry.h"
#include <errno.h>
#include <ftw.h>
#include <pthread.h>
#include <sched.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

static pthread_t owner;
static corvus_core_api original;
static corvus_subsystem_api original_slots;
static atomic_uint copies, acknowledgements;
static unsigned checks;
static pthread_mutex_t gate = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t changed = PTHREAD_COND_INITIALIZER;
static bool blocked, entered;
static atomic_bool lifecycle_returned;
static void check(bool ok, const char *message) {
    if (!ok) { fprintf(stderr,"FAIL async battery: %s\n",message); exit(1); }
    ++checks;
}
static void owner_call(void) {
    if (!pthread_equal(owner,pthread_self())) {
        fputs("FAIL core API called outside owner thread\n",stderr); abort();
    }
}
static size_t battery_size(void *core) { owner_call(); return original.battery_size(core); }
static int battery_dirty(void *core) { owner_call(); return original.battery_dirty(core); }
static int battery_copy(void *core,uint8_t *bytes,size_t size,uint64_t *generation) {
    owner_call(); atomic_fetch_add(&copies,1); return original.battery_copy(core,bytes,size,generation);
}
static int battery_ack(void *core,uint64_t generation) {
    owner_call(); atomic_fetch_add(&acknowledgements,1); return original.battery_acknowledge(core,generation);
}
static size_t slot_size(void *core,uint32_t slot) { owner_call(); return original_slots.battery_size(core,slot); }
static int slot_dirty(void *core,uint32_t slot) { owner_call(); return original_slots.battery_dirty(core,slot); }
static int slot_copy(void *core,uint32_t slot,uint8_t *bytes,size_t size,uint64_t *generation) {
    owner_call(); atomic_fetch_add(&copies,1); return original_slots.battery_copy(core,slot,bytes,size,generation);
}
static int slot_ack(void *core,uint32_t slot,uint64_t generation) {
    owner_call(); atomic_fetch_add(&acknowledgements,1); return original_slots.battery_acknowledge(core,slot,generation);
}
static void observe(cr_engine *engine) {
    original=engine->api; original_slots=engine->subsystems;
    engine->api.battery_size=battery_size; engine->api.battery_dirty=battery_dirty;
    engine->api.battery_copy=battery_copy; engine->api.battery_acknowledge=battery_ack;
    if (engine->subsystems.battery_size) {
        engine->subsystems.battery_size=slot_size; engine->subsystems.battery_dirty=slot_dirty;
        engine->subsystems.battery_copy=slot_copy; engine->subsystems.battery_acknowledge=slot_ack;
    }
}
int __real_fsync(int fd);
int __wrap_fsync(int fd) {
    pthread_mutex_lock(&gate);
    if (blocked) {
        if (pthread_equal(owner,pthread_self())) {
            fputs("FAIL filesystem sync in asynchronous request/poll\n",stderr); abort();
        }
        entered=true; pthread_cond_broadcast(&changed);
        while (blocked) pthread_cond_wait(&changed,&gate);
    }
    pthread_mutex_unlock(&gate);
    return __real_fsync(fd);
}
static void begin_slow_disk(void) {
    pthread_mutex_lock(&gate); blocked=true; entered=false; pthread_mutex_unlock(&gate);
}
static void wait_for_writer(void) {
    struct timespec until; clock_gettime(CLOCK_REALTIME,&until); until.tv_sec+=5;
    pthread_mutex_lock(&gate);
    int result=0;
    while (!entered && !result) result=pthread_cond_timedwait(&changed,&gate,&until);
    pthread_mutex_unlock(&gate);
    check(!result && entered,"background writer reaches the deliberately blocked fsync");
}
static void release_disk(void) {
    pthread_mutex_lock(&gate); blocked=false; pthread_cond_broadcast(&changed); pthread_mutex_unlock(&gate);
}
static void *release_during_lifecycle(void *unused) {
    (void)unused;
    struct timespec delay={0,20000000}; nanosleep(&delay,NULL);
    if (atomic_load(&lifecycle_returned)) { fputs("FAIL lifecycle returned before durable write\n",stderr); abort(); }
    release_disk(); return NULL;
}
static void lifecycle_barrier(cr_engine *engine, bool (*operation)(cr_engine *), const char *message) {
    begin_slow_disk();
    check(cr_engine_request_battery_save(engine),"queue snapshot before lifecycle operation"); wait_for_writer();
    pthread_t release; atomic_store(&lifecycle_returned,false);
    check(!pthread_create(&release,NULL,release_during_lifecycle,NULL),"release pending write during lifecycle operation");
    check(operation(engine),message);
    atomic_store(&lifecycle_returned,true); check(!pthread_join(release,NULL),"join lifecycle disk release helper");
}
static double seconds(void) {
    struct timespec now; clock_gettime(CLOCK_MONOTONIC,&now); return now.tv_sec+now.tv_nsec/1e9;
}
static void await_acks(cr_engine *engine,unsigned wanted) {
    double deadline=seconds()+5;
    while (atomic_load(&acknowledgements)<wanted && seconds()<deadline) {
        check(cr_engine_poll_battery_save(engine),"nonblocking poll succeeds"); sched_yield();
    }
    check(atomic_load(&acknowledgements)==wanted,"durable completion is acknowledged on owner thread");
}
static void original_gba(const char *path) {
    uint8_t bytes[512]={0};
    const uint32_t words[]={0xe59f0010,0xe3a01000,0xe2811001,0xe5c01000,0xeafffffc,0xeafffffe,0x0e000000};
    for (size_t i=0;i<sizeof(words)/sizeof(*words);++i)
        for (unsigned b=0;b<4;++b) bytes[i*4+b]=(uint8_t)(words[i]>>(b*8));
    bytes[0xb2]=0x96; memcpy(bytes+0x100,"SRAM_V",6);
    FILE *file=fopen(path,"wb"); check(file && fwrite(bytes,1,sizeof(bytes),file)==sizeof(bytes) && !fclose(file),"original SRAM writer program");
}
static void original_gb(const char *path) {
    uint8_t bytes[32768]={0};
    bytes[0x100]=0xc3; bytes[0x101]=0x50; bytes[0x102]=1;
    bytes[0x147]=3; bytes[0x149]=2;
    const uint8_t code[]={0x3e,0x0a,0xea,0,0,0x3e,0,0x3c,0xea,0,0xa0,0x18,0xfa};
    memcpy(bytes+0x150,code,sizeof(code));
    FILE *file=fopen(path,"wb"); check(file && fwrite(bytes,1,sizeof(bytes),file)==sizeof(bytes) && !fclose(file),"original linked SRAM writer program");
}
static void read_expected(const char *path,const uint8_t *bytes,size_t size) {
    uint8_t *read=malloc(size); check(read!=NULL,"allocate readback");
    FILE *file=fopen(path,"rb");
    check(file && fread(read,1,size,file)==size && fgetc(file)==EOF && !fclose(file) && !memcmp(read,bytes,size),
          "durable raw battery is exactly the requested snapshot"); free(read);
}
static int remove_fixture(const char *path,const struct stat *status,int type,struct FTW *walk) {
    (void)status;(void)type;(void)walk; return remove(path);
}
int main(void) {
    owner=pthread_self();
    char root[]="/tmp/corvus-async-battery-XXXXXX",rom[4096],saved[4096],obstruction[4096];
    check(mkdtemp(root)!=NULL,"private test storage");
    snprintf(rom,sizeof(rom),"%s/original.gba",root); original_gba(rom);
    const char *id="eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee-2";
    snprintf(saved,sizeof(saved),"%s/saves/%s.sav",root,id);
    snprintf(obstruction,sizeof(obstruction),"%s/saves/%s.sav.new",root,id);
    int system=cr_registry_find("corvus.gba"); cr_engine e; cr_engine_init(&e,root);
    check(cr_engine_open(&e,system,id,rom) && cr_engine_step(&e,0),"real GBA starts and changes SRAM"); observe(&e);
    uint8_t *snapshot=NULL;size_t size=0;
    check(cr_engine_export_battery(&e,0,&snapshot,&size) && size==32768,"copy owner-thread reference battery");
    unsigned before_copies=atomic_load(&copies), before_ack=atomic_load(&acknowledgements);
    begin_slow_disk(); check(cr_engine_request_battery_save(&e),"periodic save enqueues a snapshot"); wait_for_writer();
    check(atomic_load(&acknowledgements)==before_ack && e.api.battery_dirty(e.handle),"pending filesystem work never acknowledges dirty SRAM");
    for (unsigned frame=0;frame<20;++frame)
        check(cr_engine_step(&e,0) && cr_engine_poll_battery_save(&e),"real emulation and polling continue while fsync is blocked");
    check(cr_engine_request_battery_save(&e) && atomic_load(&copies)==before_copies+1,"in-flight requests are bounded to one immutable snapshot");
    release_disk(); await_acks(&e,before_ack+1); read_expected(saved,snapshot,size);free(snapshot);
    check(e.api.battery_dirty(e.handle),"acknowledging old durable generation retains newer SRAM changes");
    check(cr_engine_save_battery(&e) && !e.api.battery_dirty(e.handle),"synchronous lifecycle writes and acknowledges latest generation");
    before_copies=atomic_load(&copies);
    check(cr_engine_request_battery_save(&e) && atomic_load(&copies)==before_copies,"clean battery does not copy or enqueue another snapshot");

    check(cr_engine_save_state(&e) && cr_engine_step(&e,0),"capture a real state and advance its battery generation");
    lifecycle_barrier(&e,cr_engine_load_state,"state restore drains pending write before changing core generations");
    check(cr_engine_export_battery(&e,0,&snapshot,&size) && cr_engine_save_battery(&e),"persist the restored battery after older worker finishes");
    read_expected(saved,snapshot,size); free(snapshot);
    check(cr_engine_step(&e,0),"change battery before reset barrier");
    lifecycle_barrier(&e,cr_engine_reset,"reset drains pending write before resetting the core");

    check(cr_engine_step(&e,0) && mkdir(obstruction,0700)==0,"inject atomic writer failure");
    before_ack=atomic_load(&acknowledgements);
    check(cr_engine_request_battery_save(&e),"failing periodic write is asynchronous");
    bool failed=false; double deadline=seconds()+5;
    while (!failed && seconds()<deadline) { failed=!cr_engine_poll_battery_save(&e);sched_yield(); }
    check(failed && e.handle && e.loaded && e.api.battery_dirty(e.handle) && atomic_load(&acknowledgements)==before_ack,
          "failed background write retains the running core and every dirty generation");
    check(!cr_engine_close(&e) && e.handle,"failed persistence still prevents closing the core");
    check(!rmdir(obstruction) && cr_engine_save_battery(&e),"storage repair permits durable retry");

    char journal[4096]; snprintf(journal,sizeof(journal),"%s/saves/.battery-import",root);
    FILE *bad=fopen(journal,"wb");
    check(bad && fputs("invalid",bad)>=0 && !fclose(bad) && cr_engine_step(&e,0),"inject an invalid interrupted-import journal");
    before_ack=atomic_load(&acknowledgements);
    check(cr_engine_request_battery_save(&e),"import recovery also runs in the background writer");
    failed=false; deadline=seconds()+5;
    while (!failed && seconds()<deadline) { failed=!cr_engine_poll_battery_save(&e);sched_yield(); }
    check(failed && e.api.battery_dirty(e.handle) && atomic_load(&acknowledgements)==before_ack,
          "periodic saves preserve existing import-journal recovery instead of bypassing it");
    check(!unlink(journal) && cr_engine_save_battery(&e),"repair journal fixture and persist latest RAM");

    check(cr_engine_step(&e,0),"change battery before import barrier");
    begin_slow_disk();check(cr_engine_request_battery_save(&e),"queue old snapshot before import");wait_for_writer();
    uint8_t *imported=NULL;size_t imported_size=0;
    check(cr_engine_export_battery(&e,0,&imported,&imported_size),"RAM export does not wait for filesystem worker");
    imported[0]=0xc3;
    pthread_t release;atomic_store(&lifecycle_returned,false);
    check(!pthread_create(&release,NULL,release_during_lifecycle,NULL),"start deterministic delayed disk release");
    check(cr_engine_import_battery(&e,0,imported,imported_size),"import drains older writes before replacing battery files");
    atomic_store(&lifecycle_returned,true);check(!pthread_join(release,NULL),"join disk release helper");
    read_expected(saved,imported,imported_size);free(imported);
    check(cr_engine_close(&e),"close imported engine without an obsolete late writer");

    const char *id1="eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee-0";
    const char *id2="ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff-0";
    snprintf(rom,sizeof(rom),"%s/original.gb",root); original_gb(rom);
    system=cr_registry_find("corvus.gb");
    const char *ids[]={id1,id2},*paths[]={rom,rom};
    corvus_subsystem_info info;
    check(cr_engine_select(&e,system) && cr_engine_subsystem(&e,0,&info),"find real core's declared link subsystem");
    check(cr_engine_open_subsystem(&e,system,info.id,ids,paths),"open two-player battery fixture");
    unsigned masks[2]={0,0};check(cr_engine_step_players(&e,masks),"both players change SRAM");observe(&e);
    snprintf(obstruction,sizeof(obstruction),"%s/saves/%s-link-p2.sav.new",root,id2);
    check(!mkdir(obstruction,0700),"obstruct second player's atomic write");
    before_ack=atomic_load(&acknowledgements);check(cr_engine_request_battery_save(&e),"queue both player snapshots");
    failed=false;deadline=seconds()+5;
    while (!failed && seconds()<deadline) { failed=!cr_engine_poll_battery_save(&e);sched_yield(); }
    check(failed && atomic_load(&acknowledgements)==before_ack && e.subsystems.battery_dirty(e.handle,0) && e.subsystems.battery_dirty(e.handle,1),
          "failure of either slot acknowledges neither player's generation");
    check(!rmdir(obstruction) && cr_engine_save_battery(&e) && !e.subsystems.battery_dirty(e.handle,0) && !e.subsystems.battery_dirty(e.handle,1),
          "retry commits and acknowledges both players only after every durable write");
    check(cr_engine_step_players(&e,masks),"make both players dirty before close");
    begin_slow_disk();check(cr_engine_request_battery_save(&e),"queue final periodic save");wait_for_writer();
    atomic_store(&lifecycle_returned,false);
    check(!pthread_create(&release,NULL,release_during_lifecycle,NULL),"release final write after close begins");
    check(cr_engine_close(&e),"close waits for worker and preserves both latest player batteries");
    atomic_store(&lifecycle_returned,true);check(!pthread_join(release,NULL),"join final disk helper");
    check(!e.handle && !e.battery_worker,"close destroys the worker before releasing core resources");
    cr_engine_destroy(&e);
    check(!nftw(root,remove_fixture,16,FTW_DEPTH|FTW_PHYS),"clean owned synthetic fixture");
    printf("PASS %u asynchronous battery snapshot, durability, lifecycle and thread-ownership checks\n",checks);
    return 0;
}
