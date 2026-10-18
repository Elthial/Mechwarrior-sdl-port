#include "mechwarrior/combat_orders.h"
#include "mechwarrior/native_address.h"
#include <string.h>

BtechResourceAllocation orderPanelCaptureBuffer;
uint16_t lanceOrderPanelTextX[3];
uint8_t orderScreenFullRedrawPending;
uint16_t previousOrderHighlightPanel,previousOrderHighlightRow;
uint32_t orderMapPositionMask;
BtechUnitControlState *lanceOrderControls[ORDER_WINGMATE_PANELS];
int16_t orderSelectionFirstRows[ORDER_SELECTION_CLASSES],orderSelectionLastRows[ORDER_SELECTION_CLASSES];
int16_t orderNavigationPanelDeltas[ORDER_NAVIGATION_CODES],orderNavigationRowDeltas[ORDER_NAVIGATION_CODES];
static char orderPanelText[ORDER_PANEL_TEXT_END-ORDER_PANEL_TEXT_START];
const char *orderLabels[ORDER_LABELS],*tacticalOrderLabels[ORDER_TACTICAL_LABELS];
const char *orderDamageLabels[ORDER_DAMAGE_LABELS],*orderEquipmentWeaponLabels[COMBAT_WEAPON_KINDS];
const char *orderWeaponHeading,*orderOfText,*orderColon,*orderSelectionHeading;
const char *orderLocationText,*orderBaseText,*orderDamageText,*orderWeaponsText;
uint16_t orderDisplayIndices[ORDER_DISPLAY_CODES];
uint16_t orderEngineConditionColours[2][4],orderSystemConditionColours[2][4];
uint16_t mountedWeaponCountsByKind[COMBAT_WEAPON_KINDS],operationalWeaponCountsByKind[COMBAT_WEAPON_KINDS];
uint16_t orderMapScaleIndex,selectedLanceOrderPanel,selectedOrderPanelRow;
uint16_t orderPanelMode,orderActionSelection,pendingOrderCommand;
uint16_t orderDestinationRepeatCountdown,previousOrderMapScaleIndex,previousOrderNavigationKey;
uint8_t orderMapScaleShifts[ORDER_MAP_SCALE_COUNT];
uint8_t orderMapMarkerMasks[ORDER_MAP_MARKER_STYLES][ORDER_MAP_MARKER_WIDTH][ORDER_MAP_MARKER_WIDTH];
BtechPosition3D combatWorldBoundaryCorners[2];
int16_t unitOrderMapScreenX[COMBAT_MECH_SLOTS],unitOrderMapScreenY[COMBAT_MECH_SLOTS];

/* Actual fresh image/BSS boundary.0040 itself does not blanket-clear state. */
bool BtechImportOrderInitialData(const uint8_t *data,size_t bytes)
{
    if (!data || bytes<0x4F5A || MwReadNativeWord(data,0x4F2E) || MwReadNativeWord(data,0x4F30)) return false;
    memcpy(orderPanelText,data+ORDER_PANEL_TEXT_START,sizeof(orderPanelText));
    /* Authored string directories are decoded once at executable IO. Runtime
     * panels use the references below, never native-address accessors. */
    for (uint16_t directory=0;directory<4;++directory)
    {
        const uint16_t bases[]={0x02B6,0x02DA,0x02EC,0x02FA};
        const uint16_t counts[]={ORDER_LABELS,ORDER_TACTICAL_LABELS,ORDER_DAMAGE_LABELS,COMBAT_WEAPON_KINDS};
        const char **destinations[]={orderLabels,tacticalOrderLabels,orderDamageLabels,orderEquipmentWeaponLabels};
        for (uint16_t entry=0;entry<counts[directory];++entry)
        {
            uint16_t offset=MwReadNativeWord(data,bases[directory]+entry*2);
            if (offset<ORDER_PANEL_TEXT_START || offset>=ORDER_PANEL_TEXT_END || !memchr(data+offset,0,ORDER_PANEL_TEXT_END-offset)) return false;
            destinations[directory][entry]=orderPanelText+offset-ORDER_PANEL_TEXT_START;
        }
    }
    orderWeaponHeading=orderPanelText+0x026C-ORDER_PANEL_TEXT_START;
    orderOfText=orderPanelText+0x027A-ORDER_PANEL_TEXT_START;
    orderColon=orderPanelText+0x027D-ORDER_PANEL_TEXT_START;
    orderSelectionHeading=orderPanelText+0x027F-ORDER_PANEL_TEXT_START;
    orderLocationText=orderPanelText+0x028D-ORDER_PANEL_TEXT_START;
    orderBaseText=orderPanelText+0x029A-ORDER_PANEL_TEXT_START;
    orderDamageText=orderPanelText+0x02A7-ORDER_PANEL_TEXT_START;
    orderWeaponsText=orderPanelText+0x02AE-ORDER_PANEL_TEXT_START;
    for (uint16_t code=0;code<ORDER_DISPLAY_CODES;++code) orderDisplayIndices[code]=MwReadNativeWord(data,0x02C4+code*2);
    for (uint16_t scheme=0;scheme<2;++scheme)
        for (uint16_t condition=0;condition<4;++condition)
        {
            orderEngineConditionColours[scheme][condition]=MwReadNativeWord(data,0x030E+scheme*8+condition*2);
            orderSystemConditionColours[scheme][condition]=MwReadNativeWord(data,0x031E+scheme*8+condition*2);
        }
    for (uint16_t kind=0;kind<COMBAT_WEAPON_KINDS;++kind)
    {mountedWeaponCountsByKind[kind]=MwReadNativeWord(data,0x4F32+kind*2);operationalWeaponCountsByKind[kind]=MwReadNativeWord(data,0x4F46+kind*2);}
    orderPanelCaptureBuffer=(BtechResourceAllocation){0};
    for (uint16_t selection=0;selection<ORDER_SELECTION_CLASSES;++selection)
    {
        orderSelectionFirstRows[selection]=(int16_t)MwReadNativeWord(data,0x0092+selection*2);
        orderSelectionLastRows[selection]=(int16_t)MwReadNativeWord(data,0x0096+selection*2);
    }
    for (uint16_t direction=0;direction<ORDER_NAVIGATION_CODES;++direction)
    {
        orderNavigationPanelDeltas[direction]=(int16_t)MwReadNativeWord(data,0x009A+direction*2);
        orderNavigationRowDeltas[direction]=(int16_t)MwReadNativeWord(data,0x00B0+direction*2);
    }
    for (uint16_t scale=0;scale<ORDER_MAP_SCALE_COUNT;++scale) orderMapScaleShifts[scale]=data[0x0088+scale];
    for (uint16_t style=0;style<ORDER_MAP_MARKER_STYLES;++style)
        for (uint16_t x=0;x<ORDER_MAP_MARKER_WIDTH;++x)
            for (uint16_t y=0;y<ORDER_MAP_MARKER_WIDTH;++y) orderMapMarkerMasks[style][x][y]=data[0x00E8+style*9+x*3+y];
    for (uint16_t corner=0;corner<2;++corner)
    {
        uint32_t *position[]={&combatWorldBoundaryCorners[corner].positionA,&combatWorldBoundaryCorners[corner].positionB,&combatWorldBoundaryCorners[corner].positionC};
        for (uint16_t axis=0;axis<3;++axis) *position[axis]=MwReadNativeWord(data,0x0336+corner*12+axis*4)|((uint32_t)MwReadNativeWord(data,0x0338+corner*12+axis*4)<<16);
    }
    for (uint16_t slot=0;slot<COMBAT_MECH_SLOTS;++slot)
    {unitOrderMapScreenX[slot]=(int16_t)MwReadNativeWord(data,0x4F08+slot*2);unitOrderMapScreenY[slot]=(int16_t)MwReadNativeWord(data,0x4F18+slot*2);}
    for (uint16_t panel=0;panel<3;++panel) lanceOrderPanelTextX[panel]=MwReadNativeWord(data,0x008C+panel*2);
    orderScreenFullRedrawPending=data[0x4EEC];
    previousOrderHighlightPanel=MwReadNativeWord(data,0x4EEE);
    previousOrderHighlightRow=MwReadNativeWord(data,0x4EF4);
    orderMapPositionMask=MwReadNativeWord(data,0x4EF0)|((uint32_t)MwReadNativeWord(data,0x4EF2)<<16);
    for (uint16_t panel=0;panel<ORDER_WINGMATE_PANELS;++panel)
    {
        if (MwReadNativeWord(data,0x4EFC+panel*2)) return false; /* Fresh EXEC has no live near control references. */
        lanceOrderControls[panel]=NULL;
    }
    orderMapScaleIndex=MwReadNativeWord(data,0x4EF6);
    selectedLanceOrderPanel=MwReadNativeWord(data,0x4EF8);selectedOrderPanelRow=MwReadNativeWord(data,0x4EFA);
    orderPanelMode=MwReadNativeWord(data,0x4F02);orderActionSelection=MwReadNativeWord(data,0x4F04);
    pendingOrderCommand=MwReadNativeWord(data,0x4F06);
    orderDestinationRepeatCountdown=MwReadNativeWord(data,0x4F28);
    previousOrderMapScaleIndex=MwReadNativeWord(data,0x4F2A);previousOrderNavigationKey=MwReadNativeWord(data,0x4F2C);
    return true;
}
