#include "mechwarrior/port_diagnostics.h"
#include "mechwarrior/combat_cockpit.h"
#include "mechwarrior/combat_input.h"
#include "mechwarrior/combat_orders.h"
#include "mechwarrior/combat_damage.h"
#include <stdlib.h>

enum { INSPECTION_DIVIDER_X=0x9D,INSPECTION_NAME_X=0x2E,INSPECTION_NAME_Y=4,
    INSPECTION_LABEL_X=4,INSPECTION_STATUS_X=0x58,INSPECTION_FIRST_SYSTEM_Y=0x12,INSPECTION_ROW_STEP=6,
    INSPECTION_HEAT_ROW_Y=0x2A,INSPECTION_HEAT_MAXIMUM_X=0x7C,INSPECTION_HEAT_OF_X=0x6A,
    INSPECTION_FIRST_ACTUATOR_Y=0x30,INSPECTION_JUMP_ROW_Y=0x48,INSPECTION_JUMP_MAXIMUM_X=0x76,INSPECTION_JUMP_OF_X=0x64,
    INSPECTION_WEAPON_HEADING_Y=0x5A,INSPECTION_FIRST_WEAPON_Y=0x66,INSPECTION_WEAPON_LOCATION_X=0x2E,
    INSPECTION_WEAPON_CONDITION_X=0x4C,INSPECTION_WEAPON_BANK_GAP=3,INSPECTION_WEAPON_SECOND_BANK=5,
    INSPECTION_LEGEND_FIRST_Y=0xAE,INSPECTION_LEGEND_SECOND_Y=0xB4,
    INSPECTION_DIAGRAM_X=0xA2,INSPECTION_DIAGRAM_Y=3,INSPECTION_DIAGRAM_WIDTH=0x94,INSPECTION_DIAGRAM_HEIGHT=0xBF,
    INSPECTION_FUNCTIONAL_LABEL=0,INSPECTION_DESTROYED_LABEL=5,
    INSPECTION_UNDAMAGED_COLOUR=Combat_Pal_EGA_LightGrey,INSPECTION_ABOVE_HALF_COLOUR=Combat_Pal_EGA_Yellow,INSPECTION_DAMAGED_COLOUR=Combat_Pal_EGA_Red,INSPECTION_DESTROYED_COLOUR=Combat_Pal_EGA_Black };

/*0810:13A8. One slot input. Setup latches, shared inspection pointers and
 * loaded bitmap ownership remain separate. Even an exit key draws one final
 * inspection frame BEFORE restoring cockpit resources and viewport. */
void BtechUpdateMechInspectionScreen(uint16_t unitSlot)
{
    if (unitSlot>=COMBAT_MECH_SLOTS || !unitBindings[unitSlot].mech) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: unitSlot>=COMBAT_MECH_SLOTS || !unitBindings[unitSlot].mech");
    inspectionMechState=unitBindings[unitSlot].mech;
    if (inspectionMechState->templateIndex>=COMBAT_MECH_TEMPLATES) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: inspectionMechState->templateIndex>=COMBAT_MECH_TEMPLATES");
    inspectionMechTemplate=&mechTemplates[inspectionMechState->templateIndex];
    if (inspectionReloadPending)
    {
        cockpitFullRedrawPending=0;
        BtechUnloadCockpitHudResources();
        BtechClearInspectionBackground();
        BtechSelectPaletteSlot(damagePaletteSlot);
        inspectionDamagePictures=BtechLoadBitmapList(inspectionDamageBitmapFilenames[inspectionMechState->templateIndex],NULL);
        if (!inspectionDamagePictures) BtechShowFatalErrorAndExit(errorInspectionShapeMemory);
        inspectionSetupComplete=1;inspectionReloadPending=0;
    }
    uint16_t scanCode=combatInputScanCode;
    switch (scanCode)
    {
    case COMBAT_KEY_Q:
    case COMBAT_KEY_SPACE:
        combatDispatchState=COMBAT_SCREEN_COCKPIT;inspectionReloadPending=1;cockpitFullRedrawPending=1;
        break;
    case COMBAT_KEY_P:BtechShowSimulationPausedDialog();break;
    case COMBAT_KEY_S:combatSoundEnabled=combatSoundEnabled==0;break;
    case COMBAT_KEY_C:combatDispatchState=PLAYER_SCREEN_ORDERS;inspectionReloadPending=1;break;
    default:break;
    }
    drawPageSelector=backPageSelector;
    BtechDrawMechInspectionStatus();
    BtechDrawMechInspectionDiagramAndLegend();
    BtechSwapScreenPages(false);
    BtechPresentScreenRectangle(0,0,COMBAT_SCREEN_WIDTH,COMBAT_SCREEN_HEIGHT);
    if (combatDispatchState!=PLAYER_SCREEN_TARGET_INSPECTION && combatDispatchState!=PLAYER_SCREEN_INSPECTION)
    {
        BtechUnloadCockpitHudResources();
        BtechClearInspectionBackground();
        BtechApplyLandscapeCameraColours();
        BtechLoadCockpitHudResources();
        BtechRefreshCockpitViewportAndGauges();
    }
}

/*0810:14B7. EGA colour mode0 genuinely does nothing. */
void BtechClearInspectionBackground(void)
{
    if (hudColourSchemeIndex)
    {
        drawPageSelector=backPageSelector;
        drawPolygonFilled=true;screenPolygonNeedsClip=false;drawLineColour=Combat_Pal_EGA_Black;drawFillColour=Combat_Pal_EGA_Black;
        BtechDrawStyledRectangle(0,0,COMBAT_SCREEN_WIDTH,COMBAT_SCREEN_HEIGHT);
        BtechCopyHudRectangleIfReady(0,0,COMBAT_SCREEN_WIDTH,COMBAT_SCREEN_HEIGHT);
    }
}

/*0810:14FF. Native ten equipment rows, signed mount count and condition
 * tables, not tabletop critical slots. Reload is not equipment damage. */
void BtechDrawMechInspectionStatus(void)
{
    const BtechMechState *mech=inspectionMechState;
    if (!mech || mech->templateIndex>=COMBAT_MECH_TEMPLATES || hudColourSchemeIndex>=COMBAT_HUD_COLOUR_SCHEMES) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !mech || mech->templateIndex>=COMBAT_MECH_TEMPLATES || hudColourSchemeIndex>=COMBAT_HUD_COLOUR_SCHEMES");
    const BtechMechTemplate *chassis=&mechTemplates[mech->templateIndex];
    const BtechHudColourScheme *colours=&hudColourSchemes[hudColourSchemeIndex];
    screenPolygonNeedsClip=false;drawFillColour=(uint8_t)colours->inspectionBackground;drawLineColour=Combat_Pal_EGA_Black;
    BtechDrawStyledRectangle(0,0,COMBAT_SCREEN_WIDTH,COMBAT_SCREEN_HEIGHT);
    BtechDrawClippedLine(INSPECTION_DIVIDER_X,0,INSPECTION_DIVIDER_X,COMBAT_SCREEN_HEIGHT);
    textForegroundColour=(uint8_t)colours->inspectionMechName;textBackgroundColour=Combat_Pal_EGA_Black;
    BtechDrawNearText(mechTemplateNames[mech->templateIndex],INSPECTION_NAME_X,INSPECTION_NAME_Y);
    textForegroundColour=(uint8_t)colours->inspectionSubsystemText;
    for (uint16_t system=0;system<5;++system)
        BtechDrawNearText(inspectionSubsystemLabels[system],INSPECTION_LABEL_X,INSPECTION_FIRST_SYSTEM_Y+system*INSPECTION_ROW_STEP);
    if (mech->engineHits>COMBAT_ENGINE_HIT_LIMIT) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: mech->engineHits>COMBAT_ENGINE_HIT_LIMIT");
    BtechDrawNearText(inspectionConditionLabels[engineConditionLabelIndices[mech->engineHits]],INSPECTION_STATUS_X,INSPECTION_FIRST_SYSTEM_Y);
    if (mech->gyroHits>COMBAT_GYRO_DESTROYED_HITS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: mech->gyroHits>COMBAT_GYRO_DESTROYED_HITS");
    BtechDrawNearText(inspectionConditionLabels[twoHitSystemConditionLabelIndices[mech->gyroHits]],INSPECTION_STATUS_X,INSPECTION_FIRST_SYSTEM_Y+INSPECTION_ROW_STEP);
    if (mech->sensorHits>COMBAT_SENSORS_DESTROYED_HITS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: mech->sensorHits>COMBAT_SENSORS_DESTROYED_HITS");
    BtechDrawNearText(inspectionConditionLabels[twoHitSystemConditionLabelIndices[mech->sensorHits]],INSPECTION_STATUS_X,INSPECTION_FIRST_SYSTEM_Y+2*INSPECTION_ROW_STEP);
    if (mech->lifeSupportHits>COMBAT_LIFE_SUPPORT_HIT_LIMIT) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: mech->lifeSupportHits>COMBAT_LIFE_SUPPORT_HIT_LIMIT");
    BtechDrawNearText(inspectionConditionLabels[twoHitSystemConditionLabelIndices[mech->lifeSupportHits]],INSPECTION_STATUS_X,INSPECTION_FIRST_SYSTEM_Y+3*INSPECTION_ROW_STEP);
    BtechDrawNearText(BtechFormatSignedDecimalField((int16_t)(chassis->heatSinkCount-mech->destroyedHeatSinks),2,' '),INSPECTION_STATUS_X,INSPECTION_HEAT_ROW_Y);
    BtechDrawNearText(BtechFormatSignedDecimalField((int16_t)chassis->heatSinkCount,2,' '),INSPECTION_HEAT_MAXIMUM_X,INSPECTION_HEAT_ROW_Y);
    BtechDrawNearText(heatSinksOfText,INSPECTION_HEAT_OF_X,INSPECTION_HEAT_ROW_Y);
    uint16_t rowY=INSPECTION_FIRST_ACTUATOR_Y;
    for (uint16_t actuator=0;actuator<COMBAT_ACTUATORS;++actuator,rowY+=INSPECTION_ROW_STEP)
    {
        BtechDrawNearText(actuatorLabels[actuator],INSPECTION_LABEL_X,rowY);
        uint16_t damageCount=COMBAT_ACTUATOR_CONDITION_FULL-mech->actuatorCondition[actuator];
        if (damageCount>COMBAT_ACTUATOR_CONDITION_FULL) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: damageCount>COMBAT_ACTUATOR_CONDITION_FULL");
        BtechDrawNearText(inspectionConditionLabels[actuatorConditionLabelIndices[damageCount]],INSPECTION_STATUS_X,rowY);
    }
    if (chassis->jumpJetGroupCount)
    {
        BtechDrawNearText(jumpJetsText,INSPECTION_LABEL_X,INSPECTION_JUMP_ROW_Y);
        BtechDrawNearText(BtechFormatSignedDecimalField((int16_t)(chassis->jumpJetGroupCount-mech->destroyedJumpJetGroups),1,' '),INSPECTION_STATUS_X,INSPECTION_JUMP_ROW_Y);
        BtechDrawNearText(BtechFormatSignedDecimalField((int16_t)chassis->jumpJetGroupCount,1,' '),INSPECTION_JUMP_MAXIMUM_X,INSPECTION_JUMP_ROW_Y);
        BtechDrawNearText(jumpJetsOfText,INSPECTION_JUMP_OF_X,INSPECTION_JUMP_ROW_Y);
    }
    textForegroundColour=(uint8_t)colours->inspectionEquipmentHeading;
    BtechDrawNearText(inspectionWeaponHeading,INSPECTION_LABEL_X,INSPECTION_WEAPON_HEADING_Y);
    textForegroundColour=(uint8_t)colours->inspectionEquipmentText;
    rowY=INSPECTION_FIRST_WEAPON_Y;
    for (int16_t mountIndex=0;mountIndex<COMBAT_WEAPON_MOUNTS;++mountIndex,rowY+=INSPECTION_ROW_STEP)
    {
        if (mountIndex==INSPECTION_WEAPON_SECOND_BANK) rowY+=INSPECTION_WEAPON_BANK_GAP;
        if ((int16_t)chassis->weaponMountCount>mountIndex)
        {
            const BtechWeaponMount *mount=&chassis->weaponMounts[mountIndex];
            if ((uint16_t)mount->weaponKind>=COMBAT_WEAPON_KINDS || (uint16_t)mount->bodyLocation>=COMBAT_BODY_LOCATIONS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: (uint16_t)mount->weaponKind>=COMBAT_WEAPON_KINDS || (uint16_t)mount->bodyLocation>=COMBAT_BODY_LOCATIONS");
            BtechDrawNearText(weaponLabels[mount->weaponKind],INSPECTION_LABEL_X,rowY);
            BtechDrawNearText(bodyLocationLabels[mount->bodyLocation],INSPECTION_WEAPON_LOCATION_X,rowY);
            BtechDrawNearText(inspectionConditionLabels[mech->weapons[mountIndex].operational?INSPECTION_FUNCTIONAL_LABEL:INSPECTION_DESTROYED_LABEL],INSPECTION_WEAPON_CONDITION_X,rowY);
        }
        else BtechDrawNearText(inspectionBlankWeaponRow,INSPECTION_LABEL_X,rowY);
    }
}

/*0810:18BE. Actual two-call NEAR boundary. */
void BtechDrawMechInspectionDiagramAndLegend(void)
{
    BtechDrawMechDamageDiagram();
    BtechDrawMechInspectionLegend();
}

/*0810:18CC. Combined armour+structure, signed wrapped WORD sums. Full
 * equality wins before zero (including0/0); rear centre torso has no box. */
void BtechDrawMechDamageDiagram(void)
{
    if (!inspectionMechState || !inspectionMechTemplate) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !inspectionMechState || !inspectionMechTemplate");
    drawLineColour=INSPECTION_UNDAMAGED_COLOUR;drawFillColour=INSPECTION_UNDAMAGED_COLOUR;
    BtechDrawStyledRectangle(INSPECTION_DIAGRAM_X,INSPECTION_DIAGRAM_Y,INSPECTION_DIAGRAM_WIDTH,INSPECTION_DIAGRAM_HEIGHT);
    for (uint16_t armor=0;armor<COMBAT_ARMOR_LOCATIONS;++armor)
    {
        if (armor==COMBAT_ARMOR_REAR_CENTER_TORSO) continue;
        uint16_t body=armorToBodyLocations[armor];
        if (body>=COMBAT_BODY_LOCATIONS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: body>=COMBAT_BODY_LOCATIONS");
        int16_t maximum=inspectionMechTemplate->maximumArmor[armor]+inspectionMechTemplate->maximumInternalStructure[body];
        int16_t remaining=inspectionMechState->currentArmor[armor]+inspectionMechState->currentInternalStructure[body];
        int16_t halfMaximum=(maximum-(maximum<0))/2; /* Native SAR, including negative odd sums. */
        uint8_t colour=remaining==maximum?INSPECTION_UNDAMAGED_COLOUR:remaining>halfMaximum?INSPECTION_ABOVE_HALF_COLOUR:remaining?INSPECTION_DAMAGED_COLOUR:INSPECTION_DESTROYED_COLOUR;
        uint16_t chassis=inspectionMechState->templateIndex;
        if (chassis>=COMBAT_MECH_TEMPLATES) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: chassis>=COMBAT_MECH_TEMPLATES");
        const BtechScreenRectangle *region=&inspectionDiagramRegions[chassis][armor];
        const BtechHudGaugeAnchor *origin=&inspectionDiagramOrigins[chassis];
        drawFillColour=colour;drawLineColour=colour;
        BtechDrawStyledRectangle((uint16_t)(region->x-origin->x+INSPECTION_DIAGRAM_X),
            (uint16_t)(region->y-origin->y+INSPECTION_DIAGRAM_Y),(uint16_t)region->width,(uint16_t)region->height);
    }
    if (!inspectionDamagePictures || !inspectionDamagePictures->images || !inspectionDamagePictures->imageCount) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !inspectionDamagePictures || !inspectionDamagePictures->images || !inspectionDamagePictures->imageCount");
    BtechDrawBitmap(&inspectionDamagePictures->images[0],INSPECTION_DIAGRAM_X,INSPECTION_DIAGRAM_Y);
}

/*0810:19F1. Does not set background/transparency or select a font. */
void BtechDrawMechInspectionLegend(void)
{
    if (hudColourSchemeIndex>=COMBAT_HUD_COLOUR_SCHEMES) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: hudColourSchemeIndex>=COMBAT_HUD_COLOUR_SCHEMES");
    textForegroundColour=(uint8_t)hudColourSchemes[hudColourSchemeIndex].inspectionFooter;
    BtechDrawNearText(inspectionLegendText[0],INSPECTION_LABEL_X,INSPECTION_LEGEND_FIRST_Y);
    BtechDrawNearText(inspectionLegendText[1],INSPECTION_LABEL_X,INSPECTION_LEGEND_SECOND_Y);
}
