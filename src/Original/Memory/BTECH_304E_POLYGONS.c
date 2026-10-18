#include "mechwarrior/combat_polygons.h"
#include "mechwarrior/native_address.h"

int16_t screenPolygonX[COMBAT_POLYGON_VERTEX_WORDS],screenPolygonY[COMBAT_POLYGON_VERTEX_WORDS];
int16_t polygonWorkX[COMBAT_POLYGON_VERTEX_WORDS],polygonWorkY[COMBAT_POLYGON_VERTEX_WORDS];
int16_t polygonOutlineX[COMBAT_POLYGON_VERTEX_WORDS],polygonOutlineY[COMBAT_POLYGON_VERTEX_WORDS];
uint16_t polygonVertexCount;
BtechPolygonSpan polygonSpanStorage[COMBAT_SCREEN_HEIGHT+1];
BtechPolygonSpan *polygonScanlineHeader;
uint16_t polygonMinimumYVertexByteOffset,polygonMaximumYVertexByteOffset;
uint16_t polygonFirstChainVertexCount,polygonSecondChainVertexCount;
uint16_t polygonChainEdgesRemaining,polygonChainVertexByteOffset,polygonSpanBoundaryByteOffset;
uint16_t polygonPreviousVertexX,polygonPreviousVertexY,polygonOutlineVertexCount;
uint8_t egaPolygonLeadingMasks[8],egaPolygonTrailingMasks[8],egaSpanHighColour,egaSpanLowColour;
uint16_t egaSpanRowsRemaining;
void (*graphicsFillPolygonSpans)(const BtechPolygonSpan *);
void (*graphicsDrawLine)(uint16_t,uint16_t,uint16_t,uint16_t,uint16_t);

/* Executable IO projection only. Runtime uses these actual named owners. */
bool BtechImportPolygonInitialData(const uint8_t *data,size_t bytes)
{
    if (!data || bytes<0x10000 || MwReadNativeWord(data,0x3FE2)) return false;
    polygonScanlineHeader=NULL;graphicsFillPolygonSpans=NULL;
    graphicsDrawLine=NULL;
    int16_t *coordinates[]={screenPolygonX,screenPolygonY,polygonWorkX,polygonWorkY,polygonOutlineX,polygonOutlineY};
    for (uint16_t axis=0;axis<6;++axis)
        for (uint16_t i=0;i<COMBAT_POLYGON_VERTEX_WORDS;++i) coordinates[axis][i]=(int16_t)MwReadNativeWord(data,0x2EFC+axis*0x28+i*2);
    polygonVertexCount=MwReadNativeWord(data,0x2FEC);
    for (uint16_t i=0;i<=COMBAT_SCREEN_HEIGHT;++i)
    {polygonSpanStorage[i].endpoints.left=MwReadNativeWord(data,0x2FFC+i*4);polygonSpanStorage[i].endpoints.right=MwReadNativeWord(data,0x2FFE + i*4);}
    polygonMinimumYVertexByteOffset=MwReadNativeWord(data,0x3FD0);polygonMaximumYVertexByteOffset=MwReadNativeWord(data,0x3FD2);
    polygonFirstChainVertexCount=MwReadNativeWord(data,0x3FD4);polygonSecondChainVertexCount=MwReadNativeWord(data,0x3FD6);
    polygonChainEdgesRemaining=MwReadNativeWord(data,0x3FD8);polygonChainVertexByteOffset=MwReadNativeWord(data,0x3FDA);
    polygonSpanBoundaryByteOffset=MwReadNativeWord(data,0x3FDC);polygonPreviousVertexX=MwReadNativeWord(data,0x3FDE);
    polygonPreviousVertexY=MwReadNativeWord(data,0x3FE0);polygonOutlineVertexCount=MwReadNativeWord(data,0x3FE4);
    for (uint16_t i=0;i<8;++i) {egaPolygonLeadingMasks[i]=data[0x4494+i];egaPolygonTrailingMasks[i]=data[0x449C+i];}
    egaSpanHighColour=data[0x44A4];egaSpanLowColour=data[0x44A5];egaSpanRowsRemaining=MwReadNativeWord(data,0x44A6);
    return true;
}
