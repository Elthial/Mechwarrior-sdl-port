#include "mechwarrior/combat_world.h"
#include "mechwarrior/combat_import.h"
#include "mechwarrior/combat_startup.h"
#include "mechwarrior/combat_pose.h"
#include "mechwarrior/combat_sound.h"
#include <stdlib.h>

/*0810:0828. Relative torso twist becomes world heading only during rendering.
 * Keep the torso read after the addition; complete part aliases remain live. */
void BtechApplyBodyHeadingToMechParts(void)
{
    for (uint16_t slot=0;slot<COMBAT_MECH_SLOTS;++slot)
    {
        BtechUnitBinding *binding=&unitBindings[slot];
        if (!binding->mech) continue;
        if (!binding->torso || !binding->positionedLeftLeg) abort();
        binding->torso->object.prefix.orientationC+=binding->positionedLeftLeg->object.prefix.orientationC;
        uint16_t torsoHeading=binding->torso->object.prefix.orientationC;
        if (!binding->rightArm) abort();
        binding->rightArm->object.prefix.orientationC=torsoHeading;
        if (!binding->leftArm) abort();
        binding->leftArm->object.prefix.orientationC=torsoHeading;
        if (!binding->head) abort();
        binding->head->object.prefix.orientationC=torsoHeading;
    }
}

/*0810:0886. Restore relative torso twist and copy it to arms/head in order. */
void BtechRemoveBodyHeadingFromMechParts(void)
{
    for (uint16_t slot=0;slot<COMBAT_MECH_SLOTS;++slot)
    {
        BtechUnitBinding *binding=&unitBindings[slot];
        if (!binding->mech) continue;
        if (!binding->torso || !binding->positionedLeftLeg) abort();
        binding->torso->object.prefix.orientationC-=binding->positionedLeftLeg->object.prefix.orientationC;
        uint16_t torsoHeading=binding->torso->object.prefix.orientationC;
        if (!binding->rightArm) abort();
        binding->rightArm->object.prefix.orientationC=torsoHeading;
        if (!binding->leftArm) abort();
        binding->leftArm->object.prefix.orientationC=torsoHeading;
        if (!binding->head) abort();
        binding->head->object.prefix.orientationC=torsoHeading;
    }
}

/*0810:B383. Seven ordered motion WORD stores, then live player-binding test. */
void BtechClearObjectMotionTail(BtechPooledWorldObject *object)
{
    if (!object) abort();
    object->positionSteps[2]=0;object->positionSteps[1]=0;object->positionSteps[0]=0;
    object->orientationSteps[2]=0;object->orientationSteps[1]=0;object->orientationSteps[0]=0;
    object->forwardStepScale=0;
    if (!playerUnitBinding) abort();
    if (playerUnitBinding->positionedLeftLeg==object)
    {requestedPlayerForwardStepScale=0;unidentifiedField21=0;}
}

/*0810:CBD6. WORD SHL/INC bound and wrapped subtraction, not host rand(). */
uint16_t BtechRandomSignedSpread(int16_t radius)
{
    return BtechRandomBelowBound((uint16_t)((uint16_t)radius*2+1))-(uint16_t)radius;
}

/*0810:A081. Authored camera pose and native gait/fall/recovery/jump state.
 * Pose displacement is rendering motion, not the physics integrator. */
void BtechUpdatePlayerMechPose(const BtechWorldObjectPrefix *source)
{
    if (!combatTickCounter)
    {
        playerPoseSequence=ANIMATION_SEQUENCE_IDLE;
        playerPoseDistanceAccumulator=0;unidentifiedField26=0;playerPoseFrame=0;
        playerPosePerturbationTicks=PLAYER_POSE_PERTURBATION_TICKS;
    }
    if (!playerUnitBinding || !playerUnitBinding->positionedLeftLeg) abort();
    uint16_t forwardStep=playerUnitBinding->positionedLeftLeg->forwardStepScale;
    uint8_t requests=unitControls[0].animationRequestFlags;
    if ((requests&ANIMATION_REQUEST_FALL_POSE) && playerPoseSequence!=ANIMATION_SEQUENCE_FALL)
    {
        BtechClearObjectMotionTail(playerUnitBinding->positionedLeftLeg);
        playerPoseSequence=ANIMATION_SEQUENCE_FALL;goto resetFrame;
    }
    if (requests&ANIMATION_REQUEST_RECOVER)
    {
        if (playerPoseSequence==ANIMATION_SEQUENCE_FALL
            && (uint16_t)(playerPoseFrameCounts[ANIMATION_SEQUENCE_FALL]-1)==playerPoseFrame)
        {playerPoseSequence=ANIMATION_SEQUENCE_RECOVER;goto resetFrame;}
        goto drawPose;
    }
    if (requests&ANIMATION_REQUEST_FAILED_RECOVERY)
    {
        if (playerPoseSequence==ANIMATION_SEQUENCE_FALL
            && (uint16_t)(playerPoseFrameCounts[ANIMATION_SEQUENCE_FALL]-1)==playerPoseFrame)
        {playerPoseSequence=ANIMATION_SEQUENCE_FAILED_RECOVERY;goto resetFrame;}
    }
    else if (requests&ANIMATION_REQUEST_LANDING)
    {
        if (playerPoseSequence==ANIMATION_SEQUENCE_BEGIN_JUMP)
        {
            BtechClearObjectMotionTail(playerUnitBinding->positionedLeftLeg);
            playerPoseSequence=ANIMATION_SEQUENCE_LANDING;goto resetFrame;
        }
    }
    else if (requests&ANIMATION_REQUEST_BEGIN_JUMP)
    {
        if (playerPoseSequence==ANIMATION_SEQUENCE_IDLE || playerPoseSequence==ANIMATION_SEQUENCE_WALK)
        {
            BtechClearObjectMotionTail(playerUnitBinding->positionedLeftLeg);
            playerPoseSequence=ANIMATION_SEQUENCE_BEGIN_JUMP;goto resetFrame;
        }
    }
    else if (forwardStep)
    {
        if (playerPoseSequence==ANIMATION_SEQUENCE_IDLE)
        {playerPoseFrame=0;playerPoseSequence=ANIMATION_SEQUENCE_WALK;}
    }
    else if (playerPoseSequence==ANIMATION_SEQUENCE_WALK)
    {playerPoseSequence=ANIMATION_SEQUENCE_IDLE;goto resetFrame;}
    goto drawPose;
resetFrame:
    playerPoseFrame=0;
drawPose:;
    if (playerPoseSequence>=PLAYER_POSE_SEQUENCES) abort();
    const BtechPoseKeyframe *sequence;
    if (unitControls[0].movementMode==MOVEMENT_MODE_JUMP && playerPoseSequence==ANIMATION_SEQUENCE_BEGIN_JUMP)
    {sequence=playerPoseSequences[ANIMATION_SEQUENCE_IDLE];playerPoseFrame=0;}
    else sequence=playerPoseSequences[playerPoseSequence];
    size_t frame=(size_t)(sequence-playerPoseKeyframes)+(uint16_t)(playerPoseFrame*PLAYER_POSE_KEYFRAME_BYTES)/PLAYER_POSE_KEYFRAME_BYTES;
    if (frame>=PLAYER_POSE_KEYFRAMES || !source || !playerMechTemplate || !playerPoseObject) abort();
    const BtechPoseKeyframe *keyframe=&playerPoseKeyframes[frame];
    uint16_t displacement[3]={(uint16_t)keyframe->displacementA,(uint16_t)keyframe->displacementB,(uint16_t)keyframe->displacementC};
    uint16_t height=playerMechTemplate->displayScale;
    height=(height>>1)|(height&0x8000);
    displacement[2]+=height-PLAYER_POSE_HEIGHT_REFERENCE;
    playerPoseObject->object.prefix.orientationA=source->orientationA;
    playerPoseObject->object.prefix.orientationB=source->orientationB;
    playerPoseObject->object.prefix.orientationC=source->orientationC;
    uint16_t orientation[3]={playerPoseObject->object.prefix.orientationA,
        playerPoseObject->object.prefix.orientationB,playerPoseObject->object.prefix.orientationC};
    BtechOrientationBasis basis;uint16_t rotated[3];
    BtechBuildOrientationMatrix(orientation,&basis,0); /* Incoming CH changes only unused returned CX. */
    BtechTransformWordVectorFar(displacement,&basis,rotated);
    playerPoseObject->object.prefix.position.positionA=source->position.positionA+(uint32_t)(int16_t)rotated[0];
    playerPoseObject->object.prefix.position.positionB=source->position.positionB+(uint32_t)(int16_t)rotated[1];
    playerPoseObject->object.prefix.position.positionC=source->position.positionC+(uint32_t)(int16_t)rotated[2];
    playerPoseObject->object.prefix.orientationA+=keyframe->orientationStepA;
    if (playerImpactThisTick) playerPosePerturbationTicks=0;
    if ((int16_t)playerPosePerturbationTicks<PLAYER_POSE_PERTURBATION_TICKS)
    {
        uint16_t perturbation=BtechRandomSignedSpread(PLAYER_POSE_PERTURBATION_RADIUS);
        playerPoseObject->object.prefix.orientationA+=perturbation;
        perturbation=BtechRandomSignedSpread(PLAYER_POSE_PERTURBATION_RADIUS);
        playerPoseObject->object.prefix.orientationB+=perturbation;
        perturbation=BtechRandomSignedSpread(PLAYER_POSE_PERTURBATION_RADIUS);
        playerPoseObject->object.prefix.orientationC+=perturbation;
        ++playerPosePerturbationTicks;
    }
    if (playerPoseSequence==ANIMATION_SEQUENCE_WALK)
    {
        uint16_t magnitude=playerUnitBinding->positionedLeftLeg->forwardStepScale;
        if (magnitude==0x8000) magnitude=0x7FFF;
        else if ((int16_t)magnitude<0) magnitude=0-magnitude;
        playerPoseDistanceAccumulator+=magnitude;
        uint16_t threshold=playerMechTemplate->maximumSpeedScale;
        if ((int16_t)playerPoseDistanceAccumulator>(int16_t)threshold)
        {
            playerPoseDistanceAccumulator-=threshold;
            ++playerPoseFrame;
            if ((playerPoseFrame&3)==3 && combatDispatchState==COMBAT_SCREEN_COCKPIT)
                BtechPlaySoundIfEnabled(PLAYER_FOOTSTEP_EVENT,1);
        }
    }
    else ++playerPoseFrame;
    if (playerPoseSequence>=PLAYER_POSE_SEQUENCES) abort();
    if ((int16_t)playerPoseFrameCounts[playerPoseSequence]<=(int16_t)playerPoseFrame)
    {
        switch (playerPoseSequence)
        {
        case ANIMATION_SEQUENCE_WALK:
            playerPoseSequence=forwardStep?ANIMATION_SEQUENCE_WALK:ANIMATION_SEQUENCE_IDLE;
            playerPoseFrame=0;break;
        case ANIMATION_SEQUENCE_BEGIN_JUMP:
            unitControls[0].movementMode=MOVEMENT_MODE_JUMP;
            if (!playerMechState) abort();
            playerMechState->jumpCharge-=JUMP_LAUNCH_CHARGE_COST;
            playerUnitBinding->positionedLeftLeg->positionSteps[2]=(int16_t)(playerMechTemplate->jumpSpeedLimit*JUMP_LAUNCH_VERTICAL_SPEED_MULTIPLIER);
            playerPoseFrame=0;
            BtechPlaySoundIfEnabled(PLAYER_JUMP_EVENT,PLAYER_JUMP_SOUND_DELAY);break;
        case ANIMATION_SEQUENCE_FALL:
            unitControls[0].movementMode=MOVEMENT_MODE_FALLEN_POSE;
            --playerPoseFrame;break;
        case ANIMATION_SEQUENCE_FAILED_RECOVERY:
            playerPoseSequence=ANIMATION_SEQUENCE_FALL;playerPoseFrame=PLAYER_FALL_SETTLED_FRAME;break;
        default:
            unitControls[0].movementMode=MOVEMENT_MODE_GROUNDED;
            playerPoseSequence=ANIMATION_SEQUENCE_IDLE;playerPoseFrame=0;break;
        }
    }
    unitControls[0].animationRequestFlags=0;
}

/*0810:9058. Original position/orientation/cache chains, DF clear. Typed
 * positions replace six MOVSWs between whole object owners. Exact part aliases
 * remain live; arbitrary partial-object and binding-overwrite aliases do not. */
void BtechSynchronizeMechRenderParts(void)
{
    if (!activeUnitControl || activeUnitControl->unitSlot>=COMBAT_MECH_SLOTS) abort();
    BtechUnitBinding *binding=&unitBindings[activeUnitControl->unitSlot];
    if (!binding->positionedLeftLeg || !binding->rightLeg) abort();
    binding->rightLeg->object.prefix.position=binding->positionedLeftLeg->object.prefix.position;
    if (!binding->torso) abort(); /* Gate each near-zero alias at its first access, after prior stores. */
    binding->torso->object.prefix.position=binding->rightLeg->object.prefix.position;
    if (!binding->leftArm) abort();
    binding->leftArm->object.prefix.position=binding->positionedLeftLeg->object.prefix.position;
    if (!binding->rightArm) abort();
    binding->rightArm->object.prefix.position=binding->leftArm->object.prefix.position;
    if (!binding->head) abort();
    binding->head->object.prefix.position=binding->positionedLeftLeg->object.prefix.position;
    uint16_t torsoTwist=binding->torso->object.prefix.orientationC;
    /* Each native orientation chain copies A, B, C in that order. */
    binding->rightLeg->object.prefix.orientationA=binding->positionedLeftLeg->object.prefix.orientationA;
    binding->rightLeg->object.prefix.orientationB=binding->positionedLeftLeg->object.prefix.orientationB;
    binding->rightLeg->object.prefix.orientationC=binding->positionedLeftLeg->object.prefix.orientationC;
    binding->torso->object.prefix.orientationA=binding->rightLeg->object.prefix.orientationA;
    binding->torso->object.prefix.orientationB=binding->rightLeg->object.prefix.orientationB;
    binding->torso->object.prefix.orientationC=binding->rightLeg->object.prefix.orientationC;
    binding->leftArm->object.prefix.orientationA=binding->torso->object.prefix.orientationA;
    binding->leftArm->object.prefix.orientationB=binding->torso->object.prefix.orientationB;
    binding->leftArm->object.prefix.orientationC=binding->torso->object.prefix.orientationC;
    binding->rightArm->object.prefix.orientationA=binding->leftArm->object.prefix.orientationA;
    binding->rightArm->object.prefix.orientationB=binding->leftArm->object.prefix.orientationB;
    binding->rightArm->object.prefix.orientationC=binding->leftArm->object.prefix.orientationC;
    binding->head->object.prefix.orientationA=binding->positionedLeftLeg->object.prefix.orientationA;
    binding->head->object.prefix.orientationB=binding->positionedLeftLeg->object.prefix.orientationB;
    binding->head->object.prefix.orientationC=binding->positionedLeftLeg->object.prefix.orientationC;
    binding->leftArm->object.prefix.orientationC=torsoTwist;
    binding->rightArm->object.prefix.orientationC=torsoTwist;
    binding->torso->object.prefix.orientationC=torsoTwist;
    binding->head->object.prefix.orientationC=torsoTwist;
    BtechSurfaceQueryResult surface=binding->positionedLeftLeg->surfaceQuery;
    binding->head->surfaceQuery=surface;
    binding->rightLeg->surfaceQuery=binding->head->surfaceQuery;
    binding->torso->surfaceQuery=binding->rightLeg->surfaceQuery;
    binding->rightArm->surfaceQuery=surface;
    binding->leftArm->surfaceQuery=binding->rightArm->surfaceQuery;
}
