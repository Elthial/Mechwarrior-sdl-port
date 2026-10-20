#include "mechwarrior/combat_cockpit.h"
#include "mechwarrior/combat_startup.h"
#include "mechwarrior/combat_import.h"
#include "mechwarrior/combat_polygons.h"
#include <stdlib.h>

enum { AIM_RETICLE_VIEW_SHIFT=7, AIM_RETICLE_OUTER_RADIUS=6, AIM_RETICLE_INNER_RADIUS=2,
    COCKPIT_BITMAP_END_INDEX=12, COCKPIT_BITMAP_X_ALIGNMENT_MASK=0xFFF8,
    FIRING_INDICATOR_BANK_ONE_BITMAP=9, FIRING_INDICATOR_BANK_ZERO_BITMAP=6,
    FIRING_INDICATOR_UPPER_PITCH=0x0888, FIRING_INDICATOR_LOWER_PITCH=0xF778 };

/*0810:024B. Genuine FAR guard around the selected present-rectangle entry. */
void BtechPresentHudRectangleIfReady(uint16_t x,uint16_t y,uint16_t width,uint16_t height)
{
    if (!cockpitFullRedrawPending) BtechPresentScreenRectangle((int16_t)x,(int16_t)y,width,height);
}

/*0810:05FD. Actual lamps/gauges/radar, sensor flicker and target/weapon HUD. */
void BtechRefreshCockpitAndSensorTarget(void)
{
    BtechPooledWorldObject *movementObject=unitBindings[0].positionedLeftLeg;
    if (!combatTickCounter) rememberedSensorTargetSlot=0;
    drawPageSelector=backPageSelector;screenPolygonNeedsClip=false;drawPolygonFilled=true;
    BtechDrawMechSubsystemLamps(playerMechState);
    if (!playerMechState) abort();
    BtechUpdateHeatGauge((int16_t)playerMechState->heatUnits/0x50);
    if (!movementObject) abort();
    BtechDrawForwardMovementGauge(movementObject->forwardStepScale);
    if (!playerMechState || playerMechState->templateIndex>=COMBAT_MECH_TEMPLATES) abort();
    if (mechCockpitLayoutIndices[playerMechState->templateIndex]!=2)
        BtechUpdateJumpChargeGauge((int16_t)playerMechState->jumpCharge);
    BtechUpdateRadarPanel();
    uint16_t sensorAvailable=0;
    if ((int16_t)playerMechState->sensorHits<2)
        sensorAvailable=((uint8_t)(BtechRandomBelowBound(3)+(uint8_t)combatTickCounter)&8)!=0;
    sensorAvailable|=playerMechState->sensorHits<1; /*069B is UNSIGNED. */
    if (!playerTargetSlot && sensorAvailable)
    {
        if (BtechTestPlayerTargetRenderVisibility((int16_t)rememberedSensorTargetSlot)) playerTargetSlot=rememberedSensorTargetSlot;
    }
    else if (playerTargetSlot)
    {
        if (!BtechTestPlayerTargetRenderVisibility((int16_t)playerTargetSlot) || !sensorAvailable)
        {rememberedSensorTargetSlot=playerTargetSlot;playerTargetSlot=0;}
    }
    BtechDrawWeaponStatusList(selectedWeaponMount,playerTargetSlot);
    BtechUpdateTargetPanelAndReticle(playerTargetSlot);
    if (sensorAvailable) BtechDrawCockpitAimReticle(COMBAT_SCREEN_WIDTH/2,aimReticleScreenY);
}

/*0810:0561. Four short lines only at native view shift07. */
void BtechDrawCockpitAimReticle(uint16_t x,uint16_t y)
{
    if (viewScaleShift!=AIM_RETICLE_VIEW_SHIFT) return;
    drawPageSelector=backPageSelector;
    if (combatLandscapeIndex>=COMBAT_LANDSCAPES || hudColourSchemeIndex>=COMBAT_HUD_COLOUR_SCHEMES) abort();
    drawFillColour=landscapeCameraColourRows[hudColourSchemeIndex][combatLandscapeIndex].hudReticleColour;
    BtechDrawClippedLine((int16_t)x,(int16_t)(uint16_t)(y-AIM_RETICLE_OUTER_RADIUS),(int16_t)x,(int16_t)(uint16_t)(y-AIM_RETICLE_INNER_RADIUS));
    BtechDrawClippedLine((int16_t)x,(int16_t)(uint16_t)(y+AIM_RETICLE_INNER_RADIUS),(int16_t)x,(int16_t)(uint16_t)(y+AIM_RETICLE_OUTER_RADIUS));
    BtechDrawClippedLine((int16_t)(uint16_t)(x-AIM_RETICLE_OUTER_RADIUS),(int16_t)y,(int16_t)(uint16_t)(x-AIM_RETICLE_INNER_RADIUS),(int16_t)y);
    BtechDrawClippedLine((int16_t)(uint16_t)(x+AIM_RETICLE_INNER_RADIUS),(int16_t)y,(int16_t)(uint16_t)(x+AIM_RETICLE_OUTER_RADIUS),(int16_t)y);
}

/*0810:0720. Up to six strut placements; sentinel12 and byte-aligned X. */
void BtechDrawCockpitBitmapPlacements(void)
{
    screenPolygonNeedsClip=false;
    if (activeCockpitLayoutIndex>=COMBAT_COCKPIT_LAYOUTS) abort();
    const BtechCockpitBitmapPlacement *placement=cockpitLayouts[activeCockpitLayoutIndex].bitmapPlacements;
    for (uint16_t index=0;index<COMBAT_COCKPIT_PLACEMENTS;++index,++placement)
    {
        if (placement->bitmapIndex==COCKPIT_BITMAP_END_INDEX) return;
        if (!strutPictures || placement->bitmapIndex>=strutPictures->imageCount) abort();
        BtechDrawBitmap(&strutPictures->images[placement->bitmapIndex],
            (int16_t)((uint16_t)placement->x&COCKPIT_BITMAP_X_ALIGNMENT_MASK),placement->y);
    }
}

/*0810:0773. Bank1 has priority; clear its BYTE latch only after drawing. */
void BtechDrawPendingFiringIndicator(void)
{
    if (!playerMechState) abort();
    int16_t pitch=(int16_t)playerMechState->aimPitch;
    uint16_t pitchClass=(pitch<FIRING_INDICATOR_UPPER_PITCH)+(pitch<(int16_t)FIRING_INDICATOR_LOWER_PITCH);
    if (activeCockpitLayoutIndex>=COMBAT_COCKPIT_LAYOUTS) abort();
    const BtechCockpitLayout *layout=&cockpitLayouts[activeCockpitLayoutIndex];
    if (firingIndicatorBank1Pending)
    {
        const BtechHudGaugeAnchor *anchor=&layout->firingIndicatorBank1[pitchClass];
        if (!cockpitPictures || FIRING_INDICATOR_BANK_ONE_BITMAP+pitchClass>=cockpitPictures->imageCount) abort();
        BtechDrawBitmap(&cockpitPictures->images[FIRING_INDICATOR_BANK_ONE_BITMAP+pitchClass],anchor->x,anchor->y);
        firingIndicatorBank1Pending=0;
    }
    else if (firingIndicatorBank0Pending)
    {
        const BtechHudGaugeAnchor *anchor=&layout->firingIndicatorBank0[pitchClass];
        if (!cockpitPictures || FIRING_INDICATOR_BANK_ZERO_BITMAP+pitchClass>=cockpitPictures->imageCount) abort();
        BtechDrawBitmap(&cockpitPictures->images[FIRING_INDICATOR_BANK_ZERO_BITMAP+pitchClass],anchor->x,anchor->y);
        firingIndicatorBank0Pending=0;
    }
}
