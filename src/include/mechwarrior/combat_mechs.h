#ifndef MECHWARRIOR_COMBAT_MECHS_H
#define MECHWARRIOR_COMBAT_MECHS_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum { COMBAT_MECH_SLOTS=8,COMBAT_LANCE_SLOTS=4,COMBAT_MECH_TEMPLATES=8,
    COMBAT_ARMOR_LOCATIONS=9,COMBAT_BODY_LOCATIONS=8,COMBAT_WEAPON_MOUNTS=10,
    COMBAT_AMMO_FAMILIES=6,COMBAT_ACTUATORS=4,COMBAT_JET_GROUPS=3,
    COMBAT_RANDOM_WORDS=0x20,COMBAT_RANDOM_INDEX_MASK=0x1F,
    COMBAT_JUMP_CHARGE_CAPACITY=0x0320,COMBAT_ACTUATOR_CONDITION_FULL=4,
    COMBAT_RANDOM_WEAR_BOUND=6,COMBAT_RANDOM_WEAR_ROLL=3,
    COMBAT_BODY_LEFT_LEG=2,COMBAT_BODY_RIGHT_LEG=3,COMBAT_BODY_LEFT_TORSO=5,COMBAT_WEAPON_KINDS=10,
    COMBAT_WEAPON_NONE=0xFFFF,COMBAT_AMMO_NONE=0xFFFF,COMBAT_HEAT_THRESHOLDS=5,COMBAT_HEAT_CLAMP_MAXIMUM=0x0960,
    COMBAT_HEAT_SPEED_TIER_STEP=0x0190,COMBAT_HEAT_SPEED_TIER_PENALTY=6,
    COMBAT_HEAT_INHIBIT_THRESHOLD=0x0640,COMBAT_ENGINE_HIT_HEAT=5,
    COMBAT_MOVEMENT_HEAT_REFERENCE_MULTIPLIER=0x48,COMBAT_MOVEMENT_HEAT_REFERENCE_SHIFT=7,
    COMBAT_ARCTIC_LANDSCAPE=2,COMBAT_JUMP_RECHARGE_STEP=2,
    COMBAT_HEAT_UNITS_PER_DISPLAY_POINT=0x50,COMBAT_ACTUATOR_SPEED_SHIFT=2,
    COMBAT_WEAPON_CONDITION_SCORE_SHIFT=1,COMBAT_MECH_DISABLED_VALUE=-1 };

enum { COMBAT_ACTUATOR_LEFT_LEG=2,COMBAT_ACTUATOR_RIGHT_LEG=3 };

typedef struct BtechWeaponDefinition {
    uint16_t damageOrMissileCount;
    int16_t ammoFamily;
    uint16_t reloadTicks,heatUnits,rangeClass,minimumRange;
    uint16_t shortRangeBoundary,mediumRangeBoundary,longRangeBoundary;
    uint16_t criticalSlots,unidentifiedField10;
} BtechWeaponDefinition;

/* Runtime records are ordinary naturally aligned C objects. Native suffixes
 * identify field provenance, NOT sizeof/offsetof contracts for binary IO. */
typedef struct BtechBodyReferencePoint
{ int16_t componentA,componentB,componentC; } BtechBodyReferencePoint;
typedef struct BtechWeaponMount
{ int16_t weaponKind;uint16_t bodyLocation,unidentifiedField4,unidentifiedField6; } BtechWeaponMount;
typedef struct BtechJumpJetGroup
{ int16_t bodyLocation;uint16_t criticalSlots; } BtechJumpJetGroup;
typedef struct BtechMechTemplate
{
    uint16_t tonnage,displayScale,firingMovementThreshold,maximumSpeedScale;
    uint16_t jumpJetGroupCount,jumpAcceleration,jumpSpeedLimit,unidentifiedField8;
    uint16_t heatSinkCount,weaponMountCount;
    BtechBodyReferencePoint bodyReferencePoints[COMBAT_ARMOR_LOCATIONS];
    BtechWeaponMount weaponMounts[COMBAT_WEAPON_MOUNTS];
    int16_t ammoBodyLocations[COMBAT_AMMO_FAMILIES];
    BtechJumpJetGroup jumpJetGroups[COMBAT_JET_GROUPS];
    uint16_t maximumArmor[COMBAT_ARMOR_LOCATIONS];
    uint16_t maximumInternalStructure[COMBAT_BODY_LOCATIONS];
    uint16_t initialAmmoSalvos[COMBAT_AMMO_FAMILIES];
} BtechMechTemplate;
typedef struct BtechWeaponState
{ uint8_t operational;uint16_t reloadTicksRemaining; } BtechWeaponState;
typedef struct BtechMechState
{
    uint16_t templateIndex,aimPitch,movementReferenceScale,heatUnits;
    uint8_t doubleCoolingFlag;
    uint16_t jumpCharge,destroyedHeatSinks,lifeSupportHits,sensorHits;
    uint16_t engineHits,gyroHits,cockpitDisabled,destroyedJumpJetGroups;
    uint16_t currentArmor[COMBAT_ARMOR_LOCATIONS];
    uint16_t currentInternalStructure[COMBAT_BODY_LOCATIONS];
    BtechWeaponState weapons[COMBAT_WEAPON_MOUNTS];
    uint16_t ammoSalvosRemaining[COMBAT_AMMO_FAMILIES];
    uint8_t jumpJetGroupOperational[COMBAT_JET_GROUPS];
    uint16_t actuatorCondition[COMBAT_ACTUATORS];
} BtechMechState;

typedef struct BtechPooledWorldObject BtechPooledWorldObject;
typedef struct BtechUnitBinding
{
    BtechMechState *mech;
    BtechPooledWorldObject *torso,*rightLeg,*positionedLeftLeg;
    BtechPooledWorldObject *rightArm,*leftArm,*head;
} BtechUnitBinding;
typedef struct BtechUnitControlState
{
    uint16_t commandCode;
    int32_t orderDestinationPosition[3];
    int16_t trackedUnitOrSpecialSlot;
    uint16_t sideIndex,unitSlot;
    int16_t targetUnitSlot,combatValueOrBoundaryCode,lastAiFiringTargetSlot,lastDirectHitUnitSlot;
    uint16_t targetSelectionCount;
    int16_t navigationReferenceSlot;
    int32_t trackedPosition[3];
    uint16_t positionKnowledgeCode;
    int32_t lastVisiblePosition[3];
    uint16_t avoidanceReverseFlag,avoidanceTurnIndex;
    uint8_t maneuverFlags,unidentifiedField14;
    uint16_t doubleWeaponRange;
    int16_t weaponRangeMinusMargin,quarterWeaponRange;
    uint16_t controlState,decisionCountdown;
    uint8_t unidentifiedField16[2];
    uint16_t animationSequence,biasedHeadingSnapshot;
    uint8_t animationRequestFlags;
    uint16_t movementMode,animationFrame;
} BtechUnitControlState;

extern BtechMechTemplate mechTemplates[COMBAT_MECH_TEMPLATES];
extern BtechWeaponDefinition weaponDefinitions[COMBAT_WEAPON_KINDS];
extern uint16_t templateMaximumCombatScores[COMBAT_MECH_TEMPLATES];
extern uint8_t unitVisibility[COMBAT_MECH_SLOTS][COMBAT_MECH_SLOTS];
enum { COMBAT_POSITION_UNKNOWN=0,COMBAT_POSITION_REMEMBERED=1,COMBAT_POSITION_VISIBLE=2,
    COMBAT_BOUNDARY_POSITIVE_B=0,COMBAT_BOUNDARY_NEGATIVE_B=1,COMBAT_BOUNDARY_NEGATIVE_A=2,
    COMBAT_BOUNDARY_POSITIVE_A=3,COMBAT_BOUNDARY_INSIDE=4,
    COMBAT_BOUNDARY_LIMIT_A=0xAC80,COMBAT_BOUNDARY_LIMIT_B=0x5DC0,COMBAT_TACTICAL_UNENGAGED=0,
    COMBAT_ORDER_ACT_ON_OWN=0,COMBAT_CONTROL_INITIAL_MANEUVER_FLAG=1,
    COMBAT_GYRO_DESTROYED_HITS=2,COMBAT_SENSORS_DESTROYED_HITS=2 };
void BtechBuildTemplateMaximumCombatScores(void);
uint8_t BtechClassifyUnitWorldBoundary(uint16_t unitSlot);
void BtechRefreshLastVisibleUnitPositions(void);
void BtechRecalculateUnitCombatValuesAndBoundaries(void);
extern BtechMechState mechStates[COMBAT_MECH_SLOTS];
extern BtechUnitBinding unitBindings[COMBAT_MECH_SLOTS];
extern BtechUnitControlState unitControls[COMBAT_MECH_SLOTS];
extern uint16_t heatEffectThresholds[COMBAT_HEAT_THRESHOLDS];
extern uint16_t heatSpeedPenalties[COMBAT_MECH_SLOTS],heatInhibitFlags[COMBAT_MECH_SLOTS];
void BtechAdvanceWeaponReloadTimers(void);
void BtechUpdateUnitHeatEffects(void);
void BtechRechargeJumpCharge(void);
extern uint16_t specialAssaultMode;
extern uint16_t randomStateWords[COMBAT_RANDOM_WORDS],randomWriteIndex,randomAddendIndex;
/* Actual installation IO boundary; no retained304E byte image. */
bool BtechImportMechInitialData(const uint8_t *data,size_t byteCount);
void BtechIncreaseTemplateLegIntegrity(void);
void BtechInitializeMechConditionFromTemplate(BtechMechState *mech,uint16_t templateIndex);
BtechMechState *BtechClaimFreeMechStateSlot(uint16_t friendlySlots);
void BtechClearUnitStateBinding(uint16_t unitSlot);
void BtechMixRandomStateWords(uint16_t seed);
uint16_t BtechRandomBelowBound(uint16_t bound);
#endif
