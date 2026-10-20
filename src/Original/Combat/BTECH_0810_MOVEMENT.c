#include "mechwarrior/combat_movement.h"
#include "mechwarrior/combat_damage.h"
#include "mechwarrior/combat_surface.h"
#include "mechwarrior/combat_pose.h"
#include "mechwarrior/combat_import.h"
#include "mechwarrior/combat_startup.h"
#include "mechwarrior/combat_sound.h"
#include "mechwarrior/combat_render.h"
#include <stdlib.h>

/*0810:5DF6. Original axis-aligned special-target volume. Height is the query's
 * absolute C coordinate, not target-relative. All upper boundaries are strict. */
uint8_t BtechTestPositionInsideSpecialTarget(const BtechPosition3D *position,uint16_t narrowBounds)
{
    if (!specialTargetObject) abort();
    const BtechWorldObjectPrefix *target=&specialTargetObject->object.prefix;
    if (target->renderFlags&COMBAT_WORLD_OBJECT_HIDDEN) return 0;
    uint32_t deltaA=target->position.positionA-position->positionA;
    uint32_t deltaB=target->position.positionB-position->positionB;
    if (deltaA==0x8000) deltaA=0x7FFF;else if ((int32_t)deltaA<0) deltaA=0-deltaA;
    if (deltaB==0x8000) deltaB=0x7FFF;else if ((int32_t)deltaB<0) deltaB=0-deltaB;
    if ((int32_t)deltaA>=SPECIAL_TARGET_CONTACT_HALF_WIDTH || (int32_t)position->positionC>=SPECIAL_TARGET_CONTACT_HEIGHT) return 0;
    return (int32_t)deltaB<SPECIAL_TARGET_NARROW_HALF_DEPTH || (!narrowBounds && (int32_t)deltaB<SPECIAL_TARGET_CONTACT_HALF_WIDTH);
}

/*0810:8C44. Side-zero objective rule gates all contact. Uses the Mech's live
 * reference scale, not its object's current forward motion, for damage. */
uint16_t BtechResolveUnitContactWithSpecialTarget(uint16_t unitSlot)
{
    if ((int16_t)sideObjectiveRules[0]<=OBJECTIVE_OPPOSE_WITHDRAWAL) return 0;
    if (unitSlot>=COMBAT_MECH_SLOTS || !unitBindings[unitSlot].positionedLeftLeg) abort();
    const BtechPosition3D *position=&unitBindings[unitSlot].positionedLeftLeg->object.prefix.position;
    if (!BtechTestPositionInsideSpecialTarget(position,unitSlot<1)) return 0;
    BtechMechState *mech=unitBindings[unitSlot].mech;
    if (!mech || mech->templateIndex>=COMBAT_MECH_TEMPLATES) abort();
    const BtechMechTemplate *chassis=&mechTemplates[mech->templateIndex];
    uint16_t side=unitControls[unitSlot].sideIndex;
    if (side>=2) abort();
    if (sideObjectiveRules[side]==OBJECTIVE_SPECIAL_TARGET && specialTargetInteractionBlocked)
        sideCompletionFlags[side]=1;
    else
    {
        uint16_t product=(int32_t)(int16_t)mech->movementReferenceScale*(int16_t)chassis->tonnage;
        int16_t damage=(product>>CONTACT_DAMAGE_PRODUCT_SHIFT)|(product&0x8000?0xFF80:0);
        BtechApplyArmorAndInternalDamage(unitSlot,COMBAT_ARMOR_CENTER_TORSO,damage);
        BtechApplyArmorAndInternalDamage(unitSlot,COMBAT_ARMOR_LEFT_LEG,damage);
        BtechApplyArmorAndInternalDamage(unitSlot,COMBAT_ARMOR_RIGHT_LEG,damage);
    }
    return 1;
}

/*0810:89F6. The active AI context is NOT replaced with the argument's slot.
 * In particular contact speed/side read7750/7752, even in the player caller. */
uint8_t BtechTestUnitMovementBoundary(uint16_t unitSlot)
{
    if (unitSlot>=COMBAT_MECH_SLOTS || !unitBindings[unitSlot].positionedLeftLeg) abort();
    BtechPooledWorldObject *body=unitBindings[unitSlot].positionedLeftLeg;
    if (unitSlot)
    {
        if (!activeUnitControl) abort();
        if (activeUnitControl->tacticalState!=COMBAT_TACTICAL_RETREATING)
        {
            uint8_t classify=activeUnitControl->commandCode==COMBAT_ORDER_ATTACK_ENEMY;
            if (!classify && activeUnitControl->trackedPosition[2]>-1)
            {
                classify=1;
                for (uint16_t axis=0;axis<2;++axis)
                {
                    uint32_t magnitude=activeUnitControl->trackedPosition[axis];
                    if (magnitude==0x8000) magnitude=0x7FFF;else if ((int32_t)magnitude<0) magnitude=0-magnitude;
                    uint16_t limit=axis?COMBAT_BOUNDARY_LIMIT_B:COMBAT_BOUNDARY_LIMIT_A;
                    if ((int16_t)(magnitude>>16)>0 || (!(magnitude>>16) && (uint16_t)magnitude>=limit)) {classify=0;break;}
                }
            }
            if (classify && BtechClassifyUnitWorldBoundary(unitSlot)!=COMBAT_BOUNDARY_INSIDE) return 1;
        }
    }
    for (uint16_t otherSlot=0;otherSlot<COMBAT_MECH_SLOTS;++otherSlot)
    {
        if (otherSlot==unitSlot || !unitBindings[otherSlot].mech) continue;
        if (!unitBindings[otherSlot].positionedLeftLeg) abort();
        const BtechPosition3D *other=&unitBindings[otherSlot].positionedLeftLeg->object.prefix.position;
        const BtechPosition3D *position=&body->object.prefix.position;
        uint32_t differences[3]={other->positionA-position->positionA,other->positionB-position->positionB,other->positionC-position->positionC};
        uint8_t close=1;
        for (uint16_t axis=0;axis<3;++axis)
        {
            uint32_t magnitude=differences[axis];
            if (magnitude==0x8000) magnitude=0x7FFF;else if ((int32_t)magnitude<0) magnitude=0-magnitude;
            uint16_t limit=axis==2?UNIT_CONTACT_VERTICAL_LIMIT:UNIT_CONTACT_PLANAR_LIMIT;
            if ((int16_t)(magnitude>>16)>0 || (!(magnitude>>16) && (uint16_t)magnitude>=limit)) {close=0;break;}
        }
        if (close)
        {
            if (!activeUnitPositionedObject) abort(); /* Native null near-owner access is not fabricated. */
            if (activeUnitPositionedObject->forwardStepScale>UNIT_CONTACT_DAMAGE_SPEED_MINIMUM)
            {
                if (!activeUnitControl) abort();
                if (unitControls[otherSlot].sideIndex!=activeUnitControl->sideIndex)
                    BtechApplyUnitCollisionDamage(unitSlot,otherSlot);
            }
            return 1;
        }
    }
    return (uint8_t)BtechResolveUnitContactWithSpecialTarget(unitSlot);
}

/*0810:8D0E. Native LOW-WORD mass squares and speed products, not kinetic
 * energy. Preserve SAR floor halves versus later signed division toward zero. */
void BtechApplyUnitCollisionDamage(uint16_t movingSlot,uint16_t otherSlot)
{
    if (movingSlot>=COMBAT_MECH_SLOTS || otherSlot>=COMBAT_MECH_SLOTS) abort();
    BtechUnitBinding *moving=&unitBindings[movingSlot],*other=&unitBindings[otherSlot];
    if (!moving->mech || !other->mech || !moving->positionedLeftLeg || !other->positionedLeftLeg
        || moving->mech->templateIndex>=COMBAT_MECH_TEMPLATES || other->mech->templateIndex>=COMBAT_MECH_TEMPLATES) abort();
    int16_t movingMass=mechTemplates[moving->mech->templateIndex].tonnage;
    int16_t otherMass=mechTemplates[other->mech->templateIndex].tonnage;
    int16_t movingSquared=(int32_t)movingMass*movingMass;
    int16_t otherSquared=(int32_t)otherMass*otherMass;
    int16_t movingSpeed=moving->positionedLeftLeg->forwardStepScale;
    int16_t otherSpeed=other->positionedLeftLeg->forwardStepScale;
    uint32_t movingWeight=(int16_t)((int32_t)movingSpeed*movingSquared);
    uint32_t otherWeight=(int16_t)((int32_t)otherSpeed*otherSquared);
    int16_t headingDifference=moving->positionedLeftLeg->object.prefix.orientationC
        -other->positionedLeftLeg->object.prefix.orientationC;
    int16_t absoluteHeading=headingDifference==INT16_MIN?INT16_MAX:headingDifference<0?-headingDifference:headingDifference;
    if (absoluteHeading>CONTACT_HEAD_ON_ANGLE)
    {
        int16_t damage=(int16_t)BtechShiftSignedDwordRight(movingWeight,CONTACT_WEIGHT_SHIFT);
        BtechApplyArmorAndInternalDamage(otherSlot,COMBAT_ARMOR_CENTER_TORSO,damage);
        BtechApplyArmorAndInternalDamage(otherSlot,COMBAT_ARMOR_LEFT_LEG,damage/CONTACT_LIMB_DAMAGE_DIVISOR);
        BtechApplyArmorAndInternalDamage(otherSlot,COMBAT_ARMOR_RIGHT_LEG,damage/CONTACT_LIMB_DAMAGE_DIVISOR);
        damage=(int16_t)BtechShiftSignedDwordRight(otherWeight,CONTACT_WEIGHT_SHIFT);
        BtechApplyArmorAndInternalDamage(movingSlot,COMBAT_ARMOR_CENTER_TORSO,damage);
        BtechApplyArmorAndInternalDamage(movingSlot,COMBAT_ARMOR_LEFT_LEG,damage/CONTACT_LIMB_DAMAGE_DIVISOR);
        BtechApplyArmorAndInternalDamage(movingSlot,COMBAT_ARMOR_RIGHT_LEG,damage/CONTACT_LIMB_DAMAGE_DIVISOR);
        BtechClearObjectMotionTail(other->positionedLeftLeg); /* Reload binding after damage. */
        uint32_t difference=movingWeight-otherWeight;
        int16_t threshold=unitControls[otherSlot].combatValueOrBoundaryCode==COMBAT_MECH_DISABLED_VALUE?CONTACT_DISABLED_FALL_THRESHOLD:CONTACT_FALL_THRESHOLD;
        if ((int32_t)difference>0
            && (int32_t)BtechDivideSignedDword(difference,(uint32_t)otherSquared)>threshold)
        {unitControls[otherSlot].animationRequestFlags|=ANIMATION_REQUEST_FALL_POSE;return;}
        if ((int32_t)difference<0
            && (int32_t)BtechDivideSignedDword(0-difference,(uint32_t)movingSquared)>CONTACT_FALL_THRESHOLD)
            unitControls[movingSlot].animationRequestFlags|=ANIMATION_REQUEST_FALL_POSE;
        return;
    }
    int16_t damage;
    if (absoluteHeading<CONTACT_FOLLOWING_ANGLE)
    {
        int16_t relativeSpeed=(uint16_t)movingSpeed-(uint16_t)otherSpeed;
        uint32_t relativeWeight=(int16_t)((int32_t)relativeSpeed*movingSquared);
        damage=(int16_t)BtechShiftSignedDwordRight(relativeWeight,CONTACT_WEIGHT_SHIFT);
        BtechApplyArmorAndInternalDamage(otherSlot,COMBAT_ARMOR_REAR_CENTER_TORSO,damage);
        BtechApplyArmorAndInternalDamage(otherSlot,COMBAT_ARMOR_LEFT_LEG,damage/CONTACT_LIMB_DAMAGE_DIVISOR);
        BtechApplyArmorAndInternalDamage(otherSlot,COMBAT_ARMOR_RIGHT_LEG,damage/CONTACT_LIMB_DAMAGE_DIVISOR);
    }
    else
    {
        uint16_t side=headingDifference>0?1:0;
        damage=(int16_t)BtechShiftSignedDwordRight(movingWeight,CONTACT_WEIGHT_SHIFT);
        BtechApplyArmorAndInternalDamage(otherSlot,(int16_t)(side+COMBAT_ARMOR_LEFT_TORSO),damage);
        BtechApplyArmorAndInternalDamage(otherSlot,(int16_t)side,damage/CONTACT_LIMB_DAMAGE_DIVISOR);
        BtechApplyArmorAndInternalDamage(otherSlot,(int16_t)(side+COMBAT_ARMOR_LEFT_LEG),damage/CONTACT_LIMB_DAMAGE_DIVISOR);
    }
    damage=(int16_t)(((uint16_t)damage>>CONTACT_SELF_DAMAGE_SHIFT)|((uint16_t)damage&0x8000));
    BtechApplyArmorAndInternalDamage(movingSlot,COMBAT_ARMOR_CENTER_TORSO,damage);
    if (absoluteHeading<CONTACT_FOLLOWING_ANGLE)
    {
        BtechApplyArmorAndInternalDamage(movingSlot,COMBAT_ARMOR_LEFT_LEG,damage/CONTACT_LIMB_DAMAGE_DIVISOR);
        BtechApplyArmorAndInternalDamage(movingSlot,COMBAT_ARMOR_RIGHT_LEG,damage/CONTACT_LIMB_DAMAGE_DIVISOR);
    }
    else
    {
        BtechApplyArmorAndInternalDamage(movingSlot,COMBAT_ARMOR_RIGHT_LEG,damage/CONTACT_LIMB_DAMAGE_DIVISOR);
        BtechApplyArmorAndInternalDamage(movingSlot,COMBAT_ARMOR_LEFT_LEG,damage/CONTACT_LIMB_DAMAGE_DIVISOR);
    }
}

/*0810:91E2. Original player motion, landing/contact rollback and part sync.
 * Do not install player AI context before89F6: native only sets7752 at the end. */
void BtechUpdatePlayerMechMotion(void)
{
    if (!playerUnitBinding || !playerUnitBinding->positionedLeftLeg || !playerMechTemplate || !playerMechState) abort();
    BtechPooledWorldObject *body=playerUnitBinding->positionedLeftLeg;
    BtechWorldObjectPrefix *pose=&body->object.prefix;
    BtechPosition3D oldPosition=pose->position;
    uint16_t oldOrientation[3]={pose->orientationA,pose->orientationB,pose->orientationC};
    if (unitControls[0].movementMode==MOVEMENT_MODE_JUMP)
    {
        body->orientationSteps[1]=0;body->orientationSteps[0]=0;playerSpecialTargetLandingLatch=0;
        BtechAdvancePooledObjectMotion(body);
        uint32_t ground=0;
        if (BtechQueryWorldSurface(&body->surfaceQuery,&pose->position,selectedWorldObjects,(int16_t)selectedObjectCount))
            ground=body->surfaceQuery.surfacePositionC;
        uint16_t scale=playerMechTemplate->displayScale;
        int16_t halfScale=(scale>>1)|(scale&0x8000);
        uint32_t footHeight=pose->position.positionC-(uint32_t)halfScale;
        if ((int32_t)footHeight<=(int32_t)ground)
        {
            scale=playerMechTemplate->displayScale;halfScale=(int16_t)((scale>>1)|(scale&0x8000));
            pose->position.positionC=ground+(uint32_t)halfScale;
            if (body->positionSteps[2]<PLAYER_HARD_LANDING_STEP)
            {
                BtechApplyArmorAndInternalDamage(0,COMBAT_ARMOR_LEFT_LEG,(int16_t)(BtechRandomBelowBound(PLAYER_LANDING_DAMAGE_BOUND)+PLAYER_LANDING_DAMAGE_BASE));
                BtechApplyArmorAndInternalDamage(0,COMBAT_ARMOR_RIGHT_LEG,(int16_t)(BtechRandomBelowBound(PLAYER_LANDING_DAMAGE_BOUND)+PLAYER_LANDING_DAMAGE_BASE));
                unitControls[0].movementMode=MOVEMENT_MODE_FALLING;
                unitControls[0].animationRequestFlags|=ANIMATION_REQUEST_FALL_POSE;
            }
            else
            {
                unitControls[0].movementMode=MOVEMENT_MODE_LANDING;
                unitControls[0].animationRequestFlags|=ANIMATION_REQUEST_LANDING;
            }
            body->positionSteps[2]=0;
        }
        else if (BtechResolveUnitContactWithSpecialTarget(0))
        {
            BtechPlaySoundIfEnabled(MOVEMENT_IMPACT_SOUND,MOVEMENT_IMPACT_SOUND_PRIORITY);
            playerImpactThisTick=1;body->forwardStepScale=0;
            if ((int32_t)oldPosition.positionC<PLAYER_SPECIAL_LANDING_HEIGHT)
            {pose->position.positionA=oldPosition.positionA;pose->position.positionB=oldPosition.positionB;}
            else
            {
                /*9367..9379 computes a height comparison then discards AX. No height clamp. */
                body->positionSteps[2]=0;unitControls[0].movementMode=MOVEMENT_MODE_LANDING;
                unitControls[0].animationRequestFlags|=ANIMATION_REQUEST_LANDING;playerSpecialTargetLandingLatch=1;
            }
        }
    }
    else if ((int16_t)playerMechState->gyroHits>=COMBAT_GYRO_DESTROYED_HITS
        || (int16_t)playerMechState->engineHits>=COMBAT_ENGINE_HIT_LIMIT
        || unitControls[0].movementMode==MOVEMENT_MODE_FALLEN_POSE)
    {body->orientationSteps[1]=0;body->orientationSteps[0]=0;}
    else
    {
        BtechAdvancePooledObjectMotion(body);
        uint16_t surfaceClass=BtechResolveGroundSurfaceAndSlope(body,0,selectedWorldObjects,(int16_t)selectedObjectCount);
        body->orientationSteps[1]=0;pose->orientationB=0;
        if (body->forwardStepScale)
        {
            if (BtechTestUnitMovementBoundary(0) || surfaceClass==SURFACE_BLOCKS_GROUND_MOVEMENT || playerSpecialTargetLandingLatch)
            {
                BtechPlaySoundIfEnabled(MOVEMENT_IMPACT_SOUND,MOVEMENT_IMPACT_SOUND_PRIORITY);playerImpactThisTick=1;
                BtechClearObjectMotionTail(body);
                pose->position=oldPosition;
                pose->orientationA=oldOrientation[0];pose->orientationB=oldOrientation[1];pose->orientationC=oldOrientation[2];
            }
            else
            {
                uint16_t scale=playerMechTemplate->displayScale;
                pose->position.positionC+=(uint32_t)(int16_t)((scale>>1)|(scale&0x8000));
                playerMechState->doubleCoolingFlag=surfaceClass==SURFACE_DOUBLE_COOLING;
            }
        }
        else
        {
            pose->position=oldPosition;
            if (surfaceClass==SURFACE_BLOCKS_GROUND_MOVEMENT || playerSpecialTargetLandingLatch)
            {pose->orientationB=0;pose->orientationA=0;body->orientationSteps[1]=0;body->orientationSteps[0]=0;}
        }
    }
    activeUnitControl=&unitControls[0];
    BtechSynchronizeMechRenderParts();
}
