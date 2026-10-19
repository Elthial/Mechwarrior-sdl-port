#include "mechwarrior/combat_world.h"
#include "mechwarrior/combat_import.h"
#include "mechwarrior/combat_startup.h"
#include "mechwarrior/combat_runtime.h"
#include "mechwarrior/combat_damage.h"
#include "mechwarrior/combat_surface.h"
#include "mechwarrior/combat_platform.h"
#include <stdlib.h>

/*0810:5565. Sample the height grid, or the selected world surfaces outside
 * it. Visibility consumes only the signed low WORD of the resulting height. */
uint16_t BtechSampleTerrainOrSurfaceHeight(const BtechPosition3D *position)
{
    uint32_t magnitudeA=position->positionA,magnitudeB=position->positionB;
    if (magnitudeA==0x8000) magnitudeA=0x7FFF;
    else if ((int32_t)magnitudeA<0) magnitudeA=0-magnitudeA;
    if (magnitudeB==0x8000) magnitudeB=0x7FFF;
    else if ((int32_t)magnitudeB<0) magnitudeB=0-magnitudeB;
    if ((int32_t)magnitudeA<COMBAT_BOUNDARY_LIMIT_A && (int32_t)magnitudeB<COMBAT_BOUNDARY_LIMIT_B)
    {
        uint32_t numerator=position->positionA+COMBAT_BOUNDARY_LIMIT_A;
        uint16_t row=(uint16_t)((int32_t)numerator/TERRAIN_WORLD_CELL_SIZE);
        numerator=COMBAT_BOUNDARY_LIMIT_B-position->positionB;
        uint16_t column=(uint16_t)((int32_t)numerator/TERRAIN_WORLD_CELL_SIZE);
        if (row>=TERRAIN_GRID_ROWS || column>=TERRAIN_GRID_COLUMNS) abort(); /* Adjacent-owner wrapping alias. */
        return (uint16_t)(terrainHeightGrid[row][column]<<TERRAIN_HEIGHT_SHIFT);
    }
    BtechSurfaceQueryResult surfaceQuery={0};
    if (BtechQueryWorldSurface(&surfaceQuery,position,selectedWorldObjects,(int16_t)selectedObjectCount))
        return (uint16_t)surfaceQuery.surfacePositionC;
    return 0;
}

/*0810:5212. Original signed interpolation. Runtime32-bit signed division
 * becomes C division; widen only that operation to avoid INT_MIN/-1 host UB,
 * then retain its LOW WORD. Probe endpoint/overshoot, not the initial point. */
uint8_t BtechTestTerrainLineOfSight(const BtechPosition3D *start,const BtechPosition3D *end)
{
    if ((int32_t)start->positionA>(int32_t)end->positionA)
    {const BtechPosition3D *swap=start;start=end;end=swap;}
    uint32_t deltaA=end->positionA-start->positionA;
    uint32_t deltaB=end->positionB-start->positionB;
    uint32_t deltaC=end->positionC-start->positionC,magnitudeB=deltaB;
    if (magnitudeB==0x8000) magnitudeB=0x7FFF;
    else if ((int32_t)magnitudeB<0) magnitudeB=0-magnitudeB;
    if ((int32_t)deltaA<TERRAIN_WORLD_CELL_SIZE && (int32_t)magnitudeB<TERRAIN_WORLD_CELL_SIZE)
        return 1;
    uint8_t dominantA=(int32_t)deltaA>(int32_t)magnitudeB;
    uint32_t distance=dominantA?deltaA:magnitudeB,numerator;
    if (!distance) abort(); /* Native division fault. */
    int16_t stepA,stepB;
    if (dominantA)
    {stepA=TERRAIN_WORLD_CELL_SIZE;numerator=deltaB<<TERRAIN_WORLD_CELL_SHIFT;stepB=(int16_t)(uint16_t)((int64_t)(int32_t)numerator/(int32_t)distance);}
    else
    {numerator=deltaA<<TERRAIN_WORLD_CELL_SHIFT;stepA=(int16_t)(uint16_t)((int64_t)(int32_t)numerator/(int32_t)distance);stepB=(int32_t)deltaB<0?-TERRAIN_WORLD_CELL_SIZE:TERRAIN_WORLD_CELL_SIZE;}
    numerator=deltaC<<TERRAIN_WORLD_CELL_SHIFT;
    int16_t stepC=(int16_t)(uint16_t)((int64_t)(int32_t)numerator/(int32_t)distance);
    BtechPosition3D probe=*start;
    uint32_t endA=end->positionA,endB=end->positionB;
    uint8_t reached=0;
    while (!reached)
    {
        probe.positionA+=(uint32_t)(int32_t)stepA;
        probe.positionB+=(uint32_t)(int32_t)stepB;
        probe.positionC+=(uint32_t)(int32_t)stepC;
        reached=dominantA?(int32_t)probe.positionA>=(int32_t)endA:
            (int32_t)deltaB>0?(int32_t)probe.positionB>=(int32_t)endB:(int32_t)probe.positionB<=(int32_t)endB;
        int32_t height=(int16_t)BtechSampleTerrainOrSurfaceHeight(&probe);
        if (height>(int32_t)probe.positionC) return 0;
    }
    return 1;
}

/* Return the target's render-visibility mask. The objective building is a
 * separate world object, not another BattleMech slot. */
uint8_t BtechTestPlayerTargetRenderVisibility(int16_t targetSlot)
{
    if (targetSlot<=0 || targetSlot>COMBAT_SPECIAL_TARGET_SLOT || !playerUnitBinding
        || !playerUnitBinding->positionedLeftLeg) return 0;
    if (targetSlot!=COMBAT_SPECIAL_TARGET_SLOT && !unitBindings[targetSlot].mech) return 0;
    if (targetSlot==COMBAT_SPECIAL_TARGET_SLOT && !specialTargetDamageRemaining) return 0;
    const BtechPosition3D *player=&playerUnitBinding->positionedLeftLeg->object.prefix.position;
    const BtechPooledWorldObject *target=targetSlot==COMBAT_SPECIAL_TARGET_SLOT
        ?specialTargetObject:unitBindings[targetSlot].positionedLeftLeg;
    if (!target) return 0;
    uint32_t deltaA=player->positionA-target->object.prefix.position.positionA;
    uint32_t deltaB=player->positionB-target->object.prefix.position.positionB;
    if (deltaA==0x8000) deltaA=0x7FFF;else if ((int32_t)deltaA<0) deltaA=0-deltaA;
    if (deltaB==0x8000) deltaB=0x7FFF;else if ((int32_t)deltaB<0) deltaB=0-deltaB;
    if ((int32_t)deltaA>=COMBAT_VISIBLE_PLANAR_LIMIT || (int32_t)deltaB>=COMBAT_VISIBLE_PLANAR_LIMIT) return 0;
    const BtechPooledWorldObject *lastDrawnPart=targetSlot==COMBAT_SPECIAL_TARGET_SLOT
        ?unknownWorldObject:unitLastDrawParts[targetSlot];
    return lastDrawnPart?lastDrawnPart->object.prefix.flags&WORLD_OBJECT_TARGET_PROBE_VISIBLE:0;
}

/*0810:445B. Starting from no target uses slot4 then increments BEFORE its
 * first probe. Visibility is called again on exit, even after success. */
uint16_t BtechCyclePlayerVisibleTarget(uint16_t currentTargetSlot)
{
    uint16_t targetLimit=(int16_t)sideObjectiveRules[0]>OBJECTIVE_OPPOSE_WITHDRAWAL?COMBAT_MECH_SLOTS+1:COMBAT_MECH_SLOTS;
    uint16_t startingSlot=currentTargetSlot?currentTargetSlot:COMBAT_LANCE_SLOTS;
    uint16_t candidate=startingSlot;
    do
    {
        ++candidate;
        if (candidate==targetLimit) candidate=1;
    } while (!BtechTestPlayerTargetRenderVisibility((int16_t)candidate) && candidate!=startingSlot);
    return BtechTestPlayerTargetRenderVisibility((int16_t)candidate)?candidate:0;
}

/*0810:5460. Preserve present same-side cells. Opposing cells store actual
 * render mask02 or terrain predicate01, then copy that BYTE symmetrically. */
void BtechRefreshOpposingUnitVisibility(void)
{
    for (uint16_t observer=0;observer<COMBAT_MECH_SLOTS;++observer)
        for (uint16_t target=0;target<COMBAT_MECH_SLOTS;++target)
        {
            if (!unitBindings[target].mech || !unitBindings[observer].mech)
            {unitVisibility[target][observer]=0;unitVisibility[observer][target]=0;continue;}
            if (unitControls[observer].sideIndex || unitControls[target].sideIndex!=1) continue;
            if (!unitBindings[target].torso) abort();
            uint8_t projection=unitBindings[target].torso->object.prefix.renderFlags&WORLD_OBJECT_PROJECTED;
            uint8_t visible;
            if (!observer && !combatDispatchState && projection) visible=BtechTestPlayerTargetRenderVisibility((int16_t)target);
            else
            {
                if (!unitBindings[observer].positionedLeftLeg || !unitBindings[target].positionedLeftLeg) abort();
                visible=BtechTestTerrainLineOfSight(&unitBindings[observer].positionedLeftLeg->object.prefix.position,
                    &unitBindings[target].positionedLeftLeg->object.prefix.position);
            }
            unitVisibility[observer][target]=visible;
            unitVisibility[target][observer]=unitVisibility[observer][target];
        }
}

/*0810:6FE4. Native fillAFC0 clears each bound55-byte control record; typed
 * C zero-initialization represents those fields, not a fabricated stack frame.
 * Absent slots remain stale except for707C's separate score reset. */
void BtechInitializeUnitControlRecords(void)
{
    BtechBuildTemplateMaximumCombatScores();
    for (uint16_t slot=0;slot<COMBAT_MECH_SLOTS;++slot)
        if (unitBindings[slot].mech)
        {
            BtechUnitControlState *control=&unitControls[slot];
            *control=(BtechUnitControlState){0};
            control->commandCode=COMBAT_ORDER_ACT_ON_OWN;
            control->sideIndex=slot<COMBAT_LANCE_SLOTS?0:1;
            control->unitSlot=slot;
            control->lastDirectHitUnitSlot=-1;control->targetUnitSlot=-1;
            control->lastAiFiringTargetSlot=-1;control->trackedUnitOrSpecialSlot=-1;
            control->maneuverFlags|=COMBAT_CONTROL_INITIAL_MANEUVER_FLAG;
        }
    BtechRecalculateUnitCombatValuesAndBoundaries();
    BtechRefreshOpposingUnitVisibility();
    BtechRefreshLastVisibleUnitPositions();
}

/*0810:707C. Clear even absent-slot scores, then accumulate live armour,
 * structure and ALL10 operational mounts. Disabled and boundary codes override
 * that score;4112 observes the global active control, not this loop's record. */
void BtechRecalculateUnitCombatValuesAndBoundaries(void)
{
    for (uint16_t slot=0;slot<COMBAT_MECH_SLOTS;++slot)
    {
        BtechUnitControlState *control=&unitControls[slot];
        control->combatValueOrBoundaryCode=0;
        BtechUnitBinding *binding=&unitBindings[slot];
        BtechMechState *mech=binding->mech;
        if (!mech) continue;
        if (mech->templateIndex>=COMBAT_MECH_TEMPLATES) abort();
        BtechMechTemplate *chassis=&mechTemplates[mech->templateIndex];
        if (mech->gyroHits!=COMBAT_GYRO_DESTROYED_HITS)
            for (uint16_t location=0;location<COMBAT_ARMOR_LOCATIONS;++location)
                control->combatValueOrBoundaryCode=(int16_t)(uint16_t)((uint16_t)control->combatValueOrBoundaryCode+mech->currentArmor[location]);
        for (uint16_t location=0;location<COMBAT_BODY_LOCATIONS;++location)
            control->combatValueOrBoundaryCode=(int16_t)(uint16_t)((uint16_t)control->combatValueOrBoundaryCode+mech->currentInternalStructure[location]);
        if (mech->sensorHits!=COMBAT_SENSORS_DESTROYED_HITS)
            for (uint16_t mount=0;mount<COMBAT_WEAPON_MOUNTS;++mount)
                if (mech->weapons[mount].operational)
                {
                    int16_t kind=chassis->weaponMounts[mount].weaponKind;
                    if (kind<0 || kind>=COMBAT_WEAPON_KINDS) abort();
                    control->combatValueOrBoundaryCode=(int16_t)(uint16_t)((uint16_t)control->combatValueOrBoundaryCode
                        +(uint16_t)(weaponDefinitions[kind].damageOrMissileCount<<1));
                }
        if (!BtechIsMechOperational(mech)) control->combatValueOrBoundaryCode=-1;
        uint8_t boundary=BtechClassifyUnitWorldBoundary(slot);
        if (boundary<COMBAT_BOUNDARY_INSIDE)
        {
            control->combatValueOrBoundaryCode=-(boundary+2);
            control->controlState=COMBAT_TACTICAL_UNENGAGED;
            if (!binding->torso) abort();
            binding->torso->object.prefix.renderFlags|=COMBAT_WORLD_OBJECT_HIDDEN;
            if (!binding->head) abort();
            binding->head->object.prefix.renderFlags|=COMBAT_WORLD_OBJECT_HIDDEN;
            if (!binding->positionedLeftLeg) abort();
            binding->positionedLeftLeg->object.prefix.renderFlags|=COMBAT_WORLD_OBJECT_HIDDEN;
            if (!binding->rightLeg) abort();
            binding->rightLeg->object.prefix.renderFlags|=COMBAT_WORLD_OBJECT_HIDDEN;
            if (!binding->leftArm) abort();
            binding->leftArm->object.prefix.renderFlags|=COMBAT_WORLD_OBJECT_HIDDEN;
            if (!binding->rightArm) abort();
            binding->rightArm->object.prefix.renderFlags|=COMBAT_WORLD_OBJECT_HIDDEN;
        }
    }
}
