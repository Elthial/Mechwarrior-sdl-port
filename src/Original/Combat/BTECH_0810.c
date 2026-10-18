#include "mechwarrior/combat_mechs.h"
#include <stdlib.h>

/* BTECH0810:34A8..350D. Two signed SARs followed by wrapping WORD ADD,
 * armour then structure for each leg of each mutable chassis template. */
void BtechIncreaseTemplateLegIntegrity(void)
{
    for (uint16_t index=0;index<COMBAT_MECH_TEMPLATES;++index)
    {
        BtechMechTemplate *mech=&mechTemplates[index];
        for (uint16_t leg=COMBAT_BODY_LEFT_LEG;leg<=COMBAT_BODY_RIGHT_LEG;++leg)
        {
            uint16_t armor=mech->maximumArmor[leg];
            uint16_t quarter=(armor>>2)|(armor&0x8000?0xC000:0);
            mech->maximumArmor[leg]+=quarter;
            uint16_t structure=mech->maximumInternalStructure[leg];
            quarter=(structure>>2)|(structure&0x8000?0xC000:0);
            mech->maximumInternalStructure[leg]+=quarter;
        }
    }
}

/* BTECH0810:3583..36DE. Near pointers become typed references; no extra clear,
 * no movementReferenceScale copy, and unmounted/reload fields remain stale. */
void BtechInitializeMechConditionFromTemplate(BtechMechState *mech,uint16_t templateIndex)
{
    if (!mech || templateIndex>=COMBAT_MECH_TEMPLATES) abort(); /* Unrepresented adjacent data, not a native guard. */
    const BtechMechTemplate *chassis=&mechTemplates[templateIndex];
    mech->templateIndex=templateIndex;
    for (uint16_t location=0;location<COMBAT_BODY_LOCATIONS;++location)
        mech->currentInternalStructure[location]=chassis->maximumInternalStructure[location];
    for (uint16_t location=0;location<COMBAT_ARMOR_LOCATIONS;++location)
    {
        mech->currentArmor[location]=chassis->maximumArmor[location];
        if (!specialAssaultMode && BtechRandomBelowBound(COMBAT_RANDOM_WEAR_BOUND)==COMBAT_RANDOM_WEAR_ROLL)
            mech->currentArmor[location]-=BtechRandomBelowBound(chassis->maximumArmor[location]);
    }
    for (uint16_t mount=0;chassis->weaponMountCount<0x8000 && mount<chassis->weaponMountCount;++mount)
    {
        if (mount>=COMBAT_WEAPON_MOUNTS) abort();
        mech->weapons[mount].operational=1;
    }
    for (uint16_t actuator=0;actuator<COMBAT_ACTUATORS;++actuator)
    {
        mech->actuatorCondition[actuator]=COMBAT_ACTUATOR_CONDITION_FULL;
        if (!specialAssaultMode && BtechRandomBelowBound(COMBAT_RANDOM_WEAR_BOUND)==COMBAT_RANDOM_WEAR_ROLL)
            mech->actuatorCondition[actuator]-=BtechRandomBelowBound(2);
    }
    for (uint16_t family=0;family<COMBAT_AMMO_FAMILIES;++family)
        mech->ammoSalvosRemaining[family]=chassis->initialAmmoSalvos[family];
    for (uint16_t group=0;chassis->jumpJetGroupCount<0x8000 && group<chassis->jumpJetGroupCount;++group)
    {
        if (group>=COMBAT_JET_GROUPS) abort();
        mech->jumpJetGroupOperational[group]=1;
    }
    if (chassis->jumpJetGroupCount) mech->jumpCharge=COMBAT_JUMP_CHARGE_CAPACITY;
}

/* BTECH0810:36DF..3760. Only bind state and side; no initialization. */
BtechMechState *BtechClaimFreeMechStateSlot(uint16_t friendlySlots)
{
    uint16_t unitSlot=friendlySlots?0:COMBAT_LANCE_SLOTS;
    for (uint16_t examined=0;examined<COMBAT_LANCE_SLOTS;++examined,++unitSlot)
    {
        if (!unitBindings[unitSlot].mech)
        {
            unitBindings[unitSlot].mech=&mechStates[unitSlot];
            unitControls[unitSlot].sideIndex=friendlySlots<1;
            return &mechStates[unitSlot];
        }
    }
    return NULL;
}

/* BTECH0810:4163..4175. Other body references and the condition remain live. */
void BtechClearUnitStateBinding(uint16_t unitSlot)
{
    if (unitSlot>=COMBAT_MECH_SLOTS) abort(); /* Numeric wrap into unrelated native objects is not represented. */
    unitBindings[unitSlot].mech=NULL;
}

/* BTECH0810:CB56..CB7E. Cursors are deliberately not reset. */
void BtechMixRandomStateWords(uint16_t seed)
{
    for (uint16_t index=0;index<COMBAT_RANDOM_WORDS;++index) randomStateWords[index]+=seed;
}

/* BTECH0810:CB7F..CBD5. Bound0 really retries forever. FFFF faults AFTER
 * ring/cursor updates; no early validation, fallback result or host rand(). */
uint16_t BtechRandomBelowBound(uint16_t bound)
{
    uint16_t divisor=bound+1,remainder;
    do
    {
        if (randomWriteIndex>=COMBAT_RANDOM_WORDS || randomAddendIndex>=COMBAT_RANDOM_WORDS) abort();
        randomStateWords[randomWriteIndex]+=randomStateWords[randomAddendIndex];
        uint16_t value=randomStateWords[randomWriteIndex];
        randomWriteIndex=(randomWriteIndex+1)&COMBAT_RANDOM_INDEX_MASK;
        randomAddendIndex=(randomAddendIndex+1)&COMBAT_RANDOM_INDEX_MASK;
        if (!divisor) abort();
        remainder=(value&0x7FFF)%divisor;
    } while (!remainder);
    return remainder-1;
}
