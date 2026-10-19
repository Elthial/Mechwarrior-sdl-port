#include "mechwarrior/combat_ballistics.h"
#include "mechwarrior/combat_damage.h"
#include "mechwarrior/combat_effects.h"
#include "mechwarrior/combat_projectiles.h"
#include "mechwarrior/combat_pose.h"
#include "mechwarrior/combat_import.h"
#include "mechwarrior/combat_render.h"
#include "mechwarrior/combat_sound.h"
#include <stdlib.h>

/*0810:43A8. Opposing units use the cached matrix. Special slot8 measures from
 * the PLAYER binding even when firingSlot is another unit. Equality stays in band. */
int16_t BtechClassifyWeaponRange(uint16_t firingSlot,uint16_t targetSlot,uint16_t weaponKind)
{
    if (firingSlot>=COMBAT_MECH_SLOTS || targetSlot>COMBAT_SPECIAL_TARGET_SLOT || weaponKind>=COMBAT_WEAPON_KINDS) abort();
    uint32_t distance;
    if (targetSlot!=COMBAT_SPECIAL_TARGET_SLOT && unitControls[firingSlot].sideIndex!=unitControls[targetSlot].sideIndex)
        distance=unitDistances[firingSlot][targetSlot];
    else
    {
        BtechPooledWorldObject *source,*target;
        if (targetSlot==COMBAT_SPECIAL_TARGET_SLOT)
        {
            if (!playerUnitBinding) abort();
            source=playerUnitBinding->positionedLeftLeg;target=specialTargetObject;
        }
        else {source=unitBindings[firingSlot].positionedLeftLeg;target=unitBindings[targetSlot].positionedLeftLeg;}
        if (!source || !target) abort();
        distance=BtechApproximatePlanarDistance(&source->object.prefix.position,&target->object.prefix.position);
    }
    BtechWeaponDefinition *weapon=&weaponDefinitions[weaponKind];
    uint16_t boundaries[]={weapon->minimumRange,weapon->shortRangeBoundary,weapon->mediumRangeBoundary,weapon->longRangeBoundary};
    for (int16_t band=0;band<WEAPON_RANGE_BANDS-1;++band)
        if ((int32_t)(int16_t)(uint16_t)(boundaries[band]*WEAPON_RANGE_WORLD_SCALE)>=(int32_t)distance) return band;
    return WEAPON_RANGE_BANDS-1;
}

/*0810:A66C. Friendly blockers can tighten range while returning false and
 * clearing the target. Wide-angle is bit1; blocker is bit0, not a target kind. */
int16_t BtechTestAiFiringCandidate(const BtechPosition3D *position,uint32_t distance,uint16_t flags)
{
    if ((int32_t)distance>=(int32_t)(int16_t)aiCandidateDistance) return 0;
    if (!position || !activeUnitPositionedObject || !activeUnitTorsoObject) abort();
    BtechWorldObjectPrefix *root=&activeUnitPositionedObject->object.prefix;
    uint32_t differenceX=position->positionA-root->position.positionA;
    uint32_t differenceY=position->positionB-root->position.positionB;
    uint32_t invertedHeight=root->position.positionC-position->positionC;
    uint16_t yaw=BtechCalculateVectorHeading(differenceX,differenceY);
    yaw-=activeUnitPositionedObject->object.prefix.orientationC;
    yaw-=activeUnitTorsoObject->object.prefix.orientationC;
    int16_t lateral=(flags&AI_CANDIDATE_WIDE_ANGLE)?AI_WIDE_LATERAL_ALLOWANCE:AI_NORMAL_LATERAL_ALLOWANCE;
    int16_t yawLimit=(int16_t)BtechCalculateVectorHeading((uint32_t)(int32_t)lateral,distance);
    int16_t pitch=(int16_t)(uint16_t)(BtechCalculateVectorHeading(0-invertedHeight,distance)
        -activeUnitPositionedObject->object.prefix.orientationA);
    int16_t absolutePitch=(uint16_t)pitch==0x8000?INT16_MAX:pitch<0?-pitch:pitch;
    if (absolutePitch>=AI_PITCH_LIMIT && !(flags&AI_CANDIDATE_WIDE_ANGLE)) return 0;
    int16_t absoluteYaw=yaw==0x8000?INT16_MAX:(int16_t)yaw<0?-(int16_t)yaw:(int16_t)yaw;
    if (absoluteYaw>yawLimit) return 0;
    aiCandidateDistance=(uint16_t)distance;
    if (flags&AI_CANDIDATE_BLOCKER) {aiCandidateTargetSlot=UINT16_MAX;return 0;}
    return 1;
}

/* Score installed weapons that can fire at this range without overheating. */
int16_t BtechSelectAiWeaponMountForDistance(int16_t distance)
{
    if (!activeMechTemplate || !activeMechState) abort();
    int16_t selected=-1,bestScore=0;
    for (int16_t mount=0;mount<COMBAT_WEAPON_MOUNTS && mount<activeMechTemplate->weaponMountCount;++mount)
    {
        int16_t kind=activeMechTemplate->weaponMounts[mount].weaponKind;
        if ((uint16_t)kind>=COMBAT_WEAPON_KINDS) continue;
        int16_t ammoFamily=weaponDefinitions[kind].ammoFamily;
        if (!activeMechState->weapons[mount].operational) continue;
        if ((uint16_t)ammoFamily!=COMBAT_WEAPON_NONE)
        {
            if ((uint16_t)ammoFamily>=COMBAT_AMMO_FAMILIES) abort();
            if (!activeMechState->ammoSalvosRemaining[ammoFamily]) continue;
        }
        if (activeMechState->weapons[mount].reloadTicksRemaining) continue;
        BtechWeaponDefinition *weapon=&weaponDefinitions[kind];
        if ((int16_t)(uint16_t)(weapon->minimumRange*WEAPON_RANGE_WORLD_SCALE)>=distance
            || (int16_t)(uint16_t)(weapon->longRangeBoundary*WEAPON_RANGE_WORLD_SCALE)<=distance) continue;
        if ((int16_t)(uint16_t)(weapon->heatUnits+activeMechState->heatUnits)>=AI_PROJECTED_FIRING_HEAT_LIMIT) continue;
        uint16_t spread=BtechRandomSignedSpread(AI_MOUNT_SCORE_SPREAD);
        int16_t score=(int16_t)(uint16_t)(weapon->damageOrMissileCount+spread);
        if (score>bestScore) {bestScore=score;selected=mount;}
    }
    return selected;
}

/*0810:A879. Shot accounting belongs to A3FA. Origin snapshots shared5440
 * before another AC0F call. Firing-heading aspect chooses the hit-location row. */
void BtechFireAiWeaponAtUnit(int16_t targetSlot,int16_t weaponKind,uint16_t bodyLocation)
{
    if ((uint16_t)targetSlot>COMBAT_SPECIAL_TARGET_SLOT || (uint16_t)weaponKind>=COMBAT_WEAPON_KINDS || bodyLocation>=COMBAT_BODY_LOCATIONS
        || !activeUnitControl || !activeUnitPositionedObject || !activeUnitTorsoObject || !activeMechTemplate || !activeMechState) abort();
    BtechPooledWorldObject *target=targetSlot<COMBAT_MECH_SLOTS?unitBindings[targetSlot].positionedLeftLeg:specialTargetObject;
    if (!target) abort();
    uint16_t firingHeading=activeUnitPositionedObject->object.prefix.orientationC+activeUnitTorsoObject->object.prefix.orientationC;
    int16_t aspect=(int16_t)(uint16_t)(firingHeading-target->object.prefix.orientationC+ATTACK_ASPECT_HEADING_BIAS);
    int16_t magnitude=(uint16_t)aspect==0x8000?INT16_MAX:aspect<0?-aspect:aspect;
    uint16_t arc=magnitude<FRONT_ATTACK_ARC_LIMIT?ATTACK_ARC_FRONT:magnitude>REAR_ATTACK_ARC_LIMIT?ATTACK_ARC_REAR:aspect>0?ATTACK_ARC_LEFT:ATTACK_ARC_RIGHT;
    BtechPosition3D origin=*BtechGetUnitBodyReferencePosition(activeUnitControl->unitSlot,(int16_t)bodyLocationReferenceIndices[bodyLocation]);
    uint8_t hit=1;
    if (targetSlot!=COMBAT_SPECIAL_TARGET_SLOT)
    {
        int16_t band=BtechClassifyWeaponRange(activeUnitControl->unitSlot,(uint16_t)targetSlot,(uint16_t)weaponKind);
        if (activeUnitControl->unitSlot>=COMBAT_MECH_SLOTS) abort();
        int16_t targetNumber=(int16_t)(uint16_t)(gunneryTargetNumbers[activeUnitControl->unitSlot]+rangeHitModifiers[band]-TO_HIT_RANGE_BASE_ADJUSTMENT);
        int16_t speed=activeUnitPositionedObject->forwardStepScale;
        int16_t absoluteSpeed=(uint16_t)speed==0x8000?INT16_MAX:speed<0?-speed:speed;
        int16_t threshold=(int16_t)activeMechTemplate->firingMovementThreshold;
        if (absoluteSpeed>threshold) targetNumber=(int16_t)(uint16_t)(targetNumber+TO_HIT_FIRING_MOVEMENT_HIGH);
        else
        {
            int16_t half=(int16_t)((activeMechTemplate->firingMovementThreshold>>1)|(threshold<0?0x8000:0));
            if (absoluteSpeed>half) targetNumber=(int16_t)(uint16_t)(targetNumber+TO_HIT_FIRING_MOVEMENT_LOW);
        }
        speed=target->forwardStepScale;
        absoluteSpeed=(uint16_t)speed==0x8000?INT16_MAX:speed<0?-speed:speed;
        targetNumber=(int16_t)(uint16_t)(targetNumber+absoluteSpeed/TO_HIT_TARGET_MOTION_STEP);
        if (!speed) targetNumber=(int16_t)(uint16_t)(targetNumber-TO_HIT_STATIONARY_TARGET_BONUS);
        uint16_t firstDie=BtechRandomBelowBound(6);
        uint16_t secondDie=BtechRandomBelowBound(6);
        hit=(int16_t)(uint16_t)(firstDie+secondDie+2)>=targetNumber;
    }
    /* Special slot8 compares the same unwritten native WORD with itself:
     * always a hit, without inventing a C value or consuming to-hit dice. */
    if (hit)
    {
        uint16_t firstDie=BtechRandomBelowBound(6);
        uint16_t secondDie=BtechRandomBelowBound(6);
        uint16_t armor=hitLocationRows[arc][(uint16_t)(firstDie+secondDie)];
        if ((uint16_t)weaponDefinitions[weaponKind].ammoFamily==COMBAT_WEAPON_NONE || weaponKind==COMBAT_WEAPON_MACHINE_GUN)
        {
            BtechPosition3D endpoint;
            if (targetSlot<COMBAT_MECH_SLOTS) endpoint=*BtechGetUnitBodyReferencePosition((uint16_t)targetSlot,(int16_t)armor);
            else {endpoint=target->object.prefix.position;endpoint.positionC=AI_SPECIAL_TARGET_AIM_HEIGHT;}
            uint16_t effect=weaponKind==COMBAT_WEAPON_MACHINE_GUN;
            if (!effect) BtechBuildWeaponBeamGeometry(&origin,&endpoint,weaponKind);
            if (targetSlot) BtechPlaySoundIfEnabled(DIRECT_HIT_SOUND,DIRECT_HIT_SOUND_PRIORITY);
            BtechApplyArmorAndInternalDamage((uint16_t)targetSlot,(int16_t)armor,(int16_t)weaponDefinitions[weaponKind].damageOrMissileCount);
            BtechSpawnSequencedWorldEffect(&endpoint,effect);
            return;
        }
        BtechProjectileState *projectile=BtechSpawnUnitWeaponProjectile(activeUnitControl->unitSlot,weaponKind,&origin);
        if (targetSlot<COMBAT_MECH_SLOTS)
        {
            if (!projectile) return;
            projectile->targetUnitSlot=(uint16_t)targetSlot;projectile->targetArmorLocation=armor;
        }
    }
    else
    {
        if (weaponKind==COMBAT_WEAPON_MACHINE_GUN) return;
        if ((uint16_t)weaponDefinitions[weaponKind].ammoFamily==COMBAT_WEAPON_NONE)
        {
            int16_t range=(int16_t)(uint16_t)(weaponDefinitions[weaponKind].longRangeBoundary*WEAPON_RANGE_WORLD_SCALE);
            BtechPosition3D endpoint;
            endpoint.positionA=origin.positionA-BtechShiftSignedDwordRight((uint32_t)BtechMultiplySignedWords((int16_t)BtechLookupSine(firingHeading),range),AI_MISS_DIRECTION_PRODUCT_SHIFT);
            endpoint.positionB=origin.positionB+BtechShiftSignedDwordRight((uint32_t)BtechMultiplySignedWords((int16_t)BtechLookupCosine(firingHeading),range),AI_MISS_DIRECTION_PRODUCT_SHIFT);
            uint32_t cosine=(uint32_t)(int32_t)(int16_t)BtechLookupCosine(activeMechState->aimPitch);
            uint32_t sineTimesRange=(uint32_t)BtechMultiplySignedWords((int16_t)BtechLookupSine(activeMechState->aimPitch),range);
            endpoint.positionC=origin.positionC+BtechDivideSignedDword(sineTimesRange,cosine);
            BtechBuildWeaponBeamGeometry(&origin,&endpoint,weaponKind);
            return;
        }
        BtechSpawnUnitWeaponProjectile(activeUnitControl->unitSlot,weaponKind,&origin);
    }
    if (!targetSlot) BtechPlaySoundIfEnabled(AI_INCOMING_PROJECTILE_SOUND,AI_INCOMING_PROJECTILE_PRIORITY);
}

/*0810:A3FA. Shared candidate scratch can be narrowed by friendly or disabled
 * blockers before mount selection. Accounting precedes aim and the actual shot. */
void BtechChooseAiTargetAndFire(void)
{
    aiCandidateTargetSlot=UINT16_MAX;
    if (!activeUnitControl || activeUnitControl->sideIndex>=2) abort();
    aiCandidateDistance=(uint16_t)activeUnitControl->weaponRangeMinusMargin;
    uint16_t rule=sideObjectiveRules[activeUnitControl->sideIndex];
    uint16_t specialBlocks=rule==OBJECTIVE_DEFEND_SPECIAL_TARGET || specialTargetInteractionBlocked;
    if (specialBlocks || rule==OBJECTIVE_SPECIAL_TARGET)
    {
        if (!specialTargetObject || !activeUnitPositionedObject) abort();
        if (!(specialTargetObject->object.prefix.renderFlags&COMBAT_WORLD_OBJECT_HIDDEN))
        {
            const BtechPosition3D *position=&specialTargetObject->object.prefix.position;
            uint32_t distance=BtechApproximatePlanarDistance(position,&activeUnitPositionedObject->object.prefix.position)-AI_SPECIAL_TARGET_DISTANCE_MARGIN;
            if (BtechTestAiFiringCandidate(position,distance,specialBlocks+AI_CANDIDATE_WIDE_ANGLE)) aiCandidateTargetSlot=COMBAT_SPECIAL_TARGET_SLOT;
        }
    }
    for (uint16_t slot=0;slot<COMBAT_MECH_SLOTS;++slot)
    {
        if (!unitBindings[slot].mech || activeUnitControl->unitSlot==slot) continue;
        uint16_t blocker=unitControls[slot].sideIndex==activeUnitControl->sideIndex;
        if (activeUnitControl->unitSlot>=COMBAT_MECH_SLOTS) abort();
        if (!blocker && !unitVisibility[activeUnitControl->unitSlot][slot]) continue;
        if (unitControls[slot].combatValueOrBoundaryCode<0) blocker=1;
        if (!unitBindings[slot].positionedLeftLeg || !activeUnitPositionedObject) abort();
        const BtechPosition3D *position=&unitBindings[slot].positionedLeftLeg->object.prefix.position;
        uint32_t distance=blocker?BtechApproximatePlanarDistance(&activeUnitPositionedObject->object.prefix.position,position)
            :unitDistances[activeUnitControl->unitSlot][slot];
        if (unitControls[slot].movementMode!=MOVEMENT_MODE_FALLEN_POSE && BtechTestAiFiringCandidate(position,distance,blocker)) aiCandidateTargetSlot=slot;
    }
    if ((int16_t)aiCandidateTargetSlot<0) return;
    if (aiCandidateTargetSlot>COMBAT_SPECIAL_TARGET_SLOT) abort();
    BtechPooledWorldObject *target=aiCandidateTargetSlot==COMBAT_SPECIAL_TARGET_SLOT?specialTargetObject:unitBindings[aiCandidateTargetSlot].positionedLeftLeg;
    if (!target) abort();
    const BtechPosition3D *position=&target->object.prefix.position;
    int16_t mountIndex=BtechSelectAiWeaponMountForDistance((int16_t)aiCandidateDistance);
    if (mountIndex<0) return;
    BtechWeaponMount *mount=&activeMechTemplate->weaponMounts[mountIndex];
    if ((uint16_t)mount->weaponKind>=COMBAT_WEAPON_KINDS) abort();
    BtechWeaponDefinition *weapon=&weaponDefinitions[mount->weaponKind];
    activeUnitControl->lastAiFiringTargetSlot=(int16_t)aiCandidateTargetSlot;
    activeMechState->heatUnits+=weapon->heatUnits;
    activeMechState->weapons[mountIndex].reloadTicksRemaining=weapon->reloadTicks;
    if ((uint16_t)weapon->ammoFamily!=COMBAT_WEAPON_NONE)
    {
        if ((uint16_t)weapon->ammoFamily>=COMBAT_AMMO_FAMILIES) abort();
        --activeMechState->ammoSalvosRemaining[weapon->ammoFamily];
    }
    uint32_t height=position->positionC-activeUnitPositionedObject->object.prefix.position.positionC;
    uint16_t elevation=BtechCalculateVectorHeading((uint32_t)(int32_t)(int16_t)aiCandidateDistance,height);
    activeMechState->aimPitch=elevation+AI_AIM_HEADING_BIAS;
    if (activeUnitControl->animationSequence==ANIMATION_SEQUENCE_WALK && (int16_t)activeUnitControl->animationFrame>AI_FIRING_PAUSE_FRAME)
        activeUnitControl->animationRequestFlags|=ANIMATION_REQUEST_FIRING_PAUSE;
    BtechFireAiWeaponAtUnit((int16_t)aiCandidateTargetSlot,mount->weaponKind,mount->bodyLocation);
}
