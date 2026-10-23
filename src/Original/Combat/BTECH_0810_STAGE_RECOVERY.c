#include "mechwarrior/port_diagnostics.h"
#include "mechwarrior/combat_import.h"
#include <stdlib.h>

enum { SALVAGE_ENGINE_WEIGHT=30,SALVAGE_SENSOR_WEIGHT=5,SALVAGE_COCKPIT_WEIGHT=30,
    SALVAGE_GYRO_WEIGHT=10,SALVAGE_WEAPON_WEIGHT=5,SALVAGE_ARMOR_STRUCTURE_WEIGHT=20,
    STAGE_RECOVERY_NUMERATOR=3,STAGE_RECOVERY_DENOMINATOR=5,
    STAGE_WEAPON_RECOVERY_BOUND=10,STAGE_WEAPON_RECOVERY_THRESHOLD=5 };

/*0810:3D70. Native signed LOW-WORD products before CWD/IDIV; AL is a
 * weighted condition percentage, not C-bills or Battle Value. No clamping. */
uint8_t BtechCalculateSalvageConditionPercent(const BtechMechState *mech)
{
    if (!mech || mech->templateIndex>=COMBAT_MECH_TEMPLATES) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !mech || mech->templateIndex>=COMBAT_MECH_TEMPLATES");
    const BtechMechTemplate *chassis=&mechTemplates[mech->templateIndex];
    int16_t engine=(int16_t)((COMBAT_ENGINE_HIT_LIMIT-mech->engineHits)*SALVAGE_ENGINE_WEIGHT)/COMBAT_ENGINE_HIT_LIMIT;
    int16_t sensors=(int16_t)((COMBAT_SENSORS_DESTROYED_HITS-mech->sensorHits)*SALVAGE_SENSOR_WEIGHT)/COMBAT_SENSORS_DESTROYED_HITS;
    int16_t gyro=(int16_t)((COMBAT_GYRO_DESTROYED_HITS-mech->gyroHits)*SALVAGE_GYRO_WEIGHT)/COMBAT_GYRO_DESTROYED_HITS;
    uint16_t subtotal=engine+sensors+(uint16_t)((1-mech->cockpitDisabled)*SALVAGE_COCKPIT_WEIGHT)+gyro;
    int16_t operationalMounts=0;
    for (int16_t mount=0;mount<(int16_t)chassis->weaponMountCount;++mount)
    {
        if (mount>=COMBAT_WEAPON_MOUNTS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: mount>=COMBAT_WEAPON_MOUNTS");
        if (mech->weapons[mount].operational) ++operationalMounts;
    }
    if (!chassis->weaponMountCount) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !chassis->weaponMountCount"); /* Native divide fault, not invented zero score. */
    subtotal+=(int16_t)(operationalMounts*SALVAGE_WEAPON_WEIGHT)/(int16_t)chassis->weaponMountCount;
    uint16_t remaining=0,maximum=0;
    for (uint16_t armor=0;armor<COMBAT_ARMOR_LOCATIONS;++armor)
    {maximum+=chassis->maximumArmor[armor];remaining+=mech->currentArmor[armor];}
    for (uint16_t body=0;body<COMBAT_BODY_LOCATIONS;++body)
    {maximum+=chassis->maximumInternalStructure[body];remaining+=mech->currentInternalStructure[body];}
    if (!maximum) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !maximum");
    int32_t protection=(int16_t)(remaining*SALVAGE_ARMOR_STRUCTURE_WEIGHT)/(int16_t)maximum;
    if (protection<INT16_MIN || protection>INT16_MAX) /* Port emulation of a native CPU fault. */ MW_NATIVE_FAULT("Rejected condition: protection<INT16_MIN || protection>INT16_MAX"); /* Native IDIV overflow. */
    return (uint8_t)(subtotal+protection);
}

/*0810:3EAC. One salvo per present friendly unit per sweep, repeated until
 * reserve empty or no signed-capacity comparison admits another salvo. */
void BtechDistributeCampaignReserveAmmo(void)
{
    for (uint16_t family=0;family<COMBAT_AMMO_FAMILIES;++family)
    {
        MwTransferWord *reserve=&campaignResultExchange.reserveAmmo[family];
        uint16_t remaining=reserve->low|((uint16_t)reserve->high<<8);
        uint16_t distributed;
        do
        {
            distributed=0;
            for (uint16_t slot=0;slot<COMBAT_LANCE_SLOTS && remaining;++slot)
            {
                BtechMechState *mech=unitBindings[slot].mech;
                if (!mech) continue;
                if (mech->templateIndex>=COMBAT_MECH_TEMPLATES) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: mech->templateIndex>=COMBAT_MECH_TEMPLATES");
                const BtechMechTemplate *chassis=&mechTemplates[mech->templateIndex];
                if ((int16_t)chassis->initialAmmoSalvos[family]>(int16_t)mech->ammoSalvosRemaining[family])
                {++mech->ammoSalvosRemaining[family];--remaining;distributed=1;}
            }
        } while (distributed);
        reserve->low=(uint8_t)remaining;reserve->high=(uint8_t)(remaining>>8);
    }
}

/*0810:3F7B. Between-stage partial recovery, NOT campaign shop repair. All
 * signed divisions use the wrapped product LOW WORD. RNG only for disabled
 * weapons; heat, reload, ammo, jump charge and destroyed jet groups persist. */
void BtechRecoverFriendlyMechCondition(void)
{
    for (uint16_t slot=0;slot<COMBAT_LANCE_SLOTS;++slot)
    {
        BtechMechState *mech=unitBindings[slot].mech;
        if (!mech) continue;
        if (mech->templateIndex>=COMBAT_MECH_TEMPLATES) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: mech->templateIndex>=COMBAT_MECH_TEMPLATES");
        const BtechMechTemplate *chassis=&mechTemplates[mech->templateIndex];
        mech->destroyedHeatSinks=(uint16_t)((int16_t)(mech->destroyedHeatSinks*STAGE_RECOVERY_NUMERATOR)/STAGE_RECOVERY_DENOMINATOR);
        mech->lifeSupportHits=(uint16_t)((int16_t)(mech->lifeSupportHits*STAGE_RECOVERY_NUMERATOR)/STAGE_RECOVERY_DENOMINATOR);
        mech->gyroHits=(uint16_t)((int16_t)(mech->gyroHits*STAGE_RECOVERY_NUMERATOR)/STAGE_RECOVERY_DENOMINATOR);
        mech->engineHits=(uint16_t)((int16_t)(mech->engineHits*STAGE_RECOVERY_NUMERATOR)/STAGE_RECOVERY_DENOMINATOR);
        mech->cockpitDisabled=(uint16_t)((int16_t)(mech->cockpitDisabled*STAGE_RECOVERY_NUMERATOR)/STAGE_RECOVERY_DENOMINATOR);
        mech->sensorHits=(uint16_t)((int16_t)(mech->sensorHits*STAGE_RECOVERY_NUMERATOR)/STAGE_RECOVERY_DENOMINATOR);
        for (uint16_t actuator=0;actuator<COMBAT_ACTUATORS;++actuator)
        {
            uint16_t deficit=COMBAT_ACTUATOR_CONDITION_FULL-mech->actuatorCondition[actuator];
            int16_t remaining=(int16_t)(deficit*STAGE_RECOVERY_NUMERATOR)/STAGE_RECOVERY_DENOMINATOR;
            mech->actuatorCondition[actuator]=(uint16_t)(COMBAT_ACTUATOR_CONDITION_FULL-remaining);
        }
        for (int16_t mount=0;(int16_t)chassis->weaponMountCount>mount;++mount)
        {
            if (mount>=COMBAT_WEAPON_MOUNTS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: mount>=COMBAT_WEAPON_MOUNTS");
            if (!mech->weapons[mount].operational && (int16_t)BtechRandomBelowBound(STAGE_WEAPON_RECOVERY_BOUND)>STAGE_WEAPON_RECOVERY_THRESHOLD)
                mech->weapons[mount].operational=1;
        }
        for (uint16_t armor=0;armor<COMBAT_ARMOR_LOCATIONS;++armor)
        {
            uint16_t deficit=chassis->maximumArmor[armor]-mech->currentArmor[armor];
            int16_t restored=(int16_t)(deficit*STAGE_RECOVERY_NUMERATOR)/STAGE_RECOVERY_DENOMINATOR;
            mech->currentArmor[armor]+=restored;
        }
        for (uint16_t body=0;body<COMBAT_BODY_LOCATIONS;++body)
        {
            uint16_t deficit=chassis->maximumInternalStructure[body]-mech->currentInternalStructure[body];
            int16_t restored=(int16_t)(deficit*STAGE_RECOVERY_NUMERATOR)/STAGE_RECOVERY_DENOMINATOR;
            mech->currentInternalStructure[body]+=restored;
        }
    }
}
