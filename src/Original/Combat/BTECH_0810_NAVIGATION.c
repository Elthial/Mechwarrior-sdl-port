#include "mechwarrior/port_diagnostics.h"
#include "mechwarrior/combat_navigation.h"
#include "mechwarrior/combat_movement.h"
#include "mechwarrior/combat_pose.h"
#include "mechwarrior/combat_import.h"
#include "mechwarrior/combat_render.h"
#include <stdlib.h>

/*0810:7C89. Square planar arrival test, no height. Positive8000 is special;
 * INT32_MIN still wraps negative and passes the native signed comparison. */
uint8_t BtechArePlanarPositionsWithinArrivalBounds(const BtechPosition3D *destinationPosition,const BtechPosition3D *mechPosition)
{
    uint32_t absoluteSeparationX=destinationPosition->positionA-mechPosition->positionA,absoluteSeparationY=destinationPosition->positionB-mechPosition->positionB;
    if (absoluteSeparationX==0x8000) absoluteSeparationX=0x7FFF;else if ((int32_t)absoluteSeparationX<0) absoluteSeparationX=0-absoluteSeparationX;
    if ((int32_t)absoluteSeparationX>=NAVIGATION_ARRIVAL_HALF_WIDTH) return 0;
    if (absoluteSeparationY==0x8000) absoluteSeparationY=0x7FFF;else if ((int32_t)absoluteSeparationY<0) absoluteSeparationY=0-absoluteSeparationY;
    return (int32_t)absoluteSeparationY<NAVIGATION_ARRIVAL_HALF_WIDTH;
}

/*0810:94B0. Copy tracked destination first. Normal typed complete-position
 * owners are supported; arbitrary half-WORD/partial object overlaps are not. */
void BtechNavigateActiveUnitTowardPosition(const BtechPosition3D *destinationPosition)
{
    if (!activeUnitControl || !activeUnitPositionedObject || !destinationPosition) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !activeUnitControl || !activeUnitPositionedObject || !destinationPosition");
    activeUnitControl->trackedPosition[0]=(int32_t)destinationPosition->positionA;
    activeUnitControl->trackedPosition[1]=(int32_t)destinationPosition->positionB;
    activeUnitControl->trackedPosition[2]=(int32_t)destinationPosition->positionC;
    if (BtechArePlanarPositionsWithinArrivalBounds(destinationPosition,&activeUnitPositionedObject->object.prefix.position))
    {BtechSetActiveUnitGroundOrJumpMotion(activeUnitPositionedObject->object.prefix.orientationC,MOTION_DIRECTION_STOP);return;}
    if (!activeMechTemplate || !activeMechState) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !activeMechTemplate || !activeMechState");
    if (activeMechTemplate->jumpJetGroupCount!=activeMechState->destroyedJumpJetGroups
        && activeUnitControl->combatValueOrBoundaryCode>0 && (int16_t)activeMechState->gyroHits<COMBAT_GYRO_DESTROYED_HITS
        && activeUnitControl->movementMode!=MOVEMENT_MODE_JUMP && (int16_t)activeMechState->jumpCharge>NAVIGATION_JUMP_CHARGE_MINIMUM)
    {
        uint32_t distanceToDestination=BtechApproximatePlanarDistance(destinationPosition,&activeUnitPositionedObject->object.prefix.position);
        if ((int32_t)distanceToDestination>NAVIGATION_JUMP_DISTANCE_MINIMUM && !BtechRandomBelowBound(NAVIGATION_JUMP_RANDOM_BOUND))
            activeUnitControl->animationRequestFlags=ANIMATION_REQUEST_BEGIN_JUMP;
    }
    uint32_t destinationDeltaY=destinationPosition->positionB-activeUnitPositionedObject->object.prefix.position.positionB;
    uint32_t destinationDeltaX=destinationPosition->positionA-activeUnitPositionedObject->object.prefix.position.positionA;
    uint16_t destinationHeading=BtechCalculateVectorHeading(destinationDeltaX,destinationDeltaY);
    if (activeUnitControl->movementMode!=MOVEMENT_MODE_JUMP) BtechSteerActiveUnitAroundObstacles(destinationHeading,1);
    else BtechSetActiveUnitGroundOrJumpMotion(destinationHeading,MOTION_DIRECTION_FORWARD);
}

/*0810:95AD. Four authored probes, persisted avoidance side/reverse, then
 * original84EA motion. The jump gate deliberately reads PLAYER heat slot0. */
void BtechSteerActiveUnitAroundObstacles(uint16_t desiredHeading,uint8_t moveForward)
{
    if (!moveForward) desiredHeading+=AVOIDANCE_HALF_TURN;
    uint16_t headingSine=BtechLookupSine(desiredHeading);
    uint16_t scaledHeadingSine=(headingSine>>AVOIDANCE_DIRECTION_SHIFT)|((headingSine&0x8000)?0xF800:0);
    avoidanceStepX=(int16_t)(0-scaledHeadingSine);
    uint16_t headingCosine=BtechLookupCosine(desiredHeading);
    avoidanceStepY=(int16_t)((headingCosine>>AVOIDANCE_DIRECTION_SHIFT)|((headingCosine&0x8000)?0xF800:0));
    if (!BtechIsAvoidanceProbeBlocked(AVOIDANCE_PROBE_FORWARD) && activeUnitControl->avoidanceReverseFlag!=1)
        activeUnitControl->avoidanceTurnIndex=AVOIDANCE_TURN_STRAIGHT;
    else
    {
        if (!activeMechTemplate || !activeMechState) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !activeMechTemplate || !activeMechState");
        if (activeUnitControl->tacticalState!=NAVIGATION_TACTICAL_CHARGING && activeUnitControl->tacticalState!=NAVIGATION_TACTICAL_CHARGING_BASE
            && activeMechTemplate->jumpJetGroupCount && !heatInhibitFlags[0] && activeUnitControl->movementMode!=MOVEMENT_MODE_JUMP
            && (int16_t)activeMechState->jumpCharge>NAVIGATION_JUMP_CHARGE_MINIMUM)
        {
            BtechPosition3D trackedDestination={(uint32_t)activeUnitControl->trackedPosition[0],(uint32_t)activeUnitControl->trackedPosition[1],(uint32_t)activeUnitControl->trackedPosition[2]};
            if ((int32_t)BtechApproximatePlanarDistance(&trackedDestination,&activeUnitPositionedObject->object.prefix.position)>AVOIDANCE_JUMP_DISTANCE_MINIMUM)
                activeUnitControl->animationRequestFlags=ANIMATION_REQUEST_BEGIN_JUMP;
        }
        activeUnitControl->avoidanceReverseFlag=0;
        if (activeUnitControl->avoidanceTurnIndex!=AVOIDANCE_TURN_NEGATIVE && !BtechIsAvoidanceProbeBlocked(AVOIDANCE_PROBE_POSITIVE_TURN)) activeUnitControl->avoidanceTurnIndex=AVOIDANCE_TURN_POSITIVE;
        else if (!BtechIsAvoidanceProbeBlocked(AVOIDANCE_PROBE_NEGATIVE_TURN)) activeUnitControl->avoidanceTurnIndex=AVOIDANCE_TURN_NEGATIVE;
        else if (BtechIsAvoidanceProbeBlocked(AVOIDANCE_PROBE_REVERSE)) activeUnitControl->avoidanceTurnIndex=activeUnitControl->avoidanceTurnIndex!=AVOIDANCE_TURN_POSITIVE?AVOIDANCE_TURN_POSITIVE:AVOIDANCE_TURN_NEGATIVE;
        else activeUnitControl->avoidanceReverseFlag=1;
    }
    uint16_t avoidanceHeadingAdjustment=AVOIDANCE_HALF_TURN;
    if (!activeUnitControl->avoidanceReverseFlag)
    {
        if (activeUnitControl->avoidanceTurnIndex>=AVOIDANCE_TURNS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: activeUnitControl->avoidanceTurnIndex>=AVOIDANCE_TURNS");
        avoidanceHeadingAdjustment=avoidanceHeadingOffsets[activeUnitControl->avoidanceTurnIndex];
    }
    desiredHeading+=avoidanceHeadingAdjustment;
    if (!moveForward) desiredHeading+=AVOIDANCE_HALF_TURN;
    BtechSetActiveUnitGroundOrJumpMotion(desiredHeading,moveForward<1);
}

/*0810:970C. The probes change terrain lookahead, NOT the heading used by the
 * unit/special corridors. WORD displacements wrap before CWD and row indexing. */
uint8_t BtechIsAvoidanceProbeBlocked(uint16_t probeIndex)
{
    if (!activeUnitPositionedObject || !activeUnitControl) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !activeUnitPositionedObject || !activeUnitControl");
    avoidancePosition=&activeUnitPositionedObject->object.prefix.position;
    avoidanceOrientation=&activeUnitPositionedObject->object.prefix;
    if (probeIndex>=AVOIDANCE_PROBES || !avoidanceProbeComponentsX[probeIndex] || !avoidanceProbeComponentsY[probeIndex]) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: probeIndex>=AVOIDANCE_PROBES || !avoidanceProbeComponentsX[probeIndex] || !avoidanceProbeComponentsY[probeIndex]");
    int16_t probeStepX=(int32_t)*avoidanceProbeComponentsX[probeIndex]*avoidanceProbeSignsX[probeIndex];
    int16_t probeStepY=(int32_t)*avoidanceProbeComponentsY[probeIndex]*avoidanceProbeSignsY[probeIndex];
    int16_t probeDisplacementX=probeStepX,probeDisplacementY=probeStepY;
    for (uint16_t otherUnitSlot=0;otherUnitSlot<COMBAT_MECH_SLOTS;++otherUnitSlot)
    {
        if (!unitBindings[otherUnitSlot].mech || activeUnitControl->unitSlot==otherUnitSlot) continue;
        if (!unitBindings[otherUnitSlot].positionedLeftLeg) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !unitBindings[otherUnitSlot].positionedLeftLeg");
        if (BtechIsPositionInForwardObstacleCorridor(&unitBindings[otherUnitSlot].positionedLeftLeg->object.prefix.position,AVOIDANCE_UNIT_HALF_WIDTH)) return 1;
    }
    if (activeUnitControl->sideIndex>=2) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: activeUnitControl->sideIndex>=2");
    int16_t sideObjectiveRule=sideObjectiveRules[activeUnitControl->sideIndex];
    if (sideObjectiveRule>OBJECTIVE_OPPOSE_WITHDRAWAL && (sideObjectiveRule!=OBJECTIVE_SPECIAL_TARGET || !specialTargetInteractionBlocked))
    {
        if (!specialTargetObject) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !specialTargetObject");
        if (BtechIsPositionInForwardObstacleCorridor(&specialTargetObject->object.prefix.position,AVOIDANCE_SPECIAL_HALF_WIDTH)) return 1;
    }
    uint32_t lookaheadX=avoidancePosition->positionA+(uint32_t)(int16_t)((uint16_t)probeDisplacementX<<AVOIDANCE_LOOKAHEAD_SHIFT);
    uint32_t lookaheadY=avoidancePosition->positionB+(uint32_t)(int16_t)((uint16_t)probeDisplacementY<<AVOIDANCE_LOOKAHEAD_SHIFT);
    uint32_t absoluteProbeX=lookaheadX,absoluteProbeY=lookaheadY;
    if (absoluteProbeX==0x8000) absoluteProbeX=0x7FFF;else if ((int32_t)absoluteProbeX<0) absoluteProbeX=0-absoluteProbeX;
    if (absoluteProbeY==0x8000) absoluteProbeY=0x7FFF;else if ((int32_t)absoluteProbeY<0) absoluteProbeY=0-absoluteProbeY;
    if ((int32_t)absoluteProbeX>COMBAT_BOUNDARY_LIMIT_A || (int32_t)absoluteProbeY>COMBAT_BOUNDARY_LIMIT_B)
    {
        uint8_t destinationBeyondSameBoundary=0;
        if (activeUnitControl->trackedUnitOrSpecialSlot<1)
            destinationBeyondSameBoundary=((int32_t)lookaheadX>COMBAT_BOUNDARY_LIMIT_A && activeUnitControl->trackedPosition[0]>COMBAT_BOUNDARY_LIMIT_A)
                || ((int32_t)lookaheadX< -COMBAT_BOUNDARY_LIMIT_A && activeUnitControl->trackedPosition[0]< -COMBAT_BOUNDARY_LIMIT_A)
                || ((int32_t)lookaheadY>COMBAT_BOUNDARY_LIMIT_B && activeUnitControl->trackedPosition[1]>COMBAT_BOUNDARY_LIMIT_B)
                || ((int32_t)lookaheadY< -COMBAT_BOUNDARY_LIMIT_B && activeUnitControl->trackedPosition[1]< -COMBAT_BOUNDARY_LIMIT_B);
        if (activeUnitControl->tacticalState!=NAVIGATION_TACTICAL_FLEEING && !destinationBeyondSameBoundary) return 1;
    }
    for (uint16_t sampleIndex=0;sampleIndex<AVOIDANCE_PROBES;++sampleIndex)
    {
        uint32_t sampleX=avoidancePosition->positionA+(uint32_t)probeDisplacementX;
        uint32_t sampleY=avoidancePosition->positionB+(uint32_t)probeDisplacementY;
        absoluteProbeX=sampleX;absoluteProbeY=sampleY;
        if (absoluteProbeX==0x8000) absoluteProbeX=0x7FFF;else if ((int32_t)absoluteProbeX<0) absoluteProbeX=0-absoluteProbeX;
        if (absoluteProbeY==0x8000) absoluteProbeY=0x7FFF;else if ((int32_t)absoluteProbeY<0) absoluteProbeY=0-absoluteProbeY;
        if ((int32_t)absoluteProbeX<COMBAT_BOUNDARY_LIMIT_A && (int32_t)absoluteProbeY<COMBAT_BOUNDARY_LIMIT_B)
        {
            int32_t firstTerrainRow=(int32_t)BtechDivideSignedDword(sampleX+COMBAT_BOUNDARY_LIMIT_A,TERRAIN_WORLD_CELL_SIZE)-AVOIDANCE_GRID_RADIUS;
            int32_t firstTerrainColumn=(int32_t)BtechDivideSignedDword(COMBAT_BOUNDARY_LIMIT_B-sampleY,TERRAIN_WORLD_CELL_SIZE)-AVOIDANCE_GRID_RADIUS;
            for (uint16_t rowOffset=0;rowOffset<AVOIDANCE_GRID_WINDOW;++rowOffset)
            {
                int32_t gridRow=firstTerrainRow+rowOffset;
                if (gridRow<0 || gridRow>=TERRAIN_GRID_ROWS) continue;
                for (uint16_t columnOffset=0;columnOffset<AVOIDANCE_GRID_WINDOW;++columnOffset)
                {
                    int32_t gridColumn=firstTerrainColumn+columnOffset;
                    if (gridColumn<0 || gridColumn>=TERRAIN_GRID_COLUMNS) continue;
                    if (terrainHeightGrid[gridRow][gridColumn]) return 1;
                }
            }
        }
        probeDisplacementX=(int16_t)(probeDisplacementX+probeStepX);probeDisplacementY=(int16_t)(probeDisplacementY+probeStepY);
    }
    return 0;
}

/*0810:9A83. No height/side/alive filter. All range, heading and lateral tests
 * are strict. Native D91A multiplies signed LOW WORD of approximate distance. */
uint16_t BtechIsPositionInForwardObstacleCorridor(const BtechPosition3D *obstaclePosition,int16_t corridorHalfWidth)
{
    if (!obstaclePosition || !avoidancePosition || !avoidanceOrientation) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !obstaclePosition || !avoidancePosition || !avoidanceOrientation");
    uint32_t obstacleDistance=BtechApproximatePlanarDistance(obstaclePosition,avoidancePosition);
    if ((int32_t)obstacleDistance>=AVOIDANCE_CORRIDOR_RANGE) return 0;
    uint32_t obstacleDeltaY=obstaclePosition->positionB-avoidancePosition->positionB;
    uint32_t obstacleDeltaX=obstaclePosition->positionA-avoidancePosition->positionA;
    uint16_t obstacleBearing=BtechCalculateVectorHeading(obstacleDeltaX,obstacleDeltaY)-avoidanceOrientation->orientationC;
    int16_t bearingMagnitude=obstacleBearing==0x8000?INT16_MAX:(int16_t)obstacleBearing<0?-(int16_t)obstacleBearing:(int16_t)obstacleBearing;
    if (bearingMagnitude>=AVOIDANCE_CORRIDOR_ANGLE) return 0;
    uint32_t lateralOffsetProduct=(uint32_t)BtechMultiplySignedWords((int16_t)obstacleDistance,(int16_t)BtechLookupSine(obstacleBearing));
    int16_t lateralOffset=(int16_t)BtechShiftSignedDwordRight(lateralOffsetProduct,AVOIDANCE_TRIG_FRACTION_BITS);
    int16_t lateralSeparationMagnitude=(uint16_t)lateralOffset==0x8000?INT16_MAX:lateralOffset<0?-lateralOffset:lateralOffset;
    return lateralSeparationMagnitude<corridorHalfWidth;
}

/*0810:9B51. Includes player slot0, but only bound jumping units. */
void BtechDampJumpMotion(void)
{
    for (uint16_t unitSlot=0;unitSlot<COMBAT_MECH_SLOTS;++unitSlot)
    {
        if (!unitBindings[unitSlot].mech) continue;
        BtechPooledWorldObject *jumpingMechObject=unitBindings[unitSlot].positionedLeftLeg;
        if (unitControls[unitSlot].movementMode!=MOVEMENT_MODE_JUMP) continue;
        if (!jumpingMechObject) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !jumpingMechObject");
        int16_t yawTurnStep=jumpingMechObject->orientationSteps[2],yawTurnMagnitude=(uint16_t)yawTurnStep==0x8000?INT16_MAX:yawTurnStep<0?-yawTurnStep:yawTurnStep;
        if (yawTurnMagnitude<JUMP_TURN_DAMPING) jumpingMechObject->orientationSteps[2]=0;
        else if (yawTurnStep<0) jumpingMechObject->orientationSteps[2]=(int16_t)(yawTurnStep+JUMP_TURN_DAMPING);
        else if (yawTurnStep>0) jumpingMechObject->orientationSteps[2]=(int16_t)(yawTurnStep-JUMP_TURN_DAMPING);
        if (jumpingMechObject->forwardStepScale<0) ++jumpingMechObject->forwardStepScale;
        else if (jumpingMechObject->forwardStepScale>0) --jumpingMechObject->forwardStepScale;
        jumpingMechObject->positionSteps[2]=(int16_t)(jumpingMechObject->positionSteps[2]-JUMP_GRAVITY_STEP);
    }
}
