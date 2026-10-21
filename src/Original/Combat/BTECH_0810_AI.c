#include "mechwarrior/port_diagnostics.h"
#include "mechwarrior/combat_tactics.h"
#include "mechwarrior/combat_movement.h"
#include "mechwarrior/combat_pose.h"
#include "mechwarrior/combat_ballistics.h"
#include "mechwarrior/combat_import.h"
#include "mechwarrior/combat_startup.h"
#include "mechwarrior/combat_render.h"
#include <stdlib.h>

/*0810:71ED. The actual non-player update chain, not a replacement AI tick.
 * Global control finishes one-past the eight records unless739C rebinds it. */
void BtechUpdateAiMechUnits(void)
{
    for (uint16_t slot=0;slot<COMBAT_MECH_SLOTS;++slot) unitControls[slot].targetSelectionCount=0;
    BtechRefreshLastVisibleUnitPositions();
    BtechRecalculateUnitCombatValuesAndBoundaries();
    BtechUpdateUnitWeaponRangeThresholds();
    BtechUpdateSideCompletionFlags();
    activeUnitControl=&unitControls[1];
    for (uint16_t slot=1;slot<COMBAT_MECH_SLOTS;++slot,++activeUnitControl)
    {
        activeMechState=unitBindings[slot].mech;
        if (!activeMechState) continue;
        if (activeUnitControl->sideIndex>=2) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: activeUnitControl->sideIndex>=2");
        activeSideRule=&sideObjectiveRules[activeUnitControl->sideIndex];
        activeUnitPositionedObject=unitBindings[slot].positionedLeftLeg;
        if (activeMechState->templateIndex>=COMBAT_MECH_TEMPLATES) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: activeMechState->templateIndex>=COMBAT_MECH_TEMPLATES");
        activeMechTemplate=&mechTemplates[activeMechState->templateIndex];
        activeUnitTorsoObject=unitBindings[slot].torso;
        if (activeUnitControl->combatValueOrBoundaryCode<-1) continue;
        if (!activeUnitPositionedObject) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !activeUnitPositionedObject");
        if (activeUnitControl->movementMode==MOVEMENT_MODE_JUMP)
        {
            if (activeUnitControl->combatValueOrBoundaryCode!=COMBAT_MECH_DISABLED_VALUE
                && activeMechState->currentInternalStructure[COMBAT_BODY_RIGHT_LEG]
                && activeMechState->currentInternalStructure[COMBAT_BODY_LEFT_LEG]) goto tacticalUpdate;
            BtechSetActiveUnitGroundOrJumpMotion(activeUnitPositionedObject->object.prefix.orientationC,MOTION_DIRECTION_STOP);
            BtechAdvanceActiveAiUnitMotion();
            goto animate;
        }
        if (!activeMechState->currentInternalStructure[COMBAT_BODY_LEFT_LEG] || !activeMechState->currentInternalStructure[COMBAT_BODY_RIGHT_LEG])
        {
            activeUnitControl->animationRequestFlags=ANIMATION_REQUEST_FALL_POSE;
            activeUnitControl->biasedHeadingSnapshot=activeUnitPositionedObject->object.prefix.orientationC+AVOIDANCE_HALF_TURN;
        }
tacticalUpdate:
        if (activeUnitControl->combatValueOrBoundaryCode!=COMBAT_MECH_DISABLED_VALUE)
        {
            uint16_t mode=activeUnitControl->movementMode;
            if (mode!=MOVEMENT_MODE_FALLEN_POSE && mode!=MOVEMENT_MODE_FALLING && mode!=MOVEMENT_MODE_RECOVERING)
            {
                BtechUpdateTacticalNavigation();
                if ((int16_t)activeMechState->gyroHits<COMBAT_GYRO_DESTROYED_HITS
                    && activeMechState->actuatorCondition[AI_LEFT_LEG_ACTUATOR] && activeMechState->actuatorCondition[AI_RIGHT_LEG_ACTUATOR])
                {
                    BtechAdvanceActiveAiUnitMotion();
                    if ((int16_t)activeMechState->sensorHits<COMBAT_SENSORS_DESTROYED_HITS) BtechChooseAiTargetAndFire();
                }
            }
            else if (mode==MOVEMENT_MODE_FALLEN_POSE)
            {
                if (activeMechState->currentInternalStructure[COMBAT_BODY_LEFT_LEG] && activeMechState->currentInternalStructure[COMBAT_BODY_RIGHT_LEG]
                    && activeMechState->actuatorCondition[AI_LEFT_LEG_ACTUATOR] && activeMechState->actuatorCondition[AI_RIGHT_LEG_ACTUATOR]
                    && !((uint8_t)combatTickCounter&AI_RECOVERY_TICK_MASK))
                {
                    if (activeUnitControl->unitSlot>=COMBAT_MECH_SLOTS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: activeUnitControl->unitSlot>=COMBAT_MECH_SLOTS");
                    int16_t roll=(int16_t)BtechRandomBelowBound(pilotingRecoveryBounds[activeUnitControl->unitSlot]);
                    activeUnitControl->animationRequestFlags=roll<AI_RECOVERY_ROLL_LIMIT?ANIMATION_REQUEST_RECOVER:ANIMATION_REQUEST_FAILED_RECOVERY;
                }
            }
        }
animate:
        {
            BtechAdvanceUnitAnimationAndMovementMode();
            BtechSynchronizeMechRenderParts();
        }
    }
    BtechCoordinateSpecialObjectiveUnits();
}

/*0810:739C. Side ZERO's signed rule gates the whole routine. Worklist targets
 * are consumed even when reassignment fails. Formation waypoints use terrain height. */
void BtechCoordinateSpecialObjectiveUnits(void)
{
    if ((int16_t)sideObjectiveRules[0]<=OBJECTIVE_OPPOSE_WITHDRAWAL) return;
    if (!specialTargetObject) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !specialTargetObject");
    BtechPosition3D *objective=&specialTargetObject->object.prefix.position;
    for (uint16_t slot=0;slot<COMBAT_MECH_SLOTS;++slot)
    {
        if (!unitBindings[slot].mech) {unitDistancesToSpecialTarget[slot]=0;continue;}
        if (!unitBindings[slot].positionedLeftLeg) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !unitBindings[slot].positionedLeftLeg");
        unitDistancesToSpecialTarget[slot]=(int32_t)BtechApproximatePlanarDistance(&unitBindings[slot].positionedLeftLeg->object.prefix.position,objective);
    }
    for (uint16_t side=0;side<2;++side)
    {
        if (sideObjectiveRules[side]!=OBJECTIVE_DEFEND_SPECIAL_TARGET) continue;
        bool noTargetsRemaining=false;
        for (uint16_t pass=0;pass<COMBAT_LANCE_SLOTS && !noTargetsRemaining;++pass)
        {
            int32_t nearestTarget=OBJECTIVE_CANDIDATE_DISTANCE_LIMIT;
            uint16_t threat=0,firstTarget=opposingSideFirstUnitSlots[side];
            if (firstTarget>COMBAT_MECH_SLOTS-COMBAT_LANCE_SLOTS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: firstTarget>COMBAT_MECH_SLOTS-COMBAT_LANCE_SLOTS");
            for (uint16_t slot=firstTarget;slot<firstTarget+COMBAT_LANCE_SLOTS;++slot)
                if (unitDistancesToSpecialTarget[slot] && unitControls[slot].combatValueOrBoundaryCode>0 && unitDistancesToSpecialTarget[slot]<nearestTarget)
                {nearestTarget=unitDistancesToSpecialTarget[slot];threat=slot;}
            if (nearestTarget==OBJECTIVE_CANDIDATE_DISTANCE_LIMIT) {noTargetsRemaining=true;continue;}
            int32_t nearestDefender=OBJECTIVE_CANDIDATE_DISTANCE_LIMIT;
            uint16_t defender=0,firstDefender=opposingSideFirstUnitSlots[1-side];
            bool alreadyCovered=false;
            if (firstDefender>COMBAT_MECH_SLOTS-COMBAT_LANCE_SLOTS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: firstDefender>COMBAT_MECH_SLOTS-COMBAT_LANCE_SLOTS");
            int32_t incomingDistance=unitDistancesToSpecialTarget[threat];
            for (uint16_t slot=firstDefender;slot<firstDefender+COMBAT_LANCE_SLOTS;++slot)
            {
                int32_t objectiveDistance=unitDistancesToSpecialTarget[slot],targetDistance=(int32_t)unitDistances[slot][threat];
                BtechUnitControlState *control=&unitControls[slot];
                if (!objectiveDistance || targetDistance<=0 || objectiveDistance>=OBJECTIVE_DEFENDER_RADIUS || control->combatValueOrBoundaryCode<=0 || targetDistance>=nearestDefender) continue;
                if (control->tacticalState!=TACTICAL_DEFENDING)
                {
                    if (control->targetUnitSlot==threat && objectiveDistance<incomingDistance) {alreadyCovered=true;break;}
                }
                else {nearestDefender=targetDistance;defender=slot;}
            }
            if (!alreadyCovered && nearestDefender!=OBJECTIVE_CANDIDATE_DISTANCE_LIMIT)
            {
                int32_t doubledDistance=(uint32_t)unitDistancesToSpecialTarget[defender]<<1;
                if (unitDistancesToSpecialTarget[threat]<doubledDistance)
                {
                    unitControls[defender].targetUnitSlot=threat;unitControls[defender].tacticalState=TACTICAL_CHARGING;
                    unitDistancesToSpecialTarget[defender]=0;
                    BtechSelectActiveUnitContext(defender);
                    if (!unitBindings[threat].positionedLeftLeg || !activeUnitPositionedObject) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !unitBindings[threat].positionedLeftLeg || !activeUnitPositionedObject");
                    BtechPosition3D *target=&unitBindings[threat].positionedLeftLeg->object.prefix.position;
                    BtechPosition3D *root=&activeUnitPositionedObject->object.prefix.position;
                    BtechApplyActiveUnitTacticalSteering(BtechCalculateVectorHeading(target->positionA-root->positionA,target->positionB-root->positionB));
                }
            }
            unitDistancesToSpecialTarget[threat]=0;
        }
    }
    for (uint16_t slot=0;slot<COMBAT_MECH_SLOTS;++slot)
    {
        activeMechState=unitBindings[slot].mech;
        if (!activeMechState || unitControls[slot].tacticalState!=TACTICAL_DEFENDING) continue;
        BtechSelectActiveUnitContext(slot);
        BtechPosition3D destination={0};
        if (activeUnitControl->commandCode==COMBAT_ORDER_ACT_ON_OWN)
        {
            destination.positionA=unitFormationPositions[slot].positionA;
            destination.positionB=unitFormationPositions[slot].positionB;
            destination.positionC=(int16_t)BtechSampleTerrainOrSurfaceHeight(&destination);
        }
        else
        {
            const int32_t *order=activeUnitControl->orderDestinationPosition;
            destination=(BtechPosition3D){(uint32_t)order[0],(uint32_t)order[1],(uint32_t)order[2]};
        }
        BtechNavigateActiveUnitTowardPosition(&destination);
    }
}
