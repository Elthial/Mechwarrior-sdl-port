#include "mechwarrior/combat_scene.h"
BtechObjectSortScratch objectSortOrSelectionScratch;
BtechWorldObjectPrefix *distanceSortObjects[COMBAT_WORLD_OBJECTS];
uint32_t distanceSortDistances[COMBAT_WORLD_OBJECTS];
uint16_t distanceBucketShift,surfacePriorityPrefixCount;
uint16_t distanceBucketEnds[WORLD_DISTANCE_BUCKETS],distanceBucketCounts[WORLD_DISTANCE_BUCKETS];
BtechPosition3D nearbyFilterCameraPosition;
/* Fresh EXEC/BSS projection, not a new game reset procedure. */
void BtechInitializeFreshSceneOwners(void)
{
    objectSortOrSelectionScratch=(BtechObjectSortScratch){0};
    for (uint16_t slot=0;slot<COMBAT_WORLD_OBJECTS;++slot)
    {distanceSortObjects[slot]=NULL;distanceSortDistances[slot]=0;}
    for (uint16_t bucket=0;bucket<WORLD_DISTANCE_BUCKETS;++bucket)
    {distanceBucketEnds[bucket]=0;distanceBucketCounts[bucket]=0;}
    distanceBucketShift=surfacePriorityPrefixCount=0;
    nearbyFilterCameraPosition=(BtechPosition3D){0};
}
