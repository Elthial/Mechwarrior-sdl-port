#include "mechwarrior/port_diagnostics.h"
#include "mechwarrior/combat_video.h"
#include <stdlib.h>

/*1BF5:1688. BYTE arc tables and two clipped halves; callbacks remain live.
 * EGA spans own their register operations; do not add a controller reset. */
void BtechDrawStyledEllipse(int16_t radius, uint16_t centerX, uint16_t centerY)
{
    // DS=SS=304E; BP becomes the row-table byte index, not a stack object.
    // See SHARED_ELLIPSE_RECOVERY.md for BYTE clipping and callback contracts.
    uint16_t page=drawPageSelector;
    if (combatRendererKind!=COMBAT_VIDEO_EGA) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: combatRendererKind!=COMBAT_VIDEO_EGA"); /* Tandy window resolver remains outside EGA scope. */
    ellipsePageSelector=page; // Before even radius rejection.
    if (radius<=0x0001)
    {
        if (radius==0x0001)
            BtechWriteClippedPixel(centerX,centerY,(uint16_t)drawLineColour);
        return;
    }
    if ((uint16_t)radius>COMBAT_ELLIPSE_MAX_RADIUS)radius=COMBAT_ELLIPSE_MAX_RADIUS; // Also writes native argument slot.
    uint16_t arcX=0x0000,arcY=(uint16_t)radius;
    uint16_t diameter=(arcY+(arcY>>0x0002))<<0x0001;
    int16_t decision=0x0003-(arcY<<0x0001);
    while ((int16_t)arcX<=(int16_t)arcY)
    {
        uint16_t delta=(uint16_t)radius-arcX;
        uint16_t inset=delta+(delta>>0x0002);
        uint16_t rowIndex=(uint16_t)radius-arcY;
        ellipseLeftInsets[rowIndex]=(uint8_t)inset;
        ellipseInteriorWidths[rowIndex]=(uint8_t)(diameter-(inset<<0x0001));
        delta=(uint16_t)((uint16_t)radius-arcY);
        inset=(uint16_t)(delta+(delta>>0x0002));
        rowIndex=(uint16_t)((uint16_t)radius-arcX);
        ellipseLeftInsets[rowIndex]=(uint8_t)inset;
        ellipseInteriorWidths[rowIndex]=(uint8_t)(diameter-(inset<<0x0001));
        if (arcX==arcY)break;
        int16_t previousDecision=decision; // XCHG leaves OLD decision in AX.
        decision=(int16_t)((arcX<<0x0002)+(uint16_t)decision+0x0006);
        ++arcX;
        if (previousDecision>=0x0000)
        {decision=(int16_t)((uint16_t)decision-(arcY<<0x0002)+0x0004);--arcY;}
    }
    ellipseBorderWidths[0x0000]=ellipseInteriorWidths[0x0000];
    ellipseInteriorWidths[0x0000]=0x00;
    uint8_t previousInset=ellipseLeftInsets[0x0000];
    for (uint16_t rowIndex=0x0001,remaining=(uint16_t)radius;remaining!=0x0000;--remaining,++rowIndex)
    {
        uint8_t inset=ellipseLeftInsets[rowIndex];
        uint8_t borderWidth=previousInset-inset+0x01;
        ellipseBorderWidths[rowIndex]=borderWidth;
        borderWidth=(uint8_t)(borderWidth<<0x01);
        uint8_t interiorWidth=ellipseInteriorWidths[rowIndex]-borderWidth;
        if (interiorWidth==0x00)ellipseBorderWidths[rowIndex]=borderWidth;
        ellipseInteriorWidths[rowIndex]=interiorWidth;
        previousInset=inset;
    }
    page=ellipsePageSelector; // Native ES reload1789/178C.
    spanPrimaryPattern=palettePrimarySlots[drawLineColour/COMBAT_PALETTE_WORDS][drawLineColour%COMBAT_PALETTE_WORDS];
    spanSecondaryPattern=paletteSecondarySlots[drawLineColour/COMBAT_PALETTE_WORDS][drawLineColour%COMBAT_PALETTE_WORDS];
    if (spanPrimaryPattern==spanSecondaryPattern)
        shapeSpanCallback=(BtechShapeSpanCallback){graphicsFillSolidSpan,NULL};
    else shapeSpanCallback=(BtechShapeSpanCallback){NULL,graphicsFillAlternatingSpan};
    drawColourPattern=palettePrimarySlots[drawFillColour/COMBAT_PALETTE_WORDS][drawFillColour%COMBAT_PALETTE_WORDS];
    uint16_t horizontalRadius=(uint16_t)radius+((uint16_t)radius>>0x0002);
    uint16_t leftX=centerX-horizontalRadius;
    uint16_t clipWidth=(uint16_t)(horizontalRadius<<0x0001),leftMargin=0x0000;
    uint8_t clipLeft=0x00,clipRight=(uint8_t)clipWidth;
    if (screenPolygonNeedsClip!=0x00)
    {
        if ((int16_t)clipMinimumX>(int16_t)leftX)
        {
            leftMargin=(uint16_t)(clipMinimumX-leftX);
            if ((int16_t)clipWidth<=(int16_t)leftMargin)return;
        }
        uint16_t rightX=leftX+clipWidth;
        if ((uint16_t)clipMaximumX<rightX) // UNSIGNED JAE, unlike left clipping.
        {
            uint16_t rightOverflow=rightX-clipMaximumX;
            uint16_t beforeIncrement=clipWidth-rightOverflow;
            clipWidth=(uint16_t)(beforeIncrement+0x0001);
            if ((int16_t)beforeIncrement<0x0000)return; // INC/JG retains OF.
        }
        clipLeft=(uint8_t)leftMargin;clipRight=(uint8_t)clipWidth;
    }
    uint16_t rows=(uint16_t)radius,tableRow=0x0000;
    uint16_t firstRow=centerY-(uint16_t)radius;
    if (screenPolygonNeedsClip!=0x00)
    {
        if ((int16_t)clipMinimumY>(int16_t)firstRow)
        {
            uint16_t skippedRows=clipMinimumY-firstRow;
            int32_t difference=(int16_t)rows-(int16_t)skippedRows;
            rows -= skippedRows;
            if (difference<=0x0000)goto lowerHalf;
            tableRow += skippedRows;firstRow=clipMinimumY;
        }
        if ((uint16_t)(rows+firstRow)>(uint16_t)clipMaximumY)
        {
            uint16_t beforeIncrement=clipMaximumY-firstRow;
            rows=(uint16_t)(beforeIncrement+0x0001);
            if ((int16_t)beforeIncrement<0x0000)goto lowerHalf;
        }
    }
    {
        uint16_t scanlineByteIndex=firstRow<<0x0001;
        spanAlternationState=firstRow&0x0001;
        uint16_t scanlineOffset;
        if ((scanlineByteIndex>>1)<COMBAT_SCREEN_HEIGHT) scanlineOffset=graphicsRowOffsets[scanlineByteIndex>>1];
            else if ((scanlineByteIndex>>1)-COMBAT_SCREEN_HEIGHT<0x15C) scanlineOffset=herculesDisplayRowOffsets[(scanlineByteIndex>>1)-COMBAT_SCREEN_HEIGHT];
            else /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Unmapped adjacent row-table owner."); /* Unmapped adjacent row-table owner. */
        do
        {
            if (tableRow>=COMBAT_ELLIPSE_ROWS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: tableRow>=COMBAT_ELLIPSE_ROWS"); /* Unmapped adjacent arc owner. */
            // Signed BYTE side tests, unsigned right caps.
            uint8_t edge=ellipseLeftInsets[tableRow];
            uint8_t width=ellipseBorderWidths[tableRow];
            if ((int8_t)edge>(int8_t)clipRight)goto upperRowDone;
            if ((int8_t)edge<(int8_t)clipLeft)
            {
                edge -= clipLeft;
                int16_t signedSum=(int8_t)width+(int8_t)edge;
                width += edge;
                if (signedSum<=0x0000)
                {edge=(uint8_t)(width+clipLeft);goto upperInterior;}
                edge=clipLeft;
            }
            width += edge;
            if (width>clipRight)width=clipRight;
            width -= edge;
            {
                uint16_t startX=leftX+(uint16_t)edge;
                edge += width; // Saved AX contains next edge, not pattern.
                if (!graphicsFillSolidSpan) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !graphicsFillSolidSpan");
                graphicsFillSolidSpan(drawColourPattern,startX,(int16_t)width,scanlineOffset,page);
            }
upperInterior:
            width=ellipseInteriorWidths[tableRow];
            if (width==0x00)goto upperRowDone;
            if (drawPolygonFilled==0x00)
            {edge += width;goto upperRightBorder;}
            if ((int8_t)edge>(int8_t)clipRight)goto upperRowDone;
            if ((int8_t)edge<(int8_t)clipLeft)
            {
                edge -= clipLeft;
                int16_t signedSum=(int8_t)width+(int8_t)edge;
                width += edge;
                if (signedSum<=0x0000)
                {edge=(uint8_t)(width+clipLeft);goto upperRightBorder;}
                edge=clipLeft;
            }
            width += edge;
            if (width>clipRight)width=clipRight;
            width -= edge;
            {
                uint16_t startX=leftX+(uint16_t)edge;
                edge += width;
                // DX receives PRIMARY, AX SECONDARY; do not swap these patterns.
                if (shapeSpanCallback.alternating)
                    shapeSpanCallback.alternating(spanSecondaryPattern,startX,(int16_t)width,spanPrimaryPattern,scanlineOffset,page);
                else
                {
                    if (!shapeSpanCallback.solid) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !shapeSpanCallback.solid");
                    shapeSpanCallback.solid(spanSecondaryPattern,startX,(int16_t)width,scanlineOffset,page);
                }
            }
upperRightBorder:
            width=ellipseBorderWidths[tableRow]; // Reload live after callbacks.
            if ((int8_t)edge>(int8_t)clipRight)goto upperRowDone;
            if ((int8_t)edge<(int8_t)clipLeft)
            {
                edge -= clipLeft;
                int16_t signedSum=(int8_t)width+(int8_t)edge;
                width += edge;
                if (signedSum<=0x0000)goto upperRowDone;
                edge=clipLeft;
            }
            width += edge;
            if (width>clipRight)width=clipRight;
            width -= edge;
            if (!graphicsFillSolidSpan) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !graphicsFillSolidSpan");
            graphicsFillSolidSpan(drawColourPattern,(uint16_t)(leftX+(uint16_t)edge),(int16_t)width,scanlineOffset,page);
upperRowDone:
            scanlineByteIndex+=2;
            spanAlternationState=(scanlineByteIndex>>0x0001)&0x0001;
            // Native extra lookup after final row; default SS=304E, not a frame.
            if ((scanlineByteIndex>>1)<COMBAT_SCREEN_HEIGHT) scanlineOffset=graphicsRowOffsets[scanlineByteIndex>>1];
            else if ((scanlineByteIndex>>1)-COMBAT_SCREEN_HEIGHT<0x15C) scanlineOffset=herculesDisplayRowOffsets[(scanlineByteIndex>>1)-COMBAT_SCREEN_HEIGHT];
            else /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Unmapped adjacent row-table owner."); /* Unmapped adjacent row-table owner. */
            ++tableRow;
            --rows;
        } while (rows!=0x0000);
    }
lowerHalf:
    rows=(uint16_t)((uint16_t)radius+0x0001);
    firstRow=centerY;tableRow=(uint16_t)radius;
    if (screenPolygonNeedsClip!=0x00) // Reload live after upper callbacks.
    {
        if ((int16_t)clipMinimumY>(int16_t)firstRow)
        {
            uint16_t skippedRows=clipMinimumY-firstRow;
            rows -= skippedRows;
            int32_t difference=(int16_t)tableRow-(int16_t)skippedRows;
            tableRow -= skippedRows;
            if (difference<=0x0000)return; // Native tests SI, not remaining CX.
            firstRow=clipMinimumY;
        }
        if ((uint16_t)(rows+firstRow)>(uint16_t)clipMaximumY)
        {
            uint16_t beforeIncrement=clipMaximumY-firstRow;
            rows=(uint16_t)(beforeIncrement+0x0001);
            if ((int16_t)beforeIncrement<0x0000)return;
        }
    }
    // Lower half reseeds parity once; thereafter only alternating spans toggle it.
    {
        uint16_t scanlineByteIndex=firstRow<<0x0001;
        spanAlternationState=firstRow&0x0001;
        uint16_t scanlineOffset;
        if ((scanlineByteIndex>>1)<COMBAT_SCREEN_HEIGHT) scanlineOffset=graphicsRowOffsets[scanlineByteIndex>>1];
            else if ((scanlineByteIndex>>1)-COMBAT_SCREEN_HEIGHT<0x15C) scanlineOffset=herculesDisplayRowOffsets[(scanlineByteIndex>>1)-COMBAT_SCREEN_HEIGHT];
            else /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Unmapped adjacent row-table owner."); /* Unmapped adjacent row-table owner. */
        do
        {
            if (tableRow>=COMBAT_ELLIPSE_ROWS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: tableRow>=COMBAT_ELLIPSE_ROWS"); /* Unmapped adjacent arc owner. */
            // Signed BYTE side tests, unsigned right caps.
            uint8_t edge=ellipseLeftInsets[tableRow];
            uint8_t width=ellipseBorderWidths[tableRow];
            if ((int8_t)edge>(int8_t)clipRight)goto lowerRowDone;
            if ((int8_t)edge<(int8_t)clipLeft)
            {
                edge -= clipLeft;
                int16_t signedSum=(int8_t)width+(int8_t)edge;
                width += edge;
                if (signedSum<=0x0000)
                {edge=(uint8_t)(width+clipLeft);goto lowerInterior;}
                edge=clipLeft;
            }
            width += edge;
            if (width>clipRight)width=clipRight;
            width -= edge;
            {
                uint16_t startX=leftX+(uint16_t)edge;
                edge += width; // Saved AX contains next edge, not pattern.
                if (!graphicsFillSolidSpan) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !graphicsFillSolidSpan");
                graphicsFillSolidSpan(drawColourPattern,startX,(int16_t)width,scanlineOffset,page);
            }
lowerInterior:
            width=ellipseInteriorWidths[tableRow];
            if (width==0x00)goto lowerRowDone;
            if (drawPolygonFilled==0x00)
            {edge += width;goto lowerRightBorder;}
            if ((int8_t)edge>(int8_t)clipRight)goto lowerRowDone;
            if ((int8_t)edge<(int8_t)clipLeft)
            {
                edge -= clipLeft;
                int16_t signedSum=(int8_t)width+(int8_t)edge;
                width += edge;
                if (signedSum<=0x0000)
                {edge=(uint8_t)(width+clipLeft);goto lowerRightBorder;}
                edge=clipLeft;
            }
            width += edge;
            if (width>clipRight)width=clipRight;
            width -= edge;
            {
                uint16_t startX=leftX+(uint16_t)edge;
                edge += width;
                // DX receives PRIMARY, AX SECONDARY; do not swap these patterns.
                if (shapeSpanCallback.alternating)
                    shapeSpanCallback.alternating(spanSecondaryPattern,startX,(int16_t)width,spanPrimaryPattern,scanlineOffset,page);
                else
                {
                    if (!shapeSpanCallback.solid) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !shapeSpanCallback.solid");
                    shapeSpanCallback.solid(spanSecondaryPattern,startX,(int16_t)width,scanlineOffset,page);
                }
            }
lowerRightBorder:
            width=ellipseBorderWidths[tableRow]; // Reload live after callbacks.
            if ((int8_t)edge>(int8_t)clipRight)goto lowerRowDone;
            if ((int8_t)edge<(int8_t)clipLeft)
            {
                edge -= clipLeft;
                int16_t signedSum=(int8_t)width+(int8_t)edge;
                width += edge;
                if (signedSum<=0x0000)goto lowerRowDone;
                edge=clipLeft;
            }
            width += edge;
            if (width>clipRight)width=clipRight;
            width -= edge;
            if (!graphicsFillSolidSpan) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !graphicsFillSolidSpan");
            graphicsFillSolidSpan(drawColourPattern,(uint16_t)(leftX+(uint16_t)edge),(int16_t)width,scanlineOffset,page);
lowerRowDone:
            scanlineByteIndex+=2;
            // Native extra lookup after final row; default SS=304E, not a frame.
            if ((scanlineByteIndex>>1)<COMBAT_SCREEN_HEIGHT) scanlineOffset=graphicsRowOffsets[scanlineByteIndex>>1];
            else if ((scanlineByteIndex>>1)-COMBAT_SCREEN_HEIGHT<0x15C) scanlineOffset=herculesDisplayRowOffsets[(scanlineByteIndex>>1)-COMBAT_SCREEN_HEIGHT];
            else /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Unmapped adjacent row-table owner."); /* Unmapped adjacent row-table owner. */
            --tableRow;
            --rows;
        } while (rows!=0x0000);
    }
}
