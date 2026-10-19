#include "SDLBackend.h"
#include "mechwarrior/audio_devices.h"
#include "mechwarrior/campaign_input.h"
#include "mechwarrior/combat_sound.h"
#include "mechwarrior/combat_startup.h"
#include "mechwarrior/combat_mpu.h"
#include <float.h>
#include <time.h>

/* User-approved playable budget; see the human speed review in CPU_TIMING_PROFILE.md.
 * Keep standalone startup consistent with the supplied MechWarrior.ini. */
enum { HOST_DEFAULT_CPU_CYCLES_PER_MS=2500,HOST_IDLE_WAIT_NS=100000,
    HOST_NS_PER_MILLISECOND=1000000,HOST_NS_PER_SECOND=1000000000,
    HOST_MS_PER_SECOND=1000,HOST_SECONDS_PER_MINUTE=60,HOST_SECONDS_PER_HOUR=3600 };
uint32_t mwSdlCpuCyclesPerMillisecond=HOST_DEFAULT_CPU_CYCLES_PER_MS;
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
    {MwSdlPollEvents();MwSdlPumpTimer();SDL_DelayNS(HOST_IDLE_WAIT_NS);}
}
void MwSdlPaceCombatCpu(void)
{
    if (combatCpuActive)
    {
        lastCpuCycles=REFERENCE_LOGIC_CYCLES+
            (videoReadBytes+videoWriteBytes)*REFERENCE_RENDER_CYCLES/REFERENCE_VIDEO_BYTES;
        uint64_t duration=lastCpuCycles*HOST_NS_PER_MILLISECOND/mwSdlCpuCyclesPerMillisecond;
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
{WaitCpuDeadline(SDL_GetTicksNS()+(uint64_t)loops*HOST_NS_PER_MILLISECOND/mwSdlCpuCyclesPerMillisecond);}

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
    if (biosTicks>=BIOS_TICKS_PER_DAY) biosTicks=0;
    if (campaignInterruptsInstalled) MwCampaignTimerTick();
}
void MwSdlSetInterrupts(bool enabled) {interruptsEnabled=enabled;}
uint8_t MwSdlReadInterrupts(void) {return interruptsEnabled;}
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
    double elapsed=(double)(SDL_GetTicksNS()-startNanoseconds)/(double)HOST_NS_PER_MILLISECOND;
    return elapsed-hardwareTime*HOST_MS_PER_SECOND/HOST_PIT_CLOCK_HZ;
}
void MwSdlWriteDevicePort(uint16_t port,uint8_t value)
{
    /* Device writes occur at the delivered hardware time, including inside
     * an IRQ. No game state is ever touched by an SDL audio thread. */
    if (MwSdlWriteCombatVideoPort(port,value)) return;
    MwSdlAdvanceAudio(hardwareTime);
    if (port==PIC_COMMAND_PORT && value==PIC_END_OF_INTERRUPT) return;
    if (port==PIC_MASK_PORT) {picMask=value;return;}
    if (port==MPU_DATA_PORT || port==MPU_STATUS_COMMAND_PORT)
    {
        if (mwSdlSoundSystem==MW_SOUND_MT32) MwMpuWritePort(port,value);
        return;
    }
    if (port==ADLIB_INDEX_PORT || port==ADLIB_DATA_PORT)
    {
        if (mwSdlSoundSystem==MW_SOUND_ADLIB)
            MwAdlibWritePort(port==ADLIB_DATA_PORT,value,hardwareTime*1000000.0/HOST_PIT_CLOCK_HZ);
        return;
    }
    /* Native rhythm note-off can retain DX=0 from the VLQ reader. Those writes
     * reach unused DMA channel 0 ports, not the OPL card; never redirect them. */
    if (port==0 || port==1 || port==GAME_PORT) return;
    if (port==PIT_CONTROL_PORT && !(value&PIT_CONTROL_LATCH_TEST_MASK))
    {
        uint64_t cycles=(uint64_t)((double)(SDL_GetTicksNS()-counterLoadNanoseconds)*HOST_PIT_CLOCK_HZ/(double)HOST_NS_PER_SECOND);
        latchedCounter=(uint16_t)(channel0Divisor-(cycles*2)%channel0Divisor);counterReadHigh=false;return;
    }
    if (port==PIT_CONTROL_PORT && value==PIT_CHANNEL0_SQUARE_WAVE)
    {channel0HighNext=false;return;}
    if (port==PIT_CHANNEL0_PORT)
    {
        if (!channel0HighNext) {channel0Low=value;channel0HighNext=true;return;}
        uint16_t divisor=channel0Low|((uint16_t)value<<8);
        channel0Divisor=divisor?divisor:PIT_COUNTER_MODULUS;
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
    if (port==MPU_DATA_PORT || port==MPU_STATUS_COMMAND_PORT)
        return mwSdlSoundSystem==MW_SOUND_MT32?MwMpuReadPort(port):UINT8_MAX;
    if (port==ADLIB_INDEX_PORT)
        return mwSdlSoundSystem==MW_SOUND_ADLIB
            ?MwAdlibReadStatus(hardwareTime*1000000.0/HOST_PIT_CLOCK_HZ):UINT8_MAX;
    if (port==0 || port==1) return 0; /* Only ignored calibrated-delay reads here. */
    if (port==GAME_PORT) return UINT8_MAX;
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
    uint32_t seconds=localTime.tm_hour*HOST_SECONDS_PER_HOUR+localTime.tm_min*HOST_SECONDS_PER_MINUTE+localTime.tm_sec;
    biosTicks=(uint32_t)((double)seconds*HOST_PIT_CLOCK_HZ/(double)PIT_COUNTER_MODULUS);
    startNanoseconds=SDL_GetTicksNS();hardwareTime=0;channel0Divisor=PIT_COUNTER_MODULUS;
    nextIrq=channel0Divisor;irq0=BiosTimerInterrupt;interruptsEnabled=true;
    counterLoadNanoseconds=startNanoseconds;latchedCounter=0;counterReadHigh=false;picMask=0;
    campaignInterruptsInstalled=inInterrupt=channel0HighNext=false;
    MwMpuReset();
    mwMusicHardware=(MwMusicHardwareCalls){MwSdlSetInterrupts,MwSdlReadTimerHandler,MwSdlWriteTimerHandler,MwSdlWriteDevicePort,MwSdlReadDevicePort,
        MwSdlPumpTimer,btechHostGetInterruptVector,btechHostSetInterruptVector};
    /* Same physical PIT/speaker, different original sequencer. No backend
     * selection or invented combat callback is installed at this boundary. */
    btechHostWritePort=MwSdlWriteDevicePort;btechHostReadPort=MwSdlReadDevicePort;
}
void MwSdlPumpTimer(void)
{
    if (inInterrupt && !interruptsEnabled) return;
    if (!inInterrupt && interruptsEnabled && mwSdlCombatActive && mwSdlSoundSystem==MW_SOUND_MT32
        && !(picMask&BTECH_MPU_IRQ_MASK) && !(MwMpuReadPort(MPU_STATUS_COMMAND_PORT)&BTECH_MPU_INPUT_EMPTY))
    {
        inInterrupt=true;interruptsEnabled=false;
        MwSdlDeliverCombatMpuInterrupt();
        interruptsEnabled=true;inInterrupt=false;
    }
    if (!inInterrupt && interruptsEnabled && !(picMask&PIC_KEYBOARD_IRQ_MASK) && MwSdlCombatKeyboardPending())
    {
        inInterrupt=true;interruptsEnabled=false;
        MwSdlDeliverCombatKeyboardInterrupt();
        interruptsEnabled=true;inInterrupt=false;
    }
    double target=(double)(SDL_GetTicksNS()-startNanoseconds)*HOST_PIT_CLOCK_HZ/(double)HOST_NS_PER_SECOND;
    while (interruptsEnabled)
    {
        double mpuEvent=mwSdlCombatActive && mwSdlSoundSystem==MW_SOUND_MT32?MwMpuNextEventTime():DBL_MAX;
        double timerEvent=(picMask&PIC_TIMER_IRQ_MASK)?DBL_MAX:nextIrq;
        if (mpuEvent<=target && mpuEvent<timerEvent)
        {
            hardwareTime=mpuEvent;MwSdlAdvanceAudio(hardwareTime);MwMpuAdvance(hardwareTime);
            if (!(picMask&BTECH_MPU_IRQ_MASK) && !(MwMpuReadPort(MPU_STATUS_COMMAND_PORT)&BTECH_MPU_INPUT_EMPTY))
            {
                bool previousInInterrupt=inInterrupt;inInterrupt=true;interruptsEnabled=false;
                MwSdlDeliverCombatMpuInterrupt();interruptsEnabled=true;inInterrupt=previousInInterrupt;
            }
            continue;
        }
        if (timerEvent>target) break;
        /* BTECH's installed scheduler must service every elapsed PIT edge,
         * independently of presentation/poll frequency. Keep each original
         * callback and its timestamp; do not batch its game-state writes.
         * Campaign1610 waits for exact tick equality, so retain one BIOS tick
         * per caller observation while catching up intervening fast edges. */
        bool combatScheduler=irq0==BtechTimerSchedulerInterrupt;
        bool campaignScheduler=irq0==MwCampaignTimerInterrupt;
        MwMusicTimerHandler dispatchedHandler=irq0;
        uint32_t biosTicksBefore=biosTicks;
        hardwareTime=nextIrq;nextIrq+=channel0Divisor;
        MwSdlAdvanceAudio(hardwareTime);
        bool previousInInterrupt=inInterrupt;
        inInterrupt=true;bool savedInterrupts=interruptsEnabled;interruptsEnabled=false;
        if (!irq0) MwSdlFail("Unmapped IRQ0 handler");
        irq0();interruptsEnabled=savedInterrupts;inInterrupt=previousInInterrupt;
        /* MPU slot8 runs much faster than the speaker. Catch up those hardware
         * edges, but expose at most one BIOS tick to campaign1610's equality
         * wait. Stopping after every fast edge would slow MIDI with poll rate. */
        if (irq0!=dispatchedHandler || (!combatScheduler && (!campaignScheduler || biosTicks!=biosTicksBefore))) return;
    }
    if (nextIrq>target)
    {
        hardwareTime=target;MwSdlAdvanceAudio(hardwareTime);
        SDL_DelayNS(HOST_IDLE_WAIT_NS); /* Host idle only; no synthetic game ticks. */
    }
}
void MwSdlInstallCampaignInterrupts(void) {campaignInterruptsInstalled=true;}
void MwSdlRestoreCampaignInterrupts(void) {campaignInterruptsInstalled=false;}
uint32_t MwSdlReadBiosTicks(void) {MwSdlPump();return biosTicks;}
