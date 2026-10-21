#include "mechwarrior/port_diagnostics.h"
#include "mechwarrior/combat_cockpit.h"
#include "mechwarrior/combat_startup.h"
#include "mechwarrior/combat_import.h"
#include "mechwarrior/combat_orders.h"
#include <stdlib.h>

/*17FB:1C84. Terrain pixels and page-interleaved contact backgrounds. Previous
 * tick240E is authored state, not a BSS variable cleared between stages. */
void BtechDrawTerrainRadar(void)
{
    uint8_t backdropChanged=0;
    uint16_t tickDiscontinuity=(uint16_t)(radarPreviousTick+1)!=combatTickCounter;
    radarPreviousTick=combatTickCounter;
    radarCachePage=hudColourSchemeIndex?0:combatTickCounter&1;
    drawPageSelector=backPageSelector;
    if (!playerUnitBinding) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !playerUnitBinding");
    radarPlayerObject=playerUnitBinding->positionedLeftLeg;
    if (activeCockpitLayoutIndex>=COMBAT_COCKPIT_LAYOUTS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: activeCockpitLayoutIndex>=COMBAT_COCKPIT_LAYOUTS");
    radarRectangle=&cockpitLayouts[activeCockpitLayoutIndex].radarPanel;
    int16_t halfWidth=(radarRectangle->width-(radarRectangle->width<0))/2;
    int16_t halfHeight=(radarRectangle->height-(radarRectangle->height<0))/2;
    radarCentreX=(uint16_t)(radarRectangle->x+halfWidth-1);
    radarCentreY=(uint16_t)(radarRectangle->y+halfHeight-1);
    if (!radarPlayerObject) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !radarPlayerObject");
    const BtechPosition3D *player=&radarPlayerObject->object.prefix.position;
    uint16_t mapX=BtechShiftSignedDwordRightLow(player->positionA,TERRAIN_WORLD_CELL_SHIFT)-halfWidth+TERRAIN_TILE_ROWS;
    uint16_t mapY=TERRAIN_TILE_COLUMNS-halfHeight-BtechShiftSignedDwordRightLow(player->positionB,TERRAIN_WORLD_CELL_SHIFT);
    if (mapX!=radarMapOriginX || mapY!=radarMapOriginY || cockpitFullRedrawPending || tickDiscontinuity)
    {
        radarMapOriginX=mapX;radarMapOriginY=mapY;drawPolygonFilled=true;
        if (hudColourSchemeIndex>=COMBAT_HUD_COLOUR_SCHEMES) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: hudColourSchemeIndex>=COMBAT_HUD_COLOUR_SCHEMES");
        drawLineColour=(uint8_t)hudColourSchemes[hudColourSchemeIndex].radarBackground;
        drawFillColour=drawLineColour;
        BtechDrawStyledRectangle(radarRectangle->x,radarRectangle->y,radarRectangle->width,radarRectangle->height);
        for (uint16_t panelX=0;(int16_t)panelX<radarRectangle->width;++panelX,++mapX)
        {
            uint16_t terrainY=mapY;
            for (uint16_t panelY=0;(int16_t)panelY<radarRectangle->height;++panelY,++terrainY)
            {
                if (mapX>=TERRAIN_GRID_ROWS || terrainY>=TERRAIN_GRID_COLUMNS) continue;
                uint16_t height=terrainHeightGrid[mapX][terrainY];
                if (!height) continue;
                uint16_t band=height>>5;if (band>3) band=3;
                BtechWriteClippedPixel((int16_t)(radarRectangle->x+panelX),
                    (int16_t)(radarRectangle->y+panelY),hudColourSchemes[hudColourSchemeIndex].radarTerrainBands[band]);
            }
        }
        backdropChanged=1;
    }
    else
    {
        for (uint16_t marker=0;(int16_t)marker<(int16_t)radarMarkerCounts[radarCachePage];++marker)
        {
            if (marker>=COMBAT_RADAR_MARKERS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: marker>=COMBAT_RADAR_MARKERS"); /* Native next-owner alias. */
            BtechWriteClippedPixel((int16_t)radarMarkerX[marker][radarCachePage],
                (int16_t)radarMarkerY[marker][radarCachePage],radarMarkerBackground[marker][radarCachePage]);
        }
    }
    radarMarkerCounts[radarCachePage]=0;
    if (hudColourSchemeIndex>=COMBAT_HUD_COLOUR_SCHEMES) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: hudColourSchemeIndex>=COMBAT_HUD_COLOUR_SCHEMES");
    uint16_t paletteIndex=hudColourSchemes[hudColourSchemeIndex].radarFriendly;
    for (uint16_t slot=1;slot<COMBAT_MECH_SLOTS;++slot)
    {
        if (!unitBindings[slot].mech || unitControls[slot].combatValueOrBoundaryCode<=-2) continue;
        if (slot>=COMBAT_FRIENDLY_SLOTS)
        {
            if (!unitVisibility[0][slot]) continue;
            paletteIndex=hudColourSchemes[hudColourSchemeIndex].radarOpposing;
        }
        if (!unitBindings[slot].torso) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !unitBindings[slot].torso");
        BtechDrawRadarPositionMarker(&unitBindings[slot].torso->object.prefix.position,paletteIndex);
    }
    if (sideObjectiveRules[0]==OBJECTIVE_SPECIAL_TARGET || sideObjectiveRules[0]==OBJECTIVE_DEFEND_SPECIAL_TARGET)
    {
        if (!specialTargetObject) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !specialTargetObject");
        BtechDrawRadarPositionMarker(&specialTargetObject->object.prefix.position,hudColourSchemes[hudColourSchemeIndex].radarSpecialTarget);
    }
    BtechDrawRadarPositionMarker(player,hudColourSchemes[hudColourSchemeIndex].radarPlayer);
    if (backdropChanged)
    {
        BtechCopyHudRectangleIfReady(radarRectangle->x,radarRectangle->y,radarRectangle->width,radarRectangle->height);
        if (!hudColourSchemeIndex)
        {
            uint16_t otherPage=radarCachePage<1?1:0;
            radarMarkerCounts[otherPage]=radarMarkerCounts[radarCachePage];
            uint16_t count=radarMarkerCounts[radarCachePage];
            for (uint16_t marker=0;(int16_t)marker<(int16_t)count;++marker)
            {
                if (marker>=COMBAT_RADAR_MARKERS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: marker>=COMBAT_RADAR_MARKERS");
                radarMarkerX[marker][otherPage]=radarMarkerX[marker][radarCachePage];
                radarMarkerY[marker][otherPage]=radarMarkerY[marker][radarCachePage];
                radarMarkerBackground[marker][otherPage]=radarMarkerBackground[marker][radarCachePage];
            }
        }
    }
    else BtechPresentHudRectangleIfReady(radarRectangle->x,radarRectangle->y,radarRectangle->width,radarRectangle->height);
}

/*17FB:204E. Shift each absolute DWORD before subtracting LOW WORDs. Cache the
 * background even when2127 returnsFFFF for a clipped pixel. */
void BtechDrawRadarPositionMarker(const BtechPosition3D *position,uint16_t paletteIndex)
{
    if (!radarPlayerObject || !position) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !radarPlayerObject || !position");
    const BtechPosition3D *player=&radarPlayerObject->object.prefix.position;
    uint16_t playerX=BtechShiftSignedDwordRightLow(player->positionA,TERRAIN_WORLD_CELL_SHIFT);
    int16_t relativeX=BtechShiftSignedDwordRightLow(position->positionA,TERRAIN_WORLD_CELL_SHIFT)-playerX;
    uint16_t playerY=BtechShiftSignedDwordRightLow(player->positionB,TERRAIN_WORLD_CELL_SHIFT);
    int16_t relativeY=BtechShiftSignedDwordRightLow(position->positionB,TERRAIN_WORLD_CELL_SHIFT)-playerY;
    if (!radarRectangle) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !radarRectangle");
    int16_t halfWidth=(radarRectangle->width-(radarRectangle->width<0))/2;
    int16_t halfHeight=(radarRectangle->height-(radarRectangle->height<0))/2;
    int32_t absoluteX=relativeX==INT16_MIN?INT16_MAX:relativeX<0?-(int32_t)relativeX:relativeX;
    if (absoluteX>=halfWidth) return;
    int32_t absoluteY=relativeY==INT16_MIN?INT16_MAX:relativeY<0?-(int32_t)relativeY:relativeY;
    if (absoluteY>=halfHeight) return;
    uint16_t x=(uint16_t)(radarCentreX+relativeX),y=(uint16_t)(radarCentreY-relativeY);
    if (radarCachePage>=COMBAT_RADAR_CACHE_PAGES) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: radarCachePage>=COMBAT_RADAR_CACHE_PAGES");
    uint16_t marker=radarMarkerCounts[radarCachePage]++;
    if (marker>=COMBAT_RADAR_MARKERS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: marker>=COMBAT_RADAR_MARKERS"); /* Counter is already incremented. */
    radarMarkerX[marker][radarCachePage]=x;radarMarkerY[marker][radarCachePage]=y;
    radarMarkerBackground[marker][radarCachePage]=BtechReadClippedPixel((int16_t)x,(int16_t)y);
    BtechWriteClippedPixel((int16_t)x,(int16_t)y,paletteIndex);
}
