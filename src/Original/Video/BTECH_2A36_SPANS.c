#include "mechwarrior/combat_video.h"
#include <stdlib.h>

/*2A36:1E32. Native mode2 writes, including inherited controller state and
 * latch reuse by REP STOSB. Forward game path; no added clip or mask restore. */
void BtechEgaFillSolidSpan(uint16_t pattern,uint16_t x,int16_t count,uint16_t rowOffset,uint16_t page)
{
    if (count<=0) return;
    if (!btechHostWritePortWord || !btechHostReadVideoByte || !btechHostWriteVideoByte) abort();
    uint16_t cursor=(uint16_t)(rowOffset+(x>>3));
    uint16_t firstPixel=x&7,endPixel=(uint16_t)(firstPixel+(uint16_t)count);
    uint8_t mask=egaSpanLeadingMasks[firstPixel];
    if ((int16_t)endPixel<8)
    {
        mask&=egaSpanTrailingMasks[(uint8_t)endPixel];
        btechHostWritePortWord(COMBAT_EGA_GRAPHICS_PORT,COMBAT_EGA_BIT_MASK_REGISTER|((uint16_t)mask<<8));
        (void)btechHostReadVideoByte(page,cursor);
        btechHostWriteVideoByte(page,cursor,(uint8_t)pattern);
        return;
    }
    uint16_t remaining=(uint16_t)((uint16_t)count-(8-firstPixel));
    btechHostWritePortWord(COMBAT_EGA_GRAPHICS_PORT,COMBAT_EGA_BIT_MASK_REGISTER|((uint16_t)mask<<8));
    (void)btechHostReadVideoByte(page,cursor);
    btechHostWriteVideoByte(page,cursor++,(uint8_t)pattern);
    uint16_t wholeBytes=(remaining&0xFFF8)>>3;
    if (wholeBytes)
    {
        btechHostWritePortWord(COMBAT_EGA_GRAPHICS_PORT,COMBAT_EGA_ALL_PIXEL_BITS);
        do {btechHostWriteVideoByte(page,cursor++,(uint8_t)pattern);} while (--wholeBytes);
    }
    uint16_t tail=remaining&7;
    if (tail)
    {
        mask=egaSpanTrailingMasks[tail];
        btechHostWritePortWord(COMBAT_EGA_GRAPHICS_PORT,COMBAT_EGA_BIT_MASK_REGISTER|((uint16_t)mask<<8));
        (void)btechHostReadVideoByte(page,cursor);
        btechHostWriteVideoByte(page,cursor,(uint8_t)pattern);
    }
}

/*2A36:1D40. Two original raster phases kept inline: the second refreshes
 * only its FIRST bulk latch, not every destination. That quirk is observable. */
void BtechEgaFillAlternatingSpan(uint16_t primary,uint16_t x,int16_t count,uint16_t alternate,uint16_t rowOffset,uint16_t page)
{
    spanAlternationState^=1;
    uint16_t pattern=spanAlternationState?alternate:primary;
    if (count<=0) return;
    if (!btechHostWritePortWord || !btechHostReadVideoByte || !btechHostWriteVideoByte) abort();
    egaSpanPrimaryColour=(uint8_t)(pattern>>8);
    egaSpanSecondaryColour=(uint8_t)pattern;
    uint16_t firstPixel=x&7,endPixel=(uint16_t)(firstPixel+(uint16_t)count);
    uint8_t leading=egaSpanLeadingMasks[firstPixel];
    uint16_t firstByte=(uint16_t)(rowOffset+(x>>3));
    for (uint16_t phase=0;phase<2;++phase)
    {
        uint8_t columns=phase?0x55:0xAA;
        uint8_t colour=phase?egaSpanSecondaryColour:egaSpanPrimaryColour;
        uint16_t cursor=firstByte;
        if ((int16_t)endPixel<8)
        {
            uint8_t mask=leading&egaSpanTrailingMasks[(uint8_t)endPixel]&columns;
            btechHostWritePortWord(COMBAT_EGA_GRAPHICS_PORT,COMBAT_EGA_BIT_MASK_REGISTER|((uint16_t)mask<<8));
            (void)btechHostReadVideoByte(page,cursor);
            btechHostWriteVideoByte(page,cursor,colour);
            continue;
        }
        uint16_t remaining=(uint16_t)((uint16_t)count-(8-firstPixel));
        btechHostWritePortWord(COMBAT_EGA_GRAPHICS_PORT,COMBAT_EGA_BIT_MASK_REGISTER|((uint16_t)(leading&columns)<<8));
        (void)btechHostReadVideoByte(page,cursor);
        btechHostWriteVideoByte(page,cursor++,colour);
        uint16_t wholeBytes=(remaining&0xFFF8)>>3;
        if (wholeBytes)
        {
            btechHostWritePortWord(COMBAT_EGA_GRAPHICS_PORT,COMBAT_EGA_BIT_MASK_REGISTER|((uint16_t)columns<<8));
            if (phase) (void)btechHostReadVideoByte(page,cursor);
            do {btechHostWriteVideoByte(page,cursor++,colour);} while (--wholeBytes);
        }
        uint16_t tail=remaining&7;
        if (tail)
        {
            uint8_t mask=egaSpanTrailingMasks[tail]&columns;
            btechHostWritePortWord(COMBAT_EGA_GRAPHICS_PORT,COMBAT_EGA_BIT_MASK_REGISTER|((uint16_t)mask<<8));
            (void)btechHostReadVideoByte(page,cursor);
            btechHostWriteVideoByte(page,cursor,colour);
        }
    }
}
