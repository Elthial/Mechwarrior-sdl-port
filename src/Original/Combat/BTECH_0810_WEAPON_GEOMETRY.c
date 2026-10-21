#include "mechwarrior/port_diagnostics.h"
#include "mechwarrior/combat_ballistics.h"
#include "mechwarrior/combat_pose.h"
#include "mechwarrior/combat_render.h"
#include "mechwarrior/combat_scene.h"
#include "mechwarrior/combat_video.h"
#include "mechwarrior/combat_effects.h"
#include <stdlib.h>

/*0810:491A. Endpoint effect first, then a shortened non-machine-gun beam. */
void BtechBuildPlayerDirectShotGeometry(int16_t distance,const BtechWeaponMount *mount,uint8_t impactEffect)
{
    distance=(int16_t)(distance-DIRECT_SHOT_ENDPOINT_MARGIN);
    if (!playerUnitBinding || !playerUnitBinding->positionedLeftLeg || !playerPoseObject || !playerMechState) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !playerUnitBinding || !playerUnitBinding->positionedLeftLeg || !playerPoseObject || !playerMechState");
    BtechPosition3D origin=playerUnitBinding->positionedLeftLeg->object.prefix.position;
    const BtechWorldObjectPrefix *pose=&playerPoseObject->object.prefix;
    uint16_t orientation[]={pose->orientationA,pose->orientationB,pose->orientationC};
    orientation[0]-=playerMechState->aimPitch;
    int16_t direction[3];BtechBuildDirectionVectorFromOrientation(direction,orientation);
    BtechPosition3D endpoint;
    uint32_t displacement=BtechShiftSignedDwordRight((uint32_t)BtechMultiplySignedWords(direction[0],distance),ORIENTATION_FRACTION_BITS);
    endpoint.positionA=pose->position.positionA+displacement;
    displacement=BtechShiftSignedDwordRight((uint32_t)BtechMultiplySignedWords(direction[1],distance),ORIENTATION_FRACTION_BITS);
    endpoint.positionB=pose->position.positionB+displacement;
    displacement=BtechShiftSignedDwordRight((uint32_t)BtechMultiplySignedWords(direction[2],distance),ORIENTATION_FRACTION_BITS);
    endpoint.positionC=pose->position.positionC+displacement;
    uint16_t machineGun=mount->weaponKind==COMBAT_WEAPON_MACHINE_GUN;
    if (impactEffect) BtechSpawnSequencedWorldEffect(&endpoint,machineGun);
    if (!machineGun)
    {
        if (!playerUnitBinding || !playerUnitBinding->torso || mount->bodyLocation>=COMBAT_BODY_LOCATIONS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !playerUnitBinding || !playerUnitBinding->torso || mount->bodyLocation>=COMBAT_BODY_LOCATIONS");
        origin=playerUnitBinding->torso->object.prefix.position;
        int16_t lateral=muzzleLateralOffsetsByBodyLocation[mount->bodyLocation];
        origin.positionA-=BtechShiftSignedDwordRight((uint32_t)BtechMultiplySignedWords((int16_t)BtechLookupCosine(orientation[2]),lateral),ORIENTATION_FRACTION_BITS);
        origin.positionB-=BtechShiftSignedDwordRight((uint32_t)BtechMultiplySignedWords((int16_t)BtechLookupSine(orientation[2]),lateral),ORIENTATION_FRACTION_BITS);
        uint32_t difference=endpoint.positionA-pose->position.positionA;
        endpoint.positionA=pose->position.positionA+(difference>>1)+(difference&0x80000000);
        difference=endpoint.positionB-pose->position.positionB;
        endpoint.positionB=pose->position.positionB+(difference>>1)+(difference&0x80000000);
        difference=endpoint.positionC-pose->position.positionC;
        endpoint.positionC=pose->position.positionC+(difference>>1)+(difference&0x80000000);
        BtechBuildWeaponBeamGeometry(&origin,&endpoint,mount->weaponKind);
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
    const BtechPosition3D *view=&playerUnitBinding->positionedLeftLeg->object.prefix.position;
    uint16_t halfWidth=beamHalfWidths[weaponKind];
    for (uint16_t slot=0;slot<COMBAT_BEAM_SLOTS;++slot)
    {
        if (beamSlotOccupied[slot]) continue;
        beamSlotOccupied[slot]=1;
        uint32_t endpointDistance=BtechApproximatePlanarDistance(view,endpoint);
        uint32_t originDistance=BtechApproximatePlanarDistance(view,origin);
        BtechPosition3D anchor=(int32_t)originDistance>=(int32_t)endpointDistance?*endpoint:*origin;
        uint16_t heading=BtechCalculateVectorHeading(endpoint->positionA-origin->positionA,endpoint->positionB-origin->positionB);
        uint16_t widthA=(uint16_t)BtechShiftSignedDwordRight((uint32_t)BtechMultiplySignedWords((int16_t)BtechLookupCosine(heading),(int16_t)halfWidth),ORIENTATION_FRACTION_BITS);
        uint16_t widthB=(uint16_t)BtechShiftSignedDwordRight((uint32_t)BtechMultiplySignedWords((int16_t)BtechLookupSine(heading),(int16_t)halfWidth),ORIENTATION_FRACTION_BITS);
        uint16_t (*vertices)[3]=beamGeometry[slot];
        vertices[1][0]=(uint16_t)(endpoint->positionA-anchor.positionA);
        vertices[1][1]=(uint16_t)(endpoint->positionB-anchor.positionB);
        vertices[1][2]=(uint16_t)(endpoint->positionC-anchor.positionC);
        uint16_t relativeA=origin->positionA-anchor.positionA;
        uint16_t relativeB=origin->positionB-anchor.positionB;
        uint16_t relativeC=origin->positionC-anchor.positionC;
        vertices[2][0]=relativeA;vertices[2][1]=relativeB;vertices[2][2]=relativeC-halfWidth;
        vertices[3][0]=relativeA;vertices[3][1]=relativeB;vertices[3][2]=relativeC+halfWidth;
        vertices[4][0]=relativeA+widthA;vertices[4][1]=relativeB+widthB;vertices[4][2]=relativeC;
        vertices[5][0]=relativeA-widthA;vertices[5][1]=relativeB-widthB;vertices[5][2]=relativeC;
        BtechShapeResource *resource=&beamResources[slot];
        resource->flags=0;resource->scaleShift=0;
        uint32_t differences[]={origin->positionA-endpoint->positionA,origin->positionB-endpoint->positionB,origin->positionC-endpoint->positionC};
        uint16_t extent=0;
        for (uint16_t axis=0;axis<3;++axis)
        {
            uint32_t difference=differences[axis];
            uint16_t span=difference==0x8000?0x7FFF:(int32_t)difference<0?(uint16_t)(0-difference):(uint16_t)difference;
            extent+=span;
        }
        resource->baseScale=(int16_t)extent;
        for (uint16_t axis=0;axis<3;++axis) resource->referenceVector[axis]=0;
        resource->entryCount=1;resource->entries=&beamRenderEntries[slot];
        BtechRenderEntry *entry=&beamRenderEntries[slot];
        entry->firstVectorIndex=0;entry->vectorCount=COMBAT_BEAM_VECTORS;
        entry->vectors=vertices;entry->variantCount=1;
        entry->variants=weaponKind==COMBAT_WEAPON_PPC?&ppcBeamVariant:&laserBeamVariant;
        uint16_t resourceId=otherShapeResourceBase+slot+COMBAT_BEAM_RESOURCE_DELTA;
        BtechSetShapeResourceEntryAddress(resourceId,resource);
        BtechWorldObjectPrefix *object=&beamWorldObjects[slot].prefix;
        object->resourceId=resourceId;
        uint32_t scale=BtechGetResourceScaledValue(resourceId);
        object->resourceScaledValue=(object->resourceScaledValue&0xFFFF0000)|(scale&0xFFFF);
        object->resourceScaledValue=(object->resourceScaledValue&0xFFFF)|(scale&0xFFFF0000);
        object->position=anchor;object->updateCallbackOffset=WORLD_OBJECT_SELECTION_PIXEL_CALLBACK;
        if (worldObjectCount>=COMBAT_WORLD_OBJECTS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: worldObjectCount>=COMBAT_WORLD_OBJECTS");
        heapWorldObjectOwned[worldObjectCount]=0;
        worldObjects[worldObjectCount++]=object;
        if (cockpitWorldObjectCount>=COMBAT_WORLD_OBJECTS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: cockpitWorldObjectCount>=COMBAT_WORLD_OBJECTS");
        cockpitWorldObjects[cockpitWorldObjectCount++]=object;
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
