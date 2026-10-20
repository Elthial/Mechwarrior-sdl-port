#include "mechwarrior/combat_cockpit.h"
#include "mechwarrior/combat_import.h"
#include "mechwarrior/combat_orders.h"
#include "mechwarrior/combat_polygons.h"
#include <stdlib.h>

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
    screenPolygonNeedsClip=0;drawPageSelector=backPageSelector;
    if (!unitBindings[0].mech || unitBindings[0].mech->templateIndex>=COMBAT_MECH_TEMPLATES) abort();
    uint16_t layoutIndex=mechCockpitLayoutIndices[unitBindings[0].mech->templateIndex];
    if (layoutIndex>=COMBAT_COCKPIT_LAYOUTS) abort();
    const BtechHudGaugeAnchor *gauge=&cockpitLayouts[layoutIndex].heatGauge;
    uint16_t rowY=(uint16_t)(gauge->y-((uint16_t)lowerLevel<<1));
    for (int16_t row=0;row<(int16_t)difference;++row)
    {
        uint16_t rowX=(uint16_t)gauge->x;
        BtechDrawClippedLine((int16_t)rowX,(int16_t)rowY,(int16_t)(uint16_t)(rowX+10),(int16_t)rowY);
        if (!hudColourSchemeIndex)
        {
            BtechWriteClippedPixel((int16_t)(uint16_t)(gauge->x+10),(int16_t)rowY,pixelColour);
            BtechWriteClippedPixel((int16_t)(uint16_t)(gauge->x+9),(int16_t)rowY,pixelColour);
            BtechWriteClippedPixel((int16_t)(uint16_t)(gauge->x+3),(int16_t)rowY,pixelColour);
        }
        rowY-=2;
    }
    BtechCopyHudRectangleIfReady((uint16_t)gauge->x,rowY,0x12,(uint16_t)((difference<<1)+2));
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
    int16_t marker=forward>>2;
    uint16_t tickX=(uint16_t)(gauge->x+12);
    for (int16_t tick=0;tick<19;++tick)
    {
        if (tick==marker)
        {pixelColour=Combat_Pal_EGA_LightGrey;drawFillColour=(uint8_t)hudColourSchemes[hudColourSchemeIndex].selectedMovementTick;}
        BtechDrawClippedLine((int16_t)tickX,gauge->y,(int16_t)tickX,(int16_t)(uint16_t)(gauge->y+6));
        if (!hudColourSchemeIndex) BtechWriteClippedPixel((int16_t)tickX,gauge->y,pixelColour);
        tickX+=2;
    }
    drawFillColour=(uint8_t)hudColourSchemes[hudColourSchemeIndex].movementZeroReference;
    uint16_t zeroX=(uint16_t)(gauge->x+10);
    BtechDrawClippedLine((int16_t)zeroX,gauge->y,(int16_t)zeroX,(int16_t)(uint16_t)(gauge->y+6));
    if (!hudColourSchemeIndex) BtechWriteClippedPixel((int16_t)(uint16_t)(gauge->x+10),gauge->y,Combat_Pal_EGA_White);
    drawFillColour=(uint8_t)hudColourSchemes[hudColourSchemeIndex].reverseTicks;
    pixelColour=Combat_Pal_EGA_LightRed;marker=(int16_t)((reverse>>2)|((reverse&0x8000)?0xC000:0));
    tickX=(uint16_t)(gauge->x+8);
    for (int16_t tick=0;tick<9;++tick)
    {
        if (tick==marker)
        {drawFillColour=(uint8_t)hudColourSchemes[hudColourSchemeIndex].selectedMovementTick;pixelColour=Combat_Pal_EGA_LightGrey;}
        BtechDrawClippedLine((int16_t)tickX,gauge->y,(int16_t)tickX,(int16_t)(uint16_t)(gauge->y+6));
        if (!hudColourSchemeIndex) BtechWriteClippedPixel((int16_t)tickX,gauge->y,pixelColour);
        tickX-=2;
    }
    BtechPresentHudRectangleIfReady((uint16_t)gauge->x,(uint16_t)gauge->y,(uint16_t)(gauge->x+0x1E),(uint16_t)(gauge->y+7));
}

/*0810:63B8. Signed charge/20 display and separate charge500 readiness lamp. */
void BtechUpdateJumpChargeGauge(int16_t jumpCharge)
{
    enum { JUMP_GAUGE_CHARGE_PER_ROW=20,JUMP_GAUGE_READY_CHARGE=500 };
    int16_t level=jumpCharge/JUMP_GAUGE_CHARGE_PER_ROW;
    if (level==(int16_t)cachedJumpChargeGaugeLevel) return;
    drawPolygonFilled=1;screenPolygonNeedsClip=0;drawPageSelector=backPageSelector;
    if (!unitBindings[0].mech || unitBindings[0].mech->templateIndex>=COMBAT_MECH_TEMPLATES) abort();
    uint16_t layoutIndex=mechCockpitLayoutIndices[unitBindings[0].mech->templateIndex];
    if (layoutIndex>=COMBAT_COCKPIT_LAYOUTS || hudColourSchemeIndex>=COMBAT_HUD_COLOUR_SCHEMES) abort();
    const BtechHudGaugeAnchor *gauge=&cockpitLayouts[layoutIndex].jumpChargeGauge;
    uint16_t belowThreshold=jumpCharge<JUMP_GAUGE_READY_CHARGE;
    uint8_t thresholdColour=(uint8_t)(belowThreshold?hudColourSchemes[hudColourSchemeIndex].jumpBelowThreshold:hudColourSchemes[hudColourSchemeIndex].jumpCharged);
    drawLineColour=thresholdColour;drawFillColour=thresholdColour;
    BtechDrawStyledRectangle((uint16_t)(gauge->x+2),0x7C,3,3);
    if (!hudColourSchemeIndex)
    {
        uint16_t colour=belowThreshold?Combat_Pal_EGA_LightRed:Combat_Pal_EGA_LightGreen;
        BtechWriteClippedPixel((int16_t)(uint16_t)(gauge->x+3),0x7C,colour);
        BtechWriteClippedPixel((int16_t)(uint16_t)(gauge->x+4),0x7C,colour);
        BtechWriteClippedPixel((int16_t)(uint16_t)(gauge->x+4),0x7D,colour);
    }
    BtechCopyHudRectangleIfReady((uint16_t)(gauge->x+2),0x7C,3,3);
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
    BtechDrawStyledRectangle((uint16_t)gauge->x,(uint16_t)(rowY-difference+1),7,difference);
    if (!hudColourSchemeIndex)
    {
        drawFillColour=level<(int16_t)cachedJumpChargeGaugeLevel?Combat_Pal_EGA_White:Combat_Pal_EGA_LightCyan;
        uint16_t edgeX=(uint16_t)(gauge->x+6);
        BtechDrawClippedLine((int16_t)edgeX,(int16_t)rowY,(int16_t)edgeX,(int16_t)(uint16_t)(rowY-difference+1));
    }
    drawFillColour=(uint8_t)hudColourSchemes[hudColourSchemeIndex].jumpReference;
    uint16_t referenceX=(uint16_t)gauge->x;
    BtechDrawClippedLine((int16_t)referenceX,0x92,(int16_t)(uint16_t)(referenceX+6),0x92);
    if (!hudColourSchemeIndex) BtechWriteClippedPixel((int16_t)(uint16_t)(gauge->x+6),0x92,Combat_Pal_EGA_LightRed);
    BtechCopyHudRectangleIfReady((uint16_t)gauge->x,(uint16_t)(rowY-difference+1),8,(uint16_t)(difference+1));
    cachedJumpChargeGaugeLevel=(uint16_t)level;
}
