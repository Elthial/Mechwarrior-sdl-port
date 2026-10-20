#include "mechwarrior/combat_video.h"
#include "mechwarrior/combat_import.h"
#include "mechwarrior/combat_world.h"
#include "mechwarrior/combat_cockpit.h"
#include <stdlib.h>

/*0810:2678. Asymmetric native bounds: Y includes y+height, X subtracts1. */
void BtechSetCockpitCameraViewport(BtechCameraContext *camera)
{
    if (activeCockpitLayoutIndex>=COMBAT_COCKPIT_LAYOUTS || !camera) abort();
    const BtechCockpitLayout *layout=&cockpitLayouts[activeCockpitLayoutIndex];
    camera->clipMinimumB=layout->viewport.y;
    camera->clipMaximumB=(int16_t)((uint16_t)layout->viewport.y+(uint16_t)layout->viewport.height);
    camera->clipMinimumA=layout->viewport.x;
    camera->clipMaximumA=(int16_t)((uint16_t)layout->viewport.width+(uint16_t)layout->viewport.x-1);
}

/*0810:6094. The third cache WORD has no proved consumer/meaning yet. */
void BtechResetHudGaugeCaches(void)
{
    unidentifiedField25=0;
    cachedHeatGaugeLevel=0;
    cachedJumpChargeGaugeLevel=0;
}

/*0810:26C5. Ignore screen-loader result; restore only the saved clip BYTE.
 * Draw page, gauge caches, viewport and redraw latch retain native ordering. */
void BtechRefreshCockpitViewportAndGauges(void)
{
    uint8_t savedClipping=screenPolygonNeedsClip;
    if (!playerMechState || playerMechState->templateIndex>=COMBAT_MECH_TEMPLATES) abort();
    activeCockpitLayoutIndex=mechCockpitLayoutIndices[playerMechState->templateIndex];
    screenPolygonNeedsClip=false;
    drawPageSelector=backPageSelector;
    if (activeCockpitLayoutIndex>=COMBAT_COCKPIT_LAYOUTS) abort();
    BtechLoadScreenImage(cockpitScreenFilenames[activeCockpitLayoutIndex],NULL);
    screenPolygonNeedsClip=savedClipping;
    BtechResetHudGaugeCaches();
    BtechSetCockpitCameraViewport(playerPoseCameraContext);
    cockpitFullRedrawPending=1;
}

/*0810:2938. Genuine FAR veneer, no source arguments or BP result. */
void BtechRefreshCockpitViewportAndGaugesFar(void)
{
    BtechRefreshCockpitViewportAndGauges();
}

/*0810:139C. BYTE latch beside independent WORD; not a packed WORD store. */
void BtechResetMechInspectionState(void)
{
    inspectionReloadPending=1;
    inspectionSetupComplete=0;
}

/*0810:2716. Keep the first player store, then the shared map/player store,
 * and finally palette selection. Map fillA and neighbouring fields survive. */
void BtechApplyLandscapeCameraColours(void)
{
    if (combatLandscapeIndex>=COMBAT_LANDSCAPES || hudColourSchemeIndex>=COMBAT_HUD_COLOUR_SCHEMES
        || !playerPoseCameraContext || !orderMapCameraContext) abort(); /* Unrecovered unrelated-owner aliases. */
    playerPoseCameraContext->fillPaletteA=playerCameraFillASuppressed?0:
        landscapeCameraColourRows[hudColourSchemeIndex][combatLandscapeIndex].playerFillPaletteA;
    uint8_t sharedPalette=landscapeCameraColourRows[hudColourSchemeIndex][combatLandscapeIndex].sharedFillPaletteB;
    orderMapCameraContext->fillPaletteB=sharedPalette;
    playerPoseCameraContext->fillPaletteB=sharedPalette;
    BtechSelectPaletteSlot(landscapePaletteSlots[combatLandscapeIndex]);
}

/*0810:2372. Native five-bank loading order. EGA always uses HUD_NUMS;
 * non-EGA cyan HUD selection additionally requires landscape zero. */
void BtechLoadCockpitHudResources(void)
{
    pointPictures=BtechLoadBitmapList("point.bmp",NULL);
    smallMechPictures=BtechLoadBitmapList("sm_mechs.bmp",NULL);
    strutPictures=BtechLoadBitmapList("struts.bmp",NULL);
    cockpitPictures=BtechLoadBitmapList("cockpit.bmp",NULL);
    const char *filename=combatRendererKind!=COMBAT_VIDEO_EGA && combatLandscapeIndex==0?"hud_cyan.bmp":"hud_nums.bmp";
    hudGlyphPictures=BtechLoadBitmapList(filename,NULL);
}

/*0810:23D1. Live handle reads across each release; clear in reverse order. */
void BtechUnloadCockpitHudResources(void)
{
    BtechFreeResourceIfLoaded(pointPictures);
    BtechFreeResourceIfLoaded(smallMechPictures);
    BtechFreeResourceIfLoaded(strutPictures);
    BtechFreeResourceIfLoaded(cockpitPictures);
    BtechFreeResourceIfLoaded(hudGlyphPictures);
    BtechFreeResourceIfLoaded(inspectionDamagePictures);
    inspectionDamagePictures=NULL;
    hudGlyphPictures=NULL;
    cockpitPictures=NULL;
    strutPictures=NULL;
    smallMechPictures=NULL;
    pointPictures=NULL;
}

/*0810:2422. The native nullable guard belongs here, not inside1336. */
void BtechFreeResourceIfLoaded(BtechBitmapList *list)
{
    if (list) BtechFreeRegisteredBitmapList(list);
}
