#ifndef MECHWARRIOR_COMBAT_POSE_H
#define MECHWARRIOR_COMBAT_POSE_H
#include "mechwarrior/combat_world.h"
enum { PLAYER_POSE_SEQUENCES=8,PLAYER_POSE_KEYFRAMES=40,PLAYER_POSE_KEYFRAME_BYTES=8,PLAYER_FALL_SETTLED_FRAME=5,
    PLAYER_POSE_HEIGHT_REFERENCE=0x4B,PLAYER_POSE_PERTURBATION_TICKS=5,PLAYER_POSE_PERTURBATION_RADIUS=0xB6,
    PLAYER_FOOTSTEP_EVENT=0x0B,PLAYER_JUMP_EVENT=0x0D,PLAYER_JUMP_SOUND_DELAY=0x1E,JUMP_LAUNCH_CHARGE_COST=0x64,
    ANIMATION_SEQUENCE_IDLE=0,ANIMATION_SEQUENCE_WALK=1,ANIMATION_SEQUENCE_BEGIN_JUMP=2,
    ANIMATION_SEQUENCE_UNKNOWN03=3,ANIMATION_SEQUENCE_FALL=4,ANIMATION_SEQUENCE_RECOVER=5,
    ANIMATION_SEQUENCE_LANDING=6,ANIMATION_SEQUENCE_FAILED_RECOVERY=7,
    ANIMATION_REQUEST_FALL_POSE=1,ANIMATION_REQUEST_RECOVER=2,ANIMATION_REQUEST_WALK=4,
    ANIMATION_REQUEST_BEGIN_JUMP=8,ANIMATION_REQUEST_LANDING=0x20,ANIMATION_REQUEST_FAILED_RECOVERY=0x40,
    ANIMATION_REQUEST_UNKNOWN03=0x10,ANIMATION_REQUEST_FIRING_PAUSE=0x80,
    UNIT_ANIMATION_SEQUENCES=9,UNIT_ANIMATION_MAP_BYTES=0x30,
    ANIMATION_SEQUENCE_FIRING_PAUSE=8,ANIMATION_SEQUENCE_NO_REQUEST=9,
    MOVEMENT_MODE_GROUNDED=0,MOVEMENT_MODE_FALLEN_POSE=1,MOVEMENT_MODE_JUMP=3,
    MOVEMENT_MODE_FALLING=4,MOVEMENT_MODE_RECOVERING=5,MOVEMENT_MODE_LANDING=6,
    UNIT_FALL_SETTLED_FRAME=4,UNIT_RECOVER_STANDING_FRAME=4,
    UNIT_JUMP_LAUNCH_FRAME=5,UNIT_LANDING_STANDING_FRAME=3 };
typedef struct BtechPoseKeyframe {
    int16_t displacementA,displacementB,displacementC;
    uint16_t orientationStepA;
} BtechPoseKeyframe;
extern BtechPoseKeyframe playerPoseKeyframes[PLAYER_POSE_KEYFRAMES];
extern const BtechPoseKeyframe *playerPoseSequences[PLAYER_POSE_SEQUENCES];
extern uint16_t playerPoseFrameCounts[PLAYER_POSE_SEQUENCES];
extern uint16_t playerPoseDistanceAccumulator,playerPosePerturbationTicks;
extern uint16_t playerPoseSequence,playerPoseFrame,unidentifiedField26;
extern uint8_t animationFrameMapStorage[UNIT_ANIMATION_MAP_BYTES];
extern const uint8_t *animationFrameMaps[UNIT_ANIMATION_SEQUENCES];
extern uint8_t animationLogicalFrameCounts[UNIT_ANIMATION_SEQUENCES];
extern uint8_t animationPartFrameCounts[UNIT_ANIMATION_SEQUENCES];
extern uint16_t animationPrefixSequenceCounts[UNIT_ANIMATION_SEQUENCES];
extern uint8_t chassisAnimationAvailability[COMBAT_MECH_TEMPLATES][UNIT_ANIMATION_SEQUENCES];
extern BtechPooledWorldObject *activeUnitPositionedObject,*activeUnitTorsoObject;
extern BtechMechState *activeMechState;
extern BtechMechTemplate *activeMechTemplate;
bool BtechImportPoseInitialData(const uint8_t *data,size_t bytes);
void BtechApplyBodyHeadingToMechParts(void);
void BtechRemoveBodyHeadingFromMechParts(void);
void BtechClearObjectMotionTail(BtechPooledWorldObject *object);
uint16_t BtechRandomSignedSpread(int16_t radius);
void BtechUpdatePlayerMechPose(const BtechWorldObjectPrefix *source);
/* Host-only result transports the0714 saved-SI stack alias later read by739C;
 * native9C06 has no source return contract or new Reko argument. */
uint16_t BtechAdvanceUnitAnimationAndMovementMode(void);
void BtechBindAnimationResourceAndScale(BtechPooledWorldObject *object,uint16_t resourceId);
#endif
