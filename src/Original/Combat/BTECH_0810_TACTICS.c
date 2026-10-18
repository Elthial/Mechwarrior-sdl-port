#include "mechwarrior/combat_tactics.h"
#include "mechwarrior/combat_pose.h"
#include "mechwarrior/combat_import.h"
#include "mechwarrior/combat_render.h"
#include "mechwarrior/combat_ballistics.h"
#include "mechwarrior/combat_startup.h"
#include <stdlib.h>

/* Select the BattleMech, chassis, body objects and orders evaluated by the AI.
 *0810:7766. Ordered writes bind the original five active owners. */
void BtechSelectActiveUnitContext(uint16_t unitSlot)
{
    if (unitSlot>=COMBAT_MECH_SLOTS) abort();
    BtechUnitBinding *binding=&unitBindings[unitSlot];
    activeMechState=binding->mech;
    if (!activeMechState || activeMechState->templateIndex>=COMBAT_MECH_TEMPLATES) abort();
    activeMechTemplate=&mechTemplates[activeMechState->templateIndex];
    activeUnitPositionedObject=binding->positionedLeftLeg;
    activeUnitTorsoObject=binding->torso;
    activeUnitControl=&unitControls[unitSlot];
}

/*0810:7A80. Reload is deliberately ignored. WORD sums wrap before SAR;
 * equality at half maximum internal structure requests disengagement. */
uint16_t BtechShouldDisengageForWeaponsOrStructure(void)
{
    if (!activeMechTemplate || !activeMechState) abort();
    int16_t mount=0,count=(int16_t)activeMechTemplate->weaponMountCount;
    for (;mount<count;++mount)
    {
        if (mount>=COMBAT_WEAPON_MOUNTS) abort();
        int16_t kind=activeMechTemplate->weaponMounts[mount].weaponKind;
        if (kind<0 || kind>=COMBAT_WEAPON_KINDS) abort();
        int16_t ammo=weaponDefinitions[kind].ammoFamily;
        if ((uint16_t)ammo!=COMBAT_WEAPON_NONE)
        {
            if ((uint16_t)ammo>=COMBAT_AMMO_FAMILIES) abort();
            if (!activeMechState->ammoSalvosRemaining[ammo]) continue;
        }
        if (activeMechState->weapons[mount].operational) break;
    }
    if (mount==count) return 1;
    uint16_t maximumStructure=0,currentStructure=0;
    for (uint16_t location=0;location<COMBAT_BODY_LOCATIONS;++location)
    {
        maximumStructure+=activeMechTemplate->maximumInternalStructure[location];
        currentStructure+=activeMechState->currentInternalStructure[location];
    }
    int16_t halfMaximum=(int16_t)((maximumStructure>>1)|(maximumStructure&0x8000));
    return halfMaximum>=(int16_t)currentStructure;
}

/*0810:7B5A. Special-target geometry precedes ordinary visibility checks.
 * Signed thresholds are sign-extended WORDs; equality is not nearby. */
uint8_t BtechHasNearbyVisibleThreat(uint16_t *controlSideCursorResidue)
{
    /* 7BC6/7C29: this local cursor survives at the following7E3A BP-0E.
     * Numeric offset is observable as a heading there, not a C data accessor. */
    uint16_t controlSideCursor=TACTICAL_NATIVE_CONTROL_SIDE_START;
    if (controlSideCursorResidue) *controlSideCursorResidue=controlSideCursor;
    if (!activeUnitControl || activeUnitControl->sideIndex>=2) abort();
    BtechUnitControlState *control=activeUnitControl;
    if (sideObjectiveRules[control->sideIndex]==OBJECTIVE_SPECIAL_TARGET && specialTargetDamageRemaining)
    {
        if (!activeUnitPositionedObject || !specialTargetObject) abort();
        int32_t distance=(int32_t)BtechApproximatePlanarDistance(&activeUnitPositionedObject->object.prefix.position,&specialTargetObject->object.prefix.position);
        if (control->weaponRangeMinusMargin>distance
            && (control->commandCode==COMBAT_ORDER_ACT_ON_OWN || control->commandCode==TACTICAL_ORDER_ATTACK || control->commandCode==TACTICAL_ORDER_MOVE_ATTACK)) return 1;
    }
    if (control->unitSlot>=COMBAT_MECH_SLOTS) abort();
    for (uint16_t slot=0;slot<COMBAT_MECH_SLOTS;++slot)
        if (unitBindings[slot].mech && unitControls[slot].sideIndex!=control->sideIndex
            && unitVisibility[control->unitSlot][slot] && unitControls[slot].combatValueOrBoundaryCode>0
            && (int32_t)unitDistances[control->unitSlot][slot]<(int16_t)control->doubleWeaponRange)
        {
            /* The native loop increments all three cursors before checking
             * the found BYTE on its next iteration (including slot7). */
            controlSideCursor+=(uint16_t)((slot+1)*TACTICAL_NATIVE_CONTROL_STRIDE);
            if (controlSideCursorResidue) *controlSideCursorResidue=controlSideCursor;
            return 1;
        }
    if (controlSideCursorResidue) *controlSideCursorResidue=controlSideCursor+COMBAT_MECH_SLOTS*TACTICAL_NATIVE_CONTROL_STRIDE;
    return 0;
}

/*0810:7C48. The selected target contributes LAST-VISIBLE, not live position.
 * Snapshot transport is safe:94B0 writes tracked+20, never last-visible+2E. */
void BtechRefreshTargetAndNavigate(void)
{
    BtechScoreAndSelectTacticalTarget();
    if (activeUnitControl->targetUnitSlot>=0)
    {
        uint16_t target=activeUnitControl->targetUnitSlot;
        if (target>=COMBAT_MECH_SLOTS) abort(); /* Native special/adjacent-control alias. */
        const int32_t *lastSeen=unitControls[target].lastVisiblePosition;
        BtechPosition3D destination={(uint32_t)lastSeen[0],(uint32_t)lastSeen[1],(uint32_t)lastSeen[2]};
        BtechNavigateActiveUnitTowardPosition(&destination);
        activeUnitControl->navigationReferenceSlot=activeUnitControl->targetUnitSlot;
    }
    else
    {
        if (!activeUnitPositionedObject) abort();
        BtechSteerActiveUnitAroundObstacles(activeUnitPositionedObject->object.prefix.orientationC,1);
    }
}

/*0810:7D3A. Only operational, reloaded mounts count; ammunition does NOT.
 * Native also computes an unused minimum. Absent slots retain stale ranges. */
void BtechUpdateUnitWeaponRangeThresholds(void)
{
    for (uint16_t slot=0;slot<COMBAT_MECH_SLOTS;++slot)
    {
        BtechMechState *mech=unitBindings[slot].mech;
        if (!mech) continue;
        if (mech->templateIndex>=COMBAT_MECH_TEMPLATES) abort();
        BtechMechTemplate *chassis=&mechTemplates[mech->templateIndex];
        int16_t greatestRange=0;
        for (int16_t mount=0;mount<(int16_t)chassis->weaponMountCount;++mount)
        {
            if (mount>=COMBAT_WEAPON_MOUNTS) abort();
            if (!mech->weapons[mount].operational || mech->weapons[mount].reloadTicksRemaining) continue;
            int16_t kind=chassis->weaponMounts[mount].weaponKind;
            if (kind<0 || kind>=COMBAT_WEAPON_KINDS) abort();
            int16_t range=(int16_t)weaponDefinitions[kind].longRangeBoundary;
            if (range>greatestRange) greatestRange=range;
        }
        BtechUnitControlState *control=&unitControls[slot];
        control->doubleWeaponRange=(uint16_t)(greatestRange*TACTICAL_RANGE_SCALE*2);
        control->weaponRangeMinusMargin=(int16_t)(uint16_t)(greatestRange*TACTICAL_RANGE_SCALE-TACTICAL_RANGE_MARGIN);
        uint16_t scaledRange=(uint16_t)(greatestRange*TACTICAL_RANGE_SCALE);
        control->quarterWeaponRange=(int16_t)((scaledRange>>TACTICAL_QUARTER_RANGE_SHIFT)|((scaledRange&0x8000)?0xC000:0));
    }
}
