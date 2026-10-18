#define SDL_MAIN_HANDLED
#include "SDLBackend.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mechwarrior/combat_pose.h"
#include "mechwarrior/combat_cockpit.h"
#include "mechwarrior/combat_startup.h"
#include "mechwarrior/combat_import.h"
static FILE *diagnosticAudio;
static uint64_t nextDiagnosticFrame;
static void RecordDiagnosticAudio(const float *samples,uint32_t count)
{
    if (fwrite(samples,sizeof(float),count,diagnosticAudio)!=count)
        MwSdlFail("Write private diagnostic audio");
}
static void TraceDiagnosticFrame(void)
{
    uint64_t now=SDL_GetTicks();
    if (!mwSdlCombatActive || now<nextDiagnosticFrame || !playerUnitBinding
        || !playerUnitBinding->positionedLeftLeg) return;
    nextDiagnosticFrame=now+1000;
    const BtechPooledWorldObject *body=playerUnitBinding->positionedLeftLeg;
    fprintf(stderr,"Combat time=%llu tick=%u position=%ld,%ld step=%d pose=%u/%u gait=%u sound=%u timerLagMs=%.2f audioQueueMs=%u cpuCycles=%llu lateAudioFrames=%llu underrunFrames=%llu\n",
        (unsigned long long)now,combatTickCounter,
        (long)(int32_t)body->object.prefix.position.positionA,
        (long)(int32_t)body->object.prefix.position.positionB,
        body->forwardStepScale,playerPoseSequence,playerPoseFrame,
        playerPoseDistanceAccumulator,combatSoundEnabled,
        MwSdlTimerLagMilliseconds(),MwSdlQueuedAudioMilliseconds(),
        (unsigned long long)MwSdlLastCombatCpuCycles(),(unsigned long long)MwSdlLateAudioFrames(),
        (unsigned long long)MwSdlAudioUnderrunFrames());
    for (uint16_t slot=1;slot<COMBAT_MECH_SLOTS;++slot)
    {
        const BtechPooledWorldObject *other=unitBindings[slot].positionedLeftLeg;
        if (!unitBindings[slot].mech || !other) continue;
        fprintf(stderr,"AI slot=%u position=%ld,%ld step=%d\n",slot,
            (long)(int32_t)other->object.prefix.position.positionA,
            (long)(int32_t)other->object.prefix.position.positionB,
            other->forwardStepScale);
    }
    fflush(stderr);
}
#if defined(_MSC_VER)
#include <stdlib.h>
#include <signal.h>
#include <windows.h>
#include <dbghelp.h>

/* Host diagnostics only: retain the native failure, and identify it in the
 * launcher's stderr log instead of losing the call site when the window closes. */
static void ReportFatalAbort(int signalNumber)
{
    (void)signalNumber;
    HANDLE process=GetCurrentProcess();
    SymSetOptions(SYMOPT_LOAD_LINES|SYMOPT_UNDNAME|SYMOPT_DEFERRED_LOADS);
    BOOL symbols=SymInitialize(process,NULL,TRUE);
    void *frames[40];USHORT count=CaptureStackBackTrace(0,40,frames,NULL);
    fprintf(stderr,"MechWarrior fatal abort (combat active=%u).\n",mwSdlCombatActive?1:0);
    for (USHORT index=0;index<count;++index)
    {
        IMAGEHLP_LINE64 line={0};line.SizeOfStruct=sizeof(line);DWORD displacement;
        if (symbols && SymGetLineFromAddr64(process,(DWORD64)frames[index],&displacement,&line))
            fprintf(stderr,"%s:%lu\n",line.FileName,(unsigned long)line.LineNumber);
        else
            fprintf(stderr,"frame %u: %p\n",index,frames[index]);
    }
    fflush(stderr);_Exit(3);
}
#endif
int main(int argc,char **argv)
{
#if defined(_MSC_VER)
    signal(SIGABRT,ReportFatalAbort);
#endif
    const char *pilotDirectory=NULL;
    if(argc>=3 && !strcmp(argv[argc-2],"--support-pilot"))
    {pilotDirectory=argv[argc-1];argc-=2;}
    else if(argc>=2 && !strcmp(argv[argc-1],"--no-support-pilot"))--argc;
    if (argc<2 || argc>3)
    {fprintf(stderr,"Usage: mechwarrior <private game directory> [existing capture directory] [--support-pilot DIRECTORY | --no-support-pilot]\nEGA / PC speaker; arrows and Enter; Ctrl+Escape quits; F12 captures; F11 stops Support pilot.\n");return 2;}
    SDL_SetMainReady();
    if(!MwSdlConfigureSupportPilot(pilotDirectory))
    {fprintf(stderr,"Cannot open Support pilot directory.\n");return 2;}
    const char *timingPath=getenv("MECHWARRIOR_TIMING_CONFIG");
    bool optionalTiming=timingPath==NULL;
    char defaultTimingPath[4096];
    if (!timingPath)
    {
        const char *base=SDL_GetBasePath();
        if (!base || snprintf(defaultTimingPath,sizeof(defaultTimingPath),"%sMechWarrior.ini",base)>=(int)sizeof(defaultTimingPath)) return 2;
        timingPath=defaultTimingPath;
    }
    if (!MwSdlLoadTimingConfiguration(timingPath,optionalTiming))
    {fprintf(stderr,"Invalid MechWarrior configuration: %s (CPU rate, sound system or ROM path).\n",timingPath);return 2;}
    fprintf(stderr,"Reference CPU budget: %u cycles/ms; PIT/audio retain hardware rate.\n",mwSdlCpuCyclesPerMillisecond);
    if (!MwSdlInitialize(argv[1],argc==3?argv[2]:"."))
    {fprintf(stderr,"Cannot initialize the private installation/SDL: %s\n",SDL_GetError());return 1;}
    if (getenv("MECHWARRIOR_DIAGNOSTICS") && argc==3)
    {
        char audioPath[4096];
        if (snprintf(audioPath,sizeof(audioPath),"%s/speaker.f32",argv[2])>=(int)sizeof(audioPath)) return 2;
        diagnosticAudio=fopen(audioPath,"wbx");
        if (!diagnosticAudio) return 2;
        mwSdlAudioObserver=RecordDiagnosticAudio;mwSdlFrameObserver=TraceDiagnosticFrame;
    }
    int result=MwSdlRunCampaign();
    mwSdlAudioObserver=NULL;mwSdlFrameObserver=NULL;
    if (diagnosticAudio) {fclose(diagnosticAudio);diagnosticAudio=NULL;}
    MwSdlShutdown();return result;
}
