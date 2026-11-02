// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
#define _POSIX_C_SOURCE 200809L
#include "aaudio.h"
#include <limits.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

struct AAudioStream {
    AAudioStream_dataCallback data;
    AAudioStream_errorCallback error;
    void *data_context,*error_context;
    int rate,channels,format,xruns,buffer;
};
struct AAudioStreamBuilder { struct AAudioStream stream; };
static struct AAudioStreamBuilder builder;
static struct AAudioStream stream;
static int opens,starts,stops,closes,deletes,buffers,burst=8,capacity=256;
static bool fail_create,fail_open,fail_start,wrong_rate;
static int64_t clock_ns=INT64_C(1000000000);
static bool threaded;
static pthread_t consumer;
static atomic_bool consume;
static atomic_uint callback_errors;
static unsigned checks;
static void check(bool ok,const char *message) {
    if (!ok) { fprintf(stderr,"FAIL audio: %s\n",message); exit(1); }
    ++checks;
}
/* Target compile definition redirects only the driver's clock to deterministic time. */
int cr_audio_test_clock(clockid_t id,struct timespec *value) {
    (void)id; value->tv_sec=clock_ns/INT64_C(1000000000); value->tv_nsec=clock_ns%INT64_C(1000000000); return 0;
}
aaudio_result_t AAudio_createStreamBuilder(AAudioStreamBuilder **out) {
    if (fail_create) return -1;
    memset(&builder,0,sizeof(builder)); *out=&builder; return AAUDIO_OK;
}
void AAudioStreamBuilder_setDirection(AAudioStreamBuilder *b,int32_t value) { (void)b; check(value==AAUDIO_DIRECTION_OUTPUT,"output direction"); }
void AAudioStreamBuilder_setFormat(AAudioStreamBuilder *b,int32_t value) { b->stream.format=value; }
void AAudioStreamBuilder_setChannelCount(AAudioStreamBuilder *b,int32_t value) { b->stream.channels=value; }
void AAudioStreamBuilder_setSampleRate(AAudioStreamBuilder *b,int32_t value) { b->stream.rate=value; }
void AAudioStreamBuilder_setSharingMode(AAudioStreamBuilder *b,int32_t value) { (void)b; check(value==AAUDIO_SHARING_MODE_SHARED,"shared output"); }
void AAudioStreamBuilder_setPerformanceMode(AAudioStreamBuilder *b,int32_t value) { (void)b; check(value==AAUDIO_PERFORMANCE_MODE_LOW_LATENCY,"low-latency output"); }
void AAudioStreamBuilder_setDataCallback(AAudioStreamBuilder *b,AAudioStream_dataCallback callback,void *ctx) { b->stream.data=callback; b->stream.data_context=ctx; }
void AAudioStreamBuilder_setErrorCallback(AAudioStreamBuilder *b,AAudioStream_errorCallback callback,void *ctx) { b->stream.error=callback; b->stream.error_context=ctx; }
aaudio_result_t AAudioStreamBuilder_openStream(AAudioStreamBuilder *b,AAudioStream **out) {
    ++opens; if (fail_open) { *out=NULL; return -1; }
    stream=b->stream; if (wrong_rate) ++stream.rate;
    *out=&stream; return AAUDIO_OK;
}
aaudio_result_t AAudioStreamBuilder_delete(AAudioStreamBuilder *b) { (void)b; ++deletes; return AAUDIO_OK; }
static void *audio_thread(void *unused) {
    (void)unused; int16_t out[202];
    while (atomic_load_explicit(&consume,memory_order_acquire)) {
        stream.data(&stream,stream.data_context,out,101);
        for (unsigned i=0;i<101;++i)
            if (out[i*2+1] != -out[i*2]) atomic_fetch_add(&callback_errors,1);
    }
    return NULL;
}
aaudio_result_t AAudioStream_requestStart(AAudioStream *s) {
    ++starts; if (fail_start) return -1;
    cr_audio *audio=s->data_context;
    check(atomic_load(&audio->write_frame)-atomic_load(&audio->read_frame)>=(unsigned)audio->prime_frames,
          "device never starts before PCM priming");
    if (threaded) { atomic_store(&consume,true); check(pthread_create(&consumer,NULL,audio_thread,NULL)==0,"start simulated callback thread"); }
    return AAUDIO_OK;
}
aaudio_result_t AAudioStream_requestStop(AAudioStream *s) { (void)s; ++stops; return AAUDIO_OK; }
aaudio_result_t AAudioStream_close(AAudioStream *s) {
    ++closes;
    if (threaded && atomic_exchange(&consume,false)) check(pthread_join(consumer,NULL)==0,"close waits for simulated callbacks");
    int16_t out[8]; for (unsigned i=0;i<8;++i) out[i]=99;
    s->data(s,s->data_context,out,4);
    for (unsigned i=0;i<8;++i) check(out[i]==0,"callbacks silence output during teardown");
    return AAUDIO_OK;
}
int32_t AAudioStream_getSampleRate(AAudioStream *s) { return s->rate; }
int32_t AAudioStream_getChannelCount(AAudioStream *s) { return s->channels; }
int32_t AAudioStream_getFormat(AAudioStream *s) { return s->format; }
int32_t AAudioStream_getFramesPerBurst(AAudioStream *s) { (void)s; return burst; }
int32_t AAudioStream_getBufferCapacityInFrames(AAudioStream *s) { (void)s; return capacity; }
int32_t AAudioStream_getXRunCount(AAudioStream *s) { return s->xruns; }
int32_t AAudioStream_setBufferSizeInFrames(AAudioStream *s,int32_t frames) {
    ++buffers; if (frames>capacity) frames=capacity; s->buffer=frames; return frames;
}
static void emit(int16_t *out,int frames) { stream.data(&stream,stream.data_context,out,frames); }
static void samples(int16_t *out,unsigned frames,int value) {
    for (unsigned i=0;i<frames;++i) { out[i*2]=(int16_t)value; out[i*2+1]=(int16_t)-value; }
}
static int16_t pcm_value(unsigned frame) {
    unsigned bits = (frame * 109u + 31u) & 65535u;
    return (int16_t)(bits <= INT16_MAX ? (int)bits : (int)bits - 65536);
}
static void batch_priming_tests(void) {
    int16_t pcm[1200]; samples(pcm,600,1234);
    for (unsigned order=0;order<2;++order) {
        cr_audio a; cr_audio_init(&a); burst=192; capacity=4096;
        check(cr_audio_start(&a,32768),"GBA batch fixture prepares a stream");
        cr_audio_write(&a,pcm,0);
        check(!a.playing && atomic_load(&a.write_frame)==0,"empty PCM cannot start playback or change queue occupancy");
        unsigned first=order ? 549 : 548, second=1097-first;
        cr_audio_write(&a,pcm,first*2);
        check(!a.playing,"one emulated frame does not cover the longest display interval");
        cr_audio_write(&a,pcm,second*2);
        check(a.playing && atomic_load(&a.write_frame)-atomic_load(&a.read_frame)==1097,
              "two fractional GBA batches start in either order without waiting a third frame");
        check(stream.buffer==384,"software priming leaves the hardware buffer at two bursts");
        cr_audio_close(&a);
        check(cr_audio_start(&a,48000) && a.prime_frames==384,
              "fresh rate and stream discard the previous batch priming budget");
        cr_audio_close(&a);
    }
    cr_audio a;cr_audio_init(&a);burst=8;capacity=256;
    check(cr_audio_start(&a,32768),"large first batch fixture prepares a stream");
    int16_t *full=malloc(CR_AUDIO_RING_FRAMES*2*sizeof(*full));
    check(full!=NULL,"allocate full queue for priming bound");
    samples(full,CR_AUDIO_RING_FRAMES,1234);cr_audio_write(&a,full,CR_AUDIO_RING_FRAMES*2);
    check(a.playing && a.prime_frames<=(int)CR_AUDIO_RING_FRAMES,
          "an oversized first batch is bounded by queue capacity and cannot wait forever");
    free(full);cr_audio_close(&a);
}
static void gain_quality_tests(void) {
    cr_audio a;cr_audio_init(&a);burst=8;capacity=256;
    int16_t block[16]={0},out[32];
    check(cr_audio_start(&a,32768),"full-range gain fixture prepares a stream");
    cr_audio_write(&a,block,16);cr_audio_write(&a,block,16);emit(out,16);
    const int volumes[]={0,1,37,50,99,100};
    for(size_t v=0;v<sizeof(volumes)/sizeof(*volumes);++v) {
        int volume=volumes[v];bool correct=true;
        cr_audio_set_volume(&a,volume);
        for(unsigned first=0;first<65536;first+=8) {
            for(unsigned i=0;i<8;++i) {
                block[i*2]=(int16_t)((int)(first+i)-32768);
                block[i*2+1]=(int16_t)(32767-(int)(first+i));
            }
            cr_audio_write(&a,block,16);emit(out,8);
            for(unsigned i=0;i<16;++i)
                if(out[i]!=(int16_t)((int64_t)block[i]*volume/100))correct=false;
        }
        check(correct,"every signed16bit value and both channels retain exact bounded gain without overflow or clipping");
    }
    cr_audio_close(&a);
}
static void timed_quality_tests(void) {
    const int bursts[]={64,128,192,256,512};
    for(size_t b=0;b<sizeof(bursts)/sizeof(*bursts);++b) {
        cr_audio a;cr_audio_init(&a);burst=bursts[b];capacity=4096;
        int16_t block[1200],out[1024];
        double deadline=0,next_callback=0;
        const double period=280896.0/16777216.0;
        unsigned delivered=0,consumed=0,silence=0;
        bool started=false,correct=true;
        for(unsigned tick=0;tick<3600;++tick) {
            double time=(double)tick/60.0;
            while(started && next_callback<time) {
                emit(out,burst);
                for(int i=0;i<burst;++i) {
                    int16_t left=out[i*2],right=out[i*2+1];
                    if(!left && !right) {++silence;continue;}
                    int16_t expected=pcm_value(consumed++);
                    if(left!=expected || right!=(int16_t)(-1-(int)expected))correct=false;
                }
                next_callback+=(double)burst/32768.0;
            }
            if(time+1e-12>=deadline) {
                unsigned previous=(unsigned)((uint64_t)delivered*280896/512);
                unsigned next=(unsigned)((uint64_t)(delivered+1)*280896/512);
                for(unsigned i=previous;i<next;++i) {
                    int16_t left=pcm_value(i);block[(i-previous)*2]=left;
                    block[(i-previous)*2+1]=(int16_t)(-1-(int)left);
                }
                check(cr_audio_start(&a,32768),"sixty-second scheduled stream remains available");
                cr_audio_write(&a,block,(next-previous)*2);++delivered;deadline+=period;
                if(!started && a.playing) {started=true;next_callback=time;}
            }
        }
        check(started && delivered==3583 && consumed>1900000,
              "quality fixture covers sixty seconds of exact GBA cadence on a 60Hz display");
        check(!silence,"60Hz delivery jitter never inserts silent frames into the exact GBA PCM stream");
        check(correct,"full-range nonidentical stereo samples retain exact order and values through every ring wrap");
        check(stream.xruns==0 && stream.buffer==burst*2,"quality does not depend on a fabricated hardware xrun or buffer growth");
        cr_audio_close(&a);
    }
    burst=8;capacity=256;
}
int main(void) {
    cr_audio a; cr_audio_init(&a);
    atomic_init(&consume,false); atomic_init(&callback_errors,0);
    check(!cr_audio_start(&a,0) && opens==0,"invalid rate never opens a device");
    fail_create=true;
    check(!cr_audio_start(&a,32768) && opens==0,"builder creation failure remains contained");
    fail_create=false;
    check(!cr_audio_start(&a,32768) && opens==0,"builder creation failure also has bounded retry");
    clock_ns+=INT64_C(250000000);
    check(cr_audio_start(&a,32768) && opens==1 && starts==0,"start prepares but does not start an empty stream");
    check(a.prime_frames==16 && stream.buffer==16,"device buffer and PCM priming follow two bursts");
    int16_t pcm[64],out[80]; samples(pcm,32,30000);
    cr_audio_write(&a,pcm,17); check(starts==0,"odd trailing sample cannot break stereo or prematurely start");
    cr_audio_write(&a,pcm+16,16); check(starts==1 && a.playing,"enough complete stereo frames start playback");
    emit(out,20);
    for (unsigned i=0;i<16;++i) check(out[i*2]==30000 && out[i*2+1]==-30000,"primed PCM stays correctly paired");
    for (unsigned i=32;i<40;++i) check(out[i]==0,"underrun fills silence without old samples");
    cr_audio_set_volume(&a,50); cr_audio_write(&a,pcm,64); emit(out,8);
    check(out[0]==15000 && out[1]==-15000,"callback applies current volume");
    cr_audio_set_volume(&a,0); emit(out,8); check(out[0]==0 && out[1]==0,"mute preserves callback safety");
    cr_audio_set_volume(&a,100);
    int before=buffers; ++stream.xruns; check(cr_audio_start(&a,32768),"stream stays ready during an xrun");
    check(buffers==before+1 && stream.buffer==24,"xrun grows by only one burst");
    for (unsigned i=0;i<30;++i) { ++stream.xruns; check(cr_audio_start(&a,32768),"xrun adaptation remains usable"); }
    check(stream.buffer==64,"adaptive device buffer is bounded to eight bursts");
    cr_audio_pause(&a); check(!a.stream && !a.playing && atomic_load(&a.read_frame)==0 && atomic_load(&a.write_frame)==0,
                              "pause closes the asynchronous callback before resetting PCM");
    check(cr_audio_start(&a,32768),"resume prepares a fresh stream");
    check(!a.playing,"resume requires fresh PCM");
    cr_audio_write(&a,pcm,64); check(!a.playing,"resume also waits for two submitted PCM batches");
    cr_audio_write(&a,pcm,64); emit(out,4); check(out[0]==30000,"resume contains only new samples");
    before=opens; int old_closes=closes;
    AAudioStream_errorCallback old_error=stream.error;
    void *old_context=stream.error_context;
    stream.error(&stream,stream.error_context,-899);
    check(closes==old_closes,"error callback never closes on the realtime thread");
    check(!cr_audio_start(&a,32768) && closes==old_closes+1,"owner notices disconnect and closes safely");
    for (unsigned i=0;i<30;++i) check(!cr_audio_start(&a,32768),"recovery backs off while a device is unavailable");
    check(opens==before,"backoff avoids opening once per emulated frame");
    clock_ns+=INT64_C(250000000); check(cr_audio_start(&a,32768) && opens==before+1,"retry opens after the bounded delay");
    cr_audio_write(&a,pcm,64); cr_audio_write(&a,pcm,64); emit(out,4); check(out[0]==30000,"recovered stream is primed with new PCM");
    before=opens;
    old_error(&stream,old_context,-899);
    check(cr_audio_start(&a,32768) && opens==before,"late old-stream error cannot invalidate the replacement stream");
    stream.error(&stream,stream.error_context,-899);
    old_error(&stream,old_context,-899);
    check(!cr_audio_start(&a,32768),"late old error cannot overwrite a real current-stream failure");
    clock_ns+=INT64_C(250000000);
    check(cr_audio_start(&a,32768),"current-stream failure remains recoverable");
    cr_audio_close(&a); fail_open=true; before=opens;
    check(!cr_audio_start(&a,32768) && opens==before+1,"open failure is reported");
    check(!cr_audio_start(&a,32768) && opens==before+1,"open failure is throttled too");
    fail_open=false; clock_ns+=INT64_C(250000000); check(cr_audio_start(&a,32768),"open failure recovery");
    fail_start=true; cr_audio_write(&a,pcm,64); cr_audio_write(&a,pcm,64); check(!a.stream && !a.playing,"start failure closes a primed stream");
    check(!cr_audio_start(&a,32768),"start failure gets the same backoff");
    fail_start=false; clock_ns+=INT64_C(250000000); wrong_rate=true;
    check(!cr_audio_start(&a,32768) && !a.stream,"format negotiation mismatch never feeds the wrong sample rate");
    wrong_rate=false; clock_ns+=INT64_C(250000000); check(cr_audio_start(&a,32768),"format mismatch recovery");
    check(cr_audio_start(&a,48000) && a.rate==48000,"content sample-rate change prepares a fresh stream");
    cr_audio_close(&a); burst=0;
    check(cr_audio_start(&a,32768) && a.burst_frames>0,"unavailable burst metadata has a finite fallback");
    cr_audio_close(&a); burst=8;
    check(cr_audio_start(&a,32768),"overflow fixture opens");
    cr_audio_write(&a,pcm,16); cr_audio_write(&a,pcm,16); emit(out,16);
    unsigned initial=UINT_MAX-7u;
    atomic_store(&a.read_frame,initial); atomic_store(&a.write_frame,initial);
    cr_audio_write(&a,pcm,64); emit(out,32);
    check(atomic_load(&a.read_frame)==initial+32u && out[0]==30000,"unsigned counter wrap preserves FIFO stereo frames");
    int16_t *large=malloc(CR_AUDIO_RING_FRAMES*2*sizeof(*large)); check(large!=NULL,"allocate bounded host test PCM");
    samples(large,CR_AUDIO_RING_FRAMES,1234); cr_audio_write(&a,large,CR_AUDIO_RING_FRAMES*2);
    unsigned read=atomic_load(&a.read_frame),write=atomic_load(&a.write_frame);
    cr_audio_write(&a,pcm,64);
    check(atomic_load(&a.read_frame)==read && atomic_load(&a.write_frame)==write,"overflow never steals the callback's read counter");
    emit(out,8); check(out[0]==1234 && out[1]==-1234,"overflow keeps previously queued stereo data");
    free(large); cr_audio_close(&a);
    threaded=true; check(cr_audio_start(&a,32768),"concurrent callback fixture opens");
    cr_audio_write(&a,pcm,64);
    for (unsigned i=0;i<12000;++i) { samples(pcm,32,(int)(i%20000+1)); cr_audio_write(&a,pcm,64); }
    cr_audio_pause(&a);
    check(atomic_load(&callback_errors)==0,"concurrent producer and callback retain stereo pairs through wraps and drops");
    threaded=false; cr_audio_destroy(&a);
    cr_audio *temporary=malloc(sizeof(*temporary));
    check(temporary!=NULL,"allocate temporary audio owner");
    cr_audio_init(temporary); check(cr_audio_start(temporary,32768),"temporary owner opens an audio stream");
    AAudioStream_errorCallback late_error=stream.error;
    void *late_context=stream.error_context;
    cr_audio_destroy(temporary); free(temporary);
    late_error(&stream,late_context,-899);
    check(true,"late error after owner destruction never dereferences freed memory");
    cr_audio *replacement=malloc(sizeof(*replacement));
    check(replacement!=NULL,"allocate replacement audio owner");
    cr_audio_init(replacement); check(cr_audio_start(replacement,32768),"replacement owner opens a fresh stream");
    before=opens;
    late_error(&stream,late_context,-899);
    check(cr_audio_start(replacement,32768) && opens==before,"destroyed owner's callback cannot fail a new owner");
    cr_audio_destroy(replacement); free(replacement);
    late_error(&stream,late_context,-899);
    check(stops==closes && deletes==opens,"all opened streams/builders are closed through lifecycle and failures");
    batch_priming_tests(); gain_quality_tests(); timed_quality_tests();
    printf("PASS %u AAudio priming, realtime FIFO, recovery, volume, xrun and lifecycle checks\n",checks);
    return 0;
}
