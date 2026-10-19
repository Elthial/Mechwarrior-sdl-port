#ifndef MECHWARRIOR_COMBAT_NAVIGATION_H
#define MECHWARRIOR_COMBAT_NAVIGATION_H
#include "mechwarrior/combat_world.h"
enum { NAVIGATION_ARRIVAL_HALF_WIDTH=160,AVOIDANCE_PROBES=4,AVOIDANCE_TURNS=3,
    AVOIDANCE_DIRECTION_SHIFT=5,AVOIDANCE_HALF_TURN=0x7FF8,AVOIDANCE_CORRIDOR_RANGE=0x800,
    AVOIDANCE_CORRIDOR_ANGLE=0x3FFC,AVOIDANCE_UNIT_HALF_WIDTH=800,AVOIDANCE_SPECIAL_HALF_WIDTH=1500,
    AVOIDANCE_LOOKAHEAD_SHIFT=2,AVOIDANCE_GRID_WINDOW=5,AVOIDANCE_GRID_RADIUS=2,
    NAVIGATION_JUMP_CHARGE_MINIMUM=500,NAVIGATION_JUMP_DISTANCE_MINIMUM=5000,
    NAVIGATION_JUMP_RANDOM_BOUND=25,AVOIDANCE_JUMP_DISTANCE_MINIMUM=2000,
    NAVIGATION_TACTICAL_CHARGING=2,NAVIGATION_TACTICAL_FLEEING=4,NAVIGATION_TACTICAL_CHARGING_BASE=6,
    AVOIDANCE_TRIG_FRACTION_BITS=14,JUMP_TURN_DAMPING=0x80,JUMP_GRAVITY_STEP=3 };
extern int16_t avoidanceStepX,avoidanceStepY;
/* BSS bytes between the clipping selector and Gunnery table, observed only
 * by the original unbounded five-by-five terrain neighbourhood read. */
/* Exact adjacent-owner transport for that read, not a general memory API. */
extern const BtechPosition3D *avoidancePosition;
/* Native5430 references angles at object+14; C retains the owning prefix so
 * its heading remains a named field, not an untyped adjacent-field array. */
extern const BtechWorldObjectPrefix *avoidanceOrientation;
extern uint16_t avoidanceHeadingOffsets[AVOIDANCE_TURNS];
extern int16_t avoidanceProbeSignsX[AVOIDANCE_PROBES],avoidanceProbeSignsY[AVOIDANCE_PROBES];
extern const int16_t *avoidanceProbeComponentsX[AVOIDANCE_PROBES],*avoidanceProbeComponentsY[AVOIDANCE_PROBES];
uint8_t BtechArePlanarPositionsWithinArrivalBounds(const BtechPosition3D *first,const BtechPosition3D *second);
void BtechNavigateActiveUnitTowardPosition(const BtechPosition3D *destination);
void BtechSteerActiveUnitAroundObstacles(uint16_t desiredHeading,uint8_t forward);
uint8_t BtechIsAvoidanceProbeBlocked(uint16_t probe);
uint16_t BtechIsPositionInForwardObstacleCorridor(const BtechPosition3D *position,int16_t halfWidth);
void BtechDampJumpMotion(void);
#endif
