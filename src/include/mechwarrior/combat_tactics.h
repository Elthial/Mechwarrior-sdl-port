#ifndef MECHWARRIOR_COMBAT_TACTICS_H
#define MECHWARRIOR_COMBAT_TACTICS_H
#include "mechwarrior/combat_navigation.h"

enum { TACTICAL_AVOIDING=1,TACTICAL_TARGET_VALUE_FLOOR=-100,
    TACTICAL_SELECTION_PENALTY_SHIFT=7,TACTICAL_DISTANCE_SCORE_SHIFT=7,
    TACTICAL_SCORE_BASE=128,TACTICAL_STRONGER_VALUE_SHIFT=5,TACTICAL_WEAKER_VALUE_SHIFT=6,
    TACTICAL_RANGE_SCALE=480,TACTICAL_RANGE_MARGIN=400,TACTICAL_QUARTER_RANGE_SHIFT=2,
    TACTICAL_BOUNDARY_CODES=4,TACTICAL_ORDER_AMBUSH=1,TACTICAL_ORDER_DEFEND=2,
    TACTICAL_ORDER_ATTACK=3,TACTICAL_ORDER_MOVE_ATTACK=4,TACTICAL_ORDER_MOVE_AVOID=5,
    TACTICAL_CHARGING=2,TACTICAL_TRACKING=3,TACTICAL_FLEEING=4,TACTICAL_WANDERING=5,
    TACTICAL_CHARGING_BASE=6,TACTICAL_DEFENDING=7,TACTICAL_RETREATING=8,
    TACTICAL_TORSO_TURN_STEP=0x38E,TACTICAL_TORSO_TURN_LIMIT=0x18E2,
    TACTICAL_OUTER_RANGE=1,TACTICAL_POSITIVE_QUARTER_TURN=2,
    TACTICAL_INNER_HEADING_OFFSET=0x0E38,TACTICAL_OUTER_HEADING_OFFSET=0xF1C8,
    TACTICAL_FLEE_HEADING_OFFSET=0x6AA4,TACTICAL_POSITIVE_QUARTER=0x3FFC,TACTICAL_NEGATIVE_QUARTER=0xC004,
    TACTICAL_THREAT_FACING_LIMIT=0x444,TACTICAL_OBJECTIVE_COMMIT_RANGE=5000,
    TACTICAL_OBJECTIVE_COMMIT_ROLL=10,TACTICAL_RETREAT_X=0xB480,TACTICAL_RETREAT_Y=0x65C0 };
enum { OBJECTIVE_CANDIDATE_DISTANCE_LIMIT=0xFFFF,OBJECTIVE_DEFENDER_RADIUS=0x7080,
    AI_RECOVERY_TICK_MASK=7,AI_RECOVERY_ROLL_LIMIT=2,AI_LEFT_LEG_ACTUATOR=2,AI_RIGHT_LEG_ACTUATOR=3 };
/* These actual numeric cursor values become the reused native heading WORD. */
extern uint8_t boundaryMaskByCode[TACTICAL_BOUNDARY_CODES];
extern const uint16_t *activeSideRule;
extern uint16_t tacticalThreatUnitSlots[COMBAT_MECH_SLOTS],tacticalThreatHeadings[COMBAT_MECH_SLOTS];
extern int32_t unitDistancesToSpecialTarget[COMBAT_MECH_SLOTS];
extern uint16_t opposingSideFirstUnitSlots[2];
void BtechUpdateAiMechUnits(void);
void BtechCoordinateSpecialObjectiveUnits(void);
void BtechUpdateTacticalNavigation(void);
/* Choose engagement tactics from lance orders, visible threats and mission goals. */
void BtechSelectUnitTacticalState(void);
void BtechApplyActiveUnitTacticalSteering(uint16_t desiredHeading);
void BtechSelectActiveUnitContext(uint16_t unitSlot);
uint16_t BtechShouldDisengageForWeaponsOrStructure(void);
uint8_t BtechHasNearbyVisibleThreat(void);
void BtechRefreshTargetAndNavigate(void);
void BtechUpdateUnitWeaponRangeThresholds(void);
void BtechScoreAndSelectTacticalTarget(void);
void BtechUpdateSideCompletionFlags(void);
uint8_t BtechTestSideEliminationOrExitObjective(uint16_t sideIndex);
#endif
