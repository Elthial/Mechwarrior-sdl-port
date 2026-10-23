#include "mechwarrior/port_diagnostics.h"
#include "mechwarrior/combat_orders.h"
#include "mechwarrior/combat_cockpit.h"
#include "mechwarrior/combat_import.h"
#include "mechwarrior/combat_tactics.h"
#include <stdlib.h>

enum { ORDER_WEAPON_TOTAL_X=0x42,ORDER_WEAPON_OPERATIONAL_X=0x24,
    ORDER_WEAPON_OF_X=0x30,ORDER_WEAPON_COLON_X=0x18,ORDER_SYSTEM_CONDITION_LEVELS=4 };

/*17FB:0DCB. Compact friendly panels from slots1..7, independently of the
 * fixed slot1/2/3 editor references. Preserve inherited text transparency,
 * full marker colour WORDs and the native condition-colour write order. */
void BtechDrawUnitOrderPanels(void)
{
    textBackgroundColour=Combat_Pal_EGA_Black;
    drawFillColour=ORDER_PANEL_TEXT_COLOUR;
    uint16_t panel=0,friendlyMarker=0;
    for (int16_t unitSlot=1;unitSlot<COMBAT_MECH_SLOTS;++unitSlot)
    {
        BtechUnitControlState *control=&unitControls[unitSlot];
        BtechMechState *mech=unitBindings[unitSlot].mech;
        if (!mech || control->sideIndex) continue;
        if (panel>=ORDER_WINGMATE_PANELS || mech->templateIndex>=COMBAT_MECH_TEMPLATES
            || hudColourSchemeIndex>=COMBAT_HUD_COLOUR_SCHEMES || combatLandscapeIndex>=COMBAT_LANDSCAPES) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: panel>=ORDER_WINGMATE_PANELS || mech->templateIndex>=COMBAT_MECH_TEMPLATES || hudColourSchemeIndex>=COMBAT_HUD_COLOUR_SCHEMES || combatLandscapeIndex>=COMBAT_LANDSCAPES");
        const BtechMechTemplate *chassis=&mechTemplates[mech->templateIndex];
        uint16_t x=lanceOrderPanelTextX[panel];
        int16_t trackedSlot=control->trackedUnitOrSpecialSlot;
        if (trackedSlot>0 && trackedSlot<COMBAT_MECH_SLOTS && unitControls[trackedSlot].conditionScoreOrExitCode<COMBAT_CONDITION_SCORE_MINIMUM)
        {
            control->trackedUnitOrSpecialSlot=-1;
            control->commandCode=COMBAT_ORDER_ACT_ON_OWN;
        } /* Navigation reference deliberately survives this reset. */
        ++friendlyMarker;
        const BtechHudColourScheme *colours=&hudColourSchemes[hudColourSchemeIndex];
        uint16_t markerColour=hudColourSchemeIndex?ORDER_PANEL_TEXT_COLOUR:
            landscapeCameraColourRows[hudColourSchemeIndex][combatLandscapeIndex].unitMarkerColours[friendlyMarker];
        textForegroundColour=(uint8_t)markerColour;
        if (panel!=selectedLanceOrderPanel || orderPanelMode==ORDER_PANEL_SUMMARY
            || orderPanelMode==ORDER_PANEL_TARGET || orderPanelMode==ORDER_PANEL_DESTINATION)
        {
            if (unitSlot>=COMBAT_FRIENDLY_SLOTS || deployedPilotIds[unitSlot]>=COMBAT_PILOT_NAMES) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: unitSlot>=COMBAT_FRIENDLY_SLOTS || deployedPilotIds[unitSlot]>=COMBAT_PILOT_NAMES"); /* Native slot4+ pilot-directory neighbour alias. */
            BtechDrawNearText(pilotDefinitionNameDirectory[deployedPilotIds[unitSlot]],x,ORDER_PANEL_NAME_Y);
            BtechDrawThreeByThreeMarker((uint16_t)(x+ORDER_PANEL_MARKER_X),ORDER_PANEL_MARKER_Y,markerColour,friendlyMarker);
            BtechDrawNearText(mechTemplateNameDirectory[mech->templateIndex],x,ORDER_PANEL_CHASSIS_Y);
            if (control->tacticalState>=ORDER_TACTICAL_LABELS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: control->tacticalState>=ORDER_TACTICAL_LABELS");
            BtechDrawNearText(tacticalOrderLabels[control->tacticalState],x,ORDER_PANEL_TACTICAL_Y);
            textForegroundColour=ORDER_PANEL_TEXT_COLOUR;
            uint16_t displayOrder=ORDER_DISPLAY_RETREATED;
            if (control->conditionScoreOrExitCode>COMBAT_EXIT_CODE_THRESHOLD)
            {
                displayOrder=control->commandCode;
                if (displayOrder)
                {
                    if (displayOrder>=ORDER_DISPLAY_CODES) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: displayOrder>=ORDER_DISPLAY_CODES");
                    displayOrder=orderDisplayIndices[displayOrder];
                }
            }
            if (displayOrder>=ORDER_LABELS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: displayOrder>=ORDER_LABELS");
            BtechDrawNearText(orderLabels[displayOrder],x,ORDER_PANEL_COMMAND_Y);
            const char *referenceLabel=orderLocationText;
            int16_t referenceSlot=control->navigationReferenceSlot;
            if (referenceSlot!=-1)
            {
                if (referenceSlot==ORDER_SPECIAL_TARGET_SLOT) referenceLabel=orderBaseText;
                else
                {
                    if ((uint16_t)referenceSlot>=COMBAT_MECH_SLOTS || !unitBindings[referenceSlot].mech) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: (uint16_t)referenceSlot>=COMBAT_MECH_SLOTS || !unitBindings[referenceSlot].mech");
                    uint16_t referenceChassis=unitBindings[referenceSlot].mech->templateIndex;
                    if (referenceChassis>=COMBAT_MECH_TEMPLATES) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: referenceChassis>=COMBAT_MECH_TEMPLATES");
                    referenceLabel=mechTemplateNameDirectory[referenceChassis];
                    textForegroundColour=(uint8_t)BtechGetMechConditionColour(referenceSlot);
                }
            }
            BtechDrawNearText(referenceLabel,x,ORDER_PANEL_REFERENCE_Y);
            BtechSetConditionTextColours(BtechGetMechConditionColour(unitSlot));
            BtechDrawNearText(orderDamageText,x,ORDER_PANEL_DAMAGE_Y);
            uint16_t mountedWeight=0,operationalWeight=0;
            for (uint16_t mount=0;mount<COMBAT_WEAPON_MOUNTS;++mount)
            {
                uint16_t kind=chassis->weaponMounts[mount].weaponKind;
                if (kind==COMBAT_WEAPON_NONE) continue;
                if (kind>=COMBAT_WEAPON_KINDS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: kind>=COMBAT_WEAPON_KINDS");
                uint16_t weight=weaponDefinitions[kind].damageOrMissileCount;
                if (mech->weapons[mount].operational) operationalWeight+=weight;
                mountedWeight+=weight;
            }
            if (!mountedWeight) /* Port emulation of a native CPU fault. */ MW_NATIVE_FAULT("Rejected condition: !mountedWeight"); /* Native IDIV fault, after the preceding damage text. */
            int32_t condition=(int16_t)(operationalWeight*ORDER_CONDITION_SCALE)/(int16_t)mountedWeight;
            if (condition<0 || condition>=ORDER_CONDITION_BANDS) /* Port emulation of a native CPU fault. */ MW_NATIVE_FAULT("Rejected condition: condition<0 || condition>=ORDER_CONDITION_BANDS"); /* Unrecovered adjacent palette/IDIV-overflow aliases. */
            BtechSetConditionTextColours(colours->orderConditionColours[condition]);
            BtechDrawNearText(orderWeaponsText,x,ORDER_PANEL_WEAPONS_Y);
        }
        else if (orderPanelMode==ORDER_PANEL_COMMAND)
        {
            BtechDrawNearText(orderSelectionHeading,x,ORDER_PANEL_NAME_Y);
            textForegroundColour=ORDER_PANEL_TEXT_COLOUR;
            uint16_t y=ORDER_PANEL_DETAIL_FIRST_Y;
            for (uint16_t command=0;command<ORDER_COMMAND_MENU_ROWS;++command,y+=ORDER_PANEL_ROW_STEP)
                BtechDrawNearText(orderLabels[command],x,y);
        }
        else if (orderPanelMode==ORDER_PANEL_WEAPONS)
        {
            BtechDrawNearText(orderWeaponHeading,x,ORDER_PANEL_NAME_Y);
            textForegroundColour=ORDER_PANEL_TEXT_COLOUR;
            for (uint16_t kind=0;kind<COMBAT_WEAPON_KINDS;++kind)
            {mountedWeaponCountsByKind[kind]=0;operationalWeaponCountsByKind[kind]=0;}
            for (uint16_t mount=0;mount<COMBAT_WEAPON_MOUNTS;++mount)
            {
                uint16_t kind=chassis->weaponMounts[mount].weaponKind;
                if (kind==COMBAT_WEAPON_NONE) continue;
                if (kind>=COMBAT_WEAPON_KINDS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: kind>=COMBAT_WEAPON_KINDS");
                ++mountedWeaponCountsByKind[kind];
                if (mech->weapons[mount].operational) ++operationalWeaponCountsByKind[kind];
            }
            uint16_t y=ORDER_PANEL_DETAIL_FIRST_Y;
            for (uint16_t kind=0;kind<COMBAT_WEAPON_KINDS;++kind)
            {
                if (!mountedWeaponCountsByKind[kind]) continue;
                BtechDrawNearText(orderEquipmentWeaponLabels[kind],x,y);
                BtechDrawNearText(BtechFormatSignedDecimalFieldFar((int16_t)mountedWeaponCountsByKind[kind],1,' '),(uint16_t)(x+ORDER_WEAPON_TOTAL_X),y);
                BtechDrawNearText(BtechFormatSignedDecimalFieldFar((int16_t)operationalWeaponCountsByKind[kind],1,' '),(uint16_t)(x+ORDER_WEAPON_OPERATIONAL_X),y);
                BtechDrawNearText(orderOfText,(uint16_t)(x+ORDER_WEAPON_OF_X),y);
                BtechDrawNearText(orderColon,(uint16_t)(x+ORDER_WEAPON_COLON_X),y);
                y+=ORDER_PANEL_ROW_STEP;
            }
        }
        else if (orderPanelMode==ORDER_PANEL_DAMAGE)
        {
            BtechDrawNearText(orderDamageLabels[0],x,ORDER_PANEL_NAME_Y);
            uint16_t currentArmour=0,maximumArmour=0;
            for (uint16_t location=0;location<COMBAT_ARMOR_LOCATIONS;++location)
            {currentArmour+=mech->currentArmor[location];maximumArmour+=chassis->maximumArmor[location];}
            if (!maximumArmour) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !maximumArmour");
            int32_t armourCondition=(int16_t)(currentArmour*ORDER_CONDITION_SCALE)/(int16_t)maximumArmour;
            if (armourCondition<0 || armourCondition>=ORDER_CONDITION_BANDS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: armourCondition<0 || armourCondition>=ORDER_CONDITION_BANDS");
            BtechSetConditionTextColours(colours->orderConditionColours[armourCondition]);
            BtechDrawNearText(orderDamageLabels[1],x,ORDER_PANEL_DETAIL_FIRST_Y);
            uint16_t currentStructure=0,maximumStructure=0;
            for (uint16_t location=0;location<COMBAT_BODY_LOCATIONS;++location)
            {currentStructure+=mech->currentInternalStructure[location];maximumStructure+=chassis->maximumInternalStructure[location];}
            if (!maximumStructure) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !maximumStructure");
            int32_t structureCondition=(int16_t)(currentStructure*ORDER_CONDITION_SCALE)/(int16_t)maximumStructure;
            if (structureCondition<0 || structureCondition>=ORDER_CONDITION_BANDS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: structureCondition<0 || structureCondition>=ORDER_CONDITION_BANDS");
            BtechSetConditionTextColours(colours->orderConditionColours[structureCondition]);
            BtechDrawNearText(orderDamageLabels[2],x,ORDER_PANEL_DETAIL_FIRST_Y+ORDER_PANEL_ROW_STEP);
            if (mech->engineHits>=ORDER_SYSTEM_CONDITION_LEVELS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: mech->engineHits>=ORDER_SYSTEM_CONDITION_LEVELS");
            BtechSetConditionTextColours(orderEngineConditionColours[hudColourSchemeIndex][mech->engineHits]);
            BtechDrawNearText(orderDamageLabels[3],x,ORDER_PANEL_COMMAND_Y);
            if (mech->gyroHits>=ORDER_SYSTEM_CONDITION_LEVELS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: mech->gyroHits>=ORDER_SYSTEM_CONDITION_LEVELS");
            BtechSetConditionTextColours(orderSystemConditionColours[hudColourSchemeIndex][mech->gyroHits]);
            BtechDrawNearText(orderDamageLabels[4],x,ORDER_PANEL_REFERENCE_Y);
            if (mech->sensorHits>=ORDER_SYSTEM_CONDITION_LEVELS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: mech->sensorHits>=ORDER_SYSTEM_CONDITION_LEVELS");
            BtechSetConditionTextColours(orderSystemConditionColours[hudColourSchemeIndex][mech->sensorHits]);
            BtechDrawNearText(orderDamageLabels[5],x,ORDER_PANEL_DAMAGE_Y);
            if (mech->lifeSupportHits>=ORDER_SYSTEM_CONDITION_LEVELS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: mech->lifeSupportHits>=ORDER_SYSTEM_CONDITION_LEVELS");
            BtechSetConditionTextColours(orderSystemConditionColours[hudColourSchemeIndex][mech->lifeSupportHits]);
            BtechDrawNearText(orderDamageLabels[6],x,ORDER_PANEL_WEAPONS_Y);
        }
        if (!orderScreenFullRedrawPending)
            BtechCopyHudRectangleIfReady((uint16_t)(x-ORDER_PANEL_INSET),ORDER_PANEL_TOP,ORDER_PANEL_WIDTH,ORDER_PANEL_HEIGHT);
        ++panel;
    }
}
