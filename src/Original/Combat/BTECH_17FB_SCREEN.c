#include "mechwarrior/port_diagnostics.h"
#include "mechwarrior/combat_orders.h"
#include "mechwarrior/combat_cockpit.h"
#include "mechwarrior/combat_input.h"
#include "mechwarrior/combat_render.h"
#include "mechwarrior/combat_polygons.h"
#include <stdlib.h>

enum { ORDER_REDUCED_PANEL_X=0x6D,ORDER_REDUCED_PANEL_Y=0x7A,
    ORDER_REDUCED_PANEL_WIDTH=0x67,ORDER_REDUCED_PANEL_HEIGHT=0x4A,
    ORDER_REDUCED_BORDER_LEFT=0x70,ORDER_REDUCED_BORDER_RIGHT=0xD2,
    ORDER_REDUCED_BORDER_TOP=0x78,ORDER_REDUCED_BORDER_BOTTOM=0xC4,
    ORDER_PANEL_BACKGROUND=Combat_Pal_EGA_LightGrey,ORDER_PANEL_SHADOW=Combat_Pal_EGA_DarkGrey,ORDER_PANEL_HIGHLIGHT=Combat_Pal_EGA_White,
    ORDER_DESTINATION_BLINK_MASK=2,ORDER_DESTINATION_MARKER_INSET=2,ORDER_DESTINATION_MARKER_SIZE=5 };

/*17FB:00DF. Actual FAR command-screen refresh. The full-map capture excludes
 * markers/tracking/panels; cached frames restore it before drawing overlays.
 * An editor screen change returns without presenting another screen here. */
void BtechRefreshLanceOrderScreen(void)
{
    uint16_t redrawRequired=0;
    drawPageSelector=backPageSelector;
    if (orderScreenFullRedrawPending)
    {
        cockpitFullRedrawPending=0;
        screenPolygonNeedsClip=false;
        BtechLoadScreenImage("map.scr",NULL);
        if (reducedMemoryMode)
        {
            drawLineColour=ORDER_PANEL_BACKGROUND;drawFillColour=ORDER_PANEL_BACKGROUND;
            drawPolygonFilled=true;
            BtechDrawStyledRectangle(ORDER_REDUCED_PANEL_X,ORDER_REDUCED_PANEL_Y,ORDER_REDUCED_PANEL_WIDTH,ORDER_REDUCED_PANEL_HEIGHT);
            drawFillColour=ORDER_PANEL_SHADOW;
            BtechDrawClippedLine(ORDER_REDUCED_BORDER_LEFT,ORDER_REDUCED_BORDER_TOP,ORDER_REDUCED_BORDER_RIGHT,ORDER_REDUCED_BORDER_TOP);
            BtechDrawClippedLine(ORDER_REDUCED_BORDER_LEFT,ORDER_REDUCED_BORDER_BOTTOM,ORDER_REDUCED_BORDER_RIGHT,ORDER_REDUCED_BORDER_BOTTOM);
            drawFillColour=ORDER_PANEL_HIGHLIGHT;
            BtechDrawClippedLine(ORDER_REDUCED_BORDER_LEFT,ORDER_REDUCED_BORDER_TOP+1,ORDER_REDUCED_BORDER_RIGHT,ORDER_REDUCED_BORDER_TOP+1);
        }
        previousOrderHighlightPanel=ORDER_NO_HIGHLIGHT;
        previousOrderHighlightRow=ORDER_NO_HIGHLIGHT;
        BtechUpdateLanceOrderHighlight();
        orderMapPositionMask=BtechBuildOrderMapPositionMask();
    }
    if (!orderMapCameraObject || !orderMapCameraContext || orderMapScaleIndex>=ORDER_MAP_SCALE_COUNT) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !orderMapCameraObject || !orderMapCameraContext || orderMapScaleIndex>=ORDER_MAP_SCALE_COUNT");
    if (orderMapScaleIndex==ORDER_MAP_FULL_SCALE)
    {
        if (previousOrderMapScaleIndex!=ORDER_MAP_FULL_SCALE || orderScreenFullRedrawPending)
        {
            orderMapCameraObject->object.prefix.position.positionB=0;
            orderMapCameraObject->object.prefix.position.positionA=0;
            redrawRequired=1;
        }
    }
    else
    {
        BtechPosition3D *camera=&orderMapCameraObject->object.prefix.position;
        if (!playerUnitBinding || !playerUnitBinding->positionedLeftLeg) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !playerUnitBinding || !playerUnitBinding->positionedLeftLeg");
        camera->positionA=playerUnitBinding->positionedLeftLeg->object.prefix.position.positionA&orderMapPositionMask;
        /* Native reloads camera/binding/object after the X write. */
        camera=&orderMapCameraObject->object.prefix.position;
        if (!playerUnitBinding || !playerUnitBinding->positionedLeftLeg) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !playerUnitBinding || !playerUnitBinding->positionedLeftLeg");
        camera->positionB=playerUnitBinding->positionedLeftLeg->object.prefix.position.positionB&orderMapPositionMask;
        redrawRequired=1;
    }
    orderMapCameraObject->object.prefix.position.positionC=
        BtechShiftDwordLeftPair(1,orderMapScaleShifts[orderMapScaleIndex]);
    drawPolygonFilled=true;drawPageSelector=backPageSelector;
    if (redrawRequired)
    {
        BtechRenderObjectListVeneer(orderMapCameraContext,selectedWorldObjects,(int16_t)selectedObjectCount);
        if (orderMapScaleIndex==ORDER_MAP_FULL_SCALE)
        {
            copySourcePageSelector=backPageSelector;
            const BtechCameraContext *viewport=orderMapCameraContext;
            BtechCaptureScreenRectangle(orderPanelCaptureBuffer,(uint16_t)viewport->clipMinimumA,(uint16_t)viewport->clipMinimumB,
                (uint16_t)(viewport->clipMaximumA-viewport->clipMinimumA),(uint16_t)(viewport->clipMaximumB-viewport->clipMinimumB+1));
        }
    }
    else
    {
        const BtechCameraContext *viewport=orderMapCameraContext;
        BtechRestoreScreenRectangle(orderPanelCaptureBuffer,(uint16_t)viewport->clipMinimumA,(uint16_t)viewport->clipMinimumB,
            (uint16_t)(viewport->clipMaximumA-viewport->clipMinimumA),(uint16_t)(viewport->clipMaximumB-viewport->clipMinimumB+1));
    }
    for (uint16_t panel=0;panel<ORDER_WINGMATE_PANELS;++panel)
        lanceOrderControls[panel]=unitBindings[panel+1].mech?&unitControls[panel+1]:NULL;
    screenPolygonNeedsClip=false;
    BtechDrawLanceMapUnitMarkers();
    if (lanceOrderControls[0])
    {
        if (selectedLanceOrderPanel>=ORDER_WINGMATE_PANELS || !lanceOrderControls[selectedLanceOrderPanel]) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: selectedLanceOrderPanel>=ORDER_WINGMATE_PANELS || !lanceOrderControls[selectedLanceOrderPanel]");
        const BtechUnitControlState *control=lanceOrderControls[selectedLanceOrderPanel];
        uint16_t unitSlot=control->unitSlot;
        if (control->trackedPosition[2]>=0 && control->combatValueOrBoundaryCode>0)
        {
            /* A typed read-only argument value replaces native control+20,
             * not a cast between unrelated C struct and int32 array types. */
            BtechPosition3D trackedPosition={(uint32_t)control->trackedPosition[0],(uint32_t)control->trackedPosition[1],(uint32_t)control->trackedPosition[2]};
            int16_t trackedX,trackedY;
            if (BtechProjectPositionToOrderMap(&trackedPosition,&trackedX,&trackedY))
            {
                if (hudColourSchemeIndex>=COMBAT_HUD_COLOUR_SCHEMES || combatLandscapeIndex>=COMBAT_LANDSCAPES || unitSlot>=COMBAT_MECH_SLOTS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: hudColourSchemeIndex>=COMBAT_HUD_COLOUR_SCHEMES || combatLandscapeIndex>=COMBAT_LANDSCAPES || unitSlot>=COMBAT_MECH_SLOTS");
                drawFillColour=(uint8_t)landscapeCameraColourRows[hudColourSchemeIndex][combatLandscapeIndex].orderTrackingLineColour;
                BtechDrawClippedLine(unitOrderMapScreenX[unitSlot],unitOrderMapScreenY[unitSlot],trackedX,trackedY);
                if ((orderPanelMode==ORDER_PANEL_DESTINATION || orderPanelMode==ORDER_PANEL_TARGET) && (combatTickCounter&ORDER_DESTINATION_BLINK_MASK))
                {
                    drawFillColour=Combat_Pal_EGA_Black;drawPolygonFilled=false;
                    BtechDrawStyledRectangle((uint16_t)(trackedX-ORDER_DESTINATION_MARKER_INSET),(uint16_t)(trackedY-ORDER_DESTINATION_MARKER_INSET),ORDER_DESTINATION_MARKER_SIZE,ORDER_DESTINATION_MARKER_SIZE);
                }
            }
        }
    }
    if (orderMapScaleIndex==ORDER_MAP_FULL_SCALE) BtechDrawLanceMapRetreatBoundaries();
    screenPolygonNeedsClip=false;drawPolygonFilled=false;
    previousOrderMapScaleIndex=orderMapScaleIndex;
    BtechEditSelectedUnitOrders();
    if (combatDispatchState!=PLAYER_SCREEN_ORDERS)
    {screenPolygonNeedsClip=true;drawPolygonFilled=true;return;}
    BtechDrawUnitOrderPanels();
    while (BtechGetActiveRuntimeDelayFar())
    {if (!btechHostPollInterrupts) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !btechHostPollInterrupts");btechHostPollInterrupts();}
    BtechSwapScreenPages(false);
    if (orderScreenFullRedrawPending)
    {
        BtechPresentScreenRectangle(0,0,COMBAT_SCREEN_WIDTH,COMBAT_SCREEN_HEIGHT);
        copySourcePageSelector=frontPageSelector;drawPageSelector=backPageSelector;
        BtechCopyScreenRectangle(0,0,COMBAT_SCREEN_WIDTH,COMBAT_SCREEN_HEIGHT);
        orderScreenFullRedrawPending=0;
    }
    else
    {
        const BtechCameraContext *viewport=orderMapCameraContext;
        BtechPresentScreenRectangle(viewport->clipMinimumA,viewport->clipMinimumB,
            (uint16_t)(viewport->clipMaximumA-viewport->clipMinimumA),(uint16_t)(viewport->clipMaximumB-viewport->clipMinimumB+1));
    }
}
