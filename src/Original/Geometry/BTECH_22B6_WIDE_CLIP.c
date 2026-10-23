#include "mechwarrior/port_diagnostics.h"
#include "mechwarrior/combat_render.h"
#include <stdlib.h>

/*22B6:6C27. The crossing replaces projected coordinates in the OUTSIDE
 * vertex's existing slot. This is not a new allocated clipping vertex. */
void BtechIntersectWideEdgeWithNearPlane(uint16_t outsideVertex,uint16_t insideVertex)
{
    if (outsideVertex>=GEOMETRY_CACHE_VERTICES || insideVertex>=GEOMETRY_CACHE_VERTICES) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: outsideVertex>=GEOMETRY_CACHE_VERTICES || insideVertex>=GEOMETRY_CACHE_VERTICES");
    uint32_t denominator=(geometryViewDepth.wide[insideVertex]-geometryViewDepth.wide[outsideVertex]);
    BtechGeometryCoordinateCache *coordinates[]={&geometryViewX,&geometryViewZ};
    BtechGeometryCoordinateCache *projected[]={&geometryProjectedX,&geometryProjectedY};
    for (uint16_t axis=0;axis<2;++axis)
    {
        uint32_t planeDistance=(uint32_t)(int16_t)nearClipDepth-geometryViewDepth.wide[outsideVertex];
        uint32_t difference=coordinates[axis]->wide[insideVertex]-coordinates[axis]->wide[outsideVertex];
        uint64_t product=BtechMultiplySignedDwordsWide(difference,planeDistance);
        uint32_t displacement=BtechDivideSignedWideNumerator((int16_t)(product>>48),(uint16_t)(product>>32),(uint16_t)(product>>16),(uint16_t)product,(uint16_t)denominator,(int16_t)(denominator>>16),NULL);
        projected[axis]->wide[outsideVertex]=coordinates[axis]->wide[outsideVertex]+displacement;
    }
}

/*22B6:7240. The crossing replaces projected coordinates in the OUTSIDE
 * vertex's existing slot. This is not a new allocated clipping vertex. */
void BtechIntersectDwordNearEdgeWithWordProducts(uint16_t outsideVertex,uint16_t insideVertex)
{
    if (outsideVertex>=GEOMETRY_CACHE_VERTICES || insideVertex>=GEOMETRY_CACHE_VERTICES) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: outsideVertex>=GEOMETRY_CACHE_VERTICES || insideVertex>=GEOMETRY_CACHE_VERTICES");
    int16_t denominator=geometryViewDepth.wide[insideVertex]-geometryViewDepth.wide[outsideVertex];
    BtechGeometryCoordinateCache *coordinates[]={&geometryViewX,&geometryViewZ};
    BtechGeometryCoordinateCache *projected[]={&geometryProjectedX,&geometryProjectedY};
    for (uint16_t axis=0;axis<2;++axis)
    {
        uint32_t planeDistance=(uint32_t)(int16_t)nearClipDepth-geometryViewDepth.wide[outsideVertex];
        uint32_t difference=coordinates[axis]->wide[insideVertex]-coordinates[axis]->wide[outsideVertex];
        uint32_t product=(int32_t)(int16_t)difference*(int16_t)planeDistance;
        if (!denominator) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !denominator"); /* A zero-divisor IRQ retries forever, not a valid crossing. */
        int64_t quotient=(int64_t)(int32_t)product/denominator;
        while (quotient<INT16_MIN || quotient>INT16_MAX)
        {
            product=BtechDivideErrorInterrupt(product);
            quotient=(int64_t)(int32_t)product/denominator;
        }
        uint32_t displacement=(int16_t)quotient;
        projected[axis]->wide[outsideVertex]=coordinates[axis]->wide[outsideVertex]+displacement;
    }
}

/*22B6:6CD4. Previous outside -> crossing then current; current outside
 * -> crossing in current's slot. Preserve original output and partial writes. */
void BtechClipWidePolygonNearPlane(void)
{
    if (!polygonVertexCount || polygonVertexCount>POLYGON_VERTEX_INDEX_BYTES) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !polygonVertexCount || polygonVertexCount>POLYGON_VERTEX_INDEX_BYTES"); /* Native empty-list read aliases4616. */
    uint16_t previous=polygonVertexIndices[polygonVertexCount-1],output=0,input=0;
    do
    {
        uint16_t current=polygonVertexIndices[input];
        if (current>=GEOMETRY_CACHE_VERTICES || previous>=GEOMETRY_CACHE_VERTICES) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: current>=GEOMETRY_CACHE_VERTICES || previous>=GEOMETRY_CACHE_VERTICES");
        uint16_t emit[2],emitted=0;
        if (vertexNearPlaneState[current]!=GEOMETRY_VERTEX_BEHIND)
        {
            if (vertexNearPlaneState[previous]==GEOMETRY_VERTEX_BEHIND)
            {
                BtechIntersectWideEdgeWithNearPlane(previous,current);
                emit[emitted++]=previous;
            }
            emit[emitted++]=current;
        }
        else if (vertexNearPlaneState[previous]!=GEOMETRY_VERTEX_BEHIND)
        {
            BtechIntersectWideEdgeWithNearPlane(current,previous);
            emit[emitted++]=current;
        }
        for (uint16_t vertex=0;vertex<emitted;++vertex)
        {
            if (output>=WIDE_POLYGON_VERTICES) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: output>=WIDE_POLYGON_VERTICES");
            wideScreenX[output]=(uint32_t)screenCentreA+geometryProjectedX.wide[emit[vertex]];
            wideScreenY[output]=(uint32_t)screenCentreB-geometryProjectedY.wide[emit[vertex]];
            ++output;
        }
        previous=current;++input;
    } while ((int16_t)input<(int16_t)polygonVertexCount);
    polygonVertexCount=output;
}

/*22B6:72D3. Previous outside -> crossing then current; current outside
 * -> crossing in current's slot. Preserve original output and partial writes. */
void BtechClipDwordNearPolygonWithWordProducts(void)
{
    if (!polygonVertexCount || polygonVertexCount>POLYGON_VERTEX_INDEX_BYTES) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !polygonVertexCount || polygonVertexCount>POLYGON_VERTEX_INDEX_BYTES"); /* Native empty-list read aliases4616. */
    uint16_t previous=polygonVertexIndices[polygonVertexCount-1],output=0,input=0;
    do
    {
        uint16_t current=polygonVertexIndices[input];
        if (current>=GEOMETRY_CACHE_VERTICES || previous>=GEOMETRY_CACHE_VERTICES) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: current>=GEOMETRY_CACHE_VERTICES || previous>=GEOMETRY_CACHE_VERTICES");
        uint16_t emit[2],emitted=0;
        if (vertexNearPlaneState[current]!=GEOMETRY_VERTEX_BEHIND)
        {
            if (vertexNearPlaneState[previous]==GEOMETRY_VERTEX_BEHIND)
            {
                BtechIntersectDwordNearEdgeWithWordProducts(previous,current);
                emit[emitted++]=previous;
            }
            emit[emitted++]=current;
        }
        else if (vertexNearPlaneState[previous]!=GEOMETRY_VERTEX_BEHIND)
        {
            BtechIntersectDwordNearEdgeWithWordProducts(current,previous);
            emit[emitted++]=current;
        }
        for (uint16_t vertex=0;vertex<emitted;++vertex)
        {
            if (output>=WIDE_POLYGON_VERTICES) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: output>=WIDE_POLYGON_VERTICES");
            wideScreenX[output]=(uint32_t)screenCentreA+geometryProjectedX.wide[emit[vertex]];
            wideScreenY[output]=(uint32_t)screenCentreB-geometryProjectedY.wide[emit[vertex]];
            ++output;
        }
        previous=current;++input;
    } while ((int16_t)input<(int16_t)polygonVertexCount);
    polygonVertexCount=output;
}

/*22B6:6E20. Native DI advances one DWORD only when a plane is emitted.
 * The first matching flag wins; store the plane BEFORE multiplication/division. */
uint16_t BtechIntersectWideScreenEdgeA(uint16_t base,uint16_t next,uint16_t destination,uint8_t flags)
{
    if (!(flags&(POLYGON_LEFT|POLYGON_RIGHT))) return destination;
    if (base>=WIDE_POLYGON_VERTICES || next>=WIDE_POLYGON_VERTICES || destination>=WIDE_POLYGON_VERTICES) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: base>=WIDE_POLYGON_VERTICES || next>=WIDE_POLYGON_VERTICES || destination>=WIDE_POLYGON_VERTICES");
    uint32_t denominator=(wideScreenX[next]-wideScreenX[base]);
    uint32_t boundary=(uint32_t)((flags&POLYGON_LEFT)?clipMinimumX:clipMaximumX);
    widePolygonWorkX[destination]=boundary;
    uint32_t planeDistance=boundary-wideScreenX[base];
    uint32_t difference=wideScreenY[next]-wideScreenY[base];
    uint64_t product=BtechMultiplySignedDwordsWide(difference,planeDistance);
    uint32_t displacement=BtechDivideSignedWideNumerator((int16_t)(product>>48),(uint16_t)(product>>32),(uint16_t)(product>>16),(uint16_t)product,(uint16_t)denominator,(int16_t)(denominator>>16),NULL);
    widePolygonWorkY[destination]=wideScreenY[base]+displacement;
    return destination+1;
}

/*22B6:6F0B. Native DI advances one DWORD only when a plane is emitted.
 * The first matching flag wins; store the plane BEFORE multiplication/division. */
uint16_t BtechIntersectWideScreenEdgeB(uint16_t base,uint16_t next,uint16_t destination,uint8_t flags)
{
    if (!(flags&(POLYGON_TOP|POLYGON_BOTTOM))) return destination;
    if (base>=WIDE_POLYGON_VERTICES || next>=WIDE_POLYGON_VERTICES || destination>=WIDE_POLYGON_VERTICES) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: base>=WIDE_POLYGON_VERTICES || next>=WIDE_POLYGON_VERTICES || destination>=WIDE_POLYGON_VERTICES");
    uint32_t denominator=(widePolygonWorkY[next]-widePolygonWorkY[base]);
    uint32_t boundary=(uint32_t)((flags&POLYGON_TOP)?clipMinimumY:clipMaximumY);
    wideScreenY[destination]=boundary;
    uint32_t planeDistance=boundary-widePolygonWorkY[base];
    uint32_t difference=widePolygonWorkX[next]-widePolygonWorkX[base];
    uint64_t product=BtechMultiplySignedDwordsWide(difference,planeDistance);
    uint32_t displacement=BtechDivideSignedWideNumerator((int16_t)(product>>48),(uint16_t)(product>>32),(uint16_t)(product>>16),(uint16_t)product,(uint16_t)denominator,(int16_t)(denominator>>16),NULL);
    wideScreenX[destination]=widePolygonWorkX[base]+displacement;
    return destination+1;
}

/*22B6:741F. Native DI advances one DWORD only when a plane is emitted.
 * The first matching flag wins; store the plane BEFORE multiplication/division. */
uint16_t BtechIntersectScreenEdgeWordProductA(uint16_t base,uint16_t next,uint16_t destination,uint8_t flags)
{
    if (!(flags&(POLYGON_LEFT|POLYGON_RIGHT))) return destination;
    if (base>=WIDE_POLYGON_VERTICES || next>=WIDE_POLYGON_VERTICES || destination>=WIDE_POLYGON_VERTICES) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: base>=WIDE_POLYGON_VERTICES || next>=WIDE_POLYGON_VERTICES || destination>=WIDE_POLYGON_VERTICES");
    int16_t denominator=wideScreenX[next]-wideScreenX[base];
    uint32_t boundary=(uint32_t)((flags&POLYGON_LEFT)?clipMinimumX:clipMaximumX);
    widePolygonWorkX[destination]=boundary;
    uint32_t planeDistance=boundary-wideScreenX[base];
    uint32_t difference=wideScreenY[next]-wideScreenY[base];
    uint32_t product=(int32_t)(int16_t)difference*(int16_t)planeDistance;
    if (!denominator) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !denominator");
    int64_t quotient=(int64_t)(int32_t)product/denominator;
    while (quotient<INT16_MIN || quotient>INT16_MAX)
    {
        product=BtechDivideErrorInterrupt(product);
        quotient=(int64_t)(int32_t)product/denominator;
    }
    uint32_t displacement=(int16_t)quotient;
    widePolygonWorkY[destination]=wideScreenY[base]+displacement;
    return destination+1;
}

/*22B6:74F0. Native DI advances one DWORD only when a plane is emitted.
 * The first matching flag wins; store the plane BEFORE multiplication/division. */
uint16_t BtechIntersectScreenEdgeWordProductB(uint16_t base,uint16_t next,uint16_t destination,uint8_t flags)
{
    if (!(flags&(POLYGON_TOP|POLYGON_BOTTOM))) return destination;
    if (base>=WIDE_POLYGON_VERTICES || next>=WIDE_POLYGON_VERTICES || destination>=WIDE_POLYGON_VERTICES) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: base>=WIDE_POLYGON_VERTICES || next>=WIDE_POLYGON_VERTICES || destination>=WIDE_POLYGON_VERTICES");
    int16_t denominator=widePolygonWorkY[next]-widePolygonWorkY[base];
    uint32_t boundary=(uint32_t)((flags&POLYGON_TOP)?clipMinimumY:clipMaximumY);
    wideScreenY[destination]=boundary;
    uint32_t planeDistance=boundary-widePolygonWorkY[base];
    uint32_t difference=widePolygonWorkX[next]-widePolygonWorkX[base];
    uint32_t product=(int32_t)(int16_t)difference*(int16_t)planeDistance;
    if (!denominator) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !denominator");
    int64_t quotient=(int64_t)(int32_t)product/denominator;
    while (quotient<INT16_MIN || quotient>INT16_MAX)
    {
        product=BtechDivideErrorInterrupt(product);
        quotient=(int64_t)(int32_t)product/denominator;
    }
    uint32_t displacement=(int16_t)quotient;
    wideScreenX[destination]=widePolygonWorkX[base]+displacement;
    return destination+1;
}

/*22B6:6FF6. Paired X boundaries followed by paired Y boundaries, NOT
 * four independent clipping passes. Live WORD count and endpoint order retained. */
void BtechClipWidePolygonToScreen(void)
{
    if ((int16_t)polygonVertexCount<=1) return;
    if (polygonVertexCount>WIDE_POLYGON_VERTICES) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: polygonVertexCount>WIDE_POLYGON_VERTICES");
    {
        uint16_t previous=polygonVertexCount-1,current=0,output=0;
        uint32_t coordinate=wideScreenX[previous];
        int16_t upper=coordinate>>16;
        uint8_t previousFlags=0;
        if (upper<0 || (!upper && (uint16_t)coordinate<(uint16_t)clipMinimumX)) previousFlags|=POLYGON_LEFT;
        if (upper>0 || (!upper && (uint16_t)coordinate>(uint16_t)clipMaximumX)) previousFlags|=POLYGON_RIGHT;
        do
        {
            coordinate=wideScreenX[current];upper=(int16_t)(coordinate>>16);
            uint8_t currentFlags=0;
            if (upper<0 || (!upper && (uint16_t)coordinate<(uint16_t)clipMinimumX)) currentFlags|=POLYGON_LEFT;
            if (upper>0 || (!upper && (uint16_t)coordinate>(uint16_t)clipMaximumX)) currentFlags|=POLYGON_RIGHT;
            if (!(previousFlags|currentFlags))
            {
                if (output>=WIDE_POLYGON_VERTICES) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: output>=WIDE_POLYGON_VERTICES");
                widePolygonWorkX[output]=wideScreenX[current];widePolygonWorkY[output]=wideScreenY[current];++output;
            }
            else if (!(previousFlags&currentFlags))
            {
                if (!previousFlags) output=BtechIntersectWideScreenEdgeA(current,previous,output,currentFlags);
                else if (!currentFlags)
                {
                    output=BtechIntersectWideScreenEdgeA(previous,current,output,previousFlags);
                    if (output>=WIDE_POLYGON_VERTICES) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: output>=WIDE_POLYGON_VERTICES");
                    widePolygonWorkX[output]=wideScreenX[current];widePolygonWorkY[output]=wideScreenY[current];++output;
                }
                else
                {
                    output=BtechIntersectWideScreenEdgeA(previous,current,output,previousFlags);
                    output=BtechIntersectWideScreenEdgeA(current,previous,output,currentFlags);
                }
            }
            previous=current;previousFlags=currentFlags;++current;
        } while (current!=polygonVertexCount);
        polygonVertexCount=output;
    }
    if ((int16_t)polygonVertexCount<=1)
    {
        BtechCopyRendererWorkingCoordinateArrays();
        return;
    }
    {
        uint16_t previous=polygonVertexCount-1,current=0,output=0;
        uint32_t coordinate=widePolygonWorkY[previous];
        int16_t upper=coordinate>>16;
        uint8_t previousFlags=0;
        if (upper<0 || (!upper && (uint16_t)coordinate<(uint16_t)clipMinimumY)) previousFlags|=POLYGON_TOP;
        if (upper>0 || (!upper && (uint16_t)coordinate>(uint16_t)clipMaximumY)) previousFlags|=POLYGON_BOTTOM;
        do
        {
            coordinate=widePolygonWorkY[current];upper=(int16_t)(coordinate>>16);
            uint8_t currentFlags=0;
            if (upper<0 || (!upper && (uint16_t)coordinate<(uint16_t)clipMinimumY)) currentFlags|=POLYGON_TOP;
            if (upper>0 || (!upper && (uint16_t)coordinate>(uint16_t)clipMaximumY)) currentFlags|=POLYGON_BOTTOM;
            if (!(previousFlags|currentFlags))
            {
                if (output>=WIDE_POLYGON_VERTICES) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: output>=WIDE_POLYGON_VERTICES");
                wideScreenX[output]=widePolygonWorkX[current];wideScreenY[output]=widePolygonWorkY[current];++output;
            }
            else if (!(previousFlags&currentFlags))
            {
                if (!previousFlags) output=BtechIntersectWideScreenEdgeB(current,previous,output,currentFlags);
                else if (!currentFlags)
                {
                    output=BtechIntersectWideScreenEdgeB(previous,current,output,previousFlags);
                    if (output>=WIDE_POLYGON_VERTICES) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: output>=WIDE_POLYGON_VERTICES");
                    wideScreenX[output]=widePolygonWorkX[current];wideScreenY[output]=widePolygonWorkY[current];++output;
                }
                else
                {
                    output=BtechIntersectWideScreenEdgeB(previous,current,output,previousFlags);
                    output=BtechIntersectWideScreenEdgeB(current,previous,output,currentFlags);
                }
            }
            previous=current;previousFlags=currentFlags;++current;
        } while (current!=polygonVertexCount);
        polygonVertexCount=output;
    }
}

/*22B6:75C1. Paired X boundaries followed by paired Y boundaries, NOT
 * four independent clipping passes. Live WORD count and endpoint order retained. */
void BtechClipPolygonWithWordProducts(void)
{
    if ((int16_t)polygonVertexCount<=1) return;
    if (polygonVertexCount>WIDE_POLYGON_VERTICES) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: polygonVertexCount>WIDE_POLYGON_VERTICES");
    {
        uint16_t previous=polygonVertexCount-1,current=0,output=0;
        uint32_t coordinate=wideScreenX[previous];
        int16_t upper=coordinate>>16;
        uint8_t previousFlags=0;
        if (upper<0 || (!upper && (uint16_t)coordinate<(uint16_t)clipMinimumX)) previousFlags|=POLYGON_LEFT;
        if (upper>0 || (!upper && (uint16_t)coordinate>(uint16_t)clipMaximumX)) previousFlags|=POLYGON_RIGHT;
        do
        {
            coordinate=wideScreenX[current];upper=(int16_t)(coordinate>>16);
            uint8_t currentFlags=0;
            if (upper<0 || (!upper && (uint16_t)coordinate<(uint16_t)clipMinimumX)) currentFlags|=POLYGON_LEFT;
            if (upper>0 || (!upper && (uint16_t)coordinate>(uint16_t)clipMaximumX)) currentFlags|=POLYGON_RIGHT;
            if (!(previousFlags|currentFlags))
            {
                if (output>=WIDE_POLYGON_VERTICES) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: output>=WIDE_POLYGON_VERTICES");
                widePolygonWorkX[output]=wideScreenX[current];widePolygonWorkY[output]=wideScreenY[current];++output;
            }
            else if (!(previousFlags&currentFlags))
            {
                if (!previousFlags) output=BtechIntersectScreenEdgeWordProductA(current,previous,output,currentFlags);
                else if (!currentFlags)
                {
                    output=BtechIntersectScreenEdgeWordProductA(previous,current,output,previousFlags);
                    if (output>=WIDE_POLYGON_VERTICES) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: output>=WIDE_POLYGON_VERTICES");
                    widePolygonWorkX[output]=wideScreenX[current];widePolygonWorkY[output]=wideScreenY[current];++output;
                }
                else
                {
                    output=BtechIntersectScreenEdgeWordProductA(previous,current,output,previousFlags);
                    output=BtechIntersectScreenEdgeWordProductA(current,previous,output,currentFlags);
                }
            }
            previous=current;previousFlags=currentFlags;++current;
        } while (current!=polygonVertexCount);
        polygonVertexCount=output;
    }
    if ((int16_t)polygonVertexCount<=1)
    {
        BtechCopyRendererWorkingCoordinateArrays();
        return;
    }
    {
        uint16_t previous=polygonVertexCount-1,current=0,output=0;
        uint32_t coordinate=widePolygonWorkY[previous];
        int16_t upper=coordinate>>16;
        uint8_t previousFlags=0;
        if (upper<0 || (!upper && (uint16_t)coordinate<(uint16_t)clipMinimumY)) previousFlags|=POLYGON_TOP;
        if (upper>0 || (!upper && (uint16_t)coordinate>(uint16_t)clipMaximumY)) previousFlags|=POLYGON_BOTTOM;
        do
        {
            coordinate=widePolygonWorkY[current];upper=(int16_t)(coordinate>>16);
            uint8_t currentFlags=0;
            if (upper<0 || (!upper && (uint16_t)coordinate<(uint16_t)clipMinimumY)) currentFlags|=POLYGON_TOP;
            if (upper>0 || (!upper && (uint16_t)coordinate>(uint16_t)clipMaximumY)) currentFlags|=POLYGON_BOTTOM;
            if (!(previousFlags|currentFlags))
            {
                if (output>=WIDE_POLYGON_VERTICES) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: output>=WIDE_POLYGON_VERTICES");
                wideScreenX[output]=widePolygonWorkX[current];wideScreenY[output]=widePolygonWorkY[current];++output;
            }
            else if (!(previousFlags&currentFlags))
            {
                if (!previousFlags) output=BtechIntersectScreenEdgeWordProductB(current,previous,output,currentFlags);
                else if (!currentFlags)
                {
                    output=BtechIntersectScreenEdgeWordProductB(previous,current,output,previousFlags);
                    if (output>=WIDE_POLYGON_VERTICES) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: output>=WIDE_POLYGON_VERTICES");
                    wideScreenX[output]=widePolygonWorkX[current];wideScreenY[output]=widePolygonWorkY[current];++output;
                }
                else
                {
                    output=BtechIntersectScreenEdgeWordProductB(previous,current,output,previousFlags);
                    output=BtechIntersectScreenEdgeWordProductB(current,previous,output,currentFlags);
                }
            }
            previous=current;previousFlags=currentFlags;++current;
        } while (current!=polygonVertexCount);
        polygonVertexCount=output;
    }
}
