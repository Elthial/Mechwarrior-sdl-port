#ifndef MECHWARRIOR_COMBAT_WORLD_H
#define MECHWARRIOR_COMBAT_WORLD_H
#include "mechwarrior/combat_mechs.h"

enum { COMBAT_WORLD_OBJECTS=0x80,COMBAT_POOLED_OBJECTS=0x50,COMBAT_CAMERA_CONTEXTS=5,
    COMBAT_SELECTED_WORLD_OBJECTS=0x50,
    TERRAIN_TILE_COUNT=4,TERRAIN_TILE_ROWS=0x57,TERRAIN_TILE_COLUMNS=0x2F,
    TERRAIN_GRID_COLUMNS=0x5E,TERRAIN_GRID_ROWS=0xAE,
    TERRAIN_DETAIL_LOW=0,TERRAIN_DETAIL_MEDIUM=1,
    OBJECTIVE_ELIMINATE_OPPOSITION=0,OBJECTIVE_WITHDRAW=1,OBJECTIVE_OPPOSE_WITHDRAWAL=2,
    OBJECTIVE_SPECIAL_TARGET=3,OBJECTIVE_DEFEND_SPECIAL_TARGET=4,
    WORLD_OBJECT_POOLED=0x40,WORLD_OBJECT_SURFACE_QUERY_CANDIDATE=0x10,
    WORLD_OBJECT_SPECIAL_TARGET=8,CAMERA_OCCUPIED=0x80,CAMERA_PROJECTION_SHIFT=7,
    WORLD_OBJECT_SELECTION_PIXEL_CALLBACK=0xC058,
    WORLD_LIST_BEFORE_CALLBACK=0xC1F6,WORLD_LIST_AFTER_CALLBACK=0xC260,
    COMBAT_INITIAL_ALTITUDE=0x012C,SPECIAL_TARGET_ALTITUDE=0x044C,
    SPECIAL_TARGET_INITIAL_DAMAGE=0x28,SPECIAL_TARGET_SHAPE_DELTA=0x0E,
    TERRAIN_WORLD_CELL_SIZE=0x0200,TERRAIN_WORLD_CELL_SHIFT=9,TERRAIN_HEIGHT_SHIFT=4,
    WORLD_OBJECT_PROJECTED=4,WORLD_OBJECT_TARGET_PROBE_VISIBLE=2,COMBAT_VISIBLE_PLANAR_LIMIT=0x88B8,
    WORLD_OBJECT_MOTION_FROZEN=0x80,ORIENTATION_FRACTION_BITS=14 };
typedef struct BtechPosition3D {uint32_t positionA,positionB,positionC;} BtechPosition3D;
typedef struct BtechOrientationBasis {
    uint16_t coefficients[9];
    uint8_t mode,reserved;
} BtechOrientationBasis;
extern uint16_t orientationCosines[0x801]; /* Original CS22B6 Q14 table. */
extern uint16_t coefficientAngles[0x402];
extern BtechOrientationBasis cameraOrientationBasis;
extern uint16_t cameraReferenceVector[3];
/* CX transport is observed by native0DB1's planar-product quirk. It is not
 * another source stack argument or AX return in Reko metadata. */
uint16_t BtechBuildOrientationMatrix(const uint16_t orientation[3],BtechOrientationBasis *basis,uint16_t incomingCx);
void BtechTransformWordVectorFar(const uint16_t source[3],const BtechOrientationBasis *basis,uint16_t destination[3]);
void BtechBuildCameraReferenceVector(uint16_t orientation0,uint16_t orientation2);
void BtechResetCameraContextsAndReferenceVector(void);
typedef struct BtechWorldObjectPrefix {
    uint8_t renderFlags,flags;
    uint16_t resourceId;
    uint32_t resourceScaledValue;
    BtechPosition3D position;
    uint16_t orientationA,orientationB,orientationC,updateCallbackOffset;
} BtechWorldObjectPrefix;
typedef struct BtechHeapWorldObject {
    BtechWorldObjectPrefix prefix;
    const uint8_t *entryVariants;
    int16_t projectedScreenA,projectedScreenB;
    uint16_t projectionValue;
    uint32_t transformedPosition[3];
} BtechHeapWorldObject;
typedef struct BtechSurfaceDefinition BtechSurfaceDefinition;
typedef struct BtechSurfaceQueryResult {
    const BtechSurfaceDefinition *surface;
    BtechWorldObjectPrefix *worldObject;
    uint8_t patchIndex;
    uint32_t surfacePositionC;
} BtechSurfaceQueryResult;
struct BtechPooledWorldObject {
    BtechHeapWorldObject object;
    int16_t orientationSteps[3],positionSteps[3],forwardStepScale;
    BtechSurfaceQueryResult surfaceQuery;
    uint8_t occupied;
};
typedef struct BtechCameraContext {
    uint8_t flags,projectionShift;
    BtechWorldObjectPrefix *viewObject;
    uint16_t relativeOrientation[3];
    uint8_t fillPaletteA,fillPaletteB;
    int16_t clipMinimumA,clipMaximumA,clipMinimumB,clipMaximumB;
    const uint8_t *colourShadeClasses,*paletteRemap;
} BtechCameraContext;
typedef struct BtechScenarioSelectors {
    uint8_t terrainSelectors[4],formationSelectors[2][5],sideBoundaryMasks[2];
} BtechScenarioSelectors;
typedef struct BtechFormationPosition {uint32_t positionA,positionB;} BtechFormationPosition;
typedef union BtechChassisResourceBases {
    struct {uint16_t chassis[8],firstMech,otherShapes;} fields;
    uint16_t words[10];
} BtechChassisResourceBases;
extern BtechChassisResourceBases chassisResourceBases;
#define mechResourceBaseIds chassisResourceBases.words
#define firstMechShapeResourceId chassisResourceBases.fields.firstMech
#define otherShapeResourceBase chassisResourceBases.fields.otherShapes
extern BtechScenarioSelectors scenarioSelectors;
extern BtechFormationPosition unitFormationPositions[8];
extern uint8_t terrainHeightGrid[TERRAIN_GRID_ROWS][TERRAIN_GRID_COLUMNS];
extern uint16_t terrainTileGridRowBases[4],terrainTileGridColumnBases[4];
extern char terrainVariantFilenameSuffixes[16][8],sceneChassisBankNames[8][16];
extern uint8_t sceneChassisResourcePresent[10]; /* Last two are authored suffix bytes, not cleared. */
extern char terrainWorldFilename[12],terrainGridFilename[12];
extern BtechPooledWorldObject pooledWorldObjects[COMBAT_POOLED_OBJECTS];
extern BtechWorldObjectPrefix *worldObjects[COMBAT_WORLD_OBJECTS];
extern BtechWorldObjectPrefix *selectedWorldObjects[COMBAT_SELECTED_WORLD_OBJECTS];
extern uint16_t selectedObjectCount;
extern uint8_t heapWorldObjectOwned[COMBAT_WORLD_OBJECTS];
extern BtechWorldObjectPrefix *cockpitWorldObjects[COMBAT_WORLD_OBJECTS];
extern uint16_t worldObjectCount,cockpitWorldObjectCount,distinctSceneMechChassisCount;
extern uint16_t beforeObjectListCallbackOffset,afterObjectListCallbackOffset;
extern BtechCameraContext cameraContexts[COMBAT_CAMERA_CONTEXTS];
void BtechReleaseCameraContextFar(BtechCameraContext *camera);
void BtechReleaseCombatChassisResourceBanks(void);
void BtechReleaseHeapWorldObjectList(void);
void BtechResetWorldObjectPool(void);
extern BtechUnitBinding *playerUnitBinding;
extern BtechMechState *playerMechState;
extern BtechMechTemplate *playerMechTemplate;
extern BtechPooledWorldObject *playerPoseObject,*orderMapCameraObject,*specialTargetObject;
/* Separate render reference used for the objective's visibility and target box. */
extern BtechPooledWorldObject *objectiveVisibilityObject;
extern BtechCameraContext *orderMapCameraContext,*playerPoseCameraContext;
extern uint16_t specialTargetDamageRemaining;
extern uint16_t headingRatioAngles[0x200]; /* Authored CS22B6, not304E. */
extern uint8_t vertexNearPlaneState[0xFF];
bool BtechImportWorldInitialData(const uint8_t *data,size_t bytes,const uint8_t *segment22B6,size_t segmentBytes);
void BtechLoadScenarioAndWorld(uint16_t scenarioIndex);
void BtechLoadTerrainWorldResources(void);
void BtechLoadCombatChassisResourceBanks(void);
void BtechInitializePlayerCameras(void);
BtechCameraContext *BtechCreateHiddenCameraContext(BtechPooledWorldObject *object);
BtechCameraContext *BtechAllocateCameraContext(BtechWorldObjectPrefix *object);
BtechPooledWorldObject *BtechAllocatePooledWorldObject(uint16_t resourceId,const BtechPosition3D *placement);
void BtechUnregisterWorldObject(BtechWorldObjectPrefix *object);
void BtechReleasePooledWorldObject(BtechPooledWorldObject *object);
void BtechAdvancePooledObjectMotion(BtechPooledWorldObject *object);
BtechWorldObjectPrefix *BtechAllocateHeapWorldObject(uint16_t resourceId,const BtechPosition3D *placement);
void BtechRefreshWorldObjectResourceScales(void);
uint16_t BtechSampleTerrainOrSurfaceHeight(const BtechPosition3D *position);
uint8_t BtechTestTerrainLineOfSight(const BtechPosition3D *start,const BtechPosition3D *end);
uint8_t BtechTestPlayerTargetRenderVisibility(int16_t targetSlot);
uint16_t BtechCyclePlayerVisibleTarget(uint16_t currentTargetSlot);
void BtechRefreshOpposingUnitVisibility(void);
void BtechInitializeUnitControlRecords(void);
void BtechSynchronizeMechRenderParts(void);
uint16_t BtechCalculateVectorHeading(uint32_t componentA,uint32_t componentB);
uint16_t BtechLookupHeadingRatio(uint16_t ratioIndex);
#endif
