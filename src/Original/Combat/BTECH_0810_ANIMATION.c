#include "mechwarrior/combat_pose.h"
#include "mechwarrior/combat_import.h"
#include "mechwarrior/combat_resources.h"
#include <stdlib.h>

/*0810:A05F. Resource ID is stored before the real extent lookup. The DWORD
 * is numeric, not a selector:offset pair or a newly allocated render object. */
void BtechBindAnimationResourceAndScale(BtechPooledWorldObject *object,uint16_t resourceId)
{
    if (!object) abort();
    object->object.prefix.resourceId=resourceId;
    uint32_t extent=BtechGetResourceScaledValue(resourceId);
    object->object.prefix.resourceScaledValue=extent;
}

/*0810:9C06. Non-player gait/fall/recovery/launch sequence controller. The
 * player's camera pose uses the separate A081 tables and transition rules. */
uint16_t BtechAdvanceUnitAnimationAndMovementMode(void)
{
    if (!activeUnitControl) abort();
    uint8_t requests=activeUnitControl->animationRequestFlags;
    uint16_t requestedSequence=ANIMATION_SEQUENCE_NO_REQUEST;
    if (requests&ANIMATION_REQUEST_FALL_POSE) requestedSequence=ANIMATION_SEQUENCE_FALL;
    else if (requests&ANIMATION_REQUEST_LANDING) requestedSequence=ANIMATION_SEQUENCE_LANDING;
    else if (activeUnitControl->movementMode!=MOVEMENT_MODE_JUMP)
    {
        if (requests&ANIMATION_REQUEST_FAILED_RECOVERY) requestedSequence=ANIMATION_SEQUENCE_FAILED_RECOVERY;
        if (requests&ANIMATION_REQUEST_RECOVER) requestedSequence=ANIMATION_SEQUENCE_RECOVER;
        else if (requests&ANIMATION_REQUEST_BEGIN_JUMP) requestedSequence=ANIMATION_SEQUENCE_BEGIN_JUMP;
        else if (requests&ANIMATION_REQUEST_UNKNOWN03) requestedSequence=ANIMATION_SEQUENCE_UNKNOWN03;
        else if (requests&ANIMATION_REQUEST_FIRING_PAUSE) requestedSequence=ANIMATION_SEQUENCE_FIRING_PAUSE;
        else if (requests&ANIMATION_REQUEST_WALK) requestedSequence=ANIMATION_SEQUENCE_WALK;
    }
    uint16_t sequence=activeUnitControl->animationSequence;
    uint16_t frame=activeUnitControl->animationFrame;
    if (sequence>=UNIT_ANIMATION_SEQUENCES) abort(); /* Native escapes authored table. */
    uint8_t sequenceBoundary=(sequence==ANIMATION_SEQUENCE_WALK && (frame==UNIT_WALK_FIRST_TRANSITION_FRAME || frame==UNIT_WALK_SECOND_TRANSITION_FRAME))
        || frame==(uint16_t)(animationLogicalFrameCounts[sequence]-1);
    int16_t selectedSequence=-1;
    switch (requestedSequence)
    {
    case ANIMATION_SEQUENCE_WALK:
        if (!activeUnitPositionedObject) abort();
        if (sequence==ANIMATION_SEQUENCE_IDLE || (sequence==ANIMATION_SEQUENCE_WALK
            && ((frame==(uint16_t)(animationLogicalFrameCounts[ANIMATION_SEQUENCE_WALK]-1)
                && activeUnitPositionedObject->forwardStepScale>0)
                || (activeUnitPositionedObject->forwardStepScale<0 && !frame))))
            selectedSequence=ANIMATION_SEQUENCE_WALK;
        break;
    case ANIMATION_SEQUENCE_BEGIN_JUMP:
        if (sequence==ANIMATION_SEQUENCE_IDLE)
        {
            selectedSequence=ANIMATION_SEQUENCE_BEGIN_JUMP;
            BtechClearObjectMotionTail(activeUnitPositionedObject);
            if (!activeUnitTorsoObject) abort();
            activeUnitTorsoObject->object.prefix.orientationC=0;
        }
        else if (sequenceBoundary) selectedSequence=ANIMATION_SEQUENCE_IDLE;
        break;
    case ANIMATION_SEQUENCE_UNKNOWN03: break; /* Authored no-op request. */
    case ANIMATION_SEQUENCE_FALL:
        if (sequence!=ANIMATION_SEQUENCE_FALL) selectedSequence=ANIMATION_SEQUENCE_FALL;
        break;
    case ANIMATION_SEQUENCE_RECOVER:
        if (sequence==ANIMATION_SEQUENCE_FALL && sequenceBoundary) selectedSequence=ANIMATION_SEQUENCE_RECOVER;
        break;
    case ANIMATION_SEQUENCE_LANDING:
        if (sequence==ANIMATION_SEQUENCE_IDLE) selectedSequence=ANIMATION_SEQUENCE_LANDING;
        break;
    case ANIMATION_SEQUENCE_FAILED_RECOVERY:
        if (activeUnitControl->movementMode==MOVEMENT_MODE_FALLEN_POSE)
            selectedSequence=ANIMATION_SEQUENCE_FAILED_RECOVERY;
        break;
    case ANIMATION_SEQUENCE_FIRING_PAUSE:
        if (sequence==ANIMATION_SEQUENCE_IDLE && activeUnitControl->movementMode!=MOVEMENT_MODE_JUMP)
        {
            selectedSequence=ANIMATION_SEQUENCE_FIRING_PAUSE;
            BtechClearObjectMotionTail(activeUnitPositionedObject);
        }
        else if (sequence==ANIMATION_SEQUENCE_WALK && frame==UNIT_WALK_SECOND_TRANSITION_FRAME) selectedSequence=ANIMATION_SEQUENCE_IDLE;
        break;
    }
    if (selectedSequence>=0)
    {
        if (!activeUnitPositionedObject) abort();
        activeUnitControl->animationFrame=selectedSequence==ANIMATION_SEQUENCE_WALK
            && activeUnitPositionedObject->forwardStepScale<0
            ? (uint16_t)(animationLogicalFrameCounts[ANIMATION_SEQUENCE_WALK]-1):0;
        activeUnitControl->animationSequence=(uint16_t)selectedSequence;
    }
    else if (activeUnitControl->animationSequence==ANIMATION_SEQUENCE_WALK
        && activeUnitPositionedObject && activeUnitPositionedObject->forwardStepScale<0)
    {
        --activeUnitControl->animationFrame;
        if ((int16_t)activeUnitControl->animationFrame<0)
        {
            activeUnitControl->animationSequence=ANIMATION_SEQUENCE_IDLE;
            activeUnitControl->animationFrame=0;
        }
    }
    else
    {
        ++activeUnitControl->animationFrame;
        sequence=activeUnitControl->animationSequence;
        if (sequence==ANIMATION_SEQUENCE_FALL)
        {
            uint16_t lastFrame=animationLogicalFrameCounts[ANIMATION_SEQUENCE_FALL]-1;
            if (activeUnitControl->animationFrame>lastFrame) activeUnitControl->animationFrame=lastFrame;
        }
        else if (activeUnitControl->animationFrame==animationLogicalFrameCounts[sequence])
        {
            activeUnitControl->animationSequence=ANIMATION_SEQUENCE_IDLE;
            activeUnitControl->animationFrame=0;
        }
    }
    sequence=activeUnitControl->animationSequence;
    frame=activeUnitControl->animationFrame;
    if (!activeMechState || sequence>=UNIT_ANIMATION_SEQUENCES) abort();
    uint16_t chassis=activeMechState->templateIndex;
    const uint8_t *frameMap=animationFrameMaps[sequence];
    if (chassis>=COMBAT_MECH_TEMPLATES || !frameMap
        || frame>=(size_t)(animationFrameMapStorage+UNIT_ANIMATION_MAP_BYTES-frameMap)) abort();
    uint16_t resourceBase=frameMap[frame]+mechResourceBaseIds[chassis];
    uint16_t animationLookupIndex=frame; /*9EAC SI, saved by each0714 below. */
    for (int16_t preceding=0;preceding<(int16_t)animationPrefixSequenceCounts[sequence];++preceding)
    {
        if (preceding>=UNIT_ANIMATION_SEQUENCES) abort();
        animationLookupIndex=(uint16_t)(chassis*UNIT_ANIMATION_SEQUENCES); /*9ED5, even unavailable sequences. */
        if (chassisAnimationAvailability[chassis][preceding])
            resourceBase+=(uint16_t)(animationPartFrameCounts[preceding]*UNIT_ANIMATED_BODY_PARTS);
    }
    uint16_t slot=activeUnitControl->unitSlot;
    if (slot>=COMBAT_MECH_SLOTS) abort();
    BtechUnitBinding *binding=&unitBindings[slot];
    uint16_t partStride=animationPartFrameCounts[activeUnitControl->animationSequence];
    uint16_t partOffset=partStride;
    /* Native reads each binding when its call is reached, not a six-part snapshot. */
    BtechBindAnimationResourceAndScale(binding->leftArm,resourceBase);
    BtechBindAnimationResourceAndScale(binding->rightArm,(uint16_t)(resourceBase+partOffset));
    partOffset+=partStride;
    BtechBindAnimationResourceAndScale(binding->positionedLeftLeg,(uint16_t)(resourceBase+partOffset));
    partOffset+=partStride;
    BtechBindAnimationResourceAndScale(binding->rightLeg,(uint16_t)(resourceBase+partOffset));
    partOffset+=partStride;
    BtechBindAnimationResourceAndScale(binding->torso,(uint16_t)(resourceBase+partOffset));
    partOffset+=partStride;
    BtechBindAnimationResourceAndScale(binding->head,(uint16_t)(resourceBase+partOffset));
    sequence=activeUnitControl->animationSequence;
    frame=activeUnitControl->animationFrame;
    if (sequence==ANIMATION_SEQUENCE_FALL)
        activeUnitControl->movementMode=frame==UNIT_FALL_SETTLED_FRAME?MOVEMENT_MODE_FALLEN_POSE:MOVEMENT_MODE_FALLING;
    else if (sequence==ANIMATION_SEQUENCE_FAILED_RECOVERY && frame==UNIT_FALL_SETTLED_FRAME)
        activeUnitControl->animationSequence=ANIMATION_SEQUENCE_FALL; /* Keep movement mode. */
    else if (sequence==ANIMATION_SEQUENCE_RECOVER)
        activeUnitControl->movementMode=frame==UNIT_RECOVER_STANDING_FRAME?MOVEMENT_MODE_GROUNDED:MOVEMENT_MODE_RECOVERING;
    else if (sequence==ANIMATION_SEQUENCE_BEGIN_JUMP && frame==UNIT_JUMP_LAUNCH_FRAME)
    {
        activeUnitControl->movementMode=MOVEMENT_MODE_JUMP;
        activeMechState->jumpCharge-=JUMP_LAUNCH_CHARGE_COST;
        if (!activeUnitPositionedObject || !activeMechTemplate) abort();
        activeUnitPositionedObject->positionSteps[2]=(int16_t)(activeMechTemplate->jumpSpeedLimit*JUMP_LAUNCH_VERTICAL_SPEED_MULTIPLIER);
        activeUnitPositionedObject->forwardStepScale=(int16_t)activeMechTemplate->jumpSpeedLimit;
    }
    else if (sequence==ANIMATION_SEQUENCE_LANDING)
        activeUnitControl->movementMode=frame==UNIT_LANDING_STANDING_FRAME?MOVEMENT_MODE_GROUNDED:MOVEMENT_MODE_LANDING;
    else if (sequence==ANIMATION_SEQUENCE_IDLE && activeUnitControl->movementMode!=MOVEMENT_MODE_JUMP)
        activeUnitControl->movementMode=MOVEMENT_MODE_GROUNDED;
    activeUnitControl->animationRequestFlags=0;
    return animationLookupIndex;
}
