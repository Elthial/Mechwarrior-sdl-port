#include "mechwarrior/combat_polygons.h"
#include <stdlib.h>

/*2A36:0071. Native polygon-span writer, including first-attempt count0,
 * signed endpoint comparison, palette/parity, partial masks and latch reuse.
 * Two raster phases stay inline; they are not calls to invented draw helpers. */
void BtechEgaFillPolygonSpans(const BtechPolygonSpan *header)
{
    if (!btechHostWritePortWord || !btechHostWritePort || !btechHostReadVideoByte || !btechHostWriteVideoByte) abort();
    btechHostWritePortWord(COMBAT_EGA_GRAPHICS_PORT,COMBAT_EGA_WRITE_MODE_TWO);
    btechHostWritePortWord(COMBAT_EGA_GRAPHICS_PORT,COMBAT_EGA_ALL_PIXEL_BITS);
    uint16_t page=drawPageSelector,row=header->header.firstRow;
    egaSpanRowsRemaining=header->header.rowCount;
    const BtechPolygonSpan *span=header+1;
    uint8_t colour=drawLineColour;
    spanPrimaryPattern=palettePrimarySlots[colour/COMBAT_PALETTE_WORDS][colour%COMBAT_PALETTE_WORDS];
    uint16_t secondary=paletteSecondarySlots[colour/COMBAT_PALETTE_WORDS][colour%COMBAT_PALETTE_WORDS];
    bool alternating=secondary!=spanPrimaryPattern;
    if (alternating) spanSecondaryPattern=secondary;
    uint8_t parity=row&1;
    for (;;)
    {
        uint16_t left=span->endpoints.left,right=span->endpoints.right;++span;
        if ((int16_t)right>=(int16_t)left)
        {
            if (row>=COMBAT_SCREEN_HEIGHT) abort(); /* Unmapped adjacent row-table reads. */
            uint16_t width=(uint16_t)(right-left+1);
            uint16_t firstByte=(uint16_t)(graphicsRowOffsets[row]+(left>>3));
            uint16_t pattern=alternating && parity?spanSecondaryPattern:spanPrimaryPattern;
            if (alternating) {egaSpanHighColour=(uint8_t)(pattern>>8);egaSpanLowColour=(uint8_t)pattern;}
            uint16_t firstPixel=left&7,endPixel=(uint16_t)(firstPixel+width);
            uint8_t leading=egaPolygonLeadingMasks[firstPixel];
            for (uint16_t phase=0;phase<(alternating?2:1);++phase)
            {
                uint8_t columns=alternating?(phase?0x55:0xAA):0xFF;
                uint8_t paint=alternating?(phase?egaSpanLowColour:egaSpanHighColour):(uint8_t)pattern;
                uint16_t cursor=firstByte;
                if ((int16_t)endPixel<8)
                {
                    if ((uint8_t)endPixel>=8) abort(); /* Adjacent mask/scratch aliases unsupported. */
                    uint8_t mask=leading&egaPolygonTrailingMasks[(uint8_t)endPixel]&columns;
                    btechHostWritePortWord(COMBAT_EGA_GRAPHICS_PORT,COMBAT_EGA_BIT_MASK_REGISTER|((uint16_t)mask<<8));
                    (void)btechHostReadVideoByte(page,cursor);btechHostWriteVideoByte(page,cursor,paint);
                    continue;
                }
                uint16_t remaining=(uint16_t)(width-(8-firstPixel));
                btechHostWritePortWord(COMBAT_EGA_GRAPHICS_PORT,COMBAT_EGA_BIT_MASK_REGISTER|((uint16_t)(leading&columns)<<8));
                (void)btechHostReadVideoByte(page,cursor);btechHostWriteVideoByte(page,cursor++,paint);
                uint16_t bulk=remaining>>3;
                if (bulk)
                {
                    if (alternating) btechHostWritePortWord(COMBAT_EGA_GRAPHICS_PORT,COMBAT_EGA_BIT_MASK_REGISTER|((uint16_t)columns<<8));
                    else btechHostWritePort(COMBAT_EGA_GRAPHICS_DATA_PORT,0xFF);
                    if (alternating && phase) (void)btechHostReadVideoByte(page,cursor);
                    do {btechHostWriteVideoByte(page,cursor++,paint);} while (--bulk);
                }
                uint16_t tail=remaining&7;
                if (tail)
                {
                    uint8_t mask=egaPolygonTrailingMasks[tail]&columns;
                    if (alternating) btechHostWritePortWord(COMBAT_EGA_GRAPHICS_PORT,COMBAT_EGA_BIT_MASK_REGISTER|((uint16_t)mask<<8));
                    else btechHostWritePort(COMBAT_EGA_GRAPHICS_DATA_PORT,mask);
                    (void)btechHostReadVideoByte(page,cursor);btechHostWriteVideoByte(page,cursor,paint);
                }
            }
        }
        uint16_t oldCount=egaSpanRowsRemaining--;
        if ((int16_t)oldCount<=1) break;
        ++row;parity^=1;
    }
}
