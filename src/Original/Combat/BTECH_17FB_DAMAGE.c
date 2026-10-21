#include "mechwarrior/port_diagnostics.h"
#include "mechwarrior/combat_damage.h"
#include "mechwarrior/combat_import.h"
#include "mechwarrior/combat_startup.h"
#include <stdlib.h>

/*17FB:1954. Equipment losses precede subsystem/terminal effects. Repeated jet
 * destruction increments again; this is not an idempotent bookkeeping pass. */
uint8_t BtechApplyDestroyedBodyLocationEffects(BtechMechState *mech,int16_t bodyLocation)
{
    if (mech->templateIndex>=COMBAT_MECH_TEMPLATES) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: mech->templateIndex>=COMBAT_MECH_TEMPLATES");
    BtechMechTemplate *chassis=&mechTemplates[mech->templateIndex];
    uint8_t terminal=0;
    for (int16_t mount=0;mount<(int16_t)chassis->weaponMountCount;++mount)
    {
        if (mount>=COMBAT_WEAPON_MOUNTS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: mount>=COMBAT_WEAPON_MOUNTS");
        if (chassis->weaponMounts[mount].bodyLocation==(uint16_t)bodyLocation) mech->weapons[mount].operational=0;
    }
    for (uint16_t family=0;family<COMBAT_AMMO_FAMILIES;++family)
        if (chassis->ammoBodyLocations[family]==bodyLocation) mech->ammoSalvosRemaining[family]=0;
    for (uint16_t group=0;group<COMBAT_JET_GROUPS;++group)
        if (chassis->jumpJetGroups[group].bodyLocation==bodyLocation)
        {mech->jumpJetGroupOperational[group]=0;++mech->destroyedJumpJetGroups;}
    if (bodyLocation<COMBAT_BODY_HEAD)
        mech->destroyedHeatSinks+=(int16_t)(chassis->heatSinkCount+bodyLocation)/COMBAT_HEAT_SINK_LOCATION_DIVISOR;
    if (bodyLocation>=COMBAT_BODY_LEFT_ARM && bodyLocation<=COMBAT_BODY_RIGHT_LEG)
    {
        uint16_t actuator=actuatorIndicesByBodyLocation[bodyLocation];
        if (actuator>=COMBAT_ACTUATORS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: actuator>=COMBAT_ACTUATORS");
        mech->actuatorCondition[actuator]=0;
    }
    else if (bodyLocation==COMBAT_BODY_HEAD)
    {
        mech->lifeSupportHits=COMBAT_LIFE_SUPPORT_HIT_LIMIT;
        mech->sensorHits=COMBAT_SENSORS_DESTROYED_HITS;mech->cockpitDisabled=1;terminal=1;
    }
    else if (bodyLocation==COMBAT_BODY_CENTER_TORSO)
    {
        mech->gyroHits=COMBAT_GYRO_DESTROYED_HITS;mech->engineHits=COMBAT_ENGINE_HIT_LIMIT;terminal=1;
        if (mech==playerMechState && !(combatTickCounter&COMBAT_CENTER_TORSO_COCKPIT_TICK_MASK)) mech->cockpitDisabled=1;
    }
    return terminal;
}
