#include "mechwarrior/combat_cockpit.h"
#include "mechwarrior/combat_pose.h"
#include "mechwarrior/combat_startup.h"
#include "mechwarrior/combat_import.h"
#include "mechwarrior/combat_scene.h"
#include "mechwarrior/combat_render.h"
#include <stdlib.h>

/*0810:02D1. Native pose, world ordering, aim/probe and CF5C object rendering.
 * Selection pixels restore and0886 removes temporary Mech body headings.
 * HUD retains its documented gates; final text/page handling is native. */
void BtechRenderCockpitSceneAndHud(void)
{
    if (!playerUnitBinding) abort();
    BtechPooledWorldObject *torso=playerUnitBinding->torso;
    BtechPooledWorldObject *positionedBody=playerUnitBinding->positionedLeftLeg;
    if (!playerPoseCameraContext) abort();
    if (playerImpactThisTick && playerPoseCameraContext->fillPaletteA!=COMBAT_COCKPIT_IMPACT_COLOUR)
        playerPoseCameraContext->fillPaletteA=COMBAT_COCKPIT_IMPACT_COLOUR;
    else
    {
        if (playerCameraFillASuppressed) playerPoseCameraContext->fillPaletteA=0;
        else
        {
            if (combatLandscapeIndex>=COMBAT_LANDSCAPES || hudColourSchemeIndex>=COMBAT_HUD_COLOUR_SCHEMES) abort();
            playerPoseCameraContext->fillPaletteA=landscapeCameraColourRows[hudColourSchemeIndex][combatLandscapeIndex].playerFillPaletteA;
        }
    }
    BtechApplyBodyHeadingToMechParts();
    if (!torso) abort();
    BtechUpdatePlayerMechPose(&torso->object.prefix);
    drawPolygonFilled=true;
    cockpitWorldObjectCount=(uint16_t)BtechFilterNearbyWorldObjects(playerPoseCameraContext,worldObjects,
        (int16_t)worldObjectCount,cockpitWorldObjects,(int16_t)cockpitWorldObjectCount);
    BtechSortFarWorldObjectsForView(playerPoseCameraContext,cockpitWorldObjects,
        (int16_t)cockpitWorldObjectCount,&objectSortOrSelectionScratch);
    BtechOrderMechRenderParts(playerPoseCameraContext,cockpitWorldObjects,(int16_t)cockpitWorldObjectCount);
    if (!playerMechState) abort();
    uint32_t aimCosine=(int16_t)BtechLookupCosine(playerMechState->aimPitch);
    uint32_t aimSine=(int16_t)BtechLookupSine(playerMechState->aimPitch);
    uint32_t projectedAim=BtechDivideSignedDword(BtechShiftDwordLeft(aimSine,(uint8_t)viewScaleShift),aimCosine);
    uint16_t viewportYSum=playerPoseCameraContext->clipMaximumB+playerPoseCameraContext->clipMinimumB;
    aimReticleScreenY=(uint16_t)((uint16_t)projectedAim+(uint16_t)((int16_t)viewportYSum>>1));
    drawPageSelector=backPageSelector;
    uint16_t probeY=BtechRandomSignedSpread(heatEffectTiers[0])+aimReticleScreenY;
    uint16_t probeX=BtechRandomSignedSpread(heatEffectTiers[0])+COMBAT_SCREEN_WIDTH/2;
    BtechBeginSelectionPixelProbe(probeX,probeY);
    BtechRenderObjectList(playerPoseCameraContext,cockpitWorldObjects,(int16_t)cockpitWorldObjectCount);
    BtechRemoveBodyHeadingFromMechParts();
    if (!positionedBody) abort();
    BtechDrawCockpitHeadingStrip(positionedBody->object.prefix.orientationC,(int16_t)torso->object.prefix.orientationC);
    BtechRefreshCockpitAndSensorTarget();
    BtechDrawPendingFiringIndicator();
    BtechDrawCockpitBitmapPlacements();
    textTransparentBackground=true;
    textForegroundColour=landscapeCameraColourRows[hudColourSchemeIndex][combatLandscapeIndex].hudReticleColour;
    uint16_t zoomX=COCKPIT_RIGHT_LABEL_X,automaticWeaponsX=COCKPIT_LEFT_LABEL_X;
    if (playerMechState->templateIndex>=COMBAT_MECH_TEMPLATES) abort();
    if (mechCockpitLayoutIndices[playerMechState->templateIndex]==2)
    {zoomX=COCKPIT_LEFT_LABEL_X;automaticWeaponsX=COCKPIT_RIGHT_LABEL_X;}
    BtechDrawNearText(zoomLabel,zoomX,COCKPIT_STATUS_TEXT_Y);
    BtechDrawNearText(BtechFormatSignedDecimalField((int16_t)(viewScaleShift-COCKPIT_ZOOM_SHIFT_BASE),1,' '),zoomX+COCKPIT_ZOOM_VALUE_X_OFFSET,COCKPIT_STATUS_TEXT_Y);
    if (mountGroupMode) BtechDrawNearText(automaticWeaponsLabel,automaticWeaponsX,COCKPIT_STATUS_TEXT_Y);
    if (heatInhibitFlags[0] && (combatTickCounter&COCKPIT_WARNING_FLASH_MASK))
    {
        textForegroundColour=COCKPIT_REACTOR_WARNING_COLOUR;
        BtechDrawNearText(reactorShutdownText,COCKPIT_REACTOR_WARNING_X,COCKPIT_REACTOR_WARNING_Y);
    }
    textTransparentBackground=false;
    BtechSwapScreenPages(false);
    if (cockpitFullRedrawPending)
    {
        BtechSetOverscanColour(Combat_Pal_EGA_Black);
        BtechPresentScreenRectangle(0,0,COMBAT_SCREEN_WIDTH,COMBAT_SCREEN_HEIGHT);
        cockpitFullRedrawPending=0;
        drawPageSelector=backPageSelector;copySourcePageSelector=frontPageSelector;
        BtechCopyScreenRectangle(0,0,COMBAT_SCREEN_WIDTH,COMBAT_SCREEN_HEIGHT);
    }
    else BtechPresentScreenRectangle(playerPoseCameraContext->clipMinimumA,playerPoseCameraContext->clipMinimumB,
        (uint16_t)(playerPoseCameraContext->clipMaximumA-playerPoseCameraContext->clipMinimumA),
        (uint16_t)(playerPoseCameraContext->clipMaximumB-playerPoseCameraContext->clipMinimumB+1));
    drawPolygonFilled=true;screenPolygonNeedsClip=true;
}
