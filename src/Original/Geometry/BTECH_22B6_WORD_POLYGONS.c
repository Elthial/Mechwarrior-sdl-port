#include "mechwarrior/combat_render.h"
#include <stdlib.h>

/*22B6:6A24. WORD-stride cache, any nonzero flag reuses existing coordinates.
 * Native geometry BP is an ES offset, not a stack frame. */
void BtechCacheTransformedPolygonVertex(uint8_t vectorIndex)
{
    if (vectorIndex>=GEOMETRY_CACHE_VERTICES) abort();
    if (vertexNearPlaneState[vectorIndex]) return;
    vertexNearPlaneState[vectorIndex]=GEOMETRY_VERTEX_TRANSFORMED;
    for (uint16_t axis=0;axis<3;++axis) viewVectorCoefficients[axis]=renderGeometry[vectorIndex][axis];
    BtechTransformWordVector(viewVectorCoefficients,activeRenderBasis,transformedEntryVector);
    geometryViewX.words[vectorIndex]=(uint16_t)(transformedEntryVector[0]+(uint16_t)transformedViewPosition.positionA);
    geometryViewDepth.words[vectorIndex]=(uint16_t)(transformedEntryVector[1]+(uint16_t)transformedViewPosition.positionB);
    geometryViewZ.words[vectorIndex]=(uint16_t)(transformedEntryVector[2]+(uint16_t)transformedViewPosition.positionC);
}

/*22B6:6AC2. Normals share the vertex cache flags but omit translation. */
void BtechCacheTransformedPolygonNormal(uint8_t vectorIndex)
{
    if (vectorIndex>=GEOMETRY_CACHE_VERTICES) abort();
    if (vertexNearPlaneState[vectorIndex]) return;
    vertexNearPlaneState[vectorIndex]=GEOMETRY_VERTEX_TRANSFORMED;
    for (uint16_t axis=0;axis<3;++axis) viewVectorCoefficients[axis]=renderGeometry[vectorIndex][axis];
    BtechTransformWordVector(viewVectorCoefficients,activeRenderBasis,transformedEntryVector);
    geometryViewX.words[vectorIndex]=transformedEntryVector[0];
    geometryViewDepth.words[vectorIndex]=transformedEntryVector[1];
    geometryViewZ.words[vectorIndex]=transformedEntryVector[2];
}

/*22B6:6BD0, including shared6B51 direct-basis continuation, not a new call.
 * Return only the HIGH WORD observed by276A; native dot wraps at32 bits. */
int16_t BtechGetPolygonFacingDotHighWord(uint8_t normalIndex,uint8_t firstVertexIndex)
{
    if (normalIndex>=GEOMETRY_CACHE_VERTICES || firstVertexIndex>=GEOMETRY_CACHE_VERTICES) abort();
    uint16_t vertex[3],normal[3];
    if (useViewBasisDirectly)
    {
        vertex[0]=(uint16_t)(renderGeometry[firstVertexIndex][0]+(uint16_t)cameraRelativePosition.positionA);
        vertex[1]=(uint16_t)(renderGeometry[firstVertexIndex][1]+(uint16_t)cameraRelativePosition.positionB);
        vertex[2]=(uint16_t)(renderGeometry[firstVertexIndex][2]+(uint16_t)cameraRelativePosition.positionC);
        for (uint16_t axis=0;axis<3;++axis) normal[axis]=renderGeometry[normalIndex][axis];
    }
    else
    {
        BtechCacheTransformedPolygonNormal(normalIndex);
        BtechCacheTransformedPolygonVertex(firstVertexIndex);
        vertex[0]=geometryViewX.words[firstVertexIndex];normal[0]=geometryViewX.words[normalIndex];
        vertex[1]=geometryViewDepth.words[firstVertexIndex];normal[1]=geometryViewDepth.words[normalIndex];
        vertex[2]=geometryViewZ.words[firstVertexIndex];normal[2]=geometryViewZ.words[normalIndex];
    }
    uint32_t dot=0;
    for (uint16_t axis=0;axis<3;++axis) dot+=(uint32_t)((int32_t)(int16_t)vertex[axis]*(int16_t)normal[axis]);
    return (int16_t)(dot>>16);
}

/*22B6:2601. Crossing overwrites the OUTSIDE vertex's projected cache slot.
 * Differences wrap toWORD before signed IMUL and trapping IDIV. */
void BtechIntersectWordEdgeWithNearPlane(uint16_t outsideVertex,uint16_t insideVertex)
{
    if (outsideVertex>=GEOMETRY_CACHE_VERTICES || insideVertex>=GEOMETRY_CACHE_VERTICES) abort();
    int16_t denominator=(int16_t)(uint16_t)(geometryViewDepth.words[insideVertex]-geometryViewDepth.words[outsideVertex]);
    int16_t planeDistance=(int16_t)(uint16_t)(nearClipDepth-geometryViewDepth.words[outsideVertex]);
    BtechGeometryCoordinateCache *source[]={&geometryViewX,&geometryViewZ};
    BtechGeometryCoordinateCache *destination[]={&geometryProjectedX,&geometryProjectedY};
    for (uint16_t axis=0;axis<2;++axis)
    {
        int16_t difference=(int16_t)(uint16_t)(source[axis]->words[insideVertex]-source[axis]->words[outsideVertex]);
        uint32_t product=(uint32_t)((int32_t)difference*planeDistance);
        if (!denominator) abort(); /* Native zero-divisor retries forever. */
        int64_t quotient=(int64_t)(int32_t)product/denominator;
        while (quotient<INT16_MIN || quotient>INT16_MAX)
        {product=BtechDivideErrorInterrupt(product);quotient=(int64_t)(int32_t)product/denominator;}
        destination[axis]->words[outsideVertex]=(uint16_t)(source[axis]->words[outsideVertex]+(int16_t)quotient);
    }
}

/*22B6:2653. Equality-count loop and crossing/current output order. No new
 * vertices: intersections reuse the outside index. */
void BtechClipWordPolygonNearPlane(void)
{
    if (!polygonVertexCount || polygonVertexCount>POLYGON_VERTEX_INDEX_BYTES) abort(); /* Native empty aliases4616. */
    uint16_t previous=polygonVertexIndices[polygonVertexCount-1],output=0,input=0;
    do
    {
        uint16_t current=polygonVertexIndices[input],emit[2],emitted=0;
        if (previous>=GEOMETRY_CACHE_VERTICES || current>=GEOMETRY_CACHE_VERTICES) abort();
        if (vertexNearPlaneState[current]!=GEOMETRY_VERTEX_BEHIND)
        {
            if (vertexNearPlaneState[previous]==GEOMETRY_VERTEX_BEHIND)
            {BtechIntersectWordEdgeWithNearPlane(previous,current);emit[emitted++]=previous;}
            emit[emitted++]=current;
        }
        else if (vertexNearPlaneState[previous]!=GEOMETRY_VERTEX_BEHIND)
        {BtechIntersectWordEdgeWithNearPlane(current,previous);emit[emitted++]=current;}
        for (uint16_t index=0;index<emitted;++index)
        {
            if (output>=COMBAT_POLYGON_VERTEX_WORDS) abort();
            screenPolygonX[output]=(int16_t)(uint16_t)((uint16_t)screenCentreA+geometryProjectedX.words[emit[index]]);
            screenPolygonY[output]=(int16_t)(uint16_t)((uint16_t)screenCentreB-geometryProjectedY.words[emit[index]]);
            ++output;
        }
        previous=current;++input;
    } while (input!=polygonVertexCount);
    polygonVertexCount=output;
}

/*22B6:276A. FAR tails select6836 or genuine DBEE RETF; otherwise native WORD
 * facing, projection, near clipping and shared0475 viewport/raster path. */
void BtechDrawPolygonPayload(const BtechRenderPolygonPayload *payload)
{
    if (widePolygonCoordinates) {BtechRenderWidePolygonPayload(payload);return;}
    if (skipWordPolygonRenderer) return;
    wordPolygonsRemaining=(uint16_t)payload->polygonCount;
    const BtechRenderPolygon *polygon=payload->polygons;
    for (;;++polygon)
    {
        /* DEC/JGE observes overflow too:8000 ->7FFF still exits. */
        if ((int16_t)wordPolygonsRemaining--<=0) return;
        uint8_t flags=polygon->flags;
        drawPolygonFilled=(flags&POLYGON_FILLED)!=0;
        if ((flags&POLYGON_FACING_MASK)==2) continue;
        if ((flags&POLYGON_FACING_MASK)!=0
            && BtechGetPolygonFacingDotHighWord(polygon->normalVectorIndex,polygon->vertexIndices[0])>=0) continue;
        BtechSelectPolygonColoursAndShade(polygon);
        wordPolygonHasNearOutsideVertex=0;polygonVertexCount=0;
        for (const uint8_t *indices=polygon->vertexIndices;*indices!=POLYGON_VERTEX_LIST_END;++indices)
        {
            uint8_t vertex=*indices;
            BtechCacheTransformedPolygonVertex(vertex);
            if (vertexNearPlaneState[vertex]!=GEOMETRY_VERTEX_PROJECTED)
            {
                if ((int16_t)geometryViewDepth.words[vertex]<(int16_t)nearClipDepth)
                {vertexNearPlaneState[vertex]=GEOMETRY_VERTEX_BEHIND;wordPolygonHasNearOutsideVertex=GEOMETRY_VERTEX_BEHIND;}
                else
                {
                    BtechGeometryCoordinateCache *source[]={&geometryViewX,&geometryViewZ};
                    BtechGeometryCoordinateCache *destination[]={&geometryProjectedX,&geometryProjectedY};
                    int16_t depth=(int16_t)geometryViewDepth.words[vertex];
                    for (uint16_t axis=0;axis<2;++axis)
                    {
                        uint32_t numerator=(uint32_t)(int32_t)(int16_t)source[axis]->words[vertex];
                        uint16_t shifts=combatProjectionShift;
                        if ((int8_t)(uint8_t)shifts>=8)
                        {
                            numerator<<=8;shifts=(shifts&0xFF00)|(uint8_t)((uint8_t)shifts-8);
                            if (!(uint8_t)shifts) goto divideCoordinate;
                        }
                        do {numerator<<=1;} while (--shifts);
divideCoordinate:
                        if (!depth) abort(); /* Native division fault cannot progress on zero. */
                        int64_t quotient=(int64_t)(int32_t)numerator/depth;
                        while (quotient<INT16_MIN || quotient>INT16_MAX)
                        {numerator=BtechDivideErrorInterrupt(numerator);quotient=(int64_t)(int32_t)numerator/depth;}
                        destination[axis]->words[vertex]=(uint16_t)quotient;
                    }
                    vertexNearPlaneState[vertex]=GEOMETRY_VERTEX_PROJECTED;
                }
            }
            if (polygonVertexCount>=POLYGON_VERTEX_INDEX_BYTES) abort();
            polygonVertexIndices[polygonVertexCount++]=vertex;
        }
        if (wordPolygonHasNearOutsideVertex) BtechClipWordPolygonNearPlane();
        else
        {
            if (!polygonVertexCount) continue;
            for (uint16_t index=0;index<polygonVertexCount;++index)
            {
                if (index>=COMBAT_POLYGON_VERTEX_WORDS) abort();
                uint8_t vertex=polygonVertexIndices[index];
                screenPolygonX[index]=(int16_t)(uint16_t)((uint16_t)screenCentreA+geometryProjectedX.words[vertex]);
                screenPolygonY[index]=(int16_t)(uint16_t)((uint16_t)screenCentreB-geometryProjectedY.words[vertex]);
            }
        }
        screenPolygonNeedsClip=objectNeedsScreenClip;
        BtechDrawScreenPolygon((int16_t)polygonVertexCount,screenPolygonX,screenPolygonY);
    }
}
