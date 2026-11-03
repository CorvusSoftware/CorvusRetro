// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
#define main cr_audio_existing_regression_main
#include "native_audio_test.c"
#undef main
#include "audio_sync.h"
#include <math.h>

typedef struct {
    cr_audio audio;
    cr_audio_sync sync;
    double next_callback, device_rate;
    unsigned produced, consumed, zeros, queued_min, queued_max, catchup_max;
    bool started, exact;
} clock_fixture;

static void clock_start(clock_fixture *f, int burst_size, double ppm) {
    memset(f,0,sizeof(*f));
    cr_audio_init(&f->audio);
    cr_audio_sync_reset(&f->sync);
    f->device_rate=32768*(1+ppm/1000000);
    f->queued_min=CR_AUDIO_RING_FRAMES;
    f->exact=true;
    burst=burst_size;
    capacity=4096;
    threaded=false;
}
static void clock_consume(clock_fixture *f, double time) {
    int16_t out[1024];
    while (f->started && f->next_callback<time) {
        emit(out,burst);
        for (int i=0;i<burst;++i) {
            int16_t left=out[i*2],right=out[i*2+1];
            if (!left && !right) { ++f->zeros; continue; }
            int16_t expected=pcm_value(f->consumed++);
            if (left!=expected || right!=(int16_t)(-1-(int)expected)) f->exact=false;
        }
        f->next_callback+=(double)burst/f->device_rate;
    }
}
static void clock_produce(clock_fixture *f, double time, double fps, double phase) {
    int16_t block[1200];
    unsigned previous=(unsigned)floor(f->produced*32768.0/fps);
    unsigned next=(unsigned)floor((f->produced+1)*32768.0/fps);
    check(next-previous<=600,"synthetic frame PCM fits the bounded test block");
    for (unsigned i=previous;i<next;++i) {
        int16_t left=pcm_value(i);
        block[(i-previous)*2]=left;
        block[(i-previous)*2+1]=(int16_t)(-1-(int)left);
    }
    check(cr_audio_start(&f->audio,32768),"scheduled output remains available");
    cr_audio_write(&f->audio,block,(next-previous)*2);
    ++f->produced;
    if (!f->started && f->audio.playing) {
        f->started=true;
        f->next_callback=time+phase*burst/f->device_rate;
    }
}
static void clock_feedback(clock_fixture *f, double time, double fps, bool control) {
    unsigned queued=cr_audio_queued_frames(&f->audio);
    if (f->started) {
        if (queued<f->queued_min) f->queued_min=queued;
        if (queued>f->queued_max) f->queued_max=queued;
    }
    if (control)
        cr_audio_sync_update(&f->sync,time,queued,CR_AUDIO_RING_FRAMES,
                             (unsigned)f->audio.prime_frames,32768,fps,f->audio.playing);
}
static void scheduled_clock(double ppm,unsigned hz,double fps,double phase,
                            bool control,unsigned drift_kind,unsigned seconds,int burst_size) {
    clock_fixture f;
    clock_start(&f,burst_size,ppm);
    double deadline=0;
    for (unsigned tick=0;tick<seconds*hz;++tick) {
        double time=(double)tick/hz;
        double current_ppm=drift_kind==1 ? ppm*sin(time/120) :
                           drift_kind==2 ? (time<seconds/2.0 ? -ppm : ppm) : ppm;
        f.device_rate=32768*(1+current_ppm/1000000);
        clock_consume(&f,time);
        unsigned frames=0;
        while (time+1e-12>=deadline && frames<(control ? CR_AUDIO_SYNC_MAX_CATCHUP : 1u)) {
            clock_produce(&f,time,fps,phase);
            ++frames;
            deadline+=cr_audio_sync_period(&f.sync,fps);
        }
        if (frames>f.catchup_max) f.catchup_max=frames;
        clock_feedback(&f,time,fps,control);
    }
    check(f.started && f.consumed>1000000,"virtual playback covers meaningful sustained audio");
    check(f.exact,"full-range stereo PCM remains bitexact and ordered through queue wraps");
    check(f.queued_max<CR_AUDIO_RING_FRAMES,"clock drift cannot saturate the queue in the controlled range");
    if (control) {
        check(!f.zeros,"audio-master pacing prevents clock-drift silence");
        check(f.catchup_max<=CR_AUDIO_SYNC_MAX_CATCHUP,"emulation catchup stays bounded per rendered frame");
    } else if (ppm>0) {
        check(f.zeros>0,"fixed v0.22.1 pacing reproduces long-term fast-device starvation");
    }
    printf("clock control=%d hz=%u fps=%.8f ppm=%.0f phase=%.1f drift=%u seconds=%u burst=%d zeros=%u exact=%d queued=%u..%u catchup=%u\n",
           control,hz,fps,ppm,phase,drift_kind,seconds,burst_size,f.zeros,f.exact,
           f.queued_min,f.queued_max,f.catchup_max);
    cr_audio_close(&f.audio);
}
static void queue_snapshot_tests(void) {
    cr_audio a;
    cr_audio_init(&a);
    burst=192;capacity=4096;threaded=false;
    int16_t pcm[1200],out[384];samples(pcm,600,1234);
    check(!cr_audio_queued_frames(&a),"inactive queue snapshots are empty");
    check(cr_audio_start(&a,32768),"queue snapshot fixture prepares a stream");
    cr_audio_write(&a,pcm,548*2);
    check(!a.playing && !cr_audio_queued_frames(&a),"priming does not expose an active playback clock");
    cr_audio_write(&a,pcm,549*2);
    check(cr_audio_queued_frames(&a)==1097,"owner sees the ready queue occupancy");
    emit(out,192);
    check(cr_audio_queued_frames(&a)==905,"owner snapshots reflect callback consumption");
    atomic_store(&a.read_frame,UINT_MAX-7u);
    atomic_store(&a.write_frame,8u);
    check(cr_audio_queued_frames(&a)==16,"queue snapshots preserve unsigned cursor wrap");
    stream.error(&stream,stream.error_context,-899);
    check(!cr_audio_queued_frames(&a),"disconnected output cannot supply a playback clock");
    cr_audio_close(&a);
    check(!cr_audio_queued_frames(&a),"closed output retires its queue snapshot");
}
static void controller_boundaries(void) {
    cr_audio_sync s;
    cr_audio_sync_reset(&s);
    check(!s.active && s.correction==0,"fresh controller has no stale correction");
    check(cr_audio_sync_period(&s,60)==1.0/60,"inactive output uses declared frame cadence");
    const double bad_fps[]={NAN,INFINITY,-1,0,.5,1001};
    for (size_t i=0;i<sizeof(bad_fps)/sizeof(*bad_fps);++i) {
        cr_audio_sync_update(&s,1,1000,16384,1097,32768,bad_fps[i],true);
        check(!s.active && cr_audio_sync_period(&s,bad_fps[i])==1.0/60,
              "invalid frame rates cannot create non-finite or zero timing");
    }
    cr_audio_sync_update(&s,NAN,1000,16384,1097,32768,60,true);
    check(!s.active,"invalid wall clock resets feedback");
    cr_audio_sync_update(&s,INFINITY,1000,16384,1097,32768,60,true);
    check(!s.active,"infinite wall clock resets feedback");
    cr_audio_sync_update(&s,1,1000,16384,1097,0,60,true);
    check(!s.active,"zero sample rate disables feedback");
    cr_audio_sync_update(&s,1,1000,0,1097,32768,60,true);
    check(!s.active,"missing queue capacity disables feedback");
    cr_audio_sync_update(&s,1,16385,16384,1097,32768,60,true);
    check(!s.active,"incoherent queue snapshots cannot corrupt timing");
    cr_audio_sync_update(&s,1,50,100,UINT_MAX,32768,60,true);
    check(s.active && s.target==75,"requested reserve is capped below queue capacity");
    cr_audio_sync_update(&s,2,0,100,UINT_MAX,32768,60,false);
    check(!s.active && s.correction==0,"pausing clears filtered occupancy and correction");
    cr_audio_sync_update(&s,10,1000,16384,1097,32768,60,true);
    check(s.active && s.filtered==1000 && s.correction==0,"restarting initializes from fresh queue data");
    cr_audio_sync_update(&s,10,0,16384,1097,32768,60,true);
    check(s.filtered==1000 && s.last_time==10,"zero elapsed time cannot apply duplicate feedback");
    for (unsigned i=1;i<=100;++i)
        cr_audio_sync_update(&s,10+i*.1,0,16384,1097,32768,60,true);
    check(s.correction>0 && s.correction<=.005 && cr_audio_sync_period(&s,60)>0,
          "starvation correction remains bounded and produces positive deadlines");
    cr_audio_sync_update(&s,5,900,16384,1097,32768,60,true);
    check(s.filtered==900 && s.last_time==5 && s.correction==0,"backward clock reinitializes instead of integrating negative time");
    cr_audio_sync_update(&s,6,1200,16384,1097,48000,60,true);
    check(s.rate==48000 && s.filtered==1200 && s.correction==0,"sample-rate change discards old queue feedback");
    cr_audio_sync_update(&s,7,1300,16384,1097,48000,50,true);
    check(s.fps==50 && s.filtered==1300 && s.correction==0,"core frame-rate change starts a fresh controller");
    for (unsigned i=1;i<=100;++i)
        cr_audio_sync_update(&s,7+i*.1,16384,16384,1097,48000,50,true);
    check(s.correction<0 && s.correction>=-.005,"overfull correction is bounded symmetrically");
    cr_audio_sync_reset(&s);
    cr_audio_sync_update(&s,1,1000,16384,1097,32768,60,true);
    cr_audio_sync_update(&s,1000,0,16384,1097,32768,60,true);
    check(fabs(s.filtered-1000*(1-.1/.6))<1e-9,"a long scheduling gap cannot destabilize the smoothing step");
    cr_audio_sync_reset(&s);
    cr_audio_sync_reset(&s);
    check(!s.active && s.target==0 && s.last_time==0,"reset is idempotent after arbitrary timing history");
}
static void cpu_budget(double milliseconds) {
    const double fps=16777216.0/280896.0;
    clock_fixture f;
    clock_start(&f,192,100);
    double time=0,deadline=0;
    while (time<120) {
        clock_consume(&f,time);
        unsigned frames=0;
        while (time+1e-12>=deadline && frames<CR_AUDIO_SYNC_MAX_CATCHUP) {
            time+=milliseconds/1000;
            clock_consume(&f,time);
            clock_produce(&f,time,fps,0);
            ++frames;
            deadline+=cr_audio_sync_period(&f.sync,fps);
        }
        if (frames>f.catchup_max) f.catchup_max=frames;
        clock_feedback(&f,time,fps,true);
        time+=.001;
        clock_consume(&f,time);
        time=(floor(time*60+1e-9)+1)/60;
    }
    check(f.exact && f.catchup_max<=CR_AUDIO_SYNC_MAX_CATCHUP,
          "CPU work keeps every delivered sample ordered and catchup bounded");
    printf("CPU cost=%.0fms zeros=%u queued=%u..%u frames=%u catchup=%u\n",
           milliseconds,f.zeros,f.queued_min,f.queued_max,f.produced,f.catchup_max);
    if (milliseconds<=15)
        check(!f.zeros,"sustainable CPU work and blocking display swaps preserve continuous PCM");
    else
        check(f.zeros>0,"pacing cannot fabricate PCM when emulation is slower than real time");
    cr_audio_close(&f.audio);
}
int main(void) {
    atomic_init(&consume,false);
    atomic_init(&callback_errors,0);
    queue_snapshot_tests();controller_boundaries();
    const double gba=16777216.0/280896.0;
    for (unsigned hz=60;hz<=120;hz+=30)
        for (unsigned phase=0;phase<2;++phase)
            for (int ppm=-1000;ppm<=1000;ppm+=1000)
                scheduled_clock(ppm,hz,gba,phase*.5,true,0,600,192);
    for (unsigned phase=0;phase<2;++phase)
        for (int ppm=-1000;ppm<=1000;ppm+=1000)
            scheduled_clock(ppm,60,60,phase*.5,true,0,600,192);
    scheduled_clock(100,60,gba,0,false,0,600,192);
    scheduled_clock(-100,60,gba,0,false,0,600,192);
    scheduled_clock(1000,60,gba,0,true,1,3600,192);
    scheduled_clock(1000,60,60,.5,true,1,3600,192);
    scheduled_clock(1000,60,gba,0,true,2,600,192);
    scheduled_clock(-1000,60,60,.5,true,2,600,192);
    scheduled_clock(100,60,gba,0,true,0,3600,192);
    scheduled_clock(-100,60,gba,0,true,0,3600,192);
    const int bursts[]={64,128,256,512};
    for (size_t b=0;b<sizeof(bursts)/sizeof(*bursts);++b) {
        scheduled_clock(1000,60,gba,.5,true,0,600,bursts[b]);
        scheduled_clock(-1000,60,gba,.5,true,0,600,bursts[b]);
    }
    cpu_budget(6);cpu_budget(10);cpu_budget(15);cpu_budget(25);
    puts("PASS audio-master drift, bitexact PCM, bounded controller and CPU-budget regressions");
    return 0;
}
