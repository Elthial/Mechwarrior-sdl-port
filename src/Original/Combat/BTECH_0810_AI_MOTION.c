#include "mechwarrior/port_diagnostics.h"
#include "mechwarrior/combat_movement.h"
#include "mechwarrior/combat_pose.h"
#include "mechwarrior/combat_surface.h"
#include "mechwarrior/combat_render.h"
#include "mechwarrior/combat_import.h"
#include <stdlib.h>

/*0810:C878. Signed wrapped difference;8000 becomes8001 before native abs.
 * F04E is pure WORD abs; its duplicate call has no observable side effects. */
int16_t BtechClampSignedDifferenceStep(uint16_t desiredHeading,uint16_t currentHeading,int16_t maximumStep)
{
    int16_t headingError=desiredHeading-currentHeading;
    if ((uint16_t)headingError==0x8000) headingError=(int16_t)0x8001;
    int16_t headingErrorMagnitude=headingError<0?-headingError:headingError;
    if (headingErrorMagnitude>maximumStep) headingErrorMagnitude=maximumStep;
    return (int16_t)(headingError<0?-headingErrorMagnitude:headingErrorMagnitude);
}

/*0810:84EA. Ground motion only in idle/walk sequences; jump delegates85BA.
 * Unlike099A, only POSITIVE acceleration is clamped; reverse uses SAR, not DIV. */
void BtechSetActiveUnitGroundOrJumpMotion(uint16_t desiredHeading,uint16_t movementDirection)
{
    if (!activeUnitControl) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !activeUnitControl");
    if (activeUnitControl->movementMode==MOVEMENT_MODE_JUMP)
    {BtechApplyJumpMotion(desiredHeading,movementDirection);return;}
    if (activeUnitControl->animationSequence!=ANIMATION_SEQUENCE_IDLE && activeUnitControl->animationSequence!=ANIMATION_SEQUENCE_WALK) return;
    if (!activeUnitPositionedObject) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !activeUnitPositionedObject");
    int16_t currentForwardStep=activeUnitPositionedObject->forwardStepScale,requestedForwardStep=0;
    if (movementDirection!=MOTION_DIRECTION_STOP)
    {
        if (!activeMechState || !activeMechTemplate || activeUnitControl->unitSlot>=COMBAT_MECH_SLOTS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !activeMechState || !activeMechTemplate || activeUnitControl->unitSlot>=COMBAT_MECH_SLOTS");
        int16_t heatLimitedForwardStep=activeMechTemplate->maximumSpeedScale-heatSpeedPenalties[activeUnitControl->unitSlot];
        int16_t limitingLegActuatorCondition=activeMechState->actuatorCondition[COMBAT_ACTUATOR_LEFT_LEG];
        if (limitingLegActuatorCondition>(int16_t)activeMechState->actuatorCondition[COMBAT_ACTUATOR_RIGHT_LEG]) limitingLegActuatorCondition=(int16_t)activeMechState->actuatorCondition[COMBAT_ACTUATOR_RIGHT_LEG];
        uint16_t actuatorSpeedProductLow=(uint16_t)((int32_t)heatLimitedForwardStep*limitingLegActuatorCondition),speedProductSignMask=(int16_t)actuatorSpeedProductLow<0?UINT16_MAX:0;
        uint16_t absoluteSpeedProduct=(actuatorSpeedProductLow^speedProductSignMask)-speedProductSignMask;
        uint16_t actuatorLimitedStep=(absoluteSpeedProduct>>COMBAT_ACTUATOR_SPEED_SHIFT)|((absoluteSpeedProduct&0x8000)?0xC000:0);
        requestedForwardStep=(actuatorLimitedStep^speedProductSignMask)-speedProductSignMask;
        if (movementDirection==MOTION_DIRECTION_REVERSE_HALF)
        {
            uint16_t reverseHalfSpeed=((uint16_t)requestedForwardStep>>1)|(requestedForwardStep<0?0x8000:0);
            requestedForwardStep=0-reverseHalfSpeed;
        }
    }
    int16_t forwardAcceleration=requestedForwardStep-currentForwardStep;
    if (forwardAcceleration>AI_GROUND_ACCELERATION_MAXIMUM) forwardAcceleration=AI_GROUND_ACCELERATION_MAXIMUM;
    activeUnitPositionedObject->forwardStepScale=currentForwardStep+forwardAcceleration;
    activeUnitPositionedObject->orientationSteps[2]=BtechClampSignedDifferenceStep(
        desiredHeading,activeUnitPositionedObject->object.prefix.orientationC,AI_GROUND_TURN_LIMIT);
}

/*0810:85BA. Jump controls use a saved chassis reference and shared5416 height.
 * The saved native jump subrecord begins at template+08: +02 is acceleration,
 * +04 is speed limit. Do not confuse them with displayScale/ground speed. */
void BtechApplyJumpMotion(uint16_t desiredHeading,uint16_t movementDirection)
{
    if (!activeMechTemplate || !activeUnitPositionedObject || !activeUnitControl || !activeMechState) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !activeMechTemplate || !activeUnitPositionedObject || !activeUnitControl || !activeMechState");
    BtechMechTemplate *jumpMechTemplate=activeMechTemplate;
    BtechPooledWorldObject *jumpingMechObject=activeUnitPositionedObject;
    int16_t headingError=desiredHeading-jumpingMechObject->object.prefix.orientationC;
    int16_t headingStepMagnitude=(uint16_t)headingError==0x8000?INT16_MAX:headingError<0?-headingError:headingError;
    if (headingStepMagnitude>AI_JUMP_TURN_LIMIT) headingStepMagnitude=AI_JUMP_TURN_LIMIT;
    jumpingMechObject->orientationSteps[2]=(int16_t)(headingError>0?headingStepMagnitude:-headingStepMagnitude);
    uint16_t chassisHalfHeight=(activeMechTemplate->displayScale>>1)|((activeMechTemplate->displayScale&0x8000)?0x8000:0);
    uint32_t feetAltitude=jumpingMechObject->object.prefix.position.positionC-(uint32_t)(int16_t)chassisHalfHeight;
    uint32_t scaledFeetAltitude=BtechShiftSignedDwordRight(feetAltitude,AI_JUMP_CHARGE_HEIGHT_SHIFT);
    uint32_t altitudeChargeRequirement=BtechDivideSignedDword(scaledFeetAltitude,(int16_t)jumpMechTemplate->jumpAcceleration);
    int16_t requiredJumpCharge=(uint16_t)altitudeChargeRequirement+AI_JUMP_CHARGE_RESERVE;
    uint8_t jumpCapabilityLost=activeUnitControl->combatValueOrBoundaryCode==COMBAT_MECH_DISABLED_VALUE
        || !activeMechState->currentInternalStructure[COMBAT_BODY_RIGHT_LEG]
        || !activeMechState->currentInternalStructure[COMBAT_BODY_LEFT_LEG]
        || activeMechTemplate->jumpJetGroupCount==activeMechState->destroyedJumpJetGroups;
    if (movementDirection==MOTION_DIRECTION_STOP || (int16_t)activeMechState->jumpCharge<requiredJumpCharge || jumpCapabilityLost)
    {
        uint32_t savedMechAltitude=jumpingMechObject->object.prefix.position.positionC;
        uint16_t landingSurfaceClass=BtechResolveGroundSurfaceAndSlope(jumpingMechObject,0,selectedWorldObjects,selectedObjectCount);
        jumpingMechObject=activeUnitPositionedObject;
        jumpGroundHeight=jumpingMechObject->object.prefix.position.positionC;
        jumpingMechObject->object.prefix.position.positionC=savedMechAltitude;
        int16_t descentStepThreshold;
        if (landingSurfaceClass==SURFACE_BLOCKS_GROUND_MOVEMENT)
        {
            descentStepThreshold=0;
            int16_t nextForwardStep=jumpingMechObject->forwardStepScale+AI_JUMP_FORWARD_STEP;
            if ((int16_t)jumpMechTemplate->jumpSpeedLimit>nextForwardStep)
            {jumpingMechObject->forwardStepScale=nextForwardStep;activeMechState->jumpCharge-=AI_JUMP_CHARGE_COST;}
        }
        else
        {
            descentStepThreshold=(int16_t)(0-(uint16_t)(jumpMechTemplate->jumpSpeedLimit<<AI_JUMP_DESCENT_SPEED_SHIFT));
            jumpingMechObject->forwardStepScale=0;
        }
        if (jumpingMechObject->positionSteps[2]<descentStepThreshold)
            jumpingMechObject->positionSteps[2]+=jumpMechTemplate->jumpAcceleration;
    }
    else
    {
        jumpingMechObject->orientationSteps[1]=0;jumpingMechObject->orientationSteps[0]=0;
        if ((int32_t)feetAltitude>0)
        {
            int16_t nextForwardStep=jumpingMechObject->forwardStepScale+AI_JUMP_FORWARD_STEP;
            if ((int16_t)jumpMechTemplate->jumpSpeedLimit>nextForwardStep)
            {
                jumpingMechObject->forwardStepScale=nextForwardStep;activeMechState->jumpCharge-=AI_JUMP_CHARGE_COST;
                activeMechState->heatUnits+=AI_JUMP_FORWARD_HEAT;
            }
        }
        if ((int32_t)feetAltitude<AI_JUMP_CEILING)
        {
            int16_t nextVerticalStep=jumpingMechObject->positionSteps[2]+jumpMechTemplate->jumpAcceleration;
            if ((int16_t)jumpMechTemplate->jumpSpeedLimit>nextVerticalStep)
            {
                jumpingMechObject->positionSteps[2]=nextVerticalStep;activeMechState->jumpCharge-=AI_JUMP_CHARGE_COST;
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
    if (!activeUnitPositionedObject || !activeUnitControl || !activeMechTemplate || !activeMechState) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !activeUnitPositionedObject || !activeUnitControl || !activeMechTemplate || !activeMechState");
    BtechPooledWorldObject *movingMechObject=activeUnitPositionedObject;
    BtechPosition3D savedPosition=movingMechObject->object.prefix.position;
    uint16_t savedOrientation[]={movingMechObject->object.prefix.orientationA,movingMechObject->object.prefix.orientationB,movingMechObject->object.prefix.orientationC};
    movingMechObject->object.prefix.orientationB=0;movingMechObject->object.prefix.orientationA=0;
    BtechAdvancePooledObjectMotion(activeUnitPositionedObject);
    if (activeUnitControl->movementMode==MOVEMENT_MODE_JUMP)
    {
        uint16_t chassisHalfHeight=(activeMechTemplate->displayScale>>1)|((activeMechTemplate->displayScale&0x8000)?0x8000:0);
        movingMechObject=activeUnitPositionedObject;
        uint32_t feetAltitude=movingMechObject->object.prefix.position.positionC-(uint32_t)(int16_t)chassisHalfHeight;
        uint16_t jumpPitchSpeed=((uint16_t)movingMechObject->forwardStepScale>>2)|(movingMechObject->forwardStepScale<0?0xC000:0);
        movingMechObject->object.prefix.orientationA=(uint16_t)(0-(uint16_t)(jumpPitchSpeed*AI_JUMP_PITCH_SCALE));
        if ((int32_t)feetAltitude<AI_JUMP_LANDING_HEIGHT_LIMIT && (int32_t)feetAltitude<(int32_t)jumpGroundHeight)
        {
            chassisHalfHeight=(activeMechTemplate->displayScale>>1)|((activeMechTemplate->displayScale&0x8000)?0x8000:0);
            movingMechObject=activeUnitPositionedObject;
            movingMechObject->object.prefix.position.positionC=jumpGroundHeight+(uint32_t)(int16_t)chassisHalfHeight;
            movingMechObject->positionSteps[2]=0;
            if (activeUnitControl->combatValueOrBoundaryCode!=COMBAT_MECH_DISABLED_VALUE
                && activeMechState->currentInternalStructure[COMBAT_BODY_LEFT_LEG]
                && activeMechState->currentInternalStructure[COMBAT_BODY_RIGHT_LEG])
                activeUnitControl->animationRequestFlags|=ANIMATION_REQUEST_LANDING;
            else activeUnitControl->animationRequestFlags|=ANIMATION_REQUEST_FALL_POSE;
        }
        if (activeUnitControl->unitSlot>=COMBAT_MECH_SLOTS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: activeUnitControl->unitSlot>=COMBAT_MECH_SLOTS");
        unitMovementObstructionHistory[activeUnitControl->unitSlot]=0;
        return;
    }
    uint16_t groundSurfaceClass=BtechResolveGroundSurfaceAndSlope(activeUnitPositionedObject,0,selectedWorldObjects,selectedObjectCount);
    movingMechObject=activeUnitPositionedObject;
    if (movingMechObject->forwardStepScale)
    {
        activeUnitControl->animationRequestFlags|=ANIMATION_REQUEST_WALK;
        if (activeUnitControl->unitSlot>=COMBAT_MECH_SLOTS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: activeUnitControl->unitSlot>=COMBAT_MECH_SLOTS");
        uint16_t previousObstruction=unitMovementObstructionHistory[activeUnitControl->unitSlot];
        int16_t movementObstructed=(int8_t)BtechTestUnitMovementObstruction(activeUnitControl->unitSlot);
        if (activeUnitControl->unitSlot>=COMBAT_MECH_SLOTS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: activeUnitControl->unitSlot>=COMBAT_MECH_SLOTS");
        unitMovementObstructionHistory[activeUnitControl->unitSlot]=(uint16_t)movementObstructed;
        if ((unitMovementObstructionHistory[activeUnitControl->unitSlot] && !previousObstruction) || groundSurfaceClass==SURFACE_BLOCKS_GROUND_MOVEMENT)
        {
            unitMovementObstructionHistory[activeUnitControl->unitSlot]=previousObstruction;
            movingMechObject=activeUnitPositionedObject;
            movingMechObject->object.prefix.position=savedPosition;
            movingMechObject->object.prefix.orientationA=savedOrientation[0];movingMechObject->object.prefix.orientationB=savedOrientation[1];
            movingMechObject->object.prefix.orientationC=savedOrientation[2];
            if (movingMechObject->forwardStepScale<=AI_GROUND_ACCELERATION_MAXIMUM)
                movingMechObject->object.prefix.orientationC+=(movingMechObject->object.prefix.position.positionA&1)?AI_BLOCKED_HEADING_STEP:-AI_BLOCKED_HEADING_STEP;
            BtechClearObjectMotionTail(activeUnitPositionedObject);
            activeUnitControl->animationSequence=ANIMATION_SEQUENCE_IDLE;activeUnitControl->animationFrame=0;
            return;
        }
        uint16_t chassisHalfHeight=(activeMechTemplate->displayScale>>1)|((activeMechTemplate->displayScale&0x8000)?0x8000:0);
        activeUnitPositionedObject->object.prefix.position.positionC+=(uint32_t)(int16_t)chassisHalfHeight;
        activeMechState->doubleCoolingFlag=groundSurfaceClass==SURFACE_DOUBLE_COOLING;
    }
    else
    {
        uint16_t chassisHalfHeight=(activeMechTemplate->displayScale>>1)|((activeMechTemplate->displayScale&0x8000)?0x8000:0);
        movingMechObject->object.prefix.position.positionC+=(uint32_t)(int16_t)chassisHalfHeight;
    }
}
