#include "mechwarrior/combat_cockpit.h"
#include "mechwarrior/combat_startup.h"
#include "mechwarrior/combat_import.h"
#include "mechwarrior/combat_render.h"
#include "mechwarrior/combat_ballistics.h"
#include <stdlib.h>

/*0810:65E2. Signed sensor damage chooses radar or an empty coloured panel. */
void BtechUpdateRadarPanel(void)
{
    if (!playerMechState) abort();
    if ((int16_t)playerMechState->sensorHits<2)
    {
        if (headingRadarSelected) BtechDrawHeadingRadar();
        else BtechDrawTerrainRadar();
        return;
    }
    if (activeCockpitLayoutIndex>=COMBAT_COCKPIT_LAYOUTS) abort();
    const BtechScreenRectangle *panel=&cockpitLayouts[activeCockpitLayoutIndex].radarPanel;
    drawPolygonFilled=true;
    if (headingRadarSelected)
        drawLineColour=hudColourSchemeIndex?RADAR_ALTERNATE_HEADING_BACKGROUND:RADAR_HEADING_BACKGROUND;
    else
    {
        if (hudColourSchemeIndex>=COMBAT_HUD_COLOUR_SCHEMES) abort();
        drawLineColour=(uint8_t)hudColourSchemes[hudColourSchemeIndex].radarBackground;
    }
    drawFillColour=drawLineColour;
    BtechDrawStyledRectangle(panel->x,panel->y,panel->width,panel->height);
}

/*0810:665A. Body+torso-relative contacts, not the terrain radar's cache.
 * Native671B tests the binding ADDRESS, not its Mech pointer. */
void BtechDrawHeadingRadar(void)
{
    drawPageSelector=backPageSelector;
    if (activeCockpitLayoutIndex>=COMBAT_COCKPIT_LAYOUTS) abort();
    const BtechScreenRectangle *panel=&cockpitLayouts[activeCockpitLayoutIndex].radarPanel;
    int16_t halfWidth=(panel->width-(panel->width<0))/2;
    int16_t halfHeight=(panel->height-(panel->height<0))/2;
    uint16_t centreX=panel->x+halfWidth-1;
    uint16_t centreY=panel->y+halfWidth-1; /*6680 uses WIDTH. */
    drawPolygonFilled=true;
    drawLineColour=hudColourSchemeIndex?RADAR_ALTERNATE_HEADING_BACKGROUND:RADAR_HEADING_BACKGROUND;
    drawFillColour=drawLineColour;
    BtechDrawStyledRectangle(panel->x,panel->y,panel->width,panel->height);
    drawFillColour=hudColourSchemeIndex?Combat_Pal_EGA_White:Combat_Pal_EGA_Green;
    BtechDrawClippedLine(panel->x,panel->y,(int16_t)centreX,(int16_t)centreY);
    BtechDrawClippedLine((int16_t)(panel->x+panel->width-1),panel->y,(int16_t)centreX,(int16_t)centreY);
    if (!playerUnitBinding || !playerUnitBinding->torso || !playerUnitBinding->positionedLeftLeg) abort();
    BtechWorldObjectPrefix *player=&playerUnitBinding->positionedLeftLeg->object.prefix;
    uint16_t heading=player->orientationC+playerUnitBinding->torso->object.prefix.orientationC;
    for (uint16_t slot=1;slot<COMBAT_MECH_SLOTS;++slot)
    {
        if (unitControls[slot].combatValueOrBoundaryCode<=-2) continue;
        uint16_t paletteIndex;
        if (!unitControls[slot].sideIndex) paletteIndex=hudColourSchemeIndex?Combat_Pal_EGA_Black:Combat_Pal_EGA_LightBlue;
        else
        {
            if (!unitVisibility[0][slot]) continue;
            paletteIndex=hudColourSchemeIndex?Combat_Pal_EGA_Green:Combat_Pal_EGA_LightRed;
        }
        /* Do not repair671B by silently skipping absent eligible contacts. */
        if (!unitBindings[slot].torso) abort();
        const BtechPosition3D *contact=&unitBindings[slot].torso->object.prefix.position;
        int16_t relativeX=(int16_t)BtechShiftSignedDwordRight(contact->positionA-player->position.positionA,(uint8_t)radarRangeShift);
        int16_t relativeY=(int16_t)BtechShiftSignedDwordRight(contact->positionB-player->position.positionB,(uint8_t)radarRangeShift);
        BtechRotateSignedWordPair(&relativeX,&relativeY,(uint16_t)(0-heading));
        int32_t absoluteX=relativeX==INT16_MIN?INT16_MAX:relativeX<0?-(int32_t)relativeX:relativeX;
        int32_t absoluteY=relativeY==INT16_MIN?INT16_MAX:relativeY<0?-(int32_t)relativeY:relativeY;
        if (absoluteX<halfWidth && absoluteY<halfHeight)
            BtechWriteClippedPixel((int16_t)(centreX+relativeX),(int16_t)(centreY-relativeY-1),paletteIndex);
    }
    textForegroundColour=hudColourSchemeIndex?Combat_Pal_EGA_White:Combat_Pal_EGA_Green;textTransparentBackground=true;
    if (radarRangeShift<RADAR_MINIMUM_RANGE_SHIFT || radarRangeShift>RADAR_MAXIMUM_RANGE_SHIFT) abort(); /* ASCII aliases below2406. */
    BtechDrawNearText(radarRangeLabels[radarRangeShift-RADAR_MINIMUM_RANGE_SHIFT],
        (int16_t)(panel->x+panel->width/2-0x12),(int16_t)(panel->y+panel->height-8));
    textTransparentBackground=false;
    BtechPresentScreenRectangle(panel->x,panel->y,panel->width,panel->height);
}
