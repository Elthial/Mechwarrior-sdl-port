#include "mechwarrior/port_diagnostics.h"
#include "mechwarrior/combat_orders.h"
#include "mechwarrior/combat_import.h"
#include "mechwarrior/combat_world.h"
#include "mechwarrior/combat_cockpit.h"
#include "mechwarrior/combat_input.h"
#include "mechwarrior/combat_tactics.h"
#include <stdlib.h>

/* An empty lance panel's control pointer is zero. The original never tests
 * it on these paths: it reads and writes the unit control at DS:0000, the
 * C runtime's null segment (nullNearSegment). While an order procedure runs,
 * the 0x55 native bytes are decoded into nullControl; they are written back
 * as the outermost procedure leaves, so a later read (here, or the radar's
 * DS:0008) sees what the original would. */
static BtechUnitControlState nullControl;
static unsigned nullControlDepth;
static bool nullControlLoaded;
static void NullField(bool load,uint16_t offset,void *field,size_t bytes)
{
    uint32_t value=0;
    if (load)
    {
        for (size_t i=0;i<bytes;++i) value|=(uint32_t)nullNearSegment[offset+i]<<(8*i);
        if (bytes==1) *(uint8_t *)field=(uint8_t)value;
        else if (bytes==2) *(uint16_t *)field=(uint16_t)value;
        else *(uint32_t *)field=value;
        return;
    }
    if (bytes==1) value=*(const uint8_t *)field;
    else if (bytes==2) value=*(const uint16_t *)field;
    else value=*(const uint32_t *)field;
    for (size_t i=0;i<bytes;++i) nullNearSegment[offset+i]=(uint8_t)(value>>(8*i));
}
#define NULL_FIELD(load,offset,field) NullField(load,(uint16_t)(offset),&(field),sizeof(field))
static void TransferNullControl(bool load)
{
    BtechUnitControlState *c=&nullControl;
    _Static_assert(NULL_NEAR_SEGMENT_BYTES>=0x55,"native unit control");
    NULL_FIELD(load,0x00,c->commandCode);
    for (unsigned axis=0;axis<3;++axis) NULL_FIELD(load,0x02 + axis*4,c->orderDestinationPosition[axis]);
    NULL_FIELD(load,0x0E,c->trackedUnitOrSpecialSlot);NULL_FIELD(load,0x10,c->sideIndex);NULL_FIELD(load,0x12,c->unitSlot);
    NULL_FIELD(load,0x14,c->targetUnitSlot);NULL_FIELD(load,0x16,c->conditionScoreOrExitCode);
    NULL_FIELD(load,0x18,c->lastAiFiringTargetSlot);NULL_FIELD(load,0x1A,c->lastDirectHitUnitSlot);
    NULL_FIELD(load,0x1C,c->targetSelectionCount);NULL_FIELD(load,0x1E,c->navigationReferenceSlot);
    for (unsigned axis=0;axis<3;++axis) NULL_FIELD(load,0x20 + axis*4,c->trackedPosition[axis]);
    NULL_FIELD(load,0x2C,c->positionKnowledgeCode);
    for (unsigned axis=0;axis<3;++axis) NULL_FIELD(load,0x2E + axis*4,c->lastVisiblePosition[axis]);
    NULL_FIELD(load,0x3A,c->avoidanceReverseFlag);NULL_FIELD(load,0x3C,c->avoidanceTurnIndex);
    NULL_FIELD(load,0x3E,c->maneuverFlags);NULL_FIELD(load,0x3F,c->unidentifiedField14);
    NULL_FIELD(load,0x40,c->twiceReadyWeaponRange);NULL_FIELD(load,0x42,c->readyWeaponRangeMinusMargin);
    NULL_FIELD(load,0x44,c->quarterReadyWeaponRange);NULL_FIELD(load,0x46,c->tacticalState);NULL_FIELD(load,0x48,c->decisionCountdown);
    NULL_FIELD(load,0x4A,c->unidentifiedField16[0]);NULL_FIELD(load,0x4B,c->unidentifiedField16[1]);
    NULL_FIELD(load,0x4C,c->animationSequence);NULL_FIELD(load,0x4E,c->biasedHeadingSnapshot);
    NULL_FIELD(load,0x50,c->animationRequestFlags);NULL_FIELD(load,0x51,c->movementMode);NULL_FIELD(load,0x53,c->animationFrame);
}
static void EnterOrderProcedure(void) {++nullControlDepth;}
static void LeaveOrderProcedure(void)
{
    if (--nullControlDepth || !nullControlLoaded) return;
    TransferNullControl(false);nullControlLoaded=false;
}
/* The selected panel's control, or the null segment's for an empty panel. */
static BtechUnitControlState *SelectedOrderControl(void)
{
    if (selectedLanceOrderPanel>=ORDER_WINGMATE_PANELS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: selectedLanceOrderPanel>=ORDER_WINGMATE_PANELS");
    BtechUnitControlState *control=lanceOrderControls[selectedLanceOrderPanel];
    if (control) return control;
    if (!nullControlLoaded) {TransferNullControl(true);nullControlLoaded=true;}
    return &nullControl;
}

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
    if (orderMapScaleIndex>=ORDER_MAP_SCALE_COUNT) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: orderMapScaleIndex>=ORDER_MAP_SCALE_COUNT");
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
    if ((uint16_t)unitSlot>=COMBAT_MECH_SLOTS || !unitBindings[unitSlot].mech) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: (uint16_t)unitSlot>=COMBAT_MECH_SLOTS || !unitBindings[unitSlot].mech");
    const BtechMechState *mech=unitBindings[unitSlot].mech;
    if (mech->templateIndex>=COMBAT_MECH_TEMPLATES) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: mech->templateIndex>=COMBAT_MECH_TEMPLATES");
    const BtechMechTemplate *chassis=&mechTemplates[mech->templateIndex];
    uint16_t maximum=0,current=0;
    for (uint16_t armor=0;armor<COMBAT_ARMOR_LOCATIONS;++armor)
    {current+=mech->currentArmor[armor];maximum+=chassis->maximumArmor[armor];}
    for (uint16_t body=0;body<COMBAT_BODY_LOCATIONS;++body)
    {current+=mech->currentInternalStructure[body];maximum+=chassis->maximumInternalStructure[body];}
    if (!maximum) /* Port emulation of a native CPU fault. */ MW_NATIVE_FAULT("Rejected condition: !maximum"); /* Native IDIV fault, not an invented empty/healthy state. */
    int32_t quotient=(int16_t)(current*ORDER_CONDITION_SCALE)/(int16_t)maximum;
    if (quotient<INT16_MIN || quotient>INT16_MAX) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: quotient<INT16_MIN || quotient>INT16_MAX");
    int16_t band=quotient;
    if (mech->sensorHits==COMBAT_SENSORS_DESTROYED_HITS) --band;
    if (mech->lifeSupportHits==1) --band;
    if (mech->engineHits==COMBAT_ENGINE_HIT_LIMIT || mech->gyroHits==COMBAT_GYRO_DESTROYED_HITS || mech->cockpitDisabled==1) band=0;
    if (band<0 || (uint16_t)unitControls[unitSlot].conditionScoreOrExitCode==COMBAT_DISABLED_UNIT_VALUE) band=0;
    if (hudColourSchemeIndex>=COMBAT_HUD_COLOUR_SCHEMES || band>=ORDER_CONDITION_BANDS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: hudColourSchemeIndex>=COMBAT_HUD_COLOUR_SCHEMES || band>=ORDER_CONDITION_BANDS"); /* Unsupported upper-band adjacent palette alias. */
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
        if (previousOrderHighlightPanel>=ORDER_WINGMATE_PANELS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: previousOrderHighlightPanel>=ORDER_WINGMATE_PANELS");
        uint16_t previousX=lanceOrderPanelTextX[previousOrderHighlightPanel]-ORDER_PANEL_INSET;
        BtechDrawStyledRectangle(previousX,previousY,ORDER_PANEL_WIDTH,ORDER_HIGHLIGHT_HEIGHT);
        if (!orderScreenFullRedrawPending)
            BtechCopyHudRectangleIfReady(previousX,previousY,ORDER_PANEL_WIDTH,ORDER_HIGHLIGHT_HEIGHT);
    }
    if (hudColourSchemeIndex>=COMBAT_HUD_COLOUR_SCHEMES) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: hudColourSchemeIndex>=COMBAT_HUD_COLOUR_SCHEMES");
    const BtechHudColourScheme *colours=&hudColourSchemes[hudColourSchemeIndex];
    drawFillColour=(uint8_t)(selectedOrderPanelRow==ORDER_REFERENCE_ROW && (orderPanelMode==ORDER_PANEL_TARGET || orderPanelMode==ORDER_PANEL_DESTINATION)
        ?colours->orderReferenceHighlight:colours->orderHighlight);
    uint16_t selectedY=selectedOrderPanelRow? (uint16_t)(ORDER_HIGHLIGHT_ROW_BASE_Y+(uint16_t)(selectedOrderPanelRow*ORDER_PANEL_ROW_STEP)):ORDER_HIGHLIGHT_FIRST_ROW_Y;
    if (selectedLanceOrderPanel>=ORDER_WINGMATE_PANELS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: selectedLanceOrderPanel>=ORDER_WINGMATE_PANELS");
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
    if (panelIndex>=ORDER_WINGMATE_PANELS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: panelIndex>=ORDER_WINGMATE_PANELS");
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
    default:/* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Lance order has no supported selection class.");
    }
}

/*17FB:0C62. Existing tracked target, actual target, living special objective,
 * then first known non-disabled enemy. No range/visibility recomputation. */
static void OrderSeedSelectedOrderTrackingBody(void)
{
    BtechUnitControlState *control=SelectedOrderControl();
    if (control->trackedUnitOrSpecialSlot==-1)
    {
        if (control->targetUnitSlot!=-1) control->trackedUnitOrSpecialSlot=control->targetUnitSlot;
        else if (sideObjectiveRules[0]==OBJECTIVE_SPECIAL_TARGET && (int16_t)specialTargetDamageRemaining>0)
            control->trackedUnitOrSpecialSlot=ORDER_SPECIAL_TARGET_SLOT;
        else
            for (uint16_t slot=COMBAT_LANCE_SLOTS;slot<COMBAT_MECH_SLOTS;++slot)
                if (unitBindings[slot].mech && unitControls[slot].positionKnowledgeCode!=COMBAT_POSITION_UNKNOWN
                    && unitControls[slot].conditionScoreOrExitCode>COMBAT_MECH_DISABLED_VALUE)
                {control->trackedUnitOrSpecialSlot=(int16_t)slot;break;}
    }
    control->navigationReferenceSlot=control->trackedUnitOrSpecialSlot;
}
void BtechSeedSelectedOrderTracking(void)
{
    EnterOrderProcedure();OrderSeedSelectedOrderTrackingBody();LeaveOrderProcedure();
}

/*17FB:0666. Original global keys, confirm/commit and navigation share this
 * procedure. Q/Space still fall through after viewport setup; saved scan is
 * not reread after pause. No invented editor/update loop or DI argument. */
static void OrderEditSelectedUnitOrdersBody(void)
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
        if (SelectedOrderControl()->conditionScoreOrExitCode>=COMBAT_CONDITION_SCORE_MINIMUM) goto navigation;
    }
    if (orderPanelMode==ORDER_PANEL_SUMMARY)
    {
        switch (orderActionSelection)
        {
        case ORDER_ACTION_COMMAND:
            control=SelectedOrderControl();
            if (control->conditionScoreOrExitCode<=COMBAT_CONDITION_SCORE_MINIMUM) goto navigation;
            orderPanelMode=ORDER_PANEL_COMMAND;pendingOrderCommand=control->commandCode;
            selectedOrderPanelRow=control->commandCode+orderSelectionFirstRows[1];
            break;
        case ORDER_ACTION_REFERENCE:
            control=SelectedOrderControl();
            if (control->conditionScoreOrExitCode<=COMBAT_CONDITION_SCORE_MINIMUM) goto navigation;
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
            control=SelectedOrderControl();
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
        if (direction>=ORDER_NAVIGATION_CODES) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: direction>=ORDER_NAVIGATION_CODES"); /* Unsupported adjacent-table aliases, not a new direction. */
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
            if ((uint16_t)panel>=ORDER_WINGMATE_PANELS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: (uint16_t)panel>=ORDER_WINGMATE_PANELS");
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
            control=SelectedOrderControl();
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
                    if ((uint16_t)candidate>=COMBAT_MECH_SLOTS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: (uint16_t)candidate>=COMBAT_MECH_SLOTS");
                    if (unitBindings[candidate].mech && unitControls[candidate].sideIndex==1
                        && unitControls[candidate].conditionScoreOrExitCode>COMBAT_MECH_DISABLED_VALUE && unitControls[candidate].positionKnowledgeCode!=COMBAT_POSITION_UNKNOWN)
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
            if (orderMapScaleIndex>=ORDER_MAP_SCALE_COUNT) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: orderMapScaleIndex>=ORDER_MAP_SCALE_COUNT");
            control=SelectedOrderControl();
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
void BtechEditSelectedUnitOrders(void)
{
    EnterOrderProcedure();OrderEditSelectedUnitOrdersBody();LeaveOrderProcedure();
}

/*17FB:0040. Width excludes the right edge; height includes the bottom.
 * Native discards capture-size DX before requesting DOS memory. */
void BtechInitializeOrderPanelState(void)
{
    if (!orderPanelCaptureBuffer.bytes)
    {
        if (!orderMapCameraContext) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !orderMapCameraContext"); /* Native near-zero camera alias not represented. */
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
