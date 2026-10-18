#ifndef MECHWARRIOR_COMBAT_ORDERS_H
#define MECHWARRIOR_COMBAT_ORDERS_H
#include "mechwarrior/combat_video.h"
#include "mechwarrior/combat_world.h"

enum { ORDER_MAP_FULL_SCALE=2,ORDER_PANEL_SUMMARY=0,ORDER_PANEL_COMMAND=1,ORDER_PANEL_TARGET=3,
    ORDER_PANEL_DESTINATION=2,ORDER_PANEL_DAMAGE=4,ORDER_PANEL_WEAPONS=5,
    ORDER_FIRST_ACTION_ROW=3,ORDER_REFERENCE_ROW=4,ORDER_WINGMATE_PANELS=3,
    ORDER_DESTINATION_REPEAT_TICKS=5,ORDER_NO_HIGHLIGHT=0xFFFF,
    ORDER_CONDITION_BANDS=5,ORDER_CONDITION_SCALE=4,
    ORDER_PANEL_INSET=2,ORDER_PANEL_WIDTH=0x51,ORDER_PANEL_HEIGHT=0x3B,
    ORDER_PANEL_TOP=0x81,ORDER_HIGHLIGHT_FIRST_ROW_Y=0x7F,ORDER_HIGHLIGHT_ROW_BASE_Y=0x83,
    ORDER_PANEL_ROW_STEP=8,ORDER_HIGHLIGHT_HEIGHT=9,ORDER_MAP_PROJECTION_BASE_SHIFT=7,
    ORDER_CONDITION_FAILED_BACKGROUND=4 };
enum { ORDER_NAVIGATION_CODES=11,ORDER_SELECTION_CLASSES=2,ORDER_ACTION_COMMAND=0,
    ORDER_ACTION_REFERENCE=1,ORDER_ACTION_SYSTEMS=2,ORDER_ACTION_WEAPONS=3,
    ORDER_SPECIAL_TARGET_SLOT=8,ORDER_TARGET_CANDIDATES=9,ORDER_TARGET_SCAN_LIMIT=10,
    ORDER_DESTINATION_REPEAT_SHIFT=2,ORDER_DESTINATION_LIMIT_X=0xB480,ORDER_DESTINATION_LIMIT_Y=0x65C0 };
enum { ORDER_LABELS=7,ORDER_COMMAND_MENU_ROWS=6,ORDER_DISPLAY_CODES=11,ORDER_TACTICAL_LABELS=9,
    ORDER_DAMAGE_LABELS=7,ORDER_DISPLAY_RETREATED=6,ORDER_PANEL_TEXT_START=0x010C,ORDER_PANEL_TEXT_END=0x02B6,
    ORDER_PANEL_NAME_Y=0x81,ORDER_PANEL_CHASSIS_Y=0x89,ORDER_PANEL_TACTICAL_Y=0x91,
    ORDER_PANEL_COMMAND_Y=0x9D,ORDER_PANEL_REFERENCE_Y=0xA5,ORDER_PANEL_DAMAGE_Y=0xAD,ORDER_PANEL_WEAPONS_Y=0xB5,
    ORDER_PANEL_DETAIL_FIRST_Y=0x8D,ORDER_PANEL_MARKER_X=0x42,ORDER_PANEL_MARKER_Y=0x83,
    ORDER_PANEL_TEXT_COLOUR=15 };
extern BtechResourceAllocation orderPanelCaptureBuffer;
extern uint16_t lanceOrderPanelTextX[3];
extern uint8_t orderScreenFullRedrawPending;
extern uint16_t previousOrderHighlightPanel,previousOrderHighlightRow;
extern uint32_t orderMapPositionMask;
extern BtechUnitControlState *lanceOrderControls[ORDER_WINGMATE_PANELS];
extern int16_t orderSelectionFirstRows[ORDER_SELECTION_CLASSES],orderSelectionLastRows[ORDER_SELECTION_CLASSES];
extern int16_t orderNavigationPanelDeltas[ORDER_NAVIGATION_CODES],orderNavigationRowDeltas[ORDER_NAVIGATION_CODES];
extern const char *orderLabels[ORDER_LABELS],*tacticalOrderLabels[ORDER_TACTICAL_LABELS];
extern const char *orderDamageLabels[ORDER_DAMAGE_LABELS],*orderEquipmentWeaponLabels[COMBAT_WEAPON_KINDS];
extern const char *orderWeaponHeading,*orderOfText,*orderColon,*orderSelectionHeading;
extern const char *orderLocationText,*orderBaseText,*orderDamageText,*orderWeaponsText;
extern uint16_t orderDisplayIndices[ORDER_DISPLAY_CODES];
extern uint16_t orderEngineConditionColours[2][4],orderSystemConditionColours[2][4];
extern uint16_t mountedWeaponCountsByKind[COMBAT_WEAPON_KINDS],operationalWeaponCountsByKind[COMBAT_WEAPON_KINDS];
extern uint16_t orderMapScaleIndex,selectedLanceOrderPanel,selectedOrderPanelRow;
extern uint16_t orderPanelMode,orderActionSelection,pendingOrderCommand;
extern uint16_t orderDestinationRepeatCountdown,previousOrderMapScaleIndex,previousOrderNavigationKey;
enum { ORDER_MAP_SCALE_COUNT=3,ORDER_MAP_MARKER_STYLES=4,ORDER_MAP_MARKER_WIDTH=3 };
extern uint8_t orderMapScaleShifts[ORDER_MAP_SCALE_COUNT];
extern uint8_t orderMapMarkerMasks[ORDER_MAP_MARKER_STYLES][ORDER_MAP_MARKER_WIDTH][ORDER_MAP_MARKER_WIDTH];
extern BtechPosition3D combatWorldBoundaryCorners[2];
extern int16_t unitOrderMapScreenX[COMBAT_MECH_SLOTS],unitOrderMapScreenY[COMBAT_MECH_SLOTS];
uint16_t BtechShiftSignedDwordRightLow(uint32_t value,uint16_t shifts);
uint8_t BtechProjectPositionToOrderMap(const BtechPosition3D *position,int16_t *screenX,int16_t *screenY);
void BtechDrawThreeByThreeMarker(int16_t x,int16_t y,uint16_t colour,uint16_t style);
void BtechDrawLanceMapUnitMarkers(void);
void BtechDrawLanceMapRetreatBoundaries(void);
void BtechCopyHudRectangleIfReady(uint16_t x,uint16_t y,uint16_t width,uint16_t height);
void BtechInitializeOrderPanelState(void);
void BtechReleaseOrderPanelBuffer(void);
uint32_t BtechShiftDwordLeftPair(uint32_t value,uint16_t shiftCount);
uint32_t BtechBuildOrderMapPositionMask(void);
void BtechSetConditionTextColours(uint16_t conditionColour);
uint16_t BtechGetMechConditionColour(int16_t unitSlot);
void BtechUpdateLanceOrderHighlight(void);
void BtechClearLanceOrderPanel(uint16_t panelIndex);
void BtechEditSelectedUnitOrders(void);
void BtechSeedSelectedOrderTracking(void);
uint16_t BtechClassifyOrderSelection(uint16_t commandCode);
void BtechDrawUnitOrderPanels(void);
void BtechRefreshLanceOrderScreen(void);
bool BtechImportOrderInitialData(const uint8_t *data,size_t bytes);
#endif
