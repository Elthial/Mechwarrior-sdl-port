#ifndef MECHWARRIOR_COMBAT_RENDER_H
#define MECHWARRIOR_COMBAT_RENDER_H
#include "mechwarrior/combat_video.h"
#include "mechwarrior/combat_cockpit.h"
#include "mechwarrior/combat_resources.h"
#include "mechwarrior/combat_polygons.h"
/* CS geometry code observed as data by a wrapped surface-priority read. */

enum { CAMERA_NO_BACKGROUND=0x08,CAMERA_PROJECTION_MODE_10=0x10,CAMERA_MIRROR_HORIZONTAL=0x20,
    CAMERA_KEEP_SCREEN_CENTRE=0x40,CAMERA_GRADIENT_BACKGROUND=0x04,
    OBJECT_CAPTURE_PROJECTION=0x08,OBJECT_PROJECTED_VISIBLE=0x04,
    OBJECT_CALLBACK_BEFORE_RENDER=0x02,OBJECT_SKIP_ORIENTATION=0x20,OBJECT_HIDDEN=0x80,
    OBJECT_MAXIMUM_PROJECTION_SCALE=0x100,RENDER_ENTRY_PREFIX_CAPACITY=48,
    RESOURCE_SHARED_GEOMETRY=0x80 };
extern const BtechShapeResource *activeRenderHeader;
extern const BtechRenderEntry *activeRenderEntry,*orderedRenderEntries[RENDER_ENTRY_PREFIX_CAPACITY];
extern const BtechRenderPrimitive *activePrimitiveRecord;
extern const BtechRenderVariant *activeVariant;
extern const uint16_t (*activeEntryGeometry)[3],(*renderGeometry)[3];
extern uint32_t renderEntryOrderingKeys[RENDER_ENTRY_PREFIX_CAPACITY];
extern uint8_t renderEntryVariants[RENDER_ENTRY_PREFIX_CAPACITY];
extern uint16_t resourceReferenceVector[3],transformedEntryVector[3];
extern uint16_t renderTransformedReference[3];
void BtechDrawReferencePointEllipse(const BtechRenderEllipse *ellipse);
extern uint16_t projectionScaleMultiplier,renderGeometryVectorCount,resourceScaleShift;
extern uint8_t widePolygonCoordinates,useWordProductClipping,skipWordPolygonRenderer;
void BtechPrepareRenderResourceEntries(uint16_t resourceId,const uint8_t *entryVariants);
void BtechPrepareResourceCoordinatePaths(void);
uint32_t BtechDotProductSignedWordVectors(const uint16_t first[3],const uint16_t second[3]);
void BtechClearGeometryVectorCacheFlags(void);

/* The original cache has WORD and DWORD users of the SAME bytes. A normal
 * transformed for shading writes only the low WORD of a wide-stride slot. */
enum { GEOMETRY_CACHE_VERTICES=0xFF };
typedef union BtechGeometryCoordinateCache {
    uint16_t words[GEOMETRY_CACHE_VERTICES*2];
    uint32_t wide[GEOMETRY_CACHE_VERTICES];
} BtechGeometryCoordinateCache;
extern BtechGeometryCoordinateCache geometryViewX,geometryViewDepth,geometryViewZ;
extern BtechGeometryCoordinateCache geometryProjectedX,geometryProjectedY;
/*1F7C fractions alias the same three WORDs used by19CE/2227 as rotated values. */
#define geometryTransformFractions transformedEntryVector
void BtechTransformWordVector(const uint16_t source[3],const BtechOrientationBasis *basis,uint16_t destination[3]);
void BtechTransformWordVectorToDword(const uint16_t source[3],const BtechOrientationBasis *basis,BtechPosition3D *destination);
void BtechCacheTransformedGeometryVector(uint8_t vectorIndex);
void BtechCacheRotatedGeometryVector(uint8_t vectorIndex);
uint64_t BtechGetWidePolygonFacingDotHighWord(uint8_t normalIndex,uint8_t firstVertexIndex);
void BtechCachePolygonShadeNormal(uint8_t normalIndex);
extern uint8_t polygonShadeDeltas[POLYGON_SHADE_CLASSES][POLYGON_SHADE_STEPS],polygonShadeClass;
void BtechSelectPolygonColoursAndShade(const BtechRenderPolygon *polygon);
enum { GEOMETRY_VERTEX_UNTOUCHED=0,GEOMETRY_VERTEX_TRANSFORMED=1,GEOMETRY_VERTEX_PROJECTED=2,GEOMETRY_VERTEX_BEHIND=3,
    WIDE_POLYGON_VERTICES=20,POLYGON_VERTEX_INDEX_BYTES=22,
    POLYGON_LEFT=0x01,POLYGON_RIGHT=0x02,POLYGON_BOTTOM=0x04,POLYGON_TOP=0x08 };
extern uint8_t polygonVertexIndices[POLYGON_VERTEX_INDEX_BYTES],polygonHasNearClippedVertex;
extern uint16_t widePolygonsRemaining;
extern uint32_t wideScreenX[WIDE_POLYGON_VERTICES],wideScreenY[WIDE_POLYGON_VERTICES];
extern uint32_t widePolygonWorkX[WIDE_POLYGON_VERTICES],widePolygonWorkY[WIDE_POLYGON_VERTICES];
void BtechDrawPolygonPayload(const BtechRenderPolygonPayload *payload);
void BtechCacheTransformedPolygonVertex(uint8_t vectorIndex);
void BtechCacheTransformedPolygonNormal(uint8_t vectorIndex);
int16_t BtechGetPolygonFacingDotHighWord(uint8_t normalIndex,uint8_t firstVertexIndex);
void BtechIntersectWordEdgeWithNearPlane(uint16_t outsideVertex,uint16_t insideVertex);
void BtechClipWordPolygonNearPlane(void);
extern uint16_t wordPolygonsRemaining;
extern uint8_t wordPolygonHasNearOutsideVertex;
void BtechRenderWidePolygonPayload(const BtechRenderPolygonPayload *payload);
uint64_t BtechMultiplySignedDwordsWide(uint32_t first,uint32_t second);
void BtechOffsetWidePolygonVerticesToScreen(void);
void BtechCopyRendererWorkingCoordinateArrays(void);
void BtechIntersectWideEdgeWithNearPlane(uint16_t outsideVertex,uint16_t insideVertex);
void BtechIntersectDwordNearEdgeWithWordProducts(uint16_t outsideVertex,uint16_t insideVertex);
void BtechClipWidePolygonNearPlane(void);
void BtechClipDwordNearPolygonWithWordProducts(void);
uint16_t BtechIntersectWideScreenEdgeA(uint16_t base,uint16_t next,uint16_t destination,uint8_t flags);
uint16_t BtechIntersectWideScreenEdgeB(uint16_t base,uint16_t next,uint16_t destination,uint8_t flags);
uint16_t BtechIntersectScreenEdgeWordProductA(uint16_t base,uint16_t next,uint16_t destination,uint8_t flags);
uint16_t BtechIntersectScreenEdgeWordProductB(uint16_t base,uint16_t next,uint16_t destination,uint8_t flags);
void BtechClipWidePolygonToScreen(void);
void BtechClipPolygonWithWordProducts(void);
typedef union BtechProjectionRatio {
    struct {uint32_t numerator,denominator,third;} ratio;
    BtechPosition3D vector;
} BtechProjectionRatio;
typedef struct BtechDwordDivisionResult {uint16_t quotientLow,quotientHigh,remainderLow,remainderHigh;} BtechDwordDivisionResult;
extern BtechCameraContext *activeCameraContext;
extern BtechPosition3D cameraReferencePosition,cameraRelativePosition,transformedViewPosition,projectionVectorScratch;
extern BtechProjectionRatio projectionRatio;
extern BtechOrientationBasis viewTransformBasis;
extern BtechOrientationBasis composedTransformBasis;
extern uint16_t cameraViewOrientation[3],transformedReferenceVector[3],viewVectorCoefficients[3];
extern uint16_t combatProjectionShift,nearClipDepth,combatProjectionUnit,objectProjectionScale;
extern uint32_t resourceScaledExtent;
/*7756 is a real shared scratch DWORD: horizon writes only its LOW WORD. */
extern uint32_t cameraDepthScratch;
extern uint8_t forceObjectScreenClip,projectionMode10,mirrorProjectionA,objectNeedsScreenClip,useViewBasisDirectly;
extern int16_t screenCentreA,screenCentreB;
extern const uint8_t *activeColourShadeClasses,*activePaletteRemap;
extern BtechOrientationBasis *activeRenderBasis;
extern uint16_t dotProductScratchHigh,dotProductScratchMiddle,dotProductScratchLow;
extern BtechDwordDivisionResult signedDivisionResult;
extern uint16_t selectionCount;
enum { SELECTION_LIST_PREFIX=9,SELECTION_MARKER_COLOUR=Combat_Pal_EGA_LightMagenta };
extern uint16_t selectionPointScreenX,selectionPointScreenY;
extern uint16_t selectionObjectListIndices[SELECTION_LIST_PREFIX];
extern uint32_t selectionMarkerPixels,selectionCapturedPixels,selectionPointBackground;
void BtechBeginSelectionPixelProbe(uint16_t x,uint16_t y);
uint32_t BtechCaptureSelectionPixels(int16_t x,int16_t y);
void BtechWriteSelectionPixels(int16_t x,int16_t y,const uint32_t *pixels);
uint16_t BtechTestSelectionMarkerPixels(const uint32_t *pixels);
extern int16_t horizonPolygonX[4],horizonPolygonY[4];
extern uint8_t targetSelectionProbeActive;
bool BtechImportRenderInitialData(const uint8_t *data,size_t bytes);
void BtechRenderObjectListVeneer(BtechCameraContext *camera,BtechWorldObjectPrefix *const *objects,int16_t count);
void BtechRenderObjectList(BtechCameraContext *camera,BtechWorldObjectPrefix *const *objects,int16_t count);
void BtechPrepareCameraContext(BtechCameraContext *camera);
uint16_t BtechLookupSine(uint16_t angle);
uint16_t BtechLookupCosine(uint16_t angle);
void BtechTransposeTransformBasis(BtechOrientationBasis *basis);
void BtechComposeTransformBases(uint16_t incomingCx,const BtechOrientationBasis *left,const BtechOrientationBasis *right,BtechOrientationBasis *destination);
uint16_t BtechLookupSignedCoefficientAngle(uint16_t coefficient);
uint16_t BtechCalculateWordPairHeading(uint16_t componentA,uint16_t componentB);
void BtechExtractOrientationFromBasis(const BtechOrientationBasis *basis,uint16_t orientation[3]);
void BtechTransformViewVector(const BtechPosition3D *input,const BtechOrientationBasis *basis,BtechPosition3D *output);
uint64_t BtechDotSignedVectorByWordCoefficients(const BtechPosition3D *vector,const uint16_t coefficients[3]);
uint16_t BtechProjectViewPosition(const BtechPosition3D *input,BtechPosition3D *output);
void BtechProjectValueDepthRatio(const BtechProjectionRatio *input,uint32_t *output);
void BtechOffsetProjectedScreenPair(BtechPosition3D *screen);
void BtechCullAndRenderWorldObject(BtechHeapWorldObject *object);
void BtechPrepareSelectionPixels(BtechWorldObjectPrefix *const *objects,int16_t count);
void BtechRestoreSelectionPixels(BtechWorldObjectPrefix *const *objects);
void BtechUpdateObjectSelectionPixels(BtechHeapWorldObject *object,uint16_t ordinal);
BtechDwordDivisionResult *BtechDivideSignedDwordWithRemainder(uint32_t numerator,uint32_t denominator);
BtechDwordDivisionResult *BtechDivideSignedDwordStoreQuotientRemainder(uint16_t numeratorLow,uint16_t numeratorHigh,uint16_t denominatorLow,uint16_t denominatorHigh);
uint32_t BtechMultiplyDwordLow(uint16_t leftLow,uint16_t leftHigh,uint16_t rightLow,uint16_t rightHigh);
uint32_t BtechDivideSignedDword(uint32_t numerator,uint32_t denominator);
uint32_t BtechShiftDwordLeft(uint32_t value,uint8_t count);
uint32_t BtechShiftSignedDwordRight(uint32_t value,uint8_t count);
int32_t BtechMultiplySignedWords(int16_t left,int16_t right);
/* Optional CX output is native register residue used by0DB1, not a new
 * source argument. NULL means that the original caller discards that register. */
uint32_t BtechDivideSignedDwordNormalized(uint16_t numeratorLow,int16_t numeratorHigh,uint16_t denominatorLow,int16_t denominatorHigh,uint16_t *cxOut);
uint32_t BtechDivideSignedWideNumerator(int16_t highest,uint16_t upper,uint16_t middle,uint16_t low,uint16_t denominatorLow,int16_t denominatorHigh,uint16_t *cxOut);
uint32_t BtechDivideErrorInterrupt(uint32_t dividend);
void BtechFillTiltedCameraHorizonRegions(uint16_t fillA,uint16_t fillB,uint16_t pitch,uint16_t roll);
#endif
