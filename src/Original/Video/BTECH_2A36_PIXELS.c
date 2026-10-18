#include "mechwarrior/combat_video.h"
#include <stdlib.h>

/*2A36:228B. BX byte offset and CL bit shift. Output pointer only transports
 * that register; it is not a third native stack argument. */
uint16_t BtechEgaPixelByteOffset(uint16_t x,uint16_t y,uint8_t *shift)
{
    *shift=(uint8_t)(7^(x&7));
    return (uint16_t)((x>>3)+(uint16_t)(y*COMBAT_SCREEN_ROW_BYTES));
}

/*2A36:22F1. Ordered latch/controller operations around one mode2 pixel. */
void BtechEgaWritePixel(uint16_t x,uint16_t y,uint16_t colour)
{
    if (!btechHostWritePortWord || !btechHostReadVideoByte || !btechHostWriteVideoByte) abort();
    uint16_t page=drawPageSelector;uint8_t shift;
    uint16_t offset=BtechEgaPixelByteOffset(x,y,&shift);
    btechHostWritePortWord(COMBAT_EGA_GRAPHICS_PORT,COMBAT_EGA_BIT_MASK_REGISTER|((uint16_t)(1<<shift)<<8));
    btechHostWritePortWord(COMBAT_EGA_GRAPHICS_PORT,COMBAT_EGA_WRITE_MODE_TWO);
    btechHostWritePortWord(COMBAT_EGA_GRAPHICS_PORT,COMBAT_EGA_DATA_ROTATE_COPY);
    (void)btechHostReadVideoByte(page,offset);
    btechHostWriteVideoByte(page,offset,(uint8_t)colour);
    btechHostWritePortWord(COMBAT_EGA_GRAPHICS_PORT,COMBAT_EGA_ALL_PIXEL_BITS);
    btechHostWritePortWord(COMBAT_EGA_GRAPHICS_PORT,COMBAT_EGA_WRITE_MODE_ZERO);
    btechHostWritePortWord(COMBAT_EGA_GRAPHICS_PORT,COMBAT_EGA_DATA_ROTATE_COPY);
    btechHostWritePortWord(COMBAT_EGA_SEQUENCER_PORT,COMBAT_EGA_MAP_ALL_PLANES);
    btechHostWritePortWord(COMBAT_EGA_GRAPHICS_PORT,COMBAT_EGA_WRITE_MODE_TWO);
}

/*2A36:22A8. Read-map order3..0 forms the colour one bit at a time. */
uint16_t BtechEgaReadPixel(uint16_t x,uint16_t y)
{
    if (!btechHostWritePortWord || !btechHostReadVideoByte) abort();
    uint16_t page=drawPageSelector;uint8_t shift;
    uint16_t offset=BtechEgaPixelByteOffset(x,y,&shift);uint8_t colour=0;
    for (int16_t plane=COMBAT_EGA_PLANES-1;plane>=0;--plane)
    {
        btechHostWritePortWord(COMBAT_EGA_GRAPHICS_PORT,COMBAT_EGA_READ_MAP_REGISTER|((uint16_t)plane<<8));
        uint8_t bits=btechHostReadVideoByte(page,offset);
        colour=(uint8_t)((colour<<1)|((bits&(1<<shift))!=0));
    }
    btechHostWritePortWord(COMBAT_EGA_SEQUENCER_PORT,COMBAT_EGA_MAP_ALL_PLANES);
    btechHostWritePortWord(COMBAT_EGA_GRAPHICS_PORT,COMBAT_EGA_WRITE_MODE_TWO);
    return colour;
}

/*2A36:233D. Original BIOS1001 colour mapping; hardware border, not a game UI. */
void BtechEgaSetOverscanColour(uint16_t colour)
{
    uint8_t hardwareColour=(uint8_t)colour&0x0F;
    if (combatRendererKind==COMBAT_VIDEO_EGA && hardwareColour>=8) hardwareColour+=8;
    if (!btechHostSetOverscanColour) abort();
    btechHostSetOverscanColour(hardwareColour);
}
