#include "mechwarrior/port_diagnostics.h"
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
uint8_t BtechTestPositionInsideSpecialTarget(const BtechPosition3D *contactPosition,uint16_t useNarrowContactBounds)
{
    if (!specialTargetObject) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !specialTargetObject");
    const BtechWorldObjectPrefix *objectiveObject=&specialTargetObject->object.prefix;
    if (objectiveObject->renderFlags&COMBAT_WORLD_OBJECT_HIDDEN) return 0;
    uint32_t absoluteSeparationX=objectiveObject->position.positionA-contactPosition->positionA;
    uint32_t absoluteSeparationY=objectiveObject->position.positionB-contactPosition->positionB;
    if (absoluteSeparationX==0x8000) absoluteSeparationX=0x7FFF;else if ((int32_t)absoluteSeparationX<0) absoluteSeparationX=0-absoluteSeparationX;
    if (absoluteSeparationY==0x8000) absoluteSeparationY=0x7FFF;else if ((int32_t)absoluteSeparationY<0) absoluteSeparationY=0-absoluteSeparationY;
    if ((int32_t)absoluteSeparationX>=SPECIAL_TARGET_CONTACT_HALF_WIDTH || (int32_t)contactPosition->positionC>=SPECIAL_TARGET_CONTACT_HEIGHT) return 0;
    return (int32_t)absoluteSeparationY<SPECIAL_TARGET_NARROW_HALF_DEPTH || (!useNarrowContactBounds && (int32_t)absoluteSeparationY<SPECIAL_TARGET_CONTACT_HALF_WIDTH);
}

/*0810:8C44. Side-zero objective rule gates all contact. Uses the Mech's live
 * reference scale, not its object's current forward motion, for damage. */
uint16_t BtechResolveUnitContactWithSpecialTarget(uint16_t unitSlot)
{
    if ((int16_t)sideObjectiveRules[0]<=OBJECTIVE_OPPOSE_WITHDRAWAL) return 0;
    if (unitSlot>=COMBAT_MECH_SLOTS || !unitBindings[unitSlot].positionedLeftLeg) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: unitSlot>=COMBAT_MECH_SLOTS || !unitBindings[unitSlot].positionedLeftLeg");
    const BtechPosition3D *contactPosition=&unitBindings[unitSlot].positionedLeftLeg->object.prefix.position;
    if (!BtechTestPositionInsideSpecialTarget(contactPosition,unitSlot<1)) return 0;
    BtechMechState *contactingMech=unitBindings[unitSlot].mech;
    if (!contactingMech || contactingMech->templateIndex>=COMBAT_MECH_TEMPLATES) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !contactingMech || contactingMech->templateIndex>=COMBAT_MECH_TEMPLATES");
    const BtechMechTemplate *contactingMechTemplate=&mechTemplates[contactingMech->templateIndex];
    uint16_t contactingSideIndex=unitControls[unitSlot].sideIndex;
    if (contactingSideIndex>=2) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: contactingSideIndex>=2");
    if (sideObjectiveRules[contactingSideIndex]==OBJECTIVE_SPECIAL_TARGET && specialTargetInteractionBlocked)
        sideCompletionFlags[contactingSideIndex]=1;
    else
    {
        uint16_t contactDamageProductLow=(int32_t)(int16_t)contactingMech->movementReferenceScale*(int16_t)contactingMechTemplate->tonnage;
        int16_t contactDamage=(contactDamageProductLow>>CONTACT_DAMAGE_PRODUCT_SHIFT)|(contactDamageProductLow&0x8000?0xFF80:0);
        BtechApplyArmorAndInternalDamage(unitSlot,COMBAT_ARMOR_CENTER_TORSO,contactDamage);
        BtechApplyArmorAndInternalDamage(unitSlot,COMBAT_ARMOR_LEFT_LEG,contactDamage);
        BtechApplyArmorAndInternalDamage(unitSlot,COMBAT_ARMOR_RIGHT_LEG,contactDamage);
    }
    return 1;
}

/*0810:89F6. The active AI context is NOT replaced with the argument's slot.
 * In particular contact speed/side read7750/7752, even in the player caller. */
uint8_t BtechTestUnitMovementObstruction(uint16_t unitSlot)
{
    if (unitSlot>=COMBAT_MECH_SLOTS || !unitBindings[unitSlot].positionedLeftLeg) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: unitSlot>=COMBAT_MECH_SLOTS || !unitBindings[unitSlot].positionedLeftLeg");
    BtechPooledWorldObject *movingMechObject=unitBindings[unitSlot].positionedLeftLeg;
    if (unitSlot)
    {
        if (!activeUnitControl) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !activeUnitControl");
        if (activeUnitControl->tacticalState!=COMBAT_TACTICAL_RETREATING)
        {
            uint8_t checkWorldBoundary=activeUnitControl->commandCode==COMBAT_ORDER_ATTACK_ENEMY;
            if (!checkWorldBoundary && activeUnitControl->trackedPosition[2]>-1)
            {
                checkWorldBoundary=1;
                for (uint16_t axis=0;axis<2;++axis)
                {
                    uint32_t absoluteTrackedCoordinate=activeUnitControl->trackedPosition[axis];
                    if (absoluteTrackedCoordinate==0x8000) absoluteTrackedCoordinate=0x7FFF;else if ((int32_t)absoluteTrackedCoordinate<0) absoluteTrackedCoordinate=0-absoluteTrackedCoordinate;
                    uint16_t axisBoundaryLimit=axis?COMBAT_BOUNDARY_LIMIT_B:COMBAT_BOUNDARY_LIMIT_A;
                    if ((int16_t)(absoluteTrackedCoordinate>>16)>0 || (!(absoluteTrackedCoordinate>>16) && (uint16_t)absoluteTrackedCoordinate>=axisBoundaryLimit)) {checkWorldBoundary=0;break;}
                }
            }
            if (checkWorldBoundary && BtechClassifyUnitWorldBoundary(unitSlot)!=COMBAT_BOUNDARY_INSIDE) return 1;
        }
    }
    for (uint16_t otherSlot=0;otherSlot<COMBAT_MECH_SLOTS;++otherSlot)
    {
        if (otherSlot==unitSlot || !unitBindings[otherSlot].mech) continue;
        if (!unitBindings[otherSlot].positionedLeftLeg) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !unitBindings[otherSlot].positionedLeftLeg");
        const BtechPosition3D *otherMechPosition=&unitBindings[otherSlot].positionedLeftLeg->object.prefix.position;
        const BtechPosition3D *movingMechPosition=&movingMechObject->object.prefix.position;
        uint32_t contactSeparation[3]={otherMechPosition->positionA-movingMechPosition->positionA,otherMechPosition->positionB-movingMechPosition->positionB,otherMechPosition->positionC-movingMechPosition->positionC};
        uint8_t withinContactBounds=1;
        for (uint16_t axis=0;axis<3;++axis)
        {
            uint32_t absoluteSeparation=contactSeparation[axis];
            if (absoluteSeparation==0x8000) absoluteSeparation=0x7FFF;else if ((int32_t)absoluteSeparation<0) absoluteSeparation=0-absoluteSeparation;
            uint16_t axisContactLimit=axis==2?UNIT_CONTACT_VERTICAL_LIMIT:UNIT_CONTACT_PLANAR_LIMIT;
            if ((int16_t)(absoluteSeparation>>16)>0 || (!(absoluteSeparation>>16) && (uint16_t)absoluteSeparation>=axisContactLimit)) {withinContactBounds=0;break;}
        }
        if (withinContactBounds)
        {
            if (!activeUnitPositionedObject) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !activeUnitPositionedObject"); /* Native null near-owner access is not fabricated. */
            if (activeUnitPositionedObject->forwardStepScale>UNIT_CONTACT_DAMAGE_SPEED_MINIMUM)
            {
                if (!activeUnitControl) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !activeUnitControl");
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
void BtechApplyUnitCollisionDamage(uint16_t movingUnitSlot,uint16_t contactedUnitSlot)
{
    if (movingUnitSlot>=COMBAT_MECH_SLOTS || contactedUnitSlot>=COMBAT_MECH_SLOTS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: movingUnitSlot>=COMBAT_MECH_SLOTS || contactedUnitSlot>=COMBAT_MECH_SLOTS");
    BtechUnitBinding *movingMechBinding=&unitBindings[movingUnitSlot],*contactedMechBinding=&unitBindings[contactedUnitSlot];
    if (!movingMechBinding->mech || !contactedMechBinding->mech || !movingMechBinding->positionedLeftLeg || !contactedMechBinding->positionedLeftLeg
        || movingMechBinding->mech->templateIndex>=COMBAT_MECH_TEMPLATES || contactedMechBinding->mech->templateIndex>=COMBAT_MECH_TEMPLATES) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !movingMechBinding->mech || !contactedMechBinding->mech || !movingMechBinding->positionedLeftLeg || !contactedMechBinding->positionedLeftLeg || movingMechBinding->mech->templateIndex>=COMBAT_MECH_TEMPLATES || contactedMechBinding->mech->templateIndex>=COMBAT_MECH_TEMPLATES");
    int16_t movingTonnage=mechTemplates[movingMechBinding->mech->templateIndex].tonnage;
    int16_t contactedTonnage=mechTemplates[contactedMechBinding->mech->templateIndex].tonnage;
    int16_t movingTonnageSquared=(int32_t)movingTonnage*movingTonnage;
    int16_t contactedTonnageSquared=(int32_t)contactedTonnage*contactedTonnage;
    int16_t movingForwardStep=movingMechBinding->positionedLeftLeg->forwardStepScale;
    int16_t contactedForwardStep=contactedMechBinding->positionedLeftLeg->forwardStepScale;
    uint32_t movingCollisionFactor=(int16_t)((int32_t)movingForwardStep*movingTonnageSquared);
    uint32_t contactedCollisionFactor=(int16_t)((int32_t)contactedForwardStep*contactedTonnageSquared);
    int16_t headingDifference=movingMechBinding->positionedLeftLeg->object.prefix.orientationC
        -contactedMechBinding->positionedLeftLeg->object.prefix.orientationC;
    int16_t absoluteHeadingDifference=headingDifference==INT16_MIN?INT16_MAX:headingDifference<0?-headingDifference:headingDifference;
    if (absoluteHeadingDifference>CONTACT_HEAD_ON_ANGLE)
    {
        int16_t collisionDamage=(int16_t)BtechShiftSignedDwordRight(movingCollisionFactor,CONTACT_WEIGHT_SHIFT);
        BtechApplyArmorAndInternalDamage(contactedUnitSlot,COMBAT_ARMOR_CENTER_TORSO,collisionDamage);
        BtechApplyArmorAndInternalDamage(contactedUnitSlot,COMBAT_ARMOR_LEFT_LEG,collisionDamage/CONTACT_LIMB_DAMAGE_DIVISOR);
        BtechApplyArmorAndInternalDamage(contactedUnitSlot,COMBAT_ARMOR_RIGHT_LEG,collisionDamage/CONTACT_LIMB_DAMAGE_DIVISOR);
        collisionDamage=(int16_t)BtechShiftSignedDwordRight(contactedCollisionFactor,CONTACT_WEIGHT_SHIFT);
        BtechApplyArmorAndInternalDamage(movingUnitSlot,COMBAT_ARMOR_CENTER_TORSO,collisionDamage);
        BtechApplyArmorAndInternalDamage(movingUnitSlot,COMBAT_ARMOR_LEFT_LEG,collisionDamage/CONTACT_LIMB_DAMAGE_DIVISOR);
        BtechApplyArmorAndInternalDamage(movingUnitSlot,COMBAT_ARMOR_RIGHT_LEG,collisionDamage/CONTACT_LIMB_DAMAGE_DIVISOR);
        BtechClearObjectMotionTail(contactedMechBinding->positionedLeftLeg); /* Reload binding after collisionDamage. */
        uint32_t collisionFactorDifference=movingCollisionFactor-contactedCollisionFactor;
        int16_t knockdownThreshold=unitControls[contactedUnitSlot].conditionScoreOrExitCode==COMBAT_MECH_DISABLED_VALUE?CONTACT_DISABLED_FALL_THRESHOLD:CONTACT_FALL_THRESHOLD;
        if ((int32_t)collisionFactorDifference>0
            && (int32_t)BtechDivideSignedDword(collisionFactorDifference,(uint32_t)contactedTonnageSquared)>knockdownThreshold)
        {unitControls[contactedUnitSlot].animationRequestFlags|=ANIMATION_REQUEST_FALL_POSE;return;}
        if ((int32_t)collisionFactorDifference<0
            && (int32_t)BtechDivideSignedDword(0-collisionFactorDifference,(uint32_t)movingTonnageSquared)>CONTACT_FALL_THRESHOLD)
            unitControls[movingUnitSlot].animationRequestFlags|=ANIMATION_REQUEST_FALL_POSE;
        return;
    }
    int16_t collisionDamage;
    if (absoluteHeadingDifference<CONTACT_FOLLOWING_ANGLE)
    {
        int16_t relativeForwardStep=(uint16_t)movingForwardStep-(uint16_t)contactedForwardStep;
        uint32_t relativeCollisionFactor=(int16_t)((int32_t)relativeForwardStep*movingTonnageSquared);
        collisionDamage=(int16_t)BtechShiftSignedDwordRight(relativeCollisionFactor,CONTACT_WEIGHT_SHIFT);
        BtechApplyArmorAndInternalDamage(contactedUnitSlot,COMBAT_ARMOR_REAR_CENTER_TORSO,collisionDamage);
        BtechApplyArmorAndInternalDamage(contactedUnitSlot,COMBAT_ARMOR_LEFT_LEG,collisionDamage/CONTACT_LIMB_DAMAGE_DIVISOR);
        BtechApplyArmorAndInternalDamage(contactedUnitSlot,COMBAT_ARMOR_RIGHT_LEG,collisionDamage/CONTACT_LIMB_DAMAGE_DIVISOR);
    }
    else
    {
        uint16_t struckSideIndex=headingDifference>0?1:0;
        collisionDamage=(int16_t)BtechShiftSignedDwordRight(movingCollisionFactor,CONTACT_WEIGHT_SHIFT);
        BtechApplyArmorAndInternalDamage(contactedUnitSlot,(int16_t)(struckSideIndex+COMBAT_ARMOR_LEFT_TORSO),collisionDamage);
        BtechApplyArmorAndInternalDamage(contactedUnitSlot,(int16_t)struckSideIndex,collisionDamage/CONTACT_LIMB_DAMAGE_DIVISOR);
        BtechApplyArmorAndInternalDamage(contactedUnitSlot,(int16_t)(struckSideIndex+COMBAT_ARMOR_LEFT_LEG),collisionDamage/CONTACT_LIMB_DAMAGE_DIVISOR);
    }
    collisionDamage=(int16_t)(((uint16_t)collisionDamage>>CONTACT_SELF_DAMAGE_SHIFT)|((uint16_t)collisionDamage&0x8000));
    BtechApplyArmorAndInternalDamage(movingUnitSlot,COMBAT_ARMOR_CENTER_TORSO,collisionDamage);
    if (absoluteHeadingDifference<CONTACT_FOLLOWING_ANGLE)
    {
        BtechApplyArmorAndInternalDamage(movingUnitSlot,COMBAT_ARMOR_LEFT_LEG,collisionDamage/CONTACT_LIMB_DAMAGE_DIVISOR);
        BtechApplyArmorAndInternalDamage(movingUnitSlot,COMBAT_ARMOR_RIGHT_LEG,collisionDamage/CONTACT_LIMB_DAMAGE_DIVISOR);
    }
    else
    {
        BtechApplyArmorAndInternalDamage(movingUnitSlot,COMBAT_ARMOR_RIGHT_LEG,collisionDamage/CONTACT_LIMB_DAMAGE_DIVISOR);
        BtechApplyArmorAndInternalDamage(movingUnitSlot,COMBAT_ARMOR_LEFT_LEG,collisionDamage/CONTACT_LIMB_DAMAGE_DIVISOR);
    }
}

/*0810:91E2. Original player motion, landing/contact rollback and part sync.
 * Do not install player AI context before89F6: native only sets7752 at the end. */
void BtechUpdatePlayerMechMotion(void)
{
    if (!playerUnitBinding || !playerUnitBinding->positionedLeftLeg || !playerMechTemplate || !playerMechState) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !playerUnitBinding || !playerUnitBinding->positionedLeftLeg || !playerMechTemplate || !playerMechState");
    BtechPooledWorldObject *playerMechObject=playerUnitBinding->positionedLeftLeg;
    BtechWorldObjectPrefix *playerMechPose=&playerMechObject->object.prefix;
    BtechPosition3D savedPosition=playerMechPose->position;
    uint16_t savedOrientation[3]={playerMechPose->orientationA,playerMechPose->orientationB,playerMechPose->orientationC};
    if (unitControls[0].movementMode==MOVEMENT_MODE_JUMP)
    {
        playerMechObject->orientationSteps[1]=0;playerMechObject->orientationSteps[0]=0;playerSpecialTargetLandingLatch=0;
        BtechAdvancePooledObjectMotion(playerMechObject);
        uint32_t groundHeight=0;
        if (BtechQueryWorldSurface(&playerMechObject->surfaceQuery,&playerMechPose->position,selectedWorldObjects,(int16_t)selectedObjectCount))
            groundHeight=playerMechObject->surfaceQuery.surfacePositionC;
        uint16_t chassisDisplayScale=playerMechTemplate->displayScale;
        int16_t chassisHalfHeight=(chassisDisplayScale>>1)|(chassisDisplayScale&0x8000);
        uint32_t feetAltitude=playerMechPose->position.positionC-(uint32_t)chassisHalfHeight;
        if ((int32_t)feetAltitude<=(int32_t)groundHeight)
        {
            chassisDisplayScale=playerMechTemplate->displayScale;chassisHalfHeight=(int16_t)((chassisDisplayScale>>1)|(chassisDisplayScale&0x8000));
            playerMechPose->position.positionC=groundHeight+(uint32_t)chassisHalfHeight;
            if (playerMechObject->positionSteps[2]<PLAYER_HARD_LANDING_STEP)
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
            playerMechObject->positionSteps[2]=0;
        }
        else if (BtechResolveUnitContactWithSpecialTarget(0))
        {
            BtechPlaySoundIfEnabled(MOVEMENT_IMPACT_SOUND,MOVEMENT_IMPACT_SOUND_PRIORITY);
            playerImpactThisTick=1;playerMechObject->forwardStepScale=0;
            if ((int32_t)savedPosition.positionC<PLAYER_SPECIAL_LANDING_HEIGHT)
            {playerMechPose->position.positionA=savedPosition.positionA;playerMechPose->position.positionB=savedPosition.positionB;}
            else
            {
                /*9367..9379 computes a height comparison then discards AX. No height clamp. */
                playerMechObject->positionSteps[2]=0;unitControls[0].movementMode=MOVEMENT_MODE_LANDING;
                unitControls[0].animationRequestFlags|=ANIMATION_REQUEST_LANDING;playerSpecialTargetLandingLatch=1;
            }
        }
    }
    else if ((int16_t)playerMechState->gyroHits>=COMBAT_GYRO_DESTROYED_HITS
        || (int16_t)playerMechState->engineHits>=COMBAT_ENGINE_HIT_LIMIT
        || unitControls[0].movementMode==MOVEMENT_MODE_FALLEN_POSE)
    {playerMechObject->orientationSteps[1]=0;playerMechObject->orientationSteps[0]=0;}
    else
    {
        BtechAdvancePooledObjectMotion(playerMechObject);
        uint16_t groundSurfaceClass=BtechResolveGroundSurfaceAndSlope(playerMechObject,0,selectedWorldObjects,(int16_t)selectedObjectCount);
        playerMechObject->orientationSteps[1]=0;playerMechPose->orientationB=0;
        if (playerMechObject->forwardStepScale)
        {
            if (BtechTestUnitMovementObstruction(0) || groundSurfaceClass==SURFACE_BLOCKS_GROUND_MOVEMENT || playerSpecialTargetLandingLatch)
            {
                BtechPlaySoundIfEnabled(MOVEMENT_IMPACT_SOUND,MOVEMENT_IMPACT_SOUND_PRIORITY);playerImpactThisTick=1;
                BtechClearObjectMotionTail(playerMechObject);
                playerMechPose->position=savedPosition;
                playerMechPose->orientationA=savedOrientation[0];playerMechPose->orientationB=savedOrientation[1];playerMechPose->orientationC=savedOrientation[2];
            }
            else
            {
                uint16_t chassisDisplayScale=playerMechTemplate->displayScale;
                playerMechPose->position.positionC+=(uint32_t)(int16_t)((chassisDisplayScale>>1)|(chassisDisplayScale&0x8000));
                playerMechState->doubleCoolingFlag=groundSurfaceClass==SURFACE_DOUBLE_COOLING;
            }
        }
        else
        {
            playerMechPose->position=savedPosition;
            if (groundSurfaceClass==SURFACE_BLOCKS_GROUND_MOVEMENT || playerSpecialTargetLandingLatch)
            {playerMechPose->orientationB=0;playerMechPose->orientationA=0;playerMechObject->orientationSteps[1]=0;playerMechObject->orientationSteps[0]=0;}
        }
    }
    activeUnitControl=&unitControls[0];
    BtechSynchronizeMechRenderParts();
}
