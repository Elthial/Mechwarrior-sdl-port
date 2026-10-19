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
    BtechPooledWorldObject *object=projectile->worldObject;
    if (!object) abort();
    const BtechPosition3D *target=BtechGetUnitBodyReferencePosition(projectile->targetUnitSlot,(int16_t)projectile->targetArmorLocation);
    BtechWorldObjectPrefix *pose=&object->object.prefix;
    uint32_t deltaA=target->positionA-pose->position.positionA,deltaB=target->positionB-pose->position.positionB;
    uint16_t heading=BtechCalculateVectorHeading(deltaA,deltaB);
    int16_t error=(int16_t)(uint16_t)(heading-pose->orientationC);
    uint16_t magnitude=error==INT16_MIN?INT16_MAX:error<0?(uint16_t)(0-(uint16_t)error):(uint16_t)error;
    uint8_t overshoot=magnitude>COMBAT_PROJECTILE_OVERSHOOT_ANGLE;
    uint16_t turn=magnitude>COMBAT_PROJECTILE_TURN_LIMIT?COMBAT_PROJECTILE_TURN_LIMIT:magnitude;
    object->orientationSteps[2]=(int16_t)(error<0?(uint16_t)(0-turn):turn);
    uint32_t distance=BtechApproximatePlanarDistance(&pose->position,target);
    uint32_t negativeHeight=0-(target->positionC-pose->position.positionC);
    uint16_t pitch=BtechCalculateVectorHeading(negativeHeight,distance);
    error=(int16_t)(uint16_t)(pitch-pose->orientationA);
    magnitude=error==INT16_MIN?INT16_MAX:error<0?(uint16_t)(0-(uint16_t)error):(uint16_t)error;
    turn=magnitude>COMBAT_PROJECTILE_TURN_LIMIT?COMBAT_PROJECTILE_TURN_LIMIT:magnitude;
    object->orientationSteps[0]=(int16_t)(error<0?(uint16_t)(0-turn):turn);
    return overshoot;
}

/*0810:5D3E. Special-target capacity is independent of Mech armour. */
uint8_t BtechTestProjectileAgainstSpecialObject(BtechProjectileState *projectile)
{
    if (!specialTargetDamageRemaining) return 0;
    if (!projectile->worldObject) abort();
    if (!BtechTestPositionInsideSpecialTarget(&projectile->worldObject->object.prefix.position,1)) return 0;
    int16_t damage=BtechCalculateProjectileImpactDamage(projectile);
    BtechApplySpecialTargetDamage(damage);return 1;
}

/*0810:5D82. AC/5 direct damage; otherwise two ordered dice choose missiles hit.
 * Only SRM hits double into damage. No extra to-hit roll is made here. */
int16_t BtechCalculateProjectileImpactDamage(const BtechProjectileState *projectile)
{
    if (projectile->profileIndex>=COMBAT_PROJECTILE_PROFILES) abort();
    uint16_t kind=projectileProfiles[projectile->profileIndex].weaponKind;
    if (kind>=COMBAT_WEAPON_KINDS) abort();
    if (kind==COMBAT_WEAPON_AC5) return (int16_t)weaponDefinitions[kind].damageOrMissileCount;
    int16_t row=missileClusterRowByWeapon[kind];
    if ((uint16_t)row>=COMBAT_MISSILE_CLUSTER_ROWS) abort();
    uint16_t firstDie=BtechRandomBelowBound(6);
    uint16_t secondDie=BtechRandomBelowBound(6);
    uint16_t damage=missileClusterRows[row][firstDie+secondDie];
    if (kind!=COMBAT_WEAPON_LRM5) damage<<=1;
    return (int16_t)damage;
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
        if ((uint16_t)weaponKind>=COMBAT_WEAPON_KINDS) abort();
        projectile->profileIndex=projectileProfileIndicesByWeapon[weaponKind];
        if (projectile->profileIndex>=COMBAT_PROJECTILE_PROFILES) abort();
        const BtechProjectileProfile *profile=&projectileProfiles[projectile->profileIndex];
        projectile->lifetimeRemaining=profile->lifetimeTicks;
        uint16_t resource=profile->resourceIdDelta+otherShapeResourceBase;
        projectile->worldObject=BtechAllocatePooledWorldObject(resource,placement);
        if (!projectile->worldObject || projectile->profileIndex>=COMBAT_PROJECTILE_PROFILES) abort();
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
    if (unitSlot>=COMBAT_MECH_SLOTS) abort();
    BtechPooledWorldObject *torso=unitBindings[unitSlot].torso,*root=unitBindings[unitSlot].positionedLeftLeg;
    BtechProjectileState *projectile=BtechAllocateProjectileFromWeaponProfile(weaponKind,placement);
    if (projectile)
    {
        projectile->firingUnitSlot=unitSlot;
        BtechWorldObjectPrefix *pose=&projectile->worldObject->object.prefix;
        BtechMechState *mech=unitBindings[unitSlot].mech;
        if (!root || !torso || !mech) abort();
        pose->orientationA=root->object.prefix.orientationA-mech->aimPitch;
        if (unitControls[unitSlot].movementMode==MOVEMENT_MODE_FALLEN_POSE) pose->orientationA+=COMBAT_PROJECTILE_FALLEN_PITCH_BIAS;
        pose->orientationB=root->object.prefix.orientationB;
        pose->orientationC=root->object.prefix.orientationC+torso->object.prefix.orientationC;
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
    if (!playerMechTemplate || mountIndex>=COMBAT_WEAPON_MOUNTS) abort();
    const BtechWeaponMount *mount=&playerMechTemplate->weaponMounts[mountIndex];
    if (mount->bodyLocation>=COMBAT_BODY_LOCATIONS) abort();
    BtechPosition3D *position=BtechGetUnitBodyReferencePosition(0,(int16_t)bodyLocationReferenceIndices[mount->bodyLocation]);
    BtechProjectileState *projectile=BtechSpawnUnitWeaponProjectile(0,mount->weaponKind,position);
    if (!projectile) return; /* No projectile means no guidance target or fabricated hit. */
    uint16_t target,armor;BtechMapSelectedRenderPartToArmorLocation(&armor,&target);
    if (target)
    {
        projectile->targetUnitSlot=target;projectile->targetArmorLocation=armor;
    }
}

/*0810:5EF1. Spawn from the live position before release; only handle clears. */
void BtechDestroyProjectileAndSpawnEffect(BtechProjectileState *projectile)
{
    if (!projectile->worldObject) abort();
    BtechSpawnSequencedWorldEffect(&projectile->worldObject->object.prefix.position,COMBAT_IMPACT_EFFECT_SEQUENCE);
    BtechReleasePooledWorldObject(projectile->worldObject);
    projectile->worldObject=NULL;
}
