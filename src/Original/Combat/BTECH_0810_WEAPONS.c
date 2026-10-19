#include "mechwarrior/combat_ballistics.h"
#include "mechwarrior/combat_startup.h"
#include "mechwarrior/combat_render.h"
#include "mechwarrior/combat_scene.h"
#include "mechwarrior/combat_damage.h"
#include "mechwarrior/combat_sound.h"
#include "mechwarrior/combat_runtime.h"
#include "mechwarrior/combat_projectiles.h"
#include <stdlib.h>

/*0810:461E. Accounting belongs to099A; do not debit heat/ammunition twice. */
void BtechFirePlayerDirectWeapon(uint16_t mountIndex)
{
    BtechMechState *player=unitBindings[0].mech;
    if (!player || player->templateIndex>=COMBAT_MECH_TEMPLATES || mountIndex>=COMBAT_WEAPON_MOUNTS) abort();
    const BtechWeaponMount *mount=&mechTemplates[player->templateIndex].weaponMounts[mountIndex];
    if ((uint16_t)mount->weaponKind>=COMBAT_WEAPON_KINDS) abort();
    const BtechWeaponDefinition *weapon=&weaponDefinitions[mount->weaponKind];
    int32_t distance;
    uint8_t impactEffect=0;
    if (!selectedHitObject)
    {
        if (!playerPoseObject || !playerMechState) abort();
        int16_t pitch=(int16_t)(uint16_t)(playerPoseObject->object.prefix.orientationA-playerMechState->aimPitch);
        distance=(int16_t)(uint16_t)((uint16_t)(weapon->longRangeBoundary<<DIRECT_SHOT_RANGE_SHIFT)*DIRECT_SHOT_RANGE_MULTIPLIER);
        if (pitch<0)
        {
            int16_t negativeSine=(int16_t)(uint16_t)(0-BtechLookupSine((uint16_t)pitch));
            if (negativeSine)
            {
                if (!playerUnitBinding || !playerUnitBinding->positionedLeftLeg) abort();
                uint32_t numerator=playerUnitBinding->positionedLeftLeg->object.prefix.position.positionC<<ORIENTATION_FRACTION_BITS;
                int16_t groundDistance=(int16_t)(uint16_t)BtechDivideSignedDword(numerator,(uint32_t)(int32_t)negativeSine);
                if ((int32_t)groundDistance<distance)
                {BtechBuildPlayerDirectShotGeometry(groundDistance,mount,1);return;}
            }
        }
    }
    else
    {
        if (!playerUnitBinding || !playerUnitBinding->torso) abort();
        distance=(int32_t)BtechApproximatePlanarDistance(&playerUnitBinding->torso->object.prefix.position,&selectedHitObject->position);
        uint32_t scaled=BtechShiftSignedDwordRight((uint32_t)distance,DIRECT_SHOT_RANGE_SHIFT);
        int16_t range=(int16_t)(uint16_t)BtechDivideSignedDword(scaled,DIRECT_SHOT_RANGE_MULTIPLIER);
        const uint16_t boundaries[]={weapon->minimumRange,weapon->shortRangeBoundary,weapon->mediumRangeBoundary,weapon->longRangeBoundary};
        uint16_t boundary=0;
        while (boundary<4 && (int16_t)boundaries[boundary]<range) ++boundary;
        if (boundary==4)
            distance=(int16_t)(uint16_t)((uint16_t)(weaponDefinitions[mount->weaponKind].longRangeBoundary<<DIRECT_SHOT_RANGE_SHIFT)*DIRECT_SHOT_RANGE_MULTIPLIER);
        else
        {
            uint16_t targetUnit,targetArmor;
            BtechMapSelectedRenderPartToArmorLocation(&targetArmor,&targetUnit);
            if (!targetUnit)
            {
                uint16_t projectile=0;
                for (;projectile<COMBAT_PROJECTILE_SLOTS;++projectile)
                {
                    BtechPooledWorldObject *object=projectiles[projectile].worldObject;
                    if (object && &object->object.prefix==selectedHitObject)
                    {BtechDestroyProjectileAndSpawnEffect(&projectiles[projectile]);break;}
                }
                /* Native reloads selected object AFTER projectile destruction. */
                if (specialTargetObject && &specialTargetObject->object.prefix==selectedHitObject)
                    BtechApplySpecialTargetDamage((int16_t)weaponDefinitions[mount->weaponKind].damageOrMissileCount);
                impactEffect=projectile==COMBAT_PROJECTILE_SLOTS;
            }
            else
            {
                unitControls[0].lastDirectHitUnitSlot=(int16_t)targetUnit;
                BtechPlaySoundIfEnabled(DIRECT_HIT_SOUND,DIRECT_HIT_SOUND_PRIORITY);
                BtechApplyArmorAndInternalDamage(targetUnit,(int16_t)targetArmor,(int16_t)weaponDefinitions[mount->weaponKind].damageOrMissileCount);
                impactEffect=1;
            }
        }
    }
    BtechBuildPlayerDirectShotGeometry((int16_t)(uint16_t)distance,mount,impactEffect);
}

/*0810:4803. Armour output precedes unit output. Slot0 means no other Mech. */
void BtechMapSelectedRenderPartToArmorLocation(uint16_t *armorLocation,uint16_t *unitSlot)
{
    uint16_t armor=COMBAT_ARMOR_LEFT_ARM,matched=0;
    if (selectedHitObject)
        for (uint16_t slot=1;slot<COMBAT_MECH_SLOTS;++slot)
        {
            BtechUnitBinding *binding=&unitBindings[slot];
            if (!binding->mech) continue;
            BtechWorldObjectPrefix *selected=selectedHitObject;
            if (binding->leftArm && &binding->leftArm->object.prefix==selected) armor=COMBAT_ARMOR_LEFT_ARM;
            else if (binding->rightArm && &binding->rightArm->object.prefix==selected) armor=COMBAT_ARMOR_RIGHT_ARM;
            else if (binding->positionedLeftLeg && &binding->positionedLeftLeg->object.prefix==selected) armor=COMBAT_ARMOR_LEFT_LEG;
            else if (binding->rightLeg && &binding->rightLeg->object.prefix==selected) armor=COMBAT_ARMOR_RIGHT_LEG;
            else if (binding->torso && &binding->torso->object.prefix==selected)
            {
                if (!playerUnitBinding || !playerUnitBinding->positionedLeftLeg || !playerUnitBinding->torso || !binding->positionedLeftLeg) abort();
                uint16_t heading=playerUnitBinding->positionedLeftLeg->object.prefix.orientationC;
                heading-=binding->positionedLeftLeg->object.prefix.orientationC;
                heading-=binding->torso->object.prefix.orientationC;
                heading+=playerUnitBinding->torso->object.prefix.orientationC;
                heading+=DIRECT_TORSO_HEADING_BIAS;
                /* Native impossible unsigned front interval is intentional. */
                if (heading<DIRECT_TORSO_FRONT_LOWER && heading>DIRECT_TORSO_FRONT_UPPER) armor=COMBAT_ARMOR_CENTER_TORSO;
                else if (heading<DIRECT_TORSO_LEFT_LIMIT) armor=COMBAT_ARMOR_LEFT_TORSO;
                else if (heading<DIRECT_TORSO_REAR_LIMIT) armor=COMBAT_ARMOR_REAR_CENTER_TORSO;
                else armor=COMBAT_ARMOR_RIGHT_TORSO;
            }
            else if (binding->head && &binding->head->object.prefix==selected) armor=COMBAT_ARMOR_HEAD;
            else continue;
            matched=slot;break;
        }
    *armorLocation=armor;*unitSlot=matched;
}

/* Only installed, operational weapons with ammunition can be selected. */
uint8_t BtechIsPlayerWeaponMountReady(uint16_t mountIndex)
{
    if (!playerMechTemplate || !playerMechState || mountIndex>=COMBAT_WEAPON_MOUNTS
        || mountIndex>=playerMechTemplate->weaponMountCount) return 0;
    int16_t kind=playerMechTemplate->weaponMounts[mountIndex].weaponKind;
    if ((uint16_t)kind>=COMBAT_WEAPON_KINDS) return 0;
    if (!playerMechState->weapons[mountIndex].operational
        || playerMechState->weapons[mountIndex].reloadTicksRemaining) return 0;
    int16_t family=weaponDefinitions[kind].ammoFamily;
    if ((uint16_t)family==COMBAT_WEAPON_NONE) return 1;
    if ((uint16_t)family>=COMBAT_AMMO_FAMILIES) abort();
    return playerMechState->ammoSalvosRemaining[family]!=0;
}

/* Advance through the installed weapon mounts, optionally wrapping. */
uint16_t BtechFindNextReadyPlayerWeaponMount(uint16_t currentMount,uint16_t allowWrap)
{
    if (!playerMechTemplate || !playerMechTemplate->weaponMountCount) return 0;
    if (currentMount>=playerMechTemplate->weaponMountCount) currentMount=playerMechTemplate->weaponMountCount-1;
    uint16_t candidate=currentMount;
    do
    {
        ++candidate;
        if (candidate>=playerMechTemplate->weaponMountCount) candidate=allowWrap?0:currentMount;
    } while (!BtechIsPlayerWeaponMountReady(candidate) && candidate!=currentMount);
    return candidate;
}

/*0810:450E. Exhaustion may return the original unready mount. */
uint16_t BtechFindPreviousReadyPlayerWeaponMount(uint16_t currentMount,uint16_t allowWrap)
{
    if (!playerMechTemplate || !playerMechTemplate->weaponMountCount) return 0;
    if (currentMount>=playerMechTemplate->weaponMountCount) currentMount=0;
    uint16_t candidate=currentMount;
    do
    {
        --candidate;
        if ((int16_t)candidate<0) candidate=(uint8_t)allowWrap?playerMechTemplate->weaponMountCount-1:currentMount;
    } while (!BtechIsPlayerWeaponMountReady(candidate) && candidate!=currentMount);
    return candidate;
}

/*0810:4551. Group mode advances selection; it does not fire an alpha strike. */
void BtechAdvancePlayerGroupedWeaponSelection(void)
{
    if (!playerMechTemplate || !playerMechTemplate->weaponMountCount) return;
    if (selectedWeaponMount>=playerMechTemplate->weaponMountCount) selectedWeaponMount=0;
    if (BtechIsPlayerWeaponMountReady(selectedWeaponMount) || !mountGroupMode) return;
    uint16_t candidate=selectedWeaponMount;
    do
    {
        ++candidate;
        if (candidate>=playerMechTemplate->weaponMountCount) candidate=0;
        if (BtechIsPlayerWeaponMountReady(candidate))
        {
            if (playerMountFireFlags[candidate]) break;
        }
    } while (candidate!=selectedWeaponMount);
    selectedWeaponMount=candidate;
}

/*0810:4AC1. Only present side-zero rows write; same-side pairs become FFFFFFFF.
 * Absent slots/side-one-only cells retain their old values. */
void BtechRefreshUnitDistanceMatrix(void)
{
    for (uint16_t row=0;row<COMBAT_MECH_SLOTS;++row)
        for (uint16_t column=0;column<COMBAT_MECH_SLOTS;++column)
        {
            if (!unitBindings[column].mech || !unitBindings[row].mech || unitControls[row].sideIndex) continue;
            uint32_t distance=UINT32_MAX;
            if (unitControls[column].sideIndex==1)
            {
                if (!unitBindings[row].positionedLeftLeg || !unitBindings[column].positionedLeftLeg) abort();
                distance=BtechApproximatePlanarDistance(&unitBindings[row].positionedLeftLeg->object.prefix.position,
                    &unitBindings[column].positionedLeftLeg->object.prefix.position);
            }
            unitDistances[row][column]=distance;unitDistances[column][row]=distance;
        }
}
