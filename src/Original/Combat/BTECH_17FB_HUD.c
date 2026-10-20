#include "mechwarrior/combat_cockpit.h"
#include "mechwarrior/combat_import.h"
#include "mechwarrior/combat_polygons.h"
#include <stdlib.h>

/*17FB:1AD6. Four authored unrolled lamps; hit fields reload after each draw.
 * Colour-table over-indexing within the adjacent combined owner is retained. */
void BtechDrawMechSubsystemLamps(BtechMechState *mech)
{
    if (!mech) abort();
    textTransparentBackground=1;drawPolygonFilled=1;screenPolygonNeedsClip=0;
    uint16_t colourWord=(uint16_t)(hudColourSchemeIndex*6+mech->sensorHits*2)>>1;
    if (colourWord>=14) abort();
    drawLineColour=drawFillColour=(uint8_t)subsystemLampColourTables.words[colourWord];
    BtechDrawStyledRectangle(0x89,0x6B,7,7);
    textForegroundColour=drawFillColour?Combat_Pal_EGA_Black:(hudColourSchemeIndex?Combat_Pal_EGA_Red:Combat_Pal_EGA_DarkGrey);
    BtechDrawNearText(subsystemLampLabels[0],0x8A,0x6C);
    colourWord=(uint16_t)(hudColourSchemeIndex*6+mech->gyroHits*2)>>1;
    if (colourWord>=14) abort();
    drawLineColour=drawFillColour=(uint8_t)subsystemLampColourTables.words[colourWord];
    BtechDrawStyledRectangle(0x93,0x6B,7,7);
    textForegroundColour=drawFillColour?Combat_Pal_EGA_Black:(hudColourSchemeIndex?Combat_Pal_EGA_Red:Combat_Pal_EGA_DarkGrey);
    BtechDrawNearText(subsystemLampLabels[1],0x94,0x6C);
    colourWord=(uint16_t)(12+hudColourSchemeIndex*8+mech->engineHits*2)>>1;
    if (colourWord>=14) abort();
    drawLineColour=drawFillColour=(uint8_t)subsystemLampColourTables.words[colourWord];
    BtechDrawStyledRectangle(0x9D,0x6B,7,7);
    textForegroundColour=drawFillColour?Combat_Pal_EGA_Black:(hudColourSchemeIndex?Combat_Pal_EGA_Red:Combat_Pal_EGA_DarkGrey);
    BtechDrawNearText(subsystemLampLabels[2],0x9E,0x6C);
    colourWord=(uint16_t)(hudColourSchemeIndex*6+mech->lifeSupportHits*2)>>1;
    if (colourWord>=14) abort();
    drawLineColour=drawFillColour=(uint8_t)subsystemLampColourTables.words[colourWord];
    BtechDrawStyledRectangle(0xA7,0x6B,7,7);
    textForegroundColour=drawFillColour?Combat_Pal_EGA_Black:(hudColourSchemeIndex?Combat_Pal_EGA_Red:Combat_Pal_EGA_DarkGrey);
    BtechDrawNearText(subsystemLampLabels[3],0xA8,0x6C);
    textTransparentBackground=0;
    BtechPresentHudRectangleIfReady(0x89,0x6B,0x28,0x0B);
}

/*17FB:2176. Native compass labels/ticks and signed torso-relative marker.
 * Only X bounds restore; clipping is disabled and plane mask becomes0F. */
void BtechDrawCockpitHeadingStrip(uint16_t bodyHeading,int16_t torsoTwist)
{
    enum { HEADING_UNITS_PER_DEGREE=0xB6,HEADING_LABEL_STEP=30,HEADING_LABELS=12,
        HEADING_STRIP_LEFT=0x78,HEADING_STRIP_RIGHT=0xC7,HEADING_LABELS_DRAWN=4 };
    int16_t savedMinimumX=clipMinimumX,savedMaximumX=clipMaximumX;
    if (combatLandscapeIndex>=COMBAT_LANDSCAPES || hudColourSchemeIndex>=COMBAT_HUD_COLOUR_SCHEMES) abort();
    uint8_t colour=landscapeCameraColourRows[hudColourSchemeIndex][combatLandscapeIndex].hudReticleColour;
    egaBitmapEnabledPlanes=colour;drawFillColour=colour;
    clipMinimumX=HEADING_STRIP_LEFT;clipMaximumX=HEADING_STRIP_RIGHT;screenPolygonNeedsClip=1;
    if (!unitBindings[0].mech || unitBindings[0].mech->templateIndex>=COMBAT_MECH_TEMPLATES) abort();
    uint16_t layoutIndex=mechCockpitLayoutIndices[unitBindings[0].mech->templateIndex];
    if (layoutIndex>=COMBAT_COCKPIT_LAYOUTS) abort();
    int16_t y=cockpitLayouts[layoutIndex].headingStripY;
    uint16_t headingDegrees=(uint16_t)(0-bodyHeading)/HEADING_UNITS_PER_DEGREE;
    int16_t torsoMarkerOffset=torsoTwist/HEADING_UNITS_PER_DEGREE;
    int16_t windowStart=(int16_t)(uint16_t)(headingDegrees-0x29);
    if (windowStart<0) windowStart=(int16_t)(uint16_t)(windowStart+360);
    int16_t labelIndex=windowStart/HEADING_LABEL_STEP;
    int16_t labelX=(int16_t)(uint16_t)(labelIndex*HEADING_LABEL_STEP-windowStart-1);
    for (uint16_t labelsDrawn=0;labelsDrawn<HEADING_LABELS_DRAWN;++labelsDrawn)
    {
        if (labelIndex==HEADING_LABELS) labelIndex=0;
        if (!hudGlyphPictures || labelIndex<0 || labelIndex>=hudGlyphPictures->imageCount) abort();
        BtechDrawBitmap(&hudGlyphPictures->images[labelIndex],(int16_t)(uint16_t)(labelX+0x72),(int16_t)(uint16_t)(y+4));
        BtechDrawClippedLine((int16_t)(uint16_t)(labelX+0x78),(int16_t)(uint16_t)(y+0x0B),(int16_t)(uint16_t)(labelX+0x78),(int16_t)(uint16_t)(y+0x11));
        BtechWriteClippedPixel((int16_t)(uint16_t)(labelX+0x7D),(int16_t)(uint16_t)(y+0x10),landscapeCameraColourRows[hudColourSchemeIndex][combatLandscapeIndex].headingTickPaletteIndex);
        BtechDrawClippedLine((int16_t)(uint16_t)(labelX+0x82),(int16_t)(uint16_t)(y+0x0E),(int16_t)(uint16_t)(labelX+0x82),(int16_t)(uint16_t)(y+0x11));
        BtechWriteClippedPixel((int16_t)(uint16_t)(labelX+0x87),(int16_t)(uint16_t)(y+0x10),landscapeCameraColourRows[hudColourSchemeIndex][combatLandscapeIndex].headingTickPaletteIndex);
        BtechDrawClippedLine((int16_t)(uint16_t)(labelX+0x8C),(int16_t)(uint16_t)(y+0x0E),(int16_t)(uint16_t)(labelX+0x8C),(int16_t)(uint16_t)(y+0x11));
        BtechWriteClippedPixel((int16_t)(uint16_t)(labelX+0x91),(int16_t)(uint16_t)(y+0x10),landscapeCameraColourRows[hudColourSchemeIndex][combatLandscapeIndex].headingTickPaletteIndex);
        labelX=(int16_t)(uint16_t)(labelX+HEADING_LABEL_STEP);++labelIndex;
    }
    BtechDrawClippedLine(HEADING_STRIP_LEFT,(int16_t)(uint16_t)(y+0x11),HEADING_STRIP_RIGHT,(int16_t)(uint16_t)(y+0x11));
    BtechWriteClippedPixel(0x9F,(int16_t)(uint16_t)(y+0x13),landscapeCameraColourRows[hudColourSchemeIndex][combatLandscapeIndex].headingTickPaletteIndex);
    BtechDrawClippedLine(0x9E,(int16_t)(uint16_t)(y+0x14),0xA0,(int16_t)(uint16_t)(y+0x14));
    BtechDrawClippedLine(0x9D,(int16_t)(uint16_t)(y+0x15),0xA1,(int16_t)(uint16_t)(y+0x15));
    BtechWriteClippedPixel((int16_t)(uint16_t)(0x9F-torsoMarkerOffset),(int16_t)(uint16_t)(y+2),landscapeCameraColourRows[hudColourSchemeIndex][combatLandscapeIndex].headingTickPaletteIndex);
    BtechDrawClippedLine((int16_t)(uint16_t)(0x9E-torsoMarkerOffset),(int16_t)(uint16_t)(y+1),(int16_t)(uint16_t)(0xA0-torsoMarkerOffset),(int16_t)(uint16_t)(y+1));
    BtechDrawClippedLine((int16_t)(uint16_t)(0x9D-torsoMarkerOffset),y,(int16_t)(uint16_t)(0xA1-torsoMarkerOffset),y);
    clipMinimumX=savedMinimumX;clipMaximumX=savedMaximumX;
    screenPolygonNeedsClip=0;egaBitmapEnabledPlanes=COMBAT_EGA_PLANE_MASK_ALL;
}
