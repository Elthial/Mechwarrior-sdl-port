#include "mechwarrior/combat_video.h"
#include <stdlib.h>

/*2A36:1EB0. Truncate each MUL before accumulating DWORD; add one mask plane. */
uint32_t BtechEgaBitmapStorageSize(const BtechBitmapList *list,uint16_t *auxiliaryBytes)
{
    uint32_t packedBytes=0;
    for (uint16_t image=0;image<list->imageCount;++image)
        packedBytes+=(uint16_t)((list->images[image].widthPixels>>1)*list->images[image].heightPixels);
    *auxiliaryBytes=0;
    return packedBytes+(packedBytes>>2);
}

/*2A36:1EF1. Convert the ENTIRE packed stream before associating images.
 * Hardware scratch remains A600; typed RAM references replace selector pairs.
 * Each image retains native WORD carry loss before paragraph normalization. */
void BtechEgaPrepareBitmapList(BtechBitmapList *list,BtechResourceAllocation pixels,uint32_t packedBytes)
{
    uint16_t planeBytes=packedBytes>>2;
    BtechEgaUnpackNibblesToScratchPlanes(pixels,planeBytes);
    uint16_t scratchCursor=0,pixelOffset=0;
    uint32_t paragraphBase=0;
    for (uint16_t image=0;image<list->imageCount;++image)
    {
        BtechBitmap *bitmap=&list->images[image];
        uint16_t imageBytes=(bitmap->widthPixels>>1)*bitmap->heightPixels;
        planeBytes=imageBytes>>2;
        uint16_t maskOffset=pixelOffset+(uint16_t)(planeBytes*COMBAT_EGA_PLANES);
        if (!planeBytes || (uint32_t)pixelOffset+planeBytes*COMBAT_EGA_PLANES>UINT16_MAX
            || paragraphBase+maskOffset+planeBytes>pixels.byteCount) abort(); // Zero DEC loops / per-image RAM wrap remain unconverted.
        bitmap->pixelPlanes=pixels.bytes+paragraphBase+pixelOffset;
        bitmap->preserveMask=pixels.bytes+paragraphBase+maskOffset;
        bitmap->pixelStorageBytes=pixels.byteCount-paragraphBase-pixelOffset;
        bitmap->maskStorageBytes=pixels.byteCount-paragraphBase-maskOffset;
        BtechEgaCopyScratchPlanes(scratchCursor,bitmap->pixelPlanes,planeBytes);
        BtechEgaBuildZeroColourMask(scratchCursor,bitmap->preserveMask,planeBytes);
        scratchCursor+=planeBytes;
        uint16_t nextOffset=maskOffset+planeBytes;
        paragraphBase+=(uint32_t)(nextOffset>>4)*0x10;
        pixelOffset=nextOffset&0x0F;
    }
    list->pixels=pixels; // Host ownership metadata; original first-image selector owns this one DOS block.
}

/*2A36:1F98. Actual packed-nibble expansion, original two WORD reads before
 * four plane writes. Preserve port sequence and source paragraph carry. */
void BtechEgaUnpackNibblesToScratchPlanes(BtechResourceAllocation source,uint16_t planeBytes)
{
    if (!btechHostWritePortWord || !btechHostWritePort || !btechHostWriteVideoByte) abort();
    btechHostWritePortWord(COMBAT_EGA_GRAPHICS_PORT,COMBAT_EGA_WRITE_MODE_TWO);
    btechHostWritePortWord(COMBAT_EGA_GRAPHICS_PORT,COMBAT_EGA_ALL_PIXEL_BITS);
    btechHostWritePortWord(COMBAT_EGA_GRAPHICS_PORT,COMBAT_EGA_WRITE_MODE_ZERO);
    btechHostWritePortWord(COMBAT_EGA_SEQUENCER_PORT,COMBAT_EGA_MAP_PLANE_ZERO);
    uint16_t sourceCursor=0,destinationCursor=0,groupsRemaining=planeBytes;
    uint32_t sourceParagraph=0;
    do
    {
        if (sourceParagraph+sourceCursor+4>source.byteCount) abort();
        uint16_t firstPixels=source.bytes[sourceParagraph+sourceCursor]|((uint16_t)source.bytes[sourceParagraph+sourceCursor+1]<<8);
        sourceCursor+=2;
        uint16_t secondPixels=source.bytes[sourceParagraph+sourceCursor]|((uint16_t)source.bytes[sourceParagraph+sourceCursor+1]<<8);
        uint8_t sourceWrapped=sourceCursor>=0xFFFE;
        sourceCursor+=2;
        uint8_t planePixels[COMBAT_EGA_PLANES]={0};
        for (uint16_t packedByte=0;packedByte<4;++packedByte)
        {
            uint8_t pixels=(packedByte<2?firstPixels:secondPixels)>>((packedByte&1)*8);
            for (uint16_t nibble=0;nibble<2;++nibble)
            {
                uint8_t colour=nibble==0?pixels>>4:pixels&0x0F;
                for (uint16_t plane=0;plane<COMBAT_EGA_PLANES;++plane)
                    planePixels[plane]=(uint8_t)((planePixels[plane]<<1)|((colour>>plane)&1));
            }
        }
        if (sourceWrapped) sourceParagraph+=0x10000;
        for (uint16_t plane=0;plane<COMBAT_EGA_PLANES;++plane)
        {
            btechHostWritePort(COMBAT_EGA_SEQUENCER_PORT+1,(uint8_t)(1<<plane));
            btechHostWriteVideoByte(COMBAT_EGA_BITMAP_SCRATCH_PAGE,destinationCursor,planePixels[plane]);
        }
        ++destinationCursor;
    } while (--groupsRemaining);
    btechHostWritePortWord(COMBAT_EGA_GRAPHICS_PORT,COMBAT_EGA_WRITE_MODE_TWO);
    btechHostWritePortWord(COMBAT_EGA_SEQUENCER_PORT,COMBAT_EGA_MAP_ALL_PLANES);
}

/*2A36:209B. Repeat the same scratch range per plane; zero copies nothing. */
void BtechEgaCopyScratchPlanes(uint16_t sourceOffset,uint8_t *destination,uint16_t planeBytes)
{
    if (!btechHostWritePortWord || !btechHostReadVideoByte) abort();
    for (uint16_t plane=0;plane<COMBAT_EGA_PLANES;++plane)
    {
        btechHostWritePortWord(COMBAT_EGA_GRAPHICS_PORT,(uint16_t)((plane<<8)|COMBAT_EGA_READ_MAP_REGISTER));
        uint16_t sourceCursor=sourceOffset;
        for (uint16_t remaining=planeBytes;remaining;--remaining)
            *destination++=btechHostReadVideoByte(COMBAT_EGA_BITMAP_SCRATCH_PAGE,sourceCursor++);
    }
}

/*2A36:20CE. Zero-colour mask is the inverse OR of the four scratch planes.
 * The original DEC loop and read-map3 residue remain visible. */
void BtechEgaBuildZeroColourMask(uint16_t sourceOffset,uint8_t *destination,uint16_t planeBytes)
{
    if (!btechHostWritePortWord || !btechHostWritePort || !btechHostReadVideoByte) abort();
    btechHostWritePortWord(COMBAT_EGA_GRAPHICS_PORT,COMBAT_EGA_READ_MAP_REGISTER);
    uint16_t sourceCursor=sourceOffset,remaining=planeBytes;
    do
    {
        uint8_t opaquePixels=0;
        for (uint16_t plane=0;plane<COMBAT_EGA_PLANES;++plane)
        {
            btechHostWritePort(COMBAT_EGA_GRAPHICS_PORT+1,(uint8_t)plane);
            opaquePixels|=btechHostReadVideoByte(COMBAT_EGA_BITMAP_SCRATCH_PAGE,sourceCursor);
        }
        *destination++=(uint8_t)~opaquePixels;
        ++sourceCursor;
    } while (--remaining);
}
