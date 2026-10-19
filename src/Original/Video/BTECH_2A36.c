#include "mechwarrior/combat_runtime.h"
#include "mechwarrior/combat_video.h"
#include "mechwarrior/combat_startup.h"
#include <stdlib.h>
#include <string.h>

/*2A36:1C3E. Even padded plane rows, full DX:AX product and DWORD wrap. */
uint32_t BtechEgaGetCaptureStorageSize(uint16_t width,uint16_t height)
{
    uint16_t rowBytes=(width>>3)+1+((width&7)!=0);
    rowBytes+=rowBytes&1;
    return (uint32_t)rowBytes*height*COMBAT_EGA_PLANES;
}

/*2A36:000C. Installer's REP MOVSW leavesCX=0. Other palette entry offsets
 * require recovery of their actual owners, not an address registry. */
void BtechInitializeEgaVideo(uint16_t paletteByteOffset)
{
    if (paletteByteOffset) abort();
    if (!btechHostSetVideoMode || !btechHostWritePortWord) abort();
    videoColourCount=COMBAT_EGA_COLOURS;
    frontPageSelector=COMBAT_EGA_FRONT_PAGE;
    backPageSelector=COMBAT_EGA_BACK_PAGE;
    videoScratchPageSelector=COMBAT_EGA_SCRATCH_PAGE;
    btechHostSetVideoMode(0x0D);
    uint16_t pattern=0;
    for (uint16_t colour=0;colour<COMBAT_EGA_COLOURS;++colour)
    {
        palettePrimarySlots[0][colour]=pattern;
        palettePrimarySlots[0][colour+COMBAT_EGA_COLOURS]=pattern;
        paletteSecondarySlots[0][colour]=pattern;
        paletteSecondarySlots[0][colour+COMBAT_EGA_COLOURS]=pattern;
        graphicsColourPatterns[colour]=pattern;
        pattern+=0x0101; /* One repeated colour BYTE in each WORD half. */
    }
    for (uint16_t row=0,offset=0;row<COMBAT_SCREEN_HEIGHT;++row,offset+=COMBAT_SCREEN_ROW_BYTES)
        graphicsRowOffsets[row]=offset;
    btechHostWritePortWord(COMBAT_EGA_SEQUENCER_PORT,COMBAT_EGA_MAP_ALL_PLANES);
    btechHostWritePortWord(COMBAT_EGA_GRAPHICS_PORT,COMBAT_EGA_WRITE_MODE_TWO);
}

/*2A36:0343, reached through02C2's CLD. BYTE counters, native wrap, character
 * rejection, transparency and inherited raster operation remain original.
 * Direct DF-set calls, invalid row/font aliases are not converted here. */
void BtechEgaDrawGlyph(uint16_t character,uint16_t x,uint16_t y,uint16_t page)
{
    if (y>=COMBAT_SCREEN_HEIGHT) abort();
    if (!btechHostWritePortWord || !btechHostReadVideoByte || !btechHostWriteVideoByte) abort();
    // FAR AX/DX/BP entry, ambient ES page; DX/SI/BP/DS preserved. DI is scratch.
    // No clipping here; the shared text caller owns clipping decisions.
    uint8_t width=fontAdvanceWidths[0x0000];
    uint8_t byteAligned=(x&0x0007)==0x0000&&(width&0x07)==0x00;
    uint8_t bytesPerRow=(uint8_t)((width>>0x03)+(!byteAligned&&(width&0x07)!=0x00));
    uint8_t shift=(uint8_t)(x&0x0007);
    uint16_t cursor=0x0000;
    if (!byteAligned)
    {
        cursor=(uint16_t)(graphicsRowOffsets[y]+(x>>0x0003));
        glyphTailMask=glyphTailMasks[width&0x07];
    } // Native scratch write precedes character rejection on general path.
    uint8_t rowsRemaining=combatFontHeights[0x0000];
    uint8_t glyphStride=(uint8_t)(rowsRemaining*bytesPerRow); // MUL BYTE then MUL BL: high product BYTE discarded.
    uint8_t characterByte=(uint8_t)character,firstCharacter=fontFirstCharacters[0x0000];
    uint8_t relativeCharacter=(uint8_t)(characterByte-firstCharacter);
    if ((int8_t)characterByte<(int8_t)firstCharacter)return; // SUB/JL includes overflow, not just relative sign.
    uint8_t characterCount=fontCharacterCounts[0x0000];
    if (byteAligned?relativeCharacter>=characterCount:(int8_t)relativeCharacter>=(int8_t)characterCount)return;
    uint16_t glyphCursor=(uint16_t)(relativeCharacter*glyphStride);
    if (byteAligned)
        cursor=(uint16_t)(graphicsRowOffsets[y]+(x>>0x0003));
    uint8_t foreground=textForegroundPattern,background=textBackgroundPattern;
    uint8_t transparent=textTransparentBackground!=0x00;
    BtechFont *font=&fonts[0];
    if (byteAligned)
    {
        do
        {
            uint8_t bytesRemaining=bytesPerRow;
            do
            {
                if (!font->bytes || glyphCursor>=font->byteCount) abort();
                uint8_t glyphBits=font->bytes[glyphCursor];
                glyphCursor++;
                btechHostWritePortWord(COMBAT_EGA_GRAPHICS_PORT,(uint16_t)((uint16_t)glyphBits<<0x0008)|COMBAT_EGA_BIT_MASK_REGISTER);
                btechHostReadVideoByte(page,cursor); // Required latch read, even for mask0.
                btechHostWriteVideoByte(page,cursor,foreground);
                if (!transparent)
                {
                    btechHostWritePortWord(COMBAT_EGA_GRAPHICS_PORT,(uint16_t)((uint16_t)(uint8_t)~glyphBits<<0x0008)|COMBAT_EGA_BIT_MASK_REGISTER);
                    btechHostReadVideoByte(page,cursor);
                    btechHostWriteVideoByte(page,cursor,background);
                }
                cursor++;
            } while (--bytesRemaining!=0x00); // BYTE0 means256 copies.
            cursor=(uint16_t)(cursor-bytesPerRow);
            cursor=(uint16_t)(cursor+COMBAT_SCREEN_ROW_BYTES);
        } while (--rowsRemaining!=0x00); // Height0 means256 rows.
        return;
    }
    do
    {
        if (shift==0x00)
        {
            uint8_t bytesRemaining=bytesPerRow;
            if (transparent)
            {
                do
                {
                    if (!font->bytes || glyphCursor>=font->byteCount) abort();
                    uint8_t glyphBits=font->bytes[glyphCursor];
                    glyphCursor++;
                    btechHostWritePortWord(COMBAT_EGA_GRAPHICS_PORT,(uint16_t)((uint16_t)glyphBits<<0x0008)|COMBAT_EGA_BIT_MASK_REGISTER);
                    btechHostReadVideoByte(page,cursor);
                    btechHostWriteVideoByte(page,cursor,foreground); // Cached once in this path.
                    cursor++;
                } while (--bytesRemaining!=0x00);
            }
            else
            {
                while (--bytesRemaining!=0x00)
                {
                    if (!font->bytes || glyphCursor>=font->byteCount) abort();
                    uint8_t glyphBits=font->bytes[glyphCursor];
                    glyphCursor++;
                    btechHostWritePortWord(COMBAT_EGA_GRAPHICS_PORT,(uint16_t)((uint16_t)glyphBits<<0x0008)|COMBAT_EGA_BIT_MASK_REGISTER);
                    btechHostReadVideoByte(page,cursor);
                    btechHostWriteVideoByte(page,cursor,textForegroundPattern);
                    btechHostWritePortWord(COMBAT_EGA_GRAPHICS_PORT,(uint16_t)((uint16_t)(uint8_t)~glyphBits<<0x0008)|COMBAT_EGA_BIT_MASK_REGISTER);
                    btechHostReadVideoByte(page,cursor);
                    btechHostWriteVideoByte(page,cursor,textBackgroundPattern);
                    cursor++;
                }
                if (!font->bytes || glyphCursor>=font->byteCount) abort();
                uint8_t glyphBits=font->bytes[glyphCursor];
                glyphCursor++;
                btechHostWritePortWord(COMBAT_EGA_GRAPHICS_PORT,(uint16_t)((uint16_t)glyphBits<<0x0008)|COMBAT_EGA_BIT_MASK_REGISTER);
                btechHostReadVideoByte(page,cursor);
                btechHostWriteVideoByte(page,cursor,textForegroundPattern);
                uint8_t backgroundMask=(uint8_t)~glyphBits&glyphTailMask;
                btechHostWritePortWord(COMBAT_EGA_GRAPHICS_PORT,(uint16_t)((uint16_t)backgroundMask<<0x0008)|COMBAT_EGA_BIT_MASK_REGISTER);
                btechHostReadVideoByte(page,cursor);
                btechHostWriteVideoByte(page,cursor,textBackgroundPattern);
                cursor++;
            }
        }
        else if (transparent)
        {
            uint8_t previousBits=0x00;
            uint8_t bytesRemaining=(uint8_t)(bytesPerRow+0x01);
            while (--bytesRemaining!=0x00)
            {
                if (!font->bytes || glyphCursor>=font->byteCount) abort();
                uint8_t glyphBits=font->bytes[glyphCursor];
                glyphCursor++;
                uint16_t joinedBits=(uint16_t)((uint16_t)previousBits<<0x0008)|glyphBits;
                previousBits=glyphBits;
                uint8_t shiftedBits=(uint8_t)(joinedBits>>shift);
                btechHostWritePortWord(COMBAT_EGA_GRAPHICS_PORT,(uint16_t)((uint16_t)shiftedBits<<0x0008)|COMBAT_EGA_BIT_MASK_REGISTER);
                btechHostReadVideoByte(page,cursor);
                btechHostWriteVideoByte(page,cursor,textForegroundPattern);
                cursor++;
            }
            uint16_t joinedBits=(uint16_t)previousBits<<0x0008;
            uint8_t shiftedBits=(uint8_t)(joinedBits>>shift);
            btechHostWritePortWord(COMBAT_EGA_GRAPHICS_PORT,(uint16_t)((uint16_t)shiftedBits<<0x0008)|COMBAT_EGA_BIT_MASK_REGISTER);
            btechHostReadVideoByte(page,cursor);
            btechHostWriteVideoByte(page,cursor,textForegroundPattern);
            cursor++;
        }
        else
        {
            uint8_t previousBits=0x00,previousBackground=0x00;
            uint8_t bytesRemaining=bytesPerRow;
            while (--bytesRemaining!=0x00)
            {
                if (!font->bytes || glyphCursor>=font->byteCount) abort();
                uint8_t glyphBits=font->bytes[glyphCursor];
                glyphCursor++;
                uint8_t foregroundMask=(uint8_t)(((uint16_t)((uint16_t)previousBits<<0x0008)|glyphBits)>>shift);
                previousBits=glyphBits;
                btechHostWritePortWord(COMBAT_EGA_GRAPHICS_PORT,(uint16_t)((uint16_t)foregroundMask<<0x0008)|COMBAT_EGA_BIT_MASK_REGISTER);
                btechHostReadVideoByte(page,cursor);
                btechHostWriteVideoByte(page,cursor,textForegroundPattern);
                uint8_t invertedBits=(uint8_t)~previousBits;
                uint8_t backgroundMask=(uint8_t)(((uint16_t)((uint16_t)previousBackground<<0x0008)|invertedBits)>>shift);
                previousBackground=invertedBits;
                btechHostWritePortWord(COMBAT_EGA_GRAPHICS_PORT,(uint16_t)((uint16_t)backgroundMask<<0x0008)|COMBAT_EGA_BIT_MASK_REGISTER);
                btechHostReadVideoByte(page,cursor);
                btechHostWriteVideoByte(page,cursor,textBackgroundPattern);
                cursor++;
            }
            if (!font->bytes || glyphCursor>=font->byteCount) abort();
            uint8_t glyphBits=font->bytes[glyphCursor];
            glyphCursor++;
            uint8_t foregroundMask=(uint8_t)(((uint16_t)((uint16_t)previousBits<<0x0008)|glyphBits)>>shift);
            previousBits=glyphBits;
            btechHostWritePortWord(COMBAT_EGA_GRAPHICS_PORT,(uint16_t)((uint16_t)foregroundMask<<0x0008)|COMBAT_EGA_BIT_MASK_REGISTER);
            btechHostReadVideoByte(page,cursor);
            btechHostWriteVideoByte(page,cursor,textForegroundPattern);
            uint8_t invertedBits=(uint8_t)~previousBits&glyphTailMask;
            uint8_t backgroundMask=(uint8_t)(((uint16_t)((uint16_t)previousBackground<<0x0008)|invertedBits)>>shift);
            previousBackground=invertedBits;
            btechHostWritePortWord(COMBAT_EGA_GRAPHICS_PORT,(uint16_t)((uint16_t)backgroundMask<<0x0008)|COMBAT_EGA_BIT_MASK_REGISTER);
            btechHostReadVideoByte(page,cursor);
            btechHostWriteVideoByte(page,cursor,textBackgroundPattern);
            cursor++;
            foregroundMask=(uint8_t)((uint16_t)((uint16_t)previousBits<<0x0008)>>shift);
            btechHostWritePortWord(COMBAT_EGA_GRAPHICS_PORT,(uint16_t)((uint16_t)foregroundMask<<0x0008)|COMBAT_EGA_BIT_MASK_REGISTER);
            btechHostReadVideoByte(page,cursor);
            btechHostWriteVideoByte(page,cursor,textForegroundPattern);
            backgroundMask=(uint8_t)((uint16_t)((uint16_t)previousBackground<<0x0008)>>shift);
            btechHostWritePortWord(COMBAT_EGA_GRAPHICS_PORT,(uint16_t)((uint16_t)backgroundMask<<0x0008)|COMBAT_EGA_BIT_MASK_REGISTER);
            btechHostReadVideoByte(page,cursor);
            btechHostWriteVideoByte(page,cursor,textBackgroundPattern);
            // Extra opaque tail uses MOV, not STOSB: no cursor increment.
        }
        uint16_t rowBytes=bytesPerRow;
        if (shift!=0x00&&transparent)++rowBytes;
        cursor=(uint16_t)(cursor-rowBytes);
        cursor=(uint16_t)(cursor+COMBAT_SCREEN_ROW_BYTES);
    } while (--rowsRemaining!=0x00);
    // EGA leaves the final bitmask in place; it does not restore FF or change write mode.
}

/*2A36:2111. Normal forward startup path. Allocation is DOS48 here, not the
 *1EB6 largest-free-block query contract. Native slot/scratch stores precede
 * DIV BL and the do/decrement loop; do not replace it with a whole-file read. */
uint16_t BtechEgaLoadFont(BtechRuntimeStream *stream,uint16_t slot,uint8_t *scratch)
{
    if (slot>=COMBAT_FONT_SLOTS || !stream || !btechHostAllocateParagraphs) abort();
    uint8_t bytesPerRow=(uint8_t)((fontAdvanceWidths[slot]+7)/8);
    uint16_t bytes=(uint16_t)((uint8_t)(combatFontHeights[slot]*bytesPerRow)*fontCharacterCounts[slot]);
    egaFontChunkCounter=bytes;
    uint16_t paragraphs=(uint16_t)((bytes>>4)+((bytes&0x0F)!=0)),largestFreeParagraphs;
    uint8_t *allocation=btechHostAllocateParagraphs(paragraphs,&largestFreeParagraphs);
    if (!allocation) return 0;
    fonts[slot]=(BtechFont){allocation,(size_t)paragraphs*0x10};
    egaFontBuffer=allocation;
    if (bytes/COMBAT_FONT_READ_BYTES>UINT8_MAX) abort(); // Native DIV BL fault after allocation stores.
    egaFontChunkCounter=(uint16_t)((bytes/COMBAT_FONT_READ_BYTES)|((bytes%COMBAT_FONT_READ_BYTES)<<8));
    uint16_t cursor=0;
    do
    {
        size_t read=BtechReadStreamElements(scratch,COMBAT_FONT_READ_BYTES,1,stream->file);
        if (!cursor && read!=1) abort(); // First short block can expose unknown inherited stack bytes.
        if ((uint32_t)cursor+COMBAT_FONT_READ_BYTES>fonts[slot].byteCount) abort();
        memcpy(egaFontBuffer+cursor,scratch,COMBAT_FONT_READ_BYTES);
        cursor+=COMBAT_FONT_READ_BYTES;
        egaFontChunkCounter=(uint16_t)((egaFontChunkCounter&0xFF00)
            |(uint8_t)(egaFontChunkCounter-1));
    } while ((uint8_t)egaFontChunkCounter);
    uint16_t tail=egaFontChunkCounter>>8;
    BtechReadStreamElements(scratch,tail,1,stream->file); // Native ignores short counts; retains previous staging bytes.
    if ((uint32_t)cursor+tail>fonts[slot].byteCount) abort();
    memcpy(egaFontBuffer+cursor,scratch,tail);
    return slot;
}

/*2A36:21BF. Actual tagged palette bytes become typed pattern WORDs once at
 * the binary boundary. Truncated first reads expose unknown native stack
 * residue and stay gated, rather than acquiring a fabricated black tail. */
uint16_t BtechEgaLoadPalette(BtechRuntimeStream *stream,uint16_t slot)
{
    if (slot>=COMBAT_PALETTE_SLOTS || !stream) abort();
    uint8_t encoded[COMBAT_PALETTE_FILE_BYTES];
    if (BtechReadStreamElements(encoded,1,sizeof(encoded),stream->file)!=sizeof(encoded)) abort();
    uint16_t cursor=0;
    for (uint16_t colour=0;colour<COMBAT_PALETTE_WORDS;++colour,cursor+=2)
        palettePrimarySlots[slot][colour]=(encoded[cursor]&0x0F)|((uint16_t)(encoded[cursor+1]>>4)<<8);
    for (uint16_t colour=0;colour<COMBAT_PALETTE_WORDS;++colour,cursor+=2)
        paletteSecondarySlots[slot][colour]=(encoded[cursor]&0x0F)|((uint16_t)(encoded[cursor+1]>>4)<<8);
    return slot;
}

/*2A36:222D. Pattern publication and individual BIOS palette writes remain
 * interleaved per colour. Background patterns come from each bank's second
 * half; EGA bright colours skip hardware entries08..0F. */
void BtechEgaSelectPalette(uint16_t slot)
{
    if (!slot) return;
    if (slot>=COMBAT_PALETTE_SLOTS || !btechHostSetPaletteRegister) abort();
    for (uint16_t colour=0;colour<COMBAT_EGA_COLOURS;++colour)
    {
        palettePrimarySlots[0][colour]=colour;
        paletteSecondarySlots[0][colour]=colour;
        graphicsColourPatterns[colour]=colour;
        palettePrimarySlots[0][colour+COMBAT_EGA_COLOURS]=palettePrimarySlots[slot][colour+COMBAT_EGA_COLOURS];
        paletteSecondarySlots[0][colour+COMBAT_EGA_COLOURS]=paletteSecondarySlots[slot][colour+COMBAT_EGA_COLOURS];
        uint8_t hardwareColour=(uint8_t)(palettePrimarySlots[slot][colour]&0x0F);
        if (hardwareColour>=8) hardwareColour+=8;
        btechHostSetPaletteRegister((uint8_t)colour,hardwareColour);
    }
}
