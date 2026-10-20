#include "mechwarrior/combat_cockpit.h"
#include "mechwarrior/combat_import.h"
#include "mechwarrior/combat_render.h"
#include "mechwarrior/combat_orders.h"
#include "mechwarrior/combat_input.h"
#include <stdlib.h>

enum { BRIEFING_RESTORED_MAP_TOP=7, BRIEFING_RESTORED_MAP_BOTTOM=0x71,
    BRIEFING_RESTORED_MAP_LEFT=0x3E, BRIEFING_RESTORED_MAP_RIGHT=0x102 };

enum { BRIEFING_MISSION_LABEL_X=0x14,BRIEFING_MISSION_LABEL_Y=0x96,
    BRIEFING_MISSION_TITLE_X=0x19,BRIEFING_MISSION_TITLE_Y=0xA5,
    BRIEFING_ROSTER_LABEL_X=0xCF,BRIEFING_CREW_LABEL_Y=0x16,BRIEFING_MECH_LABEL_Y=0x46,
    BRIEFING_ROSTER_TEXT_X=0xD3,BRIEFING_STATUS_COLON_X=0x11B,BRIEFING_MECH_STATUS_X=0x121,
    BRIEFING_ROSTER_ROW_STEP=8,BRIEFING_MAP_LEFT=5,BRIEFING_MAP_RIGHT=0xC9,
    BRIEFING_MAP_TOP=0x0F,BRIEFING_MAP_BOTTOM=0x79 };

/*0810:4176. Map, markers, retreat boundaries, display copy and native input
 * wait, then remove only friendly bindings with a Mech but no head object. */
void BtechShowCombatStageBriefing(void)
{
    drawPageSelector=backPageSelector;
    BtechLoadScreenImage("status.scr",NULL);
    textTransparentBackground=true;
    screenPolygonNeedsClip=false;
    textForegroundColour=hudColourSchemeIndex?Combat_Pal_EGA_Black:COMBAT_BRIEFING_TEXT_COLOUR;
    BtechDrawNearText("YOUR NEXT MISSION:",BRIEFING_MISSION_LABEL_X,BRIEFING_MISSION_LABEL_Y);
    if (combatMissionDefinitionIndex>=COMBAT_BRIEFING_MISSIONS) abort(); /* Adjacent directory alias not mapped. */
    BtechDrawNearText(combatMissionTitles[combatMissionDefinitionIndex],BRIEFING_MISSION_TITLE_X,BRIEFING_MISSION_TITLE_Y);
    BtechDrawNearText("CREW STATUS:",BRIEFING_ROSTER_LABEL_X,BRIEFING_CREW_LABEL_Y);
    BtechDrawNearText("MECH STATUS:",BRIEFING_ROSTER_LABEL_X,BRIEFING_MECH_LABEL_Y);
    for (uint16_t slot=0;slot<COMBAT_FRIENDLY_SLOTS;++slot)
    {
        BtechMechState *mech=unitBindings[slot].mech;
        if (mech)
        {
            uint16_t rowY=(slot+1)*BRIEFING_ROSTER_ROW_STEP;
            if (deployedPilotIds[slot]>=COMBAT_PILOT_NAMES) abort();
            BtechDrawNearText(pilotDefinitionNames[deployedPilotIds[slot]],BRIEFING_ROSTER_TEXT_X,rowY+BRIEFING_CREW_LABEL_Y);
            BtechDrawNearText(":OK",BRIEFING_STATUS_COLON_X,rowY+BRIEFING_CREW_LABEL_Y);
            if (mech->templateIndex>=COMBAT_MECH_TEMPLATES) abort();
            BtechDrawNearText(mechTemplateNames[mech->templateIndex],BRIEFING_ROSTER_TEXT_X,rowY+BRIEFING_MECH_LABEL_Y);
            BtechDrawNearText(":",BRIEFING_STATUS_COLON_X,rowY+BRIEFING_MECH_LABEL_Y);
            const char *condition=BtechIsMechOperational(mech)?"OK":"JUNK";
            BtechDrawNearText(condition,BRIEFING_MECH_STATUS_X,rowY+BRIEFING_MECH_LABEL_Y);
        }
    }
    textTransparentBackground=false;
    if (!orderMapCameraObject) abort();
    orderMapCameraObject->object.prefix.position.positionC=COMBAT_BRIEFING_MAP_ALTITUDE;
    if (!orderMapCameraContext) abort();
    orderMapCameraContext->clipMinimumB=BRIEFING_MAP_TOP;
    orderMapCameraContext->clipMaximumB=BRIEFING_MAP_BOTTOM;
    orderMapCameraContext->clipMinimumA=BRIEFING_MAP_LEFT;
    orderMapCameraContext->clipMaximumA=BRIEFING_MAP_RIGHT;
    BtechRenderObjectListVeneer(orderMapCameraContext,selectedWorldObjects,(int16_t)selectedObjectCount);
    BtechDrawLanceMapUnitMarkers();
    BtechDrawLanceMapRetreatBoundaries();
    orderMapCameraContext->clipMinimumB=BRIEFING_RESTORED_MAP_TOP;
    orderMapCameraContext->clipMaximumB=BRIEFING_RESTORED_MAP_BOTTOM;
    orderMapCameraContext->clipMinimumA=BRIEFING_RESTORED_MAP_LEFT;
    orderMapCameraContext->clipMaximumA=BRIEFING_RESTORED_MAP_RIGHT;
    screenPolygonNeedsClip=false;
    BtechSetOverscanColour(Combat_Pal_EGA_LightGrey);
    BtechCopyHudRectangleIfReady(0,0,COMBAT_SCREEN_WIDTH,COMBAT_SCREEN_HEIGHT);
    BtechWaitForCombatInputReleaseThenPress();
    for (uint16_t slot=0;slot<COMBAT_LANCE_SLOTS;++slot)
        if (unitBindings[slot].mech && !unitBindings[slot].head)
            unitBindings[slot].mech=NULL;
}
