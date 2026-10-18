#include "mechwarrior/combat_video.h"
#include <stdlib.h>

/*2A36:1B4C. Same-coordinate aligned byte copy in EGA latch mode1.
 * No clip, page swap, map-mask reset or invented logical-pixel blit. */
void BtechEgaCopyScreenRectangle(uint16_t x,uint16_t y,uint16_t width,uint16_t height)
{
    if (!btechHostWritePortWord || !btechHostReadVideoByte || !btechHostWriteVideoByte) abort();
    btechHostWritePortWord(COMBAT_EGA_GRAPHICS_PORT,COMBAT_EGA_WRITE_MODE_ONE);
    uint16_t end=(uint16_t)(x+width+7)&0xFFF8,first=x&0xFFF8;
    uint16_t bytes=(uint16_t)(end-first)>>3,row=(uint16_t)(y*2)>>1;
    if (row>=COMBAT_SCREEN_HEIGHT) abort();
    uint16_t cursor=(uint16_t)(graphicsRowOffsets[row]+(first>>3));
    uint16_t destinationPage=drawPageSelector,sourcePage=copySourcePageSelector;
    do
    {
        uint16_t nextRow=(uint16_t)(cursor+COMBAT_SCREEN_ROW_BYTES);
        for (uint16_t remaining=bytes;remaining;--remaining)
        {
            uint8_t value=btechHostReadVideoByte(sourcePage,cursor);
            btechHostWriteVideoByte(destinationPage,cursor++,value);
        }
        cursor=nextRow;
    } while (--height);
    btechHostWritePortWord(COMBAT_EGA_GRAPHICS_PORT,COMBAT_EGA_WRITE_MODE_TWO);
}

/*2A36:000B. Installed alternate SCR:VGA target really is a single RETF. */
void BtechEgaNoOperation(void)
{
    /* Authored CB. Not a success-returning placeholder for another adapter. */
}

/*2A36:23AA..24DA (epilogue aliases2C83:0000). Forward packed IO to EGA planes.
 * No clipping/alignment policy added. Each two-WORD read precedes its four
 * plane writes. Hardware plane conversion is retained at the verified boundary.
 * This typed source covers forward, non-video IO buffers, not physical aliases. */
void BtechEgaUploadPackedScreenRegion(BtechResourceAllocation source,uint16_t left,uint16_t top,uint16_t width,uint16_t height)
{
    if (!btechHostWritePortWord || !btechHostWritePort || !btechHostWriteVideoByte) abort();
    btechHostWritePortWord(COMBAT_EGA_GRAPHICS_PORT,COMBAT_EGA_WRITE_MODE_ZERO);
    btechHostWritePortWord(COMBAT_EGA_GRAPHICS_PORT,COMBAT_EGA_ALL_PIXEL_BITS);
    btechHostWritePort(COMBAT_EGA_SEQUENCER_PORT,COMBAT_EGA_SEQUENCER_MAP_MASK);
    if (top>=COMBAT_SCREEN_HEIGHT) abort(); /* Native row-table adjacent-owner alias not represented. */
    uint16_t destination=(uint16_t)(graphicsRowOffsets[top]+(left>>3));
    uint16_t page=drawPageSelector;
    egaScreenUploadRowsRemaining=height;
    egaScreenUploadColumns=width>>3;
    uint16_t columnsRemaining=egaScreenUploadColumns;
    egaScreenUploadRowSkip=COMBAT_SCREEN_ROW_BYTES-columnsRemaining;
    uint32_t sourceCursor=0; /* Host extent check gates normalized-segment wrap, rather than guessing a paragraph alias. */
    for (;;)
    {
        uint8_t planeBits[COMBAT_EGA_PLANES]={0};
        for (uint16_t sourceWord=0;sourceWord<2;++sourceWord)
        {
            if (!source.bytes || sourceCursor+2>source.byteCount || sourceCursor+2>0x10000) abort();
            uint16_t packedPixels=source.bytes[sourceCursor]|((uint16_t)source.bytes[sourceCursor+1]<<8);
            sourceCursor+=2;
            for (uint16_t sourceByte=0;sourceByte<2;++sourceByte)
            {
                uint8_t packedPair=(uint8_t)(packedPixels>>(sourceByte*8));
                for (uint16_t nibble=0;nibble<2;++nibble)
                {
                    uint8_t colour=nibble?packedPair&0x0F:packedPair>>4;
                    for (uint16_t plane=0;plane<COMBAT_EGA_PLANES;++plane)
                        planeBits[plane]=(uint8_t)((planeBits[plane]<<1)|((colour>>plane)&1));
                }
            }
        }
        for (uint16_t plane=0;plane<COMBAT_EGA_PLANES;++plane)
        {
            btechHostWritePort(COMBAT_EGA_SEQUENCER_DATA_PORT,(uint8_t)(1<<plane));
            btechHostWriteVideoByte(page,destination,planeBits[plane]);
        }
        destination++;
        uint16_t columnsBeforeDecrement=columnsRemaining--;
        if ((int16_t)columnsBeforeDecrement>1) continue;
        if ((int16_t)egaScreenUploadRowsRemaining<=1) break;
        egaScreenUploadRowsRemaining--;
        columnsRemaining=egaScreenUploadColumns;
        destination+=egaScreenUploadRowSkip;
    }
    btechHostWritePort(COMBAT_EGA_SEQUENCER_DATA_PORT,COMBAT_EGA_PLANE_MASK_ALL);
    btechHostWritePortWord(COMBAT_EGA_GRAPHICS_PORT,COMBAT_EGA_WRITE_MODE_TWO);
    btechHostWritePortWord(COMBAT_EGA_GRAPHICS_PORT,COMBAT_EGA_DATA_ROTATE_COPY);
}
