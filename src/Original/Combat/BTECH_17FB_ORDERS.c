#include "mechwarrior/combat_orders.h"
#include "mechwarrior/combat_import.h"
#include "mechwarrior/combat_world.h"
#include "mechwarrior/combat_cockpit.h"
#include "mechwarrior/combat_input.h"
#include "mechwarrior/combat_tactics.h"
#include <stdlib.h>

/*17FB:1927. Genuine FAR three-WORD DWORD/shift entry. Signed nonpositive
 * counts leave the value unchanged; the native loop does not mask CL. */
uint32_t BtechShiftDwordLeftPair(uint32_t value,uint16_t shiftCount)
{
    if ((int16_t)shiftCount>0)
        do {value<<=1;} while (--shiftCount);
    return value;
}

/*17FB:0002. DX:AX position-grid mask, no pushed source arguments. */
uint32_t BtechBuildOrderMapPositionMask(void)
{
    if (orderMapScaleIndex>=ORDER_MAP_SCALE_COUNT) abort();
    return BtechShiftDwordLeftPair(UINT32_MAX,(uint16_t)(orderMapScaleShifts[orderMapScaleIndex]-ORDER_MAP_PROJECTION_BASE_SHIFT));
}

/*17FB:1458. AL foreground, but background tests the FULL source WORD. */
void BtechSetConditionTextColours(uint16_t conditionColour)
{
    textForegroundColour=(uint8_t)conditionColour;
    textBackgroundColour=conditionColour?Combat_Pal_EGA_Black:ORDER_CONDITION_FAILED_BACKGROUND;
}

/*17FB:1472. Combined armour/internal condition band, with exact system-hit
 * penalties and FFFF disabled test. This is not a Battle Value calculation. */
uint16_t BtechGetMechConditionColour(int16_t unitSlot)
{
    if ((uint16_t)unitSlot>=COMBAT_MECH_SLOTS || !unitBindings[unitSlot].mech) abort();
    const BtechMechState *mech=unitBindings[unitSlot].mech;
    if (mech->templateIndex>=COMBAT_MECH_TEMPLATES) abort();
    const BtechMechTemplate *chassis=&mechTemplates[mech->templateIndex];
    uint16_t maximum=0,current=0;
    for (uint16_t armor=0;armor<COMBAT_ARMOR_LOCATIONS;++armor)
    {current+=mech->currentArmor[armor];maximum+=chassis->maximumArmor[armor];}
    for (uint16_t body=0;body<COMBAT_BODY_LOCATIONS;++body)
    {current+=mech->currentInternalStructure[body];maximum+=chassis->maximumInternalStructure[body];}
    if (!maximum) abort(); /* Native IDIV fault, not an invented empty/healthy state. */
    int32_t quotient=(int16_t)(current*ORDER_CONDITION_SCALE)/(int16_t)maximum;
    if (quotient<INT16_MIN || quotient>INT16_MAX) abort();
    int16_t band=quotient;
    if (mech->sensorHits==COMBAT_SENSORS_DESTROYED_HITS) band=(int16_t)(band-1);
    if (mech->lifeSupportHits==1) band=(int16_t)(band-1);
    if (mech->engineHits==COMBAT_ENGINE_HIT_LIMIT || mech->gyroHits==COMBAT_GYRO_DESTROYED_HITS || mech->cockpitDisabled==1) band=0;
    if (band<0 || (uint16_t)unitControls[unitSlot].combatValueOrBoundaryCode==COMBAT_DISABLED_UNIT_VALUE) band=0;
    if (hudColourSchemeIndex>=COMBAT_HUD_COLOUR_SCHEMES || band>=ORDER_CONDITION_BANDS) abort(); /* Unsupported upper-band adjacent palette alias. */
    return hudColourSchemes[hudColourSchemeIndex].orderConditionColours[band];
}

/*17FB:157E. Native first-wingmate gate, even if another wingmate exists.
 * Erase previous highlight before drawing current and saving panel/row. */
void BtechUpdateLanceOrderHighlight(void)
{
    if (!unitBindings[1].mech) return;
    drawPolygonFilled=false;drawPageSelector=backPageSelector;
    if ((int16_t)previousOrderHighlightRow>=0)
    {
        drawFillColour=Combat_Pal_EGA_Black;
        uint16_t previousY=previousOrderHighlightRow? (uint16_t)(ORDER_HIGHLIGHT_ROW_BASE_Y+(uint16_t)(previousOrderHighlightRow*ORDER_PANEL_ROW_STEP)):ORDER_HIGHLIGHT_FIRST_ROW_Y;
        if (previousOrderHighlightPanel>=ORDER_WINGMATE_PANELS) abort();
        uint16_t previousX=lanceOrderPanelTextX[previousOrderHighlightPanel]-ORDER_PANEL_INSET;
        BtechDrawStyledRectangle(previousX,previousY,ORDER_PANEL_WIDTH,ORDER_HIGHLIGHT_HEIGHT);
        if (!orderScreenFullRedrawPending)
            BtechCopyHudRectangleIfReady(previousX,previousY,ORDER_PANEL_WIDTH,ORDER_HIGHLIGHT_HEIGHT);
    }
    if (hudColourSchemeIndex>=COMBAT_HUD_COLOUR_SCHEMES) abort();
    const BtechHudColourScheme *colours=&hudColourSchemes[hudColourSchemeIndex];
    drawFillColour=(uint8_t)(selectedOrderPanelRow==ORDER_REFERENCE_ROW && (orderPanelMode==ORDER_PANEL_TARGET || orderPanelMode==ORDER_PANEL_DESTINATION)
        ?colours->orderReferenceHighlight:colours->orderHighlight);
    uint16_t selectedY=selectedOrderPanelRow? (uint16_t)(ORDER_HIGHLIGHT_ROW_BASE_Y+(uint16_t)(selectedOrderPanelRow*ORDER_PANEL_ROW_STEP)):ORDER_HIGHLIGHT_FIRST_ROW_Y;
    if (selectedLanceOrderPanel>=ORDER_WINGMATE_PANELS) abort();
    uint16_t selectedX=lanceOrderPanelTextX[selectedLanceOrderPanel]-ORDER_PANEL_INSET;
    BtechDrawStyledRectangle(selectedX,selectedY,ORDER_PANEL_WIDTH,ORDER_HIGHLIGHT_HEIGHT);
    if (!orderScreenFullRedrawPending)
        BtechCopyHudRectangleIfReady(selectedX,selectedY,ORDER_PANEL_WIDTH,ORDER_HIGHLIGHT_HEIGHT);
    previousOrderHighlightRow=selectedOrderPanelRow;
    previousOrderHighlightPanel=selectedLanceOrderPanel;
}

/*17FB:16A7. Clear panel body but not its heading; leave outline mode set. */
void BtechClearLanceOrderPanel(uint16_t panelIndex)
{
    drawPageSelector=backPageSelector;drawPolygonFilled=true;
    drawFillColour=Combat_Pal_EGA_Black;drawLineColour=Combat_Pal_EGA_Black;
    if (panelIndex>=ORDER_WINGMATE_PANELS) abort();
    BtechDrawStyledRectangle((uint16_t)(lanceOrderPanelTextX[panelIndex]-ORDER_PANEL_INSET),ORDER_PANEL_TOP,ORDER_PANEL_WIDTH,ORDER_PANEL_HEIGHT);
    drawPolygonFilled=false;
}

/*17FB:0CFE. Command codes and editor panel modes are different domains.
 * Out-of-range native commands leave a stack WORD undefined, not SUMMARY. */
uint16_t BtechClassifyOrderSelection(uint16_t commandCode)
{
    switch (commandCode)
    {
    case COMBAT_ORDER_ACT_ON_OWN:return ORDER_PANEL_SUMMARY;
    case TACTICAL_ORDER_AMBUSH:
    case TACTICAL_ORDER_DEFEND:
    case TACTICAL_ORDER_MOVE_ATTACK:
    case TACTICAL_ORDER_MOVE_AVOID:return ORDER_PANEL_DESTINATION;
    case TACTICAL_ORDER_ATTACK:return ORDER_PANEL_TARGET;
    default:abort();
    }
}

/*17FB:0C62. Existing tracked target, actual target, living special objective,
 * then first known non-disabled enemy. No range/visibility recomputation. */
void BtechSeedSelectedOrderTracking(void)
{
    if (selectedLanceOrderPanel>=ORDER_WINGMATE_PANELS || !lanceOrderControls[selectedLanceOrderPanel]) abort();
    BtechUnitControlState *control=lanceOrderControls[selectedLanceOrderPanel];
    if (control->trackedUnitOrSpecialSlot==-1)
    {
        if (control->targetUnitSlot!=-1) control->trackedUnitOrSpecialSlot=control->targetUnitSlot;
        else if (sideObjectiveRules[0]==OBJECTIVE_SPECIAL_TARGET && (int16_t)specialTargetDamageRemaining>0)
            control->trackedUnitOrSpecialSlot=ORDER_SPECIAL_TARGET_SLOT;
        else
            for (uint16_t slot=COMBAT_LANCE_SLOTS;slot<COMBAT_MECH_SLOTS;++slot)
                if (unitBindings[slot].mech && unitControls[slot].positionKnowledgeCode!=COMBAT_POSITION_UNKNOWN
                    && unitControls[slot].combatValueOrBoundaryCode>-1)
                {control->trackedUnitOrSpecialSlot=(int16_t)slot;break;}
    }
    control->navigationReferenceSlot=control->trackedUnitOrSpecialSlot;
}

/*17FB:0666. Original global keys, confirm/commit and navigation share this
 * procedure. Q/Space still fall through after viewport setup; saved scan is
 * not reread after pause. No invented editor/update loop or DI argument. */
void BtechEditSelectedUnitOrders(void)
{
    uint16_t scan=combatInputScanCode;
    switch (scan)
    {
    case COMBAT_KEY_Q:
    case COMBAT_KEY_SPACE:
        combatDispatchState=COMBAT_SCREEN_COCKPIT;orderScreenFullRedrawPending=1;
        BtechRefreshCockpitViewportAndGaugesFar();cockpitFullRedrawPending=1;
        break;
    case COMBAT_KEY_P:BtechShowSimulationPausedDialogFar();break;
    case COMBAT_KEY_S:combatSoundEnabled=combatSoundEnabled==0;break;
    case COMBAT_KEY_D:combatDispatchState=PLAYER_SCREEN_INSPECTION;orderScreenFullRedrawPending=1;break;
    case COMBAT_KEY_Z:
        --orderMapScaleIndex;
        if ((int16_t)orderMapScaleIndex<0) orderMapScaleIndex=ORDER_MAP_FULL_SCALE;
        orderMapPositionMask=BtechBuildOrderMapPositionMask();
        break;
    default:break;
    }
    BtechUnitControlState *control;
    if (scan!=COMBAT_KEY_ENTER && !combatInputButton1State && !combatInputButton0State)
    {
        if (orderPanelMode==ORDER_PANEL_SUMMARY || (int16_t)orderPanelMode>=ORDER_PANEL_DAMAGE) goto navigation;
        if (selectedLanceOrderPanel>=ORDER_WINGMATE_PANELS || !lanceOrderControls[selectedLanceOrderPanel]) abort();
        if (lanceOrderControls[selectedLanceOrderPanel]->combatValueOrBoundaryCode>=0) goto navigation;
    }
    if (orderPanelMode==ORDER_PANEL_SUMMARY)
    {
        switch (orderActionSelection)
        {
        case ORDER_ACTION_COMMAND:
            if (selectedLanceOrderPanel>=ORDER_WINGMATE_PANELS || !lanceOrderControls[selectedLanceOrderPanel]) abort();
            control=lanceOrderControls[selectedLanceOrderPanel];
            if (control->combatValueOrBoundaryCode<=0) goto navigation;
            orderPanelMode=ORDER_PANEL_COMMAND;pendingOrderCommand=control->commandCode;
            selectedOrderPanelRow=control->commandCode+orderSelectionFirstRows[1];
            break;
        case ORDER_ACTION_REFERENCE:
            if (selectedLanceOrderPanel>=ORDER_WINGMATE_PANELS || !lanceOrderControls[selectedLanceOrderPanel]) abort();
            control=lanceOrderControls[selectedLanceOrderPanel];
            if (control->combatValueOrBoundaryCode<=0) goto navigation;
            if (control->commandCode==COMBAT_ORDER_ACT_ON_OWN)
            {
                switch (sideObjectiveRules[0])
                {
                case OBJECTIVE_ELIMINATE_OPPOSITION:
                case OBJECTIVE_OPPOSE_WITHDRAWAL:
                case OBJECTIVE_SPECIAL_TARGET:orderPanelMode=ORDER_PANEL_TARGET;control->commandCode=TACTICAL_ORDER_ATTACK;break;
                case OBJECTIVE_WITHDRAW:orderPanelMode=ORDER_PANEL_DESTINATION;control->commandCode=TACTICAL_ORDER_MOVE_ATTACK;break;
                case OBJECTIVE_DEFEND_SPECIAL_TARGET:orderPanelMode=ORDER_PANEL_DESTINATION;control->commandCode=TACTICAL_ORDER_DEFEND;break;
                default:break; /* Native unknown objective leaves the current panel mode alone. */
                }
            }
            else orderPanelMode=BtechClassifyOrderSelection(control->commandCode);
            if (orderPanelMode==ORDER_PANEL_TARGET) BtechSeedSelectedOrderTracking();
            else if (orderPanelMode==ORDER_PANEL_DESTINATION) control->navigationReferenceSlot=-1;
            BtechUpdateLanceOrderHighlight();goto navigation;
        case ORDER_ACTION_SYSTEMS:orderPanelMode=ORDER_PANEL_DAMAGE;selectedOrderPanelRow=0;break;
        case ORDER_ACTION_WEAPONS:orderPanelMode=ORDER_PANEL_WEAPONS;selectedOrderPanelRow=0;break;
        default:goto navigation;
        }
    }
    else
    {
        if (orderPanelMode==ORDER_PANEL_COMMAND)
        {
            if (selectedLanceOrderPanel>=ORDER_WINGMATE_PANELS || !lanceOrderControls[selectedLanceOrderPanel]) abort();
            control=lanceOrderControls[selectedLanceOrderPanel];
            if (pendingOrderCommand!=COMBAT_ORDER_ACT_ON_OWN && control->commandCode==COMBAT_ORDER_ACT_ON_OWN)
            {
                control->trackedUnitOrSpecialSlot=-1;
                control->orderDestinationPosition[1]=0;control->orderDestinationPosition[0]=0;
                control->orderDestinationPosition[2]=-1;
            }
            control->commandCode=pendingOrderCommand;control->navigationReferenceSlot=-1;
            orderPanelMode=BtechClassifyOrderSelection(pendingOrderCommand);
            if (orderPanelMode!=ORDER_PANEL_SUMMARY)
            {
                orderActionSelection=ORDER_ACTION_REFERENCE;
                if (orderPanelMode==ORDER_PANEL_DESTINATION)
                {
                    if (control->orderDestinationPosition[2]<0)
                        for (uint16_t axis=0;axis<3;++axis) control->orderDestinationPosition[axis]=control->trackedPosition[axis];
                    control->trackedUnitOrSpecialSlot=-1;
                }
                else if (orderPanelMode==ORDER_PANEL_TARGET) BtechSeedSelectedOrderTracking();
            }
        }
        else orderPanelMode=ORDER_PANEL_SUMMARY;
        selectedOrderPanelRow=orderActionSelection+orderSelectionFirstRows[0];
    }
    BtechClearLanceOrderPanel(selectedLanceOrderPanel);
    BtechUpdateLanceOrderHighlight();
navigation:
    {
        uint16_t direction=combatInputDirectionMask;
        if (!direction) {previousOrderNavigationKey=0;return;}
        if (direction>=ORDER_NAVIGATION_CODES) abort(); /* Unsupported adjacent-table aliases, not a new direction. */
        if (orderPanelMode==ORDER_PANEL_SUMMARY || orderPanelMode==ORDER_PANEL_COMMAND)
        {
            uint16_t selectionClass=orderPanelMode!=ORDER_PANEL_SUMMARY;
            int16_t panel=(int16_t)selectedLanceOrderPanel,row=(int16_t)selectedOrderPanelRow;
            if (!selectionClass) panel=(int16_t)(panel+orderNavigationPanelDeltas[direction]);
            row=(int16_t)(row+orderNavigationRowDeltas[direction]);
            int16_t lastPanel=reducedMemoryMode?1:ORDER_WINGMATE_PANELS-1;
            if (panel<0 || panel>lastPanel) panel=(int16_t)selectedLanceOrderPanel;
            if (row<orderSelectionFirstRows[selectionClass]) row=orderSelectionLastRows[selectionClass];
            else if (row>orderSelectionLastRows[selectionClass]) row=orderSelectionFirstRows[selectionClass];
            if ((uint16_t)panel>=ORDER_WINGMATE_PANELS) abort();
            if (((uint16_t)panel!=selectedLanceOrderPanel || (uint16_t)row!=selectedOrderPanelRow) && lanceOrderControls[panel])
            {
                selectedLanceOrderPanel=(uint16_t)panel;selectedOrderPanelRow=(uint16_t)row;
                BtechUpdateLanceOrderHighlight();
            }
            uint16_t selection=selectedOrderPanelRow-orderSelectionFirstRows[selectionClass];
            if (!selectionClass) orderActionSelection=selection;
            else pendingOrderCommand=selection;
        }
        else if (orderPanelMode==ORDER_PANEL_TARGET)
        {
            if (selectedLanceOrderPanel>=ORDER_WINGMATE_PANELS || !lanceOrderControls[selectedLanceOrderPanel]) abort();
            control=lanceOrderControls[selectedLanceOrderPanel];
            int16_t step=orderNavigationPanelDeltas[direction]+orderNavigationRowDeltas[direction];
            int16_t candidate=control->trackedUnitOrSpecialSlot,target=-1;
            if (candidate==-1) candidate=control->targetUnitSlot>=0?control->targetUnitSlot:COMBAT_LANCE_SLOTS;
            uint16_t scanned;
            for (scanned=0;scanned<ORDER_TARGET_SCAN_LIMIT;++scanned)
            {
                candidate=(int16_t)(candidate+(step<0?-1:step>0?1:0)+ORDER_TARGET_CANDIDATES);
                candidate%=ORDER_TARGET_CANDIDATES;
                if (candidate==ORDER_SPECIAL_TARGET_SLOT)
                {if (sideObjectiveRules[0]==OBJECTIVE_SPECIAL_TARGET) target=ORDER_SPECIAL_TARGET_SLOT;}
                else
                {
                    if ((uint16_t)candidate>=COMBAT_MECH_SLOTS) abort();
                    if (unitBindings[candidate].mech && unitControls[candidate].sideIndex==1
                        && unitControls[candidate].combatValueOrBoundaryCode>-1 && unitControls[candidate].positionKnowledgeCode!=COMBAT_POSITION_UNKNOWN)
                        target=candidate;
                }
                if (target>=0) break;
            }
            if (scanned<ORDER_TARGET_SCAN_LIMIT) {control->trackedUnitOrSpecialSlot=target;control->navigationReferenceSlot=target;}
        }
        else if (orderPanelMode==ORDER_PANEL_DESTINATION)
        {
            uint16_t repeatShift=0;
            if (direction==previousOrderNavigationKey)
            {
                if (orderDestinationRepeatCountdown) --orderDestinationRepeatCountdown;
                else repeatShift=ORDER_DESTINATION_REPEAT_SHIFT;
            }
            else orderDestinationRepeatCountdown=ORDER_DESTINATION_REPEAT_TICKS;
            if (selectedLanceOrderPanel>=ORDER_WINGMATE_PANELS || !lanceOrderControls[selectedLanceOrderPanel] || orderMapScaleIndex>=ORDER_MAP_SCALE_COUNT) abort();
            control=lanceOrderControls[selectedLanceOrderPanel];
            uint8_t shift=orderMapScaleShifts[orderMapScaleIndex]+repeatShift-ORDER_MAP_PROJECTION_BASE_SHIFT;
            int16_t deltaX=shift<16?(int16_t)((uint32_t)(uint16_t)orderNavigationPanelDeltas[direction]<<shift):0;
            int16_t deltaY=shift<16?(int16_t)((uint32_t)(uint16_t)orderNavigationRowDeltas[direction]<<shift):0;
            uint32_t x=(uint32_t)control->orderDestinationPosition[0]+(uint32_t)deltaX;
            uint32_t y=(uint32_t)control->orderDestinationPosition[1]-(uint32_t)deltaY;
            uint32_t magnitudeX=x==0x00008000?0x00007FFF:(x&UINT32_C(0x80000000))?0-x:x;
            uint32_t magnitudeY=y==0x00008000?0x00007FFF:(y&UINT32_C(0x80000000))?0-y:y;
            if ((int16_t)(magnitudeX>>16)<0 || magnitudeX<ORDER_DESTINATION_LIMIT_X) control->orderDestinationPosition[0]=(int32_t)x;
            if ((int16_t)(magnitudeY>>16)<0 || magnitudeY<ORDER_DESTINATION_LIMIT_Y) control->orderDestinationPosition[1]=(int32_t)y;
            control->orderDestinationPosition[2]=0;
            for (uint16_t axis=0;axis<3;++axis) control->trackedPosition[axis]=control->orderDestinationPosition[axis];
        }
        previousOrderNavigationKey=direction;
    }
}

/*17FB:0040. Width excludes the right edge; height includes the bottom.
 * Native discards capture-size DX before requesting DOS memory. */
void BtechInitializeOrderPanelState(void)
{
    if (!orderPanelCaptureBuffer.bytes)
    {
        if (!orderMapCameraContext) abort(); /* Native near-zero camera alias not represented. */
        uint16_t height=orderMapCameraContext->clipMaximumB-orderMapCameraContext->clipMinimumB+1;
        uint16_t width=orderMapCameraContext->clipMaximumA-orderMapCameraContext->clipMinimumA;
        uint16_t captureBytes=(uint16_t)BtechGetCaptureStorageSize(width,height);
        orderPanelCaptureBuffer.bytes=BtechAllocateDosMemoryBlock(captureBytes).bytes;
        orderPanelCaptureBuffer.byteCount=((uint32_t)captureBytes+0x0F)&~0x0Fu;
        if (!orderPanelCaptureBuffer.bytes) BtechShowFatalErrorAndExit("no memory for map buffer.");
    }
    if (reducedMemoryMode) lanceOrderPanelTextX[1]=lanceOrderPanelTextX[2];
    orderMapScaleIndex=ORDER_MAP_FULL_SCALE;previousOrderMapScaleIndex=ORDER_MAP_FULL_SCALE;
    orderScreenFullRedrawPending=1;
    previousOrderHighlightPanel=ORDER_NO_HIGHLIGHT;previousOrderHighlightRow=ORDER_NO_HIGHLIGHT;
    pendingOrderCommand=0;orderActionSelection=0;selectedLanceOrderPanel=0;
    selectedOrderPanelRow=ORDER_FIRST_ACTION_ROW;orderPanelMode=ORDER_PANEL_SUMMARY;
    previousOrderNavigationKey=0;orderDestinationRepeatCountdown=ORDER_DESTINATION_REPEAT_TICKS;
}

/*17FB:001E. Free before clearing the allocation. Other order state stays live.
 * Offset/selector halves become one typed reference; no caller can observe a
 * halfway clear after the DOS call on this supported non-reentrant path. */
void BtechReleaseOrderPanelBuffer(void)
{
    if (orderPanelCaptureBuffer.bytes)
    {
        BtechFreeDosMemorySelector(orderPanelCaptureBuffer.bytes);
        orderPanelCaptureBuffer=(BtechResourceAllocation){0};
    }
}
