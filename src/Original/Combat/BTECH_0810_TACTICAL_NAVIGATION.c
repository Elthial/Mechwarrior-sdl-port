#include "mechwarrior/port_diagnostics.h"
#include "mechwarrior/combat_tactics.h"
#include "mechwarrior/combat_movement.h"
#include "mechwarrior/combat_damage.h"
#include "mechwarrior/combat_pose.h"
#include "mechwarrior/combat_ballistics.h"
#include "mechwarrior/combat_import.h"
#include "mechwarrior/combat_startup.h"
#include "mechwarrior/combat_render.h"
#include <stdlib.h>

/* Turn the active MechWarrior's orders and target knowledge into a destination
 * and movement state; a lost contact is pursued at its last visible position.
 *0810:77A3. Original tracked/last-visible/order positions are distinct.
 * Normal complete owners cannot overlap their forward coordinate copies. */
void BtechUpdateTacticalNavigation(void)
{
    if (!activeUnitControl || !activeUnitPositionedObject) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !activeUnitControl || !activeUnitPositionedObject");
    BtechUnitControlState *control=activeUnitControl;
    BtechPosition3D waypoint;
    if (BtechHasNearbyVisibleThreat() && combatTickCounter)
    {
        BtechSelectUnitTacticalState();
        if (control->trackedUnitOrSpecialSlot>=0)
        {
            uint16_t trackedTargetSlot=control->trackedUnitOrSpecialSlot;
            if (trackedTargetSlot==COMBAT_SPECIAL_TARGET_SLOT)
            {
                if (!specialTargetObject) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !specialTargetObject");
                BtechPosition3D *position=&specialTargetObject->object.prefix.position;
                control->trackedPosition[0]=(int32_t)position->positionA;
                control->trackedPosition[1]=(int32_t)position->positionB;
                control->trackedPosition[2]=(int32_t)position->positionC;
            }
            else
            {
                if (trackedTargetSlot>=COMBAT_MECH_SLOTS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: trackedTargetSlot>=COMBAT_MECH_SLOTS");
                for (uint16_t axis=0;axis<3;++axis) control->trackedPosition[axis]=unitControls[trackedTargetSlot].lastVisiblePosition[axis];
            }
        }
        if (control->targetUnitSlot>=0)
        {
            int16_t targetConditionOrBoundaryCode;
            if (control->targetUnitSlot==COMBAT_SPECIAL_TARGET_SLOT)
                targetConditionOrBoundaryCode=specialTargetObject && specialTargetDamageRemaining?COMBAT_CONDITION_SCORE_MINIMUM:COMBAT_MECH_DISABLED_VALUE;
            else
            {
                if ((uint16_t)control->targetUnitSlot>=COMBAT_MECH_SLOTS)
                    /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: (uint16_t)control->targetUnitSlot>=COMBAT_MECH_SLOTS");
                targetConditionOrBoundaryCode=unitControls[control->targetUnitSlot].conditionScoreOrExitCode;
            }
            if (targetConditionOrBoundaryCode>COMBAT_MECH_DISABLED_VALUE) return;
        }
        if (control->tacticalState==TACTICAL_DEFENDING) return;
    }
    control->decisionCountdown=0;control->tacticalState=COMBAT_TACTICAL_UNENGAGED;
    control->lastDirectHitUnitSlot=-1;control->trackedPosition[2]=-1;control->navigationReferenceSlot=-1;
    if (BtechShouldDisengageForWeaponsOrStructure())
    {
        waypoint=activeUnitPositionedObject->object.prefix.position;
        uint32_t magnitudeX=(int32_t)waypoint.positionA<0?0-waypoint.positionA:waypoint.positionA;
        uint32_t magnitudeY=(int32_t)waypoint.positionB<0?0-waypoint.positionB:waypoint.positionB;
        if ((int32_t)magnitudeX>(int32_t)magnitudeY)
            waypoint.positionA=(int32_t)waypoint.positionA<0?(uint32_t)-TACTICAL_RETREAT_X:waypoint.positionA?TACTICAL_RETREAT_X:0;
        else waypoint.positionB=(int32_t)waypoint.positionB<0?(uint32_t)-TACTICAL_RETREAT_Y:waypoint.positionB?TACTICAL_RETREAT_Y:0;
        BtechNavigateActiveUnitTowardPosition(&waypoint);
        control->tacticalState=TACTICAL_RETREATING;
        return;
    }
    if (control->commandCode!=COMBAT_ORDER_ACT_ON_OWN)
    {
        if (control->commandCode>TACTICAL_ORDER_MOVE_AVOID) return;
        if (control->commandCode==TACTICAL_ORDER_ATTACK)
        {
            control->navigationReferenceSlot=control->trackedUnitOrSpecialSlot;
            if (control->trackedUnitOrSpecialSlot<0) BtechRefreshTargetAndNavigate();
            else if (control->trackedUnitOrSpecialSlot==COMBAT_SPECIAL_TARGET_SLOT)
            {
                if (!specialTargetObject) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !specialTargetObject");
                BtechNavigateActiveUnitTowardPosition(&specialTargetObject->object.prefix.position);
                control->tacticalState=TACTICAL_CHARGING_BASE;
            }
            else
            {
                if ((uint16_t)control->trackedUnitOrSpecialSlot>=COMBAT_MECH_SLOTS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: (uint16_t)control->trackedUnitOrSpecialSlot>=COMBAT_MECH_SLOTS");
                int32_t *lastSeen=unitControls[control->trackedUnitOrSpecialSlot].lastVisiblePosition;
                waypoint=(BtechPosition3D){(uint32_t)lastSeen[0],(uint32_t)lastSeen[1],(uint32_t)lastSeen[2]};
                BtechNavigateActiveUnitTowardPosition(&waypoint);
            }
        }
        else if (control->commandCode==TACTICAL_ORDER_DEFEND)
        {
            if (control->sideIndex>=2) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: control->sideIndex>=2");
            if (sideObjectiveRules[control->sideIndex]==OBJECTIVE_DEFEND_SPECIAL_TARGET) {control->tacticalState=TACTICAL_DEFENDING;return;}
            waypoint=(BtechPosition3D){(uint32_t)control->orderDestinationPosition[0],(uint32_t)control->orderDestinationPosition[1],(uint32_t)control->orderDestinationPosition[2]};
            BtechNavigateActiveUnitTowardPosition(&waypoint);
        }
        else
        {
            waypoint=(BtechPosition3D){(uint32_t)control->orderDestinationPosition[0],(uint32_t)control->orderDestinationPosition[1],(uint32_t)control->orderDestinationPosition[2]};
            BtechNavigateActiveUnitTowardPosition(&waypoint);
        }
        return;
    }
    if (!activeSideRule) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !activeSideRule");
    switch (*activeSideRule)
    {
    case OBJECTIVE_ELIMINATE_OPPOSITION:
    case OBJECTIVE_OPPOSE_WITHDRAWAL: BtechRefreshTargetAndNavigate();break;
    case OBJECTIVE_WITHDRAW:
    {
        waypoint=activeUnitPositionedObject->object.prefix.position;
        if (control->sideIndex>=2) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: control->sideIndex>=2");
        uint8_t mask=scenarioSelectors.sideBoundaryMasks[control->sideIndex];
        if (mask&1) waypoint.positionB=TACTICAL_RETREAT_Y;
        else if (mask&2) waypoint.positionB=(uint32_t)-TACTICAL_RETREAT_Y;
        if (mask&4) waypoint.positionA=(uint32_t)-TACTICAL_RETREAT_X;
        else if (mask&8) waypoint.positionA=TACTICAL_RETREAT_X;
        BtechNavigateActiveUnitTowardPosition(&waypoint);break;
    }
    case OBJECTIVE_SPECIAL_TARGET:
        control->tacticalState=TACTICAL_CHARGING_BASE;control->navigationReferenceSlot=COMBAT_SPECIAL_TARGET_SLOT;
        if (!specialTargetObject) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !specialTargetObject");
        waypoint=specialTargetObject->object.prefix.position;
        BtechNavigateActiveUnitTowardPosition(&waypoint);break;
    case OBJECTIVE_DEFEND_SPECIAL_TARGET:control->tacticalState=TACTICAL_DEFENDING;break;
    }
}

/* Reassess whether this MechWarrior should close, track, avoid or defend,
 * respecting lance orders, mission objectives and the decision countdown.
 * Retain the current heading when threats and orders supply no new direction. */
void BtechSelectUnitTacticalState(void)
{
    if (!activeUnitControl || !activeUnitPositionedObject) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !activeUnitControl || !activeUnitPositionedObject");
    BtechUnitControlState *control=activeUnitControl;
    BtechWorldObjectPrefix *root=&activeUnitPositionedObject->object.prefix;
    --control->decisionCountdown;
    if ((int16_t)control->decisionCountdown<0)
    {
        control->decisionCountdown=0;
        switch (control->commandCode)
        {
        case TACTICAL_ORDER_AMBUSH:
        {
            BtechPosition3D destination={(uint32_t)control->orderDestinationPosition[0],(uint32_t)control->orderDestinationPosition[1],(uint32_t)control->orderDestinationPosition[2]};
            if (!BtechArePlanarPositionsWithinArrivalBounds(&root->position,&destination)) control->tacticalState=TACTICAL_AVOIDING;
            else
            {
                control->tacticalState=TACTICAL_TRACKING;
                if (control->unitSlot>=COMBAT_MECH_SLOTS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: control->unitSlot>=COMBAT_MECH_SLOTS");
                for (uint16_t slot=0;slot<COMBAT_MECH_SLOTS;++slot)
                    if (unitControls[slot].sideIndex==control->sideIndex
                        && (unitControls[slot].targetUnitSlot==control->unitSlot || (int32_t)unitDistances[slot][control->unitSlot]<control->quarterReadyWeaponRange)) control->tacticalState=TACTICAL_CHARGING;
            }
            break;
        }
        case TACTICAL_ORDER_DEFEND:
            if (control->sideIndex>=2) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: control->sideIndex>=2");
            control->tacticalState=sideObjectiveRules[control->sideIndex]==OBJECTIVE_DEFEND_SPECIAL_TARGET?TACTICAL_DEFENDING:TACTICAL_CHARGING;break;
        case TACTICAL_ORDER_ATTACK:
        case TACTICAL_ORDER_MOVE_ATTACK:control->tacticalState=control->trackedUnitOrSpecialSlot==COMBAT_SPECIAL_TARGET_SLOT?TACTICAL_CHARGING_BASE:TACTICAL_CHARGING;break;
        case TACTICAL_ORDER_MOVE_AVOID:control->tacticalState=TACTICAL_AVOIDING;break;
        case COMBAT_ORDER_ACT_ON_OWN:
            if (!activeSideRule) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !activeSideRule");
            switch (*activeSideRule)
            {
            case OBJECTIVE_ELIMINATE_OPPOSITION:
            case OBJECTIVE_OPPOSE_WITHDRAWAL:control->tacticalState=TACTICAL_CHARGING;break;
            case OBJECTIVE_WITHDRAW:control->tacticalState=TACTICAL_AVOIDING;break;
            case OBJECTIVE_SPECIAL_TARGET:control->tacticalState=TACTICAL_CHARGING_BASE;break;
            case OBJECTIVE_DEFEND_SPECIAL_TARGET:control->tacticalState=TACTICAL_DEFENDING;break;
            }
            break;
        }
    }
    uint16_t threatCount=0,strongestThreatHeadingIndex=0,selectedHeading=root->orientationC;
    int16_t combinedThreatValue=0,strongestThreatSlot=-1;
    int16_t savedPlayerConditionScore=unitControls[0].conditionScoreOrExitCode;
    int16_t halfPlayerConditionScore=((uint16_t)savedPlayerConditionScore>>1)|((uint16_t)savedPlayerConditionScore&0x8000);
    unitControls[0].conditionScoreOrExitCode=(int16_t)(savedPlayerConditionScore+halfPlayerConditionScore);
    if (control->unitSlot>=COMBAT_MECH_SLOTS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: control->unitSlot>=COMBAT_MECH_SLOTS");
    for (uint16_t slot=0;slot<COMBAT_MECH_SLOTS;++slot)
    {
        if (!unitBindings[slot].mech) continue;
        BtechUnitControlState *enemy=&unitControls[slot];
        if (enemy->sideIndex==control->sideIndex || enemy->conditionScoreOrExitCode<=COMBAT_CONDITION_SCORE_MINIMUM || !unitVisibility[slot][control->unitSlot]) continue;
        int16_t halfEngagementRange=((uint16_t)enemy->readyWeaponRangeMinusMargin>>1)|((uint16_t)enemy->readyWeaponRangeMinusMargin&0x8000);
        int16_t threatEngagementRange=halfEngagementRange+enemy->readyWeaponRangeMinusMargin;
        if (threatEngagementRange<=(int32_t)unitDistances[slot][control->unitSlot]) continue;
        uint16_t heading=BtechCalculateVectorHeading((uint32_t)enemy->lastVisiblePosition[0]-root->position.positionA,(uint32_t)enemy->lastVisiblePosition[1]-root->position.positionB);
        BtechPooledWorldObject *enemyRoot=unitBindings[slot].positionedLeftLeg,*enemyTorso=unitBindings[slot].torso;
        if (!enemyRoot || !enemyTorso) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !enemyRoot || !enemyTorso");
        int16_t threatFacingDifference=enemyRoot->object.prefix.orientationC+enemyTorso->object.prefix.orientationC-heading+AVOIDANCE_HALF_TURN;
        if (threatFacingDifference>=TACTICAL_THREAT_FACING_LIMIT) continue; /* Signed one-sided, not abs(angle). */
        tacticalThreatUnitSlots[threatCount]=slot;tacticalThreatHeadings[threatCount]=heading;
        combinedThreatValue=(int16_t)(combinedThreatValue+enemy->conditionScoreOrExitCode);
        if (strongestThreatSlot<0 || unitControls[strongestThreatSlot].conditionScoreOrExitCode<enemy->conditionScoreOrExitCode)
        {strongestThreatHeadingIndex=threatCount;strongestThreatSlot=slot;}
        ++threatCount;
    }
    unitControls[0].conditionScoreOrExitCode=savedPlayerConditionScore;
    if (threatCount)
    {
        if (control->tacticalState==TACTICAL_CHARGING_BASE)
        {
            if (!specialTargetObject) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !specialTargetObject");
            int32_t distance=(int32_t)BtechApproximatePlanarDistance(&root->position,&specialTargetObject->object.prefix.position);
            if (distance<=TACTICAL_OBJECTIVE_COMMIT_RANGE && (int16_t)(BtechRandomBelowBound(TACTICAL_OBJECTIVE_COMMIT_ROLL)+TACTICAL_OBJECTIVE_COMMIT_ROLL)>=(int16_t)specialTargetDamageRemaining) goto steer;
        }
        if (control->tacticalState==TACTICAL_CHARGING_BASE || control->tacticalState==TACTICAL_DEFENDING) control->tacticalState=TACTICAL_CHARGING;
        control->targetUnitSlot=strongestThreatSlot;selectedHeading=tacticalThreatHeadings[strongestThreatHeadingIndex];
        if (control->tacticalState!=TACTICAL_WANDERING)
        {
            if ((int16_t)(control->conditionScoreOrExitCode*3)<combinedThreatValue) control->tacticalState=TACTICAL_FLEEING;
            else if (unitControls[strongestThreatSlot].conditionScoreOrExitCode>(int16_t)(control->conditionScoreOrExitCode*2)) control->tacticalState=TACTICAL_AVOIDING;
        }
    }
    else if (control->tacticalState==TACTICAL_CHARGING_BASE)
    {
        if (!specialTargetObject) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !specialTargetObject");
        BtechPosition3D *position=&specialTargetObject->object.prefix.position;
        selectedHeading=BtechCalculateVectorHeading(position->positionA-root->position.positionA,position->positionB-root->position.positionB);
        control->targetUnitSlot=COMBAT_SPECIAL_TARGET_SLOT;
    }
    else if (control->tacticalState==TACTICAL_CHARGING)
    {
        if (control->trackedUnitOrSpecialSlot>=0 && control->trackedUnitOrSpecialSlot<COMBAT_MECH_SLOTS) control->targetUnitSlot=control->trackedUnitOrSpecialSlot;
        else BtechScoreAndSelectTacticalTarget();
        if (control->targetUnitSlot>=0)
        {
            if (control->targetUnitSlot>=COMBAT_MECH_SLOTS || !unitBindings[control->targetUnitSlot].positionedLeftLeg) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: control->targetUnitSlot>=COMBAT_MECH_SLOTS || !unitBindings[control->targetUnitSlot].positionedLeftLeg");
            BtechPosition3D *position=&unitBindings[control->targetUnitSlot].positionedLeftLeg->object.prefix.position;
            selectedHeading=BtechCalculateVectorHeading(position->positionA-root->position.positionA,position->positionB-root->position.positionB);
        }
        else selectedHeading=root->orientationC;
    }
steer:
    if (control->tacticalState==TACTICAL_AVOIDING)
    {
        if (threatCount)
        {
            uint16_t headingSum=0;
            for (uint16_t threat=0;threat<threatCount;++threat) headingSum+=tacticalThreatHeadings[threat];
            selectedHeading=(uint16_t)((int16_t)headingSum/(int16_t)threatCount);
        }
        else {control->tacticalState=COMBAT_TACTICAL_UNENGAGED;selectedHeading=root->orientationC;}
        BtechSteerActiveUnitAroundObstacles(selectedHeading,1);
    }
    else
    {
        BtechApplyActiveUnitTacticalSteering(selectedHeading);
    }
}

/* Track the threat with the torso, then move according to engagement tactics.
 *0810:8313. Torso adjustment occurs even when the state makes no motion call.
 * Preserve the unused charging RNG draw and strict hysteresis range tests. */
void BtechApplyActiveUnitTacticalSteering(uint16_t desiredHeading)
{
    if (!activeUnitTorsoObject || !activeUnitPositionedObject || !activeUnitControl) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !activeUnitTorsoObject || !activeUnitPositionedObject || !activeUnitControl");
    int16_t relativeTorsoHeading=activeUnitTorsoObject->object.prefix.orientationC;
    int16_t torsoHeadingError=activeUnitPositionedObject->object.prefix.orientationC+relativeTorsoHeading-desiredHeading;
    int16_t headingErrorMagnitude=(uint16_t)torsoHeadingError==0x8000?INT16_MAX:torsoHeadingError<0?-torsoHeadingError:torsoHeadingError;
    int16_t turnDirection=torsoHeadingError<0?-1:torsoHeadingError!=0;
    if (headingErrorMagnitude>TACTICAL_TORSO_TURN_STEP) headingErrorMagnitude=TACTICAL_TORSO_TURN_STEP;
    relativeTorsoHeading=(int16_t)(relativeTorsoHeading-headingErrorMagnitude*turnDirection);
    headingErrorMagnitude=(uint16_t)relativeTorsoHeading==0x8000?INT16_MAX:relativeTorsoHeading<0?-relativeTorsoHeading:relativeTorsoHeading;
    if (headingErrorMagnitude<TACTICAL_TORSO_TURN_LIMIT) activeUnitTorsoObject->object.prefix.orientationC=(uint16_t)relativeTorsoHeading;
    BtechUnitControlState *control=activeUnitControl;
    switch (control->tacticalState)
    {
    case TACTICAL_CHARGING:
    {
        if (control->unitSlot>=COMBAT_MECH_SLOTS || (uint16_t)control->targetUnitSlot>=COMBAT_MECH_SLOTS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: control->unitSlot>=COMBAT_MECH_SLOTS || (uint16_t)control->targetUnitSlot>=COMBAT_MECH_SLOTS");
        int32_t distance=unitDistances[control->unitSlot][control->targetUnitSlot];
        (void)BtechRandomBelowBound((uint16_t)(((uint16_t)control->targetUnitSlot<<4)+1));
        if (control->quarterReadyWeaponRange>distance) control->maneuverFlags&=(uint8_t)~TACTICAL_OUTER_RANGE;
        else if (control->readyWeaponRangeMinusMargin<distance) control->maneuverFlags|=TACTICAL_OUTER_RANGE;
        uint16_t maneuverHeadingOffset=(control->maneuverFlags&TACTICAL_OUTER_RANGE)?TACTICAL_OUTER_HEADING_OFFSET:TACTICAL_INNER_HEADING_OFFSET;
        BtechSteerActiveUnitAroundObstacles((uint16_t)(desiredHeading+maneuverHeadingOffset),control->maneuverFlags&TACTICAL_OUTER_RANGE);break;
    }
    case TACTICAL_TRACKING:BtechSetActiveUnitGroundOrJumpMotion(desiredHeading,MOTION_DIRECTION_STOP);break;
    case TACTICAL_FLEEING:BtechSteerActiveUnitAroundObstacles((uint16_t)(desiredHeading+TACTICAL_FLEE_HEADING_OFFSET),1);break;
    case TACTICAL_WANDERING:
    {
        uint16_t maneuverHeadingOffset=(control->maneuverFlags&TACTICAL_POSITIVE_QUARTER_TURN)?TACTICAL_POSITIVE_QUARTER:TACTICAL_NEGATIVE_QUARTER;
        desiredHeading=activeUnitPositionedObject->object.prefix.orientationC+maneuverHeadingOffset;
        BtechSetActiveUnitGroundOrJumpMotion(desiredHeading,!(control->maneuverFlags&TACTICAL_OUTER_RANGE));break;
    }
    case TACTICAL_CHARGING_BASE:
    {
        if (!specialTargetObject) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !specialTargetObject");
        int32_t distance=(int32_t)BtechApproximatePlanarDistance(&activeUnitPositionedObject->object.prefix.position,&specialTargetObject->object.prefix.position);
        if (control->readyWeaponRangeMinusMargin>=distance && !specialTargetInteractionBlocked) BtechSetActiveUnitGroundOrJumpMotion(desiredHeading,MOTION_DIRECTION_STOP);
        else BtechSteerActiveUnitAroundObstacles(desiredHeading,1);
        break;
    }
    }
}
