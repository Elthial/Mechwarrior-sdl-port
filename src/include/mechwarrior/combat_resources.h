#ifndef MECHWARRIOR_COMBAT_RESOURCES_H
#define MECHWARRIOR_COMBAT_RESOURCES_H
#include "mechwarrior/combat_platform.h"
#include <stddef.h>
enum { RESOURCE_METHOD_RAW=0,RESOURCE_METHOD_RLE=1,RESOURCE_METHOD_LZW=2,
    RESOURCE_ARCHIVE_SLOTS=100,RESOURCE_DECODE_CHUNK=0x7D00,
    LZW_CLEAR_CODE=0x100,LZW_FIRST_CODE=0x101,LZW_DICTIONARY_LIMIT=0x1000,
    LZW_INITIAL_CODE_BITS=9,LZW_MAX_CODE_BITS=12,
    RESOURCE_READ_FLAG=0x40,RESOURCE_FILE_FLAG=0x20,
    RENDER_PRIMITIVE_POLYGON=0,RENDER_PRIMITIVE_ELLIPSE=1,
    RENDER_PRIMITIVE_ORIGIN_PICTURE=2,RENDER_PRIMITIVE_REFERENCE_PICTURE=3,
    POLYGON_FACING_MASK=0x03,POLYGON_SHADE_NORMAL=0x20,POLYGON_FILLED=0x80,
    POLYGON_VERTEX_LIST_END=0xFF,POLYGON_SHADE_CLASSES=8,POLYGON_SHADE_STEPS=32 };
typedef struct BtechResourceAllocation {uint8_t *bytes;uint32_t byteCount;} BtechResourceAllocation;
/* Decoded at bank IO. Runtime entry selection uses references, not near-offset
 * accessors. Picture payloads remain binary until their consumers are ported. */
typedef struct BtechRenderPolygon {
    uint8_t flags,fillColour,lineColour,alternateFillColour,alternateLineColour,normalVectorIndex;
    const uint8_t *vertexIndices; /* Borrowed, immutable FF-terminated BYTE list. */
} BtechRenderPolygon;
typedef struct BtechRenderPolygonPayload {
    int16_t polygonCount;
    BtechRenderPolygon *polygons;
} BtechRenderPolygonPayload;

typedef struct BtechRenderEllipse {
    int16_t radiusScale;
    uint8_t referenceVectorIndex,colour,alternateColour,unidentifiedField5;
} BtechRenderEllipse;
typedef struct BtechRenderPrimitive {
    uint8_t scaleLimit,kind;
    union {
        const uint8_t *encodedPayload;
        BtechRenderPolygonPayload polygonPayload;
        BtechRenderEllipse ellipsePayload;
    };
} BtechRenderPrimitive;
typedef struct BtechRenderVariant {
    int16_t primitiveCount;
    BtechRenderPrimitive *primitives; /* One preceding LOD record is retained. */
} BtechRenderVariant;
typedef struct BtechRenderEntry {
    uint8_t firstVectorIndex,vectorCount;
    uint16_t (*vectors)[3];
    uint16_t variantCount;
    BtechRenderVariant *variants;
} BtechRenderEntry;
/* A bank's four-byte on-disk relocation pairs become real references. Retain
 * paragraph origin: resource-internal near offsets are selector-relative. */
typedef struct BtechShapeResource {
    uint8_t *paragraphBase,*header;
    uint8_t flags,scaleShift;
    int16_t baseScale;
    uint16_t referenceVector[3];
    int16_t entryCount;
    BtechRenderEntry *entries;
} BtechShapeResource;
typedef struct BtechResourceBank {
    BtechResourceAllocation allocation;
    BtechShapeResource **resources;
    uint16_t resourceCount;
    BtechShapeResource *decodedResources; /* Host allocation owner, independent of live directory redirects. */
    uint16_t decodedResourceCount;
} BtechResourceBank;

typedef struct BtechSurfaceBoundary {
    int16_t coefficientA,coefficientB,anchorA,anchorB;
} BtechSurfaceBoundary;
typedef struct BtechSurfacePatch {
    int16_t orderingPlaneCoefficients[3];
    uint8_t heightProductScale,orientationResponseMagnitude,orientationResponseHeading,boundaryCount;
    BtechSurfaceBoundary *boundaries;
    int16_t baseHeight;
    BtechSurfaceBoundary heightBoundary;
} BtechSurfacePatch;
typedef struct BtechSurfaceDefinition {
    uint16_t extentA,extentB;
    uint8_t directResultFlag,priorityClass,surfaceClass,patchCount;
    BtechSurfacePatch *patches;
} BtechSurfaceDefinition;
typedef struct BtechSurfaceBank {
    BtechResourceAllocation allocation;
    BtechSurfaceDefinition **surfaces;
    uint16_t resourceCount;
} BtechSurfaceBank;
typedef struct BtechLzwReaderWorkspace {
    uint16_t prefixCodes[LZW_DICTIONARY_LIMIT];
    uint8_t unidentifiedField23[0x720],suffixBytes[LZW_DICTIONARY_LIMIT],reversePhraseBytes[0x393];
} BtechLzwReaderWorkspace;
extern BtechLzwReaderWorkspace *borrowedArchiveWorkspace;
extern uint8_t archiveStreamScratch[0x32];
typedef struct BtechArchiveDescriptor {
    uint8_t *state;
    BtechLzwReaderWorkspace *workspace;
    FILE *stream;
    uint32_t compressedPosition,compressedBytes,decodedBytes,decodedPosition;
    uint8_t pendingEnd,pendingRead;
    uint32_t initialStreamPosition;
    uint8_t methodAndFlags;
} BtechArchiveDescriptor;
extern BtechResourceBank *shapeResourceBanks,initialShapeResourceBank;
extern BtechSurfaceBank surfaceResourceBank;
extern uint16_t shapeResourceBankCount;
extern char resourceFilename[0x50];
extern BtechArchiveDescriptor *archiveDescriptors[RESOURCE_ARCHIVE_SLOTS],*selectedArchive;
extern uint8_t *archiveDestination;
extern uint16_t archiveRemainingBytes;
extern uint8_t archiveIoFlags,lzwInitialLiteralPending,lzwPhraseResumePending;
extern uint16_t lzwCodeWidth,lzwCodeWidthLimit,lzwNextFreeCode,lzwPacketBitOffset,lzwPacketBitLimit;
extern uint16_t lzwClearPending,lzwReverseReadOffset,lzwPreviousCode,lzwIncomingCode,lzwPhraseFirstByte;
extern uint8_t lzwPacketBytes[12];
uint16_t BtechSelectArchiveDescriptor(uint16_t handle);
uint16_t BtechArchiveModeContainsRead(const char *mode);
void BtechReleaseNearArchiveBlockIfNonzero(void *allocation);
uint16_t BtechReleaseArchiveDescriptor(uint16_t handle);
int16_t BtechAllocateArchiveDescriptor(const char *ignoredMode);
uint16_t BtechAllocateArchiveMethodState(int16_t method,const char *mode);
int16_t BtechOpenCompressedArchive(uint16_t method,FILE *stream,const char *mode,uint32_t physicalBytes);
uint16_t BtechReadArchiveIntoFarBuffer(uint16_t handle,uint8_t *destination,uint16_t bytes);
uint16_t BtechDecodeArchiveBytes(uint16_t handle,uint16_t requestedBytes);
uint16_t BtechFlushAndCloseCompressedArchive(uint16_t handle);
uint32_t BtechGetArchiveDecodedLength(uint16_t handle);
uint16_t BtechReadArchiveSourceBytes(uint8_t *destination,uint16_t bytes);
uint16_t BtechEmitArchiveByte(uint16_t value);
void BtechDeliverPendingArchiveBytes(void);
void BtechInitializeLzwReader(void);
uint16_t BtechReadLzwCode(void);
void BtechDecodeLzwPacketStream(void);
uint16_t BtechReadArchiveSourceByte(void);
void BtechDecodeRawArchive(void);
void BtechDecodeRleArchive(void);
void BtechCopyArchiveStreamBytes(uint8_t *destination,uint16_t bytes);
uint16_t BtechDecodeRleLiteralRun(uint16_t bytes);
uint16_t BtechDecodeRleRepeatedRun(uint16_t value,uint16_t bytes);
uint32_t BtechGetDecodedResourceLength(const char *filename);
void BtechDecodeResourceIntoAllocation(const char *filename,BtechResourceAllocation allocation);
BtechResourceAllocation BtechAllocateAndDecodeResource(const char *filename);
BtechResourceBank BtechDecodeAndRelocateResourceBank(const char *filename);
void BtechLoadCollisionResourceBank(const char *filename);
void BtechReleaseCollisionBankAllocation(void);
void BtechReleaseResourceBankSelector(BtechResourceBank bank);
BtechResourceBank BtechDetachShapeResourceBank(uint16_t bankSlot);
void BtechAllocateShapeBankRegistry(uint16_t count);
void BtechBindShapeBankDirectory(BtechResourceBank bank,uint16_t bankSlot);
uint16_t BtechCountShapeResourceBankEntries(uint16_t bankSlot);
const BtechShapeResource *BtechLookupBankResource(uint16_t resourceId);
void BtechSetShapeResourceEntryAddress(uint16_t resourceId,BtechShapeResource *resource);
uint32_t BtechGetResourceScaledValue(uint16_t resourceId);
uint16_t BtechGetResourceScaleShift(uint16_t resourceId);
const char *BtechBuildFilenameWithExtension(const char *basename,const char *extension);
#endif
