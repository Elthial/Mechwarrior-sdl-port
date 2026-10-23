#include "mechwarrior/port_diagnostics.h"
#include "mechwarrior/combat_video.h"
#include <stdlib.h>

/*1BF5:1D04. Clipped-away sides do not acquire replacement borders. Vertical
 * edges use035C; top/bottom use solid fill-colour spans, interior line patterns. */
uint16_t BtechDrawStyledRectangle(uint16_t left,uint16_t top,uint16_t width,uint16_t height)
{
    BtechSetEgaSpriteWriteRegisters();
    uint16_t page=drawPageSelector;
    if (combatRendererKind!=COMBAT_VIDEO_EGA) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: combatRendererKind!=COMBAT_VIDEO_EGA");
    spanPrimaryPattern=palettePrimarySlots[drawLineColour/COMBAT_PALETTE_WORDS][drawLineColour%COMBAT_PALETTE_WORDS];
    spanSecondaryPattern=paletteSecondarySlots[drawLineColour/COMBAT_PALETTE_WORDS][drawLineColour%COMBAT_PALETTE_WORDS];
    if (spanPrimaryPattern==spanSecondaryPattern)
        shapeSpanCallback=(BtechShapeSpanCallback){graphicsFillSolidSpan,NULL};
    else shapeSpanCallback=(BtechShapeSpanCallback){NULL,graphicsFillAlternatingSpan};
    drawColourPattern=palettePrimarySlots[drawFillColour/COMBAT_PALETTE_WORDS][drawFillColour%COMBAT_PALETTE_WORDS];
    if ((int16_t)width<=0 || (int16_t)height<=0) return width;
    uint16_t clippedSides=0;
    if (screenPolygonNeedsClip==1)
    {
        uint16_t margin=left-clipMinimumX;
        if ((int16_t)left<clipMinimumX)
        {
            left=(uint16_t)clipMinimumX;
            int32_t adjusted=(int16_t)width+(int16_t)margin;
            width=(uint16_t)adjusted;
            if (adjusted<=0) return width;
            clippedSides|=1;
        }
        margin=(uint16_t)(top-clipMinimumY);
        if ((int16_t)top<clipMinimumY)
        {
            top=(uint16_t)clipMinimumY;
            int32_t adjusted=(int16_t)height+(int16_t)margin;
            height=(uint16_t)adjusted;
            if (adjusted<=0) return width;
            clippedSides|=2;
        }
        uint16_t end=(uint16_t)(width+left),limit=(uint16_t)(clipMaximumX+1);
        margin=(uint16_t)(limit-end);
        if ((int16_t)limit<(int16_t)end)
        {
            int32_t adjusted=(int16_t)width+(int16_t)margin;
            width=(uint16_t)adjusted;
            if (adjusted<=0) return width;
            clippedSides|=4;
        }
        end=(uint16_t)(height+top);limit=(uint16_t)(clipMaximumY+1);
        margin=(uint16_t)(limit-end);
        if ((int16_t)limit<(int16_t)end)
        {
            clippedSides|=8;
            int32_t adjusted=(int16_t)height+(int16_t)margin;
            height=(uint16_t)adjusted;
            if (adjusted<=0) return width;
        }
    }
    if (!drawPolygonFilled || drawFillColour!=drawLineColour)
    {
        uint8_t savedClip=screenPolygonNeedsClip;screenPolygonNeedsClip=false;
        if (!(clippedSides&1))
        {
            BtechDrawClippedLine((int16_t)left,(int16_t)top,(int16_t)left,(int16_t)(top+height-1));
            ++left;--width;
            if ((int16_t)width<0) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: (int16_t)width<0"); /*1E0A native RETF consumes saved clip as ES and corrupts return; clip remains0. */
        }
        if (!(clippedSides&4))
        {
            uint16_t right=left+width-1;
            BtechDrawClippedLine((int16_t)right,(int16_t)top,(int16_t)right,(int16_t)(top+height-1));
            --width;
            if ((int16_t)width<0) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: (int16_t)width<0"); /* Same native broken nonlocal return, not a normal C return. */
        }
        if (!((int16_t)height<3 && (clippedSides&0x0A)==0x0A))
        {
            if (!(clippedSides&2))
            {
                if (!graphicsFillSolidSpan || top>=COMBAT_SCREEN_HEIGHT) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !graphicsFillSolidSpan || top>=COMBAT_SCREEN_HEIGHT");
                graphicsFillSolidSpan(drawColourPattern,left,(int16_t)width,graphicsRowOffsets[top],page);
                ++top;--height;
                if ((int16_t)height<=0) goto restoreClip;
            }
            if (!(clippedSides&8))
            {
                uint16_t bottom=top+height-1;
                if (!graphicsFillSolidSpan || bottom>=COMBAT_SCREEN_HEIGHT) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !graphicsFillSolidSpan || bottom>=COMBAT_SCREEN_HEIGHT");
                graphicsFillSolidSpan(drawColourPattern,left,(int16_t)width,graphicsRowOffsets[bottom],page);
                --height;
            }
        }
restoreClip:
        screenPolygonNeedsClip=savedClip;
        if ((int16_t)height<=0) return width;
    }
    if (!drawPolygonFilled) return width;
    spanAlternationState=top&1;
    if (top>=COMBAT_SCREEN_HEIGHT) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: top>=COMBAT_SCREEN_HEIGHT"); /* Adjacent row-table alias not mapped. */
    uint16_t rowOffset=graphicsRowOffsets[top];
    do
    {
        if (shapeSpanCallback.alternating)
            shapeSpanCallback.alternating(spanPrimaryPattern,left,(int16_t)width,spanSecondaryPattern,rowOffset,page);
        else
        {
            if (!shapeSpanCallback.solid) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !shapeSpanCallback.solid");
            shapeSpanCallback.solid(spanPrimaryPattern,left,(int16_t)width,rowOffset,page);
        }
        ++top;
        /* Native reads one extra row WORD after the last span. Row200 aliases
         * the first WORD of the adjacent Hercules table, not palette state. */
        if (top<COMBAT_SCREEN_HEIGHT) rowOffset=graphicsRowOffsets[top];
        else if (top==COMBAT_SCREEN_HEIGHT) rowOffset=herculesDisplayRowOffsets[0];
        else /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rectangle edge references an unsupported scanline.");
    } while (--height);
    return width;
}
