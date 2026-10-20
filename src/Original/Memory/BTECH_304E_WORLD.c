#include "mechwarrior/combat_world.h"
#include "mechwarrior/combat_resources.h"
#include "mechwarrior/combat_render.h"
#include "mechwarrior/combat_import.h"
#include "mechwarrior/combat_effects.h"
#include "mechwarrior/combat_damage.h"
#include "mechwarrior/combat_ballistics.h"
#include "mechwarrior/combat_projectiles.h"
#include "mechwarrior/combat_movement.h"
#include "mechwarrior/combat_navigation.h"
#include "mechwarrior/combat_tactics.h"
#include "mechwarrior/combat_installation.h"
#include "mechwarrior/native_address.h"
#include <string.h>

BtechChassisResourceBases chassisResourceBases;
uint8_t boundaryMaskByCode[TACTICAL_BOUNDARY_CODES];
const uint16_t *activeSideRule;
uint16_t tacticalThreatUnitSlots[COMBAT_MECH_SLOTS],tacticalThreatHeadings[COMBAT_MECH_SLOTS];
int32_t unitDistancesToSpecialTarget[COMBAT_MECH_SLOTS];
uint16_t opposingSideFirstUnitSlots[2];
BtechScenarioSelectors scenarioSelectors;
BtechFormationPosition unitFormationPositions[8];
uint8_t terrainHeightGrid[TERRAIN_GRID_ROWS][TERRAIN_GRID_COLUMNS];
uint16_t terrainTileGridRowBases[4],terrainTileGridColumnBases[4];
char terrainVariantFilenameSuffixes[16][8],sceneChassisBankNames[8][16];
uint8_t sceneChassisResourcePresent[10];
char terrainWorldFilename[12],terrainGridFilename[12];
BtechPooledWorldObject pooledWorldObjects[COMBAT_POOLED_OBJECTS];
BtechWorldObjectPrefix *worldObjects[COMBAT_WORLD_OBJECTS];
BtechWorldObjectPrefix *selectedWorldObjects[COMBAT_SELECTED_WORLD_OBJECTS];
uint16_t selectedObjectCount;
uint8_t heapWorldObjectOwned[COMBAT_WORLD_OBJECTS];
BtechWorldObjectPrefix *cockpitWorldObjects[COMBAT_WORLD_OBJECTS];
uint16_t worldObjectCount,cockpitWorldObjectCount,distinctSceneMechChassisCount;
uint16_t beforeObjectListCallbackOffset,afterObjectListCallbackOffset;
BtechCameraContext cameraContexts[COMBAT_CAMERA_CONTEXTS];
BtechOrientationBasis cameraOrientationBasis;
uint16_t cameraReferenceVector[3];
BtechUnitBinding *playerUnitBinding;
BtechMechState *playerMechState;
BtechMechTemplate *playerMechTemplate;
BtechPooledWorldObject *playerPoseObject,*orderMapCameraObject,*specialTargetObject;
BtechPooledWorldObject *objectiveVisibilityObject;
BtechCameraContext *orderMapCameraContext,*playerPoseCameraContext;
uint16_t specialTargetDamageRemaining;
uint8_t runtimeDelayActive,joystickPairInstalled[2],joystickAxisCentres[4];
uint16_t runtimeDelayRemaining,joystickTimingLoopCount,joystickAxisReciprocals[4];
const char *combatInputReplayFilename;
uint16_t combatInputReplayMode;
BtechCharacterClassStorage characterClassStorage;
BtechResourceBank *shapeResourceBanks,initialShapeResourceBank;
BtechSurfaceBank surfaceResourceBank;
BtechEffectSequenceState effectSequences[COMBAT_EFFECT_SLOTS];
BtechBodyDestructionEmitter bodyDestructionEmitters[COMBAT_DESTRUCTION_EMITTER_SLOTS];
BtechBodyDamageEffect bodyDamageEffects[COMBAT_BODY_EFFECT_SLOTS];
uint16_t bodyDamageEffectResourceDeltas[COMBAT_BODY_EFFECT_FRAMES];
BtechBodyReferencePoint fallenBodyReferenceBanks[COMBAT_MECH_TEMPLATES][COMBAT_ARMOR_LOCATIONS];
BtechPosition3D bodyReferenceWorldPosition;
uint32_t jumpGroundHeight;
uint16_t unitMovementBoundaryHistory[COMBAT_MECH_SLOTS];
uint16_t aiCandidateTargetSlot,aiCandidateDistance;
uint16_t hitLocationRows[ATTACK_ARCS][HIT_LOCATION_ROLLS],rangeHitModifiers[WEAPON_RANGE_BANDS];
int16_t avoidanceStepX,avoidanceStepY;
const BtechPosition3D *avoidancePosition;
const BtechWorldObjectPrefix *avoidanceOrientation;
uint16_t avoidanceHeadingOffsets[AVOIDANCE_TURNS];
int16_t avoidanceProbeSignsX[AVOIDANCE_PROBES],avoidanceProbeSignsY[AVOIDANCE_PROBES];
const int16_t *avoidanceProbeComponentsX[AVOIDANCE_PROBES],*avoidanceProbeComponentsY[AVOIDANCE_PROBES];
uint32_t unitDistances[COMBAT_MECH_SLOTS][COMBAT_MECH_SLOTS];
uint16_t beamHalfWidths[COMBAT_WEAPON_KINDS];
int16_t muzzleLateralOffsetsByBodyLocation[COMBAT_BODY_LOCATIONS];
BtechProjectileState projectiles[COMBAT_PROJECTILE_SLOTS];
BtechProjectileProfile projectileProfiles[COMBAT_PROJECTILE_PROFILES];
uint16_t projectileProfileIndicesByWeapon[COMBAT_WEAPON_KINDS],bodyLocationReferenceIndices[COMBAT_BODY_LOCATIONS];
int16_t missileClusterRowByWeapon[COMBAT_WEAPON_KINDS];
uint16_t missileClusterRows[COMBAT_MISSILE_CLUSTER_ROWS][COMBAT_MISSILE_CLUSTER_ROLLS];
uint8_t beamSlotOccupied[COMBAT_BEAM_SLOTS];
BtechShapeResource beamResources[COMBAT_BEAM_SLOTS];
BtechRenderEntry beamRenderEntries[COMBAT_BEAM_SLOTS];
uint16_t beamGeometry[COMBAT_BEAM_SLOTS][COMBAT_BEAM_VECTORS][3];
BtechHeapWorldObject beamWorldObjects[COMBAT_BEAM_SLOTS];
BtechRenderVariant ppcBeamVariant,laserBeamVariant;
static BtechRenderPrimitive ppcBeamPrimitive,laserBeamPrimitive;
static BtechRenderPolygon ppcBeamPolygons[2],laserBeamPolygons[2];
static uint8_t beamVerticalTriangleIndices[4],beamHorizontalTriangleIndices[4];
uint16_t defaultEffectResourceDeltas[7],machineGunEffectResourceDeltas[3];
uint16_t *const effectResourceSequences[2]={defaultEffectResourceDeltas,machineGunEffectResourceDeltas};
uint16_t shapeResourceBankCount;
char resourceFilename[0x50];
BtechArchiveDescriptor *archiveDescriptors[RESOURCE_ARCHIVE_SLOTS],*selectedArchive;
uint8_t *archiveDestination;
uint16_t archiveRemainingBytes;
uint8_t archiveIoFlags,lzwInitialLiteralPending,lzwPhraseResumePending;
uint16_t lzwCodeWidth,lzwCodeWidthLimit,lzwNextFreeCode,lzwPacketBitOffset,lzwPacketBitLimit;
uint16_t lzwClearPending,lzwReverseReadOffset,lzwPreviousCode,lzwIncomingCode,lzwPhraseFirstByte;
uint8_t lzwPacketBytes[12];
BtechLzwReaderWorkspace *borrowedArchiveWorkspace;
uint8_t archiveStreamScratch[0x32];

/* Installation IO, not a runtime memory mirror. Strings are resolved exactly
 * once from authored near directories. Runtime consumers use real C strings. */
bool BtechImportWorldInitialData(const uint8_t *data,size_t bytes,const uint8_t *segment22B6,size_t segmentBytes)
{
    if (!data || bytes<0x10000 || !segment22B6 || segmentBytes<0x6473) return false;
    if (MwReadNativeWord(data,0x4A6C) || MwReadNativeWord(data,0x4A6E)) return false;
    borrowedArchiveWorkspace=NULL;
    memcpy(boundaryMaskByCode,data+0x0EB6,sizeof(boundaryMaskByCode));
    for (uint16_t side=0;side<2;++side) opposingSideFirstUnitSlots[side]=MwReadNativeWord(data,0x2434+side*2);
    for (uint16_t turn=0;turn<AVOIDANCE_TURNS;++turn) avoidanceHeadingOffsets[turn]=MwReadNativeWord(data,0x2438+turn*2);
    for (uint16_t probe=0;probe<AVOIDANCE_PROBES;++probe)
    {
        avoidanceProbeSignsX[probe]=(int16_t)MwReadNativeWord(data,0x243E+probe*2);
        avoidanceProbeSignsY[probe]=(int16_t)MwReadNativeWord(data,0x2446+probe*2);
        uint16_t x=MwReadNativeWord(data,0x244E+probe*2),y=MwReadNativeWord(data,0x2456+probe*2);
        if ((x!=0x542A && x!=0x542C) || (y!=0x542A && y!=0x542C)) return false;
        avoidanceProbeComponentsX[probe]=x==0x542A?&avoidanceStepX:&avoidanceStepY;
        avoidanceProbeComponentsY[probe]=y==0x542A?&avoidanceStepX:&avoidanceStepY;
    }
    for (uint16_t kind=0;kind<COMBAT_WEAPON_KINDS;++kind) missileClusterRowByWeapon[kind]=(int16_t)MwReadNativeWord(data,0x234A+kind*2);
    for (uint16_t row=0;row<COMBAT_MISSILE_CLUSTER_ROWS;++row)
        for (uint16_t roll=0;roll<COMBAT_MISSILE_CLUSTER_ROLLS;++roll) missileClusterRows[row][roll]=MwReadNativeWord(data,0x235E+row*0x18+roll*2);
    for (uint16_t arc=0;arc<ATTACK_ARCS;++arc)
        for (uint16_t roll=0;roll<HIT_LOCATION_ROLLS;++roll) hitLocationRows[arc][roll]=MwReadNativeWord(data,0x26BC+arc*0x16+roll*2);
    for (uint16_t band=0;band<WEAPON_RANGE_BANDS;++band) rangeHitModifiers[band]=MwReadNativeWord(data,0x2714+band*2);
    for (uint16_t profile=0;profile<COMBAT_PROJECTILE_PROFILES;++profile)
    {
        uint16_t at=0x0E6E+profile*8;
        projectileProfiles[profile]=(BtechProjectileProfile){MwReadNativeWord(data,at),MwReadNativeWord(data,at+2),MwReadNativeWord(data,at+4),MwReadNativeWord(data,at+6)};
    }
    for (uint16_t kind=0;kind<COMBAT_WEAPON_KINDS;++kind) projectileProfileIndicesByWeapon[kind]=MwReadNativeWord(data,0x232E+kind*2);
    for (uint16_t location=0;location<COMBAT_BODY_LOCATIONS;++location) bodyLocationReferenceIndices[location]=MwReadNativeWord(data,0x0EBC+location*2);
    for (uint16_t location=0;location<COMBAT_BODY_LOCATIONS;++location) muzzleLateralOffsetsByBodyLocation[location]=(int16_t)MwReadNativeWord(data,0x2302+location*2);
    for (uint16_t kind=0;kind<COMBAT_WEAPON_KINDS;++kind) beamHalfWidths[kind]=MwReadNativeWord(data,0x2802+kind*2);
    memcpy(beamVerticalTriangleIndices,data+0x27C2,4);memcpy(beamHorizontalTriangleIndices,data+0x27C6,4);
    if (beamVerticalTriangleIndices[3]!=POLYGON_VERTEX_LIST_END || beamHorizontalTriangleIndices[3]!=POLYGON_VERTEX_LIST_END) return false;
    for (uint16_t style=0;style<2;++style)
    {
        uint16_t at=style?0x27FE:0x27E2,primitiveAt=MwReadNativeWord(data,at+2);
        BtechRenderVariant *variant=style?&laserBeamVariant:&ppcBeamVariant;
        BtechRenderPrimitive *primitive=style?&laserBeamPrimitive:&ppcBeamPrimitive;
        BtechRenderPolygon *polygons=style?laserBeamPolygons:ppcBeamPolygons;
        if (primitiveAt>0xFFF8 || MwReadNativeWord(data,at)!=1 || data[primitiveAt+1]!=RENDER_PRIMITIVE_POLYGON || MwReadNativeWord(data,primitiveAt+2)!=2) return false;
        *variant=(BtechRenderVariant){1,primitive};
        primitive->scaleLimit=data[primitiveAt];primitive->kind=data[primitiveAt+1];
        primitive->polygonPayload=(BtechRenderPolygonPayload){2,polygons};
        uint16_t polygonAt=MwReadNativeWord(data,primitiveAt+4);
        for (uint16_t polygon=0;polygon<2;++polygon,polygonAt+=8)
        {
            if (polygonAt>0xFFF8 || MwReadNativeWord(data,polygonAt+6)!=(polygon?0x27C6:0x27C2)) return false;
            polygons[polygon]=(BtechRenderPolygon){data[polygonAt],data[polygonAt+1],data[polygonAt+2],data[polygonAt+3],data[polygonAt+4],data[polygonAt+5],polygon?beamHorizontalTriangleIndices:beamVerticalTriangleIndices};
        }
    }
    for (uint16_t chassis=0;chassis<COMBAT_MECH_TEMPLATES;++chassis)
        for (uint16_t location=0;location<COMBAT_ARMOR_LOCATIONS;++location)
        {
            uint16_t at=0x0FC8+chassis*0x36+location*6;
            fallenBodyReferenceBanks[chassis][location]=(BtechBodyReferencePoint){(int16_t)MwReadNativeWord(data,at),(int16_t)MwReadNativeWord(data,at+2),(int16_t)MwReadNativeWord(data,at+4)};
        }
    for (uint16_t frame=0;frame<COMBAT_BODY_EFFECT_FRAMES;++frame) bodyDamageEffectResourceDeltas[frame]=MwReadNativeWord(data,0x2342+frame*2);
    /* Authored near directory is resolved at installation IO, not every frame. */
    if (MwReadNativeWord(data,0x0EAA)!=0x0E96 || MwReadNativeWord(data,0x0EAC)!=0x0EA4) return false;
    for (uint16_t frame=0;frame<7;++frame) defaultEffectResourceDeltas[frame]=MwReadNativeWord(data,0x0E96+frame*2);
    for (uint16_t frame=0;frame<3;++frame) machineGunEffectResourceDeltas[frame]=MwReadNativeWord(data,0x0EA4+frame*2);
    for (uint16_t tile=0;tile<TERRAIN_TILE_COUNT;++tile)
    {
        terrainTileGridRowBases[tile]=MwReadNativeWord(data,0x07E4+tile*2);
        terrainTileGridColumnBases[tile]=MwReadNativeWord(data,0x07EC+tile*2);
    }
    for (uint16_t variant=0;variant<16;++variant)
    {
        uint16_t offset=MwReadNativeWord(data,0x07C4+variant*2);
        const uint8_t *end=memchr(data+offset,0,0x10000-offset);
        if (!end || (size_t)(end-data-offset)>=sizeof(terrainVariantFilenameSuffixes[variant])) return false;
        memcpy(terrainVariantFilenameSuffixes[variant],data+offset,(size_t)(end-data-offset)+1);
    }
    for (uint16_t chassis=0;chassis<8;++chassis)
    {
        uint16_t offset=MwReadNativeWord(data,0x06DC+chassis*2);
        const uint8_t *end=memchr(data+offset,0,0x10000-offset);
        if (!end || (size_t)(end-data-offset)>=sizeof(sceneChassisBankNames[chassis])) return false;
        memcpy(sceneChassisBankNames[chassis],data+offset,(size_t)(end-data-offset)+1);
    }
    memcpy(sceneChassisResourcePresent,data+0x06EC,sizeof(sceneChassisResourcePresent));
    memcpy(characterClassStorage.bytes,data+0x4C69,sizeof(characterClassStorage.bytes));
    for (uint16_t index=0;index<0x200;++index) headingRatioAngles[index]=MwReadNativeWord(segment22B6,0x6073+index*2);
    for (uint16_t index=0;index<0x801;++index) orientationCosines[index]=MwReadNativeWord(segment22B6,0x486D+index*2);
    for (uint16_t index=0;index<0x402;++index) coefficientAngles[index]=MwReadNativeWord(segment22B6,0x586F+index*2);
    memcpy(vertexNearPlaneState,segment22B6+0x312C,sizeof(vertexNearPlaneState));
    memcpy(polygonShadeDeltas,segment22B6+0x476D,sizeof(polygonShadeDeltas));
    memcpy(polygonVertexIndices,segment22B6+0x4617,sizeof(polygonVertexIndices));
    uint32_t *wideCoordinates[]={wideScreenX,wideScreenY,widePolygonWorkX,widePolygonWorkY};
    for (uint16_t axis=0;axis<4;++axis)
        for (uint16_t vertex=0;vertex<WIDE_POLYGON_VERTICES;++vertex)
            wideCoordinates[axis][vertex]=MwReadNativeWord(segment22B6,0x462D+axis*0x50+vertex*4)
                |((uint32_t)MwReadNativeWord(segment22B6,0x462F+axis*0x50+vertex*4)<<16);
    BtechGeometryCoordinateCache *coordinateCaches[]={&geometryViewX,&geometryViewDepth,&geometryViewZ,&geometryProjectedX,&geometryProjectedY};
    for (uint16_t axis=0;axis<5;++axis)
        for (uint16_t word=0;word<GEOMETRY_CACHE_VERTICES*2;++word)
            coordinateCaches[axis]->words[word]=MwReadNativeWord(segment22B6,0x322B+axis*0x3FC+word*2);
    memcpy(&scenarioSelectors,data+0x7D6A,sizeof(scenarioSelectors));
    return true;
}

/* EXEC/DFCE data projection only, not206D or a new game procedure. Clear
 * converted owners in the native4EDC..D5DF BSS interval; reload authored
 * scalars separately. Host bindings and campaign owners are untouched. */
void BtechInitializeFreshWorldOwners(const uint8_t *data)
{
    bodyReferenceWorldPosition=(BtechPosition3D){0};
    jumpGroundHeight=0;
    aiCandidateTargetSlot=aiCandidateDistance=0;
    avoidanceStepX=avoidanceStepY=0;avoidancePosition=NULL;avoidanceOrientation=NULL;
    activeSideRule=NULL;
    memset(unitDistancesToSpecialTarget,0,sizeof(unitDistancesToSpecialTarget));
    memset(tacticalThreatUnitSlots,0,sizeof(tacticalThreatUnitSlots));
    memset(tacticalThreatHeadings,0,sizeof(tacticalThreatHeadings));
    for (uint16_t slot=0;slot<COMBAT_MECH_SLOTS;++slot) unitMovementBoundaryHistory[slot]=0;
    for (uint16_t slot=0;slot<COMBAT_PROJECTILE_SLOTS;++slot) projectiles[slot]=(BtechProjectileState){0};
    for (uint16_t row=0;row<COMBAT_MECH_SLOTS;++row)
        for (uint16_t column=0;column<COMBAT_MECH_SLOTS;++column) unitDistances[row][column]=0;
    for (uint16_t slot=0;slot<COMBAT_BEAM_SLOTS;++slot)
    {
        beamSlotOccupied[slot]=0;beamResources[slot]=(BtechShapeResource){0};
        beamRenderEntries[slot]=(BtechRenderEntry){0};beamWorldObjects[slot]=(BtechHeapWorldObject){0};
        for (uint16_t vector=0;vector<COMBAT_BEAM_VECTORS;++vector)
            for (uint16_t axis=0;axis<3;++axis) beamGeometry[slot][vector][axis]=0;
    }
    for (uint16_t slot=0;slot<COMBAT_BODY_EFFECT_SLOTS;++slot) bodyDamageEffects[slot]=(BtechBodyDamageEffect){0};
    for (uint16_t slot=0;slot<COMBAT_DESTRUCTION_EMITTER_SLOTS;++slot) bodyDestructionEmitters[slot]=(BtechBodyDestructionEmitter){0};
    for (uint16_t slot=0;slot<COMBAT_EFFECT_SLOTS;++slot) effectSequences[slot]=(BtechEffectSequenceState){0};
    for (uint16_t byte=0;byte<sizeof(archiveStreamScratch);++byte) archiveStreamScratch[byte]=0;
    scenarioSelectors=(BtechScenarioSelectors){0};
    for (uint16_t slot=0;slot<8;++slot)
        unitFormationPositions[slot]=(BtechFormationPosition){0};
    for (uint16_t slot=0;slot<TERRAIN_GRID_ROWS;++slot)
        for (uint16_t column=0;column<TERRAIN_GRID_COLUMNS;++column)
            terrainHeightGrid[slot][column]=0;
    for (uint16_t slot=0;slot<12;++slot)
        terrainWorldFilename[slot]=0;
    for (uint16_t slot=0;slot<12;++slot)
        terrainGridFilename[slot]=0;
    for (uint16_t slot=0;slot<COMBAT_POOLED_OBJECTS;++slot)
        pooledWorldObjects[slot]=(BtechPooledWorldObject){0};
    for (uint16_t slot=0;slot<COMBAT_WORLD_OBJECTS;++slot)
        worldObjects[slot]=NULL;
    for (uint16_t slot=0;slot<COMBAT_WORLD_OBJECTS;++slot)
        heapWorldObjectOwned[slot]=0;
    for (uint16_t slot=0;slot<COMBAT_WORLD_OBJECTS;++slot)
        cockpitWorldObjects[slot]=NULL;
    worldObjectCount=0;
    cockpitWorldObjectCount=0;
    distinctSceneMechChassisCount=0;
    for (uint16_t slot=0;slot<COMBAT_CAMERA_CONTEXTS;++slot)
        cameraContexts[slot]=(BtechCameraContext){0};
    cameraOrientationBasis=(BtechOrientationBasis){0};
    for (uint16_t slot=0;slot<3;++slot)
        cameraReferenceVector[slot]=0;
    playerUnitBinding=NULL;
    playerMechState=NULL;
    playerMechTemplate=NULL;
    playerPoseObject=NULL;
    orderMapCameraObject=NULL;
    specialTargetObject=NULL;
    objectiveVisibilityObject=NULL;
    orderMapCameraContext=NULL;
    playerPoseCameraContext=NULL;
    specialTargetDamageRemaining=0;
    for (uint16_t slot=0;slot<0x50;++slot)
        resourceFilename[slot]=0;
    for (uint16_t slot=0;slot<RESOURCE_ARCHIVE_SLOTS;++slot)
        archiveDescriptors[slot]=NULL;
    selectedArchive=NULL;
    archiveDestination=NULL;
    archiveRemainingBytes=0;
    archiveIoFlags=0;
    lzwInitialLiteralPending=0;
    lzwPhraseResumePending=0;
    lzwCodeWidth=0;
    lzwCodeWidthLimit=0;
    lzwNextFreeCode=0;
    lzwPacketBitOffset=0;
    lzwPacketBitLimit=0;
    lzwClearPending=0;
    lzwPreviousCode=0;
    lzwIncomingCode=0;
    lzwPhraseFirstByte=0;
    for (uint16_t slot=0;slot<sizeof(sceneChassisResourcePresent)/sizeof(sceneChassisResourcePresent[0]);++slot) sceneChassisResourcePresent[slot]=data[0x06EC+slot*1];
    beforeObjectListCallbackOffset=MwReadNativeWord(data,0x2CE8);
    afterObjectListCallbackOffset=MwReadNativeWord(data,0x2CEA);
    runtimeDelayActive=data[0x41E4];
    for (uint16_t slot=0;slot<sizeof(joystickPairInstalled)/sizeof(joystickPairInstalled[0]);++slot) joystickPairInstalled[slot]=data[0x41CA+slot*1];
    for (uint16_t slot=0;slot<sizeof(joystickAxisCentres)/sizeof(joystickAxisCentres[0]);++slot) joystickAxisCentres[slot]=data[0x41CC+slot*1];
    runtimeDelayRemaining=MwReadNativeWord(data,0x41E5);
    joystickTimingLoopCount=MwReadNativeWord(data,0x41DB);
    for (uint16_t slot=0;slot<sizeof(joystickAxisReciprocals)/sizeof(joystickAxisReciprocals[0]);++slot) joystickAxisReciprocals[slot]=MwReadNativeWord(data,0x41D0+slot*2);
    combatInputReplayMode=MwReadNativeWord(data,0x084C);
    lzwReverseReadOffset=MwReadNativeWord(data,0x4AAF);
    for (uint16_t slot=0;slot<sizeof(lzwPacketBytes)/sizeof(lzwPacketBytes[0]);++slot) lzwPacketBytes[slot]=data[0x4A9A+slot*1];
    combatInputReplayFilename=NULL;
    chassisResourceBases=(BtechChassisResourceBases){0};
    surfaceResourceBank=(BtechSurfaceBank){0};
    initialShapeResourceBank=(BtechResourceBank){0};
    shapeResourceBanks=&initialShapeResourceBank;
    shapeResourceBankCount=1;
}
