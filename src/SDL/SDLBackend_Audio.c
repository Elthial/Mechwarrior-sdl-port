#include "SDLBackend.h"
#include "mechwarrior/audio_devices.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

/* Hardware PCM only: PIT channel2 speaker plus the selected external card.
 * MIDI scheduling, allocation, instruments and rotation remain Original code;
 * the host never plays an SMF itself. Speaker Fourier integration suppresses
 * ultrasonic native period0014 rather
 * than aliasing its59kHz fundamental into the audible48kHz output. This is a
 * bounded32-harmonic digital speaker model, not verified analogue acoustics. */
static SDL_AudioStream *audioStream;
static SDL_Mutex *audioQueueMutex;
enum { AUDIO_QUEUE_FRAMES=0x4000,AUDIO_START_LEAD_FRAMES=0x0800 };
static float audioQueue[AUDIO_QUEUE_FRAMES][2];
static uint32_t outputChannels=1;
static uint64_t generatedFrames,playbackFrames,lateFrames,underrunFrames;
static uint64_t audioStartedNanoseconds,clockSkippedFrames;
static uint32_t playbackLeadFrames;
static uint32_t leadFrames;
static uint32_t speakerDivisor;
static uint8_t speakerControl,periodLow;
static bool highByteNext,counting;
static double audioTime,speakerPhase,sampleCycles,sampleIntegral;
static float queuedSamples[512];
static uint32_t queuedCount;
void (*mwSdlAudioObserver)(const float *samples,uint32_t count);

/* SDL asks for PCM only. It never runs an original timer/game callback.
 * An underrun advances the physical playback clock, just like silence from
 * an unfed device. The producer must not replay those elapsed frames later. */
static void SDLCALL FeedAudio(void *unused,SDL_AudioStream *stream,int additionalBytes,int totalBytes)
{
    (void)unused;(void)totalBytes;
    /* Hardware sample demand and SDL's monotonic PIT clock are independent.
     * In particular dummy WaitDevice uses rounded millisecond delays. Advance
     * a lagging device cursor to its timestamp instead of accumulating minutes
     * of drift into a fatal queue overflow. Already elapsed PCM stays elapsed. */
    uint64_t elapsed = SDL_GetTicksNS() - audioStartedNanoseconds;
    uint64_t physicalFrames = elapsed / UINT64_C(1000000000) * HOST_AUDIO_SAMPLE_RATE
        + elapsed % UINT64_C(1000000000) * HOST_AUDIO_SAMPLE_RATE / UINT64_C(1000000000);
    SDL_LockMutex(audioQueueMutex);
    if (!leadFrames && physicalFrames > playbackLeadFrames
        && physicalFrames - playbackLeadFrames > playbackFrames)
    {
        uint64_t currentFrame = physicalFrames - playbackLeadFrames;
        clockSkippedFrames += currentFrame - playbackFrames;
        playbackFrames = currentFrame;
    }
    SDL_UnlockMutex(audioQueueMutex);
    float samples[512];
    while (additionalBytes>0)
    {
        uint32_t count=(uint32_t)additionalBytes/(sizeof(float)*outputChannels);
        if (count>256) count=256;
        if (!count) return;
        SDL_LockMutex(audioQueueMutex);
        for (uint32_t index=0;index<count;++index)
        {
            for (uint32_t channel=0;channel<outputChannels;++channel)
                samples[index*outputChannels+channel]=!leadFrames && playbackFrames<generatedFrames
                    ?audioQueue[playbackFrames%AUDIO_QUEUE_FRAMES][channel]:0;
            if (leadFrames) {--leadFrames;continue;}
            if (playbackFrames>=generatedFrames) ++underrunFrames;
            ++playbackFrames;
        }
        SDL_UnlockMutex(audioQueueMutex);
        if (!SDL_PutAudioStreamData(stream,samples,(int)(count*sizeof(float)*outputChannels))) MwSdlFail("Feed audio PCM");
        additionalBytes-=(int)(count*sizeof(float)*outputChannels);
    }
}

bool MwSdlInitializeAudio(void)
{
    outputChannels=mwSdlSoundSystem==MW_SOUND_SPEAKER?1:2;
    if (mwSdlSoundSystem==MW_SOUND_ADLIB) MwAdlibReset(HOST_AUDIO_SAMPLE_RATE);
    if (mwSdlSoundSystem==MW_SOUND_MT32)
    {
        char error[1024];
        if (!MwMt32Open(mwSdlMt32ControlRom,mwSdlMt32PcmRom,HOST_AUDIO_SAMPLE_RATE,error,sizeof(error)))
            return SDL_SetError("%s",error);
    }
    SDL_AudioSpec format={SDL_AUDIO_F32,(int)outputChannels,HOST_AUDIO_SAMPLE_RATE};
    audioQueueMutex=SDL_CreateMutex();
    if (!audioQueueMutex) return false;
    generatedFrames=playbackFrames=lateFrames=underrunFrames=clockSkippedFrames=0;leadFrames=AUDIO_START_LEAD_FRAMES;
    audioStream=SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,&format,FeedAudio,NULL);
    if (!audioStream) return false;
    SDL_AudioSpec deviceFormat;int deviceFrames;
    if (!SDL_GetAudioDeviceFormat(SDL_GetAudioStreamDevice(audioStream),&deviceFormat,&deviceFrames)) return false;
    uint64_t deviceLead=(uint64_t)deviceFrames*HOST_AUDIO_SAMPLE_RATE*2/deviceFormat.freq;
    if (deviceLead>=AUDIO_QUEUE_FRAMES) return SDL_SetError("Audio device buffer exceeds bounded PCM capacity");
    if (deviceLead>leadFrames) leadFrames=(uint32_t)deviceLead;
    playbackLeadFrames = leadFrames;
    audioStartedNanoseconds = SDL_GetTicksNS();
    if (!SDL_ResumeAudioStreamDevice(audioStream)) return false;
    if (getenv("MECHWARRIOR_DIAGNOSTICS"))
    {
        fprintf(stderr,"Audio device: %d Hz, %d channels, %d frames (%.2f ms)\n",
            deviceFormat.freq,deviceFormat.channels,deviceFrames,1000.0*deviceFrames/deviceFormat.freq);
    }
    audioTime=speakerPhase=sampleCycles=sampleIntegral=0;queuedCount=0;
    speakerControl=0;speakerDivisor=0x10000;highByteNext=counting=false;
    return true;
}
void MwSdlAdvanceAudio(double pitTime)
{
    const double cyclesPerSample=(double)HOST_PIT_CLOCK_HZ/HOST_AUDIO_SAMPLE_RATE;
    const double pi=3.14159265358979323846;
    while (audioTime<pitTime)
    {
        double duration=fmin(pitTime-audioTime,cyclesPerSample-sampleCycles);
        if (counting && (speakerControl&SPEAKER_GATE_BITS)==SPEAKER_GATE_BITS)
        {
            double duty=(double)((speakerDivisor+1)/2)/speakerDivisor;
            double frequency=(double)HOST_PIT_CLOCK_HZ/speakerDivisor;
            for (uint32_t harmonic=1;harmonic<=32 && harmonic*frequency<HOST_AUDIO_SAMPLE_RATE/2.0;++harmonic)
            {
                double angular=2*pi*harmonic/speakerDivisor;
                double start=angular*speakerPhase-pi*harmonic*duty;
                double amplitude=4*sin(pi*harmonic*duty)/(pi*harmonic);
                sampleIntegral+=amplitude*(sin(start+angular*duration)-sin(start))/angular;
            }
        }
        if (counting && (speakerControl&1)) speakerPhase=fmod(speakerPhase+duration,speakerDivisor);
        audioTime+=duration;sampleCycles+=duration;
        if (sampleCycles>=cyclesPerSample-1e-8)
        {
            float speaker=(float)(0.15*sampleIntegral/cyclesPerSample);
            if (mwSdlSoundSystem!=MW_SOUND_SPEAKER)
            {
                if (mwSdlSoundSystem==MW_SOUND_MT32) MwMt32Render(&queuedSamples[queuedCount*outputChannels],1);
                else MwAdlibRender(&queuedSamples[queuedCount*outputChannels],1);
                queuedSamples[queuedCount*outputChannels]+=speaker;
                queuedSamples[queuedCount*outputChannels+1]+=speaker;
            }
            else queuedSamples[queuedCount]=speaker;
            ++queuedCount;
            sampleCycles=sampleIntegral=0;
            if (queuedCount==256)
            {
                if (mwSdlAudioObserver) mwSdlAudioObserver(queuedSamples,queuedCount*outputChannels);
                SDL_LockMutex(audioQueueMutex);
                for (uint32_t index=0;index<queuedCount;++index,++generatedFrames)
                {
                    if (generatedFrames<playbackFrames) {++lateFrames;continue;}
                    if (generatedFrames-playbackFrames>=AUDIO_QUEUE_FRAMES)
                    {
                        SDL_UnlockMutex(audioQueueMutex);
                        MwSdlFail("Audio producer advanced beyond bounded timeline");
                    }
                    for (uint32_t channel=0;channel<outputChannels;++channel)
                        audioQueue[generatedFrames%AUDIO_QUEUE_FRAMES][channel]=queuedSamples[index*outputChannels+channel];
                }
                SDL_UnlockMutex(audioQueueMutex);
                queuedCount=0;
            }
        }
    }
}
void MwSdlWriteSpeakerPort(uint16_t port,uint8_t value)
{
    if (port==PIT_CONTROL_PORT && value==PIT_CHANNEL2_SQUARE_WAVE)
    {highByteNext=false;counting=false;speakerPhase=0;return;}
    if (port==PIT_CHANNEL2_PORT)
    {
        if (!highByteNext) {periodLow=value;highByteNext=true;return;}
        uint16_t period=periodLow|((uint16_t)value<<8);
        speakerDivisor=period?period:0x10000;
        highByteNext=false;counting=true;speakerPhase=0;return;
    }
    if (port==SPEAKER_CONTROL_PORT)
    {
        if (!(value&1)) speakerPhase=0; /* Gate-low holds mode3 counter. */
        speakerControl=value;return;
    }
    MwSdlFail("Unsupported speaker hardware write");
}
uint8_t MwSdlReadSpeakerControl(void) {return speakerControl;}
uint32_t MwSdlQueuedAudioMilliseconds(void)
{
    int bytes=SDL_GetAudioStreamQueued(audioStream);
    if (bytes<0) MwSdlFail("Read audio queue duration");
    SDL_LockMutex(audioQueueMutex);
    uint64_t pending=leadFrames+(generatedFrames>playbackFrames?generatedFrames-playbackFrames:0);
    SDL_UnlockMutex(audioQueueMutex);
    return (uint32_t)((pending+(uint32_t)bytes/(sizeof(float)*outputChannels))*1000/HOST_AUDIO_SAMPLE_RATE);
}
uint64_t MwSdlLateAudioFrames(void)
{SDL_LockMutex(audioQueueMutex);uint64_t frames=lateFrames;SDL_UnlockMutex(audioQueueMutex);return frames;}
uint64_t MwSdlAudioUnderrunFrames(void)
{SDL_LockMutex(audioQueueMutex);uint64_t frames=underrunFrames;SDL_UnlockMutex(audioQueueMutex);return frames;}
uint64_t MwSdlAudioClockSkippedFrames(void)
{SDL_LockMutex(audioQueueMutex);uint64_t frames=clockSkippedFrames;SDL_UnlockMutex(audioQueueMutex);return frames;}
void MwSdlShutdownAudio(void)
{
    if (audioStream) SDL_DestroyAudioStream(audioStream);
    audioStream=NULL;
    SDL_DestroyMutex(audioQueueMutex);audioQueueMutex=NULL;
    MwMt32Close();
}
