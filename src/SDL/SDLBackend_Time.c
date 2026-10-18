#include "SDLBackend.h"
#include "mechwarrior/campaign_input.h"
#include "mechwarrior/combat_sound.h"
#include "mechwarrior/combat_startup.h"
#include <time.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* User-approved playable budget; see the human speed review in CPU_TIMING_PROFILE.md.
 * Keep standalone startup consistent with the supplied MechWarrior.ini. */
uint32_t mwSdlCpuCyclesPerMillisecond=2500;
/* Original x86 replay of paused cockpit RAM; see CPU_TIMING_PROFILE.md.
 * This is a measured reference workload, not an interpreter for arbitrary
 * branches. Live aperture traffic scales the dominant rendering cost. */
enum { REFERENCE_LOGIC_CYCLES=24080,REFERENCE_RENDER_CYCLES=263005,
    REFERENCE_VIDEO_BYTES=14154,REFERENCE_VIDEO_READ_NS=1000,
    REFERENCE_VIDEO_WRITE_NS=750 };
static bool combatCpuActive;
static uint64_t cpuIterationStarted,videoReadBytes,videoWriteBytes,lastCpuCycles;

void MwSdlResetCombatCpu(void)
{combatCpuActive=false;videoReadBytes=videoWriteBytes=lastCpuCycles=0;}
void MwSdlAccountCombatVideoAccess(bool write)
{
    if (!combatCpuActive) return;
    if (write) ++videoWriteBytes;else ++videoReadBytes;
}
uint64_t MwSdlLastCombatCpuCycles(void) {return lastCpuCycles;}
static void WaitCpuDeadline(uint64_t deadline)
{
    while (SDL_GetTicksNS()<deadline)
    {MwSdlPollEvents();MwSdlPumpTimer();SDL_DelayNS(100000);}
}
void MwSdlPaceCombatCpu(void)
{
    if (combatCpuActive)
    {
        lastCpuCycles=REFERENCE_LOGIC_CYCLES+
            (videoReadBytes+videoWriteBytes)*REFERENCE_RENDER_CYCLES/REFERENCE_VIDEO_BYTES;
        uint64_t duration=lastCpuCycles*UINT64_C(1000000)/mwSdlCpuCyclesPerMillisecond;
        /* Separate EGA bus time, not an extra multiplier on game motion. */
        duration+=videoReadBytes*REFERENCE_VIDEO_READ_NS+videoWriteBytes*REFERENCE_VIDEO_WRITE_NS;
        WaitCpuDeadline(cpuIterationStarted+duration);
    }
    /* No accumulated catch-up debt after loading, a native wait or a slow
     * host. Explicit original delays already consumed their wall time. */
    cpuIterationStarted=SDL_GetTicksNS();combatCpuActive=true;
    videoReadBytes=videoWriteBytes=0;
    MwSdlUpdateSupportPilot(); /* Optional host input at the existing loop boundary. */
}
void MwSdlDelayCpuLoops(uint16_t loops)
{WaitCpuDeadline(SDL_GetTicksNS()+(uint64_t)loops*UINT64_C(1000000)/mwSdlCpuCyclesPerMillisecond);}

bool MwSdlLoadTimingConfiguration(const char *path,bool optional)
{
    FILE *file=fopen(path,"r");
    if (!file) return optional && errno==ENOENT;
    char line[256];
    uint32_t cyclesPerMillisecond=mwSdlCpuCyclesPerMillisecond;
    bool valid=true;
    while (fgets(line,sizeof(line),file))
    {
        char *key=line;
        while (isspace((unsigned char)*key)) ++key;
        if (!*key || *key==';' || *key=='#') continue;
        char *value=strchr(key,'=');
        if (!value) {valid=false;break;}
        char *keyEnd=value;
        while (keyEnd>key && isspace((unsigned char)keyEnd[-1])) --keyEnd;
        *keyEnd=0;++value;
        while (isspace((unsigned char)*value)) ++value;
        if (strcmp(key,"cpu_cycles_per_ms") || !isdigit((unsigned char)*value))
        {valid=false;break;}
        char *end;
        errno=0;unsigned long rate=strtoul(value,&end,10);
        while (isspace((unsigned char)*end)) ++end;
        if (errno || *end || rate<100 || rate>100000) {valid=false;break;}
        cyclesPerMillisecond=(uint32_t)rate;
    }
    if (ferror(file)) valid=false;
    if (fclose(file)) valid=false;
    if (valid) mwSdlCpuCyclesPerMillisecond=cyclesPerMillisecond;
    return valid;
}

static MwMusicTimerHandler irq0;
static bool interruptsEnabled,campaignInterruptsInstalled,inInterrupt;
static uint64_t startNanoseconds;
static double hardwareTime,nextIrq;
static uint32_t channel0Divisor,biosTicks;
static uint8_t channel0Low;
static bool channel0HighNext;
static uint8_t picMask;
static uint16_t latchedCounter;
static bool counterReadHigh;
static uint64_t counterLoadNanoseconds;

static void BiosTimerInterrupt(void)
{
    ++biosTicks;
    if (biosTicks>=0x1800B0) biosTicks=0;
    if (campaignInterruptsInstalled) MwCampaignTimerTick();
}
void MwSdlSetInterrupts(bool enabled) {interruptsEnabled=enabled;}
MwMusicTimerHandler MwSdlReadTimerHandler(void) {return irq0;}
void MwSdlWriteTimerHandler(MwMusicTimerHandler handler) {irq0=handler;}
uint32_t MwSdlBiosTicksValue(void) {return biosTicks;}
uint8_t MwSdlExchangeInterrupts(uint8_t enabled)
{
    if (interruptsEnabled && !enabled && !inInterrupt) MwSdlPump();
    uint8_t previous=interruptsEnabled;interruptsEnabled=enabled!=0;return previous;
}
double MwSdlHardwareTime(void) {return hardwareTime;}
double MwSdlTimerLagMilliseconds(void)
{
    double elapsed=(double)(SDL_GetTicksNS()-startNanoseconds)/1.0e6;
    return elapsed-hardwareTime*1000/HOST_PIT_CLOCK_HZ;
}
void MwSdlWriteDevicePort(uint16_t port,uint8_t value)
{
    /* Device writes occur at the delivered hardware time, including inside
     * an IRQ. No game state is ever touched by an SDL audio thread. */
    if (MwSdlWriteCombatVideoPort(port,value)) return;
    MwSdlAdvanceAudio(hardwareTime);
    if (port==PIC_COMMAND_PORT && value==PIC_END_OF_INTERRUPT) return;
    if (port==PIC_MASK_PORT) {picMask=value;return;}
    if (port==ADLIB_INDEX_PORT || port==ADLIB_DATA_PORT || port==0x330 || port==0x331 || port==GAME_PORT) return; /* Unconnected devices. */
    if (port==PIT_CONTROL_PORT && !(value&0xF0))
    {
        uint64_t cycles=(uint64_t)((double)(SDL_GetTicksNS()-counterLoadNanoseconds)*HOST_PIT_CLOCK_HZ/1.0e9);
        latchedCounter=(uint16_t)(channel0Divisor-(cycles*2)%channel0Divisor);counterReadHigh=false;return;
    }
    if (port==PIT_CONTROL_PORT && value==PIT_CHANNEL0_SQUARE_WAVE)
    {channel0HighNext=false;return;}
    if (port==PIT_CHANNEL0_PORT)
    {
        if (!channel0HighNext) {channel0Low=value;channel0HighNext=true;return;}
        uint16_t divisor=channel0Low|((uint16_t)value<<8);
        channel0Divisor=divisor?divisor:0x10000;
        counterLoadNanoseconds=SDL_GetTicksNS();
        channel0HighNext=false;nextIrq=hardwareTime+channel0Divisor;
        return;
    }
    MwSdlWriteSpeakerPort(port,value);
}
uint8_t MwSdlReadDevicePort(uint16_t port)
{
    uint8_t videoValue;if (MwSdlReadCombatVideoPort(port,&videoValue)) return videoValue;
    if (port==KEYBOARD_DATA_PORT) return MwSdlReadKeyboardScanByte();
    if (port==PIC_MASK_PORT) return picMask;
    if (port==ADLIB_INDEX_PORT || port==0x330 || port==0x331 || port==GAME_PORT) return 0xFF;
    if (port==PIT_CHANNEL0_PORT)
    {uint8_t value=counterReadHigh?(uint8_t)(latchedCounter>>8):(uint8_t)latchedCounter;counterReadHigh=!counterReadHigh;return value;}
    if (port!=SPEAKER_CONTROL_PORT) MwSdlFail("Unsupported hardware read");
    return MwSdlReadSpeakerControl();
}
void MwSdlInitializeTimer(void)
{
    MwSdlResetCombatCpu();
    time_t now=time(NULL);struct tm localTime;
#if defined(_MSC_VER)
    localtime_s(&localTime,&now);
#else
    localTime=*localtime(&now);
#endif
    uint32_t seconds=localTime.tm_hour*3600+localTime.tm_min*60+localTime.tm_sec;
    biosTicks=(uint32_t)((double)seconds*HOST_PIT_CLOCK_HZ/65536.0);
    startNanoseconds=SDL_GetTicksNS();hardwareTime=0;channel0Divisor=0x10000;
    nextIrq=channel0Divisor;irq0=BiosTimerInterrupt;interruptsEnabled=true;
    counterLoadNanoseconds=startNanoseconds;latchedCounter=0;counterReadHigh=false;picMask=0;
    campaignInterruptsInstalled=inInterrupt=channel0HighNext=false;
    mwMusicHardware=(MwMusicHardwareCalls){MwSdlSetInterrupts,MwSdlReadTimerHandler,MwSdlWriteTimerHandler,MwSdlWriteDevicePort,MwSdlReadDevicePort};
    /* Same physical PIT/speaker, different original sequencer. No backend
     * selection or invented combat callback is installed at this boundary. */
    btechHostWritePort=MwSdlWriteDevicePort;btechHostReadPort=MwSdlReadDevicePort;
}
void MwSdlPumpTimer(void)
{
    if (inInterrupt) return;
    if (interruptsEnabled && !(picMask&2) && MwSdlCombatKeyboardPending())
    {
        inInterrupt=true;interruptsEnabled=false;
        MwSdlDeliverCombatKeyboardInterrupt();
        interruptsEnabled=true;inInterrupt=false;
    }
    double target=(double)(SDL_GetTicksNS()-startNanoseconds)*HOST_PIT_CLOCK_HZ/1.0e9;
    while (interruptsEnabled && !(picMask&1) && nextIrq<=target)
    {
        /* BTECH's installed scheduler must service every elapsed PIT edge,
         * independently of presentation/poll frequency. Keep each original
         * callback and its timestamp; do not batch its game-state writes.
         * Campaign1610 instead waits for exact tick equality, so retain one
         * edge per caller observation for that separate timer handler. */
        bool combatScheduler=irq0==BtechTimerSchedulerInterrupt;
        hardwareTime=nextIrq;nextIrq+=channel0Divisor;
        MwSdlAdvanceAudio(hardwareTime);
        inInterrupt=true;bool savedInterrupts=interruptsEnabled;interruptsEnabled=false;
        if (!irq0) MwSdlFail("Unmapped IRQ0 handler");
        irq0();interruptsEnabled=savedInterrupts;inInterrupt=false;
        if (!combatScheduler || irq0!=BtechTimerSchedulerInterrupt) return;
    }
    if (nextIrq>target)
    {
        hardwareTime=target;MwSdlAdvanceAudio(hardwareTime);
        SDL_DelayNS(100000); /* Host idle only; no synthetic game ticks. */
    }
}
void MwSdlInstallCampaignInterrupts(void) {campaignInterruptsInstalled=true;}
void MwSdlRestoreCampaignInterrupts(void) {campaignInterruptsInstalled=false;}
uint32_t MwSdlReadBiosTicks(void) {MwSdlPump();return biosTicks;}
