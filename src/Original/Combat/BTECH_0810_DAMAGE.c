#include "mechwarrior/combat_damage.h"
#include "mechwarrior/combat_effects.h"
#include "mechwarrior/combat_import.h"
#include "mechwarrior/combat_startup.h"
#include "mechwarrior/combat_sound.h"
#include <stdlib.h>

/*0810:4BB6. Ordered 2d6 roll; failed selection transfers without spending budget. */
void BtechRollCriticalLosses(BtechMechState *mech,int16_t bodyLocation)
{
    uint16_t firstDie=BtechRandomBelowBound(6);
    uint16_t secondDie=BtechRandomBelowBound(6);
    int16_t budget=(int16_t)(uint16_t)(firstDie+secondDie+2);
    while (budget>COMBAT_CRITICAL_ATTEMPT_THRESHOLD)
    {
        if (BtechSelectAndApplyCriticalLoss(mech,bodyLocation)) budget-=COMBAT_CRITICAL_SUCCESS_COST;
        else
        {
            if ((uint16_t)bodyLocation>=COMBAT_BODY_LOCATIONS) abort();
            uint16_t destination=criticalTransferBodyLocations[bodyLocation];
            if (destination==(uint16_t)bodyLocation) return;
            bodyLocation=(int16_t)destination;
        }
    }
}

/* Spend a critical-hit selection on a system or mounted weapon in the struck
 * BattleMech location; this weighted walk is not a tabletop critical-slot grid.
 *0810:4C16. Weighted equipment walk, not a serialized physical critical grid.
 * Compare the unhalved roll on retry, including head/legs. Preserve that quirk. */
uint8_t BtechSelectAndApplyCriticalLoss(BtechMechState *mech,int16_t bodyLocation)
{
    if (mech->templateIndex>=COMBAT_MECH_TEMPLATES) abort();
    BtechMechTemplate *chassis=&mechTemplates[mech->templateIndex];
    int16_t mountCount=(int16_t)chassis->weaponMountCount;
    for (;;)
    {
        int16_t roll=(int16_t)(uint16_t)(BtechRandomBelowBound(COMBAT_CRITICAL_SLOT_ROLL_BOUND)+1);
        int16_t budget=roll;
        if (bodyLocation==COMBAT_BODY_HEAD || bodyLocation==COMBAT_BODY_LEFT_LEG || bodyLocation==COMBAT_BODY_RIGHT_LEG) budget=(budget+1)/2;
        if (bodyLocation==COMBAT_BODY_CENTER_TORSO)
        {
            if ((int16_t)mech->engineHits<COMBAT_ENGINE_HIT_LIMIT)
            {
                budget=(int16_t)(uint16_t)(budget-(COMBAT_ENGINE_CRITICAL_WEIGHT-mech->engineHits));
                if (budget<=0)
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
                budget=(int16_t)(uint16_t)(budget-(COMBAT_GYRO_CRITICAL_WEIGHT-mech->gyroHits));
                if (budget<=0)
                {
                    if (mech==playerMechState) BtechPlaySoundIfEnabled(COMBAT_CRITICAL_SOUND,COMBAT_CRITICAL_SOUND_PRIORITY);
                    ++mech->gyroHits;return 1;
                }
            }
        }
        else if (bodyLocation>=COMBAT_BODY_LEFT_ARM && bodyLocation<=COMBAT_BODY_RIGHT_LEG)
        {
            uint16_t actuator=actuatorIndicesByBodyLocation[bodyLocation];
            if (actuator>=COMBAT_ACTUATORS) abort();
            budget=(int16_t)(uint16_t)(budget-mech->actuatorCondition[actuator]);
            if (budget<=0) {--mech->actuatorCondition[actuator];return 1;}
        }
        else if (bodyLocation==COMBAT_BODY_HEAD)
        {
            if ((int16_t)mech->lifeSupportHits<COMBAT_LIFE_SUPPORT_HIT_LIMIT)
            {
                budget=(int16_t)(uint16_t)(budget-(COMBAT_LIFE_SUPPORT_HIT_LIMIT-mech->lifeSupportHits));
                if (budget<=0)
                {
                    if (mech==playerMechState) BtechPlaySoundIfEnabled(COMBAT_CRITICAL_SOUND,COMBAT_CRITICAL_SOUND_PRIORITY);
                    ++mech->lifeSupportHits;return 1;
                }
            }
            if ((int16_t)mech->sensorHits<COMBAT_SENSORS_DESTROYED_HITS)
            {
                budget=(int16_t)(uint16_t)(budget-(COMBAT_SENSORS_DESTROYED_HITS-mech->sensorHits));
                if (budget<=0)
                {
                    if (mech==playerMechState) BtechPlaySoundIfEnabled(COMBAT_CRITICAL_SOUND,COMBAT_CRITICAL_SOUND_PRIORITY);
                    ++mech->sensorHits;return 1;
                }
            }
            if ((int16_t)mech->cockpitDisabled<COMBAT_COCKPIT_CRITICAL_WEIGHT)
            {
                budget=(int16_t)(uint16_t)(budget-(COMBAT_COCKPIT_CRITICAL_WEIGHT-mech->cockpitDisabled));
                if (budget<=0) {++mech->cockpitDisabled;return 1;}
            }
        }
        for (int16_t mount=0;mount<mountCount;++mount)
        {
            if (mount>=COMBAT_WEAPON_MOUNTS) abort();
            BtechWeaponMount *installation=&chassis->weaponMounts[mount];
            if (!mech->weapons[mount].operational || installation->bodyLocation!=(uint16_t)bodyLocation) continue;
            if ((uint16_t)installation->weaponKind>=COMBAT_WEAPON_KINDS) abort();
            budget=(int16_t)(uint16_t)(budget-weaponDefinitions[installation->weaponKind].criticalSlots);
            if (budget<=0) {mech->weapons[mount].operational=0;return 1;}
        }
        for (uint16_t family=0;family<COMBAT_AMMO_FAMILIES;++family)
        {
            if (chassis->ammoBodyLocations[family]!=bodyLocation || !mech->ammoSalvosRemaining[family]) continue;
            budget=(int16_t)(uint16_t)(budget-1);
            if (!budget) {mech->ammoSalvosRemaining[family]=0;return 1;} /* Equality, not <=; no explosion. */
        }
        for (uint16_t group=0;group<COMBAT_JET_GROUPS;++group)
        {
            if (chassis->jumpJetGroups[group].bodyLocation!=bodyLocation || !mech->jumpJetGroupOperational[group]) continue;
            budget=(int16_t)(uint16_t)(budget-chassis->jumpJetGroups[group].criticalSlots);
            if (budget<=0) {mech->jumpJetGroupOperational[group]=0;++mech->destroyedJumpJetGroups;return 1;}
        }
        if (budget==roll) return 0;
    }
}

/*0810:4EDA. Armour equality still invokes critical rolling with zero residual. */
void BtechApplyArmorAndInternalDamage(uint16_t unitSlot,int16_t armorLocation,int16_t damage)
{
    if (sideCompletionFlags[1] || ((uint16_t)musicSequences[0][0]&0xFF)) return;
    if (!unitSlot)
    {
        BtechPlaySoundIfEnabled(COMBAT_DAMAGE_SOUND,COMBAT_DAMAGE_SOUND_PRIORITY);
        playerImpactThisTick=1;
        if (playerDamageSuppressed) return;
    }
    else if (unitSlot==COMBAT_SPECIAL_TARGET_SLOT) {BtechApplySpecialTargetDamage(damage);return;}
    if (unitSlot>=COMBAT_MECH_SLOTS) abort();
    targetDamagePanelDirty[unitSlot]=1;
    BtechMechState *mech=unitBindings[unitSlot].mech;
    while (damage)
    {
        if (!mech || (uint16_t)armorLocation>=COMBAT_ARMOR_LOCATIONS) abort();
        uint16_t *armor=&mech->currentArmor[armorLocation];
        if ((int16_t)*armor>damage) {*armor-=(uint16_t)damage;damage=0;continue;}
        damage=(int16_t)(uint16_t)((uint16_t)damage-*armor);*armor=0;
        uint16_t body=armorToBodyLocations[armorLocation];
        if (body>=COMBAT_BODY_LOCATIONS) abort();
        if (!mech->currentInternalStructure[body])
        {
            if (body==COMBAT_BODY_CENTER_TORSO) damage=0;
            else armorLocation=(int16_t)damageTransferArmorLocations[body];
            continue;
        }
        BtechRollCriticalLosses(mech,(int16_t)body);
        uint16_t *internal=&mech->currentInternalStructure[body];
        if ((int16_t)*internal>damage) {*internal-=(uint16_t)damage;damage=0;continue;}
        BtechPooledWorldObject *part=BtechGetUnitRenderPartByArmorLocation(unitSlot,armorLocation);
        if (part) part->object.prefix.renderFlags|=COMBAT_WORLD_OBJECT_HIDDEN;
        BtechQueueBodyDestructionEmitter(unitSlot,armorLocation);
        damage=(int16_t)(uint16_t)((uint16_t)damage-*internal);*internal=0;
        if (BtechApplyDestroyedBodyLocationEffects(mech,(int16_t)body)) damage=0;
        else armorLocation=(int16_t)damageTransferArmorLocations[body];
    }
}

/*0810:5FA5. Armour HEAD=8 differs from internal HEAD=7. */
BtechPooledWorldObject *BtechGetUnitRenderPartByArmorLocation(uint16_t unitSlot,int16_t armorLocation)
{
    if (unitSlot>=COMBAT_MECH_SLOTS) abort();
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
    if (!specialTargetObject) abort();
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
