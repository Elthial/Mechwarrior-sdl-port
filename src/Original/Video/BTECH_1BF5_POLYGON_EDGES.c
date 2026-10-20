#include "mechwarrior/combat_polygons.h"
#include <stdlib.h>

/*1BF5:0845. Original register setup and shared JMP0931..0ED0 row tail.
 * The native200 unrolled rows become this ordered carry loop, not a new callee.
 * Forward DF only; CH0 returns, CH201..255 is an unsupported pre-kernel jump. */
void BtechAccumulateVerticalPolygonEdge(int16_t firstY,int16_t fixedX,int16_t secondY)
{
    if (firstY<=secondY) {int16_t saved=firstY;firstY=secondY;secondY=saved;}
    uint16_t row=(uint16_t)secondY,currentX=(uint16_t)fixedX;
    uint8_t rows=(firstY-secondY+1);
    uint16_t fractionStep=0,integerStep=0;
    int16_t rowStep=1;
    uint16_t fraction=fractionStep;
    if (rows>COMBAT_SCREEN_HEIGHT) abort();
    while (rows--)
    {
        if (row>=COMBAT_SCREEN_HEIGHT || (polygonSpanBoundaryByteOffset!=0 && polygonSpanBoundaryByteOffset!=2)) abort();
        if (polygonSpanBoundaryByteOffset) polygonScanlineSpans[row].endpoints.right=currentX;
        else polygonScanlineSpans[row].endpoints.left=currentX;
        row += rowStep;
        uint32_t sum=(uint32_t)fraction+fractionStep;
        fraction=(uint16_t)sum;currentX += integerStep+(sum>>16);
    }
}

/*1BF5:0860. Original register setup and shared JMP0931..0ED0 row tail.
 * The native200 unrolled rows become this ordered carry loop, not a new callee.
 * Forward DF only; CH0 returns, CH201..255 is an unsupported pre-kernel jump. */
void BtechAccumulateYMajorPolygonEdge(int16_t firstY,int16_t firstX,int16_t secondX,int16_t secondY)
{
    if (firstX>=secondX)
    {int16_t saved=firstX;firstX=secondX;secondX=saved;saved=firstY;firstY=secondY;secondY=saved;}
    uint16_t row=(uint16_t)firstY,currentX=(uint16_t)firstX;
    int16_t deltaY=secondY-firstY;
    uint16_t magnitudeY=deltaY<0?(uint16_t)(0-(uint16_t)deltaY):(uint16_t)deltaY;
    uint16_t inclusiveRows=magnitudeY+1;
    uint8_t rows=inclusiveRows;
    int16_t rowStep=deltaY<0?-1:1;
    uint16_t width=secondX-firstX+1;
    if (!inclusiveRows) abort();
    uint32_t quotient=((uint32_t)width<<16)/inclusiveRows;
    if (quotient>UINT16_MAX) abort();
    uint16_t fractionStep=(uint16_t)quotient,integerStep=0;
    uint16_t fraction=fractionStep;
    if (rows>COMBAT_SCREEN_HEIGHT) abort();
    while (rows--)
    {
        if (row>=COMBAT_SCREEN_HEIGHT || (polygonSpanBoundaryByteOffset!=0 && polygonSpanBoundaryByteOffset!=2)) abort();
        if (polygonSpanBoundaryByteOffset) polygonScanlineSpans[row].endpoints.right=currentX;
        else polygonScanlineSpans[row].endpoints.left=currentX;
        row += rowStep;
        uint32_t sum=(uint32_t)fraction+fractionStep;
        fraction=(uint16_t)sum;currentX += integerStep+(sum>>16);
    }
}

/*1BF5:0893. Original register setup and shared JMP0931..0ED0 row tail.
 * The native200 unrolled rows become this ordered carry loop, not a new callee.
 * Forward DF only; CH0 returns, CH201..255 is an unsupported pre-kernel jump. */
void BtechAccumulateDiagonalPolygonEdge(int16_t firstY,int16_t firstX,int16_t secondX,int16_t secondY)
{
    if (firstY>=secondY)
    {int16_t saved=firstY;firstY=secondY;secondY=saved;saved=firstX;firstX=secondX;secondX=saved;}
    uint16_t row=(uint16_t)firstY,currentX=(uint16_t)firstX;
    uint8_t rows=(secondY-firstY+1);
    uint16_t fractionStep=0,integerStep=firstX<secondX?1:UINT16_MAX;
    int16_t rowStep=1;
    uint16_t fraction=fractionStep;
    if (rows>COMBAT_SCREEN_HEIGHT) abort();
    while (rows--)
    {
        if (row>=COMBAT_SCREEN_HEIGHT || (polygonSpanBoundaryByteOffset!=0 && polygonSpanBoundaryByteOffset!=2)) abort();
        if (polygonSpanBoundaryByteOffset) polygonScanlineSpans[row].endpoints.right=currentX;
        else polygonScanlineSpans[row].endpoints.left=currentX;
        row += rowStep;
        uint32_t sum=(uint32_t)fraction+fractionStep;
        fraction=(uint16_t)sum;currentX += integerStep+(sum>>16);
    }
}

/*1BF5:08B9. Original register setup and shared JMP0931..0ED0 row tail.
 * The native200 unrolled rows become this ordered carry loop, not a new callee.
 * Forward DF only; CH0 returns, CH201..255 is an unsupported pre-kernel jump. */
void BtechAccumulateDescendingXMajorPolygonEdge(int16_t firstY,int16_t firstX,int16_t secondX,int16_t secondY)
{
    if (firstX<=secondX)
    {int16_t saved=firstX;firstX=secondX;secondX=saved;saved=firstY;firstY=secondY;secondY=saved;}
    uint16_t row=(uint16_t)firstY,currentX=(uint16_t)firstX;
    int16_t deltaY=secondY-firstY;
    uint16_t magnitudeY=deltaY<0?(uint16_t)(0-(uint16_t)deltaY):(uint16_t)deltaY;
    uint16_t inclusiveRows=magnitudeY+1;
    uint8_t rows=inclusiveRows;
    int16_t rowStep=deltaY<0?-1:1;
    uint16_t width=firstX-secondX+1;
    if (!inclusiveRows) abort();
    uint16_t integerStep=width/inclusiveRows;
    uint16_t remainder=width%inclusiveRows;
    uint16_t fractionStep=((uint32_t)remainder<<16)/inclusiveRows;
    integerStep=(uint16_t)(0-(uint16_t)(integerStep+(fractionStep!=0)));
    fractionStep=(uint16_t)(0-fractionStep);
    uint16_t fraction=fractionStep;
    if (rows>COMBAT_SCREEN_HEIGHT) abort();
    while (rows--)
    {
        if (row>=COMBAT_SCREEN_HEIGHT || (polygonSpanBoundaryByteOffset!=0 && polygonSpanBoundaryByteOffset!=2)) abort();
        if (polygonSpanBoundaryByteOffset) polygonScanlineSpans[row].endpoints.right=currentX;
        else polygonScanlineSpans[row].endpoints.left=currentX;
        row += rowStep;
        uint32_t sum=(uint32_t)fraction+fractionStep;
        fraction=(uint16_t)sum;currentX += integerStep+(sum>>16);
    }
}

/*1BF5:08F6. Original register setup and shared JMP0931..0ED0 row tail.
 * The native200 unrolled rows become this ordered carry loop, not a new callee.
 * Forward DF only; CH0 returns, CH201..255 is an unsupported pre-kernel jump. */
void BtechAccumulateAscendingXMajorPolygonEdge(int16_t firstY,int16_t firstX,int16_t secondX,int16_t secondY)
{
    if (firstX>=secondX)
    {int16_t saved=firstX;firstX=secondX;secondX=saved;saved=firstY;firstY=secondY;secondY=saved;}
    uint16_t row=(uint16_t)firstY,currentX=(uint16_t)firstX;
    int16_t deltaY=secondY-firstY;
    uint16_t magnitudeY=deltaY<0?(uint16_t)(0-(uint16_t)deltaY):(uint16_t)deltaY;
    uint16_t inclusiveRows=magnitudeY+1;
    uint8_t rows=inclusiveRows;
    int16_t rowStep=deltaY<0?-1:1;
    uint16_t width=secondX-firstX+1;
    if (!inclusiveRows) abort();
    uint16_t integerStep=width/inclusiveRows;
    uint16_t remainder=width%inclusiveRows;
    uint16_t fractionStep=((uint32_t)remainder<<16)/inclusiveRows;
    uint16_t fraction=fractionStep;
    if (rows>COMBAT_SCREEN_HEIGHT) abort();
    while (rows--)
    {
        if (row>=COMBAT_SCREEN_HEIGHT || (polygonSpanBoundaryByteOffset!=0 && polygonSpanBoundaryByteOffset!=2)) abort();
        if (polygonSpanBoundaryByteOffset) polygonScanlineSpans[row].endpoints.right=currentX;
        else polygonScanlineSpans[row].endpoints.left=currentX;
        row += rowStep;
        uint32_t sum=(uint32_t)fraction+fractionStep;
        fraction=(uint16_t)sum;currentX += integerStep+(sum>>16);
    }
}
