#ifndef MECHWARRIOR_COMBAT_POLYGONS_H
#define MECHWARRIOR_COMBAT_POLYGONS_H
#include "mechwarrior/combat_video.h"

enum { COMBAT_POLYGON_VERTEX_WORDS=20 };
typedef union BtechPolygonSpan {
    struct {uint16_t left,right;} endpoints;
    struct {uint16_t firstRow,rowCount;} header;
} BtechPolygonSpan;
_Static_assert(sizeof(BtechPolygonSpan)==4,"Native row/header aliases two WORDs");
extern int16_t screenPolygonX[COMBAT_POLYGON_VERTEX_WORDS],screenPolygonY[COMBAT_POLYGON_VERTEX_WORDS];
extern int16_t polygonWorkX[COMBAT_POLYGON_VERTEX_WORDS],polygonWorkY[COMBAT_POLYGON_VERTEX_WORDS];
extern int16_t polygonOutlineX[COMBAT_POLYGON_VERTEX_WORDS],polygonOutlineY[COMBAT_POLYGON_VERTEX_WORDS];
extern uint16_t polygonVertexCount;
/* Header aliases the preceding row. Element zero is native2FFC, allowing a
 * header for firstRow0 without a separate live mirror or address registry. */
extern BtechPolygonSpan polygonSpanStorage[COMBAT_SCREEN_HEIGHT+1];
#define polygonScanlineSpans (polygonSpanStorage+1)
extern BtechPolygonSpan *polygonScanlineHeader;
extern uint16_t polygonMinimumYVertexByteOffset,polygonMaximumYVertexByteOffset;
extern uint16_t polygonFirstChainVertexCount,polygonSecondChainVertexCount;
extern uint16_t polygonChainEdgesRemaining,polygonChainVertexByteOffset,polygonSpanBoundaryByteOffset;
extern uint16_t polygonPreviousVertexX,polygonPreviousVertexY,polygonOutlineVertexCount;
extern uint8_t egaPolygonLeadingMasks[8],egaPolygonTrailingMasks[8],egaSpanHighColour,egaSpanLowColour;
extern uint16_t egaSpanRowsRemaining;
extern void (*graphicsFillPolygonSpans)(const BtechPolygonSpan *header);
bool BtechImportPolygonInitialData(const uint8_t *data,size_t bytes);
BtechPolygonSpan *BtechDrawScreenPolygon(int16_t count,const int16_t *x,const int16_t *y);
void BtechClipScreenPolygonToViewport(void);
void BtechAccumulateVerticalPolygonEdge(int16_t firstY,int16_t fixedX,int16_t secondY);
void BtechAccumulateYMajorPolygonEdge(int16_t firstY,int16_t firstX,int16_t secondX,int16_t secondY);
void BtechAccumulateDiagonalPolygonEdge(int16_t firstY,int16_t firstX,int16_t secondX,int16_t secondY);
void BtechAccumulateDescendingXMajorPolygonEdge(int16_t firstY,int16_t firstX,int16_t secondX,int16_t secondY);
void BtechAccumulateAscendingXMajorPolygonEdge(int16_t firstY,int16_t firstX,int16_t secondX,int16_t secondY);
void BtechEgaFillPolygonSpans(const BtechPolygonSpan *header);
extern void (*graphicsDrawLine)(uint16_t x0,uint16_t y0,uint16_t x1,uint16_t y1,uint16_t page);
void BtechDrawClippedLine(int16_t firstX,int16_t firstY,int16_t secondX,int16_t secondY);
void BtechDrawScreenPolyline(uint16_t segments,const int16_t *x,const int16_t *y);
#endif
