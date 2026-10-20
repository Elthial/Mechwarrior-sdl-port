#include "mechwarrior/combat_video.h"
#include <stdlib.h>

enum { EGA_CLEAR_SET_RESET_COLOUR=0x0000 };

/*2A36:1BB0. Four plane-major REP MOVSW passes, high plane first. The RAM
 * cursor wraps as a WORD, row start advances40 regardless of row width.
 * Converted callers supply paragraph-aligned storage and clear DF. */
void BtechEgaCaptureScreenRectangle(BtechResourceAllocation buffer,uint16_t x,uint16_t y,uint16_t width,uint16_t height)
{
    if (!btechHostWritePortWord || !btechHostReadVideoByte) abort();
    btechHostWritePortWord(COMBAT_EGA_GRAPHICS_PORT,COMBAT_EGA_WRITE_MODE_ZERO);
    btechHostWritePortWord(COMBAT_EGA_GRAPHICS_PORT,COMBAT_EGA_ALL_PIXEL_BITS);
    btechHostWritePortWord(COMBAT_EGA_GRAPHICS_PORT,EGA_CLEAR_SET_RESET_COLOUR);
    uint16_t leftByte=x>>3;
    uint16_t coveredBytes=((uint16_t)(x+width)>>3)-leftByte+1;
    uint16_t wordsPerRow=(coveredBytes>>1)+(coveredBytes&1);
    if (y>=COMBAT_SCREEN_HEIGHT) abort(); /* Adjacent row-owner aliases not represented. */
    uint16_t screenStart=graphicsRowOffsets[y]+leftByte;
    uint16_t page=copySourcePageSelector,bufferCursor=0;
    for (int16_t plane=COMBAT_EGA_PLANES-1;plane>=0;--plane)
    {
        btechHostWritePortWord(COMBAT_EGA_GRAPHICS_PORT,(uint16_t)(plane<<8)|COMBAT_EGA_READ_MAP_REGISTER);
        uint16_t rowsRemaining=height,screenCursor=screenStart;
        do
        {
            uint16_t rowStart=screenCursor;
            for (uint16_t wordsRemaining=wordsPerRow;wordsRemaining;--wordsRemaining)
            {
                if (!buffer.bytes || (uint32_t)bufferCursor+2>buffer.byteCount) abort();
                /* A native WORD startingFFFF crosses the segment physically;
                 * only the later SI increment wraps. It is not two LODSBs. */
                uint8_t low=btechHostReadVideoByte(page,screenCursor);
                uint8_t high=btechHostReadVideoByte((uint16_t)(page+(screenCursor==UINT16_MAX?0x1000:0)),(uint16_t)(screenCursor+1));
                buffer.bytes[bufferCursor]=low;buffer.bytes[bufferCursor+1]=high;
                bufferCursor+=2;screenCursor+=2;
            }
            screenCursor=rowStart+COMBAT_SCREEN_ROW_BYTES;
        } while (--rowsRemaining);
    }
    btechHostWritePortWord(COMBAT_EGA_GRAPHICS_PORT,COMBAT_EGA_WRITE_MODE_TWO);
}

/*2A36:1C69. WORD-aligned RAM cursor, inherited logical operation/set-reset
 * enables and native live latches. No clipping, pixel-mask helper or repaint. */
void BtechEgaRestoreScreenRectangle(BtechResourceAllocation buffer,uint16_t x,uint16_t y,uint16_t width,uint16_t height)
{
    if (!btechHostWritePortWord || !btechHostWriteVideoByte) abort();
    btechHostWritePortWord(COMBAT_EGA_GRAPHICS_PORT,COMBAT_EGA_WRITE_MODE_ZERO);
    btechHostWritePortWord(COMBAT_EGA_GRAPHICS_PORT,COMBAT_EGA_ALL_PIXEL_BITS);
    btechHostWritePortWord(COMBAT_EGA_GRAPHICS_PORT,EGA_CLEAR_SET_RESET_COLOUR);
    uint16_t leftByte=x>>3;
    uint16_t coveredBytes=((uint16_t)(x+width)>>3)-leftByte+1;
    uint16_t wordsPerRow=(coveredBytes>>1)+(coveredBytes&1);
    if (y>=COMBAT_SCREEN_HEIGHT) abort();
    uint16_t screenStart=graphicsRowOffsets[y]+leftByte;
    uint16_t page=drawPageSelector,bufferCursor=0;
    for (int16_t plane=COMBAT_EGA_PLANES-1;plane>=0;--plane)
    {
        btechHostWritePortWord(COMBAT_EGA_SEQUENCER_PORT,(uint16_t)((1u<<plane)<<8)|COMBAT_EGA_SEQUENCER_MAP_MASK);
        uint16_t rowsRemaining=height,screenCursor=screenStart;
        do
        {
            uint16_t rowStart=screenCursor;
            for (uint16_t wordsRemaining=wordsPerRow;wordsRemaining;--wordsRemaining)
            {
                if (!buffer.bytes || (uint32_t)bufferCursor+2>buffer.byteCount) abort();
                uint8_t low=buffer.bytes[bufferCursor],high=buffer.bytes[bufferCursor+1];
                btechHostWriteVideoByte(page,screenCursor,low);
                btechHostWriteVideoByte((uint16_t)(page+(screenCursor==UINT16_MAX?0x1000:0)),(uint16_t)(screenCursor+1),high);
                bufferCursor+=2;screenCursor+=2;
            }
            screenCursor=rowStart+COMBAT_SCREEN_ROW_BYTES;
        } while (--rowsRemaining);
    }
    btechHostWritePortWord(COMBAT_EGA_GRAPHICS_PORT,COMBAT_EGA_WRITE_MODE_TWO);
    btechHostWritePortWord(COMBAT_EGA_SEQUENCER_PORT,COMBAT_EGA_MAP_ALL_PLANES);
}
