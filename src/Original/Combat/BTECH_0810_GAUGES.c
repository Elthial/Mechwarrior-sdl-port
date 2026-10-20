#include "mechwarrior/combat_cockpit.h"
#include "mechwarrior/combat_import.h"
#include "mechwarrior/combat_orders.h"
#include "mechwarrior/combat_polygons.h"
#include <stdlib.h>

enum { HEAT_GAUGE_ROW_SHIFT=1, HEAT_GAUGE_ROW_HEIGHT=2, HEAT_GAUGE_LAST_COLUMN=10,
    HEAT_GAUGE_EDGE_COLUMN=9, HEAT_GAUGE_INNER_COLUMN=3, HEAT_GAUGE_COPY_WIDTH=0x12,
    MOVEMENT_GAUGE_FORWARD_X=12, MOVEMENT_GAUGE_ZERO_X=10, MOVEMENT_GAUGE_REVERSE_X=8,
    MOVEMENT_GAUGE_FORWARD_TICKS=19, MOVEMENT_GAUGE_REVERSE_TICKS=9,
    MOVEMENT_GAUGE_TICK_SPACING=2, MOVEMENT_GAUGE_TICK_LAST_ROW=6,
    MOVEMENT_GAUGE_SCALE_SHIFT=2, MOVEMENT_GAUGE_PRESENT_WIDTH_BIAS=0x1E,
    MOVEMENT_GAUGE_PRESENT_HEIGHT_BIAS=7, JUMP_LAMP_X_INSET=2, JUMP_LAMP_Y=0x7C,
    JUMP_LAMP_SIZE=3, JUMP_LAMP_HIGHLIGHT_X=3, JUMP_LAMP_HIGHLIGHT_RIGHT_X=4,
    JUMP_LAMP_HIGHLIGHT_BOTTOM_Y=0x7D, JUMP_GAUGE_WIDTH=7,
    JUMP_GAUGE_LAST_COLUMN=6, JUMP_GAUGE_COPY_WIDTH=8, JUMP_GAUGE_BASELINE_Y=0x92 };

/*0810:60A0. Signed heat rows; cache updates AFTER the native partial copy. */
void BtechUpdateHeatGauge(int16_t heatLevel)
{
    if (heatLevel==(int16_t)cachedHeatGaugeLevel) return;
    int16_t lowerLevel=heatLevel;
    if (lowerLevel>(int16_t)cachedHeatGaugeLevel) lowerLevel=(int16_t)cachedHeatGaugeLevel;
    uint16_t difference;
    if ((uint16_t)(cachedHeatGaugeLevel-(uint16_t)heatLevel)==0x8000) difference=0x7FFF;
    else if ((int16_t)cachedHeatGaugeLevel<heatLevel) difference=(uint16_t)(heatLevel-cachedHeatGaugeLevel);
    else difference=(uint16_t)(cachedHeatGaugeLevel-heatLevel);
    if (hudColourSchemeIndex>=COMBAT_HUD_COLOUR_SCHEMES) abort();
    drawFillColour=(uint8_t)(heatLevel<(int16_t)cachedHeatGaugeLevel?
        hudColourSchemes[hudColourSchemeIndex].heatFalling:hudColourSchemes[hudColourSchemeIndex].heatRising);
    uint16_t pixelColour=heatLevel<(int16_t)cachedHeatGaugeLevel?Combat_Pal_EGA_White:Combat_Pal_EGA_LightRed;
    screenPolygonNeedsClip=false;drawPageSelector=backPageSelector;
    if (!unitBindings[0].mech || unitBindings[0].mech->templateIndex>=COMBAT_MECH_TEMPLATES) abort();
    uint16_t layoutIndex=mechCockpitLayoutIndices[unitBindings[0].mech->templateIndex];
    if (layoutIndex>=COMBAT_COCKPIT_LAYOUTS) abort();
    const BtechHudGaugeAnchor *gauge=&cockpitLayouts[layoutIndex].heatGauge;
    uint16_t rowY=(uint16_t)(gauge->y-((uint16_t)lowerLevel<<HEAT_GAUGE_ROW_SHIFT));
    for (int16_t row=0;row<(int16_t)difference;++row)
    {
        uint16_t rowX=(uint16_t)gauge->x;
        BtechDrawClippedLine((int16_t)rowX,(int16_t)rowY,(int16_t)(uint16_t)(rowX+HEAT_GAUGE_LAST_COLUMN),(int16_t)rowY);
        if (!hudColourSchemeIndex)
        {
            BtechWriteClippedPixel((int16_t)(uint16_t)(gauge->x+HEAT_GAUGE_LAST_COLUMN),(int16_t)rowY,pixelColour);
            BtechWriteClippedPixel((int16_t)(uint16_t)(gauge->x+HEAT_GAUGE_EDGE_COLUMN),(int16_t)rowY,pixelColour);
            BtechWriteClippedPixel((int16_t)(uint16_t)(gauge->x+HEAT_GAUGE_INNER_COLUMN),(int16_t)rowY,pixelColour);
        }
        rowY-=HEAT_GAUGE_ROW_HEIGHT;
    }
    BtechCopyHudRectangleIfReady((uint16_t)gauge->x,rowY,HEAT_GAUGE_COPY_WIDTH,(uint16_t)((difference<<HEAT_GAUGE_ROW_SHIFT)+HEAT_GAUGE_ROW_HEIGHT));
    cachedHeatGaugeLevel=(uint16_t)heatLevel;
}

/*0810:61F3. Native selected colour persists through the rest of each bank,
 * not merely its one marker tick. Final present takes x+1E/y+7 as dimensions. */
void BtechDrawForwardMovementGauge(int16_t forwardScale)
{
    if (!unitBindings[0].mech || unitBindings[0].mech->templateIndex>=COMBAT_MECH_TEMPLATES) abort();
    uint16_t layoutIndex=mechCockpitLayoutIndices[unitBindings[0].mech->templateIndex];
    if (layoutIndex>=COMBAT_COCKPIT_LAYOUTS || hudColourSchemeIndex>=COMBAT_HUD_COLOUR_SCHEMES) abort();
    const BtechHudGaugeAnchor *gauge=&cockpitLayouts[layoutIndex].movementGauge;
    int16_t forward=forwardScale<0?0:forwardScale;
    uint16_t reverse=forward?0:(uint16_t)(0-(uint16_t)forwardScale);
    drawFillColour=(uint8_t)hudColourSchemes[hudColourSchemeIndex].forwardTicks;
    uint16_t pixelColour=Combat_Pal_EGA_LightGreen;
    int16_t marker=forward>>MOVEMENT_GAUGE_SCALE_SHIFT;
    uint16_t tickX=(uint16_t)(gauge->x+MOVEMENT_GAUGE_FORWARD_X);
    for (int16_t tick=0;tick<MOVEMENT_GAUGE_FORWARD_TICKS;++tick)
    {
        if (tick==marker)
        {pixelColour=Combat_Pal_EGA_LightGrey;drawFillColour=(uint8_t)hudColourSchemes[hudColourSchemeIndex].selectedMovementTick;}
        BtechDrawClippedLine((int16_t)tickX,gauge->y,(int16_t)tickX,(int16_t)(uint16_t)(gauge->y+MOVEMENT_GAUGE_TICK_LAST_ROW));
        if (!hudColourSchemeIndex) BtechWriteClippedPixel((int16_t)tickX,gauge->y,pixelColour);
        tickX+=MOVEMENT_GAUGE_TICK_SPACING;
    }
    drawFillColour=(uint8_t)hudColourSchemes[hudColourSchemeIndex].movementZeroReference;
    uint16_t zeroX=(uint16_t)(gauge->x+MOVEMENT_GAUGE_ZERO_X);
    BtechDrawClippedLine((int16_t)zeroX,gauge->y,(int16_t)zeroX,(int16_t)(uint16_t)(gauge->y+MOVEMENT_GAUGE_TICK_LAST_ROW));
    if (!hudColourSchemeIndex) BtechWriteClippedPixel((int16_t)(uint16_t)(gauge->x+MOVEMENT_GAUGE_ZERO_X),gauge->y,Combat_Pal_EGA_White);
    drawFillColour=(uint8_t)hudColourSchemes[hudColourSchemeIndex].reverseTicks;
    pixelColour=Combat_Pal_EGA_LightRed;marker=(int16_t)((reverse>>MOVEMENT_GAUGE_SCALE_SHIFT)|((reverse&0x8000)?0xC000:0));
    tickX=(uint16_t)(gauge->x+MOVEMENT_GAUGE_REVERSE_X);
    for (int16_t tick=0;tick<MOVEMENT_GAUGE_REVERSE_TICKS;++tick)
    {
        if (tick==marker)
        {drawFillColour=(uint8_t)hudColourSchemes[hudColourSchemeIndex].selectedMovementTick;pixelColour=Combat_Pal_EGA_LightGrey;}
        BtechDrawClippedLine((int16_t)tickX,gauge->y,(int16_t)tickX,(int16_t)(uint16_t)(gauge->y+MOVEMENT_GAUGE_TICK_LAST_ROW));
        if (!hudColourSchemeIndex) BtechWriteClippedPixel((int16_t)tickX,gauge->y,pixelColour);
        tickX-=MOVEMENT_GAUGE_TICK_SPACING;
    }
    BtechPresentHudRectangleIfReady((uint16_t)gauge->x,(uint16_t)gauge->y,(uint16_t)(gauge->x+MOVEMENT_GAUGE_PRESENT_WIDTH_BIAS),(uint16_t)(gauge->y+MOVEMENT_GAUGE_PRESENT_HEIGHT_BIAS));
}

/*0810:63B8. Signed charge/20 display and separate charge500 readiness lamp. */
void BtechUpdateJumpChargeGauge(int16_t jumpCharge)
{
    enum { JUMP_GAUGE_CHARGE_PER_ROW=20,JUMP_GAUGE_READY_CHARGE=500 };
    int16_t level=jumpCharge/JUMP_GAUGE_CHARGE_PER_ROW;
    if (level==(int16_t)cachedJumpChargeGaugeLevel) return;
    drawPolygonFilled=true;screenPolygonNeedsClip=false;drawPageSelector=backPageSelector;
    if (!unitBindings[0].mech || unitBindings[0].mech->templateIndex>=COMBAT_MECH_TEMPLATES) abort();
    uint16_t layoutIndex=mechCockpitLayoutIndices[unitBindings[0].mech->templateIndex];
    if (layoutIndex>=COMBAT_COCKPIT_LAYOUTS || hudColourSchemeIndex>=COMBAT_HUD_COLOUR_SCHEMES) abort();
    const BtechHudGaugeAnchor *gauge=&cockpitLayouts[layoutIndex].jumpChargeGauge;
    uint16_t belowThreshold=jumpCharge<JUMP_GAUGE_READY_CHARGE;
    uint8_t thresholdColour=(uint8_t)(belowThreshold?hudColourSchemes[hudColourSchemeIndex].jumpBelowThreshold:hudColourSchemes[hudColourSchemeIndex].jumpCharged);
    drawLineColour=thresholdColour;drawFillColour=thresholdColour;
    BtechDrawStyledRectangle((uint16_t)(gauge->x+JUMP_LAMP_X_INSET),JUMP_LAMP_Y,JUMP_LAMP_SIZE,JUMP_LAMP_SIZE);
    if (!hudColourSchemeIndex)
    {
        uint16_t colour=belowThreshold?Combat_Pal_EGA_LightRed:Combat_Pal_EGA_LightGreen;
        BtechWriteClippedPixel((int16_t)(uint16_t)(gauge->x+JUMP_LAMP_HIGHLIGHT_X),JUMP_LAMP_Y,colour);
        BtechWriteClippedPixel((int16_t)(uint16_t)(gauge->x+JUMP_LAMP_HIGHLIGHT_RIGHT_X),JUMP_LAMP_Y,colour);
        BtechWriteClippedPixel((int16_t)(uint16_t)(gauge->x+JUMP_LAMP_HIGHLIGHT_RIGHT_X),JUMP_LAMP_HIGHLIGHT_BOTTOM_Y,colour);
    }
    BtechCopyHudRectangleIfReady((uint16_t)(gauge->x+JUMP_LAMP_X_INSET),JUMP_LAMP_Y,JUMP_LAMP_SIZE,JUMP_LAMP_SIZE);
    int16_t lowerLevel=level;
    if (lowerLevel>(int16_t)cachedJumpChargeGaugeLevel) lowerLevel=(int16_t)cachedJumpChargeGaugeLevel;
    uint16_t difference;
    if ((uint16_t)(cachedJumpChargeGaugeLevel-(uint16_t)level)==0x8000) difference=0x7FFF;
    else if ((int16_t)cachedJumpChargeGaugeLevel<level) difference=(uint16_t)(level-cachedJumpChargeGaugeLevel);
    else difference=(uint16_t)(cachedJumpChargeGaugeLevel-level);
    uint8_t colour=(uint8_t)(level<(int16_t)cachedJumpChargeGaugeLevel?
        hudColourSchemes[hudColourSchemeIndex].jumpFalling:hudColourSchemes[hudColourSchemeIndex].jumpRising);
    drawLineColour=colour;drawFillColour=colour;
    uint16_t rowY=(uint16_t)(gauge->y-lowerLevel);
    BtechDrawStyledRectangle((uint16_t)gauge->x,(uint16_t)(rowY-difference+1),JUMP_GAUGE_WIDTH,difference);
    if (!hudColourSchemeIndex)
    {
        drawFillColour=level<(int16_t)cachedJumpChargeGaugeLevel?Combat_Pal_EGA_White:Combat_Pal_EGA_LightCyan;
        uint16_t edgeX=(uint16_t)(gauge->x+JUMP_GAUGE_LAST_COLUMN);
        BtechDrawClippedLine((int16_t)edgeX,(int16_t)rowY,(int16_t)edgeX,(int16_t)(uint16_t)(rowY-difference+1));
    }
    drawFillColour=(uint8_t)hudColourSchemes[hudColourSchemeIndex].jumpReference;
    uint16_t referenceX=(uint16_t)gauge->x;
    BtechDrawClippedLine((int16_t)referenceX,JUMP_GAUGE_BASELINE_Y,(int16_t)(uint16_t)(referenceX+6),0x92);
    if (!hudColourSchemeIndex) BtechWriteClippedPixel((int16_t)(uint16_t)(gauge->x+JUMP_GAUGE_LAST_COLUMN),JUMP_GAUGE_BASELINE_Y,Combat_Pal_EGA_LightRed);
    BtechCopyHudRectangleIfReady((uint16_t)gauge->x,(uint16_t)(rowY-difference+1),JUMP_GAUGE_COPY_WIDTH,(uint16_t)(difference+1));
    cachedJumpChargeGaugeLevel=(uint16_t)level;
}
