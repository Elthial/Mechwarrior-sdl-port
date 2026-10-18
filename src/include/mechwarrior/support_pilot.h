#ifndef MECHWARRIOR_SUPPORT_PILOT_H
#define MECHWARRIOR_SUPPORT_PILOT_H
/* OPTIONAL HOST AUTOMATION, not reconstructed Dynamix AI or native ABI.
 * Observations are copies; outputs are PC keyboard scans, never game setters. */
#include <stdbool.h>
#include <stdint.h>
enum { PILOT_TARGETS=9,PILOT_WEAPONS=10,PILOT_CHECKPOINTS=16,
    PILOT_SCAN_NONE=0,PILOT_SCAN_FIRE=0x39,PILOT_SCAN_TARGET=0x1C,
    PILOT_SCAN_LEFT=0x4B,PILOT_SCAN_RIGHT=0x4D,PILOT_SCAN_UP=0x48,
    PILOT_SCAN_DOWN=0x50,PILOT_SCAN_STOP=0x4C,PILOT_SCAN_ALIGN=0x1E,
    PILOT_SCAN_AIM_UP=0x32,PILOT_SCAN_AIM_DOWN=0x31,
    PILOT_SCAN_TORSO_LEFT=0x33,PILOT_SCAN_TORSO_RIGHT=0x34,
    PILOT_SCAN_RECOVER=0x16,PILOT_SCAN_AWS=0x11,PILOT_SCAN_SALVO=0x0F };
typedef enum PilotGoal {PILOT_IDLE,PILOT_ENGAGE,PILOT_NAVIGATE} PilotGoal;
typedef enum PilotTactic {PILOT_KEEP_RANGE,PILOT_ORBIT_LEFT,PILOT_ORBIT_RIGHT} PilotTactic;
typedef struct PilotPoint {int32_t x,y,z;} PilotPoint;
typedef struct PilotTarget {
    bool known,visible,hostile,alive,underReticle;
    PilotPoint position;
    int16_t screenX,screenY;
    uint16_t armor,internal;
} PilotTarget;
typedef struct PilotWeapon {bool ready;uint16_t heat,maximumRange;} PilotWeapon;
typedef struct PilotObservation {
    bool cockpit,alive,fallen,heatInhibited,aws,salvo,missionComplete;
    uint16_t tick,heading,torsoHeading,heat,lockedTarget,selectedWeapon;
    int16_t turnRate,speed,requestedSpeed,maximumSpeed,reticleX,reticleY;
    PilotPoint position;
    PilotTarget targets[PILOT_TARGETS];
    PilotWeapon weapons[PILOT_WEAPONS];
} PilotObservation;
typedef struct PilotOrders {
    PilotGoal goal;PilotTactic tactic;
    unsigned targetSlot; /* 0 follows the existing hostile lock, then visible enemies. */
    uint16_t minimumRange,maximumRange,heatCeiling,arrivalRadius;
    unsigned checkpointCount;
    PilotPoint checkpoints[PILOT_CHECKPOINTS];
} PilotOrders;
typedef struct PilotControls {uint8_t directionScan,commandScan;} PilotControls;
typedef struct SupportPilot {
    PilotOrders orders;
    unsigned targetSlot,checkpoint,stalledTicks,unseenTicks,recoveryTicks;
    uint16_t previousTick;
    bool haveObservation,complete;
    PilotPoint previousPosition,lastTargetPosition;
    double range,headingError;
    const char *movementReason,*weaponReason;
    PilotControls controls;
} SupportPilot;
void SupportPilotReset(SupportPilot *pilot,const PilotOrders *orders);
PilotControls SupportPilotUpdate(SupportPilot *pilot,const PilotObservation *observation);
/* Read-only bridge from canonical owners; no native procedures are called. */
void SupportPilotObserve(PilotObservation *observation,bool combatActive);
#endif
