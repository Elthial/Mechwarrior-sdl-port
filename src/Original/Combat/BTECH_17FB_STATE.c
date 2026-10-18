#include "mechwarrior/combat_world.h"
#include <stdlib.h>

/*17FB:243C. One table, three accumulating cursors. This game's condition
 * weight is NOT tabletop Battle Value. Mount count is signed, not always10. */
void BtechBuildTemplateMaximumCombatScores(void)
{
    for (uint16_t templateIndex=0;templateIndex<COMBAT_MECH_TEMPLATES;++templateIndex)
    {
        const BtechMechTemplate *mech=&mechTemplates[templateIndex];
        templateMaximumCombatScores[templateIndex]=0;
        for (uint16_t location=0;location<COMBAT_ARMOR_LOCATIONS;++location)
            templateMaximumCombatScores[templateIndex]+=mech->maximumArmor[location];
        for (uint16_t location=0;location<COMBAT_BODY_LOCATIONS;++location)
            templateMaximumCombatScores[templateIndex]+=mech->maximumInternalStructure[location];
        for (int16_t mount=0;mount<(int16_t)mech->weaponMountCount;++mount)
        {
            if (mount>=COMBAT_WEAPON_MOUNTS) abort(); /* Native adjacent-template alias. */
            int16_t kind=mech->weaponMounts[mount].weaponKind;
            if (kind<0 || kind>=COMBAT_WEAPON_KINDS) abort(); /* Native adjacent-definition alias. */
            templateMaximumCombatScores[templateIndex]+=(uint16_t)(weaponDefinitions[kind].damageOrMissileCount<<1);
        }
    }
}

/*17FB:2888. Positive00008000 has a native special case, INT32_MIN does not.
 * Signed comparison leaves wrapped INT32_MIN inside. B wins at corners. */
uint8_t BtechClassifyUnitWorldBoundary(uint16_t unitSlot)
{
    if (unitSlot>=COMBAT_MECH_SLOTS || !unitBindings[unitSlot].positionedLeftLeg) abort();
    const BtechPosition3D *position=&unitBindings[unitSlot].positionedLeftLeg->object.prefix.position;
    uint8_t boundary=COMBAT_BOUNDARY_INSIDE;
    uint32_t magnitude=position->positionA;
    if (magnitude==0x8000) magnitude=0x7FFF;
    else if ((int32_t)magnitude<0) magnitude=0-magnitude;
    if ((int32_t)magnitude>COMBAT_BOUNDARY_LIMIT_A)
        boundary=(int32_t)position->positionA>0?COMBAT_BOUNDARY_POSITIVE_A:COMBAT_BOUNDARY_NEGATIVE_A;
    magnitude=position->positionB;
    if (magnitude==0x8000) magnitude=0x7FFF;
    else if ((int32_t)magnitude<0) magnitude=0-magnitude;
    if ((int32_t)magnitude>COMBAT_BOUNDARY_LIMIT_B)
        boundary=(int32_t)position->positionB>0?COMBAT_BOUNDARY_POSITIVE_B:COMBAT_BOUNDARY_NEGATIVE_B;
    return boundary;
}

/*17FB:2954. Target-owned last-seen coordinates, not an observer cache. Two
 * passes visit side1 then side0; matrix indexing is [observer][target]. */
void BtechRefreshLastVisibleUnitPositions(void)
{
    for (uint16_t pass=0;pass<2;++pass)
    {
        uint16_t targetSide=pass?0:1,observerSide=pass?1:0;
        for (uint16_t target=0;target<COMBAT_MECH_SLOTS;++target)
        {
            BtechUnitControlState *control=&unitControls[target];
            if (!unitBindings[target].mech || control->sideIndex!=targetSide) continue;
            if (control->positionKnowledgeCode!=COMBAT_POSITION_UNKNOWN)
                control->positionKnowledgeCode=COMBAT_POSITION_REMEMBERED;
            for (uint16_t observer=0;observer<COMBAT_MECH_SLOTS;++observer)
                if (unitBindings[observer].mech && unitControls[observer].sideIndex==observerSide
                    && unitVisibility[observer][target])
                {
                    control->positionKnowledgeCode=COMBAT_POSITION_VISIBLE;
                    if (!unitBindings[target].positionedLeftLeg) abort();
                    const BtechPosition3D *position=&unitBindings[target].positionedLeftLeg->object.prefix.position;
                    control->lastVisiblePosition[0]=(int32_t)position->positionA;
                    control->lastVisiblePosition[1]=(int32_t)position->positionB;
                    control->lastVisiblePosition[2]=(int32_t)position->positionC;
                    break;
                }
        }
    }
}
