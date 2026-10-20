#include "mechwarrior/combat_orders.h"
#include "mechwarrior/combat_import.h"
#include "mechwarrior/combat_cockpit.h"
#include "mechwarrior/combat_polygons.h"
#include "mechwarrior/combat_startup.h"
#include <stdlib.h>

/*17FB:0D3C. A pending full cockpit redraw suppresses this partial copy. */
void BtechCopyHudRectangleIfReady(uint16_t x,uint16_t y,uint16_t width,uint16_t height)
{
    if (!cockpitFullRedrawPending)
    {
        copySourcePageSelector=backPageSelector;
        drawPageSelector=frontPageSelector;
        BtechCopyScreenRectangle(x,y,width,height);
        drawPageSelector=backPageSelector;
    }
}

/*17FB:18FC. FAR signed DWORD right shift; native result is only LOW WORD. */
uint16_t BtechShiftSignedDwordRightLow(uint32_t value,uint16_t shifts)
{
    while (shifts--) value=(value>>1)|(value&UINT32_C(0x80000000));
    return (uint16_t)value;
}

/*17FB:059F. Strict map bounds. Neither output changes on either rejection. */
uint8_t BtechProjectPositionToOrderMap(const BtechPosition3D *position,int16_t *screenX,int16_t *screenY)
{
    if (orderMapScaleIndex>=ORDER_MAP_SCALE_COUNT || !orderMapCameraObject || !orderMapCameraContext) abort();
    uint32_t relative=position->positionA-orderMapCameraObject->object.prefix.position.positionA;
    uint16_t scaled=BtechShiftSignedDwordRightLow(relative,(uint16_t)(orderMapScaleShifts[orderMapScaleIndex]-7));
    BtechCameraContext *viewport=orderMapCameraContext;
    uint16_t midpoint=(uint16_t)viewport->clipMinimumA+(uint16_t)viewport->clipMaximumA;
    midpoint=(midpoint>>1)|(midpoint&0x8000);
    int16_t x=scaled+midpoint;
    if (x<=viewport->clipMinimumA || x>=viewport->clipMaximumA) return 0;
    relative=orderMapCameraObject->object.prefix.position.positionB-position->positionB;
    scaled=BtechShiftSignedDwordRightLow(relative,(uint16_t)(orderMapScaleShifts[orderMapScaleIndex]-7));
    viewport=orderMapCameraContext;
    midpoint=(uint16_t)((uint16_t)viewport->clipMinimumB+(uint16_t)viewport->clipMaximumB);
    midpoint=(midpoint>>1)|(midpoint&0x8000);
    int16_t y=scaled+midpoint;
    if (y<=viewport->clipMinimumB || y>=viewport->clipMaximumB) return 0;
    *screenX=x;*screenY=y;
    return 1;
}

/*17FB:0D6E. Authored masks are X-major; keep the original pixel call order. */
void BtechDrawThreeByThreeMarker(int16_t x,int16_t y,uint16_t colour,uint16_t style)
{
    if (style>=ORDER_MAP_MARKER_STYLES) abort(); /* Other near table/owner aliases are not extra styles. */
    for (uint16_t column=0;column<ORDER_MAP_MARKER_WIDTH;++column)
        for (uint16_t row=0;row<ORDER_MAP_MARKER_WIDTH;++row)
            if (orderMapMarkerMasks[style][column][row])
                BtechWriteClippedPixel((int16_t)(x+column-1),(int16_t)(y+row-1),colour);
}

/*17FB:0457. Side1 chooses remembered positions; side0 chooses numbered
 * styles and palette entries. These deliberately remain different tests. */
void BtechDrawLanceMapUnitMarkers(void)
{
    int16_t x,y;
    if (sideObjectiveRules[0]==OBJECTIVE_SPECIAL_TARGET || sideObjectiveRules[1]==OBJECTIVE_SPECIAL_TARGET)
    {
        if (!specialTargetObject) abort();
        if (BtechProjectPositionToOrderMap(&specialTargetObject->object.prefix.position,&x,&y))
            BtechDrawThreeByThreeMarker(x,y,0x0B,0);
    }
    for (uint16_t slot=0;slot<COMBAT_MECH_SLOTS;++slot)
    {
        const BtechUnitBinding *binding=&unitBindings[slot];
        if (!binding->mech) continue;
        const BtechUnitControlState *control=&unitControls[slot];
        BtechPosition3D remembered;
        const BtechPosition3D *position;
        if (control->sideIndex==1)
        {
            if (control->positionKnowledgeCode==COMBAT_POSITION_UNKNOWN) continue;
            if (control->positionKnowledgeCode==COMBAT_POSITION_REMEMBERED && (combatTickCounter&1)) continue;
            remembered=(BtechPosition3D){(uint32_t)control->lastVisiblePosition[0],(uint32_t)control->lastVisiblePosition[1],(uint32_t)control->lastVisiblePosition[2]};
            position=&remembered;
        }
        else
        {
            if (!binding->positionedLeftLeg) abort();
            position=&binding->positionedLeftLeg->object.prefix.position;
        }
        if (!BtechProjectPositionToOrderMap(position,&unitOrderMapScreenX[slot],&unitOrderMapScreenY[slot])) continue;
        uint16_t style=control->sideIndex==0?slot:0,paletteEntry=control->sideIndex==0?slot:4;
        if (combatLandscapeIndex>=COMBAT_LANDSCAPES || hudColourSchemeIndex>=COMBAT_HUD_COLOUR_SCHEMES || paletteEntry>=5) abort();
        uint16_t colour=landscapeCameraColourRows[hudColourSchemeIndex][combatLandscapeIndex].unitMarkerColours[paletteEntry];
        BtechDrawThreeByThreeMarker(unitOrderMapScreenX[slot],unitOrderMapScreenY[slot],colour,style);
    }
}

/*17FB:16E4. Animated world-boundary dashes; the last WITHDRAW mask wins.
 * Omit the boundary decoration when its corners are outside the map view. */
void BtechDrawLanceMapRetreatBoundaries(void)
{
    uint8_t retreatMask=0;
    for (uint16_t side=0;side<2;++side)
        if (sideObjectiveRules[side]==OBJECTIVE_WITHDRAW) retreatMask=scenarioSelectors.sideBoundaryMasks[side];
    int16_t left,right,top,bottom;
    uint8_t firstVisible=BtechProjectPositionToOrderMap(&combatWorldBoundaryCorners[0],&left,&top);
    uint8_t secondVisible=BtechProjectPositionToOrderMap(&combatWorldBoundaryCorners[1],&right,&bottom);
    if (!firstVisible || !secondVisible) return;
    clipMinimumY=top;clipMaximumY=bottom;clipMinimumX=left;clipMaximumX=right;
    screenPolygonNeedsClip=true;
    int16_t phase=(combatTickCounter&7)-4;
    if (combatLandscapeIndex>=COMBAT_LANDSCAPES || hudColourSchemeIndex>=COMBAT_HUD_COLOUR_SCHEMES) abort();
    const BtechLandscapeCameraColourRow *colours=&landscapeCameraColourRows[hudColourSchemeIndex][combatLandscapeIndex];
    drawFillColour=(uint8_t)((retreatMask&1)?colours->mapRetreatBoundaryColour:colours->mapBoundaryColour);
    for (int16_t x=left+phase;x<right;x=(int16_t)(x+8))
        BtechDrawClippedLine(x,top,(int16_t)(x+4),top);
    drawFillColour=(uint8_t)((retreatMask&2)?colours->mapRetreatBoundaryColour:colours->mapBoundaryColour);
    for (int16_t x=right-phase;x>left;x=(int16_t)(x-8))
        BtechDrawClippedLine(x,bottom,(int16_t)(x-4),bottom);
    drawFillColour=(uint8_t)((retreatMask&8)?colours->mapRetreatBoundaryColour:colours->mapBoundaryColour);
    for (int16_t y=top+phase-5;y<bottom;y=(int16_t)(y+8))
        BtechDrawClippedLine(right,y,right,(int16_t)(y+4));
    drawFillColour=(uint8_t)((retreatMask&4)?colours->mapRetreatBoundaryColour:colours->mapBoundaryColour);
    for (int16_t y=bottom-phase+5;y>top;y=(int16_t)(y-8))
        BtechDrawClippedLine(left,y,left,(int16_t)(y-4));
}
