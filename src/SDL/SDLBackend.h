#ifndef MECHWARRIOR_SDL_BACKEND_H
#define MECHWARRIOR_SDL_BACKEND_H
#include "mechwarrior/sdl_host.h"
#include "mechwarrior/campaign_music.h"
#include "mechwarrior/launcher.h"
#include <SDL3/SDL.h>
enum { HOST_PIT_CLOCK_HZ=1193182,HOST_AUDIO_SAMPLE_RATE=48000 };
typedef enum MwHostSoundSystem {MW_SOUND_SPEAKER,MW_SOUND_MT32,MW_SOUND_ADLIB} MwHostSoundSystem;
extern MwHostSoundSystem mwSdlSoundSystem;
extern char mwSdlMt32ControlRom[4096],mwSdlMt32PcmRom[4096];
void MwMpuReset(void);
void MwMpuWritePort(uint16_t port,uint8_t value);
uint8_t MwMpuReadPort(uint16_t port);
/* Captured DOS machine profile, shared by physical aliases and first-fit MCBs. */
enum { HOST_DOS_COMBAT_CODE_SELECTOR=0x0956 };
_Noreturn void MwSdlFail(const char *operation);
void MwSdlInitializeInput(void);
void MwSdlPollEvents(void);
void MwSdlInitializeDebugControl(void);
void MwSdlPollDebugControl(void);
void MwSdlShutdownDebugControl(void);
/* Optional host automation, never enabled by build configuration alone. */
bool MwSdlConfigureSupportPilot(const char *directory);
void MwSdlPollSupportPilot(void);
void MwSdlUpdateSupportPilot(void);
void MwSdlStopSupportPilot(void);
void MwSdlQueueCombatScan(uint8_t scan);
void MwSdlQueueBiosKey(uint16_t key,uint8_t shift);
void MwSdlInitializeTimer(void);
void MwSdlPumpTimer(void);
/* Measured reference-workload CPU budget, not an instruction interpreter. */
extern uint32_t mwSdlCpuCyclesPerMillisecond;
void MwSdlResetCombatCpu(void);
void MwSdlAccountCombatVideoAccess(bool write);
void MwSdlPaceCombatCpu(void);
void MwSdlDelayCpuLoops(uint16_t loops);
uint64_t MwSdlLastCombatCpuCycles(void);
bool MwSdlLoadTimingConfiguration(const char *path,bool optional);
void MwSdlSetInterrupts(bool enabled);
void MwSdlInstallCampaignInterrupts(void);
void MwSdlRestoreCampaignInterrupts(void);
uint32_t MwSdlReadBiosTicks(void);
double MwSdlHardwareTime(void);
double MwSdlTimerLagMilliseconds(void);
uint32_t MwSdlQueuedAudioMilliseconds(void);
uint64_t MwSdlLateAudioFrames(void);
uint64_t MwSdlAudioUnderrunFrames(void);
uint64_t MwSdlAudioClockSkippedFrames(void);
bool MwSdlInitializeVideo(const char *captureDirectory);
void MwSdlUpdateWindowTitle(void);
void MwSdlPresentVideo(bool force);
void MwSdlShutdownVideo(void);
uint8_t MwSdlGetVideoMode(void);
void MwSdlSetVideoMode(uint8_t mode);
void MwSdlSetMousePosition(uint16_t horizontalPosition,uint16_t verticalPosition);
void MwSdlReadMousePosition(uint16_t *horizontalPosition,uint16_t *verticalPosition);
void MwSdlConvertMouseMotion(float windowXMotion,float windowYMotion,
    float *logicalXMotion,float *logicalYMotion);
bool MwSdlInitializeAudio(void);
void MwSdlAdvanceAudio(double pitTime);
void MwSdlWriteSpeakerPort(uint16_t port,uint8_t value);
uint8_t MwSdlReadSpeakerControl(void);
void MwSdlShutdownAudio(void);
/* Shared physical EGA adapter. Game rasterizers remain in Original. */
extern bool mwSdlCombatActive;
void MwSdlBindCombatVideo(void);
void MwSdlResetCombatVideoHardware(void);
bool MwSdlWriteCombatVideoPort(uint16_t port,uint8_t value);
bool MwSdlReadCombatVideoPort(uint16_t port,uint8_t *value);
uint16_t MwSdlCombatDisplayByteOffset(void);
void MwSdlWriteDevicePort(uint16_t port,uint8_t value);
uint8_t MwSdlReadDevicePort(uint16_t port);
uint8_t MwSdlExchangeInterrupts(uint8_t enabled);
MwMusicTimerHandler MwSdlReadTimerHandler(void);
void MwSdlWriteTimerHandler(MwMusicTimerHandler handler);
uint32_t MwSdlBiosTicksValue(void);
void MwSdlInitializeCombatHardware(void);
void MwSdlResetCombatHardware(void);
bool MwSdlInitializeCombatProcess(const char *directory);
bool MwSdlExecuteCombatChild(const char *path,const MwCampaignLaunch *launch,uint8_t audioCode,uint8_t *exitCode);
void MwSdlRetainCampaignPictureTail(const uint8_t *pictureBank);
bool MwSdlCombatKeyboardPending(void);
uint16_t MwSdlCombatDosAllocationCount(void);
void MwSdlDeliverCombatKeyboardInterrupt(void);
uint8_t MwSdlReadKeyboardScanByte(void);
#endif
