#ifndef MECHWARRIOR_CAMPAIGN_SERVICE_H
#define MECHWARRIOR_CAMPAIGN_SERVICE_H
#include "mechwarrior/ega_campaign.h"
#include "mechwarrior/campaign_state.h"
#include "mechwarrior/campaign_mechanics.h"
#include "mechwarrior/campaign_economy.h"
#include "mechwarrior/campaign_ui.h"

/* The native header is eight WORDs followed by a near-pointer directory.
 * Here the directory holds real references to the same mutable text rows. */
enum { CAMPAIGN_PANEL_MAX_LINES = 0x20 };
typedef struct MwPanelDescriptor
{
    uint16_t x, y, width, bottomInset;
    uint16_t lineCount, borderEnabled, lineSpacing, background;
    MwPanelText *lines[CAMPAIGN_PANEL_MAX_LINES];
} MwPanelDescriptor;

typedef struct MwOwnedUnitListRow
{
    MwPanelText display;
    union
    {
        struct { char mechName[0x0E], mechWarriorName[0x0A], terminator; } columns;
        char text[0x19];
    } caption;
} MwOwnedUnitListRow;

typedef struct MwComponentConditionTextRow
{
    MwPanelText display;
    union
    {
        struct { char componentLabel[0x0D], condition[0x0C], terminator; } columns;
        char text[0x1A];
    } caption;
} MwComponentConditionTextRow;

typedef struct MwServiceCaption
{
    MwPanelText display;
    char text[0x80];
} MwServiceCaption;

typedef struct MwBodyConditionText
{
    MwPanelText display;
    union
    {
        struct { char armourLabel[0x0D], protectionPercent[3], percentSuffix[3]; } columns;
        char text[0x13];
    } caption;
} MwBodyConditionText;

typedef struct MwMechDiagramRectangle
{
    uint16_t x, y, width, height;
} MwMechDiagramRectangle;
typedef struct MwMechBodyDiagramLayout
{
    MwMechDiagramRectangle background, bodyRegions[9];
} MwMechBodyDiagramLayout;
_Static_assert(sizeof(MwMechDiagramRectangle)==8, "Four native WORD coordinates");
void MwDrawDescriptorTextPanel(const MwPanelDescriptor *textPanel);
void MwDrawStoredStandardPanelBorder(void);
extern uint16_t panelBorderEnabled;
/* Host continuation transport only: zero means native RET, otherwise JMP to
 * the returned CS081B offset. Each entry documents whether native POPs the IP.
 * These WORDs are not native AX results or new gameplay action enums. */
uint16_t MwOpenOwnedMechList(void);
uint16_t MwSelectOwnedUnitListRowCallback(void);
void MwDrawMechComplexBackground(void);
void MwDrawMechInspectionGrid(void);
void MwDrawMechBodyDamageDiagram(uint16_t unitKind, CampaignUnitDamage *damage);
void MwRefreshSelectedMechDetailPicture(void);
uint16_t MwOpenSelectedMechDetailContinuation(void);
uint16_t MwReturnToSelectedMechDetailCallback(void);
uint16_t MwDrawSelectedMechDetailContinuation(uint16_t entryOffset);
uint16_t MwOpenMechComponentRepairMenuCallback(void);
void MwRefreshSelectedComponentRepairQuote(void);
uint16_t MwRepairSelectedMechComponentCallback(void);
uint16_t MwRepairAllSelectedMechComponentsCallback(void);
enum { REPAIR_UI_RECORD_COUNT=0x0016,
    MECH_SERVICE_ROW_COMPONENT_REPAIR=0,MECH_SERVICE_ROW_REPAIR_ALL=1,
    REPAIR_COMPONENT_LABEL_COLUMNS=0x0D,REPAIR_CONDITION_HEALTHY_COLOUR=Campaign_Pal_EGA_LightGrey,
    REPAIR_CONDITION_DAMAGED_COLOUR=Campaign_Pal_EGA_LightRed,REPAIR_BODY_TOTAL_TIERS=0x001B,
    REPAIR_WEAPON_MENU_X=0x000C,
    REPAIR_WEAPON_FIRST_Y=0x0060,REPAIR_WEAPON_ROW_HEIGHT=6,
    REPAIR_DONE_X=0x004D,REPAIR_DONE_Y=0x00A2,
    REPAIR_PANEL_X=8,REPAIR_PANEL_Y=8,REPAIR_PANEL_WIDTH=0x00A2,REPAIR_PANEL_HEIGHT=0x00B8,
    REPAIR_FUNDS_X=0x0036,REPAIR_FUNDS_Y=0x00B7,
    REPAIR_COMPONENT_PRICE_X=0x0066,REPAIR_COMPONENT_PRICE_Y=0x00AB,REPAIR_PRICE_DIGITS=8 };
enum { REPAIR_ATTEMPT_ROLL_MAXIMUM=0x0064,REPAIR_MESSAGE_X=0x0010,REPAIR_MESSAGE_Y=0x0010,
    REPAIR_MESSAGE_ROWS=5,REPAIR_NO_WORK_COLUMNS=0x001C,REPAIR_CANNOT_AFFORD_COLUMNS=0x001F,
    REPAIR_CANNOT_AFFORD_ROWS=6,REPAIR_RESULT_COLUMNS=0x0022,REPAIR_FAILURE_COLUMNS=0x001A };
enum { INSPECTION_GRID_X=0x00AA,INSPECTION_GRID_LAST_X=0x013F,
    INSPECTION_GRID_WIDTH=0x0096,INSPECTION_GRID_LAST_Y=0x00C7,INSPECTION_GRID_SPACING=0x000A,
    INSPECTION_GRID_LAST_INTERIOR_X=0x0136,INSPECTION_GRID_LAST_INTERIOR_Y=0x00BE,
    INSPECTION_GRID_BACKGROUND=Campaign_Pal_EGA_LightBlue,INSPECTION_GRID_LINE_COLOUR=Campaign_Pal_EGA_DarkGrey,
    INSPECTION_CAPTURE_X=0x00A0,INSPECTION_CAPTURE_WIDTH=0x00A0,
    INSPECTION_PUBLISH_Y=6,INSPECTION_PUBLISH_WIDTH=0x009A,INSPECTION_PUBLISH_HEIGHT=0x00BC,
    MECH_BODY_DIAGRAM_BACKGROUND=Campaign_Pal_EGA_LightGrey,MECH_DETAIL_PICTURE_BASE=0x000C,DROPSHIP_DETAIL_PICTURE=0x004B,
    DETAIL_PICTURE_X_INSET=5,CAMPAIGN_BODY_REAR_CENTRE_TORSO=6 };
enum { MECH_DETAIL_BACKGROUND=Campaign_Pal_EGA_DarkGrey,MECH_DETAIL_TEXT_COLOUR=Campaign_Pal_EGA_LightRed,
    MECH_FUNCTIONAL_MAXIMUM_TIER=2,MECH_DETAIL_NAME_X=0x002C,MECH_DETAIL_NAME_Y=0x0018,
    MECH_DETAIL_CONDITION_X=0x004A,MECH_DETAIL_CONDITION_Y=0x0024,
    MECH_DETAIL_REPAIR_PRICE_X=0x0056,MECH_DETAIL_REPAIR_PRICE_Y=0x0030,
    MECH_DETAIL_TONNAGE_X=0x0038,MECH_DETAIL_TONNAGE_Y=0x003C,
    MECH_DETAIL_SPEED_X=0x0032,MECH_DETAIL_SPEED_Y=0x0048,
    MECH_DETAIL_JUMP_CAPACITY_X=0x0044,MECH_DETAIL_JUMP_CAPACITY_Y=0x0054,
    MECH_DETAIL_AMMO_LABEL_X=0x001A,MECH_DETAIL_AMMO_QUANTITY_X=0x0062,
    MECH_DETAIL_AMMO_FIRST_Y=0x006C,MECH_DETAIL_AMMO_ROW_HEIGHT=8 };
extern uint16_t unitInspectionContextCode;
extern uint16_t ownedUnitListSelectedRow;
extern uint16_t marketUnitListSelectedRow;
extern uint16_t mechServiceMenuSelectedRow;
extern MwPanelDescriptor ownedUnitListPanel;
extern MwOwnedUnitListRow ownedUnitListRows[0x000D];
extern uint16_t selectedOwnedUnitSlot;
extern uint16_t selectedOwnedUnitByteOffset;
extern uint16_t selectedMechAmmoFamilies[0x0003];
extern uint16_t selectedMechAmmoQuantities[0x0003];
extern uint8_t unitConditionSystemFieldCounts[0x000A];
extern uint8_t unitConditionScoreDivisors[0x000B];
extern uint8_t mechNonfunctionalTextBytes[0x000E];
extern uint8_t unitDisplayTonnages[0x000A];
extern uint8_t unitDisplaySpeedsKph[0x000A];
extern uint8_t unitDisplayJumpCapacitiesMetres[0x000A];
extern uint16_t unitDetailPictureXOffsets[0x000B];
extern uint16_t unitDetailPictureYOrigins[0x000B];
extern uint8_t repairWorkIncomplete;
extern MwPanelDescriptor mechStatusPanel;
extern MwPanelDescriptor dropShipStatusPanel;
extern uint16_t ownedListRowUnitByteOffset;
extern uint16_t saleMarketSlotByteOffset;
extern uint16_t selectedMechRearmQuoteLow;
extern uint16_t selectedMechRearmQuoteHigh;
extern uint16_t mechServiceNextAnimationTick;
extern uint16_t mechServiceAnimationByteOffset;
extern uint16_t repairTitleLeadingSpacesByKind[0x000B];
extern uint16_t repairSystemRowIndex;
extern uint16_t repairMenuSelectedCategory;
extern MwPanelDescriptor componentRepairPanel;
extern MwComponentConditionTextRow repairSystemConditionRows[0x000A];
extern MwComponentConditionTextRow repairWeaponConditionRows[0x000B];
extern MwUiRecord componentRepairUiRecords[REPAIR_UI_RECORD_COUNT];
extern uint8_t unitHeatSinkCounts[0x000A];
extern uint8_t repairNoJumpJetsTextBytes[0x0005];
extern uint8_t repairJumpJetCountTemplateBytes[0x0009];
extern uint8_t repairSuccessRollLimitsByMarketTier[0x0004];
extern uint16_t unusedRepairTimeByDamageTier[0x0004];
extern uint16_t mechDiagramUnitKind;
extern uint16_t bodyDamageTierFillColours[0x0004];
extern MwMechBodyDiagramLayout mechBodyDiagramLayouts[0x0008];
extern MwUiRecord ownedUnitListUiRecords[0x000D];
extern MwUiMenu ownedUnitListMenu, mechServiceMenu, dropShipServiceMenu, componentRepairMenu;
extern MwServiceCaption ownedUnitListHeading, mechStatusLabels[6], dropShipStatusLabels[3];
extern MwServiceCaption repairTitle, repairFooter;
extern MwBodyConditionText repairBodyCondition;
extern char shopUnitNames[CAMPAIGN_UNIT_KIND_COUNT][0x0F];
extern char unitDisplayNames[CAMPAIGN_UNIT_KIND_COUNT][0x0D];
extern char ammoFamilyDisplayNames[CAMPAIGN_AMMO_FAMILY_COUNT][0x0D];
extern char campaignDamageTierLabels[4][0x0D];
#define mechFunctionalTextBytes campaignDamageTierLabels[0]
extern char unassignedMechWarriorText[0x0B], ownedListDoneText[0x18];
extern char cannotAffordRepairText[0x37], noRepairWorkText[0x1A];
extern char repairCompleteText[0x20], repairIncompleteText[0x1E], repairFailedText[0x17];
extern char repairWeaponLabelStorage[CAMPAIGN_MECH_KIND_COUNT][CAMPAIGN_WEAPON_SLOTS+1][0x0F];
extern char *repairWeaponLabels[CAMPAIGN_MECH_KIND_COUNT][CAMPAIGN_WEAPON_SLOTS+2];
extern uint8_t *mechDiagramBodyDamage;
extern MwMechDiagramRectangle *mechDiagramRectangle;
extern MwMechBodyDiagramLayout *mechBodyDiagramLayoutsByKind[CAMPAIGN_MECH_KIND_COUNT];
#endif
