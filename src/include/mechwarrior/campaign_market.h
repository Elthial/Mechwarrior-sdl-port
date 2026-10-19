#ifndef MECHWARRIOR_CAMPAIGN_MARKET_H
#define MECHWARRIOR_CAMPAIGN_MARKET_H
#include "mechwarrior/campaign_service.h"

enum { RESERVE_AMMO_MENU_ROWS=13,RESERVE_AMMO_DONE_ROW=12,RESERVE_AMMO_MACHINE_GUN_FAMILY=5,
    RESERVE_AMMO_MACHINE_GUN_BATCH=10,RESERVE_AMMO_ROW_Y=0x82,RESERVE_AMMO_ROW_HEIGHT=6,
    RESERVE_AMMO_PRICE_X=0x98,RESERVE_AMMO_QUANTITY_X=0xD7,
    RESERVE_AMMO_PREVIEW_WIDTH=0x6C,RESERVE_AMMO_PREVIEW_HEIGHT=0x3E };
typedef struct MwReserveAmmoQuantityRow {MwPanelText display;char text[6];} MwReserveAmmoQuantityRow;
extern uint16_t reserveAmmoShopSelectedRow,reserveAmmoPreviewSelection;
extern uint8_t reserveAmmoSelling,reserveAmmoMechColours[11];
extern char reserveAmmoBuyLabel[5],reserveAmmoSellLabel[5];
extern MwServiceCaption reserveAmmoShopHeading,reserveAmmoFamilyLabels[6],reserveAmmoDoneLabel;
extern MwReserveAmmoQuantityRow reserveAmmoQuantityRows[6];
extern MwPanelDescriptor reserveAmmoShopPanel,reserveAmmoCompatibilityPanels[6];
extern MwServiceCaption reserveAmmoCompatibilityHeadings[6];
extern uint16_t reserveAmmoCompatibleMechs[6][6];
extern MwUiRecord reserveAmmoShopRecords[RESERVE_AMMO_MENU_ROWS];
extern MwUiMenu reserveAmmoShopMenu;
uint16_t MwOpenReserveAmmunitionShop(void);
void MwFormatReserveAmmunitionQuantity(uint16_t family,uint16_t quantity);
void MwDrawReserveAmmunitionPrices(bool selling);
void MwDrawAmmunitionShopCompanyFunds(void);
void MwRefreshReserveAmmunitionSelection(void);
void MwShowReserveAmmunitionCompatibility(uint16_t family);
void MwDrawReserveAmmunitionCompatibleMechs(uint16_t family);
void MwMarkOwnedAmmunitionCompatibleMechs(void);
void MwBuyReserveAmmunition(void);
uint16_t MwSellReserveAmmunition(void);

enum { MECH_SERVICE_ROW_REARM=2, MECH_SERVICE_ROW_SELL=3,
    REARM_DIALOG_X=0x10, REARM_DIALOG_Y=0x8E, REARM_DIALOG_WIDTH=0xB2, REARM_DIALOG_HEIGHT=0x37,
    SALE_DIALOG_X=0x0A, SALE_DIALOG_Y=0x72, SALE_DIALOG_WIDTH=0xC8, SALE_DIALOG_HEIGHT=0x50,
    DEALER_LIST_NAME_COLUMNS=0x0C, DEALER_AMMO_ROW_HEIGHT=6,
    MECH_COMPLEX_ANIMATION_FRAMES=0x17, MECH_COMPLEX_ANIMATION_END=0xFFFF,
    MECH_COMPLEX_ANIMATION_PAUSE_MAXIMUM=0x1E, MECH_COMPLEX_ANIMATION_PAUSE_MINIMUM=6 };
enum { REARM_QUOTE_X=0x6A, REARM_QUOTE_Y=0x96, REARM_FUNDS_X=0x4C, REARM_FUNDS_Y=0xA2,
    SALE_QUOTE_X=0x1A, SALE_QUOTE_Y=0x8A, DEALER_PRICE_X=0x32, DEALER_PRICE_Y=0x84,
    DEALER_FUNDS_X=0x38, DEALER_FUNDS_Y=0x90, MARKET_PRICE_FIELD_DIGITS=8,
    MARKET_MESSAGE_X=0x10, MARKET_MESSAGE_Y=0x10,
    REARM_FULL_MESSAGE_X=0x12, REARM_FULL_MESSAGE_COLUMNS=0x15, REARM_FULL_MESSAGE_ROWS=5,
    DEALER_QUOTA_MESSAGE_COLUMNS=0x20, DEALER_QUOTA_MESSAGE_ROWS=6,
    PURCHASE_FUNDS_MESSAGE_COLUMNS=0x1B, PURCHASE_FUNDS_MESSAGE_ROWS=5,
    OWNERSHIP_LIMIT_MESSAGE_X=0x0C, OWNERSHIP_LIMIT_MESSAGE_Y=8,
    OWNERSHIP_LIMIT_MESSAGE_COLUMNS=0x1E, OWNERSHIP_LIMIT_MESSAGE_ROWS=4,
    DEALER_EMPTY_MESSAGE_Y=0x0C, DEALER_EMPTY_MESSAGE_COLUMNS=0x1C, DEALER_EMPTY_MESSAGE_ROWS=5,
    DEALER_COMPONENT_DONE_Y=0xB8, DEALER_COMPONENT_FIXED_LINES=0x0D,
    MECH_COMPLEX_ANIMATION_X=0x60, MECH_COMPLEX_ANIMATION_Y=0x40,
    MECH_COMPLEX_HUB_ROW=1, MECH_COMPLEX_FIRST_VISIT=0xFFFF,
    MECH_COMPLEX_REVIEW_CONTEXT=0, MECH_COMPLEX_DEALER_CONTEXT=2, MECH_COMPLEX_EVENT_CLASS=4,
    REARM_QUOTE_TIME_UNITS=1, SALE_QUOTE_TIME_UNITS=2, DEALER_QUOTA_RESTRICTED_TIER=2 };
typedef struct MwMarketUnitListRow
{
    MwPanelText display;
    char text[DEALER_LIST_NAME_COLUMNS+1];
} MwMarketUnitListRow;

extern MwPanelDescriptor rearmConfirmationPanel, saleConfirmationPanel;
extern MwPanelDescriptor marketUnitListPanel, marketMechStatusPanel, marketDropShipStatusPanel;
extern MwPanelDescriptor marketComponentPanel, mechComplexPanel;
extern MwServiceCaption rearmConfirmationLabels[3], saleConfirmationLabels[3];
extern MwServiceCaption marketListHeading, marketMechStatusLabels[4], marketDropShipStatusLabels[3];
extern MwServiceCaption mechComplexLabels[5], marketComponentTitle;
extern MwMarketUnitListRow marketUnitListRows[0x0D];
extern MwComponentConditionTextRow marketSystemConditionRows[0x0A], marketWeaponConditionRows[0x0A];
extern MwServiceCaption marketComponentDone;
extern MwServiceCaption marketWeaponHeading;
extern MwServiceCaption positionedText;
extern MwBodyConditionText marketBodyCondition;
extern MwUiMenu rearmConfirmationMenu, saleConfirmationMenu, marketUnitListMenu;
extern MwUiMenu marketMechMenu, marketDropShipMenu, marketComponentMenu, mechComplexMenu;
extern char ammoBayFullText[0x12], noMarketMechsText[0x2E];
extern char dealerPurchaseLimitText[0x33], purchaseInsufficientFundsText[0x17], ownedMechCapacityText[0x37];
/* The existing repair menu's FF probe is inside this very message. */
#define marketCapacityNavigationProbe (*(uint8_t *)&ownedMechCapacityText[3])
extern char marketListDoneText[DEALER_LIST_NAME_COLUMNS];
extern uint16_t selectedMarketUnitSlot, selectedMarketUnitByteOffset;
extern uint16_t selectedMarketAmmoFamilies[CAMPAIGN_AMMO_GROUPS], selectedMarketAmmoQuantities[CAMPAIGN_AMMO_GROUPS];
extern uint16_t marketSystemRowIndex, purchaseOwnedSlotByteOffset;
extern uint16_t mechComplexAnimationFrames[MECH_COMPLEX_ANIMATION_FRAMES];
/* First-visit6236 is a native event call with AX4, not a no-argument callback. */
uint16_t MwQuoteSelectedMechRearm(void);
uint16_t MwRearmSelectedMech(void);
uint16_t MwCancelSelectedMechRearm(void);
void MwRestoreMechServiceDialogRegion(void);
void MwCalculateSelectedUnitNetPrice(void);
uint16_t MwBeginOwnedUnitSaleCallback(void);
uint16_t MwSaleContinuation(void);
uint16_t MwCancelOwnedUnitSale(void);
uint16_t MwOpenMarketMechList(void);
uint16_t MwSelectMarketUnitListRowCallback(void);
void MwInitializeMechServiceIdle(void);
/* entryOffset is shared-entry/JMP transport, not a native source argument. */
uint16_t MwDrawMarketMechDetailContinuation(uint16_t entryOffset);
uint16_t MwOpenMarketMechComponentInspectionCallback(void);
uint16_t MwPurchaseSelectedMarketUnit(uint16_t entryOffset);
/*2DE2 resumes after6236; entryOffset is host transport, not a native argument. */
uint16_t MwEnterMechServiceMenuContinuation(uint16_t entryOffset);
void MwAnimateMechServiceIdle(void);

#endif
