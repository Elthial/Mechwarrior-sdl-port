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
    BtechMechTemplate *damagedMechTemplate=&mechTemplates[mech->templateIndex];
    uint8_t stopDamageTransfer=0;
    for (int16_t mountIndex=0;mountIndex<(int16_t)damagedMechTemplate->weaponMountCount;++mountIndex)
    {
        if (mountIndex>=COMBAT_WEAPON_MOUNTS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: mountIndex>=COMBAT_WEAPON_MOUNTS");
        if (damagedMechTemplate->weaponMounts[mountIndex].bodyLocation==(uint16_t)bodyLocation) mech->weapons[mountIndex].operational=0;
    }
    for (uint16_t ammoFamily=0;ammoFamily<COMBAT_AMMO_FAMILIES;++ammoFamily)
        if (damagedMechTemplate->ammoBodyLocations[ammoFamily]==bodyLocation) mech->ammoSalvosRemaining[ammoFamily]=0;
    for (uint16_t jumpJetGroupIndex=0;jumpJetGroupIndex<COMBAT_JET_GROUPS;++jumpJetGroupIndex)
        if (damagedMechTemplate->jumpJetGroups[jumpJetGroupIndex].bodyLocation==bodyLocation)
        {mech->jumpJetGroupOperational[jumpJetGroupIndex]=0;++mech->destroyedJumpJetGroups;}
    if (bodyLocation<COMBAT_BODY_HEAD)
        mech->destroyedHeatSinks+=(int16_t)(damagedMechTemplate->heatSinkCount+bodyLocation)/COMBAT_HEAT_SINK_LOCATION_DIVISOR;
    if (bodyLocation>=COMBAT_BODY_LEFT_ARM && bodyLocation<=COMBAT_BODY_RIGHT_LEG)
    {
        uint16_t actuatorIndex=actuatorIndicesByBodyLocation[bodyLocation];
        if (actuatorIndex>=COMBAT_ACTUATORS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: actuatorIndex>=COMBAT_ACTUATORS");
        mech->actuatorCondition[actuatorIndex]=0;
    }
    else if (bodyLocation==COMBAT_BODY_HEAD)
    {
        mech->lifeSupportHits=COMBAT_LIFE_SUPPORT_HIT_LIMIT;
        mech->sensorHits=COMBAT_SENSORS_DESTROYED_HITS;mech->cockpitDisabled=1;stopDamageTransfer=1;
    }
    else if (bodyLocation==COMBAT_BODY_CENTER_TORSO)
    {
        mech->gyroHits=COMBAT_GYRO_DESTROYED_HITS;mech->engineHits=COMBAT_ENGINE_HIT_LIMIT;stopDamageTransfer=1;
        if (mech==playerMechState && !(combatTickCounter&COMBAT_CENTER_TORSO_COCKPIT_TICK_MASK)) mech->cockpitDisabled=1;
    }
    return stopDamageTransfer;
}
