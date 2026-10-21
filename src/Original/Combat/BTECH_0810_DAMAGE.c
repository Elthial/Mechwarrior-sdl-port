#include "mechwarrior/port_diagnostics.h"
#include "mechwarrior/combat_damage.h"
#include "mechwarrior/combat_effects.h"
#include "mechwarrior/combat_import.h"
#include "mechwarrior/combat_startup.h"
#include "mechwarrior/combat_sound.h"
#include <stdlib.h>

/*0810:4BB6. Ordered 2d6 roll; failed selection transfers without spending budget. */
void BtechRollCriticalLosses(BtechMechState *mech,int16_t bodyLocation)
{
    uint16_t firstDie=BtechRandomBelowBound(DICE_1D6);
    uint16_t secondDie=BtechRandomBelowBound(DICE_1D6);
    int16_t remainingCriticalRollScore=firstDie+secondDie+COMBAT_CRITICAL_DICE_BASE;
    while (remainingCriticalRollScore>COMBAT_CRITICAL_ATTEMPT_THRESHOLD)
    {
        if (BtechSelectAndApplyCriticalLoss(mech,bodyLocation)) remainingCriticalRollScore-=COMBAT_CRITICAL_SUCCESS_COST;
        else
        {
            if ((uint16_t)bodyLocation>=COMBAT_BODY_LOCATIONS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: (uint16_t)bodyLocation>=COMBAT_BODY_LOCATIONS");
            uint16_t criticalTransferLocation=criticalTransferBodyLocations[bodyLocation];
            if (criticalTransferLocation==(uint16_t)bodyLocation) return;
            bodyLocation=(int16_t)criticalTransferLocation;
        }
    }
}

/* Spend a critical-hit selection on a system or mounted weapon in the struck
 * BattleMech location; this weighted walk is not a tabletop critical-slot grid.
 *0810:4C16. Weighted equipment walk, not a serialized physical critical grid.
 * Compare the unhalved roll on retry, including head/legs. Preserve that quirk. */
uint8_t BtechSelectAndApplyCriticalLoss(BtechMechState *mech,int16_t bodyLocation)
{
    if (mech->templateIndex>=COMBAT_MECH_TEMPLATES) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: mech->templateIndex>=COMBAT_MECH_TEMPLATES");
    BtechMechTemplate *damagedMechTemplate=&mechTemplates[mech->templateIndex];
    int16_t weaponMountCount=damagedMechTemplate->weaponMountCount;
    for (;;)
    {
        int16_t equipmentSelectionRoll=BtechRandomBelowBound(COMBAT_CRITICAL_SLOT_ROLL_BOUND)+1;
        int16_t remainingEquipmentWeight=equipmentSelectionRoll;
        if (bodyLocation==COMBAT_BODY_HEAD || bodyLocation==COMBAT_BODY_LEFT_LEG || bodyLocation==COMBAT_BODY_RIGHT_LEG) remainingEquipmentWeight=(remainingEquipmentWeight+1)/2;
        if (bodyLocation==COMBAT_BODY_CENTER_TORSO)
        {
            if ((int16_t)mech->engineHits<COMBAT_ENGINE_HIT_LIMIT)
            {
                remainingEquipmentWeight-=(COMBAT_ENGINE_CRITICAL_WEIGHT-mech->engineHits);
                if (remainingEquipmentWeight<=0)
                {
                    ++mech->engineHits;
                    if (mech==playerMechState)
                    {
                        if (mech->engineHits==COMBAT_ENGINE_HIT_LIMIT
                            && BtechRandomBelowBound(COMBAT_ENGINE_COCKPIT_ROLL_BOUND)==COMBAT_ENGINE_COCKPIT_LOSS_ROLL) mech->cockpitDisabled=1;
                        else BtechPlaySoundIfEnabled(COMBAT_CRITICAL_SOUND,COMBAT_CRITICAL_SOUND_PRIORITY);
                    }
                    return 1;
                }
            }
            if ((int16_t)mech->gyroHits<COMBAT_GYRO_DESTROYED_HITS)
            {
                remainingEquipmentWeight-=(COMBAT_GYRO_CRITICAL_WEIGHT-mech->gyroHits);
                if (remainingEquipmentWeight<=0)
                {
                    if (mech==playerMechState) BtechPlaySoundIfEnabled(COMBAT_CRITICAL_SOUND,COMBAT_CRITICAL_SOUND_PRIORITY);
                    ++mech->gyroHits;return 1;
                }
            }
        }
        else if (bodyLocation>=COMBAT_BODY_LEFT_ARM && bodyLocation<=COMBAT_BODY_RIGHT_LEG)
        {
            uint16_t actuatorIndex=actuatorIndicesByBodyLocation[bodyLocation];
            if (actuatorIndex>=COMBAT_ACTUATORS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: actuatorIndex>=COMBAT_ACTUATORS");
            remainingEquipmentWeight-=mech->actuatorCondition[actuatorIndex];
            if (remainingEquipmentWeight<=0) {--mech->actuatorCondition[actuatorIndex];return 1;}
        }
        else if (bodyLocation==COMBAT_BODY_HEAD)
        {
            if ((int16_t)mech->lifeSupportHits<COMBAT_LIFE_SUPPORT_HIT_LIMIT)
            {
                remainingEquipmentWeight-=(COMBAT_LIFE_SUPPORT_HIT_LIMIT-mech->lifeSupportHits);
                if (remainingEquipmentWeight<=0)
                {
                    if (mech==playerMechState) BtechPlaySoundIfEnabled(COMBAT_CRITICAL_SOUND,COMBAT_CRITICAL_SOUND_PRIORITY);
                    ++mech->lifeSupportHits;return 1;
                }
            }
            if ((int16_t)mech->sensorHits<COMBAT_SENSORS_DESTROYED_HITS)
            {
                remainingEquipmentWeight-=(COMBAT_SENSORS_DESTROYED_HITS-mech->sensorHits);
                if (remainingEquipmentWeight<=0)
                {
                    if (mech==playerMechState) BtechPlaySoundIfEnabled(COMBAT_CRITICAL_SOUND,COMBAT_CRITICAL_SOUND_PRIORITY);
                    ++mech->sensorHits;return 1;
                }
            }
            if ((int16_t)mech->cockpitDisabled<COMBAT_COCKPIT_CRITICAL_WEIGHT)
            {
                remainingEquipmentWeight-=(COMBAT_COCKPIT_CRITICAL_WEIGHT-mech->cockpitDisabled);
                if (remainingEquipmentWeight<=0) {++mech->cockpitDisabled;return 1;}
            }
        }
        for (int16_t mountIndex=0;mountIndex<weaponMountCount;++mountIndex)
        {
            if (mountIndex>=COMBAT_WEAPON_MOUNTS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: mountIndex>=COMBAT_WEAPON_MOUNTS");
            BtechWeaponMount *weaponMount=&damagedMechTemplate->weaponMounts[mountIndex];
            if (!mech->weapons[mountIndex].operational || weaponMount->bodyLocation!=(uint16_t)bodyLocation) continue;
            if ((uint16_t)weaponMount->weaponKind>=COMBAT_WEAPON_KINDS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: (uint16_t)weaponMount->weaponKind>=COMBAT_WEAPON_KINDS");
            remainingEquipmentWeight-=weaponDefinitions[weaponMount->weaponKind].criticalSlots;
            if (remainingEquipmentWeight<=0) {mech->weapons[mountIndex].operational=0;return 1;}
        }
        for (uint16_t ammoFamily=0;ammoFamily<COMBAT_AMMO_FAMILIES;++ammoFamily)
        {
            if (damagedMechTemplate->ammoBodyLocations[ammoFamily]!=bodyLocation || !mech->ammoSalvosRemaining[ammoFamily]) continue;
            remainingEquipmentWeight-=COMBAT_AMMO_CRITICAL_WEIGHT;
            if (!remainingEquipmentWeight) {mech->ammoSalvosRemaining[ammoFamily]=0;return 1;} /* Equality, not <=; no explosion. */
        }
        for (uint16_t jumpJetGroupIndex=0;jumpJetGroupIndex<COMBAT_JET_GROUPS;++jumpJetGroupIndex)
        {
            if (damagedMechTemplate->jumpJetGroups[jumpJetGroupIndex].bodyLocation!=bodyLocation || !mech->jumpJetGroupOperational[jumpJetGroupIndex]) continue;
            remainingEquipmentWeight-=damagedMechTemplate->jumpJetGroups[jumpJetGroupIndex].criticalSlots;
            if (remainingEquipmentWeight<=0) {mech->jumpJetGroupOperational[jumpJetGroupIndex]=0;++mech->destroyedJumpJetGroups;return 1;}
        }
        if (remainingEquipmentWeight==equipmentSelectionRoll) return 0;
    }
}

/*0810:4EDA. Armour equality still invokes critical rolling with zero residual. */
void BtechApplyArmorAndInternalDamage(uint16_t unitSlot,int16_t damageArmorLocation,int16_t residualDamage)
{
    /* Native 4EEB reads the low byte of this existing music row. Keep the
     * shared owner; this is not an independent invulnerability flag. */
    if (sideCompletionFlags[1] || ((uint16_t)musicSequences[0][0]&0xFF)) return;
    if (!unitSlot)
    {
        BtechPlaySoundIfEnabled(COMBAT_DAMAGE_SOUND,COMBAT_DAMAGE_SOUND_PRIORITY);
        playerImpactThisTick=1;
        if (playerDamageSuppressed) return;
    }
    else if (unitSlot==COMBAT_SPECIAL_TARGET_SLOT) {BtechApplySpecialTargetDamage(residualDamage);return;}
    if (unitSlot>=COMBAT_MECH_SLOTS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: unitSlot>=COMBAT_MECH_SLOTS");
    targetDamagePanelDirty[unitSlot]=1;
    BtechMechState *damagedMech=unitBindings[unitSlot].mech;
    while (residualDamage)
    {
        if (!damagedMech || (uint16_t)damageArmorLocation>=COMBAT_ARMOR_LOCATIONS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !damagedMech || (uint16_t)damageArmorLocation>=COMBAT_ARMOR_LOCATIONS");
        uint16_t *armorPoints=&damagedMech->currentArmor[damageArmorLocation];
        if ((int16_t)*armorPoints>residualDamage) {*armorPoints-=(uint16_t)residualDamage;residualDamage=0;continue;}
        residualDamage=(int16_t)((uint16_t)residualDamage-*armorPoints);*armorPoints=0;
        uint16_t internalBodyLocation=armorToBodyLocations[damageArmorLocation];
        if (internalBodyLocation>=COMBAT_BODY_LOCATIONS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: internalBodyLocation>=COMBAT_BODY_LOCATIONS");
        if (!damagedMech->currentInternalStructure[internalBodyLocation])
        {
            if (internalBodyLocation==COMBAT_BODY_CENTER_TORSO) residualDamage=0;
            else damageArmorLocation=(int16_t)damageTransferArmorLocations[internalBodyLocation];
            continue;
        }
        BtechRollCriticalLosses(damagedMech,(int16_t)internalBodyLocation);
        uint16_t *internalStructurePoints=&damagedMech->currentInternalStructure[internalBodyLocation];
        if ((int16_t)*internalStructurePoints>residualDamage) {*internalStructurePoints-=(uint16_t)residualDamage;residualDamage=0;continue;}
        BtechPooledWorldObject *destroyedBodyPart=BtechGetUnitRenderPartByArmorLocation(unitSlot,damageArmorLocation);
        if (destroyedBodyPart) destroyedBodyPart->object.prefix.renderFlags|=COMBAT_WORLD_OBJECT_HIDDEN;
        BtechQueueBodyDestructionEmitter(unitSlot,damageArmorLocation);
        residualDamage=(int16_t)((uint16_t)residualDamage-*internalStructurePoints);*internalStructurePoints=0;
        if (BtechApplyDestroyedBodyLocationEffects(damagedMech,(int16_t)internalBodyLocation)) residualDamage=0;
        else damageArmorLocation=(int16_t)damageTransferArmorLocations[internalBodyLocation];
    }
}

/*0810:5FA5. Armour HEAD=8 differs from internal HEAD=7. */
BtechPooledWorldObject *BtechGetUnitRenderPartByArmorLocation(uint16_t unitSlot,int16_t armorLocation)
{
    if (unitSlot>=COMBAT_MECH_SLOTS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: unitSlot>=COMBAT_MECH_SLOTS");
    BtechUnitBinding *binding=&unitBindings[unitSlot];
    switch (armorLocation)
    {
    case COMBAT_ARMOR_LEFT_LEG:return binding->positionedLeftLeg;
    case COMBAT_ARMOR_RIGHT_LEG:return binding->rightLeg;
    case COMBAT_ARMOR_LEFT_ARM:return binding->leftArm;
    case COMBAT_ARMOR_RIGHT_ARM:return binding->rightArm;
    case COMBAT_ARMOR_HEAD:return binding->head;
    default:return NULL;
    }
}

/*0810:5AC0. Player excluded; full emitter pool silently drops the request. */
void BtechQueueBodyDestructionEmitter(uint16_t unitSlot,int16_t armorLocation)
{
    if (!unitSlot) return;
    for (uint16_t slot=0;slot<COMBAT_DESTRUCTION_EMITTER_SLOTS;++slot)
    {
        BtechBodyDestructionEmitter *emitter=&bodyDestructionEmitters[slot];
        if (emitter->ticksRemaining) continue;
        emitter->unitSlot=unitSlot;emitter->armorLocation=armorLocation;
        emitter->ticksRemaining=COMBAT_DESTRUCTION_EMITTER_TICKS;return;
    }
}

/*0810:5017. Special objective capacity is not a ninth Mech. Five ordered
 * destruction effects use the original local position, not target mutations. */
void BtechApplySpecialTargetDamage(int16_t damage)
{
    if (!specialTargetDamageRemaining || specialTargetInteractionBlocked) return;
    specialTargetDamagePanelDirty=1;
    if (damage<(int16_t)specialTargetDamageRemaining) {specialTargetDamageRemaining-=(uint16_t)damage;return;}
    specialTargetDamageRemaining=0;
    if (!specialTargetObject) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !specialTargetObject");
    BtechWorldObjectPrefix *target=&specialTargetObject->object.prefix;
    target->renderFlags|=COMBAT_WORLD_OBJECT_HIDDEN;
    target->renderFlags&=(uint8_t)~WORLD_OBJECT_SPECIAL_TARGET;
    BtechPosition3D position=target->position;
    position.positionC=COMBAT_DESTRUCTION_HIGH_ALTITUDE;
    BtechSpawnSequencedWorldEffect(&position,COMBAT_IMPACT_EFFECT_SEQUENCE);
    position.positionC=COMBAT_DESTRUCTION_LOW_ALTITUDE;
    position.positionA+=COMBAT_DESTRUCTION_OFFSET;position.positionB+=COMBAT_DESTRUCTION_OFFSET;
    BtechSpawnSequencedWorldEffect(&position,COMBAT_IMPACT_EFFECT_SEQUENCE);
    position.positionA-=COMBAT_DESTRUCTION_OFFSET*2;
    BtechSpawnSequencedWorldEffect(&position,COMBAT_IMPACT_EFFECT_SEQUENCE);
    position.positionB-=COMBAT_DESTRUCTION_OFFSET*2;
    BtechSpawnSequencedWorldEffect(&position,COMBAT_IMPACT_EFFECT_SEQUENCE);
    position.positionA+=COMBAT_DESTRUCTION_OFFSET*2;
    BtechSpawnSequencedWorldEffect(&position,COMBAT_IMPACT_EFFECT_SEQUENCE);
}
