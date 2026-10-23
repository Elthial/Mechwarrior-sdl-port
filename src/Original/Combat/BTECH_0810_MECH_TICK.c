#include "mechwarrior/port_diagnostics.h"
#include "mechwarrior/combat_mechs.h"
#include "mechwarrior/combat_world.h"
#include "mechwarrior/combat_import.h"
#include "mechwarrior/combat_startup.h"
#include "mechwarrior/combat_pose.h"
#include "mechwarrior/combat_render.h"
#include <stdlib.h>

/* Native per-update heat ratios, not tabletop heat per turn. */
enum { MOVEMENT_HEAT_UPPER_THRESHOLD_MULTIPLIER=2,MOVEMENT_HEAT_THRESHOLD_DIVISOR=3,
    MOVEMENT_HEAT_UPPER_INCREMENT=2,DOUBLE_COOLING_MULTIPLIER=2,
    HEAT_SINK_COOLING_NUMERATOR=4,HEAT_SINK_COOLING_DENOMINATOR=3 };

/*0810:AD12. The chassis sentinel, not live operational state, controls which
 * reload timers run. Destroying a weapon does not clear/freeze its timer. */
void BtechAdvanceWeaponReloadTimers(void)
{
    for (uint16_t slot=0;slot<COMBAT_MECH_SLOTS;++slot)
    {
        BtechMechState *mech=unitBindings[slot].mech;
        if (!mech) continue;
        if (mech->templateIndex>=COMBAT_MECH_TEMPLATES) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: mech->templateIndex>=COMBAT_MECH_TEMPLATES");
        const BtechMechTemplate *chassis=&mechTemplates[mech->templateIndex];
        int16_t mountCount=chassis->weaponMountCount;
        for (int16_t mount=0;mount<mountCount;++mount)
        {
            if (mount>=COMBAT_WEAPON_MOUNTS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: mount>=COMBAT_WEAPON_MOUNTS"); /* Native neighbouring-owner overrun. */
            if ((uint16_t)chassis->weaponMounts[mount].weaponKind!=COMBAT_WEAPON_NONE
                && mech->weapons[mount].reloadTicksRemaining)
                --mech->weapons[mount].reloadTicksRemaining;
        }
    }
}

/*0810:AD92. Per-tick movement/engine heat and cooling publish speed penalty,
 * aim jitter tier and fire/jump inhibition. No invented shutdown state. */
void BtechUpdateUnitHeatEffects(void)
{
    for (uint16_t unitSlot=0;unitSlot<COMBAT_MECH_SLOTS;++unitSlot)
    {
        BtechMechState *heatedMech=unitBindings[unitSlot].mech;
        if (!heatedMech) continue; /* Absent slots keep old derived entries. */
        uint16_t movementHeatProductLow=heatedMech->movementReferenceScale*COMBAT_MOVEMENT_HEAT_REFERENCE_MULTIPLIER;
        /* Native LOW IMUL word, then SAR7, before either signed division. */
        int16_t movementHeatReference=(movementHeatProductLow>>COMBAT_MOVEMENT_HEAT_REFERENCE_SHIFT)
            | (movementHeatProductLow&0x8000?0xFE00:0);
        if (unitControls[unitSlot].movementMode!=MOVEMENT_MODE_JUMP)
        {
            const BtechPooledWorldObject *movingMechObject=unitBindings[unitSlot].positionedLeftLeg;
            if (!movingMechObject) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !movingMechObject");
            int16_t absoluteForwardStep=movingMechObject->forwardStepScale;
            if (absoluteForwardStep==INT16_MIN) absoluteForwardStep=INT16_MAX;
            else if (absoluteForwardStep<0) absoluteForwardStep=-absoluteForwardStep;
            int16_t runningHeatThresholdNumerator=(uint16_t)movementHeatReference*MOVEMENT_HEAT_UPPER_THRESHOLD_MULTIPLIER;
            if (absoluteForwardStep>runningHeatThresholdNumerator/MOVEMENT_HEAT_THRESHOLD_DIVISOR) heatedMech->heatUnits+=MOVEMENT_HEAT_UPPER_INCREMENT;
            else if (absoluteForwardStep>movementHeatReference/MOVEMENT_HEAT_THRESHOLD_DIVISOR) ++heatedMech->heatUnits;
        }
        heatedMech->heatUnits+=heatedMech->engineHits*COMBAT_ENGINE_HIT_HEAT;
        if (heatedMech->templateIndex>=COMBAT_MECH_TEMPLATES) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: heatedMech->templateIndex>=COMBAT_MECH_TEMPLATES");
        uint16_t effectiveCoolingSinkCount=mechTemplates[heatedMech->templateIndex].heatSinkCount-heatedMech->destroyedHeatSinks;
        if (heatedMech->doubleCoolingFlag || combatLandscapeIndex==COMBAT_ARCTIC_LANDSCAPE) effectiveCoolingSinkCount*=DOUBLE_COOLING_MULTIPLIER;
        int16_t coolingNumeratorLow=effectiveCoolingSinkCount*HEAT_SINK_COOLING_NUMERATOR;
        heatedMech->heatUnits-=coolingNumeratorLow/HEAT_SINK_COOLING_DENOMINATOR;
        if ((int16_t)heatedMech->heatUnits<=0) heatedMech->heatUnits=0;
        else if ((int16_t)heatedMech->heatUnits>=COMBAT_HEAT_CLAMP_MAXIMUM) heatedMech->heatUnits=COMBAT_HEAT_CLAMP_MAXIMUM;
        heatSpeedPenalties[unitSlot]=0;
        int16_t remainingSpeedPenaltyHeat=heatedMech->heatUnits-COMBAT_HEAT_SPEED_TIER_STEP;
        while (remainingSpeedPenaltyHeat>0)
        {
            heatSpeedPenalties[unitSlot]+=COMBAT_HEAT_SPEED_TIER_PENALTY;
            remainingSpeedPenaltyHeat-=COMBAT_HEAT_SPEED_TIER_STEP;
        }
        heatAimJitterTiers[unitSlot]=-1;
        do
        {
            ++heatAimJitterTiers[unitSlot];
            if ((uint16_t)heatAimJitterTiers[unitSlot]>=COMBAT_HEAT_THRESHOLDS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: (uint16_t)heatAimJitterTiers[unitSlot]>=COMBAT_HEAT_THRESHOLDS");
        }
        while ((int16_t)heatAimJitterThresholds[heatAimJitterTiers[unitSlot]]<=(int16_t)heatedMech->heatUnits);
        heatInhibitFlags[unitSlot]=(int16_t)heatedMech->heatUnits>COMBAT_HEAT_INHIBIT_THRESHOLD;
    }
}

/*0810:AF5B. Installed chassis jets govern charge recovery, not the number
 * of operational jet groups. Odd799 remains799; this is not saturation. */
void BtechRechargeJumpCharge(void)
{
    for (uint16_t slot=0;slot<COMBAT_MECH_SLOTS;++slot)
    {
        BtechMechState *mech=unitBindings[slot].mech;
        if (!mech) continue;
        if (mech->templateIndex>=COMBAT_MECH_TEMPLATES) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: mech->templateIndex>=COMBAT_MECH_TEMPLATES");
        if (!mechTemplates[mech->templateIndex].jumpJetGroupCount
            || (int16_t)mech->jumpCharge>=COMBAT_JUMP_CHARGE_CAPACITY
            || unitControls[slot].movementMode==MOVEMENT_MODE_JUMP) continue;
        if ((int16_t)(mech->jumpCharge+COMBAT_JUMP_RECHARGE_STEP)<=COMBAT_JUMP_CHARGE_CAPACITY)
            mech->jumpCharge+=COMBAT_JUMP_RECHARGE_STEP;
    }
}

/*0810:C5B8. Shared Mech/projectile integrator: orientation WORDs wrap first,
 * projected forward movement follows, then three signed additive WORD steps.
 * Second-byte freeze80 is not first-byte hidden80. No host floating point. */
void BtechAdvancePooledObjectMotion(BtechPooledWorldObject *object)
{
    if (!object) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !object");
    BtechWorldObjectPrefix *pose=&object->object.prefix;
    if (pose->flags&WORLD_OBJECT_MOTION_FROZEN) return;
    pose->orientationA+=object->orientationSteps[0];
    pose->orientationB+=object->orientationSteps[1];
    pose->orientationC+=object->orientationSteps[2];
    uint32_t speed=(uint32_t)object->forwardStepScale;
    if (pose->orientationA || pose->orientationB)
    {
        int16_t negativeCosine=0-BtechLookupCosine(pose->orientationA);
        uint32_t product=(uint32_t)negativeCosine*speed;
        uint32_t planarStep=BtechShiftSignedDwordRight(product,ORIENTATION_FRACTION_BITS);
        product=planarStep*(uint32_t)(int16_t)BtechLookupSine(pose->orientationC);
        pose->position.positionA+=BtechShiftSignedDwordRight(product,ORIENTATION_FRACTION_BITS);
        product=(uint32_t)(int16_t)BtechLookupCosine(pose->orientationA)*speed;
        planarStep=BtechShiftSignedDwordRight(product,ORIENTATION_FRACTION_BITS);
        product=planarStep*(uint32_t)(int16_t)BtechLookupCosine(pose->orientationC);
        pose->position.positionB+=BtechShiftSignedDwordRight(product,ORIENTATION_FRACTION_BITS);
        product=(uint32_t)(int16_t)BtechLookupSine(pose->orientationA)*speed;
        pose->position.positionC+=BtechShiftSignedDwordRight(product,ORIENTATION_FRACTION_BITS);
    }
    else
    {
        uint32_t forwardStep=speed;
        if (pose->orientationC)
        {
            int16_t negativeSine=0-BtechLookupSine(pose->orientationC);
            uint32_t product=(uint32_t)negativeSine*speed;
            pose->position.positionA+=BtechShiftSignedDwordRight(product,ORIENTATION_FRACTION_BITS);
            product=(uint32_t)(int16_t)BtechLookupCosine(pose->orientationC)*speed;
            forwardStep=BtechShiftSignedDwordRight(product,ORIENTATION_FRACTION_BITS);
        }
        pose->position.positionB+=forwardStep;
    }
    pose->position.positionA+=(uint32_t)object->positionSteps[0];
    pose->position.positionB+=(uint32_t)object->positionSteps[1];
    pose->position.positionC+=(uint32_t)object->positionSteps[2];
}
