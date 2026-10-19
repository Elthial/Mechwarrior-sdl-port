#ifndef MECHWARRIOR_COMBAT_SCENE_H
#define MECHWARRIOR_COMBAT_SCENE_H
#include "mechwarrior/combat_world.h"
#include "mechwarrior/combat_resources.h"
enum { WORLD_DISTANCE_BUCKETS=0xC8,WORLD_DISTANCE_BUCKET_TARGET=0xC6,WORLD_DISTANCE_BUCKET_OVERFLOW=0xC7,
    WORLD_DISTANCE_OVERFLOW_THRESHOLD=0x8E00,WORLD_NEARBY_LIMIT=0xC350,WORLD_FILTER_REUSE_DISTANCE=0x400,
    WORLD_ORDER_INDEX_BANK_BYTES=0x100,OBJECT_LAST_MECH_DRAW_PART=0x08 };
typedef union BtechObjectSortScratch {
    uint32_t distances[COMBAT_WORLD_OBJECTS];
    uint8_t priorityBytes[COMBAT_WORLD_OBJECTS*4];
    struct {
        uint8_t ordinaryIndices[WORLD_ORDER_INDEX_BANK_BYTES];
        union {uint8_t attachedIndices[WORLD_ORDER_INDEX_BANK_BYTES];BtechWorldObjectPrefix *objects[COMBAT_WORLD_OBJECTS];} tail;
    } surfaceOrder;
} BtechObjectSortScratch;
extern BtechObjectSortScratch objectSortOrSelectionScratch;
extern BtechWorldObjectPrefix *distanceSortObjects[COMBAT_WORLD_OBJECTS];
extern uint32_t distanceSortDistances[COMBAT_WORLD_OBJECTS];
extern uint16_t distanceBucketShift,surfacePriorityPrefixCount;
extern uint16_t distanceBucketEnds[WORLD_DISTANCE_BUCKETS],distanceBucketCounts[WORLD_DISTANCE_BUCKETS];
extern BtechPosition3D nearbyFilterCameraPosition;
void BtechInitializeFreshSceneOwners(void);
int16_t BtechFilterNearbyWorldObjects(BtechCameraContext *camera,BtechWorldObjectPrefix *const *source,int16_t sourceCount,BtechWorldObjectPrefix **destination,int16_t previousCount);
int16_t BtechSortWorldObjectsBySurfacePriority(BtechWorldObjectPrefix **objects,int16_t count,uint8_t *priorities);
void BtechChooseDistanceBucketShift(uint32_t maximum);
uint16_t BtechGetDistanceBucket(uint32_t distance);
void BtechBucketSortWorldObjectsByDistance(BtechWorldObjectPrefix **objects,int16_t count,uint32_t *distances,uint32_t maximum);
uint32_t BtechApproximatePlanarDistance(const BtechPosition3D *first,const BtechPosition3D *second);
void BtechSortFarWorldObjectsForView(BtechCameraContext *camera,BtechWorldObjectPrefix **objects,int16_t count,BtechObjectSortScratch *scratch);
uint16_t BtechOrderObjectsAroundSurfacePlanes(BtechCameraContext *camera,BtechWorldObjectPrefix **objects,int16_t count,BtechObjectSortScratch *scratch);
uint16_t BtechCalculateVectorHeadingFar(uint32_t componentA,uint32_t componentB);
void BtechOrderMechRenderParts(BtechCameraContext *camera,BtechWorldObjectPrefix **objects,int16_t count);
#endif
