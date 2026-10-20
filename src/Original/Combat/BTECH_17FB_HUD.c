#include "mechwarrior/combat_cockpit.h"
#include "mechwarrior/combat_import.h"
#include "mechwarrior/combat_polygons.h"
#include <stdlib.h>

enum { SUBSYSTEM_LAMP_Y=0x6B, SUBSYSTEM_LAMP_SIZE=7, SUBSYSTEM_LAMP_TEXT_Y=0x6C,
    SENSOR_LAMP_X=0x89, SENSOR_LAMP_TEXT_X=0x8A, GYRO_LAMP_X=0x93, GYRO_LAMP_TEXT_X=0x94,
    ENGINE_LAMP_X=0x9D, ENGINE_LAMP_TEXT_X=0x9E,
    LIFE_SUPPORT_LAMP_X=0xA7, LIFE_SUPPORT_LAMP_TEXT_X=0xA8,
    SUBSYSTEM_LAMPS_COPY_WIDTH=0x28, SUBSYSTEM_LAMPS_COPY_HEIGHT=0x0B };

/*17FB:1AD6. Four authored unrolled lamps; hit fields reload after each draw.
 * Colour-table over-indexing within the adjacent combined owner is retained. */
void BtechDrawMechSubsystemLamps(BtechMechState *mech)
{
    if (!mech) abort();
    textTransparentBackground=true;drawPolygonFilled=true;screenPolygonNeedsClip=false;
    uint16_t colourWord=(uint16_t)(hudColourSchemeIndex*6+mech->sensorHits*2)>>1;
    if (colourWord>=14) abort();
    drawLineColour=drawFillColour=(uint8_t)subsystemLampColourTables.words[colourWord];
    BtechDrawStyledRectangle(SENSOR_LAMP_X,SUBSYSTEM_LAMP_Y,SUBSYSTEM_LAMP_SIZE,SUBSYSTEM_LAMP_SIZE);
    textForegroundColour=drawFillColour?Combat_Pal_EGA_Black:(hudColourSchemeIndex?Combat_Pal_EGA_Red:Combat_Pal_EGA_DarkGrey);
    BtechDrawNearText(subsystemLampLabels[0],SENSOR_LAMP_TEXT_X,SUBSYSTEM_LAMP_TEXT_Y);
    colourWord=(uint16_t)(hudColourSchemeIndex*6+mech->gyroHits*2)>>1;
    if (colourWord>=14) abort();
    drawLineColour=drawFillColour=(uint8_t)subsystemLampColourTables.words[colourWord];
    BtechDrawStyledRectangle(GYRO_LAMP_X,SUBSYSTEM_LAMP_Y,SUBSYSTEM_LAMP_SIZE,SUBSYSTEM_LAMP_SIZE);
    textForegroundColour=drawFillColour?Combat_Pal_EGA_Black:(hudColourSchemeIndex?Combat_Pal_EGA_Red:Combat_Pal_EGA_DarkGrey);
    BtechDrawNearText(subsystemLampLabels[1],GYRO_LAMP_TEXT_X,SUBSYSTEM_LAMP_TEXT_Y);
    colourWord=(uint16_t)(12+hudColourSchemeIndex*8+mech->engineHits*2)>>1;
    if (colourWord>=14) abort();
    drawLineColour=drawFillColour=(uint8_t)subsystemLampColourTables.words[colourWord];
    BtechDrawStyledRectangle(ENGINE_LAMP_X,SUBSYSTEM_LAMP_Y,SUBSYSTEM_LAMP_SIZE,SUBSYSTEM_LAMP_SIZE);
    textForegroundColour=drawFillColour?Combat_Pal_EGA_Black:(hudColourSchemeIndex?Combat_Pal_EGA_Red:Combat_Pal_EGA_DarkGrey);
    BtechDrawNearText(subsystemLampLabels[2],ENGINE_LAMP_TEXT_X,SUBSYSTEM_LAMP_TEXT_Y);
    colourWord=(uint16_t)(hudColourSchemeIndex*6+mech->lifeSupportHits*2)>>1;
    if (colourWord>=14) abort();
    drawLineColour=drawFillColour=(uint8_t)subsystemLampColourTables.words[colourWord];
    BtechDrawStyledRectangle(LIFE_SUPPORT_LAMP_X,SUBSYSTEM_LAMP_Y,SUBSYSTEM_LAMP_SIZE,SUBSYSTEM_LAMP_SIZE);
    textForegroundColour=drawFillColour?Combat_Pal_EGA_Black:(hudColourSchemeIndex?Combat_Pal_EGA_Red:Combat_Pal_EGA_DarkGrey);
    BtechDrawNearText(subsystemLampLabels[3],LIFE_SUPPORT_LAMP_TEXT_X,SUBSYSTEM_LAMP_TEXT_Y);
    textTransparentBackground=false;
    BtechPresentHudRectangleIfReady(SENSOR_LAMP_X,SUBSYSTEM_LAMP_Y,SUBSYSTEM_LAMPS_COPY_WIDTH,SUBSYSTEM_LAMPS_COPY_HEIGHT);
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
    clipMinimumX=HEADING_STRIP_LEFT;clipMaximumX=HEADING_STRIP_RIGHT;screenPolygonNeedsClip=true;
    if (!unitBindings[0].mech || unitBindings[0].mech->templateIndex>=COMBAT_MECH_TEMPLATES) abort();
    uint16_t layoutIndex=mechCockpitLayoutIndices[unitBindings[0].mech->templateIndex];
    if (layoutIndex>=COMBAT_COCKPIT_LAYOUTS) abort();
    int16_t y=cockpitLayouts[layoutIndex].headingStripY;
    uint16_t headingDegrees=(uint16_t)(0-bodyHeading)/HEADING_UNITS_PER_DEGREE;
    int16_t torsoMarkerOffset=torsoTwist/HEADING_UNITS_PER_DEGREE;
    int16_t windowStart=headingDegrees-0x29;
    if (windowStart<0) windowStart=(int16_t)(windowStart+360);
    int16_t labelIndex=windowStart/HEADING_LABEL_STEP;
    int16_t labelX=labelIndex*HEADING_LABEL_STEP-windowStart-1;
    for (uint16_t labelsDrawn=0;labelsDrawn<HEADING_LABELS_DRAWN;++labelsDrawn)
    {
        if (labelIndex==HEADING_LABELS) labelIndex=0;
        if (!hudGlyphPictures || labelIndex<0 || labelIndex>=hudGlyphPictures->imageCount) abort();
        BtechDrawBitmap(&hudGlyphPictures->images[labelIndex],(int16_t)(labelX+0x72),(int16_t)(y+4));
        BtechDrawClippedLine((int16_t)(labelX+0x78),(int16_t)(y+0x0B),(int16_t)(labelX+0x78),(int16_t)(y+0x11));
        BtechWriteClippedPixel((int16_t)(labelX+0x7D),(int16_t)(y+0x10),landscapeCameraColourRows[hudColourSchemeIndex][combatLandscapeIndex].headingTickPaletteIndex);
        BtechDrawClippedLine((int16_t)(labelX+0x82),(int16_t)(y+0x0E),(int16_t)(labelX+0x82),(int16_t)(y+0x11));
        BtechWriteClippedPixel((int16_t)(labelX+0x87),(int16_t)(y+0x10),landscapeCameraColourRows[hudColourSchemeIndex][combatLandscapeIndex].headingTickPaletteIndex);
        BtechDrawClippedLine((int16_t)(labelX+0x8C),(int16_t)(y+0x0E),(int16_t)(labelX+0x8C),(int16_t)(y+0x11));
        BtechWriteClippedPixel((int16_t)(labelX+0x91),(int16_t)(y+0x10),landscapeCameraColourRows[hudColourSchemeIndex][combatLandscapeIndex].headingTickPaletteIndex);
        labelX=(int16_t)(labelX+HEADING_LABEL_STEP);++labelIndex;
    }
    BtechDrawClippedLine(HEADING_STRIP_LEFT,(int16_t)(y+0x11),HEADING_STRIP_RIGHT,(int16_t)(y+0x11));
    BtechWriteClippedPixel(0x9F,(int16_t)(y+0x13),landscapeCameraColourRows[hudColourSchemeIndex][combatLandscapeIndex].headingTickPaletteIndex);
    BtechDrawClippedLine(0x9E,(int16_t)(y+0x14),0xA0,(int16_t)(y+0x14));
    BtechDrawClippedLine(0x9D,(int16_t)(y+0x15),0xA1,(int16_t)(y+0x15));
    BtechWriteClippedPixel((int16_t)(0x9F-torsoMarkerOffset),(int16_t)(y+2),landscapeCameraColourRows[hudColourSchemeIndex][combatLandscapeIndex].headingTickPaletteIndex);
    BtechDrawClippedLine((int16_t)(0x9E-torsoMarkerOffset),(int16_t)(y+1),(int16_t)(0xA0-torsoMarkerOffset),(int16_t)(y+1));
    BtechDrawClippedLine((int16_t)(0x9D-torsoMarkerOffset),y,(int16_t)(0xA1-torsoMarkerOffset),y);
    clipMinimumX=savedMinimumX;clipMaximumX=savedMaximumX;
    screenPolygonNeedsClip=false;egaBitmapEnabledPlanes=COMBAT_EGA_PLANE_MASK_ALL;
}
