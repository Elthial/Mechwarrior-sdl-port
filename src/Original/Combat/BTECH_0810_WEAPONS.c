#include "mechwarrior/port_diagnostics.h"
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
    BtechMechState *playerMech=unitBindings[0].mech;
    if (!playerMech || playerMech->templateIndex>=COMBAT_MECH_TEMPLATES || mountIndex>=COMBAT_WEAPON_MOUNTS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !playerMech || playerMech->templateIndex>=COMBAT_MECH_TEMPLATES || mountIndex>=COMBAT_WEAPON_MOUNTS");
    const BtechWeaponMount *weaponMount=&mechTemplates[playerMech->templateIndex].weaponMounts[mountIndex];
    if ((uint16_t)weaponMount->weaponKind>=COMBAT_WEAPON_KINDS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: (uint16_t)weaponMount->weaponKind>=COMBAT_WEAPON_KINDS");
    const BtechWeaponDefinition *weaponDefinition=&weaponDefinitions[weaponMount->weaponKind];
    int32_t shotDistanceWorld;
    uint8_t spawnImpactEffect=0;
    if (!selectedHitObject)
    {
        if (!playerPoseObject || !playerMechState) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !playerPoseObject || !playerMechState");
        int16_t shotPitch=playerPoseObject->object.prefix.orientationA-playerMechState->aimPitch;
        shotDistanceWorld=(int16_t)((uint16_t)(weaponDefinition->longRangeBoundary<<DIRECT_SHOT_RANGE_SHIFT)*DIRECT_SHOT_RANGE_MULTIPLIER);
        if (shotPitch<0)
        {
            int16_t downwardAimSine=0-BtechLookupSine((uint16_t)shotPitch);
            if (downwardAimSine)
            {
                if (!playerUnitBinding || !playerUnitBinding->positionedLeftLeg) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !playerUnitBinding || !playerUnitBinding->positionedLeftLeg");
                uint32_t scaledPlayerAltitude=playerUnitBinding->positionedLeftLeg->object.prefix.position.positionC<<ORIENTATION_FRACTION_BITS;
                int16_t groundIntersectionDistance=(int16_t)BtechDivideSignedDword(scaledPlayerAltitude,(uint32_t)downwardAimSine);
                if ((int32_t)groundIntersectionDistance<shotDistanceWorld)
                {BtechBuildPlayerDirectShotGeometry(groundIntersectionDistance,weaponMount,1);return;}
            }
        }
    }
    else
    {
        if (!playerUnitBinding || !playerUnitBinding->torso) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !playerUnitBinding || !playerUnitBinding->torso");
        shotDistanceWorld=(int32_t)BtechApproximatePlanarDistance(&playerUnitBinding->torso->object.prefix.position,&selectedHitObject->position);
        uint32_t shiftedTargetDistance=BtechShiftSignedDwordRight((uint32_t)shotDistanceWorld,DIRECT_SHOT_RANGE_SHIFT);
        int16_t targetRangeUnits=(int16_t)BtechDivideSignedDword(shiftedTargetDistance,DIRECT_SHOT_RANGE_MULTIPLIER);
        const uint16_t rangeBoundaries[]={weaponDefinition->minimumRange,weaponDefinition->shortRangeBoundary,weaponDefinition->mediumRangeBoundary,weaponDefinition->longRangeBoundary};
        uint16_t rangeBand=0;
        while (rangeBand<WEAPON_RANGE_BOUNDARIES && (int16_t)rangeBoundaries[rangeBand]<targetRangeUnits) ++rangeBand;
        if (rangeBand==WEAPON_RANGE_BEYOND_LONG)
            shotDistanceWorld=(int16_t)((uint16_t)(weaponDefinitions[weaponMount->weaponKind].longRangeBoundary<<DIRECT_SHOT_RANGE_SHIFT)*DIRECT_SHOT_RANGE_MULTIPLIER);
        else
        {
            uint16_t targetUnitSlot,targetArmorLocation;
            BtechMapSelectedRenderPartToArmorLocation(&targetArmorLocation,&targetUnitSlot);
            if (!targetUnitSlot)
            {
                uint16_t projectileSlot=0;
                for (;projectileSlot<COMBAT_PROJECTILE_SLOTS;++projectileSlot)
                {
                    BtechPooledWorldObject *projectileObject=projectiles[projectileSlot].worldObject;
                    if (projectileObject && &projectileObject->object.prefix==selectedHitObject)
                    {BtechDestroyProjectileAndSpawnEffect(&projectiles[projectileSlot]);break;}
                }
                /* Native reloads selected object AFTER projectile destruction. */
                if (specialTargetObject && &specialTargetObject->object.prefix==selectedHitObject)
                    BtechApplySpecialTargetDamage((int16_t)weaponDefinitions[weaponMount->weaponKind].damageOrMissileCount);
                spawnImpactEffect=projectileSlot==COMBAT_PROJECTILE_SLOTS;
            }
            else
            {
                unitControls[0].lastDirectHitUnitSlot=(int16_t)targetUnitSlot;
                BtechPlaySoundIfEnabled(DIRECT_HIT_SOUND,DIRECT_HIT_SOUND_PRIORITY);
                BtechApplyArmorAndInternalDamage(targetUnitSlot,(int16_t)targetArmorLocation,(int16_t)weaponDefinitions[weaponMount->weaponKind].damageOrMissileCount);
                spawnImpactEffect=1;
            }
        }
    }
    BtechBuildPlayerDirectShotGeometry((int16_t)shotDistanceWorld,weaponMount,spawnImpactEffect);
}

/*0810:4803. Armour output precedes unit output. Slot0 means no other Mech. */
void BtechMapSelectedRenderPartToArmorLocation(uint16_t *armorLocation,uint16_t *unitSlot)
{
    uint16_t hitArmorLocation=COMBAT_ARMOR_LEFT_ARM,matchedUnitSlot=0;
    if (selectedHitObject)
        for (uint16_t candidateUnitSlot=1;candidateUnitSlot<COMBAT_MECH_SLOTS;++candidateUnitSlot)
        {
            BtechUnitBinding *targetBinding=&unitBindings[candidateUnitSlot];
            if (!targetBinding->mech) continue;
            BtechWorldObjectPrefix *hitPart=selectedHitObject;
            if (targetBinding->leftArm && &targetBinding->leftArm->object.prefix==hitPart) hitArmorLocation=COMBAT_ARMOR_LEFT_ARM;
            else if (targetBinding->rightArm && &targetBinding->rightArm->object.prefix==hitPart) hitArmorLocation=COMBAT_ARMOR_RIGHT_ARM;
            else if (targetBinding->positionedLeftLeg && &targetBinding->positionedLeftLeg->object.prefix==hitPart) hitArmorLocation=COMBAT_ARMOR_LEFT_LEG;
            else if (targetBinding->rightLeg && &targetBinding->rightLeg->object.prefix==hitPart) hitArmorLocation=COMBAT_ARMOR_RIGHT_LEG;
            else if (targetBinding->torso && &targetBinding->torso->object.prefix==hitPart)
            {
                if (!playerUnitBinding || !playerUnitBinding->positionedLeftLeg || !playerUnitBinding->torso || !targetBinding->positionedLeftLeg) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !playerUnitBinding || !playerUnitBinding->positionedLeftLeg || !playerUnitBinding->torso || !targetBinding->positionedLeftLeg");
                uint16_t relativeTorsoHeading=playerUnitBinding->positionedLeftLeg->object.prefix.orientationC;
                relativeTorsoHeading-=targetBinding->positionedLeftLeg->object.prefix.orientationC;
                relativeTorsoHeading-=targetBinding->torso->object.prefix.orientationC;
                relativeTorsoHeading+=playerUnitBinding->torso->object.prefix.orientationC;
                relativeTorsoHeading+=DIRECT_TORSO_HEADING_BIAS;
                /* Native impossible unsigned front interval is intentional. */
                if (relativeTorsoHeading<DIRECT_TORSO_FRONT_LOWER && relativeTorsoHeading>DIRECT_TORSO_FRONT_UPPER) hitArmorLocation=COMBAT_ARMOR_CENTER_TORSO;
                else if (relativeTorsoHeading<DIRECT_TORSO_LEFT_LIMIT) hitArmorLocation=COMBAT_ARMOR_LEFT_TORSO;
                else if (relativeTorsoHeading<DIRECT_TORSO_REAR_LIMIT) hitArmorLocation=COMBAT_ARMOR_REAR_CENTER_TORSO;
                else hitArmorLocation=COMBAT_ARMOR_RIGHT_TORSO;
            }
            else if (targetBinding->head && &targetBinding->head->object.prefix==hitPart) hitArmorLocation=COMBAT_ARMOR_HEAD;
            else continue;
            matchedUnitSlot=candidateUnitSlot;break;
        }
    *armorLocation=hitArmorLocation;*unitSlot=matchedUnitSlot;
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
    if ((uint16_t)family==COMBAT_AMMO_NONE) return 1;
    if ((uint16_t)family>=COMBAT_AMMO_FAMILIES) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: (uint16_t)family>=COMBAT_AMMO_FAMILIES");
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
                if (!unitBindings[row].positionedLeftLeg || !unitBindings[column].positionedLeftLeg) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !unitBindings[row].positionedLeftLeg || !unitBindings[column].positionedLeftLeg");
                distance=BtechApproximatePlanarDistance(&unitBindings[row].positionedLeftLeg->object.prefix.position,
                    &unitBindings[column].positionedLeftLeg->object.prefix.position);
            }
            unitDistances[row][column]=distance;unitDistances[column][row]=distance;
        }
}
