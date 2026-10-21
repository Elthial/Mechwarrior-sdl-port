#include "mechwarrior/port_diagnostics.h"
#include "mechwarrior/combat_video.h"
#include <stdlib.h>

/*2A36:0608. Native point/horizontal/vertical/diagonal/run paths. No new
 * clipping, raster-state reset or Bresenham replacement. */
void BtechEgaDrawLine(uint16_t firstX,uint16_t firstY,uint16_t secondX,uint16_t secondY,uint16_t page)
{
    if (!btechHostWritePortWord || !btechHostReadVideoByte || !btechHostWriteVideoByte) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !btechHostWritePortWord || !btechHostReadVideoByte || !btechHostWriteVideoByte");
    // Coordinates are native BX/CX/DX/SI; page denotes AMBIENT ES,
    // not a fifth pushed argument. DS=304E; all general registers are scratch.
    drawColourPattern=palettePrimarySlots[drawFillColour/COMBAT_PALETTE_WORDS][drawFillColour%COMBAT_PALETTE_WORDS];
    egaLinePixelMask=egaPixelBitMasks[firstX&0x0007];
    uint16_t row=(uint16_t)(firstY*2)>>1;
    if (row>=COMBAT_SCREEN_HEIGHT) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: row>=COMBAT_SCREEN_HEIGHT"); /* Other row-table owners are outside this caller scope. */
    uint16_t cursor=graphicsRowOffsets[row];
    cursor += (firstX>>0x0003);
    uint8_t mask=egaLinePixelMask;
    if (firstX==secondX&&firstY==secondY)
    {
        (void)btechHostReadVideoByte(page,cursor);
        btechHostWritePortWord(COMBAT_EGA_GRAPHICS_PORT,COMBAT_EGA_BIT_MASK_REGISTER|((uint16_t)mask<<0x0008));
        btechHostWriteVideoByte(page,cursor,(uint8_t)drawColourPattern); // Pattern reread after OUT.
        return;
    }
    uint8_t colour=drawColourPattern;
    uint16_t horizontalExtent=secondX-firstX;
    if (firstY==secondY)
    {
        //065E horizontal path: initial pixel then exactly DX rotations/writes.
        uint16_t pixelsRemaining=horizontalExtent;
        (void)btechHostReadVideoByte(page,cursor);
        btechHostWritePortWord(COMBAT_EGA_GRAPHICS_PORT,COMBAT_EGA_BIT_MASK_REGISTER|((uint16_t)mask<<0x0008));
        btechHostWriteVideoByte(page,cursor,colour);
        do
        {
            uint16_t crossedByte=mask&0x01;
            mask=(uint8_t)((mask>>0x01)|(mask<<0x07)); // ROR, not SHR to zero.
            if (crossedByte)++cursor;
            (void)btechHostReadVideoByte(page,cursor);
            btechHostWritePortWord(COMBAT_EGA_GRAPHICS_PORT,COMBAT_EGA_BIT_MASK_REGISTER|((uint16_t)mask<<0x0008));
            btechHostWriteVideoByte(page,cursor,colour);
        } while (--pixelsRemaining!=0x0000);
        return;
    }
    uint16_t verticalExtent=(uint16_t)(secondY-firstY),rowStep=COMBAT_SCREEN_ROW_BYTES;
    if ((int16_t)secondY<=(int16_t)firstY)
    {
        verticalExtent=(uint16_t)(0x0000-verticalExtent);
        rowStep=(uint16_t)(0-COMBAT_SCREEN_ROW_BYTES);
    } // SUB SI,CX / JG tests signed OPERANDS, including OF; NEG may still yield8000.
    if (firstX==secondX)
    {
        uint16_t pixelsRemaining=verticalExtent;
        btechHostWritePortWord(COMBAT_EGA_GRAPHICS_PORT,COMBAT_EGA_BIT_MASK_REGISTER|((uint16_t)mask<<0x0008));
        do
        {
            (void)btechHostReadVideoByte(page,cursor);
            btechHostWriteVideoByte(page,cursor,colour);
            cursor += rowStep;
            --pixelsRemaining;
        } while ((pixelsRemaining&0x8000)==0x0000); // DEC CX/JS, NOT signed <=0.
        return;
    }
    if (verticalExtent==horizontalExtent)
    {
        uint16_t pixelsRemaining=horizontalExtent;
        (void)btechHostReadVideoByte(page,cursor);
        btechHostWritePortWord(COMBAT_EGA_GRAPHICS_PORT,COMBAT_EGA_BIT_MASK_REGISTER|((uint16_t)mask<<0x0008));
        btechHostWriteVideoByte(page,cursor,colour);
        do
        {
            uint16_t crossedByte=mask&0x01;
            mask=(uint8_t)((mask>>0x01)|(mask<<0x07));
            if (crossedByte)++cursor;
            (void)btechHostReadVideoByte(page,cursor);
            btechHostWritePortWord(COMBAT_EGA_GRAPHICS_PORT,COMBAT_EGA_BIT_MASK_REGISTER|((uint16_t)mask<<0x0008));
            btechHostWriteVideoByte(page,cursor,colour);
            // The two native branches order their final decrement differently.
            // Byte-wrap branch can finish without its otherwise matching Y step.
            if (crossedByte&&--pixelsRemaining==0x0000)break;
            cursor += rowStep;
            (void)btechHostReadVideoByte(page,cursor);
            btechHostWriteVideoByte(page,cursor,colour);
            if (!crossedByte&&--pixelsRemaining==0x0000)break;
        } while (1);
        return;
    }
    uint16_t steep=verticalExtent>horizontalExtent; // Unsigned CMP/JB/JZ.
    uint16_t remainingMajorPixels=steep?verticalExtent:horizontalExtent;
    uint16_t divisor=(steep?horizontalExtent:verticalExtent)+0x0001;
    // Major>minor proves divisor nonzero and both DIV results fit WORD here.
    egaLineRunLength=remainingMajorPixels/divisor;
    uint16_t remainder=remainingMajorPixels%divisor;
    egaLineFractionStep=(uint16_t)(((uint32_t)remainder<<0x0010)/divisor);
    egaLineFractionAccumulator=0x0000;
    uint16_t runPixels=egaLineRunLength+0x0001;
    (void)btechHostReadVideoByte(page,cursor);
    btechHostWritePortWord(COMBAT_EGA_GRAPHICS_PORT,COMBAT_EGA_BIT_MASK_REGISTER|((uint16_t)mask<<0x0008));
    btechHostWriteVideoByte(page,cursor,colour);
    do
    {
        egaLineRemainingBeforeRun=remainingMajorPixels;
        // SUB SI,CX / JGE compares the signed operands, not the wrapped result.
        if ((int16_t)remainingMajorPixels<(int16_t)runPixels)
        {
            runPixels=egaLineRemainingBeforeRun;
            remainingMajorPixels=0x0000;
        }
        else remainingMajorPixels -= runPixels;
        do
        {
            if (steep)cursor += rowStep;
            else
            {
                uint16_t crossedByte=mask&0x01;
                mask=(uint8_t)((mask>>0x01)|(mask<<0x07));
                if (crossedByte)++cursor;
            }
            (void)btechHostReadVideoByte(page,cursor);
            btechHostWritePortWord(COMBAT_EGA_GRAPHICS_PORT,COMBAT_EGA_BIT_MASK_REGISTER|((uint16_t)mask<<0x0008));
            btechHostWriteVideoByte(page,cursor,colour);
        } while (--runPixels!=0x0000); // Native zero run would execute00010000 times.
        if (remainingMajorPixels==0x0000)break;
        if (steep)
        {
            uint16_t crossedByte=mask&0x01;
            mask=(uint8_t)((mask>>0x01)|(mask<<0x07));
            if (crossedByte)++cursor;
            (void)btechHostReadVideoByte(page,cursor);
            btechHostWritePortWord(COMBAT_EGA_GRAPHICS_PORT,COMBAT_EGA_BIT_MASK_REGISTER|((uint16_t)mask<<0x0008));
        }
        else
        {
            cursor += rowStep;
            (void)btechHostReadVideoByte(page,cursor); // Shallow Y step reuses current bit mask, no OUT.
        }
        btechHostWriteVideoByte(page,cursor,colour);
        uint16_t fractionStep=egaLineFractionStep;
        uint16_t oldFraction=egaLineFractionAccumulator;
        egaLineFractionAccumulator=(uint16_t)(oldFraction+fractionStep);
        runPixels=egaLineRunLength;
        if ((uint32_t)oldFraction+fractionStep>0x0000FFFF)++runPixels;
    } while (1);
    // No CLD, mode/map-mask setup or restoration; last programmed bit mask remains.
}
