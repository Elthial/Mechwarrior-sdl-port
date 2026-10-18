/* User-requested optional subsumption pilot. NOT original game logic.
 * Higher priority layers suppress lower outputs, independently for locomotion
 * and weapons. The only effect is an ordinary keyboard decision per game tick. */
#include "mechwarrior/support_pilot.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>

enum { PILOT_HEADING_TURN=65536,PILOT_HEADING_HALF=32768,
    PILOT_TURN_DEADBAND=192,PILOT_TORSO_STEP=1024,PILOT_TORSO_LIMIT=7168,
    PILOT_LOST_TARGET_TICKS=24,PILOT_STUCK_TICKS=32,PILOT_RECOVERY_INTERVAL=16 };
static double Distance(PilotPoint first,PilotPoint second)
{return hypot((double)second.x-first.x,(double)second.y-first.y);}
static double AngleDifference(double desired,double actual)
{
    double difference=fmod(desired-actual+PILOT_HEADING_HALF,PILOT_HEADING_TURN);
    if(difference<0)difference+=PILOT_HEADING_TURN;
    return difference-PILOT_HEADING_HALF;
}
static double Bearing(PilotPoint from,PilotPoint to)
{return atan2(-((double)to.x-from.x),(double)to.y-from.y)*(65536.0/6.2831853071795864769);}
void SupportPilotReset(SupportPilot *pilot,const PilotOrders *orders)
{
    memset(pilot,0,sizeof(*pilot));pilot->orders=*orders;
    pilot->movementReason="idle";pilot->weaponReason="idle";
}
/* Host-only copilot: recovery overrides navigation, then range/orbit steering
 * and heat-limited firing produce ordinary input without changing combat state. */
PilotControls SupportPilotUpdate(SupportPilot *pilot,const PilotObservation *view)
{
    PilotControls controls={0};
    if(pilot->haveObservation && view->tick==pilot->previousTick)return pilot->controls;
    double travelled=pilot->haveObservation?Distance(pilot->previousPosition,view->position):0;
    pilot->haveObservation=true;pilot->previousTick=view->tick;pilot->previousPosition=view->position;
    pilot->movementReason="idle";pilot->weaponReason="not aiming";
    /* Safety overrides all lower behaviours. No automatic unpause or screen changes. */
    if(!view->cockpit || !view->alive || view->missionComplete || pilot->orders.goal==PILOT_IDLE || pilot->complete)
    {pilot->movementReason="inactive";goto done;}
    if(view->fallen)
    {
        pilot->movementReason="recovering";
        if(view->tick%PILOT_RECOVERY_INTERVAL==0)controls.commandScan=PILOT_SCAN_RECOVER;
        goto done;
    }
    PilotPoint destination=view->position;
    const PilotTarget *target=NULL;
    if(pilot->orders.goal==PILOT_ENGAGE)
    {
        unsigned slot=pilot->orders.targetSlot;
        if(!slot && view->lockedTarget<PILOT_TARGETS && view->targets[view->lockedTarget].hostile)
            slot=view->lockedTarget;
        if(!slot)for(unsigned candidate=1;candidate<PILOT_TARGETS;++candidate)
            if(view->targets[candidate].visible && view->targets[candidate].hostile && view->targets[candidate].alive)
                {slot=candidate;break;}
        if(!slot)slot=pilot->targetSlot;
        if(slot && slot<PILOT_TARGETS && view->targets[slot].known && view->targets[slot].hostile)
        {
            target=&view->targets[slot];pilot->targetSlot=slot;
            if(!target->alive){pilot->complete=true;pilot->movementReason="target disabled";controls.commandScan=PILOT_SCAN_STOP;goto done;}
            destination=target->position;pilot->lastTargetPosition=destination;pilot->unseenTicks=0;
        }
        else if(pilot->targetSlot && ++pilot->unseenTicks<=PILOT_LOST_TARGET_TICKS)
            destination=pilot->lastTargetPosition; /* Last observation, never hidden current coordinates. */
        else {pilot->movementReason="awaiting visible target";controls.commandScan=PILOT_SCAN_STOP;goto done;}
    }
    else
    {
        while(pilot->checkpoint<pilot->orders.checkpointCount &&
            Distance(view->position,pilot->orders.checkpoints[pilot->checkpoint])<=pilot->orders.arrivalRadius)
            ++pilot->checkpoint;
        if(pilot->checkpoint==pilot->orders.checkpointCount)
        {pilot->complete=true;pilot->movementReason="navigation complete";controls.commandScan=PILOT_SCAN_STOP;goto done;}
        destination=pilot->orders.checkpoints[pilot->checkpoint];
    }
    pilot->range=Distance(view->position,destination);
    double heading=Bearing(view->position,destination);
    int desiredSpeed=view->maximumSpeed;
    pilot->movementReason="approach";
    if(pilot->orders.goal==PILOT_ENGAGE)
    {
        /* Stop before entering the band: include both our braking distance and
         * a conservative closing margin. These are pilot tactics, not physics. */
        double braking=fabs((double)view->speed)*fabs((double)view->speed)/8.0;
        if(pilot->range<pilot->orders.minimumRange)
        {desiredSpeed=-view->maximumSpeed/2;pilot->movementReason="open range";}
        else if(pilot->range<pilot->orders.maximumRange+braking+512)
        {
            desiredSpeed=0;pilot->movementReason="hold range";
            if(pilot->orders.tactic!=PILOT_KEEP_RANGE && pilot->range>pilot->orders.minimumRange+512)
            {
                heading+=pilot->orders.tactic==PILOT_ORBIT_LEFT?5461:-5461;
                desiredSpeed=view->maximumSpeed/3;pilot->movementReason="orbit";
            }
        }
    }
    else if(pilot->range<pilot->orders.arrivalRadius+512)desiredSpeed=view->maximumSpeed/4;
    pilot->headingError=AngleDifference(heading,view->heading);
    /* Locomotion arbitration: range braking > body heading > throttle.
     * Native direction groups are mutually overwriting, so emit only one. */
    if(view->requestedSpeed>desiredSpeed+3)controls.directionScan=PILOT_SCAN_DOWN;
    else if(fabs(pilot->headingError-view->turnRate*2)>PILOT_TURN_DEADBAND)
        controls.directionScan=pilot->headingError-view->turnRate*2>0?PILOT_SCAN_LEFT:PILOT_SCAN_RIGHT;
    else if(view->requestedSpeed<desiredSpeed-3)controls.directionScan=PILOT_SCAN_UP;
    /* Ground contact can clear actual speed while the pilot keeps requesting
     * throttle. Detect failed movement intent rather than requiring motion. */
    if (desiredSpeed != 0 && travelled < 2
        && fabs(pilot->headingError) < PILOT_TURN_DEADBAND * 2)
        ++pilot->stalledTicks;
    else
        pilot->stalledTicks = 0;
    if (pilot->stalledTicks >= PILOT_STUCK_TICKS && !pilot->recoveryTicks)
    {
        pilot->recoveryTicks = PILOT_STUCK_TICKS;
        pilot->stalledTicks = 0;
    }
    if (pilot->recoveryTicks)
    {
        pilot->movementReason="stalled: reverse and turn";
        controls.directionScan = pilot->recoveryTicks > PILOT_STUCK_TICKS / 2
            ? PILOT_SCAN_DOWN : PILOT_SCAN_LEFT;
        --pilot->recoveryTicks;
    }
    /* Weapons arbitration: cancel uncontrolled salvo/AWS > aim > choose > fire.
     * Each individual shot is gated on real renderer hit identity and projected heat. */
    if(view->salvo){controls.commandScan=PILOT_SCAN_SALVO;pilot->weaponReason="cancel salvo";goto done;}
    if(view->aws){controls.commandScan=PILOT_SCAN_AWS;pilot->weaponReason="manual weapon control";goto done;}
    if(!target || !target->visible){pilot->weaponReason="target not rendered";goto done;}
    if(view->lockedTarget!=pilot->targetSlot)
    {controls.commandScan=PILOT_SCAN_TARGET;pilot->weaponReason="acquire target";goto done;}
    double aimError=AngleDifference(Bearing(view->position,destination),(double)view->heading+(int16_t)view->torsoHeading);
    if(fabs(aimError)>PILOT_TORSO_STEP/2 && abs((int16_t)view->torsoHeading)<PILOT_TORSO_LIMIT)
    {controls.commandScan=aimError>0?PILOT_SCAN_TORSO_LEFT:PILOT_SCAN_TORSO_RIGHT;pilot->weaponReason="track torso";goto done;}
    if(abs(target->screenY-view->reticleY)>2)
    {controls.commandScan=target->screenY>view->reticleY?PILOT_SCAN_AIM_DOWN:PILOT_SCAN_AIM_UP;pilot->weaponReason="track elevation";goto done;}
    if(!target->underReticle){pilot->weaponReason="no clear shot";goto done;}
    if(view->heatInhibited || view->heat>=pilot->orders.heatCeiling)
    {pilot->weaponReason="cooling";goto done;}
    for(unsigned mount=0;mount<PILOT_WEAPONS;++mount)
    {
        const PilotWeapon *weapon=&view->weapons[mount];
        if(!weapon->ready || pilot->range>weapon->maximumRange || (unsigned)view->heat+weapon->heat>pilot->orders.heatCeiling)continue;
        controls.commandScan=view->selectedWeapon==mount?PILOT_SCAN_FIRE:(uint8_t)(2+mount);
        pilot->weaponReason=view->selectedWeapon==mount?"fire":"select weapon";goto done;
    }
    pilot->weaponReason="reload, ammunition, range or heat limit";
done:
    pilot->controls=controls;return controls;
}
