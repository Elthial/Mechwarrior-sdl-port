/* OPTIONAL SUPPORT PILOT transport. No game-state writes. Enabled explicitly
 * in Release or Debug by --support-pilot DIRECTORY; F11 releases/disarms it. */
#include "SDLBackend.h"
#include "mechwarrior/support_pilot.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>

enum { PILOT_POLL_MS=25,PILOT_STATUS_MS=250,PILOT_STALE_FRAME_MS=1500,
    PILOT_MIN_LEASE_MS=2000,PILOT_MAX_LEASE_MS=60000 };
static char pilotCommandPath[4096],pilotStatusPath[4096];
static bool pilotEnabled,pilotArmed,wasCombat,pauseOnNextFrame;
static uint64_t pilotDeadline,nextPilotPoll,nextPilotStatus,lastPilotFrame,lastCommand;
static uint8_t heldDirection,heldCommand;
static const char *pilotMessage="disabled";
static SupportPilot pilot;
static PilotObservation observation;

static void ReleasePilotKeys(void)
{
    if(mwSdlCombatActive)
    {
        if(heldDirection)MwSdlQueueCombatScan(heldDirection|0x80);
        if(heldCommand)MwSdlQueueCombatScan(heldCommand|0x80);
    }
    heldDirection=heldCommand=0;
}
void MwSdlStopSupportPilot(void)
{ReleasePilotKeys();pilotArmed=false;pauseOnNextFrame=false;pilotMessage="stopped";}
bool MwSdlConfigureSupportPilot(const char *directory)
{
    pilotEnabled=false;pilotArmed=false;
    if(!directory)return true;
    if(snprintf(pilotCommandPath,sizeof(pilotCommandPath),"%s/pilot-command.txt",directory)>=(int)sizeof(pilotCommandPath)
        || snprintf(pilotStatusPath,sizeof(pilotStatusPath),"%s/pilot-status.json",directory)>=(int)sizeof(pilotStatusPath))return false;
    /* Fail early for a missing/unwritable selected directory. */
    FILE *file=fopen(pilotStatusPath,"w");if(!file)return false;
    fputs("{\"enabled\":true,\"armed\":false,\"message\":\"ready\"}\n",file);fclose(file);
    pilotEnabled=true;pilotMessage="ready";nextPilotPoll=nextPilotStatus=lastCommand=0;wasCombat=false;
    return true;
}
static void PublishPilotStatus(uint64_t now)
{
    FILE *file=fopen(pilotStatusPath,"w");if(!file)return;
    fprintf(file,"{\"enabled\":true,\"armed\":%s,\"commandId\":%llu,\"message\":\"%s\",\"timeMs\":%llu,\"leaseRemainingMs\":%llu,\"frameAgeMs\":%llu,\"tick\":%u,\"goal\":%u,\"complete\":%s,\"target\":%u,\"checkpoint\":%u,\"range\":%.1f,\"headingError\":%.1f,\"movement\":\"%s\",\"weapons\":\"%s\",\"keys\":[%u,%u],\"player\":{\"position\":[%ld,%ld,%ld],\"heading\":%u,\"torso\":%d,\"speed\":%d,\"heat\":%u,\"alive\":%s},\"contacts\":[",
        pilotArmed?"true":"false",(unsigned long long)lastCommand,pilotMessage,(unsigned long long)now,
        (unsigned long long)(pilotArmed && pilotDeadline>now?pilotDeadline-now:0),(unsigned long long)(now-lastPilotFrame),
        observation.tick,pilot.orders.goal,pilot.complete?"true":"false",pilot.targetSlot,pilot.checkpoint,pilot.range,pilot.headingError,
        pilot.movementReason?pilot.movementReason:"idle",pilot.weaponReason?pilot.weaponReason:"idle",heldDirection,heldCommand,
        (long)observation.position.x,(long)observation.position.y,(long)observation.position.z,observation.heading,
        (int16_t)observation.torsoHeading,observation.speed,observation.heat,observation.alive?"true":"false");
    bool first=true;
    for(unsigned slot=1;slot<PILOT_TARGETS;++slot)
    {
        const PilotTarget *target=&observation.targets[slot];if(!target->known)continue;
        fprintf(file,"%s{\"slot\":%u,\"visible\":%s,\"hostile\":%s,\"alive\":%s,\"underReticle\":%s,\"position\":[%ld,%ld,%ld],\"screen\":[%d,%d],\"armor\":%u,\"internal\":%u}",
            first?"":",",slot,target->visible?"true":"false",target->hostile?"true":"false",target->alive?"true":"false",
            target->underReticle?"true":"false",(long)target->position.x,(long)target->position.y,(long)target->position.z,
            target->screenX,target->screenY,target->armor,target->internal);first=false;
    }
    fprintf(file,"],\"audioQueueMs\":%u,\"audioClockSkippedFrames\":%llu}\n",
        MwSdlQueuedAudioMilliseconds(),(unsigned long long)MwSdlAudioClockSkippedFrames());
    fclose(file);
}
void MwSdlPollSupportPilot(void)
{
    if(!pilotEnabled)return;
    uint64_t now=SDL_GetTicks();if(now<nextPilotPoll)return;nextPilotPoll=now+PILOT_POLL_MS;
    if(wasCombat && !mwSdlCombatActive)
    {MwSdlStopSupportPilot();memset(&observation,0,sizeof(observation));pilotMessage="mission ended";}
    wasCombat=mwSdlCombatActive;
    if(pilotArmed && now>=pilotDeadline)
    {MwSdlStopSupportPilot();pauseOnNextFrame=true;pilotMessage="supervisor lease expired";}
    /* A native pause/dialog stops the main-loop hook. Release keys, never send
     * a make into that dialog to dismiss it. Orders remain leased for resume. */
    if(now-lastPilotFrame>PILOT_STALE_FRAME_MS)ReleasePilotKeys();
    FILE *file=fopen(pilotCommandPath,"r");
    if(file)
    {
        char line[256],action[24]={0};unsigned long long id=0;
        unsigned slot=0,minimum=0,maximum=0,heat=0,tactic=0,lease=0;int consumed=0;
        bool valid=fgets(line,sizeof(line),file) && sscanf(line,"%llu %23s %u %u %u %u %u %u %n",
            &id,action,&slot,&minimum,&maximum,&heat,&tactic,&lease,&consumed)==8;
        if(valid)for(char *ch=line+consumed;*ch;++ch)if(!isspace((unsigned char)*ch))valid=false;
        PilotOrders orders={0};orders.targetSlot=slot;orders.minimumRange=(uint16_t)minimum;
        orders.maximumRange=(uint16_t)maximum;orders.heatCeiling=(uint16_t)heat;
        orders.tactic=(PilotTactic)tactic;orders.arrivalRadius=256;
        while(fgets(line,sizeof(line),file))
        {
            long x,y,z;consumed=0;
            if(orders.checkpointCount>=PILOT_CHECKPOINTS || sscanf(line,"point %ld %ld %ld %n",&x,&y,&z,&consumed)!=3)
            {valid=false;break;}
            for(char *ch=line+consumed;*ch;++ch)if(!isspace((unsigned char)*ch))valid=false;
            orders.checkpoints[orders.checkpointCount++]=(PilotPoint){(int32_t)x,(int32_t)y,(int32_t)z};
        }
        if(ferror(file))valid=false;fclose(file);remove(pilotCommandPath);
        valid=valid && id>lastCommand && slot<PILOT_TARGETS && minimum>=256 && maximum>minimum && maximum<=30000
            && heat>=1 && heat<1600 && tactic<=PILOT_ORBIT_RIGHT && lease>=PILOT_MIN_LEASE_MS && lease<=PILOT_MAX_LEASE_MS;
        if(id>lastCommand)lastCommand=id;
        if(!valid)pilotMessage="rejected command";
        else if(!strcmp(action,"stop")){MwSdlStopSupportPilot();}
        else if(!strcmp(action,"renew"))
        {if(pilotArmed){pilotDeadline=now+lease;pilotMessage="renewed";}else pilotMessage="not armed";}
        else if((!strcmp(action,"engage") && !orders.checkpointCount) || (!strcmp(action,"navigate") && orders.checkpointCount))
        {
            if (!mwSdlCombatActive || !observation.cockpit || now-lastPilotFrame>PILOT_STALE_FRAME_MS)
                pilotMessage="rejected: no active cockpit";
            else
            {
                ReleasePilotKeys();pauseOnNextFrame=false;
                orders.goal=!strcmp(action,"engage")?PILOT_ENGAGE:PILOT_NAVIGATE;
                SupportPilotReset(&pilot,&orders);pilotDeadline=now+lease;pilotArmed=true;pilotMessage="accepted";
            }
        }
        else pilotMessage="rejected goal";
        nextPilotStatus=0;
    }
    if(now>=nextPilotStatus){nextPilotStatus=now+PILOT_STATUS_MS;PublishPilotStatus(now);}
}
void MwSdlUpdateSupportPilot(void)
{
    if(!pilotEnabled)return;
    lastPilotFrame=SDL_GetTicks();SupportPilotObserve(&observation,mwSdlCombatActive);
    if(pauseOnNextFrame && observation.cockpit)
    {
        pauseOnNextFrame=false;MwSdlQueueCombatScan(0x19); /* Ordinary P; never sent inside a paused dialog. */
        for(unsigned scans=0;scans<16 && MwSdlCombatKeyboardPending();++scans)MwSdlPumpTimer();
    }
    if(!pilotArmed)return;
    PilotControls controls=SupportPilotUpdate(&pilot,&observation);
    /* Break before make, with the action make LAST: native IRQ1 break clears
     * last-key state. Drain through the normal installed IRQ, not game calls. */
    ReleasePilotKeys();
    if(controls.directionScan){MwSdlQueueCombatScan(controls.directionScan);heldDirection=controls.directionScan;}
    if(controls.commandScan){MwSdlQueueCombatScan(controls.commandScan);heldCommand=controls.commandScan;}
    for(unsigned scans=0;scans<16 && MwSdlCombatKeyboardPending();++scans)MwSdlPumpTimer();
    if(MwSdlCombatKeyboardPending()){MwSdlStopSupportPilot();pilotMessage="keyboard delivery blocked";}
    if(pilot.complete){pilotArmed=false;pilotMessage="goal complete";}
}
