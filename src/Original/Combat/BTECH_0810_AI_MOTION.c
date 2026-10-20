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
    int16_t previous=activeUnitPositionedObject->forwardStepScale,requested=0;
    if (direction!=MOTION_DIRECTION_STOP)
    {
        if (!activeMechState || !activeMechTemplate || activeUnitControl->unitSlot>=COMBAT_MECH_SLOTS) abort();
        int16_t speed=activeMechTemplate->maximumSpeedScale-heatSpeedPenalties[activeUnitControl->unitSlot];
        int16_t actuator=activeMechState->actuatorCondition[COMBAT_ACTUATOR_LEFT_LEG];
        if (actuator>(int16_t)activeMechState->actuatorCondition[COMBAT_ACTUATOR_RIGHT_LEG]) actuator=(int16_t)activeMechState->actuatorCondition[COMBAT_ACTUATOR_RIGHT_LEG];
        uint16_t product=(uint16_t)((int32_t)speed*actuator),sign=(int16_t)product<0?UINT16_MAX:0;
        uint16_t folded=(product^sign)-sign;
        uint16_t quarter=(folded>>COMBAT_ACTUATOR_SPEED_SHIFT)|((folded&0x8000)?0xC000:0);
        requested=(quarter^sign)-sign;
        if (direction==MOTION_DIRECTION_REVERSE_HALF)
        {
            uint16_t half=((uint16_t)requested>>1)|(requested<0?0x8000:0);
            requested=0-half;
        }
    }
    int16_t acceleration=requested-previous;
    if (acceleration>AI_GROUND_ACCELERATION_MAXIMUM) acceleration=AI_GROUND_ACCELERATION_MAXIMUM;
    activeUnitPositionedObject->forwardStepScale=previous+acceleration;
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
    int16_t error=desiredHeading-motion->object.prefix.orientationC;
    int16_t turn=(uint16_t)error==0x8000?INT16_MAX:error<0?-error:error;
    if (turn>AI_JUMP_TURN_LIMIT) turn=AI_JUMP_TURN_LIMIT;
    motion->orientationSteps[2]=(int16_t)(error>0?turn:-turn);
    uint16_t halfBits=(activeMechTemplate->displayScale>>1)|((activeMechTemplate->displayScale&0x8000)?0x8000:0);
    uint32_t footHeight=motion->object.prefix.position.positionC-(uint32_t)(int16_t)halfBits;
    uint32_t shiftedHeight=BtechShiftSignedDwordRight(footHeight,AI_JUMP_CHARGE_HEIGHT_SHIFT);
    uint32_t quotient=BtechDivideSignedDword(shiftedHeight,(int16_t)chassis->jumpAcceleration);
    int16_t requiredCharge=(uint16_t)quotient+AI_JUMP_CHARGE_RESERVE;
    uint8_t capabilityLost=activeUnitControl->combatValueOrBoundaryCode==COMBAT_MECH_DISABLED_VALUE
        || !activeMechState->currentInternalStructure[COMBAT_BODY_RIGHT_LEG]
        || !activeMechState->currentInternalStructure[COMBAT_BODY_LEFT_LEG]
        || activeMechTemplate->jumpJetGroupCount==activeMechState->destroyedJumpJetGroups;
    if (direction==MOTION_DIRECTION_STOP || (int16_t)activeMechState->jumpCharge<requiredCharge || capabilityLost)
    {
        motion=activeUnitPositionedObject;
        uint32_t height=motion->object.prefix.position.positionC;
        uint16_t surface=BtechResolveGroundSurfaceAndSlope(motion,0,selectedWorldObjects,selectedObjectCount);
        motion=activeUnitPositionedObject;
        jumpGroundHeight=motion->object.prefix.position.positionC;
        motion->object.prefix.position.positionC=height;
        int16_t verticalThreshold;
        if (surface==SURFACE_BLOCKS_GROUND_MOVEMENT)
        {
            verticalThreshold=0;
            int16_t forward=motion->forwardStepScale+AI_JUMP_FORWARD_STEP;
            if ((int16_t)chassis->jumpSpeedLimit>forward)
            {motion->forwardStepScale=forward;activeMechState->jumpCharge-=AI_JUMP_CHARGE_COST;}
        }
        else
        {
            verticalThreshold=(int16_t)(0-(uint16_t)(chassis->jumpSpeedLimit<<AI_JUMP_DESCENT_SPEED_SHIFT));
            motion->forwardStepScale=0;
        }
        if (motion->positionSteps[2]<verticalThreshold)
            motion->positionSteps[2]+=chassis->jumpAcceleration;
    }
    else
    {
        motion=activeUnitPositionedObject;
        motion->orientationSteps[1]=0;motion->orientationSteps[0]=0;
        if ((int32_t)footHeight>0)
        {
            int16_t forward=motion->forwardStepScale+AI_JUMP_FORWARD_STEP;
            if ((int16_t)chassis->jumpSpeedLimit>forward)
            {
                motion->forwardStepScale=forward;activeMechState->jumpCharge-=AI_JUMP_CHARGE_COST;
                activeMechState->heatUnits+=AI_JUMP_FORWARD_HEAT;
            }
        }
        if ((int32_t)footHeight<AI_JUMP_CEILING)
        {
            int16_t vertical=motion->positionSteps[2]+chassis->jumpAcceleration;
            if ((int16_t)chassis->jumpSpeedLimit>vertical)
            {
                motion->positionSteps[2]=vertical;activeMechState->jumpCharge-=AI_JUMP_CHARGE_COST;
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
        uint16_t halfBits=(activeMechTemplate->displayScale>>1)|((activeMechTemplate->displayScale&0x8000)?0x8000:0);
        motion=activeUnitPositionedObject;
        uint32_t footHeight=motion->object.prefix.position.positionC-(uint32_t)(int16_t)halfBits;
        uint16_t quarter=((uint16_t)motion->forwardStepScale>>2)|(motion->forwardStepScale<0?0xC000:0);
        motion->object.prefix.orientationA=(uint16_t)(0-(uint16_t)(quarter*AI_JUMP_PITCH_SCALE));
        if ((int32_t)footHeight<AI_JUMP_LANDING_HEIGHT_LIMIT && (int32_t)footHeight<(int32_t)jumpGroundHeight)
        {
            halfBits=(activeMechTemplate->displayScale>>1)|((activeMechTemplate->displayScale&0x8000)?0x8000:0);
            motion=activeUnitPositionedObject;
            motion->object.prefix.position.positionC=jumpGroundHeight+(uint32_t)(int16_t)halfBits;
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
    uint16_t surface=BtechResolveGroundSurfaceAndSlope(activeUnitPositionedObject,0,selectedWorldObjects,selectedObjectCount);
    motion=activeUnitPositionedObject;
    if (motion->forwardStepScale)
    {
        activeUnitControl->animationRequestFlags|=ANIMATION_REQUEST_WALK;
        if (activeUnitControl->unitSlot>=COMBAT_MECH_SLOTS) abort();
        uint16_t previousBoundary=unitMovementBoundaryHistory[activeUnitControl->unitSlot];
        int16_t boundary=(int8_t)BtechTestUnitMovementBoundary(activeUnitControl->unitSlot);
        if (activeUnitControl->unitSlot>=COMBAT_MECH_SLOTS) abort();
        unitMovementBoundaryHistory[activeUnitControl->unitSlot]=(uint16_t)boundary;
        if ((unitMovementBoundaryHistory[activeUnitControl->unitSlot] && !previousBoundary) || surface==SURFACE_BLOCKS_GROUND_MOVEMENT)
        {
            unitMovementBoundaryHistory[activeUnitControl->unitSlot]=previousBoundary;
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
        uint16_t halfBits=(activeMechTemplate->displayScale>>1)|((activeMechTemplate->displayScale&0x8000)?0x8000:0);
        activeUnitPositionedObject->object.prefix.position.positionC+=(uint32_t)(int16_t)halfBits;
        activeMechState->doubleCoolingFlag=surface==SURFACE_DOUBLE_COOLING;
    }
    else
    {
        uint16_t halfBits=(activeMechTemplate->displayScale>>1)|((activeMechTemplate->displayScale&0x8000)?0x8000:0);
        motion->object.prefix.position.positionC+=(uint32_t)(int16_t)halfBits;
    }
}
