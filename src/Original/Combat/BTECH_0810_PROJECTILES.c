#include "mechwarrior/port_diagnostics.h"
#include "mechwarrior/combat_projectiles.h"
#include "mechwarrior/combat_effects.h"
#include "mechwarrior/combat_ballistics.h"
#include "mechwarrior/combat_pose.h"
#include "mechwarrior/combat_video.h"
#include "mechwarrior/combat_scene.h"
#include "mechwarrior/combat_movement.h"
#include "mechwarrior/combat_damage.h"
#include "mechwarrior/combat_sound.h"
#include "mechwarrior/combat_surface.h"
#include <stdlib.h>

/*0810:5660. Guidance precedes each of five real motion substeps. Impact restores
 * the pre-step position BEFORE spawning effects; expiry spawns no effect. */
void BtechUpdateProjectiles(void)
{
    for (uint16_t slot=0;slot<COMBAT_PROJECTILE_SLOTS;++slot)
    {
        BtechProjectileState *projectile=&projectiles[slot];
        if (!projectile->worldObject) continue;
        --projectile->lifetimeRemaining;
        if (!projectile->lifetimeRemaining)
        {BtechReleasePooledWorldObject(projectile->worldObject);projectile->worldObject=NULL;continue;}
        for (uint16_t substep=0;substep<COMBAT_PROJECTILE_MOVE_SUBSTEPS;++substep)
        {
            uint8_t overshoot=BtechSteerProjectileAndTestOvershoot(projectile);
            BtechPosition3D previousPosition=projectile->worldObject->object.prefix.position;
            BtechAdvancePooledObjectMotion(projectile->worldObject);
            BtechPooledWorldObject *object=projectile->worldObject;
            if (overshoot)
            {
                int16_t damage=BtechCalculateProjectileImpactDamage(projectile);
                if (projectile->targetUnitSlot) BtechPlaySoundIfEnabled(DIRECT_HIT_SOUND,DIRECT_HIT_SOUND_PRIORITY);
                BtechApplyArmorAndInternalDamage(projectile->targetUnitSlot,(int16_t)projectile->targetArmorLocation,damage);
                goto impact;
            }
            if (BtechTestProjectileAgainstSpecialObject(projectile)) goto impact;
            uint32_t height=0;
            if ((uint8_t)BtechQueryWorldSurface(&projectile->worldObject->surfaceQuery,&object->object.prefix.position,selectedWorldObjects,(int16_t)selectedObjectCount))
                height=object->surfaceQuery.surfacePositionC;
            if ((int32_t)height<(int32_t)object->object.prefix.position.positionC) continue;
impact:
            projectile->worldObject->object.prefix.position=previousPosition;
            BtechDestroyProjectileAndSpawnEffect(projectile);
            break;
        }
    }
}

/*0810:5797. Overshoot is yaw error only, not proximity or a physical hit test. */
uint8_t BtechSteerProjectileAndTestOvershoot(BtechProjectileState *projectile)
{
    if (projectile->targetUnitSlot==COMBAT_PROJECTILE_NO_TARGET) return 0;
    BtechPooledWorldObject *projectileObject=projectile->worldObject;
    if (!projectileObject) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !projectileObject");
    const BtechPosition3D *targetPosition=BtechGetUnitBodyReferencePosition(projectile->targetUnitSlot,(int16_t)projectile->targetArmorLocation);
    BtechWorldObjectPrefix *projectilePose=&projectileObject->object.prefix;
    uint32_t targetDeltaA=targetPosition->positionA-projectilePose->position.positionA,targetDeltaB=targetPosition->positionB-projectilePose->position.positionB;
    uint16_t targetHeading=BtechCalculateVectorHeading(targetDeltaA,targetDeltaB);
    int16_t yawError=targetHeading-projectilePose->orientationC;
    uint16_t yawErrorMagnitude=yawError==INT16_MIN?INT16_MAX:yawError<0?(uint16_t)(0-(uint16_t)yawError):(uint16_t)yawError;
    uint8_t overshotTarget=yawErrorMagnitude>COMBAT_PROJECTILE_OVERSHOOT_ANGLE;
    uint16_t yawTurnStep=yawErrorMagnitude>COMBAT_PROJECTILE_TURN_LIMIT?COMBAT_PROJECTILE_TURN_LIMIT:yawErrorMagnitude;
    projectileObject->orientationSteps[2]=(int16_t)(yawError<0?(uint16_t)(0-yawTurnStep):yawTurnStep);
    uint32_t targetPlanarDistance=BtechApproximatePlanarDistance(&projectilePose->position,targetPosition);
    uint32_t negativeAltitudeDelta=0-(targetPosition->positionC-projectilePose->position.positionC);
    uint16_t targetPitch=BtechCalculateVectorHeading(negativeAltitudeDelta,targetPlanarDistance);
    int16_t pitchError=(int16_t)(targetPitch-projectilePose->orientationA);
    uint16_t pitchErrorMagnitude=pitchError==INT16_MIN?INT16_MAX:pitchError<0?(uint16_t)(0-(uint16_t)pitchError):(uint16_t)pitchError;
    uint16_t pitchTurnStep=pitchErrorMagnitude>COMBAT_PROJECTILE_TURN_LIMIT?COMBAT_PROJECTILE_TURN_LIMIT:pitchErrorMagnitude;
    projectileObject->orientationSteps[0]=(int16_t)(pitchError<0?(uint16_t)(0-pitchTurnStep):pitchTurnStep);
    return overshotTarget;
}

/*0810:5D3E. Special-target capacity is independent of Mech armour. */
uint8_t BtechTestProjectileAgainstSpecialObject(BtechProjectileState *projectile)
{
    if (!specialTargetDamageRemaining) return 0;
    if (!projectile->worldObject) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !projectile->worldObject");
    if (!BtechTestPositionInsideSpecialTarget(&projectile->worldObject->object.prefix.position,1)) return 0;
    int16_t damage=BtechCalculateProjectileImpactDamage(projectile);
    BtechApplySpecialTargetDamage(damage);return 1;
}

/*0810:5D82. AC/5 direct damage; otherwise two ordered dice choose missiles hit.
 * Only SRM hits double into damage. No extra to-hit roll is made here. */
int16_t BtechCalculateProjectileImpactDamage(const BtechProjectileState *projectile)
{
    if (projectile->profileIndex>=COMBAT_PROJECTILE_PROFILES) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: projectile->profileIndex>=COMBAT_PROJECTILE_PROFILES");
    uint16_t weaponKind=projectileProfiles[projectile->profileIndex].weaponKind;
    if (weaponKind>=COMBAT_WEAPON_KINDS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: weaponKind>=COMBAT_WEAPON_KINDS");
    if (weaponKind==COMBAT_WEAPON_AC5) return (int16_t)weaponDefinitions[weaponKind].damageOrMissileCount;
    int16_t clusterRow=missileClusterRowByWeapon[weaponKind];
    if ((uint16_t)clusterRow>=COMBAT_MISSILE_CLUSTER_ROWS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: (uint16_t)clusterRow>=COMBAT_MISSILE_CLUSTER_ROWS");
    uint16_t firstDie=BtechRandomBelowBound(DICE_1D6);
    uint16_t secondDie=BtechRandomBelowBound(DICE_1D6);
    uint16_t missilesHit=missileClusterRows[clusterRow][firstDie+secondDie];
    if (weaponKind!=COMBAT_WEAPON_LRM5) return (int16_t)(missilesHit<<COMBAT_SRM_DAMAGE_SHIFT);
    return (int16_t)missilesHit;
}

/*0810:59A4. Record allocation is separate from pooled-object allocation. */
BtechProjectileState *BtechAllocateProjectileFromWeaponProfile(int16_t weaponKind,const BtechPosition3D *placement)
{
    uint16_t freeObject=0;
    while (freeObject<COMBAT_POOLED_OBJECTS && pooledWorldObjects[freeObject].occupied) ++freeObject;
    if (freeObject==COMBAT_POOLED_OBJECTS || worldObjectCount>=COMBAT_WORLD_OBJECTS
        || cockpitWorldObjectCount>=COMBAT_WORLD_OBJECTS) return NULL;
    for (uint16_t slot=0;slot<COMBAT_PROJECTILE_SLOTS;++slot)
    {
        BtechProjectileState *projectile=&projectiles[slot];
        if (projectile->worldObject) continue;
        if ((uint16_t)weaponKind>=COMBAT_WEAPON_KINDS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: (uint16_t)weaponKind>=COMBAT_WEAPON_KINDS");
        projectile->profileIndex=projectileProfileIndicesByWeapon[weaponKind];
        if (projectile->profileIndex>=COMBAT_PROJECTILE_PROFILES) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: projectile->profileIndex>=COMBAT_PROJECTILE_PROFILES");
        const BtechProjectileProfile *profile=&projectileProfiles[projectile->profileIndex];
        projectile->lifetimeRemaining=profile->lifetimeTicks;
        uint16_t resource=profile->resourceIdDelta+otherShapeResourceBase;
        projectile->worldObject=BtechAllocatePooledWorldObject(resource,placement);
        if (!projectile->worldObject || projectile->profileIndex>=COMBAT_PROJECTILE_PROFILES) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !projectile->worldObject || projectile->profileIndex>=COMBAT_PROJECTILE_PROFILES");
        /* Original reload AFTER allocation, before speed write. */
        projectile->worldObject->forwardStepScale=(int16_t)projectileProfiles[projectile->profileIndex].speedScale;
        projectile->worldObject->object.prefix.renderFlags=0;
        return projectile;
    }
    return NULL;
}

/*0810:58D9. Save part references before allocation; aim follows live Mech. */
BtechProjectileState *BtechSpawnUnitWeaponProjectile(uint16_t unitSlot,int16_t weaponKind,const BtechPosition3D *placement)
{
    if (unitSlot>=COMBAT_MECH_SLOTS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: unitSlot>=COMBAT_MECH_SLOTS");
    BtechPooledWorldObject *firingTorsoObject=unitBindings[unitSlot].torso,*firingLegObject=unitBindings[unitSlot].positionedLeftLeg;
    BtechProjectileState *projectile=BtechAllocateProjectileFromWeaponProfile(weaponKind,placement);
    if (projectile)
    {
        projectile->firingUnitSlot=unitSlot;
        BtechWorldObjectPrefix *projectilePose=&projectile->worldObject->object.prefix;
        BtechMechState *firingMech=unitBindings[unitSlot].mech;
        if (!firingLegObject || !firingTorsoObject || !firingMech) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !firingLegObject || !firingTorsoObject || !firingMech");
        projectilePose->orientationA=firingLegObject->object.prefix.orientationA-firingMech->aimPitch;
        if (unitControls[unitSlot].movementMode==MOVEMENT_MODE_FALLEN_POSE) projectilePose->orientationA+=COMBAT_PROJECTILE_FALLEN_PITCH_BIAS;
        projectilePose->orientationB=firingLegObject->object.prefix.orientationB;
        projectilePose->orientationC=firingLegObject->object.prefix.orientationC+firingTorsoObject->object.prefix.orientationC;
        projectile->worldObject->orientationSteps[0]=0;
        projectile->worldObject->orientationSteps[1]=COMBAT_PROJECTILE_ROLL_STEP;
        projectile->worldObject->orientationSteps[2]=0;
        projectile->targetUnitSlot=COMBAT_PROJECTILE_NO_TARGET;
    }
    return projectile;
}

/*0810:5A35. Clear only handles; caller owns world-pool teardown. */
void BtechResetProjectileHandles(void)
{
    for (uint16_t slot=0;slot<COMBAT_PROJECTILE_SLOTS;++slot) projectiles[slot].worldObject=NULL;
}

/*0810:5A55. Body reference supplies origin; selected render part supplies target. */
void BtechFirePlayerWeaponProjectile(uint16_t mountIndex)
{
    if (!playerMechTemplate || mountIndex>=COMBAT_WEAPON_MOUNTS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !playerMechTemplate || mountIndex>=COMBAT_WEAPON_MOUNTS");
    const BtechWeaponMount *weaponMount=&playerMechTemplate->weaponMounts[mountIndex];
    if (weaponMount->bodyLocation>=COMBAT_BODY_LOCATIONS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: weaponMount->bodyLocation>=COMBAT_BODY_LOCATIONS");
    BtechPosition3D *launchPosition=BtechGetUnitBodyReferencePosition(0,(int16_t)bodyLocationReferenceIndices[weaponMount->bodyLocation]);
    BtechProjectileState *projectile=BtechSpawnUnitWeaponProjectile(0,weaponMount->weaponKind,launchPosition);
    if (!projectile) return; /* No projectile means no guidance target or fabricated hit. */
    uint16_t targetUnitSlot,targetArmorLocation;BtechMapSelectedRenderPartToArmorLocation(&targetArmorLocation,&targetUnitSlot);
    if (targetUnitSlot)
    {
        projectile->targetUnitSlot=targetUnitSlot;projectile->targetArmorLocation=targetArmorLocation;
    }
}

/*0810:5EF1. Spawn from the live position before release; only handle clears. */
void BtechDestroyProjectileAndSpawnEffect(BtechProjectileState *projectile)
{
    if (!projectile->worldObject) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !projectile->worldObject");
    BtechSpawnSequencedWorldEffect(&projectile->worldObject->object.prefix.position,COMBAT_IMPACT_EFFECT_SEQUENCE);
    BtechReleasePooledWorldObject(projectile->worldObject);
    projectile->worldObject=NULL;
}
