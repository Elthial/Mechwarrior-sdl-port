#include "mechwarrior/combat_runtime.h"
#include "mechwarrior/combat_import.h"
#include "mechwarrior/combat_heap.h"
#include "mechwarrior/combat_world.h"
#include "mechwarrior/combat_resources.h"
#include "mechwarrior/native_address.h"
#include "mechwarrior/combat_startup.h"
#include <stdlib.h>
#include <string.h>

/*0810:AFE1. Heap objects clear0030, unlike pooled objects. Host sizeof is
 * not a serialized length. The common prefix is shared through real pointers. */
BtechWorldObjectPrefix *BtechAllocateHeapWorldObject(uint16_t resourceId,const BtechPosition3D *placement)
{
    BtechHeapWorldObject *object=BtechAllocateNearBytes(BTECH_NEAR_WORLD_OBJECT_BYTES,sizeof(*object),BTECH_NEAR_WORLD_OBJECT);
    if (!object || worldObjectCount>=COMBAT_WORLD_OBJECTS) abort();
    *object=(BtechHeapWorldObject){0};
    object->prefix.updateCallbackOffset=WORLD_OBJECT_SELECTION_PIXEL_CALLBACK;
    object->prefix.resourceId=resourceId;
    if (placement) object->prefix.position=*placement;
    uint16_t registryIndex=worldObjectCount;
    heapWorldObjectOwned[registryIndex]=1;++worldObjectCount;
    worldObjects[registryIndex]=&object->prefix;
    return &object->prefix;
}

/*0810:B163. Reclaimed entries retain every field not assigned here. */
BtechPooledWorldObject *BtechAllocatePooledWorldObject(uint16_t resourceId,const BtechPosition3D *placement)
{
    for (uint16_t slot=0;slot<COMBAT_POOLED_OBJECTS;++slot)
    {
        BtechPooledWorldObject *object=&pooledWorldObjects[slot];
        if (object->occupied) continue;
        BtechWorldObjectPrefix *prefix=&object->object.prefix;
        prefix->flags|=WORLD_OBJECT_POOLED;object->occupied=1;
        prefix->resourceId=resourceId;prefix->updateCallbackOffset=WORLD_OBJECT_SELECTION_PIXEL_CALLBACK;
        if (!(resourceId&0x8000)) prefix->resourceScaledValue=BtechGetResourceScaledValue(resourceId);
        if (placement) prefix->position=*placement;
        if (worldObjectCount>=COMBAT_WORLD_OBJECTS || cockpitWorldObjectCount>=COMBAT_WORLD_OBJECTS) abort();
        heapWorldObjectOwned[worldObjectCount]=0;
        worldObjects[worldObjectCount++]=prefix;
        cockpitWorldObjects[cockpitWorldObjectCount++]=&object->object.prefix;
        return object;
    }
    /* B203 passes304E:2778 to the game's fatal renderer/cleanup, not CRT abort. */
    BtechShowFatalErrorAndExit("Could not allocate dynamic object...");
    abort(); /* The native fatal exit must not return an invented object. */
}

/*0810:B0FE. Swap-last removal leaves stale tail entries; no storage is freed.
 * Native first search is unbounded. Foreign/adjacent-owner search is unsupported. */
void BtechUnregisterWorldObject(BtechWorldObjectPrefix *object)
{
    uint16_t slot=0;
    while (slot<COMBAT_WORLD_OBJECTS && worldObjects[slot]!=object) ++slot;
    if (slot==COMBAT_WORLD_OBJECTS) abort();
    --worldObjectCount;
    if (worldObjectCount>=COMBAT_WORLD_OBJECTS) abort();
    worldObjects[slot]=worldObjects[worldObjectCount];
    /* Host allocation provenance travels with the reference, never native game state. */
    heapWorldObjectOwned[slot]=heapWorldObjectOwned[worldObjectCount];
    for (uint16_t cockpitSlot=0;(int16_t)cockpitSlot<(int16_t)cockpitWorldObjectCount;++cockpitSlot)
    {
        if (cockpitSlot>=COMBAT_WORLD_OBJECTS) abort();
        if (cockpitWorldObjects[cockpitSlot]!=object) continue;
        --cockpitWorldObjectCount;
        if (cockpitWorldObjectCount>=COMBAT_WORLD_OBJECTS) abort();
        cockpitWorldObjects[cockpitSlot]=cockpitWorldObjects[cockpitWorldObjectCount];
        return;
    }
}

/*0810:B070. Signed live count. Only heap-owned entries are freed/cleared;
 * pooled list references remain stale after the final count reset. */
void BtechReleaseHeapWorldObjectList(void)
{
    for (int16_t slot=0;slot<(int16_t)worldObjectCount;++slot)
    {
        if (slot>=COMBAT_WORLD_OBJECTS) abort();
        if (heapWorldObjectOwned[slot])
        {
            BtechReleaseNearHeapBlock(worldObjects[slot]);
            worldObjects[slot]=NULL;heapWorldObjectOwned[slot]=0;
        }
    }
    worldObjectCount=0;
}

/*0810:B215. Occupancy-clear phase precedes the whole native1720-byte pool
 * fill. Host pointer members widen the same records; no unregister calls. */
void BtechResetWorldObjectPool(void)
{
    for (uint16_t slot=0;slot<COMBAT_POOLED_OBJECTS;++slot) pooledWorldObjects[slot].occupied=0;
    for (uint16_t slot=0;slot<COMBAT_POOLED_OBJECTS;++slot) pooledWorldObjects[slot]=(BtechPooledWorldObject){0};
}

/*0810:228B. Detach/release every bank before collision and two cameras.
 * Wrapped count+2 and signed loop bound retained. */
void BtechReleaseCombatChassisResourceBanks(void)
{
    for (int16_t bank=0;bank<(int16_t)(distinctSceneMechChassisCount+2);++bank)
        BtechReleaseResourceBankSelector(BtechDetachShapeResourceBank((uint16_t)bank));
    BtechReleaseCollisionBankAllocation();
    BtechReleaseCameraContextFar(orderMapCameraContext);
    BtechReleaseCameraContextFar(playerPoseCameraContext);
}

/*0810:B246. Clear occupancy before unregistering, then clear the whole record. */
void BtechReleasePooledWorldObject(BtechPooledWorldObject *object)
{
    object->occupied=0;
    BtechUnregisterWorldObject(&object->object.prefix);
    *object=(BtechPooledWorldObject){0};
}

/*0810:B0BB. Signed count, live reread; no negative-resource shortcut here. */
void BtechRefreshWorldObjectResourceScales(void)
{
    for (uint16_t index=0;!(worldObjectCount&0x8000) && index<worldObjectCount;++index)
    {
        if (index>=COMBAT_WORLD_OBJECTS || !worldObjects[index]) abort();
        BtechWorldObjectPrefix *object=worldObjects[index];
        object->resourceScaledValue=BtechGetResourceScaledValue(object->resourceId);
    }
}


/*0810:25B0. Native failed allocation writes304E:0001. That unrelated-owner
 * corruption is an explicit host failure, never a successful camera. */
BtechCameraContext *BtechCreateHiddenCameraContext(BtechPooledWorldObject *object)
{
    object->object.prefix.renderFlags|=COMBAT_WORLD_OBJECT_HIDDEN;
    BtechCameraContext *camera=BtechAllocateCameraContext(&object->object.prefix);
    if (!camera) abort();
    camera->projectionShift=CAMERA_PROJECTION_SHIFT;
    return camera;
}

/*0810:25D4. The player Mech, template, six hidden parts and two cameras are
 * separate owners; they are not combined into a new player context. */
void BtechInitializePlayerCameras(void)
{
    playerUnitBinding=&unitBindings[0];
    playerMechState=playerUnitBinding->mech;
    if (!playerMechState || playerMechState->templateIndex>=COMBAT_MECH_TEMPLATES) abort();
    playerMechTemplate=&mechTemplates[playerMechState->templateIndex];
    playerUnitBinding->torso->object.prefix.renderFlags|=COMBAT_WORLD_OBJECT_HIDDEN;
    playerUnitBinding->rightLeg->object.prefix.renderFlags|=COMBAT_WORLD_OBJECT_HIDDEN;
    playerUnitBinding->positionedLeftLeg->object.prefix.renderFlags|=COMBAT_WORLD_OBJECT_HIDDEN;
    playerUnitBinding->rightArm->object.prefix.renderFlags|=COMBAT_WORLD_OBJECT_HIDDEN;
    playerUnitBinding->leftArm->object.prefix.renderFlags|=COMBAT_WORLD_OBJECT_HIDDEN;
    playerUnitBinding->head->object.prefix.renderFlags|=COMBAT_WORLD_OBJECT_HIDDEN;
    playerPoseObject=BtechAllocatePooledWorldObject(otherShapeResourceBase,NULL);
    orderMapCameraObject=BtechAllocatePooledWorldObject(otherShapeResourceBase,NULL);
    BtechWorldObjectPrefix *mapCamera=&orderMapCameraObject->object.prefix;
    mapCamera->orientationB=0;mapCamera->orientationC=0;mapCamera->orientationA=0xC000;
    orderMapCameraContext=BtechCreateHiddenCameraContext(orderMapCameraObject);
    playerPoseCameraContext=BtechCreateHiddenCameraContext(playerPoseObject);
}

/*0810:211B. Count slots01..07; the player-only chassis is deliberately not
 * added. The final two presence bytes are not cleared by2148. */
void BtechLoadCombatChassisResourceBanks(void)
{
    uint16_t chassisBankSlot=2;
    distinctSceneMechChassisCount=0;
    beforeObjectListCallbackOffset=WORLD_LIST_BEFORE_CALLBACK;afterObjectListCallbackOffset=WORLD_LIST_AFTER_CALLBACK;
    memset(sceneChassisResourcePresent,0,8);
    for (uint16_t slot=1;slot<COMBAT_MECH_SLOTS;++slot)
    {
        const BtechMechState *mech=unitBindings[slot].mech;
        if (!mech) continue;
        uint16_t chassis=mech->templateIndex;
        if (chassis>=10) abort();
        if (!sceneChassisResourcePresent[chassis])
        {sceneChassisResourcePresent[chassis]=1;++distinctSceneMechChassisCount;}
    }
    BtechAllocateShapeBankRegistry(distinctSceneMechChassisCount+2);
    BtechResourceBank bank=BtechDecodeAndRelocateResourceBank(BtechBuildFilenameWithExtension("terpck","tbl"));
    BtechBindShapeBankDirectory(bank,0);
    BtechLoadCollisionResourceBank(BtechBuildFilenameWithExtension("terpck","gi"));
    otherShapeResourceBase=BtechCountShapeResourceBankEntries(0);
    bank=BtechDecodeAndRelocateResourceBank(BtechBuildFilenameWithExtension("othpck","tbl"));
    BtechBindShapeBankDirectory(bank,1);
    uint16_t nextResourceBase=otherShapeResourceBase+BtechCountShapeResourceBankEntries(1);
    firstMechShapeResourceId=nextResourceBase;
    for (uint16_t chassis=0;chassis<10;++chassis)
    {
        if (!sceneChassisResourcePresent[chassis]) continue;
        mechResourceBaseIds[chassis]=nextResourceBase;
        if (chassis>=8) abort(); /* Native basename aliases presence WORDs; no invented ninth/tenth chassis. */
        bank=BtechDecodeAndRelocateResourceBank(BtechBuildFilenameWithExtension(sceneChassisBankNames[chassis],"tbl"));
        BtechBindShapeBankDirectory(bank,chassisBankSlot);
        uint16_t previousSlot=chassisBankSlot++;
        nextResourceBase+=BtechCountShapeResourceBankEntries(previousSlot);
    }
    BtechRefreshWorldObjectResourceScales();
}

/*0810:2438. WLD ID and placement are distinct reads. Short later records
 * retain the previous local's bytes; the first incomplete local has unknown
 * native stack residue and is explicitly unsupported, not zero-filled. */
void BtechLoadTerrainWorldResources(void)
{
    if (combatTerrainDetailLevel==TERRAIN_DETAIL_LOW) return;
    uint8_t resourceBytes[2],positionBytes[12];
    uint8_t haveResource=0,havePosition=0;
    for (uint16_t tile=0;tile<TERRAIN_TILE_COUNT;++tile)
    {
        uint8_t variant=scenarioSelectors.terrainSelectors[tile];
        if (variant>=16) abort();
        strcpy(terrainWorldFilename,"TILE");strcat(terrainWorldFilename,terrainVariantFilenameSuffixes[variant]);
        if (combatTerrainDetailLevel==TERRAIN_DETAIL_MEDIUM) strcat(terrainWorldFilename,"B");
        strcpy(terrainGridFilename,terrainWorldFilename);
        strcat(terrainWorldFilename,".WLD");strcat(terrainGridFilename,".GRD");
        FILE *stream=BtechOpenStreamWithRetry(terrainWorldFilename,"rb");
        size_t read;
        while ((read=BtechReadStreamElements(resourceBytes,1,2,stream))!=0)
        {
            if (read<2 && !haveResource) abort();
            haveResource=1;
            read=BtechReadStreamElements(positionBytes,1,12,stream);
            if (read<12 && !havePosition) abort();
            havePosition=1;
            BtechPosition3D placement={
                (uint32_t)MwReadNativeWord(positionBytes,0)|((uint32_t)MwReadNativeWord(positionBytes,2)<<16),
                (uint32_t)MwReadNativeWord(positionBytes,4)|((uint32_t)MwReadNativeWord(positionBytes,6)<<16),
                (uint32_t)MwReadNativeWord(positionBytes,8)|((uint32_t)MwReadNativeWord(positionBytes,10)<<16)};
            BtechWorldObjectPrefix *object=BtechAllocateHeapWorldObject(MwReadNativeWord(resourceBytes,0),&placement);
            object->renderFlags=WORLD_OBJECT_SURFACE_QUERY_CANDIDATE;
        }
        BtechCloseStreamFile(stream);
        stream=BtechOpenStreamWithRetry(terrainGridFilename,"rb");
        uint16_t rowBase=terrainTileGridRowBases[tile],columnBase=terrainTileGridColumnBases[tile];
        for (uint16_t row=0;row<TERRAIN_TILE_ROWS;++row)
        {
            if (rowBase+row>=TERRAIN_GRID_ROWS || columnBase+TERRAIN_TILE_COLUMNS>TERRAIN_GRID_COLUMNS) abort();
            BtechReadStreamElements(&terrainHeightGrid[rowBase+row][columnBase],TERRAIN_TILE_COLUMNS,1,stream);
        }
        BtechCloseStreamFile(stream);
    }
}


/*0810:C75A. Two native DWORD arguments; all pair arithmetic wraps32 bits. */
uint16_t BtechCalculateVectorHeading(uint32_t componentA,uint32_t componentB)
{
    uint8_t negativeA=(componentA&0x80000000)!=0,negativeB=(componentB&0x80000000)!=0;
    if (negativeA) componentA=0u-componentA;
    if (negativeB) componentB=0u-componentB;
    uint16_t angle;
    if (!componentA) angle=0x0400;
    else if (!componentB) angle=0;
    else if (componentA==componentB) angle=0x0200;
    else
    {
        uint8_t firstSmaller=(int16_t)(componentA>>16)<(int16_t)(componentB>>16)
            || ((uint16_t)(componentA>>16)==(uint16_t)(componentB>>16) && (uint16_t)componentA<(uint16_t)componentB);
        uint32_t numerator=(firstSmaller?componentA:componentB)<<9,denominator=firstSmaller?componentB:componentA;
        int64_t signedNumerator=numerator<0x80000000?numerator:(int64_t)numerator-0x100000000LL;
        int64_t signedDenominator=denominator<0x80000000?denominator:(int64_t)denominator-0x100000000LL;
        angle=BtechLookupHeadingRatio((uint16_t)(signedNumerator/signedDenominator));
        if (firstSmaller) angle=0x0400-angle;
    }
    if (negativeA) angle=0x0800-angle;
    if (negativeB) angle=0x1000-angle;
    return (uint16_t)((uint16_t)(angle-0x0400)<<4);
}

/*0810:1A2E. Scenario -> terrain -> six part allocations -> resource banks ->
 * cameras -> formations -> headings. No substitute scenario loader. */
void BtechLoadScenarioAndWorld(uint16_t scenarioIndex)
{
    _Static_assert(sizeof(BtechScenarioSelectors)==0x10,"Native scenario selector IO");
    FILE *stream=BtechOpenStreamWithRetry("snario.dat","rb");
    uint16_t offset=scenarioIndex<<4;
    fseek(stream,offset<0x8000?offset:(long)offset-0x10000,SEEK_SET);
    BtechReadStreamElements(&scenarioSelectors,1,sizeof(scenarioSelectors),stream);BtechCloseStreamFile(stream);
    BtechLoadTerrainWorldResources();
    for (uint16_t slot=0;slot<COMBAT_MECH_SLOTS;++slot)
    {
        BtechUnitBinding *binding=&unitBindings[slot];
        if (!binding->mech) continue;
        binding->torso=BtechAllocatePooledWorldObject(UINT16_MAX,NULL);
        binding->rightLeg=BtechAllocatePooledWorldObject(UINT16_MAX,NULL);
        binding->positionedLeftLeg=BtechAllocatePooledWorldObject(UINT16_MAX,NULL);
        binding->rightArm=BtechAllocatePooledWorldObject(UINT16_MAX,NULL);
        binding->leftArm=BtechAllocatePooledWorldObject(UINT16_MAX,NULL);
        binding->head=BtechAllocatePooledWorldObject(UINT16_MAX,NULL);
    }
    resourceDiskPromptNumber=4;
    BtechLoadCombatChassisResourceBanks();BtechInitializePlayerCameras();
    stream=BtechOpenStreamWithRetry("snario.dat","rb");
    int16_t specialTerrain=-1;
    for (uint16_t side=0;side<2;++side)
    {
        if (sideObjectiveRules[0]>=5) abort();
        uint8_t terrainSlot=scenarioSelectors.formationSelectors[side][sideObjectiveRules[0]];
        if (terrainSlot>=4) abort();
        uint16_t variant=scenarioSelectors.terrainSelectors[terrainSlot];
        offset=(uint16_t)((sideObjectiveRules[side]<<5)+variant*0xA0+0x03C0);
        fseek(stream,offset<0x8000?offset:(long)offset-0x10000,SEEK_SET);
        uint8_t formationBytes[0x20];
        for (uint16_t unit=0;unit<4;++unit)
        {
            const BtechFormationPosition *position=&unitFormationPositions[side*4+unit];
            for (uint16_t byte=0;byte<4;++byte)
            {formationBytes[unit*8+byte]=(uint8_t)(position->positionA>>(byte*8));formationBytes[unit*8+4+byte]=(uint8_t)(position->positionB>>(byte*8));}
        }
        BtechReadStreamElements(formationBytes,1,sizeof(formationBytes),stream);
        for (uint16_t unit=0;unit<4;++unit)
        {
            BtechFormationPosition *position=&unitFormationPositions[side*4+unit];
            position->positionA=(uint32_t)MwReadNativeWord(formationBytes,unit*8)|((uint32_t)MwReadNativeWord(formationBytes,unit*8+2)<<16);
            position->positionB=(uint32_t)MwReadNativeWord(formationBytes,unit*8+4)|((uint32_t)MwReadNativeWord(formationBytes,unit*8+6)<<16);
        }
        if (sideObjectiveRules[side]==OBJECTIVE_DEFEND_SPECIAL_TARGET) specialTerrain=(int16_t)variant;
    }
    if (specialTerrain>=0)
    {
        offset=(uint16_t)((specialTerrain<<3)+0x0DC0);
        fseek(stream,offset<0x8000?offset:(long)offset-0x10000,SEEK_SET);
        uint8_t positionBytes[8];
        if (BtechReadStreamElements(positionBytes,1,8,stream)!=8) abort(); /* Unknown initial native local residue. */
        BtechPosition3D position={
            (uint32_t)MwReadNativeWord(positionBytes,0)|((uint32_t)MwReadNativeWord(positionBytes,2)<<16),
            (uint32_t)MwReadNativeWord(positionBytes,4)|((uint32_t)MwReadNativeWord(positionBytes,6)<<16),SPECIAL_TARGET_ALTITUDE};
        specialTargetDamageRemaining=SPECIAL_TARGET_INITIAL_DAMAGE;
        specialTargetObject=BtechAllocatePooledWorldObject(otherShapeResourceBase+SPECIAL_TARGET_SHAPE_DELTA,&position);
        specialTargetObject->object.prefix.renderFlags|=WORLD_OBJECT_SPECIAL_TARGET;
        objectiveVisibilityObject=specialTargetObject;
    }
    BtechCloseStreamFile(stream);
    for (uint16_t slot=0;slot<COMBAT_MECH_SLOTS;++slot)
        if (unitBindings[slot].mech)
        {
            BtechWorldObjectPrefix *object=&unitBindings[slot].positionedLeftLeg->object.prefix;
            object->position.positionA=unitFormationPositions[slot].positionA;
            object->position.positionB=unitFormationPositions[slot].positionB;
            object->position.positionC=COMBAT_INITIAL_ALTITUDE;
        }
    /* Face the opposing deployment area. These are the positions just copied
     * into each lance leader's leg object; no enemy object is needed here. */
    const BtechFormationPosition *friendly=&unitFormationPositions[0];
    const BtechFormationPosition *opposing=&unitFormationPositions[COMBAT_LANCE_SLOTS];
    uint16_t heading=BtechCalculateVectorHeading(opposing->positionA-friendly->positionA,
        opposing->positionB-friendly->positionB);
    for (uint16_t slot=0;slot<COMBAT_MECH_SLOTS;++slot)
    {
        if (!unitBindings[slot].mech) continue;
        uint16_t side=slot<COMBAT_LANCE_SLOTS?0:1;
        BtechWorldObjectPrefix *object=&unitBindings[slot].positionedLeftLeg->object.prefix;
        switch (sideObjectiveRules[side])
        {
        case OBJECTIVE_ELIMINATE_OPPOSITION:case OBJECTIVE_OPPOSE_WITHDRAWAL:case OBJECTIVE_DEFEND_SPECIAL_TARGET:
            object->orientationC=heading+(side?0x7FF8:0);break;
        case OBJECTIVE_WITHDRAW:;
            uint8_t mask=scenarioSelectors.sideBoundaryMasks[side];
            if (mask&1) object->orientationC=0;
            else if (mask&2) object->orientationC=0x7FF8;
            else if (mask&4) object->orientationC=0x3FFC;
            else if (mask&8) object->orientationC=0xC004;
            break;
        case OBJECTIVE_SPECIAL_TARGET:;
            if (!specialTargetObject || !unitBindings[side*4].positionedLeftLeg) abort();
            const BtechPosition3D *target=&specialTargetObject->object.prefix.position;
            const BtechPosition3D *reference=&unitBindings[side*4].positionedLeftLeg->object.prefix.position;
            object->orientationC=BtechCalculateVectorHeading(target->positionA-reference->positionA,target->positionB-reference->positionB);
            break;
        }
    }
}
