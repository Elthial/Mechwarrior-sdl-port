#include "mechwarrior/port_diagnostics.h"
#include "mechwarrior/combat_polygons.h"
#include <stdlib.h>

/*1BF5:0475. The native copies, winding decision, two edge chains and scanline
 * header remain in this procedure. No host polygon rasterizer is substituted.
 * Typed references transport the unobserved near addresses; buffers retain
 * their original20-WORD unaliased extents and the preceding-row header alias. */
BtechPolygonSpan *BtechDrawScreenPolygon(int16_t count,const int16_t *x,const int16_t *y)
{
    polygonScanlineHeader=NULL;
    if (count>=0)
    {
        polygonVertexCount=(uint16_t)count;
        if (count>COMBAT_POLYGON_VERTEX_WORDS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: count>COMBAT_POLYGON_VERTEX_WORDS");
        for (uint16_t axis=0;axis<2;++axis)
        {
            const int16_t *source=axis?y:x;int16_t *destination=axis?screenPolygonY:screenPolygonX;
            for (uint16_t i=0;i<(uint16_t)count;++i) destination[i]=source[i];
        }
    }
    if (count<2) goto returnPolygon;
    if (count==2) goto drawSingleSegment;
    if (!drawPolygonFilled)
    {
        uint16_t vertices=polygonVertexCount;
        if (vertices>=COMBAT_POLYGON_VERTEX_WORDS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: vertices>=COMBAT_POLYGON_VERTEX_WORDS");
        screenPolygonX[vertices]=screenPolygonX[0];screenPolygonY[vertices]=screenPolygonY[0];
        BtechDrawScreenPolyline(vertices,screenPolygonX,screenPolygonY);
        goto returnPolygon;
    }
    if (drawFillColour!=drawLineColour)
    {
        polygonOutlineVertexCount=polygonVertexCount;
        if (polygonOutlineVertexCount>=COMBAT_POLYGON_VERTEX_WORDS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: polygonOutlineVertexCount>=COMBAT_POLYGON_VERTEX_WORDS");
        for (uint16_t axis=0;axis<2;++axis)
        {
            int16_t *source=axis?screenPolygonY:screenPolygonX;
            int16_t *destination=axis?polygonOutlineY:polygonOutlineX;
            int16_t first=source[0];destination[0]=first;
            for (uint16_t i=1;i<polygonOutlineVertexCount;++i) destination[i]=source[i];
            destination[polygonOutlineVertexCount]=first;
        }
    }
    if (screenPolygonNeedsClip) BtechClipScreenPolygonToViewport();
    if (polygonVertexCount==2) goto drawSingleSegment;
    if ((int16_t)polygonVertexCount<2) goto returnPolygon;
    polygonPreviousVertexY=(uint16_t)screenPolygonY[0];polygonPreviousVertexX=(uint16_t)screenPolygonX[0];
    polygonMinimumYVertexByteOffset=0;polygonMaximumYVertexByteOffset=0;
    uint16_t source=(uint16_t)(polygonVertexCount-1),accepted=0;
    uint16_t minimumY=0x7FFF,maximumY=0x8001,minimumXAtMaximumY=0x7FFF,maximumXAtMinimumY=0x8001;
    for (;;)
    {
        uint16_t vertexY=screenPolygonY[source];
        if (vertexY!=polygonPreviousVertexY || (uint16_t)screenPolygonX[source]!=polygonPreviousVertexX)
        {
            polygonPreviousVertexY=vertexY;polygonWorkY[accepted]=(int16_t)vertexY;
            if ((int16_t)vertexY<(int16_t)minimumY || (vertexY==minimumY && screenPolygonX[source]>(int16_t)maximumXAtMinimumY))
            {polygonMinimumYVertexByteOffset=accepted*2;minimumY=vertexY;maximumXAtMinimumY=(uint16_t)screenPolygonX[source];}
            if ((int16_t)vertexY>(int16_t)maximumY || (vertexY==maximumY && screenPolygonX[source]<=(int16_t)minimumXAtMaximumY))
            {polygonMaximumYVertexByteOffset=accepted*2;maximumY=vertexY;minimumXAtMaximumY=(uint16_t)screenPolygonX[source];}
            uint16_t vertexX=screenPolygonX[source];polygonPreviousVertexX=vertexX;
            polygonWorkX[accepted++]=(int16_t)vertexX;
        }
        if (!source) break;
        --source;
    }
    if (minimumY==maximumY || accepted==2)
    {BtechDrawClippedLine((int16_t)minimumXAtMaximumY,(int16_t)maximumY,(int16_t)maximumXAtMinimumY,(int16_t)minimumY);goto returnPolygon;}
    if (accepted<2) goto returnPolygon;
    polygonVertexCount=accepted;
    uint16_t minimum=polygonMinimumYVertexByteOffset/2,next=minimum+1;
    if (next>=accepted) next=0;
    uint16_t nextNumerator=polygonWorkX[next]-polygonWorkX[minimum];
    uint16_t nextDenominator=polygonWorkY[next]-polygonWorkY[minimum];
    if (!nextDenominator) {nextDenominator=1;nextNumerator=(int16_t)nextNumerator<0?0x8001:0x7FFF;}
    uint16_t previous=minimum?minimum-1:accepted-1;
    uint16_t previousNumerator=polygonWorkX[previous]-polygonWorkX[minimum];
    uint16_t previousDenominator=polygonWorkY[previous]-polygonWorkY[minimum];
    if (!previousDenominator) {previousDenominator=1;previousNumerator=(int16_t)previousNumerator<0?0x8001:0x7FFF;}
    bool reverseWinding;
    if ((int16_t)previousNumerator>=0 && (int16_t)nextNumerator<=0) reverseWinding=true;
    else if ((int16_t)previousNumerator<0 && (int16_t)nextNumerator>=0) reverseWinding=false;
    else
    {
        if ((int16_t)previousNumerator<0)
        {
            uint16_t saved=0-nextNumerator;nextNumerator=(uint16_t)(0-previousNumerator);previousNumerator=saved;
            saved=nextDenominator;nextDenominator=previousDenominator;previousDenominator=saved;
        }
        uint16_t previousQuotient=previousNumerator/previousDenominator,previousRemainder=previousNumerator%previousDenominator;
        uint16_t nextQuotient=nextNumerator/nextDenominator,nextRemainder=nextNumerator%nextDenominator;
        reverseWinding=nextQuotient<previousQuotient;
        if (nextQuotient==previousQuotient)
        {
            uint16_t nextFraction=((uint32_t)nextRemainder<<16)/nextDenominator;
            uint16_t previousFraction=((uint32_t)previousRemainder<<16)/previousDenominator;
            reverseWinding=nextFraction<previousFraction;
        }
    }
    for (uint16_t axis=0;axis<2;++axis)
    {
        int16_t *work=axis?polygonWorkY:polygonWorkX,*screen=axis?screenPolygonY:screenPolygonX;
        for (uint16_t i=0;i<accepted;++i) screen[reverseWinding?accepted-1-i:i]=work[i];
    }
    if (reverseWinding)
    {
        uint16_t lastByteOffset=(accepted-1)*2;
        polygonMinimumYVertexByteOffset=lastByteOffset-polygonMinimumYVertexByteOffset;
        polygonMaximumYVertexByteOffset=lastByteOffset-polygonMaximumYVertexByteOffset;
    }
    maximumY=(uint16_t)screenPolygonY[polygonMaximumYVertexByteOffset/2];
    source=polygonMinimumYVertexByteOffset/2;
    uint16_t chainDestination=0;
    for (uint16_t chain=0;chain<2;++chain)
    {
        if (chain)
        {minimumY=(uint16_t)screenPolygonY[polygonMinimumYVertexByteOffset/2];source=polygonMaximumYVertexByteOffset/2;}
        for (;;)
        {
            if (chainDestination>=COMBAT_POLYGON_VERTEX_WORDS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: chainDestination>=COMBAT_POLYGON_VERTEX_WORDS");
            polygonWorkX[chainDestination]=screenPolygonX[source];
            uint16_t vertexY=screenPolygonY[source];polygonWorkY[chainDestination++]=(int16_t)vertexY;
            if (chain?(int16_t)vertexY<=(int16_t)minimumY:(int16_t)vertexY>=(int16_t)maximumY) break;
            if (++source>=accepted) source=0;
        }
        if (!chain) polygonFirstChainVertexCount=chainDestination;
        else polygonSecondChainVertexCount=chainDestination-polygonFirstChainVertexCount;
    }
    polygonSpanBoundaryByteOffset=2;polygonChainVertexByteOffset=0;
    uint16_t chainVertices=polygonFirstChainVertexCount;
    for (;;)
    {
        if (--chainVertices==0)
        {
            if (!polygonSpanBoundaryByteOffset) break;
            polygonChainVertexByteOffset+=2;polygonSpanBoundaryByteOffset=0;
            chainVertices=polygonSecondChainVertexCount;continue;
        }
        polygonChainEdgesRemaining=chainVertices;
        uint16_t chainOffset=polygonChainVertexByteOffset/2;polygonChainVertexByteOffset+=2;
        if (chainOffset+1>=COMBAT_POLYGON_VERTEX_WORDS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: chainOffset+1>=COMBAT_POLYGON_VERTEX_WORDS");
        int16_t firstX=polygonWorkX[chainOffset],secondX=polygonWorkX[chainOffset+1];
        int16_t firstY=polygonWorkY[chainOffset],secondY=polygonWorkY[chainOffset+1];
        int16_t deltaX=(int16_t)(firstX-secondX),deltaY=(int16_t)(firstY-secondY);
        uint16_t magnitudeX=deltaX<0?(uint16_t)(0-(uint16_t)deltaX):(uint16_t)deltaX;
        uint16_t magnitudeY=deltaY<0?(uint16_t)(0-(uint16_t)deltaY):(uint16_t)deltaY;
        if (!magnitudeX) BtechAccumulateVerticalPolygonEdge(firstY,secondX,secondY);
        else if (!magnitudeY)
        {
            if (firstX>=secondX) {int16_t saved=firstX;firstX=secondX;secondX=saved;}
            if ((uint16_t)firstY>=COMBAT_SCREEN_HEIGHT) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: (uint16_t)firstY>=COMBAT_SCREEN_HEIGHT");
            if (polygonSpanBoundaryByteOffset) polygonScanlineSpans[(uint16_t)firstY].endpoints.right=(uint16_t)secondX;
            else polygonScanlineSpans[(uint16_t)firstY].endpoints.left=(uint16_t)firstX;
        }
        else if ((int16_t)magnitudeX<(int16_t)magnitudeY) BtechAccumulateYMajorPolygonEdge(firstY,firstX,secondX,secondY);
        else if (magnitudeX==magnitudeY) BtechAccumulateDiagonalPolygonEdge(firstY,firstX,secondX,secondY);
        else if (!polygonSpanBoundaryByteOffset) BtechAccumulateAscendingXMajorPolygonEdge(firstY,firstX,secondX,secondY);
        else BtechAccumulateDescendingXMajorPolygonEdge(firstY,firstX,secondX,secondY);
        chainVertices=polygonChainEdgesRemaining;
    }
    minimumY=(uint16_t)screenPolygonY[polygonMinimumYVertexByteOffset/2];
    if (minimumY>=COMBAT_SCREEN_HEIGHT) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: minimumY>=COMBAT_SCREEN_HEIGHT");
    polygonScanlineHeader=&polygonSpanStorage[minimumY];
    polygonScanlineHeader->header.firstRow=minimumY;
    maximumY=(uint16_t)screenPolygonY[polygonMaximumYVertexByteOffset/2];
    polygonScanlineHeader->header.rowCount=(uint16_t)(maximumY-minimumY+1);
    if (!graphicsFillPolygonSpans) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !graphicsFillPolygonSpans");
    graphicsFillPolygonSpans(polygonScanlineHeader);
    if (drawFillColour!=drawLineColour) BtechDrawScreenPolyline(polygonOutlineVertexCount,polygonOutlineX,polygonOutlineY);
    goto returnPolygon;
drawSingleSegment:
    BtechDrawScreenPolyline(1,screenPolygonX,screenPolygonY);
returnPolygon:
    return polygonScanlineHeader;
}
