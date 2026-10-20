#ifndef MECHWARRIOR_COMBAT_BALLISTICS_H
#define MECHWARRIOR_COMBAT_BALLISTICS_H
#include "mechwarrior/combat_world.h"
#include "mechwarrior/combat_resources.h"
enum { COMBAT_WEAPON_SMALL_LASER=0,COMBAT_WEAPON_MEDIUM_LASER=1,COMBAT_WEAPON_LARGE_LASER=2,
    COMBAT_WEAPON_MACHINE_GUN=3,COMBAT_WEAPON_PPC=4,COMBAT_WEAPON_AC5=5,
    COMBAT_WEAPON_LRM5=6,COMBAT_WEAPON_SRM2=7,COMBAT_WEAPON_SRM4=8,COMBAT_WEAPON_SRM6=9,
    COMBAT_BEAM_SLOTS=8,COMBAT_BEAM_VECTORS=6,COMBAT_BEAM_RESOURCE_DELTA=15,
    DIRECT_SHOT_RANGE_SHIFT=5,DIRECT_SHOT_RANGE_MULTIPLIER=15,DIRECT_SHOT_ENDPOINT_MARGIN=150,
    DIRECT_HIT_SOUND=5,DIRECT_HIT_SOUND_PRIORITY=5,
    DIRECT_TORSO_HEADING_BIAS=0x7FF8,DIRECT_TORSO_FRONT_LOWER=0x2AA8,DIRECT_TORSO_FRONT_UPPER=0xD548,
    DIRECT_TORSO_LEFT_LIMIT=0x6AA4,DIRECT_TORSO_REAR_LIMIT=0x954C };
extern uint16_t beamHalfWidths[COMBAT_WEAPON_KINDS];
extern int16_t muzzleLateralOffsetsByBodyLocation[COMBAT_BODY_LOCATIONS];
extern uint8_t beamSlotOccupied[COMBAT_BEAM_SLOTS];
extern BtechShapeResource beamResources[COMBAT_BEAM_SLOTS];
extern BtechRenderEntry beamRenderEntries[COMBAT_BEAM_SLOTS];
extern uint16_t beamGeometry[COMBAT_BEAM_SLOTS][COMBAT_BEAM_VECTORS][3];
extern BtechHeapWorldObject beamWorldObjects[COMBAT_BEAM_SLOTS];
extern BtechRenderVariant ppcBeamVariant,laserBeamVariant;
extern BtechBodyReferencePoint fallenBodyReferenceBanks[COMBAT_MECH_TEMPLATES][COMBAT_ARMOR_LOCATIONS];
extern BtechPosition3D bodyReferenceWorldPosition;
void BtechRotateSignedWordPair(int16_t *componentA,int16_t *componentB,uint16_t angle);
BtechPosition3D *BtechGetUnitBodyReferencePosition(uint16_t unitSlot,int16_t referenceIndex);
void BtechBuildDirectionVectorFromOrientation(int16_t direction[3],const uint16_t orientation[3]);
void BtechBuildWeaponBeamGeometry(const BtechPosition3D *origin,const BtechPosition3D *endpoint,int16_t weaponKind);
void BtechUnregisterTransientWorldObjects(void);
uint8_t BtechIsPlayerWeaponMountReady(uint16_t mountIndex);
uint16_t BtechFindNextReadyPlayerWeaponMount(uint16_t currentMount,uint16_t allowWrap);
uint16_t BtechFindPreviousReadyPlayerWeaponMount(uint16_t currentMount,uint16_t allowWrap);
void BtechAdvancePlayerGroupedWeaponSelection(void);
extern uint32_t unitDistances[COMBAT_MECH_SLOTS][COMBAT_MECH_SLOTS];
void BtechRefreshUnitDistanceMatrix(void);
void BtechMapSelectedRenderPartToArmorLocation(uint16_t *armorLocation,uint16_t *unitSlot);
void BtechBuildPlayerDirectShotGeometry(int16_t distance,const BtechWeaponMount *mount,uint8_t impactEffect);
void BtechFirePlayerDirectWeapon(uint16_t mountIndex);
enum { WEAPON_RANGE_WORLD_SCALE=0x1E0,WEAPON_RANGE_BANDS=5,
    WEAPON_RANGE_BOUNDARIES=4,WEAPON_RANGE_BEYOND_LONG=4,ATTACK_ARCS=4,HIT_LOCATION_ROLLS=11,
    ATTACK_ARC_FRONT=0,ATTACK_ARC_REAR=1,ATTACK_ARC_LEFT=2,ATTACK_ARC_RIGHT=3,
    ATTACK_ASPECT_HEADING_BIAS=0x7FF8,FRONT_ATTACK_ARC_LIMIT=0x2AA8,REAR_ATTACK_ARC_LIMIT=0x6AA4,
    AI_PROJECTED_FIRING_HEAT_LIMIT=0x5A0,AI_MOUNT_SCORE_SPREAD=3,
    AI_CANDIDATE_BLOCKER=1,AI_CANDIDATE_WIDE_ANGLE=2,AI_SPECIAL_TARGET_DISTANCE_MARGIN=0x44C,
    AI_NORMAL_LATERAL_ALLOWANCE=-180,AI_WIDE_LATERAL_ALLOWANCE=-1200,AI_PITCH_LIMIT=0xB60,
    AI_AIM_HEADING_BIAS=0x3FFC,AI_FIRING_PAUSE_FRAME=4,AI_SPECIAL_TARGET_AIM_HEIGHT=500,
    TO_HIT_RANGE_BASE_ADJUSTMENT=4,TO_HIT_FIRING_MOVEMENT_HIGH=2,TO_HIT_FIRING_MOVEMENT_LOW=1,
    TO_HIT_TARGET_MOTION_STEP=30,TO_HIT_STATIONARY_TARGET_BONUS=4,
    AI_INCOMING_PROJECTILE_SOUND=8,AI_INCOMING_PROJECTILE_PRIORITY=15,AI_MISS_DIRECTION_PRODUCT_SHIFT=14 };
extern uint16_t aiCandidateTargetSlot,aiCandidateDistance;
extern uint16_t hitLocationRows[ATTACK_ARCS][HIT_LOCATION_ROLLS],rangeHitModifiers[WEAPON_RANGE_BANDS];
int16_t BtechClassifyWeaponRange(uint16_t firingSlot,uint16_t targetSlot,uint16_t weaponKind);
int16_t BtechTestAiFiringCandidate(const BtechPosition3D *position,uint32_t distance,uint16_t flags);
int16_t BtechSelectAiWeaponMountForDistance(int16_t distance);
void BtechFireAiWeaponAtUnit(int16_t targetSlot,int16_t weaponKind,uint16_t bodyLocation);
void BtechChooseAiTargetAndFire(void);
#endif
