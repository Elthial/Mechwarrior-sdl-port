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
    if (!activeUnitControl || !activeUnitPositionedObject) abort();
    BtechUnitControlState *control=activeUnitControl;
    BtechPosition3D waypoint;
    if (BtechHasNearbyVisibleThreat() && combatTickCounter)
    {
        BtechSelectUnitTacticalState();
        if (control->trackedUnitOrSpecialSlot>=0)
        {
            uint16_t tracked=control->trackedUnitOrSpecialSlot;
            if (tracked==COMBAT_SPECIAL_TARGET_SLOT)
            {
                if (!specialTargetObject) abort();
                BtechPosition3D *position=&specialTargetObject->object.prefix.position;
                control->trackedPosition[0]=(int32_t)position->positionA;
                control->trackedPosition[1]=(int32_t)position->positionB;
                control->trackedPosition[2]=(int32_t)position->positionC;
            }
            else
            {
                if (tracked>=COMBAT_MECH_SLOTS) abort();
                for (uint16_t axis=0;axis<3;++axis) control->trackedPosition[axis]=unitControls[tracked].lastVisiblePosition[axis];
            }
        }
        if (control->targetUnitSlot>=0)
        {
            int16_t value;
            if (control->targetUnitSlot==COMBAT_SPECIAL_TARGET_SLOT)
                value=specialTargetObject && specialTargetDamageRemaining?0:-1;
            else
            {
                if ((uint16_t)control->targetUnitSlot>=COMBAT_MECH_SLOTS)
                    abort();
                value=unitControls[control->targetUnitSlot].combatValueOrBoundaryCode;
            }
            if (value>-1) return;
        }
        if (control->controlState==TACTICAL_DEFENDING) return;
    }
    control->decisionCountdown=0;control->controlState=COMBAT_TACTICAL_UNENGAGED;
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
        control->controlState=TACTICAL_RETREATING;
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
                if (!specialTargetObject) abort();
                BtechNavigateActiveUnitTowardPosition(&specialTargetObject->object.prefix.position);
                control->controlState=TACTICAL_CHARGING_BASE;
            }
            else
            {
                if ((uint16_t)control->trackedUnitOrSpecialSlot>=COMBAT_MECH_SLOTS) abort();
                int32_t *lastSeen=unitControls[control->trackedUnitOrSpecialSlot].lastVisiblePosition;
                waypoint=(BtechPosition3D){(uint32_t)lastSeen[0],(uint32_t)lastSeen[1],(uint32_t)lastSeen[2]};
                BtechNavigateActiveUnitTowardPosition(&waypoint);
            }
        }
        else if (control->commandCode==TACTICAL_ORDER_DEFEND)
        {
            if (control->sideIndex>=2) abort();
            if (sideObjectiveRules[control->sideIndex]==OBJECTIVE_DEFEND_SPECIAL_TARGET) {control->controlState=TACTICAL_DEFENDING;return;}
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
    if (!activeSideRule) abort();
    switch (*activeSideRule)
    {
    case OBJECTIVE_ELIMINATE_OPPOSITION:
    case OBJECTIVE_OPPOSE_WITHDRAWAL: BtechRefreshTargetAndNavigate();break;
    case OBJECTIVE_WITHDRAW:
    {
        waypoint=activeUnitPositionedObject->object.prefix.position;
        if (control->sideIndex>=2) abort();
        uint8_t mask=scenarioSelectors.sideBoundaryMasks[control->sideIndex];
        if (mask&1) waypoint.positionB=TACTICAL_RETREAT_Y;
        else if (mask&2) waypoint.positionB=(uint32_t)-TACTICAL_RETREAT_Y;
        if (mask&4) waypoint.positionA=(uint32_t)-TACTICAL_RETREAT_X;
        else if (mask&8) waypoint.positionA=TACTICAL_RETREAT_X;
        BtechNavigateActiveUnitTowardPosition(&waypoint);break;
    }
    case OBJECTIVE_SPECIAL_TARGET:
        control->controlState=TACTICAL_CHARGING_BASE;control->navigationReferenceSlot=COMBAT_SPECIAL_TARGET_SLOT;
        if (!specialTargetObject) abort();
        waypoint=specialTargetObject->object.prefix.position;
        BtechNavigateActiveUnitTowardPosition(&waypoint);break;
    case OBJECTIVE_DEFEND_SPECIAL_TARGET:control->controlState=TACTICAL_DEFENDING;break;
    }
}

/* Reassess whether this MechWarrior should close, track, avoid or defend,
 * respecting lance orders, mission objectives and the decision countdown.
 * Retain the current heading when threats and orders supply no new direction. */
void BtechSelectUnitTacticalState(void)
{
    if (!activeUnitControl || !activeUnitPositionedObject) abort();
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
            if (!BtechArePlanarPositionsWithinArrivalBounds(&root->position,&destination)) control->controlState=TACTICAL_AVOIDING;
            else
            {
                control->controlState=TACTICAL_TRACKING;
                if (control->unitSlot>=COMBAT_MECH_SLOTS) abort();
                for (uint16_t slot=0;slot<COMBAT_MECH_SLOTS;++slot)
                    if (unitControls[slot].sideIndex==control->sideIndex
                        && (unitControls[slot].targetUnitSlot==control->unitSlot || (int32_t)unitDistances[slot][control->unitSlot]<control->quarterWeaponRange)) control->controlState=TACTICAL_CHARGING;
            }
            break;
        }
        case TACTICAL_ORDER_DEFEND:
            if (control->sideIndex>=2) abort();
            control->controlState=sideObjectiveRules[control->sideIndex]==OBJECTIVE_DEFEND_SPECIAL_TARGET?TACTICAL_DEFENDING:TACTICAL_CHARGING;break;
        case TACTICAL_ORDER_ATTACK:
        case TACTICAL_ORDER_MOVE_ATTACK:control->controlState=control->trackedUnitOrSpecialSlot==COMBAT_SPECIAL_TARGET_SLOT?TACTICAL_CHARGING_BASE:TACTICAL_CHARGING;break;
        case TACTICAL_ORDER_MOVE_AVOID:control->controlState=TACTICAL_AVOIDING;break;
        case COMBAT_ORDER_ACT_ON_OWN:
            if (!activeSideRule) abort();
            switch (*activeSideRule)
            {
            case OBJECTIVE_ELIMINATE_OPPOSITION:
            case OBJECTIVE_OPPOSE_WITHDRAWAL:control->controlState=TACTICAL_CHARGING;break;
            case OBJECTIVE_WITHDRAW:control->controlState=TACTICAL_AVOIDING;break;
            case OBJECTIVE_SPECIAL_TARGET:control->controlState=TACTICAL_CHARGING_BASE;break;
            case OBJECTIVE_DEFEND_SPECIAL_TARGET:control->controlState=TACTICAL_DEFENDING;break;
            }
            break;
        }
    }
    uint16_t threatCount=0,strongestHeadingIndex=0,selectedHeading=root->orientationC;
    int16_t combinedThreatValue=0,strongestThreat=-1;
    int16_t savedPlayerValue=unitControls[0].combatValueOrBoundaryCode;
    int16_t halfPlayerValue=(int16_t)(((uint16_t)savedPlayerValue>>1)|((uint16_t)savedPlayerValue&0x8000));
    unitControls[0].combatValueOrBoundaryCode=(int16_t)(uint16_t)(savedPlayerValue+halfPlayerValue);
    if (control->unitSlot>=COMBAT_MECH_SLOTS) abort();
    for (uint16_t slot=0;slot<COMBAT_MECH_SLOTS;++slot)
    {
        if (!unitBindings[slot].mech) continue;
        BtechUnitControlState *enemy=&unitControls[slot];
        if (enemy->sideIndex==control->sideIndex || enemy->combatValueOrBoundaryCode<=0 || !unitVisibility[slot][control->unitSlot]) continue;
        int16_t halfRange=(int16_t)(((uint16_t)enemy->weaponRangeMinusMargin>>1)|((uint16_t)enemy->weaponRangeMinusMargin&0x8000));
        int16_t range=(int16_t)(uint16_t)(halfRange+enemy->weaponRangeMinusMargin);
        if (range<=(int32_t)unitDistances[slot][control->unitSlot]) continue;
        uint16_t heading=BtechCalculateVectorHeading((uint32_t)enemy->lastVisiblePosition[0]-root->position.positionA,(uint32_t)enemy->lastVisiblePosition[1]-root->position.positionB);
        BtechPooledWorldObject *enemyRoot=unitBindings[slot].positionedLeftLeg,*enemyTorso=unitBindings[slot].torso;
        if (!enemyRoot || !enemyTorso) abort();
        int16_t facing=(int16_t)(uint16_t)(enemyRoot->object.prefix.orientationC+enemyTorso->object.prefix.orientationC-heading+AVOIDANCE_HALF_TURN);
        if (facing>=TACTICAL_THREAT_FACING_LIMIT) continue; /* Signed one-sided, not abs(angle). */
        tacticalThreatUnitSlots[threatCount]=slot;tacticalThreatHeadings[threatCount]=heading;
        combinedThreatValue=(int16_t)(uint16_t)(combinedThreatValue+enemy->combatValueOrBoundaryCode);
        if (strongestThreat<0 || unitControls[strongestThreat].combatValueOrBoundaryCode<enemy->combatValueOrBoundaryCode)
        {strongestHeadingIndex=threatCount;strongestThreat=slot;}
        ++threatCount;
    }
    unitControls[0].combatValueOrBoundaryCode=savedPlayerValue;
    if (threatCount)
    {
        if (control->controlState==TACTICAL_CHARGING_BASE)
        {
            if (!specialTargetObject) abort();
            int32_t distance=(int32_t)BtechApproximatePlanarDistance(&root->position,&specialTargetObject->object.prefix.position);
            if (distance<=TACTICAL_OBJECTIVE_COMMIT_RANGE && (int16_t)(uint16_t)(BtechRandomBelowBound(TACTICAL_OBJECTIVE_COMMIT_ROLL)+TACTICAL_OBJECTIVE_COMMIT_ROLL)>=(int16_t)specialTargetDamageRemaining) goto steer;
        }
        if (control->controlState==TACTICAL_CHARGING_BASE || control->controlState==TACTICAL_DEFENDING) control->controlState=TACTICAL_CHARGING;
        control->targetUnitSlot=strongestThreat;selectedHeading=tacticalThreatHeadings[strongestHeadingIndex];
        if (control->controlState!=TACTICAL_WANDERING)
        {
            if ((int16_t)(uint16_t)(control->combatValueOrBoundaryCode*3)<combinedThreatValue) control->controlState=TACTICAL_FLEEING;
            else if (unitControls[strongestThreat].combatValueOrBoundaryCode>(int16_t)(uint16_t)(control->combatValueOrBoundaryCode*2)) control->controlState=TACTICAL_AVOIDING;
        }
    }
    else if (control->controlState==TACTICAL_CHARGING_BASE)
    {
        if (!specialTargetObject) abort();
        BtechPosition3D *position=&specialTargetObject->object.prefix.position;
        selectedHeading=BtechCalculateVectorHeading(position->positionA-root->position.positionA,position->positionB-root->position.positionB);
        control->targetUnitSlot=COMBAT_SPECIAL_TARGET_SLOT;
    }
    else if (control->controlState==TACTICAL_CHARGING)
    {
        if (control->trackedUnitOrSpecialSlot>=0 && control->trackedUnitOrSpecialSlot<COMBAT_MECH_SLOTS) control->targetUnitSlot=control->trackedUnitOrSpecialSlot;
        else BtechScoreAndSelectTacticalTarget();
        if (control->targetUnitSlot>=0)
        {
            if (control->targetUnitSlot>=COMBAT_MECH_SLOTS || !unitBindings[control->targetUnitSlot].positionedLeftLeg) abort();
            BtechPosition3D *position=&unitBindings[control->targetUnitSlot].positionedLeftLeg->object.prefix.position;
            selectedHeading=BtechCalculateVectorHeading(position->positionA-root->position.positionA,position->positionB-root->position.positionB);
        }
        else selectedHeading=root->orientationC;
    }
steer:
    if (control->controlState==TACTICAL_AVOIDING)
    {
        if (threatCount)
        {
            uint16_t sum=0;
            for (uint16_t threat=0;threat<threatCount;++threat) sum+=tacticalThreatHeadings[threat];
            selectedHeading=(uint16_t)((int16_t)sum/(int16_t)threatCount);
        }
        else {control->controlState=COMBAT_TACTICAL_UNENGAGED;selectedHeading=root->orientationC;}
        BtechSteerActiveUnitAroundObstacles(selectedHeading,1);
    }
    else
    {
        BtechSteerActiveUnitHeading(selectedHeading);
    }
}

/*0810:8313. Torso adjustment occurs even when the state makes no motion call.
 * Preserve the unused charging RNG draw and strict hysteresis range tests. */
void BtechSteerActiveUnitHeading(uint16_t desiredHeading)
{
    if (!activeUnitTorsoObject || !activeUnitPositionedObject || !activeUnitControl) abort();
    int16_t relativeHeading=(int16_t)activeUnitTorsoObject->object.prefix.orientationC;
    int16_t error=(int16_t)(uint16_t)(activeUnitPositionedObject->object.prefix.orientationC+relativeHeading-desiredHeading);
    int16_t magnitude=(uint16_t)error==0x8000?INT16_MAX:error<0?-error:error;
    int16_t direction=error<0?-1:error!=0;
    if (magnitude>TACTICAL_TORSO_TURN_STEP) magnitude=TACTICAL_TORSO_TURN_STEP;
    relativeHeading=(int16_t)(uint16_t)(relativeHeading-magnitude*direction);
    magnitude=(uint16_t)relativeHeading==0x8000?INT16_MAX:relativeHeading<0?-relativeHeading:relativeHeading;
    if (magnitude<TACTICAL_TORSO_TURN_LIMIT) activeUnitTorsoObject->object.prefix.orientationC=(uint16_t)relativeHeading;
    BtechUnitControlState *control=activeUnitControl;
    switch (control->controlState)
    {
    case TACTICAL_CHARGING:
    {
        if (control->unitSlot>=COMBAT_MECH_SLOTS || (uint16_t)control->targetUnitSlot>=COMBAT_MECH_SLOTS) abort();
        int32_t distance=(int32_t)unitDistances[control->unitSlot][control->targetUnitSlot];
        (void)BtechRandomBelowBound((uint16_t)(((uint16_t)control->targetUnitSlot<<4)+1));
        if (control->quarterWeaponRange>distance) control->maneuverFlags&=(uint8_t)~TACTICAL_OUTER_RANGE;
        else if (control->weaponRangeMinusMargin<distance) control->maneuverFlags|=TACTICAL_OUTER_RANGE;
        uint16_t offset=(control->maneuverFlags&TACTICAL_OUTER_RANGE)?TACTICAL_OUTER_HEADING_OFFSET:TACTICAL_INNER_HEADING_OFFSET;
        BtechSteerActiveUnitAroundObstacles((uint16_t)(desiredHeading+offset),control->maneuverFlags&TACTICAL_OUTER_RANGE);break;
    }
    case TACTICAL_TRACKING:BtechSetActiveUnitGroundOrJumpMotion(desiredHeading,MOTION_DIRECTION_STOP);break;
    case TACTICAL_FLEEING:BtechSteerActiveUnitAroundObstacles((uint16_t)(desiredHeading+TACTICAL_FLEE_HEADING_OFFSET),1);break;
    case TACTICAL_WANDERING:
    {
        uint16_t offset=(control->maneuverFlags&TACTICAL_POSITIVE_QUARTER_TURN)?TACTICAL_POSITIVE_QUARTER:TACTICAL_NEGATIVE_QUARTER;
        desiredHeading=activeUnitPositionedObject->object.prefix.orientationC+offset;
        BtechSetActiveUnitGroundOrJumpMotion(desiredHeading,!(control->maneuverFlags&TACTICAL_OUTER_RANGE));break;
    }
    case TACTICAL_CHARGING_BASE:
    {
        if (!specialTargetObject) abort();
        int32_t distance=(int32_t)BtechApproximatePlanarDistance(&activeUnitPositionedObject->object.prefix.position,&specialTargetObject->object.prefix.position);
        if (control->weaponRangeMinusMargin>=distance && !specialTargetInteractionBlocked) BtechSetActiveUnitGroundOrJumpMotion(desiredHeading,MOTION_DIRECTION_STOP);
        else BtechSteerActiveUnitAroundObstacles(desiredHeading,1);
        break;
    }
    }
}
