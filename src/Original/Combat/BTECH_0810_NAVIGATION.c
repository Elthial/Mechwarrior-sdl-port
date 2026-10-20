#include "mechwarrior/combat_navigation.h"
#include "mechwarrior/combat_movement.h"
#include "mechwarrior/combat_pose.h"
#include "mechwarrior/combat_import.h"
#include "mechwarrior/combat_render.h"
#include <stdlib.h>

/*0810:7C89. Square planar arrival test, no height. Positive8000 is special;
 * INT32_MIN still wraps negative and passes the native signed comparison. */
uint8_t BtechArePlanarPositionsWithinArrivalBounds(const BtechPosition3D *first,const BtechPosition3D *second)
{
    uint32_t x=first->positionA-second->positionA,y=first->positionB-second->positionB;
    if (x==0x8000) x=0x7FFF;else if ((int32_t)x<0) x=0-x;
    if ((int32_t)x>=NAVIGATION_ARRIVAL_HALF_WIDTH) return 0;
    if (y==0x8000) y=0x7FFF;else if ((int32_t)y<0) y=0-y;
    return (int32_t)y<NAVIGATION_ARRIVAL_HALF_WIDTH;
}

/*0810:94B0. Copy tracked destination first. Normal typed complete-position
 * owners are supported; arbitrary half-WORD/partial object overlaps are not. */
void BtechNavigateActiveUnitTowardPosition(const BtechPosition3D *destination)
{
    if (!activeUnitControl || !activeUnitPositionedObject || !destination) abort();
    activeUnitControl->trackedPosition[0]=(int32_t)destination->positionA;
    activeUnitControl->trackedPosition[1]=(int32_t)destination->positionB;
    activeUnitControl->trackedPosition[2]=(int32_t)destination->positionC;
    if (BtechArePlanarPositionsWithinArrivalBounds(destination,&activeUnitPositionedObject->object.prefix.position))
    {BtechSetActiveUnitGroundOrJumpMotion(activeUnitPositionedObject->object.prefix.orientationC,MOTION_DIRECTION_STOP);return;}
    if (!activeMechTemplate || !activeMechState) abort();
    if (activeMechTemplate->jumpJetGroupCount!=activeMechState->destroyedJumpJetGroups
        && activeUnitControl->combatValueOrBoundaryCode>0 && (int16_t)activeMechState->gyroHits<COMBAT_GYRO_DESTROYED_HITS
        && activeUnitControl->movementMode!=MOVEMENT_MODE_JUMP && (int16_t)activeMechState->jumpCharge>NAVIGATION_JUMP_CHARGE_MINIMUM)
    {
        uint32_t distance=BtechApproximatePlanarDistance(destination,&activeUnitPositionedObject->object.prefix.position);
        if ((int32_t)distance>NAVIGATION_JUMP_DISTANCE_MINIMUM && !BtechRandomBelowBound(NAVIGATION_JUMP_RANDOM_BOUND))
            activeUnitControl->animationRequestFlags=ANIMATION_REQUEST_BEGIN_JUMP;
    }
    uint32_t y=destination->positionB-activeUnitPositionedObject->object.prefix.position.positionB;
    uint32_t x=destination->positionA-activeUnitPositionedObject->object.prefix.position.positionA;
    uint16_t heading=BtechCalculateVectorHeading(x,y);
    if (activeUnitControl->movementMode!=MOVEMENT_MODE_JUMP) BtechSteerActiveUnitAroundObstacles(heading,1);
    else BtechSetActiveUnitGroundOrJumpMotion(heading,MOTION_DIRECTION_FORWARD);
}

/*0810:95AD. Four authored probes, persisted avoidance side/reverse, then
 * original84EA motion. The jump gate deliberately reads PLAYER heat slot0. */
void BtechSteerActiveUnitAroundObstacles(uint16_t desiredHeading,uint8_t forward)
{
    if (!forward) desiredHeading+=AVOIDANCE_HALF_TURN;
    uint16_t sine=BtechLookupSine(desiredHeading);
    uint16_t shifted=(sine>>AVOIDANCE_DIRECTION_SHIFT)|((sine&0x8000)?0xF800:0);
    avoidanceStepX=(int16_t)(0-shifted);
    uint16_t cosine=BtechLookupCosine(desiredHeading);
    avoidanceStepY=(int16_t)((cosine>>AVOIDANCE_DIRECTION_SHIFT)|((cosine&0x8000)?0xF800:0));
    if (!BtechIsAvoidanceProbeBlocked(0) && activeUnitControl->avoidanceReverseFlag!=1)
        activeUnitControl->avoidanceTurnIndex=0;
    else
    {
        if (!activeMechTemplate || !activeMechState) abort();
        if (activeUnitControl->controlState!=NAVIGATION_TACTICAL_CHARGING && activeUnitControl->controlState!=NAVIGATION_TACTICAL_CHARGING_BASE
            && activeMechTemplate->jumpJetGroupCount && !heatInhibitFlags[0] && activeUnitControl->movementMode!=MOVEMENT_MODE_JUMP
            && (int16_t)activeMechState->jumpCharge>NAVIGATION_JUMP_CHARGE_MINIMUM)
        {
            BtechPosition3D destination={(uint32_t)activeUnitControl->trackedPosition[0],(uint32_t)activeUnitControl->trackedPosition[1],(uint32_t)activeUnitControl->trackedPosition[2]};
            if ((int32_t)BtechApproximatePlanarDistance(&destination,&activeUnitPositionedObject->object.prefix.position)>AVOIDANCE_JUMP_DISTANCE_MINIMUM)
                activeUnitControl->animationRequestFlags=ANIMATION_REQUEST_BEGIN_JUMP;
        }
        activeUnitControl->avoidanceReverseFlag=0;
        if (activeUnitControl->avoidanceTurnIndex!=2 && !BtechIsAvoidanceProbeBlocked(1)) activeUnitControl->avoidanceTurnIndex=1;
        else if (!BtechIsAvoidanceProbeBlocked(2)) activeUnitControl->avoidanceTurnIndex=2;
        else if (BtechIsAvoidanceProbeBlocked(3)) activeUnitControl->avoidanceTurnIndex=activeUnitControl->avoidanceTurnIndex!=1?1:2;
        else activeUnitControl->avoidanceReverseFlag=1;
    }
    uint16_t adjustment=AVOIDANCE_HALF_TURN;
    if (!activeUnitControl->avoidanceReverseFlag)
    {
        if (activeUnitControl->avoidanceTurnIndex>=AVOIDANCE_TURNS) abort();
        adjustment=avoidanceHeadingOffsets[activeUnitControl->avoidanceTurnIndex];
    }
    desiredHeading+=adjustment;
    if (!forward) desiredHeading+=AVOIDANCE_HALF_TURN;
    BtechSetActiveUnitGroundOrJumpMotion(desiredHeading,forward<1);
}

/*0810:970C. The probes change terrain lookahead, NOT the heading used by the
 * unit/special corridors. WORD displacements wrap before CWD and row indexing. */
uint8_t BtechIsAvoidanceProbeBlocked(uint16_t probe)
{
    if (!activeUnitPositionedObject || !activeUnitControl) abort();
    avoidancePosition=&activeUnitPositionedObject->object.prefix.position;
    avoidanceOrientation=&activeUnitPositionedObject->object.prefix;
    if (probe>=AVOIDANCE_PROBES || !avoidanceProbeComponentsX[probe] || !avoidanceProbeComponentsY[probe]) abort();
    int16_t stepX=(int32_t)*avoidanceProbeComponentsX[probe]*avoidanceProbeSignsX[probe];
    int16_t stepY=(int32_t)*avoidanceProbeComponentsY[probe]*avoidanceProbeSignsY[probe];
    int16_t displacementX=stepX,displacementY=stepY;
    for (uint16_t slot=0;slot<COMBAT_MECH_SLOTS;++slot)
    {
        if (!unitBindings[slot].mech || activeUnitControl->unitSlot==slot) continue;
        if (!unitBindings[slot].positionedLeftLeg) abort();
        if (BtechIsPositionInForwardObstacleCorridor(&unitBindings[slot].positionedLeftLeg->object.prefix.position,AVOIDANCE_UNIT_HALF_WIDTH)) return 1;
    }
    if (activeUnitControl->sideIndex>=2) abort();
    int16_t rule=sideObjectiveRules[activeUnitControl->sideIndex];
    if (rule>OBJECTIVE_OPPOSE_WITHDRAWAL && (rule!=OBJECTIVE_SPECIAL_TARGET || !specialTargetInteractionBlocked))
    {
        if (!specialTargetObject) abort();
        if (BtechIsPositionInForwardObstacleCorridor(&specialTargetObject->object.prefix.position,AVOIDANCE_SPECIAL_HALF_WIDTH)) return 1;
    }
    uint32_t lookX=avoidancePosition->positionA+(uint32_t)(int16_t)((uint16_t)displacementX<<AVOIDANCE_LOOKAHEAD_SHIFT);
    uint32_t lookY=avoidancePosition->positionB+(uint32_t)(int16_t)((uint16_t)displacementY<<AVOIDANCE_LOOKAHEAD_SHIFT);
    uint32_t magnitudeX=lookX,magnitudeY=lookY;
    if (magnitudeX==0x8000) magnitudeX=0x7FFF;else if ((int32_t)magnitudeX<0) magnitudeX=0-magnitudeX;
    if (magnitudeY==0x8000) magnitudeY=0x7FFF;else if ((int32_t)magnitudeY<0) magnitudeY=0-magnitudeY;
    if ((int32_t)magnitudeX>COMBAT_BOUNDARY_LIMIT_A || (int32_t)magnitudeY>COMBAT_BOUNDARY_LIMIT_B)
    {
        uint8_t sameBoundary=0;
        if (activeUnitControl->trackedUnitOrSpecialSlot<1)
            sameBoundary=((int32_t)lookX>COMBAT_BOUNDARY_LIMIT_A && activeUnitControl->trackedPosition[0]>COMBAT_BOUNDARY_LIMIT_A)
                || ((int32_t)lookX< -COMBAT_BOUNDARY_LIMIT_A && activeUnitControl->trackedPosition[0]< -COMBAT_BOUNDARY_LIMIT_A)
                || ((int32_t)lookY>COMBAT_BOUNDARY_LIMIT_B && activeUnitControl->trackedPosition[1]>COMBAT_BOUNDARY_LIMIT_B)
                || ((int32_t)lookY< -COMBAT_BOUNDARY_LIMIT_B && activeUnitControl->trackedPosition[1]< -COMBAT_BOUNDARY_LIMIT_B);
        if (activeUnitControl->controlState!=NAVIGATION_TACTICAL_FLEEING && !sameBoundary) return 1;
    }
    for (uint16_t sample=0;sample<AVOIDANCE_PROBES;++sample)
    {
        uint32_t x=avoidancePosition->positionA+(uint32_t)displacementX;
        uint32_t y=avoidancePosition->positionB+(uint32_t)displacementY;
        magnitudeX=x;magnitudeY=y;
        if (magnitudeX==0x8000) magnitudeX=0x7FFF;else if ((int32_t)magnitudeX<0) magnitudeX=0-magnitudeX;
        if (magnitudeY==0x8000) magnitudeY=0x7FFF;else if ((int32_t)magnitudeY<0) magnitudeY=0-magnitudeY;
        if ((int32_t)magnitudeX<COMBAT_BOUNDARY_LIMIT_A && (int32_t)magnitudeY<COMBAT_BOUNDARY_LIMIT_B)
        {
            int32_t firstRow=(int32_t)BtechDivideSignedDword(x+COMBAT_BOUNDARY_LIMIT_A,TERRAIN_WORLD_CELL_SIZE)-AVOIDANCE_GRID_RADIUS;
            int32_t firstColumn=(int32_t)BtechDivideSignedDword(COMBAT_BOUNDARY_LIMIT_B-y,TERRAIN_WORLD_CELL_SIZE)-AVOIDANCE_GRID_RADIUS;
            for (uint16_t row=0;row<AVOIDANCE_GRID_WINDOW;++row)
            {
                int32_t gridRow=firstRow+row;
                if (gridRow<0 || gridRow>=TERRAIN_GRID_ROWS) continue;
                for (uint16_t column=0;column<AVOIDANCE_GRID_WINDOW;++column)
                {
                    int32_t gridColumn=firstColumn+column;
                    if (gridColumn<0 || gridColumn>=TERRAIN_GRID_COLUMNS) continue;
                    if (terrainHeightGrid[gridRow][gridColumn]) return 1;
                }
            }
        }
        displacementX=(int16_t)(displacementX+stepX);displacementY=(int16_t)(displacementY+stepY);
    }
    return 0;
}

/*0810:9A83. No height/side/alive filter. All range, heading and lateral tests
 * are strict. Native D91A multiplies signed LOW WORD of approximate distance. */
uint16_t BtechIsPositionInForwardObstacleCorridor(const BtechPosition3D *position,int16_t halfWidth)
{
    if (!position || !avoidancePosition || !avoidanceOrientation) abort();
    uint32_t distance=BtechApproximatePlanarDistance(position,avoidancePosition);
    if ((int32_t)distance>=AVOIDANCE_CORRIDOR_RANGE) return 0;
    uint32_t y=position->positionB-avoidancePosition->positionB;
    uint32_t x=position->positionA-avoidancePosition->positionA;
    uint16_t heading=BtechCalculateVectorHeading(x,y)-avoidanceOrientation->orientationC;
    int16_t magnitude=heading==0x8000?INT16_MAX:(int16_t)heading<0?-(int16_t)heading:(int16_t)heading;
    if (magnitude>=AVOIDANCE_CORRIDOR_ANGLE) return 0;
    uint32_t product=(uint32_t)BtechMultiplySignedWords((int16_t)distance,(int16_t)BtechLookupSine(heading));
    int16_t lateral=(int16_t)BtechShiftSignedDwordRight(product,AVOIDANCE_TRIG_FRACTION_BITS);
    magnitude=(uint16_t)lateral==0x8000?INT16_MAX:lateral<0?-lateral:lateral;
    return magnitude<halfWidth;
}

/*0810:9B51. Includes player slot0, but only bound jumping units. */
void BtechDampJumpMotion(void)
{
    for (uint16_t slot=0;slot<COMBAT_MECH_SLOTS;++slot)
    {
        if (!unitBindings[slot].mech) continue;
        BtechPooledWorldObject *motion=unitBindings[slot].positionedLeftLeg;
        if (unitControls[slot].movementMode!=MOVEMENT_MODE_JUMP) continue;
        if (!motion) abort();
        int16_t turn=motion->orientationSteps[2],magnitude=(uint16_t)turn==0x8000?INT16_MAX:turn<0?-turn:turn;
        if (magnitude<JUMP_TURN_DAMPING) motion->orientationSteps[2]=0;
        else if (turn<0) motion->orientationSteps[2]=(int16_t)(turn+JUMP_TURN_DAMPING);
        else if (turn>0) motion->orientationSteps[2]=(int16_t)(turn-JUMP_TURN_DAMPING);
        if (motion->forwardStepScale<0) ++motion->forwardStepScale;
        else if (motion->forwardStepScale>0) --motion->forwardStepScale;
        motion->positionSteps[2]=(int16_t)(motion->positionSteps[2]-JUMP_GRAVITY_STEP);
    }
}
