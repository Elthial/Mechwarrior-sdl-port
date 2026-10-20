#include "mechwarrior/combat_movement.h"
#include "mechwarrior/combat_pose.h"
#include "mechwarrior/combat_surface.h"
#include "mechwarrior/combat_render.h"
#include "mechwarrior/combat_import.h"
#include <stdlib.h>

/*0810:C878. Signed wrapped difference;8000 becomes8001 before native abs.
 * F04E is pure WORD abs; its duplicate call has no observable side effects. */
int16_t BtechClampSignedDifferenceStep(uint16_t desired,uint16_t current,int16_t maximumStep)
{
    int16_t difference=desired-current;
    if ((uint16_t)difference==0x8000) difference=(int16_t)0x8001;
    int16_t magnitude=difference<0?-difference:difference;
    if (magnitude>maximumStep) magnitude=maximumStep;
    return (int16_t)(difference<0?-magnitude:magnitude);
}

/*0810:84EA. Ground motion only in idle/walk sequences; jump delegates85BA.
 * Unlike099A, only POSITIVE acceleration is clamped; reverse uses SAR, not DIV. */
void BtechSetActiveUnitGroundOrJumpMotion(uint16_t desiredHeading,uint16_t direction)
{
    if (!activeUnitControl) abort();
    if (activeUnitControl->movementMode==MOVEMENT_MODE_JUMP)
    {BtechApplyJumpMotion(desiredHeading,direction);return;}
    if (activeUnitControl->animationSequence!=ANIMATION_SEQUENCE_IDLE && activeUnitControl->animationSequence!=ANIMATION_SEQUENCE_WALK) return;
    if (!activeUnitPositionedObject) abort();
    int16_t previousForwardStep=activeUnitPositionedObject->forwardStepScale,requestedForwardStep=0;
    if (direction!=MOTION_DIRECTION_STOP)
    {
        if (!activeMechState || !activeMechTemplate || activeUnitControl->unitSlot>=COMBAT_MECH_SLOTS) abort();
        int16_t heatLimitedForwardStep=activeMechTemplate->maximumSpeedScale-heatSpeedPenalties[activeUnitControl->unitSlot];
        int16_t limitingLegActuatorCondition=activeMechState->actuatorCondition[COMBAT_ACTUATOR_LEFT_LEG];
        if (limitingLegActuatorCondition>(int16_t)activeMechState->actuatorCondition[COMBAT_ACTUATOR_RIGHT_LEG]) limitingLegActuatorCondition=(int16_t)activeMechState->actuatorCondition[COMBAT_ACTUATOR_RIGHT_LEG];
        uint16_t actuatorScaledStep=(uint16_t)((int32_t)heatLimitedForwardStep*limitingLegActuatorCondition),stepSignMask=(int16_t)actuatorScaledStep<0?UINT16_MAX:0;
        uint16_t absoluteScaledStep=(actuatorScaledStep^stepSignMask)-stepSignMask;
        uint16_t actuatorAdjustedStep=(absoluteScaledStep>>COMBAT_ACTUATOR_SPEED_SHIFT)|((absoluteScaledStep&0x8000)?0xC000:0);
        requestedForwardStep=(actuatorAdjustedStep^stepSignMask)-stepSignMask;
        if (direction==MOTION_DIRECTION_REVERSE_HALF)
        {
            uint16_t halfReverseStep=((uint16_t)requestedForwardStep>>1)|(requestedForwardStep<0?0x8000:0);
            requestedForwardStep=0-halfReverseStep;
        }
    }
    int16_t acceleration=requestedForwardStep-previousForwardStep;
    if (acceleration>AI_GROUND_ACCELERATION_MAXIMUM) acceleration=AI_GROUND_ACCELERATION_MAXIMUM;
    activeUnitPositionedObject->forwardStepScale=previousForwardStep+acceleration;
    activeUnitPositionedObject->orientationSteps[2]=BtechClampSignedDifferenceStep(
        desiredHeading,activeUnitPositionedObject->object.prefix.orientationC,AI_GROUND_TURN_LIMIT);
}

/*0810:85BA. Jump controls use a saved chassis reference and shared5416 height.
 * The saved native jump subrecord begins at template+08: +02 is acceleration,
 * +04 is speed limit. Do not confuse them with displayScale/ground speed. */
void BtechApplyJumpMotion(uint16_t desiredHeading,uint16_t direction)
{
    if (!activeMechTemplate || !activeUnitPositionedObject || !activeUnitControl || !activeMechState) abort();
    BtechMechTemplate *chassis=activeMechTemplate;
    BtechPooledWorldObject *motion=activeUnitPositionedObject;
    int16_t headingError=desiredHeading-motion->object.prefix.orientationC;
    int16_t headingStepMagnitude=(uint16_t)headingError==0x8000?INT16_MAX:headingError<0?-headingError:headingError;
    if (headingStepMagnitude>AI_JUMP_TURN_LIMIT) headingStepMagnitude=AI_JUMP_TURN_LIMIT;
    motion->orientationSteps[2]=(int16_t)(headingError>0?headingStepMagnitude:-headingStepMagnitude);
    uint16_t halfDisplayScale=(activeMechTemplate->displayScale>>1)|((activeMechTemplate->displayScale&0x8000)?0x8000:0);
    uint32_t feetAltitude=motion->object.prefix.position.positionC-(uint32_t)(int16_t)halfDisplayScale;
    uint32_t scaledFeetAltitude=BtechShiftSignedDwordRight(feetAltitude,AI_JUMP_CHARGE_HEIGHT_SHIFT);
    uint32_t altitudeChargeRequirement=BtechDivideSignedDword(scaledFeetAltitude,(int16_t)chassis->jumpAcceleration);
    int16_t requiredJumpCharge=(uint16_t)altitudeChargeRequirement+AI_JUMP_CHARGE_RESERVE;
    uint8_t jumpCapabilityLost=activeUnitControl->combatValueOrBoundaryCode==COMBAT_MECH_DISABLED_VALUE
        || !activeMechState->currentInternalStructure[COMBAT_BODY_RIGHT_LEG]
        || !activeMechState->currentInternalStructure[COMBAT_BODY_LEFT_LEG]
        || activeMechTemplate->jumpJetGroupCount==activeMechState->destroyedJumpJetGroups;
    if (direction==MOTION_DIRECTION_STOP || (int16_t)activeMechState->jumpCharge<requiredJumpCharge || jumpCapabilityLost)
    {
        motion=activeUnitPositionedObject;
        uint32_t savedAltitude=motion->object.prefix.position.positionC;
        uint16_t groundSurface=BtechResolveGroundSurfaceAndSlope(motion,0,selectedWorldObjects,selectedObjectCount);
        motion=activeUnitPositionedObject;
        jumpGroundHeight=motion->object.prefix.position.positionC;
        motion->object.prefix.position.positionC=savedAltitude;
        int16_t descentStepThreshold;
        if (groundSurface==SURFACE_BLOCKS_GROUND_MOVEMENT)
        {
            descentStepThreshold=0;
            int16_t nextForwardStep=motion->forwardStepScale+AI_JUMP_FORWARD_STEP;
            if ((int16_t)chassis->jumpSpeedLimit>nextForwardStep)
            {motion->forwardStepScale=nextForwardStep;activeMechState->jumpCharge-=AI_JUMP_CHARGE_COST;}
        }
        else
        {
            descentStepThreshold=(int16_t)(0-(uint16_t)(chassis->jumpSpeedLimit<<AI_JUMP_DESCENT_SPEED_SHIFT));
            motion->forwardStepScale=0;
        }
        if (motion->positionSteps[2]<descentStepThreshold)
            motion->positionSteps[2]+=chassis->jumpAcceleration;
    }
    else
    {
        motion=activeUnitPositionedObject;
        motion->orientationSteps[1]=0;motion->orientationSteps[0]=0;
        if ((int32_t)feetAltitude>0)
        {
            int16_t nextForwardStep=motion->forwardStepScale+AI_JUMP_FORWARD_STEP;
            if ((int16_t)chassis->jumpSpeedLimit>nextForwardStep)
            {
                motion->forwardStepScale=nextForwardStep;activeMechState->jumpCharge-=AI_JUMP_CHARGE_COST;
                activeMechState->heatUnits+=AI_JUMP_FORWARD_HEAT;
            }
        }
        if ((int32_t)feetAltitude<AI_JUMP_CEILING)
        {
            int16_t nextVerticalStep=motion->positionSteps[2]+chassis->jumpAcceleration;
            if ((int16_t)chassis->jumpSpeedLimit>nextVerticalStep)
            {
                motion->positionSteps[2]=nextVerticalStep;activeMechState->jumpCharge-=AI_JUMP_CHARGE_COST;
                activeMechState->heatUnits+=AI_JUMP_VERTICAL_HEAT;
            }
        }
    }
    if ((int16_t)activeMechState->jumpCharge<0) activeMechState->jumpCharge=0;
}

/*0810:87D0. Snapshot before clearing pitch/roll and integrating. Boundary
 * rejection restores pose but not the surface cache; stopped cooling is stale. */
void BtechAdvanceActiveAiUnitMotion(void)
{
    if (!activeUnitPositionedObject || !activeUnitControl || !activeMechTemplate || !activeMechState) abort();
    BtechPooledWorldObject *motion=activeUnitPositionedObject;
    BtechPosition3D savedPosition=motion->object.prefix.position;
    uint16_t savedOrientation[]={motion->object.prefix.orientationA,motion->object.prefix.orientationB,motion->object.prefix.orientationC};
    motion->object.prefix.orientationB=0;motion->object.prefix.orientationA=0;
    BtechAdvancePooledObjectMotion(activeUnitPositionedObject);
    if (activeUnitControl->movementMode==MOVEMENT_MODE_JUMP)
    {
        uint16_t halfDisplayScale=(activeMechTemplate->displayScale>>1)|((activeMechTemplate->displayScale&0x8000)?0x8000:0);
        motion=activeUnitPositionedObject;
        uint32_t feetAltitude=motion->object.prefix.position.positionC-(uint32_t)(int16_t)halfDisplayScale;
        uint16_t quarterForwardStep=((uint16_t)motion->forwardStepScale>>2)|(motion->forwardStepScale<0?0xC000:0);
        motion->object.prefix.orientationA=(uint16_t)(0-(uint16_t)(quarterForwardStep*AI_JUMP_PITCH_SCALE));
        if ((int32_t)feetAltitude<AI_JUMP_LANDING_HEIGHT_LIMIT && (int32_t)feetAltitude<(int32_t)jumpGroundHeight)
        {
            halfDisplayScale=(activeMechTemplate->displayScale>>1)|((activeMechTemplate->displayScale&0x8000)?0x8000:0);
            motion=activeUnitPositionedObject;
            motion->object.prefix.position.positionC=jumpGroundHeight+(uint32_t)(int16_t)halfDisplayScale;
            motion->positionSteps[2]=0;
            if (activeUnitControl->combatValueOrBoundaryCode!=COMBAT_MECH_DISABLED_VALUE
                && activeMechState->currentInternalStructure[COMBAT_BODY_LEFT_LEG]
                && activeMechState->currentInternalStructure[COMBAT_BODY_RIGHT_LEG])
                activeUnitControl->animationRequestFlags|=ANIMATION_REQUEST_LANDING;
            else activeUnitControl->animationRequestFlags|=ANIMATION_REQUEST_FALL_POSE;
        }
        if (activeUnitControl->unitSlot>=COMBAT_MECH_SLOTS) abort();
        unitMovementBoundaryHistory[activeUnitControl->unitSlot]=0;
        return;
    }
    uint16_t groundSurface=BtechResolveGroundSurfaceAndSlope(activeUnitPositionedObject,0,selectedWorldObjects,selectedObjectCount);
    motion=activeUnitPositionedObject;
    if (motion->forwardStepScale)
    {
        activeUnitControl->animationRequestFlags|=ANIMATION_REQUEST_WALK;
        if (activeUnitControl->unitSlot>=COMBAT_MECH_SLOTS) abort();
        uint16_t previousMovementBoundary=unitMovementBoundaryHistory[activeUnitControl->unitSlot];
        int16_t movementBoundary=(int8_t)BtechTestUnitMovementBoundary(activeUnitControl->unitSlot);
        if (activeUnitControl->unitSlot>=COMBAT_MECH_SLOTS) abort();
        unitMovementBoundaryHistory[activeUnitControl->unitSlot]=(uint16_t)movementBoundary;
        if ((unitMovementBoundaryHistory[activeUnitControl->unitSlot] && !previousMovementBoundary) || groundSurface==SURFACE_BLOCKS_GROUND_MOVEMENT)
        {
            unitMovementBoundaryHistory[activeUnitControl->unitSlot]=previousMovementBoundary;
            motion=activeUnitPositionedObject;
            motion->object.prefix.position=savedPosition;
            motion->object.prefix.orientationA=savedOrientation[0];motion->object.prefix.orientationB=savedOrientation[1];
            motion->object.prefix.orientationC=savedOrientation[2];
            if (motion->forwardStepScale<=AI_GROUND_ACCELERATION_MAXIMUM)
                motion->object.prefix.orientationC+=(motion->object.prefix.position.positionA&1)?AI_BLOCKED_HEADING_STEP:-AI_BLOCKED_HEADING_STEP;
            BtechClearObjectMotionTail(activeUnitPositionedObject);
            activeUnitControl->animationSequence=ANIMATION_SEQUENCE_IDLE;activeUnitControl->animationFrame=0;
            return;
        }
        uint16_t halfDisplayScale=(activeMechTemplate->displayScale>>1)|((activeMechTemplate->displayScale&0x8000)?0x8000:0);
        activeUnitPositionedObject->object.prefix.position.positionC+=(uint32_t)(int16_t)halfDisplayScale;
        activeMechState->doubleCoolingFlag=groundSurface==SURFACE_DOUBLE_COOLING;
    }
    else
    {
        uint16_t halfDisplayScale=(activeMechTemplate->displayScale>>1)|((activeMechTemplate->displayScale&0x8000)?0x8000:0);
        motion->object.prefix.position.positionC+=(uint32_t)(int16_t)halfDisplayScale;
    }
}
