#include "mechwarrior/combat_render.h"
#include <stdlib.h>

/*22B6:6501. Record colours, optional normal shade and ordered palette remap.
 * The authored CS table is imported once, not rebuilt from the SDL palette. */
void BtechSelectPolygonColoursAndShade(const BtechRenderPolygon *polygon)
{
    if (graphicsColourMode)
    {
        drawFillColour=polygon->fillColour;
        drawLineColour=polygon->lineColour;
    }
    else
    {
        drawFillColour=polygon->alternateFillColour;
        drawLineColour=polygon->alternateLineColour;
    }
    uint8_t flags=polygon->flags;
    drawPolygonFilled=(flags&POLYGON_FILLED)!=0;
    if (drawPolygonFilled && (flags&POLYGON_SHADE_NORMAL)
        && (flags&POLYGON_FACING_MASK)==1 && graphicsColourMode && activeColourShadeClasses)
    {
        uint8_t shadeClass=activeColourShadeClasses[drawLineColour];
        if (shadeClass && (int8_t)shadeClass<=POLYGON_SHADE_CLASSES) /* Native signed JG; do not clamp80..FF. */
        {
            polygonShadeClass=shadeClass;
            BtechCachePolygonShadeNormal(polygon->normalVectorIndex);
            uint16_t dotHigh=BtechDotProductSignedWordVectors(transformedReferenceVector,viewVectorCoefficients)>>16;
            if (dotHigh==0x0010) dotHigh=0x000F;
            uint16_t shadeOffset=dotHigh+0x0010+(uint16_t)(uint8_t)(polygonShadeClass-1)*POLYGON_SHADE_STEPS;
            if (shadeOffset>=sizeof(polygonShadeDeltas)) abort(); /* Native read enters other CS owners, not a ninth shade row. */
            uint8_t originalLineColour=drawLineColour;
            drawLineColour+=polygonShadeDeltas[shadeOffset/POLYGON_SHADE_STEPS][shadeOffset%POLYGON_SHADE_STEPS];
            if (originalLineColour==drawFillColour) drawFillColour=drawLineColour;
        }
    }
    if (activePaletteRemap)
    {
        drawFillColour=activePaletteRemap[drawFillColour];
        drawLineColour=activePaletteRemap[drawLineColour];
    }
}
