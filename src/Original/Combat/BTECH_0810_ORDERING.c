#include "mechwarrior/combat_scene.h"
#include "mechwarrior/combat_startup.h"
#include <stdlib.h>
#include <string.h>

/*0810:BB84. Genuine FAR veneer, two DWORD arguments forwarded unchanged. */
uint16_t BtechCalculateVectorHeadingFar(uint32_t componentA,uint32_t componentB)
{
    return BtechCalculateVectorHeading(componentA,componentB);
}

/*0810:BA56. Descending BYTE priority selection sort. Native does NOT test
 * the final key for zero; a lone last zero remains part of the prefix.
 * Original capture20261017 proves sixFFFF parts read priorityE9 through an
 * out-of-bank bitmap-byte/far-pointer/code alias; see ORIGINAL_COMBAT_CAPTURE_20261017.md.
 * The physical DOS alias boundary below resolves recovered typed byte owners;
 * it must not turn that observed load layout into an unconditional E9 default. */
int16_t BtechSortWorldObjectsBySurfacePriority(BtechWorldObjectPrefix **objects,int16_t count,uint8_t *priorities)
{
    if (count<2) return 0;
    for (int16_t index=0;index<count;++index)
    {
        if (index>=COMBAT_WORLD_OBJECTS || !objects[index]) abort();
        int16_t resource=(int16_t)objects[index]->resourceId;
        uint8_t priority=0;
        if (resource<(int16_t)surfaceResourceBank.resourceCount)
        {
            if (resource<0)
            {
                if (!btechHostResolveWrappedResourcePriority) abort();
                const uint8_t *priorityByte=btechHostResolveWrappedResourcePriority(surfaceResourceBank.allocation.bytes,resource);
                if (!priorityByte) abort();
                priority=*priorityByte;
            }
            else
            {
                if (!surfaceResourceBank.surfaces || !surfaceResourceBank.surfaces[resource]) abort();
                priority=surfaceResourceBank.surfaces[resource]->priorityClass;
            }
        }
        priorities[index]=priority;
    }
    int16_t firstZero=-1;
    for (int16_t index=0;index<count-1;++index)
    {
        int16_t greatest=index;uint8_t priority=priorities[index];
        for (int16_t candidate=index+1;candidate<count;++candidate)
            if (priorities[candidate]>priority) {greatest=candidate;priority=priorities[candidate];}
        if (greatest!=index)
        {
            priorities[greatest]=priorities[index];priorities[index]=priority;
            BtechWorldObjectPrefix *saved=objects[greatest];objects[greatest]=objects[index];objects[index]=saved;
        }
        if (firstZero==-1 && !priorities[index]) firstZero=index;
    }
    return firstZero==-1?count:firstZero;
}

/*0810:B64C. Native tick-ZERO reuse test, three independent axis bounds and
 * positive00008000 quirk. This is not a spherical-distance cull. */
int16_t BtechFilterNearbyWorldObjects(BtechCameraContext *camera,BtechWorldObjectPrefix *const *source,int16_t sourceCount,BtechWorldObjectPrefix **destination,int16_t previousCount)
{
    if (!camera || !camera->viewObject) abort();
    const BtechPosition3D *position=&camera->viewObject->position;
    uint32_t differences[3];
    if (!combatTickCounter)
    {
        differences[0]=position->positionA-nearbyFilterCameraPosition.positionA;
        differences[1]=position->positionB-nearbyFilterCameraPosition.positionB;
        differences[2]=position->positionC-nearbyFilterCameraPosition.positionC;
        uint8_t reuse=1;
        for (uint16_t axis=0;axis<3;++axis)
        {
            uint32_t magnitude=differences[axis];
            if (magnitude==0x00008000) magnitude=0x00007FFF;
            else if (magnitude&UINT32_C(0x80000000)) magnitude=0-magnitude;
            if ((int16_t)(magnitude>>16)>0 || (!(magnitude>>16) && (uint16_t)magnitude>=WORLD_FILTER_REUSE_DISTANCE))
            {reuse=0;break;}
        }
        if (reuse) return previousCount;
    }
    nearbyFilterCameraPosition.positionA=position->positionA;
    nearbyFilterCameraPosition.positionB=position->positionB;
    nearbyFilterCameraPosition.positionC=position->positionC;
    int16_t count=0;
    for (int16_t index=0;index<sourceCount;++index)
    {
        if (index>=COMBAT_WORLD_OBJECTS || !source[index]) abort();
        BtechWorldObjectPrefix *object=source[index];
        differences[0]=nearbyFilterCameraPosition.positionA-object->position.positionA;
        differences[1]=nearbyFilterCameraPosition.positionB-object->position.positionB;
        differences[2]=nearbyFilterCameraPosition.positionC-object->position.positionC;
        uint8_t nearby=1;
        for (uint16_t axis=0;axis<3;++axis)
        {
            uint32_t magnitude=differences[axis];
            if (magnitude==0x00008000) magnitude=0x00007FFF;
            else if (magnitude&UINT32_C(0x80000000)) magnitude=0-magnitude;
            if ((int16_t)(magnitude>>16)>0 || (!(magnitude>>16) && (uint16_t)magnitude>=WORLD_NEARBY_LIMIT))
            {nearby=0;break;}
        }
        if (nearby) destination[count++]=object;
        else object->renderFlags&=(uint8_t)~WORLD_OBJECT_PROJECTED;
    }
    surfacePriorityPrefixCount=BtechSortWorldObjectsBySurfacePriority(destination,count,objectSortOrSelectionScratch.priorityBytes);
    return count;
}

/*0810:B3C8. At least one and at most nine arithmetic DWORD shifts. */
void BtechChooseDistanceBucketShift(uint32_t maximum)
{
    uint16_t shifts=0;
    do {++shifts;maximum=(maximum>>1)|(maximum&UINT32_C(0x80000000));}
    while (((int16_t)(maximum>>16)>0 || (!(maximum>>16) && (uint16_t)maximum>WORLD_DISTANCE_BUCKET_TARGET)) && shifts<9);
    distanceBucketShift=shifts;
}

/*0810:B3EE. LOW BYTE shift count, native overflow bucket and signed shift. */
uint16_t BtechGetDistanceBucket(uint32_t distance)
{
    if ((int16_t)(distance>>16)>0 || (!(distance>>16) && (uint16_t)distance>=WORLD_DISTANCE_OVERFLOW_THRESHOLD)) return WORLD_DISTANCE_BUCKET_OVERFLOW;
    uint8_t shifts=(uint8_t)distanceBucketShift;
    while (shifts--) distance=(distance>>1)|(distance&UINT32_C(0x80000000));
    return (uint16_t)distance;
}

/*0810:B414. Native bucket insertion, ascending in-bucket bubble passes,
 * then reverse copy to far-to-near order. Equal-distance ordering is retained. */
void BtechBucketSortWorldObjectsByDistance(BtechWorldObjectPrefix **objects,int16_t count,uint32_t *distances,uint32_t maximum)
{
    if (count<2) return;
    BtechChooseDistanceBucketShift(maximum);
    for (uint16_t bucket=0;bucket<WORLD_DISTANCE_BUCKETS;++bucket) distanceBucketCounts[bucket]=0;
    for (int16_t index=0;index<count;++index)
    {
        if (index>=COMBAT_WORLD_OBJECTS) abort();
        uint16_t bucket=BtechGetDistanceBucket(distances[index]);
        if (bucket>=WORLD_DISTANCE_BUCKETS) abort();
        ++distanceBucketCounts[bucket];
    }
    uint16_t total=0;
    for (uint16_t bucket=0;bucket<WORLD_DISTANCE_BUCKETS;++bucket)
    {total+=distanceBucketCounts[bucket];distanceBucketEnds[bucket]=total;}
    for (int16_t index=0;index<count;++index)
    {
        uint16_t bucket=BtechGetDistanceBucket(distances[index]);
        if (bucket>=WORLD_DISTANCE_BUCKETS) abort();
        uint16_t destination=--distanceBucketEnds[bucket];
        if (destination>=COMBAT_WORLD_OBJECTS) abort();
        distanceSortDistances[destination]=distances[index];distanceSortObjects[destination]=objects[index];
    }
    uint16_t start=0;
    for (uint16_t bucket=0;bucket<WORLD_DISTANCE_BUCKETS;++bucket)
    {
        uint16_t bucketCount=distanceBucketCounts[bucket];uint8_t swapped=1;
        for (int16_t comparisons=(int16_t)(uint16_t)(bucketCount-1);(int16_t)bucketCount>1 && comparisons>0 && swapped;--comparisons)
        {
            swapped=0;
            for (uint16_t index=0;index<(uint16_t)comparisons;++index)
            {
                uint16_t at=start+index;
                if (at+1>=COMBAT_WORLD_OBJECTS) abort();
                if ((int32_t)distanceSortDistances[at+1]<(int32_t)distanceSortDistances[at])
                {
                    uint32_t savedDistance=distanceSortDistances[at];
                    distanceSortDistances[at]=distanceSortDistances[at+1];distanceSortDistances[at+1]=savedDistance;
                    BtechWorldObjectPrefix *savedObject=distanceSortObjects[at];
                    distanceSortObjects[at]=distanceSortObjects[at+1];distanceSortObjects[at+1]=savedObject;swapped=1;
                }
            }
        }
        start+=bucketCount;
    }
    for (int16_t index=0;index<count;++index)
    {objects[index]=distanceSortObjects[count-index-1];distances[index]=distanceSortDistances[count-index-1];}
}

/*0810:B96D. Wrapped planar max+3/8 min, including the positive8000 quirk. */
uint32_t BtechApproximatePlanarDistance(const BtechPosition3D *first,const BtechPosition3D *second)
{
    uint32_t a=second->positionA-first->positionA,b=second->positionB-first->positionB;
    if (a==0x00008000) a=0x7FFF;else if (a&UINT32_C(0x80000000)) a=0-a;
    if (b==0x00008000) b=0x7FFF;else if (b&UINT32_C(0x80000000)) b=0-b;
    if ((int32_t)b>(int32_t)a) {uint32_t saved=a;a=b;b=saved;}
    b*=3;
    for (uint16_t shift=0;shift<3;++shift) b=(b>>1)|(b&UINT32_C(0x80000000));
    return a+b;
}

/*0810:C8C0. BYTE index banks and the later typed object snapshot share one
 * owner. Missing parents drop attached entries from the rebuilt PREFIX only. */
uint16_t BtechOrderObjectsAroundSurfacePlanes(BtechCameraContext *camera,BtechWorldObjectPrefix **objects,int16_t count,BtechObjectSortScratch *scratch)
{
    if (count<2 || count>WORLD_ORDER_INDEX_BANK_BYTES) return (uint16_t)count;
    uint16_t ordinaryCount=0,attachedCount=0;
    for (int16_t index=0;index<count;++index)
    {
        if (index>=COMBAT_WORLD_OBJECTS || !objects[index]) abort();
        BtechWorldObjectPrefix *object=objects[index];uint8_t attached=0;
        if (object->flags&WORLD_OBJECT_POOLED)
        {
            BtechSurfaceQueryResult *query=&((BtechPooledWorldObject *)object)->surfaceQuery;
            if (query->worldObject)
            {if (!query->surface) abort();attached=query->surface->priorityClass==0;}
        }
        if (attached) scratch->surfaceOrder.tail.attachedIndices[attachedCount++]=(uint8_t)index;
        else scratch->surfaceOrder.ordinaryIndices[ordinaryCount++]=(uint8_t)index;
    }
    if (!attachedCount) return (uint16_t)count;
    if (!camera || !camera->viewObject) abort();
    const BtechPosition3D *cameraPosition=&camera->viewObject->position;
    for (uint16_t remaining=attachedCount;remaining;)
    {
        uint8_t objectIndex=scratch->surfaceOrder.tail.attachedIndices[--remaining];
        BtechSurfaceQueryResult *query=&((BtechPooledWorldObject *)objects[objectIndex])->surfaceQuery;
        uint16_t insertion=0;
        while (insertion<ordinaryCount && objects[scratch->surfaceOrder.ordinaryIndices[insertion]]!=query->worldObject) ++insertion;
        if (insertion>=ordinaryCount) continue;
        const BtechSurfaceDefinition *surface=query->surface;
        if (!surface || query->patchIndex>=surface->patchCount) abort();
        const BtechSurfacePatch *patch=&surface->patches[query->patchIndex];
        const BtechSurfaceBoundary *boundary=&patch->boundaries[0];
        const BtechPosition3D *parent=&query->worldObject->position;
        uint32_t a=(uint32_t)(int32_t)boundary->anchorA+parent->positionA-cameraPosition->positionA;
        uint32_t b=(uint32_t)(int32_t)boundary->anchorB+parent->positionB-cameraPosition->positionB;
        uint32_t c=(uint32_t)(int32_t)patch->baseHeight+parent->positionC-cameraPosition->positionC;
        uint32_t product=(uint32_t)(int32_t)patch->orderingPlaneCoefficients[0]*a;
        product+=(uint32_t)(int32_t)patch->orderingPlaneCoefficients[1]*b;
        product+=(uint32_t)(int32_t)patch->orderingPlaneCoefficients[2]*c;
        if (product&UINT32_C(0x80000000)) ++insertion;
        memmove(&scratch->surfaceOrder.ordinaryIndices[insertion+1],&scratch->surfaceOrder.ordinaryIndices[insertion],ordinaryCount-insertion);
        scratch->surfaceOrder.ordinaryIndices[insertion]=scratch->surfaceOrder.tail.attachedIndices[remaining];++ordinaryCount;
    }
    for (int16_t index=0;index<count;++index) scratch->surfaceOrder.tail.objects[index]=objects[index];
    for (uint16_t index=0;index<ordinaryCount;++index) objects[index]=scratch->surfaceOrder.tail.objects[scratch->surfaceOrder.ordinaryIndices[index]];
    return ordinaryCount;
}

/*0810:B8B4. Keep the priority prefix, sort only the tail, ignore C8C0's
 * rebuilt count. Distance scratch begins at zero, not at the prefix index. */
void BtechSortFarWorldObjectsForView(BtechCameraContext *camera,BtechWorldObjectPrefix **objects,int16_t count,BtechObjectSortScratch *scratch)
{
    uint32_t maximum=0;uint16_t distanceIndex=0;
    for (int16_t index=(int16_t)surfacePriorityPrefixCount;index<count;++index)
    {
        if (index<0 || index>=COMBAT_WORLD_OBJECTS || !camera || !camera->viewObject || !objects[index]) abort();
        uint32_t distance=BtechApproximatePlanarDistance(&camera->viewObject->position,&objects[index]->position);
        scratch->distances[distanceIndex++]=distance;
        if ((int32_t)distance>(int32_t)maximum) maximum=distance;
    }
    if (surfacePriorityPrefixCount>COMBAT_WORLD_OBJECTS) abort();
    BtechWorldObjectPrefix **tail=objects+surfacePriorityPrefixCount;
    int16_t tailCount=(int16_t)(uint16_t)(count-surfacePriorityPrefixCount);
    BtechBucketSortWorldObjectsByDistance(tail,tailCount,scratch->distances,maximum);
    (void)BtechOrderObjectsAroundSurfacePlanes(camera,objects+surfacePriorityPrefixCount,
        (int16_t)(uint16_t)(count-surfacePriorityPrefixCount),scratch);
}
