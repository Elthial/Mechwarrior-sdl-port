#include "SDLBackend.h"
#include "mechwarrior/campaign_runtime.h"
#include "mechwarrior/campaign_ui.h"
#include "mechwarrior/original_data.h"
#include "mechwarrior/dos_files.h"
#include "mechwarrior/launcher.h"
#include "mechwarrior/campaign_startup.h"
#include "mechwarrior/combat_startup.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint8_t *resourceAllocations[8];
static uint16_t allocationCount;
static char installationDirectory[4096];
static bool initialized;

static void ReleaseChildResources(void)
{
    /* DOS frees a child's allocations and files after EXEC returns, including
     * startup failures. Do not move this cleanup into native allocation errors. */
    MwHostCloseProcessFiles();
    for (uint16_t i=0;i<allocationCount;++i)
    {MwBindNativeSegment((uint16_t)(0x4000+i*0x1000),NULL);free(resourceAllocations[i]);resourceAllocations[i]=NULL;}
    allocationCount=0;
}
static bool ExecuteChild(const char *filename,const MwCampaignLaunch *launch,uint8_t audioCode,uint8_t *exitCode)
{
    char path[4096];
    if (snprintf(path,sizeof(path),"%s/%s",installationDirectory,filename)>=(int)sizeof(path)) return true;
    FILE *file=fopen(path,"rb");
    if (!file) return true;
    if (fclose(file)!=0) return true;
    ReleaseChildResources();
    if (!strcmp(filename,"BTECH.EXE")) return MwSdlExecuteCombatChild(path,launch,audioCode,exitCode);
    if (strcmp(filename,"MW_MAIN.EXE")) MwSdlFail("Unknown launcher child executable");
    /* Fresh DOS child image on EACH execution, then0002 decides new/resume.
     * Keep input/ticks, physical video and resident exchange alive across EXEC. */
    if (!MwHostLoadCampaignInitialData(path) || !MwHostLoadCampaignEventText(path)
        || !MwHostLoadCampaignMusicData(path)) return true;
    *exitCode=(uint8_t)MwRunCampaignProgram(launch,audioCode);
    if (*exitCode==CAMPAIGN_EXIT_COMBAT)
        MwSdlRetainCampaignPictureTail(MwResolveNativeSegment(secondaryPictureBankSelector));
    ReleaseChildResources();
    return false;
}
static bool ResizeResident(uint16_t paragraphs)
{
    /* Host launcher owns named static data, not a DOS MCB. This genuine INT21
     * shrink has no competing allocation/failure on the supported host path. */
    return paragraphs!=LAUNCHER_RESIDENT_PARAGRAPHS;
}
static void *AllocateLauncherProbe(uint16_t paragraphs) {return malloc((size_t)paragraphs*0x10);}
static uint16_t DetectConfiguredAdapter(void) {return LAUNCHER_ADAPTER_EGA;}
static void ClearLauncherText(void) {MwSdlSetVideoMode(3);}
static void WriteLauncherChar(uint8_t character) {fputc(character,stderr);fflush(stderr);}

_Noreturn void MwSdlFail(const char *operation)
{fprintf(stderr,"MechWarrior: %s. SDL: %s\n",operation,SDL_GetError());abort();}
static bool AllocateParagraphs(uint16_t paragraphs,uint16_t *selectorOrError)
{
    (void)paragraphs; /* Full resource windows retain native WORD-cursor wrap. */
    if (allocationCount>=8) { *selectorOrError=8;return true; }
    uint8_t *buffer=malloc(NATIVE_SEGMENT_BYTE_COUNT);
    if (!buffer) { *selectorOrError=8;return true; }
    *selectorOrError=(uint16_t)(0x4000+allocationCount*0x1000);
    resourceAllocations[allocationCount++]=buffer;
    MwBindNativeSegment(*selectorOrError,buffer);
    return false;
}
static void ValidateFileDestination(const uint8_t *buffer,uint16_t bytesRead)
{
    uintptr_t destination=(uintptr_t)buffer;
    for (uint16_t i=0;i<allocationCount;++i)
    {
        uintptr_t allocation=(uintptr_t)resourceAllocations[i];
        if (destination>=allocation && destination-allocation<NATIVE_SEGMENT_BYTE_COUNT)
        {
            if (bytesRead>NATIVE_SEGMENT_BYTE_COUNT-(destination-allocation))
                MwSdlFail("Unsupported resource read beyond its native offset window");
            return;
        }
    }
    /* Other destinations are typed IO buffers supplied by original wrappers. */
}
static bool LoadOverlay(const char *filename,uint16_t selector)
{
    (void)selector;
    if (strcmp(filename,"MW_EGA.EXE")) MwSdlFail("Only the native EGA overlay is converted");
    char path[4096];
    if (snprintf(path,sizeof(path),"%s/%s",installationDirectory,filename)>=(int)sizeof(path)) return true;
    return !MwHostLoadEgaInitialData(path);
}
static uint16_t ResetMouse(void) {return UINT16_MAX;} /* SDL supplies the INT33-equivalent boundary. */
static void DosString(const char *text)
{while (*text && *text!='$') fputc(*text++,stderr);fflush(stderr);}
static void UnsupportedCampaignCall(uint16_t codeOffset)
{
    fprintf(stderr,"Unconverted campaign callback081B:%04X (combat transition remains outside this live slice).\n",codeOffset);
    MwSdlFail("Unconverted campaign callback");
}
static bool LoadResidentLauncher(const char *path)
{
    uint8_t header[0x1C],data[0x09EE];FILE *file=fopen(path,"rb");if (!file) return false;
    bool loaded=false;
    if (fread(header,1,sizeof(header),file)!=sizeof(header) || header[0]!='M' || header[1]!='Z') goto finished;
    uint16_t paragraphs=(uint16_t)(header[8]|((uint16_t)header[9]<<8));
    /* MW.EXE load0800, resident data0806, real transfer offset0004. */
    if (fseek(file,(long)paragraphs*0x10+0x60,SEEK_SET)!=0) goto finished;
    if (fread(data,1,sizeof(data),file)!=sizeof(data) || ferror(file)) goto finished;
    loaded=MwImportLauncherInitialData(data,sizeof(data));
finished:
    if (fclose(file)!=0) loaded=false;
    return loaded;
}
bool MwSdlInitialize(const char *gameDirectory,const char *captureDirectory)
{
    if (initialized || strlen(gameDirectory)>=sizeof(installationDirectory)) return false;
    strcpy(installationDirectory,gameDirectory);
    char path[4096];
    if (snprintf(path,sizeof(path),"%s/MW_MAIN.EXE",gameDirectory)>=(int)sizeof(path)
        || !MwHostLoadCampaignInitialData(path) || !MwHostLoadCampaignEventText(path)
        || !MwHostLoadCampaignMusicData(path) || !MwHostSetOriginalGameDirectory(gameDirectory)) return false;
    if (snprintf(path,sizeof(path),"%s/MW.EXE",gameDirectory)>=(int)sizeof(path) || !LoadResidentLauncher(path)) return false;
    if (!SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO|SDL_INIT_EVENTS)) return false;
    const char *configuredSaveDirectory=getenv("MECHWARRIOR_SAVE_DIRECTORY");
    bool saveDirectorySet;
    if (configuredSaveDirectory) saveDirectorySet=MwHostSetCampaignSaveDirectory(configuredSaveDirectory);
    else
    {
        char *saveDirectory=SDL_GetPrefPath("BattleTechReverse","MechWarrior");
        if (!saveDirectory) {SDL_Quit();return false;}
        saveDirectorySet=MwHostSetCampaignSaveDirectory(saveDirectory);SDL_free(saveDirectory);
    }
    if (!saveDirectorySet) {SDL_Quit();return false;}
    initialized=true;
    if (!MwSdlInitializeVideo(captureDirectory) || !MwSdlInitializeAudio()) {MwSdlShutdown();return false;}
    MwSdlInitializeTimer();MwSdlInitializeInput();
    MwSdlInitializeCombatHardware();
    mwMusicHardware.readInterruptVector=btechHostGetInterruptVector;
    mwMusicHardware.writeInterruptVector=btechHostSetInterruptVector;
    if (!MwSdlInitializeCombatProcess(gameDirectory)) {MwSdlShutdown();return false;}
    mwHostValidateFileDestination=ValidateFileDestination;
    mwCampaignPlatform=(MwCampaignPlatformCalls){AllocateParagraphs,LoadOverlay,
        MwSdlInstallCampaignInterrupts,MwSdlRestoreCampaignInterrupts,MwSdlGetVideoMode,MwSdlSetVideoMode,
        MwSdlReadBiosTicks,ResetMouse,DosString};
    campaignStartMusic=MwStartMidiTrackFar;campaignStopMusic=MwStopMidiTrackFar;
    campaignNearCallback=UnsupportedCampaignCall;
    mwLauncherSystem=(MwLauncherSystemCalls){ResizeResident,AllocateLauncherProbe,free,DetectConfiguredAdapter,
        ClearLauncherText,ExecuteChild,DosString,WriteLauncherChar};
    return true;
}
int MwSdlRunCampaign(void)
{
    if (!initialized) MwSdlFail("Host not initialized");
    /* Keep MW.EXE's original graphics/audio command-tail selection. */
    const uint8_t commandTail[]={'1',mwSdlSoundSystem==MW_SOUND_MT32?'R':'S'};
    MwLauncherCommandLine commandLine={commandTail,sizeof(commandTail)};
    return MwLauncherMain(&commandLine);
}
void MwSdlPump(void)
{MwSdlPollEvents();MwSdlPumpTimer();MwSdlPresentVideo(false);}
void MwSdlShutdown(void)
{
    if (!initialized) return;
    MwSdlShutdownDebugControl();
    mwHostValidateFileDestination=NULL;
    ReleaseChildResources();
    MwSdlShutdownAudio();MwSdlShutdownVideo();
    SDL_Quit();initialized=false;
}
