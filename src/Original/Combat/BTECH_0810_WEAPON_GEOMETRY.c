#include "mechwarrior/port_diagnostics.h"
#include "mechwarrior/combat_ballistics.h"
#include "mechwarrior/combat_pose.h"
#include "mechwarrior/combat_render.h"
#include "mechwarrior/combat_scene.h"
#include "mechwarrior/combat_video.h"
#include "mechwarrior/combat_effects.h"
#include <stdlib.h>

/*0810:491A. Endpoint effect first, then a shortened non-machine-gun beam. */
void BtechBuildPlayerDirectShotGeometry(int16_t shotDistance,const BtechWeaponMount *weaponMount,uint8_t spawnImpactEffect)
{
    shotDistance=(int16_t)(shotDistance-DIRECT_SHOT_ENDPOINT_MARGIN);
    if (!playerUnitBinding || !playerUnitBinding->positionedLeftLeg || !playerPoseObject || !playerMechState) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !playerUnitBinding || !playerUnitBinding->positionedLeftLeg || !playerPoseObject || !playerMechState");
    const BtechWorldObjectPrefix *aimPose=&playerPoseObject->object.prefix;
    uint16_t shotOrientation[]={aimPose->orientationA,aimPose->orientationB,aimPose->orientationC};
    shotOrientation[0]-=playerMechState->aimPitch;
    int16_t shotDirection[3];BtechBuildDirectionVectorFromOrientation(shotDirection,shotOrientation);
    BtechPosition3D impactPosition;
    uint32_t shotDisplacement=BtechShiftSignedDwordRight((uint32_t)BtechMultiplySignedWords(shotDirection[0],shotDistance),ORIENTATION_FRACTION_BITS);
    impactPosition.positionA=aimPose->position.positionA+shotDisplacement;
    shotDisplacement=BtechShiftSignedDwordRight((uint32_t)BtechMultiplySignedWords(shotDirection[1],shotDistance),ORIENTATION_FRACTION_BITS);
    impactPosition.positionB=aimPose->position.positionB+shotDisplacement;
    shotDisplacement=BtechShiftSignedDwordRight((uint32_t)BtechMultiplySignedWords(shotDirection[2],shotDistance),ORIENTATION_FRACTION_BITS);
    impactPosition.positionC=aimPose->position.positionC+shotDisplacement;
    uint16_t isMachineGun=weaponMount->weaponKind==COMBAT_WEAPON_MACHINE_GUN;
    if (spawnImpactEffect) BtechSpawnSequencedWorldEffect(&impactPosition,isMachineGun);
    if (!isMachineGun)
    {
        if (!playerUnitBinding || !playerUnitBinding->torso || weaponMount->bodyLocation>=COMBAT_BODY_LOCATIONS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !playerUnitBinding || !playerUnitBinding->torso || weaponMount->bodyLocation>=COMBAT_BODY_LOCATIONS");
        BtechPosition3D muzzlePosition=playerUnitBinding->torso->object.prefix.position;
        int16_t muzzleLateralOffset=muzzleLateralOffsetsByBodyLocation[weaponMount->bodyLocation];
        muzzlePosition.positionA-=BtechShiftSignedDwordRight((uint32_t)BtechMultiplySignedWords((int16_t)BtechLookupCosine(shotOrientation[2]),muzzleLateralOffset),ORIENTATION_FRACTION_BITS);
        muzzlePosition.positionB-=BtechShiftSignedDwordRight((uint32_t)BtechMultiplySignedWords((int16_t)BtechLookupSine(shotOrientation[2]),muzzleLateralOffset),ORIENTATION_FRACTION_BITS);
        uint32_t impactOffset=impactPosition.positionA-aimPose->position.positionA;
        impactPosition.positionA=aimPose->position.positionA+(impactOffset>>1)+(impactOffset&0x80000000);
        impactOffset=impactPosition.positionB-aimPose->position.positionB;
        impactPosition.positionB=aimPose->position.positionB+(impactOffset>>1)+(impactOffset&0x80000000);
        impactOffset=impactPosition.positionC-aimPose->position.positionC;
        impactPosition.positionC=aimPose->position.positionC+(impactOffset>>1)+(impactOffset&0x80000000);
        BtechBuildWeaponBeamGeometry(&muzzlePosition,&impactPosition,weaponMount->weaponKind);
    }
}

/*0810:BB9C. Native roll merely selects the full pitch branch; it is not used
 * as another rotation. Preserve the live orientation reads/output write order. */
void BtechBuildDirectionVectorFromOrientation(int16_t direction[3],const uint16_t orientation[3])
{
    uint16_t pitch=orientation[0];
    if (pitch|orientation[1])
    {
        int16_t cosine=(int16_t)BtechLookupCosine(pitch);
        direction[0]=(int16_t)BtechShiftSignedDwordRight((uint32_t)BtechMultiplySignedWords((int16_t)BtechLookupSine(orientation[2]),(int16_t)(0-(uint16_t)cosine)),ORIENTATION_FRACTION_BITS);
        direction[1]=(int16_t)BtechShiftSignedDwordRight((uint32_t)BtechMultiplySignedWords((int16_t)BtechLookupCosine(orientation[2]),cosine),ORIENTATION_FRACTION_BITS);
        direction[2]=(int16_t)BtechLookupSine(orientation[0]);
    }
    else if (orientation[2])
    {
        direction[0]=(int16_t)(0-BtechLookupSine(orientation[2]));
        direction[1]=(int16_t)BtechLookupCosine(orientation[2]);direction[2]=0;
    }
    else {direction[1]=1<<ORIENTATION_FRACTION_BITS;direction[2]=0;direction[0]=0;}
}

/*0810:BC6A. One transient six-vector resource; vertex zero stays untouched. */
void BtechBuildWeaponBeamGeometry(const BtechPosition3D *origin,const BtechPosition3D *endpoint,int16_t weaponKind)
{
    if (!playerUnitBinding || !playerUnitBinding->positionedLeftLeg || (uint16_t)weaponKind>=COMBAT_WEAPON_KINDS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !playerUnitBinding || !playerUnitBinding->positionedLeftLeg || (uint16_t)weaponKind>=COMBAT_WEAPON_KINDS");
    const BtechPosition3D *viewPosition=&playerUnitBinding->positionedLeftLeg->object.prefix.position;
    uint16_t beamHalfWidth=beamHalfWidths[weaponKind];
    for (uint16_t beamSlot=0;beamSlot<COMBAT_BEAM_SLOTS;++beamSlot)
    {
        if (beamSlotOccupied[beamSlot]) continue;
        beamSlotOccupied[beamSlot]=1;
        uint32_t endpointViewDistance=BtechApproximatePlanarDistance(viewPosition,endpoint);
        uint32_t originViewDistance=BtechApproximatePlanarDistance(viewPosition,origin);
        BtechPosition3D beamAnchor=(int32_t)originViewDistance>=(int32_t)endpointViewDistance?*endpoint:*origin;
        uint16_t beamHeading=BtechCalculateVectorHeading(endpoint->positionA-origin->positionA,endpoint->positionB-origin->positionB);
        uint16_t beamWidthA=(uint16_t)BtechShiftSignedDwordRight((uint32_t)BtechMultiplySignedWords((int16_t)BtechLookupCosine(beamHeading),(int16_t)beamHalfWidth),ORIENTATION_FRACTION_BITS);
        uint16_t beamWidthB=(uint16_t)BtechShiftSignedDwordRight((uint32_t)BtechMultiplySignedWords((int16_t)BtechLookupSine(beamHeading),(int16_t)beamHalfWidth),ORIENTATION_FRACTION_BITS);
        uint16_t (*beamVertices)[3]=beamGeometry[beamSlot];
        beamVertices[1][0]=(uint16_t)(endpoint->positionA-beamAnchor.positionA);
        beamVertices[1][1]=(uint16_t)(endpoint->positionB-beamAnchor.positionB);
        beamVertices[1][2]=(uint16_t)(endpoint->positionC-beamAnchor.positionC);
        uint16_t originOffsetA=origin->positionA-beamAnchor.positionA;
        uint16_t originOffsetB=origin->positionB-beamAnchor.positionB;
        uint16_t originOffsetC=origin->positionC-beamAnchor.positionC;
        beamVertices[2][0]=originOffsetA;beamVertices[2][1]=originOffsetB;beamVertices[2][2]=originOffsetC-beamHalfWidth;
        beamVertices[3][0]=originOffsetA;beamVertices[3][1]=originOffsetB;beamVertices[3][2]=originOffsetC+beamHalfWidth;
        beamVertices[4][0]=originOffsetA+beamWidthA;beamVertices[4][1]=originOffsetB+beamWidthB;beamVertices[4][2]=originOffsetC;
        beamVertices[5][0]=originOffsetA-beamWidthA;beamVertices[5][1]=originOffsetB-beamWidthB;beamVertices[5][2]=originOffsetC;
        BtechShapeResource *beamResource=&beamResources[beamSlot];
        beamResource->flags=0;beamResource->scaleShift=0;
        uint32_t endpointOffsets[]={origin->positionA-endpoint->positionA,origin->positionB-endpoint->positionB,origin->positionC-endpoint->positionC};
        uint16_t beamExtent=0;
        for (uint16_t axis=0;axis<3;++axis)
        {
            uint32_t endpointOffset=endpointOffsets[axis];
            uint16_t axisExtent=endpointOffset==0x8000?0x7FFF:(int32_t)endpointOffset<0?(uint16_t)(0-endpointOffset):(uint16_t)endpointOffset;
            beamExtent+=axisExtent;
        }
        beamResource->baseScale=(int16_t)beamExtent;
        for (uint16_t axis=0;axis<3;++axis) beamResource->referenceVector[axis]=0;
        beamResource->entryCount=1;beamResource->entries=&beamRenderEntries[beamSlot];
        BtechRenderEntry *beamEntry=&beamRenderEntries[beamSlot];
        beamEntry->firstVectorIndex=0;beamEntry->vectorCount=COMBAT_BEAM_VECTORS;
        beamEntry->vectors=beamVertices;beamEntry->variantCount=1;
        beamEntry->variants=weaponKind==COMBAT_WEAPON_PPC?&ppcBeamVariant:&laserBeamVariant;
        uint16_t beamResourceId=otherShapeResourceBase+beamSlot+COMBAT_BEAM_RESOURCE_DELTA;
        BtechSetShapeResourceEntryAddress(beamResourceId,beamResource);
        BtechWorldObjectPrefix *beamObject=&beamWorldObjects[beamSlot].prefix;
        beamObject->resourceId=beamResourceId;
        beamObject->resourceScaledValue=BtechGetResourceScaledValue(beamResourceId);
        beamObject->position=beamAnchor;beamObject->updateCallbackOffset=WORLD_OBJECT_SELECTION_PIXEL_CALLBACK;
        if (worldObjectCount>=COMBAT_WORLD_OBJECTS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: worldObjectCount>=COMBAT_WORLD_OBJECTS");
        heapWorldObjectOwned[worldObjectCount]=0;
        worldObjects[worldObjectCount++]=beamObject;
        if (cockpitWorldObjectCount>=COMBAT_WORLD_OBJECTS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: cockpitWorldObjectCount>=COMBAT_WORLD_OBJECTS");
        cockpitWorldObjects[cockpitWorldObjectCount++]=beamObject;
        return;
    }
    BtechShowFatalErrorAndExit("Could not allocate laser object..."); /*304E:279E. */
}

/*0810:C004. Remove membership only; beam geometry/resource headers stay live. */
void BtechUnregisterTransientWorldObjects(void)
{
    for (uint16_t slot=0;slot<COMBAT_BEAM_SLOTS;++slot)
        if (beamSlotOccupied[slot])
        {beamSlotOccupied[slot]=0;BtechUnregisterWorldObject(&beamWorldObjects[slot].prefix);}
}

/*0810:6002. Both products use the live inputs before either output is stored. */
void BtechRotateSignedWordPair(int16_t *componentA,int16_t *componentB,uint16_t angle)
{
    int16_t sine=(int16_t)BtechLookupSine(angle);
    int16_t cosine=(int16_t)BtechLookupCosine(angle);
    uint32_t difference=(uint32_t)BtechMultiplySignedWords(*componentA,cosine)
        -(uint32_t)BtechMultiplySignedWords(*componentB,sine);
    int16_t rotatedA=(int16_t)BtechShiftSignedDwordRight(difference,ORIENTATION_FRACTION_BITS);
    uint32_t sum=(uint32_t)BtechMultiplySignedWords(*componentA,sine)
        +(uint32_t)BtechMultiplySignedWords(*componentB,cosine);
    int16_t rotatedB=(int16_t)BtechShiftSignedDwordRight(sum,ORIENTATION_FRACTION_BITS);
    *componentA=rotatedA;*componentB=rotatedB;
}

/*0810:AC0F. Return the one shared scratch position; next call overwrites it. */
BtechPosition3D *BtechGetUnitBodyReferencePosition(uint16_t unitSlot,int16_t referenceIndex)
{
    if (unitSlot>=COMBAT_MECH_SLOTS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: unitSlot>=COMBAT_MECH_SLOTS");
    BtechUnitBinding *binding=&unitBindings[unitSlot];
    if (!binding->positionedLeftLeg || !binding->mech) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !binding->positionedLeftLeg || !binding->mech");
    BtechWorldObjectPrefix *body=&binding->positionedLeftLeg->object.prefix;
    bodyReferenceWorldPosition=body->position;
    uint16_t chassis=binding->mech->templateIndex;
    if (chassis>=COMBAT_MECH_TEMPLATES || (uint16_t)referenceIndex>=COMBAT_ARMOR_LOCATIONS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: chassis>=COMBAT_MECH_TEMPLATES || (uint16_t)referenceIndex>=COMBAT_ARMOR_LOCATIONS");
    uint16_t mode=unitControls[unitSlot].movementMode;
    const BtechBodyReferencePoint *reference=mode==MOVEMENT_MODE_FALLEN_POSE || mode==MOVEMENT_MODE_FALLING
        ?&fallenBodyReferenceBanks[chassis][referenceIndex]:&mechTemplates[chassis].bodyReferencePoints[referenceIndex];
    int16_t componentA=reference->componentA,componentB=reference->componentB;
    uint16_t heading=body->orientationC;
    if (referenceIndex!=COMBAT_BODY_LEFT_LEG && referenceIndex!=COMBAT_BODY_RIGHT_LEG)
    {
        if (!binding->torso) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !binding->torso");
        heading+=binding->torso->object.prefix.orientationC;
    }
    BtechRotateSignedWordPair(&componentA,&componentB,heading);
    bodyReferenceWorldPosition.positionA+=(uint32_t)componentA;
    bodyReferenceWorldPosition.positionB+=(uint32_t)componentB;
    bodyReferenceWorldPosition.positionC+=(uint32_t)reference->componentC;
    return &bodyReferenceWorldPosition;
}
