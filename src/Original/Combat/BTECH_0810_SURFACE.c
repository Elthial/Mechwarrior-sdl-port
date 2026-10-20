#include "mechwarrior/combat_surface.h"
#include "mechwarrior/combat_render.h"
#include "mechwarrior/combat_startup.h"
#include <stdlib.h>

/*0810:D190. Same-sign shortcuts precede the potentially overflowing sum.
 * Boundary equality is inside. No bank and zero boundaries both reject. */
uint8_t BtechTestPatchBoundaryHalfPlanes(const BtechSurfacePatch *patch,uint16_t queryA,uint16_t queryB)
{
    if (!surfaceResourceBank.allocation.bytes) return 0;
    uint16_t count=patch->boundaryCount;
    if (!count) return 0;
    if (count>SURFACE_MAXIMUM_BOUNDARIES) BtechShowFatalErrorAndExit("too many edges"); // Authored304E:2CB6.
    if (!patch->boundaries) abort();
    for (uint16_t index=0;index<count;++index)
    {
        const BtechSurfaceBoundary *boundary=&patch->boundaries[index];
        int16_t coefficientB=boundary->coefficientB;
        int16_t differenceB=coefficientB?(int16_t)(queryB-(uint16_t)boundary->anchorB):0;
        int16_t coefficientA=boundary->coefficientA;
        int16_t differenceA=coefficientA?(int16_t)(queryA-(uint16_t)boundary->anchorA):0;
        uint8_t positiveA=(coefficientA>0)==(differenceA>0);
        uint8_t positiveB=(coefficientB>0)==(differenceB>0);
        int32_t result;
        if (!coefficientB || !differenceB) result=(!coefficientA || !differenceA)?0:(positiveA?1:-1);
        else if (!coefficientA || !differenceA) result=positiveB?1:-1;
        else if (positiveA==positiveB) result=positiveA?1:-1;
        else
        {
            uint32_t productA=(uint32_t)BtechMultiplySignedWords(coefficientA,differenceA);
            uint32_t productB=(uint32_t)BtechMultiplySignedWords(coefficientB,differenceB);
            result=(int32_t)(productA+productB);
        }
        if (result>0) return 0;
    }
    return 1;
}

/*0810:D33C. Wrapped signed products, LOW DWORD scale, then HIGH WORD.
 * This is the authored plane-height equation, not interpolation of vertices. */
uint16_t BtechEvaluateSurfacePatchHeight(const BtechSurfacePatch *patch,uint16_t queryA,uint16_t queryB)
{
    if (!surfaceResourceBank.allocation.bytes) return 0;
    const BtechSurfaceBoundary *boundary=&patch->heightBoundary;
    uint32_t products=0;
    if (boundary->coefficientA)
        products=(uint32_t)BtechMultiplySignedWords(boundary->coefficientA,(int16_t)((uint16_t)boundary->anchorA-queryA));
    if (boundary->coefficientB)
        products+=(uint32_t)BtechMultiplySignedWords(boundary->coefficientB,(int16_t)((uint16_t)boundary->anchorB-queryB));
    uint16_t scaledHeight=products?(uint16_t)((products*patch->heightProductScale)>>16):0;
    return (uint16_t)((uint16_t)patch->baseHeight+scaledHeight);
}

/*0810:D3F2. Cached patch is tried before the ordinary directory scan.
 * No failure clears the result; D5CE owns invalidation. */
uint16_t BtechTestSurfacePatches(BtechSurfaceQueryResult *result,const BtechPosition3D *position,
    const BtechSurfaceDefinition *surface,BtechWorldObjectPrefix *object,uint16_t cachedPatchFirst)
{
    uint32_t relativeA=BtechShiftSignedDwordRight(position->positionA-object->position.positionA,(uint8_t)resourceScaleShift);
    uint16_t queryA=relativeA;
    uint32_t magnitudeA=(int32_t)relativeA<0?0-relativeA:relativeA;
    uint32_t relativeB=BtechShiftSignedDwordRight(position->positionB-object->position.positionB,(uint8_t)resourceScaleShift);
    uint16_t queryB=relativeB;
    uint32_t magnitudeB=(int32_t)relativeB<0?0-relativeB:relativeB;
    if ((int32_t)(int16_t)surface->extentA<(int32_t)magnitudeA
        || (int32_t)(int16_t)surface->extentB<(int32_t)magnitudeB) return 0;
    uint16_t patchIndex=0;
    const BtechSurfacePatch *patch=surface->patches;
    if (!surface->directResultFlag)
    {
        uint8_t accepted=0;
        if ((uint8_t)cachedPatchFirst)
        {
            uint16_t cachedIndex=result->patchIndex;
            if (cachedIndex>=surface->patchCount || !surface->patches) abort(); /* Native overrun aliases unknown storage. */
            patch=&surface->patches[cachedIndex];
            if (BtechTestPatchBoundaryHalfPlanes(patch,queryA,queryB)) {patchIndex=cachedIndex;accepted=1;}
            else patch=surface->patches;
        }
        if (!accepted)
        {
            uint16_t patchCount=surface->patchCount;
            while (patchIndex<patchCount)
            {
                if (!patch) abort();
                if (BtechTestPatchBoundaryHalfPlanes(patch,queryA,queryB)) break;
                ++patchIndex;++patch;
            }
            if (surface->patchCount==patchIndex) return 0;
        }
    }
    result->surface=surface;
    result->worldObject=object;
    result->patchIndex=(uint8_t)patchIndex;
    if (surface->priorityClass) result->surfacePositionC=0;
    else
    {
        if (!patch) abort();
        uint16_t height=BtechEvaluateSurfacePatchHeight(patch,queryA,queryB);
        result->surfacePositionC=(int16_t)height;
        result->surfacePositionC=BtechShiftDwordLeft(result->surfacePositionC,(uint8_t)resourceScaleShift);
        result->surfacePositionC+=object->position.positionC;
    }
    return 1;
}

/*0810:D5CE. Reuse priority-zero cache; otherwise lowest priority wins,
 * with later equal-priority hits replacing earlier ones. Zero stops scanning. */
uint16_t BtechQueryWorldSurface(BtechSurfaceQueryResult *result,const BtechPosition3D *position,
    BtechWorldObjectPrefix *const *objects,int16_t objectCount)
{
    if (!surfaceResourceBank.allocation.bytes) return 0; /* Leave old result entirely untouched. */
    const BtechSurfaceDefinition *surface=result->surface;
    if (surface && !surface->priorityClass)
    {
        BtechWorldObjectPrefix *object=result->worldObject;
        if (!object) abort();
        resourceScaleShift=BtechGetResourceScaleShift(object->resourceId);
        if (BtechTestSurfacePatches(result,position,surface,object,1)) return 1;
    }
    result->surface=NULL;result->worldObject=NULL; /* Patch/height tail stays stale. */
    uint8_t found=0;uint16_t bestPriority=0;
    for (int16_t index=0;index<objectCount;++index)
    {
        if (index>=COMBAT_WORLD_OBJECTS || !objects[index]) abort();
        BtechWorldObjectPrefix *object=objects[index];
        if (!(object->renderFlags&WORLD_OBJECT_SURFACE_QUERY_CANDIDATE)) continue;
        uint16_t resource=object->resourceId;
        if (resource>=surfaceResourceBank.resourceCount || !surfaceResourceBank.surfaces
            || !surfaceResourceBank.surfaces[resource]) abort();
        surface=surfaceResourceBank.surfaces[resource];
        if (!surface->patchCount || (found && surface->priorityClass>bestPriority)) continue;
        resourceScaleShift=BtechGetResourceScaleShift(object->resourceId);
        if (BtechTestSurfacePatches(result,position,surface,object,0))
        {
            found=1;bestPriority=surface->priorityClass;
            if (!bestPriority) break;
        }
    }
    return found;
}

/*0810:C3E4. Signed low BYTE operands, not shortest circular distance. */
uint8_t BtechCalculateByteOrientationStep(uint16_t target,uint16_t current,uint16_t limit)
{
    int16_t difference=(int8_t)(uint8_t)target-(int8_t)(uint8_t)current;
    int16_t magnitude=difference<0?-difference:difference;
    int16_t stepLimit=(int8_t)(uint8_t)limit;
    if (magnitude>stepLimit) magnitude=stepLimit;
    uint8_t step=magnitude;
    return difference<0?(uint8_t)(0x100-(int8_t)step):step;
}

/*0810:C43A. Save orientation high BYTEs before query/height writes. Surface
 * height is added only for priority-zero geometry; other hits return class. */
uint16_t BtechResolveGroundSurfaceAndSlope(BtechPooledWorldObject *object,int16_t startingHeight,
    BtechWorldObjectPrefix *const *objects,int16_t objectCount)
{
    BtechWorldObjectPrefix *pose=&object->object.prefix;
    uint8_t pitch=(uint8_t)(pose->orientationA>>8),roll=(uint8_t)(pose->orientationB>>8);
    uint8_t heading=pose->orientationC>>8;
    pose->position.positionC=(uint32_t)startingHeight;
    uint8_t responseA=0,responseB=0;
    const BtechSurfaceDefinition *surface=NULL;
    if (BtechQueryWorldSurface(&object->surfaceQuery,&pose->position,objects,objectCount))
    {
        surface=object->surfaceQuery.surface;
        if (!surface) abort();
        if (!surface->priorityClass)
        {
            uint16_t patchIndex=object->surfaceQuery.patchIndex;
            if (patchIndex>=surface->patchCount || !surface->patches) abort();
            const BtechSurfacePatch *patch=&surface->patches[patchIndex];
            uint8_t difference=patch->orientationResponseHeading-heading;
            if (difference>=0x80) difference=(uint8_t)(0-difference);
            uint16_t weight=SURFACE_RESPONSE_QUARTER_TURN-difference;
            uint16_t product=weight*patch->orientationResponseMagnitude;
            responseA=(uint8_t)(product>>SURFACE_RESPONSE_FRACTION_BITS);
            difference=(uint8_t)(patch->orientationResponseHeading-heading-SURFACE_RESPONSE_QUARTER_TURN);
            if (difference>=0x80) difference=(uint8_t)(0-difference);
            weight=(uint16_t)(SURFACE_RESPONSE_QUARTER_TURN-difference);
            product=(uint16_t)(weight*patch->orientationResponseMagnitude);
            responseB=(uint8_t)(product>>SURFACE_RESPONSE_FRACTION_BITS);
            pose->position.positionC+=object->surfaceQuery.surfacePositionC;
        }
    }
    responseA=BtechCalculateByteOrientationStep(responseA,pitch,SURFACE_ORIENTATION_STEP_LIMIT);
    object->orientationSteps[0]=(int16_t)((uint16_t)responseA<<8);
    responseB=BtechCalculateByteOrientationStep(responseB,roll,SURFACE_ORIENTATION_STEP_LIMIT);
    object->orientationSteps[1]=(int16_t)((uint16_t)responseB<<8);
    return surface?surface->surfaceClass:0;
}
