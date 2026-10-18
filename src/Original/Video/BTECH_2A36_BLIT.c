#include "mechwarrior/combat_video.h"
#include <stdlib.h>

/*2A36:083F. Eight native shift continuations share this one FAR boundary.
 * Descriptor snapshots precede clipping. The mask clears ALL enabled hardware
 * planes before the live2E5F-selected colour passes OR their ink. DF-clear and
 * ordinary RAM payload owners only; no invented shifted-blit helper. */
void BtechEgaDrawBitmap(const BtechBitmap *bitmap,uint16_t x,uint16_t y)
{
    if (!bitmap) abort();
    const BtechBitmap image=*bitmap;
    uint16_t page=drawPageSelector,rows=image.heightPixels;
    uint16_t rowStart,sourceSkip=0;
    uint8_t rowBytes;
    if (!screenPolygonNeedsClip)
    {
        if (y>=COMBAT_SCREEN_HEIGHT) abort();
        rowStart=(uint16_t)(graphicsRowOffsets[y]+(x>>3));
        egaBitmapShiftPhase=x&7;
        rowBytes=(uint8_t)(image.widthPixels>>3);
        egaBitmapPlaneStride=(uint16_t)(rowBytes*(uint8_t)rows);
        egaBitmapSourceRowSkip=0;egaBitmapClipSides=0;
        egaBitmapDestinationRowSkip=(uint8_t)(COMBAT_SCREEN_ROW_BYTES-rowBytes);
    }
    else
    {
        uint16_t packedWidth=image.widthPixels>>3;
        egaBitmapPlaneStride=(uint16_t)((uint8_t)packedWidth*(uint8_t)rows);
        uint16_t rowY=y;
        if ((int16_t)rowY<clipMinimumY)
        {
            uint16_t skippedRows=(uint16_t)(clipMinimumY-rowY);
            if ((int16_t)rows<=(int16_t)skippedRows) return;
            rows-=skippedRows;sourceSkip=(uint16_t)((uint8_t)skippedRows*(uint8_t)packedWidth);
            rowY=(uint16_t)clipMinimumY;
        }
        if ((uint16_t)(rowY+rows)>(uint16_t)clipMaximumY)
        {
            rows=(uint16_t)(clipMaximumY-rowY+1);
            if (!rows || rows>0x8000) return; /* INC/JLE admits8000. */
        }
        if (rowY>=COMBAT_SCREEN_HEIGHT) abort();
        rowStart=graphicsRowOffsets[rowY];
        uint16_t clippedWidth=(uint16_t)(packedWidth<<3),leftX=x;
        uint16_t minimumX=(uint16_t)clipMinimumX&0xFFF8;
        if ((int16_t)leftX<(int16_t)minimumX)
        {
            uint16_t skippedPixels=(uint16_t)(minimumX-leftX);
            if ((int16_t)clippedWidth<=(int16_t)skippedPixels) return;
            clippedWidth-=skippedPixels;egaBitmapShiftPhase=clippedWidth&7;
            sourceSkip+=(uint16_t)((uint16_t)((clippedWidth&7)+skippedPixels)>>3);
            leftX=(uint16_t)clipMinimumX&0xFFF8;egaBitmapClipSides=1;
        }
        else {egaBitmapShiftPhase=leftX&7;egaBitmapClipSides=0;}
        rowStart+=(leftX>>3);
        egaBitmapRoundedEndX=(uint16_t)(clipMaximumX+1)&0xFFF8;
        if ((uint16_t)(leftX+clippedWidth)>=egaBitmapRoundedEndX)
        {
            if ((int16_t)egaBitmapRoundedEndX<=(int16_t)leftX) return;
            clippedWidth=(uint16_t)(egaBitmapRoundedEndX-leftX);egaBitmapClipSides|=2;
        }
        rowBytes=(uint8_t)(clippedWidth>>3);
        if (egaBitmapClipSides==3 && (x&7)) --rowBytes;
        uint16_t originalRowBytes=image.widthPixels>>3;
        egaBitmapSourceRowSkip=(originalRowBytes&0xFF00)|(uint8_t)((uint8_t)originalRowBytes-rowBytes);
        egaBitmapDestinationRowSkip=(uint8_t)(COMBAT_SCREEN_ROW_BYTES-rowBytes);
    }
    uint8_t phase=(uint8_t)egaBitmapShiftPhase;
    uint16_t maskCursor=sourceSkip,destination=rowStart;
    uint8_t rowsRemaining=(uint8_t)rows;
    if (!btechHostWritePortWord || !btechHostReadVideoByte || !btechHostWriteVideoByte) abort();
    do
    {
        uint8_t bytesRemaining=rowBytes;
        if (!phase)
        {
            do
            {
                if (maskCursor>=image.maskStorageBytes) abort();
                uint8_t preserve=image.preserveMask[maskCursor++];
                btechHostWritePortWord(COMBAT_EGA_GRAPHICS_PORT,COMBAT_EGA_BIT_MASK_REGISTER|((uint16_t)(uint8_t)~preserve<<8));
                (void)btechHostReadVideoByte(page,destination);btechHostWriteVideoByte(page,destination++,0);
            } while (--bytesRemaining);
        }
        else
        {
            uint8_t previous=0xFF;
            if (egaBitmapClipSides&1)
            {
                uint16_t prior=(uint16_t)(maskCursor-1);if (prior>=image.maskStorageBytes) abort();
                previous=image.preserveMask[prior];
            }
            while ((int8_t)bytesRemaining>0)
            {
                --bytesRemaining;
                if (maskCursor>=image.maskStorageBytes) abort();
                uint8_t current=image.preserveMask[maskCursor++];
                uint8_t preserve=(uint8_t)((previous<<(8-phase))|(current>>phase));previous=current;
                btechHostWritePortWord(COMBAT_EGA_GRAPHICS_PORT,COMBAT_EGA_BIT_MASK_REGISTER|((uint16_t)(uint8_t)~preserve<<8));
                (void)btechHostReadVideoByte(page,destination);btechHostWriteVideoByte(page,destination++,0);
            }
            uint8_t current=0xFF;
            if (egaBitmapClipSides&2)
            {if (maskCursor>=image.maskStorageBytes) abort();current=image.preserveMask[maskCursor];}
            uint8_t preserve=(uint8_t)((previous<<(8-phase))|(current>>phase));
            btechHostWritePortWord(COMBAT_EGA_GRAPHICS_PORT,COMBAT_EGA_BIT_MASK_REGISTER|((uint16_t)(uint8_t)~preserve<<8));
            (void)btechHostReadVideoByte(page,destination);btechHostWriteVideoByte(page,destination,0);
        }
        if (!--rowsRemaining) break;
        maskCursor+=egaBitmapSourceRowSkip;destination+=egaBitmapDestinationRowSkip;
    } while (1);
    btechHostWritePortWord(COMBAT_EGA_GRAPHICS_PORT,COMBAT_EGA_WRITE_MODE_ZERO);
    btechHostWritePortWord(COMBAT_EGA_GRAPHICS_PORT,COMBAT_EGA_ALL_PIXEL_BITS);
    uint8_t clipSides=egaBitmapClipSides;
    uint16_t planeStart=sourceSkip;
    for (uint16_t plane=0;plane<COMBAT_EGA_PLANES;++plane)
    {
        if (plane) planeStart+=egaBitmapPlaneStride;
        if (!(egaBitmapEnabledPlanes&(1<<plane))) continue;
        btechHostWritePortWord(COMBAT_EGA_SEQUENCER_PORT,COMBAT_EGA_SEQUENCER_MAP_MASK|((uint16_t)(1<<plane)<<8));
        btechHostWritePortWord(COMBAT_EGA_GRAPHICS_PORT,COMBAT_EGA_READ_MAP_REGISTER|(plane<<8));
        uint16_t cursor=planeStart;destination=rowStart;rowsRemaining=(uint8_t)rows;
        do
        {
            uint8_t previous=0;
            if (phase && (clipSides&1))
            {
                uint16_t prior=(uint16_t)(cursor-1);if (prior>=image.pixelStorageBytes) abort();
                previous=image.pixelPlanes[prior];
            }
            uint16_t count=!phase?(rowBytes?rowBytes:0x100):((int8_t)rowBytes>0?rowBytes:0);
            while (count--)
            {
                if (cursor>=image.pixelStorageBytes) abort();
                uint8_t pixels=image.pixelPlanes[cursor++];
                uint8_t ink=phase?(uint8_t)((previous<<(8-phase))|(pixels>>phase)):pixels;previous=pixels;
                uint8_t background=btechHostReadVideoByte(page,destination);
                btechHostWriteVideoByte(page,destination++,(uint8_t)(background|ink));
            }
            if (phase)
            {
                uint8_t pixels=0;
                if (clipSides&2) {if (cursor>=image.pixelStorageBytes) abort();pixels=image.pixelPlanes[cursor];}
                uint8_t background=btechHostReadVideoByte(page,destination);
                btechHostWriteVideoByte(page,destination,(uint8_t)(background|(previous<<(8-phase))|(pixels>>phase)));
            }
            if (!--rowsRemaining) break;
            cursor+=egaBitmapSourceRowSkip;destination+=egaBitmapDestinationRowSkip;
        } while (1);
    }
    btechHostWritePortWord(COMBAT_EGA_SEQUENCER_PORT,COMBAT_EGA_MAP_ALL_PLANES);
    btechHostWritePortWord(COMBAT_EGA_GRAPHICS_PORT,COMBAT_EGA_WRITE_MODE_TWO);
}
