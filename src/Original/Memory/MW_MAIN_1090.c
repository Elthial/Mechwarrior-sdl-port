#include "mechwarrior/campaign.h"
#include "mechwarrior/campaign_mechanics.h"
#include "mechwarrior/campaign_economy.h"
#include "mechwarrior/campaign_ui.h"
#include "mechwarrior/campaign_input.h"
#include "mechwarrior/campaign_service.h"
#include "mechwarrior/campaign_market.h"
#include "mechwarrior/original_data.h"
#include "mechwarrior/campaign_startup.h"
#include "mechwarrior/campaign_events.h"
#include "mechwarrior/campaign_state.h"
#include "mechwarrior/campaign_pilots.h"
#include "mechwarrior/campaign_contracts.h"
#include "mechwarrior/campaign_travel.h"
#include "mechwarrior/campaign_options.h"
#include "mechwarrior/campaign_handoff.h"
#include <stdlib.h>
#include <string.h>

_Static_assert(sizeof(uint8_t) == 1, "Native BYTE width");
_Static_assert(sizeof(uint16_t) == 2, "Native WORD width");
uint8_t campaignTimerInhibited, campaignTimerByte;
char combatExecutableFilename[10];
MwCombatDiskPrompt combatDiskPrompt;
uint8_t deploymentResolvedWithoutCombat;
char darkWingVictoryText[0x185];
char afterActionCrewHeading[0x0D],afterActionMechHeading[0x0D];
char afterActionSalvageHeading[9],afterActionPaymentHeading[9];
char afterActionPercentSuffix[2],afterActionPilotStatusText[3][8];
uint16_t afterActionRosterRowOffset;
uint16_t campaignMechTonnages[CAMPAIGN_MECH_KIND_COUNT];
uint16_t settlementPayoutPercent;
uint16_t houseDeductionRollThresholds[CAMPAIGN_HOUSE_COUNT];
uint16_t houseReducedPayoutPercentages[CAMPAIGN_HOUSE_COUNT];
uint16_t missionDeploymentUnitsByScenario[DEPLOYMENT_SCENARIO_TIME_COUNT];
char deploymentUneventfulText[0x56];
char afterActionKuritaFailure[0x8D],afterActionLiaoFailure[0x73],afterActionGeneralFailure[0x91];
char afterActionKuritaSuccess[0x62],afterActionLiaoSuccess[0x6C],afterActionGeneralSuccess[0x44];
char settlementSupplyDeductionText[0xA8],settlementPerformanceDeductionText[0xAC];
char settlementTransportDeductionText[0x98],settlementNoPaymentText[0x95],darkWingAttackFailedText[0x49];
const char *afterActionFailureMessages[CAMPAIGN_HOUSE_COUNT]={afterActionKuritaFailure,
    afterActionGeneralFailure,afterActionGeneralFailure,afterActionLiaoFailure,afterActionGeneralFailure};
const char *afterActionSuccessMessages[CAMPAIGN_HOUSE_COUNT]={afterActionKuritaSuccess,
    afterActionGeneralSuccess,afterActionGeneralSuccess,afterActionLiaoSuccess,afterActionGeneralSuccess};
const char *settlementDeductionMessages[3]={settlementSupplyDeductionText,
    settlementPerformanceDeductionText,settlementTransportDeductionText};
uint16_t campaignTimerTicks;
uint8_t campaignTimerAlternatingPhase, campaignTimerPhaseTicks;
uint8_t mouseInstalled, joystickInstalled, inputSinglePoll;
uint16_t joystickDirection, joystickButtonOnePressed, joystickButtonTwoPressed;
uint8_t joystickPreviousButtons;
uint16_t joystickXCount, joystickYCount, joystickDeflection;
uint16_t mouseMotionX,mouseMotionY;
uint8_t joystickRepeatActive;
uint16_t joystickRepeatDeadline,joystickNavigationKeys[9];
uint16_t joystickCentreX,joystickCentreY;
uint16_t joystickXDeflectionStep,joystickYDeflectionStep;
uint16_t joystickXLow, joystickXHigh, joystickYLow, joystickYHigh;
uint16_t joystickXFarLow, joystickXFarHigh, joystickYFarLow, joystickYFarHigh;

/* Original UI tables occupy1083..1154 without an overlapping raw DS image.
 * Authored records/text still require native-image initialization. */
MwUiRecord planetHubRecords[HUB_UI_RECORD_COUNT];
MwUiRecord contractHubRecords[CONTRACT_HUB_UI_RECORD_COUNT];
MwUiRecord remotePlanetHubRecords[HUB_UI_RECORD_COUNT];
MwUiMenu *uiMenu;
uint16_t selectedUiRecordIndex;
uint16_t uiSelectionChangedCodeOffset, uiIdleCodeOffset;
/* FF link probes are read BEFORE sentinel rejection. No invented early guard. */
uint8_t hubFirstNavigationProbe,hubSecondNavigationProbe,hubThirdNavigationProbe;
uint16_t panelOriginX, panelOriginY, panelWidth, panelHeight;
uint16_t panelBorderPictureBase;
uint16_t panelBorderEnabled;
uint8_t commanderNameBytes[0x000A];
uint16_t ownedAmmoKindsByGroup[CAMPAIGN_AMMO_GROUPS * CAMPAIGN_OWNED_UNIT_SLOTS];
uint16_t ownedAmmoQuantitiesByGroup[CAMPAIGN_AMMO_GROUPS * CAMPAIGN_OWNED_UNIT_SLOTS];
uint16_t textPanelInsetX, textPanelInsetY;
uint16_t textPanelOriginX, textPanelOriginY;
uint16_t textPanelWidthPixels, textPanelHeightPixels;
uint16_t textPanelPreviousFont, textPanelFont;
uint16_t textPanelContinuationMode, textPanelYOverride;
uint16_t notificationTextInsetX[4], notificationTextInsetY[4];
uint16_t notificationColumns[4], notificationRows[4];
uint16_t notificationTextOffsets[4];
uint8_t notificationTextStorage[NOTIFICATION_TEXT_END - NOTIFICATION_TEXT_START];
MwChangeDiskPrompt changeDiskPrompt;
MwInsertDiskPrompt insertDiskOnePrompt;

/* Original 1090:016D. The allocator/loader assigns this selector later. */
uint16_t compressedPictureBufferSelector;

/* Original 1090:03F5/03F7 executable initializers. Do not substitute libc rand
 * or randomize these before the native BIOS seed call. */
uint16_t randomStateWord0 = 0x0615;
uint16_t randomStateWord1 = 0x1988;

/* Original 1090:0F6D..0F76; decoder-private storage. BYTE phases remain BYTEs,
 * not bools: direct helpers treat every nonzero phase alike. One canonical
 * owner, not a second copy of a raw DS image or serialized host structure. */
uint16_t pictureCompressedByteCount;
uint16_t pictureOutputSelector;
uint8_t pictureReadHighNibbleNext;
uint16_t pictureReadByteOffset;
uint8_t pictureWriteLowNibbleNext;
uint16_t pictureWriteByteOffset;

/* Native allocated-buffer WORDs; backing buffers belong to the allocator. */
uint16_t indexedPictureOutputSelector, generalPictureBankSelector;
uint16_t cockpitPictureBankSelector, activePictureBankSelector;
uint16_t secondaryPictureBankSelector;
uint8_t indexedPictureDirectoryLoaded;
uint16_t indexedPictureCachedIndex, indexedPictureStoredBytes;
uint8_t indexedPictureDirectory[INDEXED_PICTURE_DIRECTORY_BYTES];
uint16_t indexedPictureFileHandle, generalPictureFileHandle;
uint16_t selectedPictureFileHandle;
const char indexedPictureFilename[] = "MW_PICS.BIN";
const char generalPictureFilename[] = "MW_GPICS.BIN";
const char cockpitPictureFilename[] = "MW_CPICS.BIN";
const char primaryPictureFilename[] = "MW_1PICS.BIN";
const char secondaryPictureFilename[] = "MW_2PICS.BIN";
const char tPictureFilename[] = "MW_TPICS.BIN";
const char alternatePictureFilename[] = "MW_APICS.BIN";
const char fPictureFilename[] = "MW_FPICS.BIN";
/* Native private filename pointer; identity is tested only against 0E2C.
 * Host pointer keeps that identity; never serialize this as a native WORD. */
const char *selectedPictureFilename;

/* Native record region, not another full DS mirror. Initial data/near offsets
 * come from the original image; future mutable views must use this same owner. */
uint16_t planetRecordOffsets[CAMPAIGN_PLANET_COUNT];
uint8_t planetRecordStorage[PLANET_RECORDS_END - PLANET_RECORDS_START];
const uint8_t *calendarMonthNames[CAMPAIGN_MONTH_COUNT];
uint16_t calendarMonthNameWidths[CAMPAIGN_MONTH_COUNT];
uint16_t calendarMonthIndex, calendarYear;
uint16_t calendarTextX, calendarTextY;
uint8_t calendarBackground, calendarForeground;
uint8_t numericLeadingDigitSeen, numericGroupingEnabled;
uint8_t numericDecimalDigits[DECIMAL_DIGIT_COUNT];
uint16_t numericRemainderHigh, numericRemainderLow;
uint8_t numericSuppressLeadingZeros, numericNegativeSign;
uint16_t numericFieldDigitCount, numericCursorX, numericCursorY;
uint8_t numericBackgroundColour, numericForegroundColour;

/* 1ACE's planet/market outputs are original WORDs even for BYTE record fields.
 * Original image tables are populated by native data initialization, not by a
 * substitute safe/default market. The compiled startup is still unconverted. */
uint16_t currentPlanetIndex, currentPlanetLandscapeIndex;
uint16_t currentPlanetHouseIndex, currentPlanetMarketTier;
uint16_t currentPlanetFacilityDisplayFlag;
uint16_t currentPlanetMapPositionX, currentPlanetMapPositionY;
uint16_t planetMarketPremiumPercent, contractNegotiationAttemptLimit;
uint8_t houseNegotiationAttemptBase[CAMPAIGN_HOUSE_COUNT];
uint16_t ammunitionBasePrices[AMMUNITION_PRICE_COUNT];
uint16_t ammunitionTransactionPrices[AMMUNITION_PRICE_COUNT];
/* Shared by kind/component, NOT by owned Mech slot. 3FFD clears all 00DC bytes,
 * including the two trailing slots per row not reached by normal repair work. */
CampaignRepairAttemptRow repairAttemptFailuresByKind[REPAIR_ATTEMPT_KIND_COUNT];
_Static_assert(sizeof(repairAttemptFailuresByKind) == 0x00DC, "Native reset extent");

/* Native game tables/state. Original image initialization remains required;
 * no default prices, authored lookup values or pilot definitions are invented. */
CampaignUnitDamage ownedUnitDamage[CAMPAIGN_OWNED_UNIT_SLOTS];
CampaignUnitDamage marketUnitDamage[CAMPAIGN_MARKET_UNIT_SLOTS];
CampaignUnitDamage generatedMechDamage;
uint8_t unitWeaponCounts[CAMPAIGN_MECH_KIND_COUNT];
uint8_t unitPriceTierMultipliers[UNIT_PRICE_TIER_COUNT];
uint16_t unitBasePrices[CAMPAIGN_UNIT_KIND_COUNT];
uint16_t unitTransactionPriceLow, unitTransactionPriceHigh;
uint16_t selectedOwnedUnitKind, selectedMarketUnitKind;
uint16_t unitRepairQuoteLow, unitRepairQuoteHigh;
uint16_t repairQuoteMechKind;
CampaignUnitDamage *repairQuoteDamage;
uint16_t componentRepairPriceLow, componentRepairPriceHigh;
uint16_t repairPriceCategory;
CampaignUnitDamage *componentRepairDamage;
CampaignSystemRepairPriceRow systemRepairBasePrices[CAMPAIGN_SYSTEM_COUNT];
uint16_t *weaponRepairPricesByKind[CAMPAIGN_MECH_KIND_COUNT];
uint16_t weaponRepairPrices[REPAIR_WEAPON_PRICE_WORD_COUNT];
uint8_t bodyRepairBasePricesByKind[CAMPAIGN_MECH_KIND_COUNT];

CampaignPilotIdentityStorage pilotIdentityStorage;
uint8_t rosterStatus[CAMPAIGN_ROSTER_SLOTS];
uint16_t rosterPayTiers[CAMPAIGN_ROSTER_SLOTS];
uint16_t rosterGunneryTiers[CAMPAIGN_ROSTER_SLOTS];
uint16_t rosterPilotingTiers[CAMPAIGN_ROSTER_SLOTS];
uint16_t rosterUnknownWords[CAMPAIGN_ROSTER_SLOTS];
uint16_t rosterMechAssignments[CAMPAIGN_ROSTER_SLOTS];
uint16_t rosterPortraitIds[CAMPAIGN_ROSTER_SLOTS];
uint8_t pilotGunneryTiers[CAMPAIGN_PILOT_ID_COUNT];
uint8_t pilotPilotingTiers[CAMPAIGN_PILOT_ID_COUNT];
uint8_t pilotExperienceCounts[CAMPAIGN_PILOT_ID_COUNT];
uint16_t pilotDefinitionOffsets[CAMPAIGN_PILOT_ID_COUNT];
uint8_t pilotDefinitionStorage[PILOT_DEFINITIONS_END - PILOT_DEFINITIONS_START];

uint16_t travelSquareRootValues[TRAVEL_ROOT_TABLE_COUNT];
uint16_t travelDestinationMapPositionX, travelDestinationMapPositionY;
uint16_t travelMapDeltaX, travelMapDeltaY, travelDestinationTimeWeight;
uint16_t travelDestinationPlanetOffset, travelDistanceUnits;
uint16_t travelQuoteCbillsLow, travelQuoteCbillsHigh;
uint16_t travelMapSelection, travelDestinationPlanetIndex;
uint16_t travelMarkerX, travelMarkerY;
uint16_t travelSavedMarkerX, travelSavedMarkerY;
uint16_t travelJoystickLastTick, travelAnimationFrameStep;
uint8_t travelDropShipSelected;
uint16_t hiredTransportReverseTerminator;
uint16_t hiredTransportFrames[HIRED_TRANSPORT_FRAME_COUNT];
uint16_t dropShipTravelFrames[DROPSHIP_TRAVEL_FRAME_COUNT];
char travelDestinationName[TRAVEL_PLANET_NAME_CAPACITY];
uint16_t alphabeticPlanetRecordOffsets[TRAVEL_PLANET_DIRECTORY_COUNT];
uint16_t travelSelectedHouseByteOffset, travelHousePlanetStart;
uint16_t travelHousePlanetCount;
uint16_t travelSelectedHouseRow;
char travelHouseSearchNames[TRAVEL_HOUSE_COUNT][0x10];
MwPanelDescriptor travelHouseBrowserPanel, travelHousePlanetPanel;
MwServiceCaption travelHouseBrowserLabels[TRAVEL_HOUSE_BROWSER_LINE_COUNT];
MwServiceCaption travelHouseSearchCaption;
MwServiceCaption travelHousePlanetRows[TRAVEL_PLANET_ROWS_PER_HOUSE];
MwServiceCaption travelHousePlanetCancelLabel;
static MwUiRecord travelHouseBrowserRecords[TRAVEL_HOUSE_BROWSER_RECORD_COUNT];
static MwUiRecord travelHousePlanetRecords[TRAVEL_HOUSE_PLANET_RECORD_COUNT];
static MwUiRecord travelMapRecords[TRAVEL_MAP_MENU_RECORD_COUNT];
MwUiMenu travelHouseBrowserMenu={travelHouseBrowserRecords,TRAVEL_HOUSE_BROWSER_RECORD_COUNT,
    &planetRecordStorage[0x686D-PLANET_RECORDS_START]};
MwUiMenu travelHousePlanetMenu={travelHousePlanetRecords,TRAVEL_HOUSE_PLANET_RECORD_COUNT,
    NULL,&planetRecordOffsets[(0x65FA-0x6570)/sizeof(uint16_t)],8};
MwUiMenu travelMapMenu={travelMapRecords,TRAVEL_MAP_MENU_RECORD_COUNT,
    &planetRecordStorage[0x697E-PLANET_RECORDS_START]};
MwPanelDescriptor travelDestinationHeaderPanel;
MwPanelDescriptor travelCostPanel, travelConfirmPanel, travelBrowsePanel;
MwServiceCaption travelDestinationHeaderLabels[2];
MwServiceCaption travelCostLabel, travelConfirmLabel, travelBrowseLabel;
char travelPopulationSuffix[9];
char travelLandscapeLabels[TRAVEL_LANDSCAPE_LABEL_COUNT][9];
char travelInsufficientFundsText[0x29];
uint16_t reserveAmmoQuantitiesByFamily[CAMPAIGN_AMMO_FAMILY_COUNT];
uint16_t reserveAmmoFreightRates[CAMPAIGN_AMMO_FAMILY_COUNT];

/* Native stock generator policy and scratch; populated from the original image. */
uint8_t generatedKindRangeMaxima[STOCK_KIND_RANGE_COUNT];
uint8_t generatedKindRangeStarts[STOCK_KIND_RANGE_COUNT];
uint16_t generatedDamageMaximumIndex, generatedUnitKind;
uint16_t generatedAmmoKinds[CAMPAIGN_AMMO_GROUPS];
uint16_t generatedAmmoQuantities[CAMPAIGN_AMMO_GROUPS];
uint8_t generateStartingJenner;
uint8_t unitAmmoGroupCounts[CAMPAIGN_MECH_KIND_COUNT];
CampaignAmmoProfileEntry *unitAmmoProfiles[CAMPAIGN_MECH_KIND_COUNT];
CampaignAmmoProfileEntry unitAmmoProfileEntries[AMMO_PROFILE_ENTRY_COUNT];
uint8_t unitJumpJetGroupCounts[CAMPAIGN_MECH_KIND_COUNT];
uint16_t dealerUnitPurchaseLimit, unitsSoldSinceMarketRefresh;
uint16_t marketUnitCount, marketUnitKinds[CAMPAIGN_MARKET_UNIT_SLOTS];
uint16_t marketAmmoKindsByGroup[CAMPAIGN_AMMO_GROUPS * CAMPAIGN_MARKET_UNIT_SLOTS];
uint16_t marketAmmoQuantitiesByGroup[CAMPAIGN_AMMO_GROUPS * CAMPAIGN_MARKET_UNIT_SLOTS];
uint16_t availableRecruitCount, availableRecruitIds[RECRUIT_OFFER_SLOTS];
uint16_t ownedUnitCount, ownedUnitKinds[CAMPAIGN_OWNED_UNIT_SLOTS];
uint16_t houseFavorPoints[CAMPAIGN_HOUSE_COUNT], houseGrievancePoints[CAMPAIGN_HOUSE_COUNT];
uint16_t playerCombatCredit, generatingMissionOfferSlot;
uint16_t missionForceBudget, missionPaymentBaseThousands;
uint8_t missionForceWeightByUnitKind[CAMPAIGN_MECH_KIND_COUNT];
uint16_t housePaymentRollBias[CAMPAIGN_HOUSE_COUNT];
int16_t housePaymentAdjustmentPercent[CAMPAIGN_HOUSE_COUNT];
uint16_t houseSalvageBasePercent[CAMPAIGN_HOUSE_COUNT], houseAdvanceBasePercent[CAMPAIGN_HOUSE_COUNT];
uint8_t houseMissionTypeLimits[CAMPAIGN_HOUSE_COUNT * MISSION_TARGET_COLUMNS];
uint8_t majorCampaignMissionRollThreshold;
uint16_t missionDescriptionOffsets[MISSION_OFFER_SLOTS], missionPlanetRecordIndices[MISSION_OFFER_SLOTS];
uint16_t missionTargetHouseIndices[MISSION_OFFER_SLOTS], missionTypeCodes[MISSION_OFFER_SLOTS];
uint16_t missionEnemyHeavyCounts[MISSION_OFFER_SLOTS], missionEnemyMediumCounts[MISSION_OFFER_SLOTS];
uint16_t missionEnemyLightCounts[MISSION_OFFER_SLOTS], missionPaymentThousands[MISSION_OFFER_SLOTS];
uint16_t missionSalvagePercentages[MISSION_OFFER_SLOTS], missionAdvancePercentages[MISSION_OFFER_SLOTS];
uint16_t missionPaymentCeilingsThousands[MISSION_OFFER_SLOTS], missionSalvagePercentageCeilings[MISSION_OFFER_SLOTS];
uint16_t missionAdvancePercentageCeilings[MISSION_OFFER_SLOTS], missionScenarioCodes[MISSION_OFFER_SLOTS];
uint16_t housePlanetRangeStarts[CAMPAIGN_HOUSE_COUNT], housePlanetRangeCounts[CAMPAIGN_HOUSE_COUNT];
uint8_t missionDescriptionMaximumIndicesByCategory[MISSION_CATEGORY_COUNT];
uint16_t missionDescriptionListOffsetsByCategory[MISSION_CATEGORY_COUNT];
uint16_t missionDescriptionListEntries[MISSION_DESCRIPTION_LIST_WORDS];
uint8_t missionDescriptionStorage[MISSION_DESCRIPTION_STORAGE_END - MISSION_DESCRIPTION_STORAGE_START];
uint16_t houseMissionPlanetTableOffsets[CAMPAIGN_HOUSE_COUNT];
MwHouseMissionPlanetPolicy houseMissionPlanetPolicies[CAMPAIGN_HOUSE_COUNT];
uint16_t calendarSubmonthUnits, missionOfferCount;
uint16_t marketRefreshUnits, elapsedBillingMonths;
uint16_t campaignFundsLow, campaignFundsHigh;
uint16_t dropShipMissedUpkeepPayments, unpaidPilotWageMarker;
uint16_t pilotWageAmounts[CAMPAIGN_ROSTER_SLOTS];
/* 0D49 can clear04BF+pilot42 =04E9, immediately after the identity union.
 * Do not extend that aligned union through04EA and duplicate pay-tier storage. */
uint8_t pilotDepartureFlagTail;
uint8_t notificationPending[CAMPAIGN_NOTIFICATION_COUNT];

/* Mech Complex data: one owner per object, no emulated DS or address registry. */
uint16_t unitInspectionContextCode;
uint16_t ownedUnitListSelectedRow;
uint16_t marketUnitListSelectedRow;
uint16_t mechServiceMenuSelectedRow;
MwPanelDescriptor ownedUnitListPanel;
MwOwnedUnitListRow ownedUnitListRows[0x000D];
uint16_t selectedOwnedUnitSlot;
uint16_t selectedOwnedUnitByteOffset;
uint16_t selectedMechAmmoFamilies[0x0003];
uint16_t selectedMechAmmoQuantities[0x0003];
uint8_t unitConditionSystemFieldCounts[0x000A];
uint8_t unitConditionScoreDivisors[0x000B];
uint8_t mechNonfunctionalTextBytes[0x000E];
uint8_t unitDisplayTonnages[0x000A];
uint8_t unitDisplaySpeedsKph[0x000A];
uint8_t unitDisplayJumpCapacitiesMetres[0x000A];
uint16_t unitDetailPictureXOffsets[0x000B];
uint16_t unitDetailPictureYOrigins[0x000B];
uint8_t repairWorkIncomplete;
MwPanelDescriptor mechStatusPanel;
MwPanelDescriptor dropShipStatusPanel;
/* Private backing arrays; shared access goes through the typed menus below. */
static MwUiRecord mechServiceUiRecords[0x0005];
static MwUiRecord dropShipServiceUiRecords[0x0002];
uint16_t ownedListRowUnitByteOffset;
uint16_t saleMarketSlotByteOffset;
uint16_t selectedMechRearmQuoteLow;
uint16_t selectedMechRearmQuoteHigh;
uint16_t mechServiceNextAnimationTick;
uint16_t mechServiceAnimationByteOffset;
uint16_t repairTitleLeadingSpacesByKind[0x000B];
uint16_t repairSystemRowIndex;
uint16_t repairMenuSelectedCategory;
MwPanelDescriptor componentRepairPanel;
MwComponentConditionTextRow repairSystemConditionRows[0x000A];
MwComponentConditionTextRow repairWeaponConditionRows[0x000B];
MwUiRecord componentRepairUiRecords[REPAIR_UI_RECORD_COUNT];
uint8_t unitHeatSinkCounts[0x000A];
uint8_t repairNoJumpJetsTextBytes[0x0005];
uint8_t repairJumpJetCountTemplateBytes[0x0009];
uint8_t repairSuccessRollLimitsByMarketTier[0x0004];
uint16_t unusedRepairTimeByDamageTier[0x0004];
uint16_t mechDiagramUnitKind;
uint16_t bodyDamageTierFillColours[0x0004];
MwMechBodyDiagramLayout mechBodyDiagramLayouts[0x0008];
MwUiRecord ownedUnitListUiRecords[0x000D];
MwServiceCaption ownedUnitListHeading, mechStatusLabels[6], dropShipStatusLabels[3];
MwServiceCaption repairTitle, repairFooter;
MwBodyConditionText repairBodyCondition;
char shopUnitNames[CAMPAIGN_UNIT_KIND_COUNT][0x0F];
char unitDisplayNames[CAMPAIGN_UNIT_KIND_COUNT][0x0D];
char ammoFamilyDisplayNames[CAMPAIGN_AMMO_FAMILY_COUNT][0x0D];
char campaignDamageTierLabels[4][0x0D];
char unassignedMechWarriorText[0x0B], ownedListDoneText[0x18];
char cannotAffordRepairText[0x37], noRepairWorkText[0x1A];
char repairCompleteText[0x20], repairIncompleteText[0x1E], repairFailedText[0x17];
char repairWeaponLabelStorage[CAMPAIGN_MECH_KIND_COUNT][CAMPAIGN_WEAPON_SLOTS+1][0x0F];
char *repairWeaponLabels[CAMPAIGN_MECH_KIND_COUNT][CAMPAIGN_WEAPON_SLOTS+2];
uint8_t *mechDiagramBodyDamage;
MwMechDiagramRectangle *mechDiagramRectangle;
MwMechBodyDiagramLayout *mechBodyDiagramLayoutsByKind[CAMPAIGN_MECH_KIND_COUNT];

MwUiMenu planetHubMenu = { planetHubRecords, HUB_UI_RECORD_COUNT, &hubFirstNavigationProbe };
MwUiMenu contractHubMenu = { contractHubRecords, CONTRACT_HUB_UI_RECORD_COUNT, &hubSecondNavigationProbe };
MwUiMenu remotePlanetHubMenu = { remotePlanetHubRecords, HUB_UI_RECORD_COUNT, &hubThirdNavigationProbe };
/* FF probes alias362D (record15 X high),3B45 (kind9 failure8), and3B8B
 * (sensor price for kind8 low). Keep typed aliases, not host-endian BYTE casts. */
MwUiMenu ownedUnitListMenu = { ownedUnitListUiRecords, 0x0D, NULL, &componentRepairUiRecords[15].x, 8 };
MwUiMenu mechServiceMenu = { mechServiceUiRecords, 5, &repairAttemptFailuresByKind[9].failedByDamageIndex[8] };
MwUiMenu dropShipServiceMenu = { dropShipServiceUiRecords, 2, NULL, &systemRepairBasePrices[REPAIR_SENSORS].basePriceByMechKind[CAMPAIGN_UNIT_MARAUDER], 0 };
MwUiMenu componentRepairMenu = { componentRepairUiRecords, REPAIR_UI_RECORD_COUNT, &marketCapacityNavigationProbe };

/* Rearm/dealer objects are ordinary C data, independent of their disk encoding. */
MwPanelDescriptor rearmConfirmationPanel, saleConfirmationPanel;
MwPanelDescriptor marketUnitListPanel, marketMechStatusPanel, marketDropShipStatusPanel;
MwPanelDescriptor marketComponentPanel, mechComplexPanel;
MwServiceCaption rearmConfirmationLabels[3], saleConfirmationLabels[3];
MwServiceCaption marketListHeading, marketMechStatusLabels[4], marketDropShipStatusLabels[3];
MwServiceCaption mechComplexLabels[5], marketComponentTitle;
MwMarketUnitListRow marketUnitListRows[0x0D];
MwComponentConditionTextRow marketSystemConditionRows[0x0A], marketWeaponConditionRows[0x0A];
MwServiceCaption marketComponentDone;
MwServiceCaption marketWeaponHeading;
MwServiceCaption positionedText;
uint16_t hubSelectedRecordIndex;
MwBodyConditionText marketBodyCondition;
static MwUiRecord rearmConfirmationRecords[2], saleConfirmationRecords[2], marketUnitListRecords[0x0D];
static MwUiRecord marketMechRecords[3], marketDropShipRecords[2], marketComponentRecords[1], mechComplexRecords[4];
/* FF navigation probes alias the canonical caption/diagram/root-table owners;
 * they are read before the dispatcher rejects the FF navigation link. */
MwUiMenu rearmConfirmationMenu={rearmConfirmationRecords,2,NULL,&mechBodyDiagramLayouts[7].bodyRegions[7].x,8};
MwUiMenu saleConfirmationMenu={saleConfirmationRecords,2,NULL,&mechBodyDiagramLayouts[6].bodyRegions[1].width,0};
MwUiMenu marketUnitListMenu={marketUnitListRecords,0x0D,(const uint8_t *)&repairWeaponLabelStorage[CAMPAIGN_UNIT_SHADOW_HAWK][4][2]}; /*380D, DONE label byte. */
MwUiMenu marketMechMenu={marketMechRecords,3,NULL,&travelSquareRootValues[0x02DA],8}; /*529B, high BYTE. */
MwUiMenu marketDropShipMenu={marketDropShipRecords,2,NULL,&travelSquareRootValues[0x02EF],8}; /*52C5, high BYTE. */
MwUiMenu marketComponentMenu={marketComponentRecords,1,(const uint8_t *)&positionedText.text[0x10]}; /*5601, final space. */
MwUiMenu mechComplexMenu={mechComplexRecords,4,NULL,&repairWeaponConditionRows[1].display.y,0};
char ammoBayFullText[0x12], noMarketMechsText[0x2E];
char dealerPurchaseLimitText[0x33], purchaseInsufficientFundsText[0x17], ownedMechCapacityText[0x37];
char marketListDoneText[DEALER_LIST_NAME_COLUMNS];
uint16_t selectedMarketUnitSlot, selectedMarketUnitByteOffset;
uint16_t selectedMarketAmmoFamilies[CAMPAIGN_AMMO_GROUPS], selectedMarketAmmoQuantities[CAMPAIGN_AMMO_GROUPS];
uint16_t marketSystemRowIndex, purchaseOwnedSlotByteOffset;
uint16_t mechComplexAnimationFrames[MECH_COMPLEX_ANIMATION_FRAMES];

uint16_t campaignMusicBufferSelector, graphicsOverlaySelector;
uint16_t launcherMemoryTier;
uint8_t launcherExitCode, tandyGraphicsSelected, savedVideoMode;
uint8_t keyboardInterruptState;
char startupErrorText[0x0120];
const char *startupErrorMessages[7];
uint16_t musicFileHandle;
char introNarrationLines[10][0x50];
MwIntroPositionedText introFirstLeftText,introFirstRightText;
MwIntroPositionedText introSecondLeftText,introSecondRightText;
MwIntroPositionedText introThirdLeftText,introThirdRightText;
MwIntroPositionedText introFourthLeftText,introFourthRightText;
MwIntroPositionedText introFifthLeftText,introFifthRightText;
MwIntroPositionedText introSixthLeftText,introSixthRightText;
MwIntroPositionedText introSeventhLeftText,introSeventhRightText;
uint16_t copyProtectionSelection;
uint16_t copyProtectionPictureIndices[COPY_PROTECTION_MECH_COUNT];
char copyProtectionCallsigns[COPY_PROTECTION_QUESTION_COUNT][COPY_PROTECTION_TEXT_CAPACITY];
char copyProtectionAnswers[COPY_PROTECTION_QUESTION_COUNT][COPY_PROTECTION_TEXT_CAPACITY];
char copyProtectionPrompt[0x85],copyProtectionAcceptedText[0x33];
uint8_t campaignTextInputBuffer[0x29];
uint16_t textInputCursorX,textInputCursorY,textInputMaximumIndex;
uint8_t textInputForeground,textInputBackground,textInputLastPhase;
uint16_t textInputLength;
uint16_t campaignEventPlanetSelectors[6];
uint8_t eventPlanetChoicePool0[8],eventPlanetChoicePool1[3];
uint8_t eventPlanetChoicePool2[4],eventPlanetChoicePool3[3];
uint8_t eventPlanetChoicePool4[4],eventPlanetChoicePool5[3];

MwCampaignEvent campaignEventRecords[CAMPAIGN_EVENT_RECORD_COUNT];
MwEventCommand campaignEventCommands[CAMPAIGN_EVENT_COMMAND_COUNT];
MwCampaignEvent *eventListEntries[CAMPAIGN_EVENT_LIST_ENTRIES];
MwCampaignEvent **eventLists[CAMPAIGN_EVENT_LIST_COUNT];
MwCampaignEvent *fixedCampaignEvent;
MwEventCaption eventChoiceCaptions[CAMPAIGN_EVENT_CAPTION_COUNT];
uint16_t eventDirectoryIndex,campaignEventClass,currentEventId,dispatchedEventCount;
uint8_t eventSeenFlags[CAMPAIGN_EVENT_SEEN_FLAGS];
uint8_t eventConditionResult,eventCombineWithAnd;
uint16_t eventScheduledMonthScratch,eventExpirySubmonthScratch,eventScheduledYearScratch;
uint8_t planetArrivalEventsPending,darkWingBaseVictoryRecorded,campaignRestartPending;
uint8_t eventChoiceCount,eventTextRowCount;
MwCampaignEvent *eventChoiceBranches[3];
uint16_t eventChoiceSavedFontIndex;
MwUiRecord eventTwoChoiceRecords[2],eventThreeChoiceRecords[3];
/* FF-link probes alias existing planet text bytes, NOT new globals at6C0E/2A. */
MwUiMenu eventTwoChoiceMenu={eventTwoChoiceRecords,2,&planetRecordStorage[0x6C0E-PLANET_RECORDS_START],NULL,0};
MwUiMenu eventThreeChoiceMenu={eventThreeChoiceRecords,3,&planetRecordStorage[0x6C2A-PLANET_RECORDS_START],NULL,0};
MwEventClassHandler eventClassHandlers[7]={MwShowCampaignEventText,MwShowCampaignEventText,
    MwShowCampaignEventText,MwShowClassThreeEventPages,MwShowCampaignEventText,
    MwShowCampaignEventText,MwShowCampaignEventText};
const MwEventCommand *eventOperandCursor;

uint8_t unidentifiedField18,missionCategoryOrMemoryTier,deploymentLandscapeIndex;
uint8_t combatEnemyHeavyCount,combatEnemyMediumCount,combatEnemyLightCount;
uint8_t combatPlanetSelector,combatScenarioCode,deploymentTerrainDetailLevel;
CampaignSalvageEntry salvageEntries[4];
uint16_t returnedDisabledEnemyCount;
uint8_t combatTransitionResult,acceptedContractActive;
uint8_t unidentifiedField19[2];
uint16_t commanderAgeYears,houseNetStandings[CAMPAIGN_HOUSE_COUNT];
uint16_t requestedContractPaymentThousands,requestedContractSalvagePercent;
uint16_t requestedContractAdvancePercent,requestContractPaymentEnabled;
uint16_t requestContractSalvageEnabled,requestContractAdvanceEnabled;
uint8_t unidentifiedField20[6];
uint16_t acceptedContractPaymentThousands,acceptedContractSalvagePercent;
uint16_t acceptedContractAdvancePercent,remainingContractPaymentThousands;
uint8_t audioOptionLabelIndex,campaignTerrainDetailLevel;
uint16_t acceptedContractTargetHouseIndex,playerSkillProgress;
uint8_t defaultHouseMissionTypeLimits[0x28],campaignDefaultCommanderNameBytes[0x0A];
uint16_t companyMenuSelectedRow,barMenuSelectedRow,optionsMenuSelection;

/* Installation data import. Decode once into the canonical C objects;
 * the input is never retained. These are file-format operations, not new
 * native game methods. Pointer topology is the reviewed shipped revision. */
/* Month strings end at4B19.4B1A onward belongs to contract date/editor data. */
static uint8_t calendarMonthText[0x6E];
static void ImportInitialWords(const uint8_t *data, uint16_t offset, uint16_t *words, size_t count)
{
    for (size_t i=0;i<count;++i) words[i]=MwReadNativeWord(data,(uint16_t)(offset+i*2));
}
static void ImportInitialText(const uint8_t *data, uint16_t offset, MwPanelText *line, char *text, size_t count)
{
    line->x=MwReadNativeWord(data,offset);
    line->y=MwReadNativeWord(data,(uint16_t)(offset+2));
    line->colour=data[(uint16_t)(offset+4)];
    memcpy(text,data+offset+5,count);line->text=text;
}
static bool ImportInitialString(const uint8_t *data, uint16_t offset, char *text, size_t capacity)
{
    if ((size_t)offset+capacity>NATIVE_SEGMENT_BYTE_COUNT) return false;
    const uint8_t *end=memchr(data+offset,0,capacity);
    if (!end) return false;
    memcpy(text,data+offset,(size_t)(end-(data+offset))+1);return true;
}
static bool ImportTravelPanel(const uint8_t *data, uint16_t panelOffset,
    MwPanelDescriptor *panel, MwServiceCaption *captions, uint16_t expectedLines)
{
    uint16_t header[8];ImportInitialWords(data,panelOffset,header,8);
    panel->x=header[0];panel->y=header[1];panel->width=header[2];panel->bottomInset=header[3];
    panel->lineCount=header[4];panel->borderEnabled=header[5];panel->lineSpacing=header[6];panel->background=header[7];
    if (panel->lineCount!=expectedLines) return false;
    for (uint16_t line=0;line<expectedLines;++line)
    {
        uint16_t offset=MwReadNativeWord(data,(uint16_t)(panelOffset+0x10+line*2));
        if (!ImportInitialString(data,(uint16_t)(offset+5),captions[line].text,sizeof(captions[line].text))) return false;
        ImportInitialText(data,offset,&captions[line].display,captions[line].text,0);
        panel->lines[line]=&captions[line].display;
    }
    return true;
}
static bool ImportTravelCaption(const uint8_t *data, uint16_t offset, MwServiceCaption *caption)
{
    if (!ImportInitialString(data,(uint16_t)(offset+5),caption->text,sizeof(caption->text))) return false;
    ImportInitialText(data,offset,&caption->display,caption->text,0);return true;
}
static void ImportTravelMenu(const uint8_t *data, uint16_t offset, MwUiMenu *menu)
{
    for (uint16_t index=0;index<menu->recordCount;++index)
    {
        const uint8_t *encoded=data+offset+index*CAMPAIGN_UI_RECORD_BYTES;
        MwUiRecord *record=&menu->records[index];
        record->enabled=encoded[0];record->textRecord=encoded[1];
        record->x=MwReadNativeWord(encoded,2);record->y=MwReadNativeWord(encoded,4);
        record->activateCodeOffset=MwReadNativeWord(encoded,6);record->pictureIndex=MwReadNativeWord(encoded,8);
        record->upRecordIndex=encoded[10];record->downRecordIndex=encoded[11];
        record->leftRecordIndex=encoded[12];record->rightRecordIndex=encoded[13];
        record->text=NULL;record->textColour=NULL;
    }
}
static bool ImportTravelUiData(const uint8_t *data)
{
    if (!ImportTravelPanel(data,0x5C35,&travelDestinationHeaderPanel,travelDestinationHeaderLabels,2)
        || !ImportTravelPanel(data,0x5BF9,&travelCostPanel,&travelCostLabel,1)
        || !ImportTravelPanel(data,0x5C17,&travelConfirmPanel,&travelConfirmLabel,1)
        || !ImportTravelPanel(data,0x5C75,&travelBrowsePanel,&travelBrowseLabel,1)
        || !ImportTravelPanel(data,0x5A0D,&travelHouseBrowserPanel,travelHouseBrowserLabels,TRAVEL_HOUSE_BROWSER_LINE_COUNT)) return false;
    uint16_t panelHeader[8];ImportInitialWords(data,0x54E4,panelHeader,8);
    travelHousePlanetPanel.x=panelHeader[0];travelHousePlanetPanel.y=panelHeader[1];
    travelHousePlanetPanel.width=panelHeader[2];travelHousePlanetPanel.bottomInset=panelHeader[3];
    travelHousePlanetPanel.lineCount=panelHeader[4];travelHousePlanetPanel.borderEnabled=panelHeader[5];
    travelHousePlanetPanel.lineSpacing=panelHeader[6];travelHousePlanetPanel.background=panelHeader[7];
    if (travelHousePlanetPanel.lineCount!=TRAVEL_HOUSE_PLANET_LINE_COUNT
        || !ImportTravelCaption(data,0x5534,&travelHouseSearchCaption)
        || !ImportTravelCaption(data,0x57FD,&travelHousePlanetCancelLabel)) return false;
    travelHousePlanetPanel.lines[0]=&travelHouseSearchCaption.display;
    for (uint16_t row=0;row<TRAVEL_PLANET_ROWS_PER_HOUSE;++row)
    {
        uint16_t rowOffset=0x554B+row*TRAVEL_PLANET_ROW_BYTES;
        if (!ImportTravelCaption(data,rowOffset,&travelHousePlanetRows[row])) return false;
        travelHousePlanetPanel.lines[row+1]=&travelHousePlanetRows[row].display;
    }
    travelHousePlanetPanel.lines[TRAVEL_HOUSE_PLANET_LINE_COUNT-1]=&travelHousePlanetCancelLabel.display;
    ImportTravelMenu(data,0x5A7B,&travelHouseBrowserMenu);
    for (uint16_t house=0;house<TRAVEL_HOUSE_COUNT;++house)
    {
        travelHouseBrowserMenu.records[house].text=travelHouseBrowserLabels[house+1].text;
        travelHouseBrowserMenu.records[house].textColour=&travelHouseBrowserLabels[house+1].display.colour;
        uint16_t textOffset=MwReadNativeWord(data,(uint16_t)(0x59BB+house*sizeof(uint16_t)));
        if (!ImportInitialString(data,textOffset,travelHouseSearchNames[house],sizeof(travelHouseSearchNames[house]))) return false;
    }
    travelHouseBrowserMenu.records[TRAVEL_HOUSE_COUNT].text=travelHouseBrowserLabels[TRAVEL_HOUSE_COUNT+1].text;
    travelHouseBrowserMenu.records[TRAVEL_HOUSE_COUNT].textColour=&travelHouseBrowserLabels[TRAVEL_HOUSE_COUNT+1].display.colour;
    ImportTravelMenu(data,0x5809,&travelHousePlanetMenu);
    for (uint16_t row=0;row<TRAVEL_PLANET_ROWS_PER_HOUSE;++row)
    {
        travelHousePlanetMenu.records[row].text=travelHousePlanetRows[row].text;
        travelHousePlanetMenu.records[row].textColour=&travelHousePlanetRows[row].display.colour;
    }
    travelHousePlanetMenu.records[TRAVEL_PLANET_ROWS_PER_HOUSE].text=travelHousePlanetCancelLabel.text;
    travelHousePlanetMenu.records[TRAVEL_PLANET_ROWS_PER_HOUSE].textColour=&travelHousePlanetCancelLabel.display.colour;
    ImportTravelMenu(data,0x5B8C,&travelMapMenu);
    travelMapMenu.records[0].text=travelDestinationHeaderLabels[0].text;
    travelMapMenu.records[0].textColour=&travelDestinationHeaderLabels[0].display.colour;
    travelMapMenu.records[1].text=travelCostLabel.text;
    travelMapMenu.records[1].textColour=&travelCostLabel.display.colour;
    travelMapMenu.records[2].text=travelBrowseLabel.text;
    travelMapMenu.records[2].textColour=&travelBrowseLabel.display.colour;
    travelMapMenu.records[3].text=travelConfirmLabel.text;
    travelMapMenu.records[3].textColour=&travelConfirmLabel.display.colour;
    if (!ImportInitialString(data,0x5B83,travelPopulationSuffix,sizeof(travelPopulationSuffix))) return false;
    const uint16_t landscapeLabels[TRAVEL_LANDSCAPE_LABEL_COUNT]={0x5CA6,0x5CAD,0x5CB6};
    for (uint16_t label=0;label<TRAVEL_LANDSCAPE_LABEL_COUNT;++label)
        if (!ImportInitialString(data,landscapeLabels[label],travelLandscapeLabels[label],sizeof(travelLandscapeLabels[label]))) return false;
    return true;
}
/* Native UI directories become typed references. Persistent pilot tables
 * remain parallel arrays; these objects do not invent a packed pilot record. */
MwPanelDescriptor companyMenuPanel,commanderProfilePanel,crewLabelsPanel,barMenuPanel,recruitInterviewPanel;
MwServiceCaption companyMenuLabels[4],commanderProfileLabels[11],crewLabels[7],barMenuLabels[4],recruitInterviewLabels[3];
char commanderProfileName[0x0A],commanderReputationLabels[4][0x10];
char houseAttitudeLabels[5][0x0A],pilotSkillLabels[4][0x0A];
const char *houseAttitudeText[9];
char pilotAssignmentCaptions[12][0x0C];
uint8_t pilotAssignmentCaptionColours[12];
uint16_t crewScreenRightEdge,pilotCardRosterSlot,pilotCardTextX,displayedPilotId;
uint16_t pilotCardGunneryTier,pilotCardPilotingTier,pilotCardPortraitId,pilotCardWageTier,pilotCardOwnedUnitSlot,pilotCardColumnOrigins[4];
uint16_t assignmentSelectedPilotSlot,assignmentCandidateOwnedSlot,assignmentConflictingPilotSlot,assignmentConflictingOwnedSlot;
uint8_t assignmentConflictHighlighted;
uint16_t recruitCandidateByteOffset,selectedRecruitPilotId;
char recruitStatLabels[0x17],noAvailableRecruitsText[0x21],fullFourPilotRosterText[0x33],fullThreePilotRosterText[0x35];
const MwPilotDefinitionPrefix *pilotDefinitions[CAMPAIGN_PILOT_ID_COUNT];
const char *pilotDialogue[CAMPAIGN_PILOT_ID_COUNT];
static MwUiRecord companyRecords[3],crewRecords[5],barRecords[3],recruitInterviewRecords[2];
/*0960 reads FF-link probes before rejecting the link. Options and restore
 * now own the mutable bytes also probed by Bar and recruitment. */
static uint8_t companyNavigationPadding;
MwUiMenu companyMenu={companyRecords,3,&companyNavigationPadding};
MwUiMenu crewMenu={crewRecords,5,&rearmConfirmationRecords[1].textRecord};
MwUiMenu barMenu={barRecords,3,NULL};
MwUiMenu recruitInterviewMenu={recruitInterviewRecords,2,NULL,&saveListRows[4].display.x,0};
static bool ImportPilotUiData(const uint8_t *data);
static bool ImportReserveAmmoUiData(const uint8_t *data);
static bool ImportContractData(const uint8_t *data);
bool MwImportCampaignInitialData(const uint8_t *data, size_t byteCount)
{
    if (!data || byteCount<NATIVE_SEGMENT_BYTE_COUNT) return false; /* Transient file image, not live storage. */
    memcpy(&campaignTimerInhibited,data+0x0007,sizeof(campaignTimerInhibited));
    memcpy(&campaignTimerByte,data+0x0008,sizeof(campaignTimerByte));
    memcpy(combatExecutableFilename,data+0x0EF6,sizeof(combatExecutableFilename));
    memcpy(&combatDiskPrompt,data+0x0F00,sizeof(combatDiskPrompt));
    memcpy(&deploymentResolvedWithoutCombat,data+0x140C,sizeof(deploymentResolvedWithoutCombat));
    memcpy(darkWingVictoryText,data+0x1A37,sizeof(darkWingVictoryText));
    memcpy(afterActionCrewHeading,data+0x11DF,sizeof(afterActionCrewHeading));
    memcpy(afterActionMechHeading,data+0x11EC,sizeof(afterActionMechHeading));
    memcpy(afterActionSalvageHeading,data+0x11F9,sizeof(afterActionSalvageHeading));
    memcpy(afterActionPaymentHeading,data+0x1202,sizeof(afterActionPaymentHeading));
    memcpy(afterActionPercentSuffix,data+0x120B,sizeof(afterActionPercentSuffix));
    memcpy(afterActionPilotStatusText,data+0x120D,sizeof(afterActionPilotStatusText));
    afterActionRosterRowOffset=MwReadNativeWord(data,0x1229);
    ImportInitialWords(data,0x122B,(uint16_t *)campaignMechTonnages,sizeof(campaignMechTonnages)/sizeof(uint16_t));
    settlementPayoutPercent=MwReadNativeWord(data,0x123F);
    ImportInitialWords(data,0x1241,(uint16_t *)houseDeductionRollThresholds,sizeof(houseDeductionRollThresholds)/sizeof(uint16_t));
    ImportInitialWords(data,0x124B,(uint16_t *)houseReducedPayoutPercentages,sizeof(houseReducedPayoutPercentages)/sizeof(uint16_t));
    ImportInitialWords(data,0x8E46,(uint16_t *)missionDeploymentUnitsByScenario,sizeof(missionDeploymentUnitsByScenario)/sizeof(uint16_t));
    memcpy(deploymentUneventfulText,data+0x140D,sizeof(deploymentUneventfulText));
    memcpy(afterActionKuritaFailure,data+0x149B,sizeof(afterActionKuritaFailure));
    memcpy(afterActionLiaoFailure,data+0x1528,sizeof(afterActionLiaoFailure));
    memcpy(afterActionGeneralFailure,data+0x159B,sizeof(afterActionGeneralFailure));
    memcpy(afterActionKuritaSuccess,data+0x1674,sizeof(afterActionKuritaSuccess));
    memcpy(afterActionLiaoSuccess,data+0x16D6,sizeof(afterActionLiaoSuccess));
    memcpy(afterActionGeneralSuccess,data+0x1742,sizeof(afterActionGeneralSuccess));
    memcpy(settlementSupplyDeductionText,data+0x178C,sizeof(settlementSupplyDeductionText));
    memcpy(settlementPerformanceDeductionText,data+0x1834,sizeof(settlementPerformanceDeductionText));
    memcpy(settlementTransportDeductionText,data+0x18E0,sizeof(settlementTransportDeductionText));
    memcpy(settlementNoPaymentText,data+0x19A2,sizeof(settlementNoPaymentText));
    memcpy(darkWingAttackFailedText,data+0x1BBC,sizeof(darkWingAttackFailedText));
    campaignTimerTicks=MwReadNativeWord(data,0x000C);
    memcpy(&campaignTimerAlternatingPhase,data+0x000E,sizeof(campaignTimerAlternatingPhase));
    memcpy(&campaignTimerPhaseTicks,data+0x000F,sizeof(campaignTimerPhaseTicks));
    memcpy(&mouseInstalled,data+0x02AC,sizeof(mouseInstalled));
    memcpy(&joystickInstalled,data+0x02AD,sizeof(joystickInstalled));
    memcpy(&inputSinglePoll,data+0x03E2,sizeof(inputSinglePoll));
    joystickDirection=MwReadNativeWord(data,0x02E6);
    joystickButtonOnePressed=MwReadNativeWord(data,0x02E8);
    joystickButtonTwoPressed=MwReadNativeWord(data,0x02EA);
    memcpy(&joystickPreviousButtons,data+0x02EC,sizeof(joystickPreviousButtons));
    joystickXCount=MwReadNativeWord(data,0x02ED);
    joystickYCount=MwReadNativeWord(data,0x02EF);
    joystickDeflection=MwReadNativeWord(data,0x02F1);
    mouseMotionX=MwReadNativeWord(data,0x03DA);
    mouseMotionY=MwReadNativeWord(data,0x03DC);
    memcpy(&joystickRepeatActive,data+0x02F3,sizeof(joystickRepeatActive));
    joystickRepeatDeadline=MwReadNativeWord(data,0x02F4);
    ImportInitialWords(data,0x02F6,(uint16_t *)joystickNavigationKeys,sizeof(joystickNavigationKeys)/sizeof(uint16_t));
    joystickCentreX=MwReadNativeWord(data,0x0308);
    joystickCentreY=MwReadNativeWord(data,0x030A);
    joystickXDeflectionStep=MwReadNativeWord(data,0x031C);
    joystickYDeflectionStep=MwReadNativeWord(data,0x031E);
    joystickXLow=MwReadNativeWord(data,0x030C);
    joystickXHigh=MwReadNativeWord(data,0x030E);
    joystickYLow=MwReadNativeWord(data,0x0310);
    joystickYHigh=MwReadNativeWord(data,0x0312);
    joystickXFarLow=MwReadNativeWord(data,0x0314);
    joystickXFarHigh=MwReadNativeWord(data,0x0316);
    joystickYFarLow=MwReadNativeWord(data,0x0318);
    joystickYFarHigh=MwReadNativeWord(data,0x031A);
    selectedUiRecordIndex=MwReadNativeWord(data,0x03E5);
    uiSelectionChangedCodeOffset=MwReadNativeWord(data,0x03E7);
    uiIdleCodeOffset=MwReadNativeWord(data,0x03E9);
    memcpy(&hubFirstNavigationProbe,data+0x1E75,sizeof(hubFirstNavigationProbe));
    memcpy(&hubSecondNavigationProbe,data+0x1EC9,sizeof(hubSecondNavigationProbe));
    memcpy(&hubThirdNavigationProbe,data+0x1EF3,sizeof(hubThirdNavigationProbe));
    panelOriginX=MwReadNativeWord(data,0x03EB);
    panelOriginY=MwReadNativeWord(data,0x03ED);
    panelWidth=MwReadNativeWord(data,0x03EF);
    panelHeight=MwReadNativeWord(data,0x03F1);
    panelBorderPictureBase=MwReadNativeWord(data,0x0444);
    panelBorderEnabled=MwReadNativeWord(data,0x03F3);
    memcpy(commanderNameBytes,data+0x0483,sizeof(commanderNameBytes));
    ImportInitialWords(data,0x0742,(uint16_t *)ownedAmmoKindsByGroup,sizeof(ownedAmmoKindsByGroup)/sizeof(uint16_t));
    ImportInitialWords(data,0x06A6,(uint16_t *)ownedAmmoQuantitiesByGroup,sizeof(ownedAmmoQuantitiesByGroup)/sizeof(uint16_t));
    textPanelInsetX=MwReadNativeWord(data,0x0432);
    textPanelInsetY=MwReadNativeWord(data,0x0434);
    textPanelOriginX=MwReadNativeWord(data,0x0436);
    textPanelOriginY=MwReadNativeWord(data,0x0438);
    textPanelWidthPixels=MwReadNativeWord(data,0x043A);
    textPanelHeightPixels=MwReadNativeWord(data,0x043C);
    textPanelPreviousFont=MwReadNativeWord(data,0x043E);
    textPanelFont=MwReadNativeWord(data,0x0440);
    textPanelContinuationMode=MwReadNativeWord(data,0x0442);
    textPanelYOverride=MwReadNativeWord(data,0x0446);
    ImportInitialWords(data,0x5EB3,(uint16_t *)notificationTextInsetX,sizeof(notificationTextInsetX)/sizeof(uint16_t));
    ImportInitialWords(data,0x5EBB,(uint16_t *)notificationTextInsetY,sizeof(notificationTextInsetY)/sizeof(uint16_t));
    ImportInitialWords(data,0x5EC3,(uint16_t *)notificationColumns,sizeof(notificationColumns)/sizeof(uint16_t));
    ImportInitialWords(data,0x5ECB,(uint16_t *)notificationRows,sizeof(notificationRows)/sizeof(uint16_t));
    ImportInitialWords(data,0x5ED3,(uint16_t *)notificationTextOffsets,sizeof(notificationTextOffsets)/sizeof(uint16_t));
    memcpy(notificationTextStorage,data+0x5EDB,sizeof(notificationTextStorage));
    memcpy(&changeDiskPrompt,data+0x0E71,sizeof(changeDiskPrompt));
    memcpy(&insertDiskOnePrompt,data+0x0ED6,sizeof(insertDiskOnePrompt));
    compressedPictureBufferSelector=MwReadNativeWord(data,0x016D);
    randomStateWord0=MwReadNativeWord(data,0x03F5);
    randomStateWord1=MwReadNativeWord(data,0x03F7);
    pictureCompressedByteCount=MwReadNativeWord(data,0x0F6D);
    pictureOutputSelector=MwReadNativeWord(data,0x0F6F);
    memcpy(&pictureReadHighNibbleNext,data+0x0F71,sizeof(pictureReadHighNibbleNext));
    pictureReadByteOffset=MwReadNativeWord(data,0x0F72);
    memcpy(&pictureWriteLowNibbleNext,data+0x0F74,sizeof(pictureWriteLowNibbleNext));
    pictureWriteByteOffset=MwReadNativeWord(data,0x0F75);
    indexedPictureOutputSelector=MwReadNativeWord(data,0x0161);
    generalPictureBankSelector=MwReadNativeWord(data,0x0163);
    cockpitPictureBankSelector=MwReadNativeWord(data,0x0165);
    activePictureBankSelector=MwReadNativeWord(data,0x0167);
    secondaryPictureBankSelector=MwReadNativeWord(data,0x0169);
    memcpy(&indexedPictureDirectoryLoaded,data+0x0B62,sizeof(indexedPictureDirectoryLoaded));
    indexedPictureCachedIndex=MwReadNativeWord(data,0x0B63);
    indexedPictureStoredBytes=MwReadNativeWord(data,0x0B65);
    memcpy(indexedPictureDirectory,data+0x0B67,sizeof(indexedPictureDirectory));
    indexedPictureFileHandle=MwReadNativeWord(data,0x0DBF);
    generalPictureFileHandle=MwReadNativeWord(data,0x0DDA);
    selectedPictureFileHandle=MwReadNativeWord(data,0x0DE9);
    ImportInitialWords(data,0x6570,(uint16_t *)planetRecordOffsets,sizeof(planetRecordOffsets)/sizeof(uint16_t));
    memcpy(planetRecordStorage,data+0x67B6,sizeof(planetRecordStorage));
    ImportInitialWords(data,0x11BA,(uint16_t *)calendarMonthNameWidths,sizeof(calendarMonthNameWidths)/sizeof(uint16_t));
    calendarMonthIndex=MwReadNativeWord(data,0x047B);
    calendarYear=MwReadNativeWord(data,0x047D);
    calendarTextX=MwReadNativeWord(data,0x117E);
    calendarTextY=MwReadNativeWord(data,0x1180);
    memcpy(&calendarBackground,data+0x1182,sizeof(calendarBackground));
    memcpy(&calendarForeground,data+0x1183,sizeof(calendarForeground));
    memcpy(&numericLeadingDigitSeen,data+0xA136,sizeof(numericLeadingDigitSeen));
    memcpy(&numericGroupingEnabled,data+0xA137,sizeof(numericGroupingEnabled));
    memcpy(numericDecimalDigits,data+0xA138,sizeof(numericDecimalDigits));
    numericRemainderHigh=MwReadNativeWord(data,0xA142);
    numericRemainderLow=MwReadNativeWord(data,0xA144);
    memcpy(&numericSuppressLeadingZeros,data+0xA146,sizeof(numericSuppressLeadingZeros));
    memcpy(&numericNegativeSign,data+0xA149,sizeof(numericNegativeSign));
    numericFieldDigitCount=MwReadNativeWord(data,0xA147);
    numericCursorX=MwReadNativeWord(data,0xA14A);
    numericCursorY=MwReadNativeWord(data,0xA14C);
    memcpy(&numericBackgroundColour,data+0xA14E,sizeof(numericBackgroundColour));
    memcpy(&numericForegroundColour,data+0xA14F,sizeof(numericForegroundColour));
    currentPlanetIndex=MwReadNativeWord(data,0x0469);
    currentPlanetLandscapeIndex=MwReadNativeWord(data,0x046B);
    currentPlanetHouseIndex=MwReadNativeWord(data,0x046D);
    currentPlanetMarketTier=MwReadNativeWord(data,0x046F);
    currentPlanetFacilityDisplayFlag=MwReadNativeWord(data,0x0471);
    currentPlanetMapPositionX=MwReadNativeWord(data,0x0473);
    currentPlanetMapPositionY=MwReadNativeWord(data,0x0475);
    planetMarketPremiumPercent=MwReadNativeWord(data,0x0477);
    contractNegotiationAttemptLimit=MwReadNativeWord(data,0x09E6);
    memcpy(houseNegotiationAttemptBase,data+0x11A3,sizeof(houseNegotiationAttemptBase));
    ImportInitialWords(data,0x2EF8,(uint16_t *)ammunitionBasePrices,sizeof(ammunitionBasePrices)/sizeof(uint16_t));
    ImportInitialWords(data,0x2F10,(uint16_t *)ammunitionTransactionPrices,sizeof(ammunitionTransactionPrices)/sizeof(uint16_t));
    memcpy(repairAttemptFailuresByKind,data+0x3A77,sizeof(repairAttemptFailuresByKind));
    memcpy(ownedUnitDamage,data+0x054A,sizeof(ownedUnitDamage));
    memcpy(marketUnitDamage,data+0x07EC,sizeof(marketUnitDamage));
    memcpy(&generatedMechDamage,data+0x3A41,sizeof(generatedMechDamage));
    memcpy(unitWeaponCounts,data+0x2AED,sizeof(unitWeaponCounts));
    memcpy(unitPriceTierMultipliers,data+0x3057,sizeof(unitPriceTierMultipliers));
    ImportInitialWords(data,0x305F,(uint16_t *)unitBasePrices,sizeof(unitBasePrices)/sizeof(uint16_t));
    unitTransactionPriceLow=MwReadNativeWord(data,0x305B);
    unitTransactionPriceHigh=MwReadNativeWord(data,0x305D);
    selectedOwnedUnitKind=MwReadNativeWord(data,0x2AD5);
    selectedMarketUnitKind=MwReadNativeWord(data,0x4322);
    unitRepairQuoteLow=MwReadNativeWord(data,0x31E9);
    unitRepairQuoteHigh=MwReadNativeWord(data,0x31EB);
    repairQuoteMechKind=MwReadNativeWord(data,0x31ED);
    componentRepairPriceLow=MwReadNativeWord(data,0x320C);
    componentRepairPriceHigh=MwReadNativeWord(data,0x320E);
    repairPriceCategory=MwReadNativeWord(data,0x3210);
    ImportInitialWords(data,0x3C2F,(uint16_t *)weaponRepairPrices,sizeof(weaponRepairPrices)/sizeof(uint16_t));
    memcpy(bodyRepairBasePricesByKind,data+0x3CAB,sizeof(bodyRepairBasePricesByKind));
    memcpy(rosterStatus,data+0x0451,sizeof(rosterStatus));
    ImportInitialWords(data,0x04EA,(uint16_t *)rosterPayTiers,sizeof(rosterPayTiers)/sizeof(uint16_t));
    ImportInitialWords(data,0x04F2,(uint16_t *)rosterGunneryTiers,sizeof(rosterGunneryTiers)/sizeof(uint16_t));
    ImportInitialWords(data,0x04FA,(uint16_t *)rosterPilotingTiers,sizeof(rosterPilotingTiers)/sizeof(uint16_t));
    ImportInitialWords(data,0x0502,(uint16_t *)rosterUnknownWords,sizeof(rosterUnknownWords)/sizeof(uint16_t));
    ImportInitialWords(data,0x050A,(uint16_t *)rosterMechAssignments,sizeof(rosterMechAssignments)/sizeof(uint16_t));
    ImportInitialWords(data,0x0512,(uint16_t *)rosterPortraitIds,sizeof(rosterPortraitIds)/sizeof(uint16_t));
    memcpy(pilotGunneryTiers,data+0x0A37,sizeof(pilotGunneryTiers));
    memcpy(pilotPilotingTiers,data+0x0A62,sizeof(pilotPilotingTiers));
    memcpy(pilotExperienceCounts,data+0x0A8D,sizeof(pilotExperienceCounts));
    ImportInitialWords(data,0x9121,(uint16_t *)pilotDefinitionOffsets,sizeof(pilotDefinitionOffsets)/sizeof(uint16_t));
    memcpy(pilotDefinitionStorage,data+0x9177,sizeof(pilotDefinitionStorage));
    ImportInitialWords(data,0x4CE6,(uint16_t *)travelSquareRootValues,sizeof(travelSquareRootValues)/sizeof(uint16_t));
    travelDestinationMapPositionX=MwReadNativeWord(data,0x54D0);
    travelDestinationMapPositionY=MwReadNativeWord(data,0x54D2);
    travelMapDeltaX=MwReadNativeWord(data,0x54D4);
    travelMapDeltaY=MwReadNativeWord(data,0x54D6);
    travelDestinationTimeWeight=MwReadNativeWord(data,0x54DA);
    travelDestinationPlanetOffset=MwReadNativeWord(data,0x5AD3);
    travelDistanceUnits=MwReadNativeWord(data,0x5AD9);
    travelQuoteCbillsLow=MwReadNativeWord(data,0x5AD5);
    travelQuoteCbillsHigh=MwReadNativeWord(data,0x5AD7);
    travelMapSelection=MwReadNativeWord(data,0x5ACF);
    travelDestinationPlanetIndex=MwReadNativeWord(data,0x5AD1);
    travelMarkerX=MwReadNativeWord(data,0x5ADB);
    travelMarkerY=MwReadNativeWord(data,0x5ADD);
    travelSavedMarkerX=MwReadNativeWord(data,0x5ADF);
    travelSavedMarkerY=MwReadNativeWord(data,0x5AE1);
    travelJoystickLastTick=MwReadNativeWord(data,0x5AE3);
    travelAnimationFrameStep=MwReadNativeWord(data,0x5AE6);
    memcpy(&travelDropShipSelected,data+0x5AE5,sizeof(travelDropShipSelected));
    hiredTransportReverseTerminator=MwReadNativeWord(data,0x5AE8);
    ImportInitialWords(data,0x5AEA,(uint16_t *)hiredTransportFrames,sizeof(hiredTransportFrames)/sizeof(uint16_t));
    ImportInitialWords(data,0x5B2E,(uint16_t *)dropShipTravelFrames,sizeof(dropShipTravelFrames)/sizeof(uint16_t));
    memcpy(travelDestinationName,data+0x5C4E,sizeof(travelDestinationName));
    ImportInitialWords(data,0x6694,(uint16_t *)alphabeticPlanetRecordOffsets,sizeof(alphabeticPlanetRecordOffsets)/sizeof(uint16_t));
    travelSelectedHouseByteOffset=MwReadNativeWord(data,0x54DC);
    travelHousePlanetStart=MwReadNativeWord(data,0x54DE);
    travelHousePlanetCount=MwReadNativeWord(data,0x54E2);
    travelSelectedHouseRow=MwReadNativeWord(data,0x5A0B);
    memcpy(travelPopulationSuffix,data+0x5B83,sizeof(travelPopulationSuffix));
    memcpy(travelLandscapeLabels,data+0x5CA6,sizeof(travelLandscapeLabels));
    memcpy(travelInsufficientFundsText,data+0x5B5A,sizeof(travelInsufficientFundsText));
    ImportInitialWords(data,0x0736,(uint16_t *)reserveAmmoQuantitiesByFamily,sizeof(reserveAmmoQuantitiesByFamily)/sizeof(uint16_t));
    ImportInitialWords(data,0x5C94,(uint16_t *)reserveAmmoFreightRates,sizeof(reserveAmmoFreightRates)/sizeof(uint16_t));
    memcpy(generatedKindRangeMaxima,data+0x3A37,sizeof(generatedKindRangeMaxima));
    memcpy(generatedKindRangeStarts,data+0x3A3A,sizeof(generatedKindRangeStarts));
    generatedDamageMaximumIndex=MwReadNativeWord(data,0x3A3D);
    generatedUnitKind=MwReadNativeWord(data,0x3A3F);
    ImportInitialWords(data,0x3A5E,(uint16_t *)generatedAmmoKinds,sizeof(generatedAmmoKinds)/sizeof(uint16_t));
    ImportInitialWords(data,0x3A64,(uint16_t *)generatedAmmoQuantities,sizeof(generatedAmmoQuantities)/sizeof(uint16_t));
    memcpy(&generateStartingJenner,data+0x3A6A,sizeof(generateStartingJenner));
    memcpy(unitAmmoGroupCounts,data+0x2EEE,sizeof(unitAmmoGroupCounts));
    memcpy(unitJumpJetGroupCounts,data+0x3696,sizeof(unitJumpJetGroupCounts));
    dealerUnitPurchaseLimit=MwReadNativeWord(data,0x0B56);
    unitsSoldSinceMarketRefresh=MwReadNativeWord(data,0x0B58);
    marketUnitCount=MwReadNativeWord(data,0x07D2);
    ImportInitialWords(data,0x07D4,(uint16_t *)marketUnitKinds,sizeof(marketUnitKinds)/sizeof(uint16_t));
    ImportInitialWords(data,0x078A,(uint16_t *)marketAmmoKindsByGroup,sizeof(marketAmmoKindsByGroup)/sizeof(uint16_t));
    ImportInitialWords(data,0x06EE,(uint16_t *)marketAmmoQuantitiesByGroup,sizeof(marketAmmoQuantitiesByGroup)/sizeof(uint16_t));
    availableRecruitCount=MwReadNativeWord(data,0x051E);
    ImportInitialWords(data,0x0520,(uint16_t *)availableRecruitIds,sizeof(availableRecruitIds)/sizeof(uint16_t));
    ownedUnitCount=MwReadNativeWord(data,0x0530);
    ImportInitialWords(data,0x0532,(uint16_t *)ownedUnitKinds,sizeof(ownedUnitKinds)/sizeof(uint16_t));
    ImportInitialWords(data,0x049F,(uint16_t *)houseFavorPoints,sizeof(houseFavorPoints)/sizeof(uint16_t));
    ImportInitialWords(data,0x04A9,(uint16_t *)houseGrievancePoints,sizeof(houseGrievancePoints)/sizeof(uint16_t));
    playerCombatCredit=MwReadNativeWord(data,0x04B3);
    generatingMissionOfferSlot=MwReadNativeWord(data,0x4B77);
    missionForceBudget=MwReadNativeWord(data,0x4B79);
    missionPaymentBaseThousands=MwReadNativeWord(data,0x4B7B);
    memcpy(missionForceWeightByUnitKind,data+0x4B55,sizeof(missionForceWeightByUnitKind));
    ImportInitialWords(data,0x4B41,(uint16_t *)housePaymentRollBias,sizeof(housePaymentRollBias)/sizeof(uint16_t));
    ImportInitialWords(data,0x4B4B,(uint16_t *)housePaymentAdjustmentPercent,sizeof(housePaymentAdjustmentPercent)/sizeof(uint16_t));
    ImportInitialWords(data,0x4B5F,(uint16_t *)houseSalvageBasePercent,sizeof(houseSalvageBasePercent)/sizeof(uint16_t));
    ImportInitialWords(data,0x4B69,(uint16_t *)houseAdvanceBasePercent,sizeof(houseAdvanceBasePercent)/sizeof(uint16_t));
    memcpy(houseMissionTypeLimits,data+0x0A02,sizeof(houseMissionTypeLimits));
    memcpy(&majorCampaignMissionRollThreshold,data+0x0B52,sizeof(majorCampaignMissionRollThreshold));
    ImportInitialWords(data,0x094A,(uint16_t *)missionDescriptionOffsets,sizeof(missionDescriptionOffsets)/sizeof(uint16_t));
    ImportInitialWords(data,0x0956,(uint16_t *)missionPlanetRecordIndices,sizeof(missionPlanetRecordIndices)/sizeof(uint16_t));
    ImportInitialWords(data,0x0962,(uint16_t *)missionTargetHouseIndices,sizeof(missionTargetHouseIndices)/sizeof(uint16_t));
    ImportInitialWords(data,0x096E,(uint16_t *)missionTypeCodes,sizeof(missionTypeCodes)/sizeof(uint16_t));
    ImportInitialWords(data,0x097A,(uint16_t *)missionEnemyHeavyCounts,sizeof(missionEnemyHeavyCounts)/sizeof(uint16_t));
    ImportInitialWords(data,0x0986,(uint16_t *)missionEnemyMediumCounts,sizeof(missionEnemyMediumCounts)/sizeof(uint16_t));
    ImportInitialWords(data,0x0992,(uint16_t *)missionEnemyLightCounts,sizeof(missionEnemyLightCounts)/sizeof(uint16_t));
    ImportInitialWords(data,0x099E,(uint16_t *)missionPaymentThousands,sizeof(missionPaymentThousands)/sizeof(uint16_t));
    ImportInitialWords(data,0x09AA,(uint16_t *)missionSalvagePercentages,sizeof(missionSalvagePercentages)/sizeof(uint16_t));
    ImportInitialWords(data,0x09B6,(uint16_t *)missionAdvancePercentages,sizeof(missionAdvancePercentages)/sizeof(uint16_t));
    ImportInitialWords(data,0x09C2,(uint16_t *)missionPaymentCeilingsThousands,sizeof(missionPaymentCeilingsThousands)/sizeof(uint16_t));
    ImportInitialWords(data,0x09CE,(uint16_t *)missionSalvagePercentageCeilings,sizeof(missionSalvagePercentageCeilings)/sizeof(uint16_t));
    ImportInitialWords(data,0x09DA,(uint16_t *)missionAdvancePercentageCeilings,sizeof(missionAdvancePercentageCeilings)/sizeof(uint16_t));
    ImportInitialWords(data,0x0A2B,(uint16_t *)missionScenarioCodes,sizeof(missionScenarioCodes)/sizeof(uint16_t));
    ImportInitialWords(data,0x655C,(uint16_t *)housePlanetRangeStarts,sizeof(housePlanetRangeStarts)/sizeof(uint16_t));
    ImportInitialWords(data,0x6566,(uint16_t *)housePlanetRangeCounts,sizeof(housePlanetRangeCounts)/sizeof(uint16_t));
    memcpy(missionDescriptionMaximumIndicesByCategory,data+0x8A16,sizeof(missionDescriptionMaximumIndicesByCategory));
    ImportInitialWords(data,0x8A1B,(uint16_t *)missionDescriptionListOffsetsByCategory,sizeof(missionDescriptionListOffsetsByCategory)/sizeof(uint16_t));
    ImportInitialWords(data,0x8A25,(uint16_t *)missionDescriptionListEntries,sizeof(missionDescriptionListEntries)/sizeof(uint16_t));
    memcpy(missionDescriptionStorage,data+0x8A69,sizeof(missionDescriptionStorage));
    ImportInitialWords(data,0x8E90,(uint16_t *)houseMissionPlanetTableOffsets,sizeof(houseMissionPlanetTableOffsets)/sizeof(uint16_t));
    memcpy(houseMissionPlanetPolicies,data+0x8E9A,sizeof(houseMissionPlanetPolicies));
    calendarSubmonthUnits=MwReadNativeWord(data,0x0479);
    missionOfferCount=MwReadNativeWord(data,0x0948);
    marketRefreshUnits=MwReadNativeWord(data,0x047F);
    elapsedBillingMonths=MwReadNativeWord(data,0x0481);
    campaignFundsLow=MwReadNativeWord(data,0x0491);
    campaignFundsHigh=MwReadNativeWord(data,0x0493);
    dropShipMissedUpkeepPayments=MwReadNativeWord(data,0x051A);
    unpaidPilotWageMarker=MwReadNativeWord(data,0x051C);
    ImportInitialWords(data,0x2476,(uint16_t *)pilotWageAmounts,sizeof(pilotWageAmounts)/sizeof(uint16_t));
    memcpy(&pilotDepartureFlagTail,data+0x04E9,sizeof(pilotDepartureFlagTail));
    memcpy(notificationPending,data+0x5EAE,sizeof(notificationPending));
    unitInspectionContextCode=MwReadNativeWord(data,0x25BD);
    ownedUnitListSelectedRow=MwReadNativeWord(data,0x25BF);
    marketUnitListSelectedRow=MwReadNativeWord(data,0x25C1);
    mechServiceMenuSelectedRow=MwReadNativeWord(data,0x25C3);
    selectedOwnedUnitSlot=MwReadNativeWord(data,0x2AD1);
    selectedOwnedUnitByteOffset=MwReadNativeWord(data,0x2AD3);
    ImportInitialWords(data,0x2AD7,(uint16_t *)selectedMechAmmoFamilies,sizeof(selectedMechAmmoFamilies)/sizeof(uint16_t));
    ImportInitialWords(data,0x2ADD,(uint16_t *)selectedMechAmmoQuantities,sizeof(selectedMechAmmoQuantities)/sizeof(uint16_t));
    memcpy(unitConditionSystemFieldCounts,data+0x2AE3,sizeof(unitConditionSystemFieldCounts));
    memcpy(unitConditionScoreDivisors,data+0x2B18,sizeof(unitConditionScoreDivisors));
    memcpy(mechNonfunctionalTextBytes,data+0x2B55,sizeof(mechNonfunctionalTextBytes));
    memcpy(unitDisplayTonnages,data+0x2B65,sizeof(unitDisplayTonnages));
    memcpy(unitDisplaySpeedsKph,data+0x2B6F,sizeof(unitDisplaySpeedsKph));
    memcpy(unitDisplayJumpCapacitiesMetres,data+0x2B79,sizeof(unitDisplayJumpCapacitiesMetres));
    ImportInitialWords(data,0x2B83,(uint16_t *)unitDetailPictureXOffsets,sizeof(unitDetailPictureXOffsets)/sizeof(uint16_t));
    ImportInitialWords(data,0x2B99,(uint16_t *)unitDetailPictureYOrigins,sizeof(unitDetailPictureYOrigins)/sizeof(uint16_t));
    memcpy(&repairWorkIncomplete,data+0x2BAF,sizeof(repairWorkIncomplete));
    ownedListRowUnitByteOffset=MwReadNativeWord(data,0x3022);
    saleMarketSlotByteOffset=MwReadNativeWord(data,0x3055);
    selectedMechRearmQuoteLow=MwReadNativeWord(data,0x310C);
    selectedMechRearmQuoteHigh=MwReadNativeWord(data,0x310E);
    mechServiceNextAnimationTick=MwReadNativeWord(data,0x3189);
    mechServiceAnimationByteOffset=MwReadNativeWord(data,0x318B);
    ImportInitialWords(data,0x31F2,(uint16_t *)repairTitleLeadingSpacesByKind,sizeof(repairTitleLeadingSpacesByKind)/sizeof(uint16_t));
    repairSystemRowIndex=MwReadNativeWord(data,0x3208);
    repairMenuSelectedCategory=MwReadNativeWord(data,0x320A);
    memcpy(unitHeatSinkCounts,data+0x368C,sizeof(unitHeatSinkCounts));
    memcpy(repairNoJumpJetsTextBytes,data+0x36A0,sizeof(repairNoJumpJetsTextBytes));
    memcpy(repairJumpJetCountTemplateBytes,data+0x36A5,sizeof(repairJumpJetCountTemplateBytes));
    memcpy(repairSuccessRollLimitsByMarketTier,data+0x3A6B,sizeof(repairSuccessRollLimitsByMarketTier));
    ImportInitialWords(data,0x3A6F,(uint16_t *)unusedRepairTimeByDamageTier,sizeof(unusedRepairTimeByDamageTier)/sizeof(uint16_t));
    mechDiagramUnitKind=MwReadNativeWord(data,0x3CCC);
    ImportInitialWords(data,0x3CD2,(uint16_t *)bodyDamageTierFillColours,sizeof(bodyDamageTierFillColours)/sizeof(uint16_t));
    memcpy(unassignedMechWarriorText,data+0x3024,sizeof(unassignedMechWarriorText));
    memcpy(ownedListDoneText,data+0x302F,sizeof(ownedListDoneText));
    memcpy(cannotAffordRepairText,data+0x2BB0,sizeof(cannotAffordRepairText));
    memcpy(noRepairWorkText,data+0x2BE7,sizeof(noRepairWorkText));
    memcpy(repairCompleteText,data+0x2C01,sizeof(repairCompleteText));
    memcpy(repairIncompleteText,data+0x2C21,sizeof(repairIncompleteText));
    memcpy(repairFailedText,data+0x3CB5,sizeof(repairFailedText));
    hubSelectedRecordIndex=MwReadNativeWord(data,0x1080);
    memcpy(ammoBayFullText,data+0x315B,sizeof(ammoBayFullText));
    memcpy(noMarketMechsText,data+0x31BB,sizeof(noMarketMechsText));
    memcpy(dealerPurchaseLimitText,data+0x258A,sizeof(dealerPurchaseLimitText));
    memcpy(purchaseInsufficientFundsText,data+0x4330,sizeof(purchaseInsufficientFundsText));
    memcpy(ownedMechCapacityText,data+0x4347,sizeof(ownedMechCapacityText));
    memcpy(marketListDoneText,data+0x3049,sizeof(marketListDoneText));
    selectedMarketUnitSlot=MwReadNativeWord(data,0x431E);
    selectedMarketUnitByteOffset=MwReadNativeWord(data,0x4320);
    ImportInitialWords(data,0x4324,(uint16_t *)selectedMarketAmmoFamilies,sizeof(selectedMarketAmmoFamilies)/sizeof(uint16_t));
    ImportInitialWords(data,0x432A,(uint16_t *)selectedMarketAmmoQuantities,sizeof(selectedMarketAmmoQuantities)/sizeof(uint16_t));
    marketSystemRowIndex=MwReadNativeWord(data,0x44EF);
    purchaseOwnedSlotByteOffset=MwReadNativeWord(data,0x481D);
    ImportInitialWords(data,0x318D,(uint16_t *)mechComplexAnimationFrames,sizeof(mechComplexAnimationFrames)/sizeof(uint16_t));
    campaignMusicBufferSelector=MwReadNativeWord(data,0x016F);
    graphicsOverlaySelector=MwReadNativeWord(data,0x002E);
    launcherMemoryTier=MwReadNativeWord(data,0x0B5D);
    memcpy(&launcherExitCode,data+0x0006,sizeof(launcherExitCode));
    memcpy(&tandyGraphicsSelected,data+0x03A4,sizeof(tandyGraphicsSelected));
    memcpy(&savedVideoMode,data+0x0B5F,sizeof(savedVideoMode));
    memcpy(&keyboardInterruptState,data+0x02AB,sizeof(keyboardInterruptState));
    memcpy(startupErrorText,data+0x018B,sizeof(startupErrorText));
    musicFileHandle=MwReadNativeWord(data,0x0E39);
    copyProtectionSelection=MwReadNativeWord(data,0x1F1A);
    ImportInitialWords(data,0x1FA1,(uint16_t *)copyProtectionPictureIndices,sizeof(copyProtectionPictureIndices)/sizeof(uint16_t));
    memcpy(copyProtectionPrompt,data+0x1F1C,sizeof(copyProtectionPrompt));
    memcpy(copyProtectionAcceptedText,data+0x2153,sizeof(copyProtectionAcceptedText));
    memcpy(campaignTextInputBuffer,data+0x03A5,sizeof(campaignTextInputBuffer));
    textInputCursorX=MwReadNativeWord(data,0x03CE);
    textInputCursorY=MwReadNativeWord(data,0x03D0);
    textInputMaximumIndex=MwReadNativeWord(data,0x03D2);
    memcpy(&textInputForeground,data+0x03D4,sizeof(textInputForeground));
    memcpy(&textInputBackground,data+0x03D5,sizeof(textInputBackground));
    memcpy(&textInputLastPhase,data+0x03D8,sizeof(textInputLastPhase));
    textInputLength=MwReadNativeWord(data,0x03D6);
    ImportInitialWords(data,0x0B46,(uint16_t *)campaignEventPlanetSelectors,sizeof(campaignEventPlanetSelectors)/sizeof(uint16_t));
    memcpy(eventPlanetChoicePool0,data+0x8FDA,sizeof(eventPlanetChoicePool0));
    memcpy(eventPlanetChoicePool1,data+0x8FE2,sizeof(eventPlanetChoicePool1));
    memcpy(eventPlanetChoicePool2,data+0x8FE5,sizeof(eventPlanetChoicePool2));
    memcpy(eventPlanetChoicePool3,data+0x8FE9,sizeof(eventPlanetChoicePool3));
    memcpy(eventPlanetChoicePool4,data+0x8FEC,sizeof(eventPlanetChoicePool4));
    memcpy(eventPlanetChoicePool5,data+0x8FF0,sizeof(eventPlanetChoicePool5));
    eventDirectoryIndex=MwReadNativeWord(data,0x0AB8);
    campaignEventClass=MwReadNativeWord(data,0x5E62);
    currentEventId=MwReadNativeWord(data,0x5E64);
    dispatchedEventCount=MwReadNativeWord(data,0x5E68);
    memcpy(eventSeenFlags,data+0x0ABA,sizeof(eventSeenFlags));
    memcpy(&eventConditionResult,data+0x5E70,sizeof(eventConditionResult));
    memcpy(&eventCombineWithAnd,data+0x5E71,sizeof(eventCombineWithAnd));
    eventScheduledMonthScratch=MwReadNativeWord(data,0x5E72);
    eventExpirySubmonthScratch=MwReadNativeWord(data,0x5E74);
    eventScheduledYearScratch=MwReadNativeWord(data,0x5E76);
    memcpy(&planetArrivalEventsPending,data+0x1082,sizeof(planetArrivalEventsPending));
    memcpy(&darkWingBaseVictoryRecorded,data+0x0B5C,sizeof(darkWingBaseVictoryRecorded));
    memcpy(&campaignRestartPending,data+0x5EB2,sizeof(campaignRestartPending));
    memcpy(&eventChoiceCount,data+0x5CEC,sizeof(eventChoiceCount));
    memcpy(&eventTextRowCount,data+0x5CED,sizeof(eventTextRowCount));
    eventChoiceSavedFontIndex=MwReadNativeWord(data,0x5E66);
    memcpy(&unidentifiedField18,data+0x0448,sizeof(unidentifiedField18));
    memcpy(&missionCategoryOrMemoryTier,data+0x0449,sizeof(missionCategoryOrMemoryTier));
    memcpy(&deploymentLandscapeIndex,data+0x044A,sizeof(deploymentLandscapeIndex));
    memcpy(&combatEnemyHeavyCount,data+0x044B,sizeof(combatEnemyHeavyCount));
    memcpy(&combatEnemyMediumCount,data+0x044C,sizeof(combatEnemyMediumCount));
    memcpy(&combatEnemyLightCount,data+0x044D,sizeof(combatEnemyLightCount));
    memcpy(&combatPlanetSelector,data+0x044E,sizeof(combatPlanetSelector));
    memcpy(&combatScenarioCode,data+0x044F,sizeof(combatScenarioCode));
    memcpy(&deploymentTerrainDetailLevel,data+0x0450,sizeof(deploymentTerrainDetailLevel));
    memcpy(salvageEntries,data+0x0455,sizeof(salvageEntries));
    returnedDisabledEnemyCount=MwReadNativeWord(data,0x0465);
    memcpy(&combatTransitionResult,data+0x0467,sizeof(combatTransitionResult));
    memcpy(&acceptedContractActive,data+0x0468,sizeof(acceptedContractActive));
    memcpy(unidentifiedField19,data+0x048D,sizeof(unidentifiedField19));
    commanderAgeYears=MwReadNativeWord(data,0x048F);
    ImportInitialWords(data,0x0495,(uint16_t *)houseNetStandings,sizeof(houseNetStandings)/sizeof(uint16_t));
    requestedContractPaymentThousands=MwReadNativeWord(data,0x09E8);
    requestedContractSalvagePercent=MwReadNativeWord(data,0x09EA);
    requestedContractAdvancePercent=MwReadNativeWord(data,0x09EC);
    requestContractPaymentEnabled=MwReadNativeWord(data,0x09EE);
    requestContractSalvageEnabled=MwReadNativeWord(data,0x09F0);
    requestContractAdvanceEnabled=MwReadNativeWord(data,0x09F2);
    memcpy(unidentifiedField20,data+0x09F4,sizeof(unidentifiedField20));
    acceptedContractPaymentThousands=MwReadNativeWord(data,0x09FA);
    acceptedContractSalvagePercent=MwReadNativeWord(data,0x09FC);
    acceptedContractAdvancePercent=MwReadNativeWord(data,0x09FE);
    remainingContractPaymentThousands=MwReadNativeWord(data,0x0A00);
    memcpy(&audioOptionLabelIndex,data+0x0A2A,sizeof(audioOptionLabelIndex));
    memcpy(&campaignTerrainDetailLevel,data+0x0B55,sizeof(campaignTerrainDetailLevel));
    acceptedContractTargetHouseIndex=MwReadNativeWord(data,0x0B53);
    playerSkillProgress=MwReadNativeWord(data,0x0B5A);
    memcpy(defaultHouseMissionTypeLimits,data+0x5CBA,sizeof(defaultHouseMissionTypeLimits));
    memcpy(campaignDefaultCommanderNameBytes,data+0x5CE2,sizeof(campaignDefaultCommanderNameBytes));
    companyMenuSelectedRow=MwReadNativeWord(data,0x21CC);
    barMenuSelectedRow=MwReadNativeWord(data,0x8FF4);
    optionsMenuSelection=MwReadNativeWord(data,0x9D50);
    pilotRosterCount=MwReadNativeWord(data,0x04B5);
    for (uint16_t question=0;question<COPY_PROTECTION_QUESTION_COUNT;++question)
    {
        if (!ImportInitialString(data,MwReadNativeWord(data,(uint16_t)(0x0F8F+question*2)),copyProtectionCallsigns[question],COPY_PROTECTION_TEXT_CAPACITY)
            || !ImportInitialString(data,MwReadNativeWord(data,(uint16_t)(0x163A+question*2)),copyProtectionAnswers[question],COPY_PROTECTION_TEXT_CAPACITY)) return false;
    }
    uint16_t narrationOffset=0x129C;
    for (uint16_t line=0;line<10;++line)
    {
        if (!ImportInitialString(data,narrationOffset,introNarrationLines[line],sizeof(introNarrationLines[line]))) return false;
        narrationOffset+=(uint16_t)(strlen(introNarrationLines[line])+1);
    }
    introFirstLeftText.x=MwReadNativeWord(data,0x1C33);introFirstLeftText.y=MwReadNativeWord(data,0x1C35);
    if (!ImportInitialString(data,0x1C37,introFirstLeftText.text,sizeof(introFirstLeftText.text))) return false;
    introFirstRightText.x=MwReadNativeWord(data,0x1C57);introFirstRightText.y=MwReadNativeWord(data,0x1C59);
    if (!ImportInitialString(data,0x1C5B,introFirstRightText.text,sizeof(introFirstRightText.text))) return false;
    introSecondLeftText.x=MwReadNativeWord(data,0x1C80);introSecondLeftText.y=MwReadNativeWord(data,0x1C82);
    if (!ImportInitialString(data,0x1C84,introSecondLeftText.text,sizeof(introSecondLeftText.text))) return false;
    introSecondRightText.x=MwReadNativeWord(data,0x1CA9);introSecondRightText.y=MwReadNativeWord(data,0x1CAB);
    if (!ImportInitialString(data,0x1CAD,introSecondRightText.text,sizeof(introSecondRightText.text))) return false;
    introThirdLeftText.x=MwReadNativeWord(data,0x1CCF);introThirdLeftText.y=MwReadNativeWord(data,0x1CD1);
    if (!ImportInitialString(data,0x1CD3,introThirdLeftText.text,sizeof(introThirdLeftText.text))) return false;
    introThirdRightText.x=MwReadNativeWord(data,0x1D1F);introThirdRightText.y=MwReadNativeWord(data,0x1D21);
    if (!ImportInitialString(data,0x1D23,introThirdRightText.text,sizeof(introThirdRightText.text))) return false;
    introFourthLeftText.x=MwReadNativeWord(data,0x1D53);introFourthLeftText.y=MwReadNativeWord(data,0x1D55);
    if (!ImportInitialString(data,0x1D57,introFourthLeftText.text,sizeof(introFourthLeftText.text))) return false;
    introFourthRightText.x=MwReadNativeWord(data,0x1D6F);introFourthRightText.y=MwReadNativeWord(data,0x1D71);
    if (!ImportInitialString(data,0x1D73,introFourthRightText.text,sizeof(introFourthRightText.text))) return false;
    introFifthLeftText.x=MwReadNativeWord(data,0x1D8E);introFifthLeftText.y=MwReadNativeWord(data,0x1D90);
    if (!ImportInitialString(data,0x1D92,introFifthLeftText.text,sizeof(introFifthLeftText.text))) return false;
    introFifthRightText.x=MwReadNativeWord(data,0x1DD8);introFifthRightText.y=MwReadNativeWord(data,0x1DDA);
    if (!ImportInitialString(data,0x1DDC,introFifthRightText.text,sizeof(introFifthRightText.text))) return false;
    introSixthLeftText.x=MwReadNativeWord(data,0x1DF7);introSixthLeftText.y=MwReadNativeWord(data,0x1DF9);
    if (!ImportInitialString(data,0x1DFB,introSixthLeftText.text,sizeof(introSixthLeftText.text))) return false;
    introSixthRightText.x=MwReadNativeWord(data,0x1E18);introSixthRightText.y=MwReadNativeWord(data,0x1E1A);
    if (!ImportInitialString(data,0x1E1C,introSixthRightText.text,sizeof(introSixthRightText.text))) return false;
    introSeventhLeftText.x=MwReadNativeWord(data,0x1E40);introSeventhLeftText.y=MwReadNativeWord(data,0x1E42);
    if (!ImportInitialString(data,0x1E44,introSeventhLeftText.text,sizeof(introSeventhLeftText.text))) return false;
    introSeventhRightText.x=MwReadNativeWord(data,0x1EB8);introSeventhRightText.y=MwReadNativeWord(data,0x1EBA);
    if (!ImportInitialString(data,0x1EBC,introSeventhRightText.text,sizeof(introSeventhRightText.text))) return false;
    for (uint16_t error=0;error<7;++error)
    {
        uint16_t text=MwReadNativeWord(data,(uint16_t)(0x017D+error*2));
        if (text<0x018B || text>=0x02AB || !memchr(data+text,'$',0x02AB-text)) return false;
        startupErrorMessages[error]=startupErrorText+text-0x018B;
    }
    ImportInitialWords(data,0x04B7,rosterPilotIds,CAMPAIGN_ROSTER_SLOTS);
    memcpy(pilotIdentityStorage.roster.unavailablePilots1To42,data+0x04BF,sizeof(pilotIdentityStorage.roster.unavailablePilots1To42));
    for (uint16_t row=0;row<CAMPAIGN_SYSTEM_COUNT;++row)
        ImportInitialWords(data,(uint16_t)(0x3B53+row*0x14),systemRepairBasePrices[row].basePriceByMechKind,CAMPAIGN_MECH_KIND_COUNT);
    for (uint16_t entry=0;entry<AMMO_PROFILE_ENTRY_COUNT;++entry)
    {
        unitAmmoProfileEntries[entry].ammoFamily=MwReadNativeWord(data,(uint16_t)(0x2F3C+entry*4));
        unitAmmoProfileEntries[entry].quantityMaximum=MwReadNativeWord(data,(uint16_t)(0x2F3E+entry*4));
    }
    for (uint16_t kind=0;kind<CAMPAIGN_MECH_KIND_COUNT;++kind)
    {
        uint16_t profile=MwReadNativeWord(data,(uint16_t)(0x2F28+kind*2));
        uint16_t prices=MwReadNativeWord(data,(uint16_t)(0x3C1B+kind*2));
        uint16_t layout=MwReadNativeWord(data,(uint16_t)(0x3CDA+kind*2));
        if (profile<0x2F3C || profile>=0x2F78 || (profile-0x2F3C)%4
            || prices<0x3C2F || prices>=0x3CAB || (prices-0x3C2F)%2
            || (layout && (layout<0x3CEE || layout>=0x3F6E || (layout-0x3CEE)%0x50))) return false;
        unitAmmoProfiles[kind]=&unitAmmoProfileEntries[(profile-0x2F3C)/4];
        weaponRepairPricesByKind[kind]=&weaponRepairPrices[(prices-0x3C2F)/2];
        mechBodyDiagramLayoutsByKind[kind]=layout?&mechBodyDiagramLayouts[(layout-0x3CEE)/0x50]:NULL;
        uint16_t labelOffset=MwReadNativeWord(data,(uint16_t)(0x36AE+kind*2)),label=0;
        while (data[labelOffset]!=0xFF && label<CAMPAIGN_WEAPON_SLOTS+1)
        {
            if (!ImportInitialString(data,labelOffset,repairWeaponLabelStorage[kind][label],0x0F)) return false;
            repairWeaponLabels[kind][label]=repairWeaponLabelStorage[kind][label];
            labelOffset+=(uint16_t)(strlen(repairWeaponLabels[kind][label])+1);++label;
        }
        repairWeaponLabels[kind][label]=NULL;
    }
    for (uint16_t i=0;i<0x0B;++i)
        if (!ImportInitialString(data,MwReadNativeWord(data,(uint16_t)(0x2DB5+i*2)),shopUnitNames[i],sizeof(shopUnitNames[i]))) return false;
    for (uint16_t i=0;i<0x0B;++i)
        if (!ImportInitialString(data,MwReadNativeWord(data,(uint16_t)(0x2E70+i*2)),unitDisplayNames[i],sizeof(unitDisplayNames[i]))) return false;
    if (!ImportReserveAmmoUiData(data)) return false;
    for (uint16_t i=0;i<0x06;++i)
        if (!ImportInitialString(data,MwReadNativeWord(data,(uint16_t)(0x2F78+i*2)),ammoFamilyDisplayNames[i],sizeof(ammoFamilyDisplayNames[i]))) return false;
    for (uint16_t i=0;i<0x04;++i)
        if (!ImportInitialString(data,MwReadNativeWord(data,(uint16_t)(0x2B23+i*2)),campaignDamageTierLabels[i],sizeof(campaignDamageTierLabels[i]))) return false;
    memcpy(calendarMonthText,data+0x4AAC,sizeof(calendarMonthText));
    for (uint16_t month=0;month<CAMPAIGN_MONTH_COUNT;++month)
    {
        uint16_t text=MwReadNativeWord(data,(uint16_t)(0x4A94+month*2));
        if (text<0x4AAC || text>=0x4B1A || !memchr(data+text,0,0x4B1A-text)) return false;
        calendarMonthNames[month]=calendarMonthText+text-0x4AAC;
    }
    ImportInitialText(data,0x55EC,&positionedText.display,positionedText.text,18);
    if (!ImportInitialString(data,0x46A2,marketWeaponHeading.text,sizeof(marketWeaponHeading.text))) return false;
    ImportInitialText(data,0x469D,&marketWeaponHeading.display,marketWeaponHeading.text,0);
    if (!ImportInitialString(data,0x312B,rearmConfirmationLabels[0].text,sizeof(rearmConfirmationLabels[0].text))) return false;
    ImportInitialText(data,0x3126,&rearmConfirmationLabels[0].display,rearmConfirmationLabels[0].text,0);
    if (!ImportInitialString(data,0x3148,rearmConfirmationLabels[1].text,sizeof(rearmConfirmationLabels[1].text))) return false;
    ImportInitialText(data,0x3143,&rearmConfirmationLabels[1].display,rearmConfirmationLabels[1].text,0);
    if (!ImportInitialString(data,0x3154,rearmConfirmationLabels[2].text,sizeof(rearmConfirmationLabels[2].text))) return false;
    ImportInitialText(data,0x314F,&rearmConfirmationLabels[2].display,rearmConfirmationLabels[2].text,0);
    if (!ImportInitialString(data,0x3090,saleConfirmationLabels[0].text,sizeof(saleConfirmationLabels[0].text))) return false;
    ImportInitialText(data,0x308B,&saleConfirmationLabels[0].display,saleConfirmationLabels[0].text,0);
    if (!ImportInitialString(data,0x30DD,saleConfirmationLabels[1].text,sizeof(saleConfirmationLabels[1].text))) return false;
    ImportInitialText(data,0x30D8,&saleConfirmationLabels[1].display,saleConfirmationLabels[1].text,0);
    if (!ImportInitialString(data,0x30E9,saleConfirmationLabels[2].text,sizeof(saleConfirmationLabels[2].text))) return false;
    ImportInitialText(data,0x30E4,&saleConfirmationLabels[2].display,saleConfirmationLabels[2].text,0);
    if (!ImportInitialString(data,0x2922,marketListHeading.text,sizeof(marketListHeading.text))) return false;
    ImportInitialText(data,0x291D,&marketListHeading.display,marketListHeading.text,0);
    if (!ImportInitialString(data,0x439B,marketMechStatusLabels[0].text,sizeof(marketMechStatusLabels[0].text))) return false;
    ImportInitialText(data,0x4396,&marketMechStatusLabels[0].display,marketMechStatusLabels[0].text,0);
    if (!ImportInitialString(data,0x441D,marketMechStatusLabels[1].text,sizeof(marketMechStatusLabels[1].text))) return false;
    ImportInitialText(data,0x4418,&marketMechStatusLabels[1].display,marketMechStatusLabels[1].text,0);
    if (!ImportInitialString(data,0x4426,marketMechStatusLabels[2].text,sizeof(marketMechStatusLabels[2].text))) return false;
    ImportInitialText(data,0x4421,&marketMechStatusLabels[2].display,marketMechStatusLabels[2].text,0);
    if (!ImportInitialString(data,0x4439,marketMechStatusLabels[3].text,sizeof(marketMechStatusLabels[3].text))) return false;
    ImportInitialText(data,0x4434,&marketMechStatusLabels[3].display,marketMechStatusLabels[3].text,0);
    if (!ImportInitialString(data,0x4459,marketDropShipStatusLabels[0].text,sizeof(marketDropShipStatusLabels[0].text))) return false;
    ImportInitialText(data,0x4454,&marketDropShipStatusLabels[0].display,marketDropShipStatusLabels[0].text,0);
    if (!ImportInitialString(data,0x449B,marketDropShipStatusLabels[1].text,sizeof(marketDropShipStatusLabels[1].text))) return false;
    ImportInitialText(data,0x4496,&marketDropShipStatusLabels[1].display,marketDropShipStatusLabels[1].text,0);
    if (!ImportInitialString(data,0x44A4,marketDropShipStatusLabels[2].text,sizeof(marketDropShipStatusLabels[2].text))) return false;
    ImportInitialText(data,0x449F,&marketDropShipStatusLabels[2].display,marketDropShipStatusLabels[2].text,0);
    if (!ImportInitialString(data,0x25E4,mechComplexLabels[0].text,sizeof(mechComplexLabels[0].text))) return false;
    ImportInitialText(data,0x25DF,&mechComplexLabels[0].display,mechComplexLabels[0].text,0);
    if (!ImportInitialString(data,0x25F6,mechComplexLabels[1].text,sizeof(mechComplexLabels[1].text))) return false;
    ImportInitialText(data,0x25F1,&mechComplexLabels[1].display,mechComplexLabels[1].text,0);
    if (!ImportInitialString(data,0x2608,mechComplexLabels[2].text,sizeof(mechComplexLabels[2].text))) return false;
    ImportInitialText(data,0x2603,&mechComplexLabels[2].display,mechComplexLabels[2].text,0);
    if (!ImportInitialString(data,0x2619,mechComplexLabels[3].text,sizeof(mechComplexLabels[3].text))) return false;
    ImportInitialText(data,0x2614,&mechComplexLabels[3].display,mechComplexLabels[3].text,0);
    if (!ImportInitialString(data,0x2629,mechComplexLabels[4].text,sizeof(mechComplexLabels[4].text))) return false;
    ImportInitialText(data,0x2624,&mechComplexLabels[4].display,mechComplexLabels[4].text,0);
    if (!ImportInitialString(data,0x4536,marketComponentTitle.text,sizeof(marketComponentTitle.text))) return false;
    ImportInitialText(data,0x4531,&marketComponentTitle.display,marketComponentTitle.text,0);
    ImportInitialText(data,0x4685,&marketBodyCondition.display,marketBodyCondition.caption.text,19);
    if (!ImportInitialString(data,0x47F5,marketComponentDone.text,sizeof(marketComponentDone.text))) return false;
    ImportInitialText(data,0x47F0,&marketComponentDone.display,marketComponentDone.text,0);
    ImportInitialText(data,0x2931,&marketUnitListRows[0].display,marketUnitListRows[0].text,13);
    ImportInitialText(data,0x2943,&marketUnitListRows[1].display,marketUnitListRows[1].text,13);
    ImportInitialText(data,0x2955,&marketUnitListRows[2].display,marketUnitListRows[2].text,13);
    ImportInitialText(data,0x2967,&marketUnitListRows[3].display,marketUnitListRows[3].text,13);
    ImportInitialText(data,0x2979,&marketUnitListRows[4].display,marketUnitListRows[4].text,13);
    ImportInitialText(data,0x298B,&marketUnitListRows[5].display,marketUnitListRows[5].text,13);
    ImportInitialText(data,0x299D,&marketUnitListRows[6].display,marketUnitListRows[6].text,13);
    ImportInitialText(data,0x29AF,&marketUnitListRows[7].display,marketUnitListRows[7].text,13);
    ImportInitialText(data,0x29C1,&marketUnitListRows[8].display,marketUnitListRows[8].text,13);
    ImportInitialText(data,0x29D3,&marketUnitListRows[9].display,marketUnitListRows[9].text,13);
    ImportInitialText(data,0x29E5,&marketUnitListRows[10].display,marketUnitListRows[10].text,13);
    ImportInitialText(data,0x29F7,&marketUnitListRows[11].display,marketUnitListRows[11].text,13);
    ImportInitialText(data,0x2A09,&marketUnitListRows[12].display,marketUnitListRows[12].text,13);
    ImportInitialText(data,0x454F,&marketSystemConditionRows[0].display,marketSystemConditionRows[0].caption.text,26);
    ImportInitialText(data,0x46BA,&marketWeaponConditionRows[0].display,marketWeaponConditionRows[0].caption.text,26);
    ImportInitialText(data,0x456E,&marketSystemConditionRows[1].display,marketSystemConditionRows[1].caption.text,26);
    ImportInitialText(data,0x46D9,&marketWeaponConditionRows[1].display,marketWeaponConditionRows[1].caption.text,26);
    ImportInitialText(data,0x458D,&marketSystemConditionRows[2].display,marketSystemConditionRows[2].caption.text,26);
    ImportInitialText(data,0x46F8,&marketWeaponConditionRows[2].display,marketWeaponConditionRows[2].caption.text,26);
    ImportInitialText(data,0x45AC,&marketSystemConditionRows[3].display,marketSystemConditionRows[3].caption.text,26);
    ImportInitialText(data,0x4717,&marketWeaponConditionRows[3].display,marketWeaponConditionRows[3].caption.text,26);
    ImportInitialText(data,0x45CB,&marketSystemConditionRows[4].display,marketSystemConditionRows[4].caption.text,26);
    ImportInitialText(data,0x4736,&marketWeaponConditionRows[4].display,marketWeaponConditionRows[4].caption.text,26);
    ImportInitialText(data,0x45EA,&marketSystemConditionRows[5].display,marketSystemConditionRows[5].caption.text,26);
    ImportInitialText(data,0x4755,&marketWeaponConditionRows[5].display,marketWeaponConditionRows[5].caption.text,26);
    ImportInitialText(data,0x4609,&marketSystemConditionRows[6].display,marketSystemConditionRows[6].caption.text,26);
    ImportInitialText(data,0x4774,&marketWeaponConditionRows[6].display,marketWeaponConditionRows[6].caption.text,26);
    ImportInitialText(data,0x4628,&marketSystemConditionRows[7].display,marketSystemConditionRows[7].caption.text,26);
    ImportInitialText(data,0x4793,&marketWeaponConditionRows[7].display,marketWeaponConditionRows[7].caption.text,26);
    ImportInitialText(data,0x4647,&marketSystemConditionRows[8].display,marketSystemConditionRows[8].caption.text,26);
    ImportInitialText(data,0x47B2,&marketWeaponConditionRows[8].display,marketWeaponConditionRows[8].caption.text,26);
    ImportInitialText(data,0x4666,&marketSystemConditionRows[9].display,marketSystemConditionRows[9].caption.text,26);
    ImportInitialText(data,0x47D1,&marketWeaponConditionRows[9].display,marketWeaponConditionRows[9].caption.text,26);
    ImportInitialText(data,0x2696,&ownedUnitListHeading.display,ownedUnitListHeading.text,26);
    ImportInitialText(data,0x3254,&repairTitle.display,repairTitle.text,25);
    ImportInitialText(data,0x33A8,&repairBodyCondition.display,repairBodyCondition.caption.text,19);
    ImportInitialText(data,0x33C0,&repairFooter.display,repairFooter.text,62);
    ImportInitialText(data,0x26B5,&ownedUnitListRows[0].display,ownedUnitListRows[0].caption.text,25);
    ImportInitialText(data,0x26D3,&ownedUnitListRows[1].display,ownedUnitListRows[1].caption.text,25);
    ImportInitialText(data,0x26F1,&ownedUnitListRows[2].display,ownedUnitListRows[2].caption.text,25);
    ImportInitialText(data,0x270F,&ownedUnitListRows[3].display,ownedUnitListRows[3].caption.text,25);
    ImportInitialText(data,0x272D,&ownedUnitListRows[4].display,ownedUnitListRows[4].caption.text,25);
    ImportInitialText(data,0x274B,&ownedUnitListRows[5].display,ownedUnitListRows[5].caption.text,25);
    ImportInitialText(data,0x2769,&ownedUnitListRows[6].display,ownedUnitListRows[6].caption.text,25);
    ImportInitialText(data,0x2787,&ownedUnitListRows[7].display,ownedUnitListRows[7].caption.text,25);
    ImportInitialText(data,0x27A5,&ownedUnitListRows[8].display,ownedUnitListRows[8].caption.text,25);
    ImportInitialText(data,0x27C3,&ownedUnitListRows[9].display,ownedUnitListRows[9].caption.text,25);
    ImportInitialText(data,0x27E1,&ownedUnitListRows[10].display,ownedUnitListRows[10].caption.text,25);
    ImportInitialText(data,0x27FF,&ownedUnitListRows[11].display,ownedUnitListRows[11].caption.text,25);
    ImportInitialText(data,0x281D,&ownedUnitListRows[12].display,ownedUnitListRows[12].caption.text,25);
    ImportInitialText(data,0x3272,&repairSystemConditionRows[0].display,repairSystemConditionRows[0].caption.text,26);
    ImportInitialText(data,0x3291,&repairSystemConditionRows[1].display,repairSystemConditionRows[1].caption.text,26);
    ImportInitialText(data,0x32B0,&repairSystemConditionRows[2].display,repairSystemConditionRows[2].caption.text,26);
    ImportInitialText(data,0x32CF,&repairSystemConditionRows[3].display,repairSystemConditionRows[3].caption.text,26);
    ImportInitialText(data,0x32EE,&repairSystemConditionRows[4].display,repairSystemConditionRows[4].caption.text,26);
    ImportInitialText(data,0x330D,&repairSystemConditionRows[5].display,repairSystemConditionRows[5].caption.text,26);
    ImportInitialText(data,0x332C,&repairSystemConditionRows[6].display,repairSystemConditionRows[6].caption.text,26);
    ImportInitialText(data,0x334B,&repairSystemConditionRows[7].display,repairSystemConditionRows[7].caption.text,26);
    ImportInitialText(data,0x336A,&repairSystemConditionRows[8].display,repairSystemConditionRows[8].caption.text,26);
    ImportInitialText(data,0x3389,&repairSystemConditionRows[9].display,repairSystemConditionRows[9].caption.text,26);
    ImportInitialText(data,0x3403,&repairWeaponConditionRows[0].display,repairWeaponConditionRows[0].caption.text,26);
    ImportInitialText(data,0x3422,&repairWeaponConditionRows[1].display,repairWeaponConditionRows[1].caption.text,26);
    ImportInitialText(data,0x3441,&repairWeaponConditionRows[2].display,repairWeaponConditionRows[2].caption.text,26);
    ImportInitialText(data,0x3460,&repairWeaponConditionRows[3].display,repairWeaponConditionRows[3].caption.text,26);
    ImportInitialText(data,0x347F,&repairWeaponConditionRows[4].display,repairWeaponConditionRows[4].caption.text,26);
    ImportInitialText(data,0x349E,&repairWeaponConditionRows[5].display,repairWeaponConditionRows[5].caption.text,26);
    ImportInitialText(data,0x34BD,&repairWeaponConditionRows[6].display,repairWeaponConditionRows[6].caption.text,26);
    ImportInitialText(data,0x34DC,&repairWeaponConditionRows[7].display,repairWeaponConditionRows[7].caption.text,26);
    ImportInitialText(data,0x34FB,&repairWeaponConditionRows[8].display,repairWeaponConditionRows[8].caption.text,26);
    ImportInitialText(data,0x351A,&repairWeaponConditionRows[9].display,repairWeaponConditionRows[9].caption.text,26);
    ImportInitialText(data,0x3539,&repairWeaponConditionRows[10].display,repairWeaponConditionRows[10].caption.text,26);
    ImportInitialText(data,0x2C5B,&mechStatusLabels[0].display,mechStatusLabels[0].text,106);
    ImportInitialText(data,0x2CCA,&mechStatusLabels[1].display,mechStatusLabels[1].text,7);
    ImportInitialText(data,0x2CD6,&mechStatusLabels[2].display,mechStatusLabels[2].text,11);
    ImportInitialText(data,0x2CE6,&mechStatusLabels[3].display,mechStatusLabels[3].text,7);
    ImportInitialText(data,0x2CF2,&mechStatusLabels[4].display,mechStatusLabels[4].text,5);
    ImportInitialText(data,0x2CFC,&mechStatusLabels[5].display,mechStatusLabels[5].text,5);
    ImportInitialText(data,0x2D1C,&dropShipStatusLabels[0].display,dropShipStatusLabels[0].text,30);
    ImportInitialText(data,0x2D3F,&dropShipStatusLabels[1].display,dropShipStatusLabels[1].text,5);
    ImportInitialText(data,0x2D49,&dropShipStatusLabels[2].display,dropShipStatusLabels[2].text,5);
    rearmConfirmationPanel.x=MwReadNativeWord(data,0x3110);
    rearmConfirmationPanel.y=MwReadNativeWord(data,0x3112);
    rearmConfirmationPanel.width=MwReadNativeWord(data,0x3114);
    rearmConfirmationPanel.bottomInset=MwReadNativeWord(data,0x3116);
    rearmConfirmationPanel.lineCount=MwReadNativeWord(data,0x3118);
    rearmConfirmationPanel.borderEnabled=MwReadNativeWord(data,0x311A);
    rearmConfirmationPanel.lineSpacing=MwReadNativeWord(data,0x311C);
    rearmConfirmationPanel.background=MwReadNativeWord(data,0x311E);
    if (MwReadNativeWord(data,0x3120)!=0x3126) return false;
    rearmConfirmationPanel.lines[0]=&rearmConfirmationLabels[0].display;
    if (MwReadNativeWord(data,0x3122)!=0x3143) return false;
    rearmConfirmationPanel.lines[1]=&rearmConfirmationLabels[1].display;
    if (MwReadNativeWord(data,0x3124)!=0x314F) return false;
    rearmConfirmationPanel.lines[2]=&rearmConfirmationLabels[2].display;
    saleConfirmationPanel.x=MwReadNativeWord(data,0x3075);
    saleConfirmationPanel.y=MwReadNativeWord(data,0x3077);
    saleConfirmationPanel.width=MwReadNativeWord(data,0x3079);
    saleConfirmationPanel.bottomInset=MwReadNativeWord(data,0x307B);
    saleConfirmationPanel.lineCount=MwReadNativeWord(data,0x307D);
    saleConfirmationPanel.borderEnabled=MwReadNativeWord(data,0x307F);
    saleConfirmationPanel.lineSpacing=MwReadNativeWord(data,0x3081);
    saleConfirmationPanel.background=MwReadNativeWord(data,0x3083);
    if (MwReadNativeWord(data,0x3085)!=0x308B) return false;
    saleConfirmationPanel.lines[0]=&saleConfirmationLabels[0].display;
    if (MwReadNativeWord(data,0x3087)!=0x30D8) return false;
    saleConfirmationPanel.lines[1]=&saleConfirmationLabels[1].display;
    if (MwReadNativeWord(data,0x3089)!=0x30E4) return false;
    saleConfirmationPanel.lines[2]=&saleConfirmationLabels[2].display;
    marketUnitListPanel.x=MwReadNativeWord(data,0x28F1);
    marketUnitListPanel.y=MwReadNativeWord(data,0x28F3);
    marketUnitListPanel.width=MwReadNativeWord(data,0x28F5);
    marketUnitListPanel.bottomInset=MwReadNativeWord(data,0x28F7);
    marketUnitListPanel.lineCount=MwReadNativeWord(data,0x28F9);
    marketUnitListPanel.borderEnabled=MwReadNativeWord(data,0x28FB);
    marketUnitListPanel.lineSpacing=MwReadNativeWord(data,0x28FD);
    marketUnitListPanel.background=MwReadNativeWord(data,0x28FF);
    if (MwReadNativeWord(data,0x2901)!=0x291D) return false;
    marketUnitListPanel.lines[0]=&marketListHeading.display;
    if (MwReadNativeWord(data,0x2903)!=0x2931) return false;
    marketUnitListPanel.lines[1]=&marketUnitListRows[0].display;
    if (MwReadNativeWord(data,0x2905)!=0x2943) return false;
    marketUnitListPanel.lines[2]=&marketUnitListRows[1].display;
    if (MwReadNativeWord(data,0x2907)!=0x2955) return false;
    marketUnitListPanel.lines[3]=&marketUnitListRows[2].display;
    if (MwReadNativeWord(data,0x2909)!=0x2967) return false;
    marketUnitListPanel.lines[4]=&marketUnitListRows[3].display;
    if (MwReadNativeWord(data,0x290B)!=0x2979) return false;
    marketUnitListPanel.lines[5]=&marketUnitListRows[4].display;
    if (MwReadNativeWord(data,0x290D)!=0x298B) return false;
    marketUnitListPanel.lines[6]=&marketUnitListRows[5].display;
    if (MwReadNativeWord(data,0x290F)!=0x299D) return false;
    marketUnitListPanel.lines[7]=&marketUnitListRows[6].display;
    if (MwReadNativeWord(data,0x2911)!=0x29AF) return false;
    marketUnitListPanel.lines[8]=&marketUnitListRows[7].display;
    if (MwReadNativeWord(data,0x2913)!=0x29C1) return false;
    marketUnitListPanel.lines[9]=&marketUnitListRows[8].display;
    if (MwReadNativeWord(data,0x2915)!=0x29D3) return false;
    marketUnitListPanel.lines[10]=&marketUnitListRows[9].display;
    if (MwReadNativeWord(data,0x2917)!=0x29E5) return false;
    marketUnitListPanel.lines[11]=&marketUnitListRows[10].display;
    if (MwReadNativeWord(data,0x2919)!=0x29F7) return false;
    marketUnitListPanel.lines[12]=&marketUnitListRows[11].display;
    if (MwReadNativeWord(data,0x291B)!=0x2A09) return false;
    marketUnitListPanel.lines[13]=&marketUnitListRows[12].display;
    marketMechStatusPanel.x=MwReadNativeWord(data,0x437E);
    marketMechStatusPanel.y=MwReadNativeWord(data,0x4380);
    marketMechStatusPanel.width=MwReadNativeWord(data,0x4382);
    marketMechStatusPanel.bottomInset=MwReadNativeWord(data,0x4384);
    marketMechStatusPanel.lineCount=MwReadNativeWord(data,0x4386);
    marketMechStatusPanel.borderEnabled=MwReadNativeWord(data,0x4388);
    marketMechStatusPanel.lineSpacing=MwReadNativeWord(data,0x438A);
    marketMechStatusPanel.background=MwReadNativeWord(data,0x438C);
    if (MwReadNativeWord(data,0x438E)!=0x4396) return false;
    marketMechStatusPanel.lines[0]=&marketMechStatusLabels[0].display;
    if (MwReadNativeWord(data,0x4390)!=0x4418) return false;
    marketMechStatusPanel.lines[1]=&marketMechStatusLabels[1].display;
    if (MwReadNativeWord(data,0x4392)!=0x4421) return false;
    marketMechStatusPanel.lines[2]=&marketMechStatusLabels[2].display;
    if (MwReadNativeWord(data,0x4394)!=0x4434) return false;
    marketMechStatusPanel.lines[3]=&marketMechStatusLabels[3].display;
    marketDropShipStatusPanel.x=MwReadNativeWord(data,0x443E);
    marketDropShipStatusPanel.y=MwReadNativeWord(data,0x4440);
    marketDropShipStatusPanel.width=MwReadNativeWord(data,0x4442);
    marketDropShipStatusPanel.bottomInset=MwReadNativeWord(data,0x4444);
    marketDropShipStatusPanel.lineCount=MwReadNativeWord(data,0x4446);
    marketDropShipStatusPanel.borderEnabled=MwReadNativeWord(data,0x4448);
    marketDropShipStatusPanel.lineSpacing=MwReadNativeWord(data,0x444A);
    marketDropShipStatusPanel.background=MwReadNativeWord(data,0x444C);
    if (MwReadNativeWord(data,0x444E)!=0x4454) return false;
    marketDropShipStatusPanel.lines[0]=&marketDropShipStatusLabels[0].display;
    if (MwReadNativeWord(data,0x4450)!=0x4496) return false;
    marketDropShipStatusPanel.lines[1]=&marketDropShipStatusLabels[1].display;
    if (MwReadNativeWord(data,0x4452)!=0x449F) return false;
    marketDropShipStatusPanel.lines[2]=&marketDropShipStatusLabels[2].display;
    marketComponentPanel.x=MwReadNativeWord(data,0x44F1);
    marketComponentPanel.y=MwReadNativeWord(data,0x44F3);
    marketComponentPanel.width=MwReadNativeWord(data,0x44F5);
    marketComponentPanel.bottomInset=MwReadNativeWord(data,0x44F7);
    marketComponentPanel.lineCount=MwReadNativeWord(data,0x44F9);
    marketComponentPanel.borderEnabled=MwReadNativeWord(data,0x44FB);
    marketComponentPanel.lineSpacing=MwReadNativeWord(data,0x44FD);
    marketComponentPanel.background=MwReadNativeWord(data,0x44FF);
    if (MwReadNativeWord(data,0x4501)!=0x4531) return false;
    marketComponentPanel.lines[0]=&marketComponentTitle.display;
    if (MwReadNativeWord(data,0x4503)!=0x454F) return false;
    marketComponentPanel.lines[1]=&marketSystemConditionRows[0].display;
    if (MwReadNativeWord(data,0x4505)!=0x456E) return false;
    marketComponentPanel.lines[2]=&marketSystemConditionRows[1].display;
    if (MwReadNativeWord(data,0x4507)!=0x458D) return false;
    marketComponentPanel.lines[3]=&marketSystemConditionRows[2].display;
    if (MwReadNativeWord(data,0x4509)!=0x45AC) return false;
    marketComponentPanel.lines[4]=&marketSystemConditionRows[3].display;
    if (MwReadNativeWord(data,0x450B)!=0x45CB) return false;
    marketComponentPanel.lines[5]=&marketSystemConditionRows[4].display;
    if (MwReadNativeWord(data,0x450D)!=0x45EA) return false;
    marketComponentPanel.lines[6]=&marketSystemConditionRows[5].display;
    if (MwReadNativeWord(data,0x450F)!=0x4609) return false;
    marketComponentPanel.lines[7]=&marketSystemConditionRows[6].display;
    if (MwReadNativeWord(data,0x4511)!=0x4628) return false;
    marketComponentPanel.lines[8]=&marketSystemConditionRows[7].display;
    if (MwReadNativeWord(data,0x4513)!=0x4647) return false;
    marketComponentPanel.lines[9]=&marketSystemConditionRows[8].display;
    if (MwReadNativeWord(data,0x4515)!=0x4666) return false;
    marketComponentPanel.lines[10]=&marketSystemConditionRows[9].display;
    if (MwReadNativeWord(data,0x4517)!=0x4685) return false;
    marketComponentPanel.lines[11]=&marketBodyCondition.display;
    if (MwReadNativeWord(data,0x4519)!=0x469D) return false;
    marketComponentPanel.lines[12]=&marketWeaponHeading.display;
    if (MwReadNativeWord(data,0x451B)!=0x46BA) return false;
    marketComponentPanel.lines[13]=&marketWeaponConditionRows[0].display;
    if (MwReadNativeWord(data,0x451D)!=0x46D9) return false;
    marketComponentPanel.lines[14]=&marketWeaponConditionRows[1].display;
    if (MwReadNativeWord(data,0x451F)!=0x46F8) return false;
    marketComponentPanel.lines[15]=&marketWeaponConditionRows[2].display;
    if (MwReadNativeWord(data,0x4521)!=0x4717) return false;
    marketComponentPanel.lines[16]=&marketWeaponConditionRows[3].display;
    if (MwReadNativeWord(data,0x4523)!=0x4736) return false;
    marketComponentPanel.lines[17]=&marketWeaponConditionRows[4].display;
    if (MwReadNativeWord(data,0x4525)!=0x4755) return false;
    marketComponentPanel.lines[18]=&marketWeaponConditionRows[5].display;
    if (MwReadNativeWord(data,0x4527)!=0x4774) return false;
    marketComponentPanel.lines[19]=&marketWeaponConditionRows[6].display;
    if (MwReadNativeWord(data,0x4529)!=0x4793) return false;
    marketComponentPanel.lines[20]=&marketWeaponConditionRows[7].display;
    if (MwReadNativeWord(data,0x452B)!=0x47B2) return false;
    marketComponentPanel.lines[21]=&marketWeaponConditionRows[8].display;
    if (MwReadNativeWord(data,0x452D)!=0x47D1) return false;
    marketComponentPanel.lines[22]=&marketWeaponConditionRows[9].display;
    if (MwReadNativeWord(data,0x452F)!=0x47F0) return false;
    marketComponentPanel.lines[23]=&marketComponentDone.display;
    mechComplexPanel.x=MwReadNativeWord(data,0x25C5);
    mechComplexPanel.y=MwReadNativeWord(data,0x25C7);
    mechComplexPanel.width=MwReadNativeWord(data,0x25C9);
    mechComplexPanel.bottomInset=MwReadNativeWord(data,0x25CB);
    mechComplexPanel.lineCount=MwReadNativeWord(data,0x25CD);
    mechComplexPanel.borderEnabled=MwReadNativeWord(data,0x25CF);
    mechComplexPanel.lineSpacing=MwReadNativeWord(data,0x25D1);
    mechComplexPanel.background=MwReadNativeWord(data,0x25D3);
    if (MwReadNativeWord(data,0x25D5)!=0x25DF) return false;
    mechComplexPanel.lines[0]=&mechComplexLabels[0].display;
    if (MwReadNativeWord(data,0x25D7)!=0x25F1) return false;
    mechComplexPanel.lines[1]=&mechComplexLabels[1].display;
    if (MwReadNativeWord(data,0x25D9)!=0x2603) return false;
    mechComplexPanel.lines[2]=&mechComplexLabels[2].display;
    if (MwReadNativeWord(data,0x25DB)!=0x2614) return false;
    mechComplexPanel.lines[3]=&mechComplexLabels[3].display;
    if (MwReadNativeWord(data,0x25DD)!=0x2624) return false;
    mechComplexPanel.lines[4]=&mechComplexLabels[4].display;
    ownedUnitListPanel.x=MwReadNativeWord(data,0x266A);
    ownedUnitListPanel.y=MwReadNativeWord(data,0x266C);
    ownedUnitListPanel.width=MwReadNativeWord(data,0x266E);
    ownedUnitListPanel.bottomInset=MwReadNativeWord(data,0x2670);
    ownedUnitListPanel.lineCount=MwReadNativeWord(data,0x2672);
    ownedUnitListPanel.borderEnabled=MwReadNativeWord(data,0x2674);
    ownedUnitListPanel.lineSpacing=MwReadNativeWord(data,0x2676);
    ownedUnitListPanel.background=MwReadNativeWord(data,0x2678);
    if (MwReadNativeWord(data,0x267A)!=0x2696) return false;
    ownedUnitListPanel.lines[0]=&ownedUnitListHeading.display;
    if (MwReadNativeWord(data,0x267C)!=0x26B5) return false;
    ownedUnitListPanel.lines[1]=&ownedUnitListRows[0].display;
    if (MwReadNativeWord(data,0x267E)!=0x26D3) return false;
    ownedUnitListPanel.lines[2]=&ownedUnitListRows[1].display;
    if (MwReadNativeWord(data,0x2680)!=0x26F1) return false;
    ownedUnitListPanel.lines[3]=&ownedUnitListRows[2].display;
    if (MwReadNativeWord(data,0x2682)!=0x270F) return false;
    ownedUnitListPanel.lines[4]=&ownedUnitListRows[3].display;
    if (MwReadNativeWord(data,0x2684)!=0x272D) return false;
    ownedUnitListPanel.lines[5]=&ownedUnitListRows[4].display;
    if (MwReadNativeWord(data,0x2686)!=0x274B) return false;
    ownedUnitListPanel.lines[6]=&ownedUnitListRows[5].display;
    if (MwReadNativeWord(data,0x2688)!=0x2769) return false;
    ownedUnitListPanel.lines[7]=&ownedUnitListRows[6].display;
    if (MwReadNativeWord(data,0x268A)!=0x2787) return false;
    ownedUnitListPanel.lines[8]=&ownedUnitListRows[7].display;
    if (MwReadNativeWord(data,0x268C)!=0x27A5) return false;
    ownedUnitListPanel.lines[9]=&ownedUnitListRows[8].display;
    if (MwReadNativeWord(data,0x268E)!=0x27C3) return false;
    ownedUnitListPanel.lines[10]=&ownedUnitListRows[9].display;
    if (MwReadNativeWord(data,0x2690)!=0x27E1) return false;
    ownedUnitListPanel.lines[11]=&ownedUnitListRows[10].display;
    if (MwReadNativeWord(data,0x2692)!=0x27FF) return false;
    ownedUnitListPanel.lines[12]=&ownedUnitListRows[11].display;
    if (MwReadNativeWord(data,0x2694)!=0x281D) return false;
    ownedUnitListPanel.lines[13]=&ownedUnitListRows[12].display;
    mechStatusPanel.x=MwReadNativeWord(data,0x2C3F);
    mechStatusPanel.y=MwReadNativeWord(data,0x2C41);
    mechStatusPanel.width=MwReadNativeWord(data,0x2C43);
    mechStatusPanel.bottomInset=MwReadNativeWord(data,0x2C45);
    mechStatusPanel.lineCount=MwReadNativeWord(data,0x2C47);
    mechStatusPanel.borderEnabled=MwReadNativeWord(data,0x2C49);
    mechStatusPanel.lineSpacing=MwReadNativeWord(data,0x2C4B);
    mechStatusPanel.background=MwReadNativeWord(data,0x2C4D);
    if (MwReadNativeWord(data,0x2C4F)!=0x2C5B) return false;
    mechStatusPanel.lines[0]=&mechStatusLabels[0].display;
    if (MwReadNativeWord(data,0x2C51)!=0x2CCA) return false;
    mechStatusPanel.lines[1]=&mechStatusLabels[1].display;
    if (MwReadNativeWord(data,0x2C53)!=0x2CD6) return false;
    mechStatusPanel.lines[2]=&mechStatusLabels[2].display;
    if (MwReadNativeWord(data,0x2C55)!=0x2CE6) return false;
    mechStatusPanel.lines[3]=&mechStatusLabels[3].display;
    if (MwReadNativeWord(data,0x2C57)!=0x2CF2) return false;
    mechStatusPanel.lines[4]=&mechStatusLabels[4].display;
    if (MwReadNativeWord(data,0x2C59)!=0x2CFC) return false;
    mechStatusPanel.lines[5]=&mechStatusLabels[5].display;
    dropShipStatusPanel.x=MwReadNativeWord(data,0x2D06);
    dropShipStatusPanel.y=MwReadNativeWord(data,0x2D08);
    dropShipStatusPanel.width=MwReadNativeWord(data,0x2D0A);
    dropShipStatusPanel.bottomInset=MwReadNativeWord(data,0x2D0C);
    dropShipStatusPanel.lineCount=MwReadNativeWord(data,0x2D0E);
    dropShipStatusPanel.borderEnabled=MwReadNativeWord(data,0x2D10);
    dropShipStatusPanel.lineSpacing=MwReadNativeWord(data,0x2D12);
    dropShipStatusPanel.background=MwReadNativeWord(data,0x2D14);
    if (MwReadNativeWord(data,0x2D16)!=0x2D1C) return false;
    dropShipStatusPanel.lines[0]=&dropShipStatusLabels[0].display;
    if (MwReadNativeWord(data,0x2D18)!=0x2D3F) return false;
    dropShipStatusPanel.lines[1]=&dropShipStatusLabels[1].display;
    if (MwReadNativeWord(data,0x2D1A)!=0x2D49) return false;
    dropShipStatusPanel.lines[2]=&dropShipStatusLabels[2].display;
    componentRepairPanel.x=MwReadNativeWord(data,0x3214);
    componentRepairPanel.y=MwReadNativeWord(data,0x3216);
    componentRepairPanel.width=MwReadNativeWord(data,0x3218);
    componentRepairPanel.bottomInset=MwReadNativeWord(data,0x321A);
    componentRepairPanel.lineCount=MwReadNativeWord(data,0x321C);
    componentRepairPanel.borderEnabled=MwReadNativeWord(data,0x321E);
    componentRepairPanel.lineSpacing=MwReadNativeWord(data,0x3220);
    componentRepairPanel.background=MwReadNativeWord(data,0x3222);
    if (MwReadNativeWord(data,0x3224)!=0x3254) return false;
    componentRepairPanel.lines[0]=&repairTitle.display;
    if (MwReadNativeWord(data,0x3226)!=0x3272) return false;
    componentRepairPanel.lines[1]=&repairSystemConditionRows[0].display;
    if (MwReadNativeWord(data,0x3228)!=0x3291) return false;
    componentRepairPanel.lines[2]=&repairSystemConditionRows[1].display;
    if (MwReadNativeWord(data,0x322A)!=0x32B0) return false;
    componentRepairPanel.lines[3]=&repairSystemConditionRows[2].display;
    if (MwReadNativeWord(data,0x322C)!=0x32CF) return false;
    componentRepairPanel.lines[4]=&repairSystemConditionRows[3].display;
    if (MwReadNativeWord(data,0x322E)!=0x32EE) return false;
    componentRepairPanel.lines[5]=&repairSystemConditionRows[4].display;
    if (MwReadNativeWord(data,0x3230)!=0x330D) return false;
    componentRepairPanel.lines[6]=&repairSystemConditionRows[5].display;
    if (MwReadNativeWord(data,0x3232)!=0x332C) return false;
    componentRepairPanel.lines[7]=&repairSystemConditionRows[6].display;
    if (MwReadNativeWord(data,0x3234)!=0x334B) return false;
    componentRepairPanel.lines[8]=&repairSystemConditionRows[7].display;
    if (MwReadNativeWord(data,0x3236)!=0x336A) return false;
    componentRepairPanel.lines[9]=&repairSystemConditionRows[8].display;
    if (MwReadNativeWord(data,0x3238)!=0x3389) return false;
    componentRepairPanel.lines[10]=&repairSystemConditionRows[9].display;
    if (MwReadNativeWord(data,0x323A)!=0x33A8) return false;
    componentRepairPanel.lines[11]=&repairBodyCondition.display;
    if (MwReadNativeWord(data,0x323C)!=0x33C0) return false;
    componentRepairPanel.lines[12]=&repairFooter.display;
    if (MwReadNativeWord(data,0x323E)!=0x3403) return false;
    componentRepairPanel.lines[13]=&repairWeaponConditionRows[0].display;
    if (MwReadNativeWord(data,0x3240)!=0x3422) return false;
    componentRepairPanel.lines[14]=&repairWeaponConditionRows[1].display;
    if (MwReadNativeWord(data,0x3242)!=0x3441) return false;
    componentRepairPanel.lines[15]=&repairWeaponConditionRows[2].display;
    if (MwReadNativeWord(data,0x3244)!=0x3460) return false;
    componentRepairPanel.lines[16]=&repairWeaponConditionRows[3].display;
    if (MwReadNativeWord(data,0x3246)!=0x347F) return false;
    componentRepairPanel.lines[17]=&repairWeaponConditionRows[4].display;
    if (MwReadNativeWord(data,0x3248)!=0x349E) return false;
    componentRepairPanel.lines[18]=&repairWeaponConditionRows[5].display;
    if (MwReadNativeWord(data,0x324A)!=0x34BD) return false;
    componentRepairPanel.lines[19]=&repairWeaponConditionRows[6].display;
    if (MwReadNativeWord(data,0x324C)!=0x34DC) return false;
    componentRepairPanel.lines[20]=&repairWeaponConditionRows[7].display;
    if (MwReadNativeWord(data,0x324E)!=0x34FB) return false;
    componentRepairPanel.lines[21]=&repairWeaponConditionRows[8].display;
    if (MwReadNativeWord(data,0x3250)!=0x351A) return false;
    componentRepairPanel.lines[22]=&repairWeaponConditionRows[9].display;
    if (MwReadNativeWord(data,0x3252)!=0x3539) return false;
    componentRepairPanel.lines[23]=&repairWeaponConditionRows[10].display;
    for (uint16_t i=0;i<rearmConfirmationMenu.recordCount;++i)
    {
        const uint8_t *encoded=data+0x316D+i*CAMPAIGN_UI_RECORD_BYTES;
        MwUiRecord *row=&rearmConfirmationMenu.records[i];
        row->enabled=encoded[0];row->textRecord=encoded[1];
        row->x=MwReadNativeWord(encoded,2);row->y=MwReadNativeWord(encoded,4);
        row->activateCodeOffset=MwReadNativeWord(encoded,6);row->pictureIndex=MwReadNativeWord(encoded,8);
        row->upRecordIndex=encoded[10];row->downRecordIndex=encoded[11];
        row->leftRecordIndex=encoded[12];row->rightRecordIndex=encoded[13];
        row->text=NULL;row->textColour=NULL;
    }
    if (MwReadNativeWord(data,0x3175)!=0x3147) return false;
    rearmConfirmationMenu.records[0].text=rearmConfirmationLabels[1].text;
    rearmConfirmationMenu.records[0].textColour=&(rearmConfirmationLabels[1].display).colour;
    if (MwReadNativeWord(data,0x3183)!=0x3153) return false;
    rearmConfirmationMenu.records[1].text=rearmConfirmationLabels[2].text;
    rearmConfirmationMenu.records[1].textColour=&(rearmConfirmationLabels[2].display).colour;
    for (uint16_t i=0;i<saleConfirmationMenu.recordCount;++i)
    {
        const uint8_t *encoded=data+0x30F0+i*CAMPAIGN_UI_RECORD_BYTES;
        MwUiRecord *row=&saleConfirmationMenu.records[i];
        row->enabled=encoded[0];row->textRecord=encoded[1];
        row->x=MwReadNativeWord(encoded,2);row->y=MwReadNativeWord(encoded,4);
        row->activateCodeOffset=MwReadNativeWord(encoded,6);row->pictureIndex=MwReadNativeWord(encoded,8);
        row->upRecordIndex=encoded[10];row->downRecordIndex=encoded[11];
        row->leftRecordIndex=encoded[12];row->rightRecordIndex=encoded[13];
        row->text=NULL;row->textColour=NULL;
    }
    if (MwReadNativeWord(data,0x30F8)!=0x30DC) return false;
    saleConfirmationMenu.records[0].text=saleConfirmationLabels[1].text;
    saleConfirmationMenu.records[0].textColour=&(saleConfirmationLabels[1].display).colour;
    if (MwReadNativeWord(data,0x3106)!=0x30E8) return false;
    saleConfirmationMenu.records[1].text=saleConfirmationLabels[2].text;
    saleConfirmationMenu.records[1].textColour=&(saleConfirmationLabels[2].display).colour;
    for (uint16_t i=0;i<marketUnitListMenu.recordCount;++i)
    {
        const uint8_t *encoded=data+0x2A1B+i*CAMPAIGN_UI_RECORD_BYTES;
        MwUiRecord *row=&marketUnitListMenu.records[i];
        row->enabled=encoded[0];row->textRecord=encoded[1];
        row->x=MwReadNativeWord(encoded,2);row->y=MwReadNativeWord(encoded,4);
        row->activateCodeOffset=MwReadNativeWord(encoded,6);row->pictureIndex=MwReadNativeWord(encoded,8);
        row->upRecordIndex=encoded[10];row->downRecordIndex=encoded[11];
        row->leftRecordIndex=encoded[12];row->rightRecordIndex=encoded[13];
        row->text=NULL;row->textColour=NULL;
    }
    if (MwReadNativeWord(data,0x2A23)!=0x2935) return false;
    marketUnitListMenu.records[0].text=marketUnitListRows[0].text;
    marketUnitListMenu.records[0].textColour=&(marketUnitListRows[0].display).colour;
    if (MwReadNativeWord(data,0x2A31)!=0x2947) return false;
    marketUnitListMenu.records[1].text=marketUnitListRows[1].text;
    marketUnitListMenu.records[1].textColour=&(marketUnitListRows[1].display).colour;
    if (MwReadNativeWord(data,0x2A3F)!=0x2959) return false;
    marketUnitListMenu.records[2].text=marketUnitListRows[2].text;
    marketUnitListMenu.records[2].textColour=&(marketUnitListRows[2].display).colour;
    if (MwReadNativeWord(data,0x2A4D)!=0x296B) return false;
    marketUnitListMenu.records[3].text=marketUnitListRows[3].text;
    marketUnitListMenu.records[3].textColour=&(marketUnitListRows[3].display).colour;
    if (MwReadNativeWord(data,0x2A5B)!=0x297D) return false;
    marketUnitListMenu.records[4].text=marketUnitListRows[4].text;
    marketUnitListMenu.records[4].textColour=&(marketUnitListRows[4].display).colour;
    if (MwReadNativeWord(data,0x2A69)!=0x298F) return false;
    marketUnitListMenu.records[5].text=marketUnitListRows[5].text;
    marketUnitListMenu.records[5].textColour=&(marketUnitListRows[5].display).colour;
    if (MwReadNativeWord(data,0x2A77)!=0x29A1) return false;
    marketUnitListMenu.records[6].text=marketUnitListRows[6].text;
    marketUnitListMenu.records[6].textColour=&(marketUnitListRows[6].display).colour;
    if (MwReadNativeWord(data,0x2A85)!=0x29B3) return false;
    marketUnitListMenu.records[7].text=marketUnitListRows[7].text;
    marketUnitListMenu.records[7].textColour=&(marketUnitListRows[7].display).colour;
    if (MwReadNativeWord(data,0x2A93)!=0x29C5) return false;
    marketUnitListMenu.records[8].text=marketUnitListRows[8].text;
    marketUnitListMenu.records[8].textColour=&(marketUnitListRows[8].display).colour;
    if (MwReadNativeWord(data,0x2AA1)!=0x29D7) return false;
    marketUnitListMenu.records[9].text=marketUnitListRows[9].text;
    marketUnitListMenu.records[9].textColour=&(marketUnitListRows[9].display).colour;
    if (MwReadNativeWord(data,0x2AAF)!=0x29E9) return false;
    marketUnitListMenu.records[10].text=marketUnitListRows[10].text;
    marketUnitListMenu.records[10].textColour=&(marketUnitListRows[10].display).colour;
    if (MwReadNativeWord(data,0x2ABD)!=0x29FB) return false;
    marketUnitListMenu.records[11].text=marketUnitListRows[11].text;
    marketUnitListMenu.records[11].textColour=&(marketUnitListRows[11].display).colour;
    if (MwReadNativeWord(data,0x2ACB)!=0x2A0D) return false;
    marketUnitListMenu.records[12].text=marketUnitListRows[12].text;
    marketUnitListMenu.records[12].textColour=&(marketUnitListRows[12].display).colour;
    for (uint16_t i=0;i<marketMechMenu.recordCount;++i)
    {
        const uint8_t *encoded=data+0x44A9+i*CAMPAIGN_UI_RECORD_BYTES;
        MwUiRecord *row=&marketMechMenu.records[i];
        row->enabled=encoded[0];row->textRecord=encoded[1];
        row->x=MwReadNativeWord(encoded,2);row->y=MwReadNativeWord(encoded,4);
        row->activateCodeOffset=MwReadNativeWord(encoded,6);row->pictureIndex=MwReadNativeWord(encoded,8);
        row->upRecordIndex=encoded[10];row->downRecordIndex=encoded[11];
        row->leftRecordIndex=encoded[12];row->rightRecordIndex=encoded[13];
        row->text=NULL;row->textColour=NULL;
    }
    if (MwReadNativeWord(data,0x44B1)!=0x441C) return false;
    marketMechMenu.records[0].text=marketMechStatusLabels[1].text;
    marketMechMenu.records[0].textColour=&(marketMechStatusLabels[1].display).colour;
    if (MwReadNativeWord(data,0x44BF)!=0x4425) return false;
    marketMechMenu.records[1].text=marketMechStatusLabels[2].text;
    marketMechMenu.records[1].textColour=&(marketMechStatusLabels[2].display).colour;
    if (MwReadNativeWord(data,0x44CD)!=0x4438) return false;
    marketMechMenu.records[2].text=marketMechStatusLabels[3].text;
    marketMechMenu.records[2].textColour=&(marketMechStatusLabels[3].display).colour;
    for (uint16_t i=0;i<marketDropShipMenu.recordCount;++i)
    {
        const uint8_t *encoded=data+0x44D3+i*CAMPAIGN_UI_RECORD_BYTES;
        MwUiRecord *row=&marketDropShipMenu.records[i];
        row->enabled=encoded[0];row->textRecord=encoded[1];
        row->x=MwReadNativeWord(encoded,2);row->y=MwReadNativeWord(encoded,4);
        row->activateCodeOffset=MwReadNativeWord(encoded,6);row->pictureIndex=MwReadNativeWord(encoded,8);
        row->upRecordIndex=encoded[10];row->downRecordIndex=encoded[11];
        row->leftRecordIndex=encoded[12];row->rightRecordIndex=encoded[13];
        row->text=NULL;row->textColour=NULL;
    }
    if (MwReadNativeWord(data,0x44DB)!=0x449A) return false;
    marketDropShipMenu.records[0].text=marketDropShipStatusLabels[1].text;
    marketDropShipMenu.records[0].textColour=&(marketDropShipStatusLabels[1].display).colour;
    if (MwReadNativeWord(data,0x44E9)!=0x44A3) return false;
    marketDropShipMenu.records[1].text=marketDropShipStatusLabels[2].text;
    marketDropShipMenu.records[1].textColour=&(marketDropShipStatusLabels[2].display).colour;
    for (uint16_t i=0;i<marketComponentMenu.recordCount;++i)
    {
        const uint8_t *encoded=data+0x480F+i*CAMPAIGN_UI_RECORD_BYTES;
        MwUiRecord *row=&marketComponentMenu.records[i];
        row->enabled=encoded[0];row->textRecord=encoded[1];
        row->x=MwReadNativeWord(encoded,2);row->y=MwReadNativeWord(encoded,4);
        row->activateCodeOffset=MwReadNativeWord(encoded,6);row->pictureIndex=MwReadNativeWord(encoded,8);
        row->upRecordIndex=encoded[10];row->downRecordIndex=encoded[11];
        row->leftRecordIndex=encoded[12];row->rightRecordIndex=encoded[13];
        row->text=NULL;row->textColour=NULL;
    }
    if (MwReadNativeWord(data,0x4817)!=0x4553) return false;
    marketComponentMenu.records[0].text=marketSystemConditionRows[0].caption.text;
    marketComponentMenu.records[0].textColour=&(marketSystemConditionRows[0].display).colour;
    for (uint16_t i=0;i<mechComplexMenu.recordCount;++i)
    {
        const uint8_t *encoded=data+0x2632+i*CAMPAIGN_UI_RECORD_BYTES;
        MwUiRecord *row=&mechComplexMenu.records[i];
        row->enabled=encoded[0];row->textRecord=encoded[1];
        row->x=MwReadNativeWord(encoded,2);row->y=MwReadNativeWord(encoded,4);
        row->activateCodeOffset=MwReadNativeWord(encoded,6);row->pictureIndex=MwReadNativeWord(encoded,8);
        row->upRecordIndex=encoded[10];row->downRecordIndex=encoded[11];
        row->leftRecordIndex=encoded[12];row->rightRecordIndex=encoded[13];
        row->text=NULL;row->textColour=NULL;
    }
    if (MwReadNativeWord(data,0x263A)!=0x25F5) return false;
    mechComplexMenu.records[0].text=mechComplexLabels[1].text;
    mechComplexMenu.records[0].textColour=&(mechComplexLabels[1].display).colour;
    if (MwReadNativeWord(data,0x2648)!=0x2607) return false;
    mechComplexMenu.records[1].text=mechComplexLabels[2].text;
    mechComplexMenu.records[1].textColour=&(mechComplexLabels[2].display).colour;
    if (MwReadNativeWord(data,0x2656)!=0x2618) return false;
    mechComplexMenu.records[2].text=mechComplexLabels[3].text;
    mechComplexMenu.records[2].textColour=&(mechComplexLabels[3].display).colour;
    if (MwReadNativeWord(data,0x2664)!=0x2628) return false;
    mechComplexMenu.records[3].text=mechComplexLabels[4].text;
    mechComplexMenu.records[3].textColour=&(mechComplexLabels[4].display).colour;
    for (uint16_t i=0;i<planetHubMenu.recordCount;++i)
    {
        const uint8_t *encoded=data+0x1083+i*CAMPAIGN_UI_RECORD_BYTES;
        MwUiRecord *row=&planetHubMenu.records[i];
        row->enabled=encoded[0];row->textRecord=encoded[1];
        row->x=MwReadNativeWord(encoded,2);row->y=MwReadNativeWord(encoded,4);
        row->activateCodeOffset=MwReadNativeWord(encoded,6);row->pictureIndex=MwReadNativeWord(encoded,8);
        row->upRecordIndex=encoded[10];row->downRecordIndex=encoded[11];
        row->leftRecordIndex=encoded[12];row->rightRecordIndex=encoded[13];
        row->text=NULL;row->textColour=NULL;
    }
    for (uint16_t i=0;i<contractHubMenu.recordCount;++i)
    {
        const uint8_t *encoded=data+0x10D7+i*CAMPAIGN_UI_RECORD_BYTES;
        MwUiRecord *row=&contractHubMenu.records[i];
        row->enabled=encoded[0];row->textRecord=encoded[1];
        row->x=MwReadNativeWord(encoded,2);row->y=MwReadNativeWord(encoded,4);
        row->activateCodeOffset=MwReadNativeWord(encoded,6);row->pictureIndex=MwReadNativeWord(encoded,8);
        row->upRecordIndex=encoded[10];row->downRecordIndex=encoded[11];
        row->leftRecordIndex=encoded[12];row->rightRecordIndex=encoded[13];
        row->text=NULL;row->textColour=NULL;
    }
    for (uint16_t i=0;i<remotePlanetHubMenu.recordCount;++i)
    {
        const uint8_t *encoded=data+0x1101+i*CAMPAIGN_UI_RECORD_BYTES;
        MwUiRecord *row=&remotePlanetHubMenu.records[i];
        row->enabled=encoded[0];row->textRecord=encoded[1];
        row->x=MwReadNativeWord(encoded,2);row->y=MwReadNativeWord(encoded,4);
        row->activateCodeOffset=MwReadNativeWord(encoded,6);row->pictureIndex=MwReadNativeWord(encoded,8);
        row->upRecordIndex=encoded[10];row->downRecordIndex=encoded[11];
        row->leftRecordIndex=encoded[12];row->rightRecordIndex=encoded[13];
        row->text=NULL;row->textColour=NULL;
    }
    for (uint16_t i=0;i<ownedUnitListMenu.recordCount;++i)
    {
        const uint8_t *encoded=data+0x283B+i*CAMPAIGN_UI_RECORD_BYTES;
        MwUiRecord *row=&ownedUnitListMenu.records[i];
        row->enabled=encoded[0];row->textRecord=encoded[1];
        row->x=MwReadNativeWord(encoded,2);row->y=MwReadNativeWord(encoded,4);
        row->activateCodeOffset=MwReadNativeWord(encoded,6);row->pictureIndex=MwReadNativeWord(encoded,8);
        row->upRecordIndex=encoded[10];row->downRecordIndex=encoded[11];
        row->leftRecordIndex=encoded[12];row->rightRecordIndex=encoded[13];
        row->text=NULL;row->textColour=NULL;
    }
    if (MwReadNativeWord(data,0x2843)!=0x26B9) return false;
    ownedUnitListMenu.records[0].text=ownedUnitListRows[0].caption.text;
    ownedUnitListMenu.records[0].textColour=&(ownedUnitListRows[0].display).colour;
    if (MwReadNativeWord(data,0x2851)!=0x26D7) return false;
    ownedUnitListMenu.records[1].text=ownedUnitListRows[1].caption.text;
    ownedUnitListMenu.records[1].textColour=&(ownedUnitListRows[1].display).colour;
    if (MwReadNativeWord(data,0x285F)!=0x26F5) return false;
    ownedUnitListMenu.records[2].text=ownedUnitListRows[2].caption.text;
    ownedUnitListMenu.records[2].textColour=&(ownedUnitListRows[2].display).colour;
    if (MwReadNativeWord(data,0x286D)!=0x2713) return false;
    ownedUnitListMenu.records[3].text=ownedUnitListRows[3].caption.text;
    ownedUnitListMenu.records[3].textColour=&(ownedUnitListRows[3].display).colour;
    if (MwReadNativeWord(data,0x287B)!=0x2731) return false;
    ownedUnitListMenu.records[4].text=ownedUnitListRows[4].caption.text;
    ownedUnitListMenu.records[4].textColour=&(ownedUnitListRows[4].display).colour;
    if (MwReadNativeWord(data,0x2889)!=0x274F) return false;
    ownedUnitListMenu.records[5].text=ownedUnitListRows[5].caption.text;
    ownedUnitListMenu.records[5].textColour=&(ownedUnitListRows[5].display).colour;
    if (MwReadNativeWord(data,0x2897)!=0x276D) return false;
    ownedUnitListMenu.records[6].text=ownedUnitListRows[6].caption.text;
    ownedUnitListMenu.records[6].textColour=&(ownedUnitListRows[6].display).colour;
    if (MwReadNativeWord(data,0x28A5)!=0x278B) return false;
    ownedUnitListMenu.records[7].text=ownedUnitListRows[7].caption.text;
    ownedUnitListMenu.records[7].textColour=&(ownedUnitListRows[7].display).colour;
    if (MwReadNativeWord(data,0x28B3)!=0x27A9) return false;
    ownedUnitListMenu.records[8].text=ownedUnitListRows[8].caption.text;
    ownedUnitListMenu.records[8].textColour=&(ownedUnitListRows[8].display).colour;
    if (MwReadNativeWord(data,0x28C1)!=0x27C7) return false;
    ownedUnitListMenu.records[9].text=ownedUnitListRows[9].caption.text;
    ownedUnitListMenu.records[9].textColour=&(ownedUnitListRows[9].display).colour;
    if (MwReadNativeWord(data,0x28CF)!=0x27E5) return false;
    ownedUnitListMenu.records[10].text=ownedUnitListRows[10].caption.text;
    ownedUnitListMenu.records[10].textColour=&(ownedUnitListRows[10].display).colour;
    if (MwReadNativeWord(data,0x28DD)!=0x2803) return false;
    ownedUnitListMenu.records[11].text=ownedUnitListRows[11].caption.text;
    ownedUnitListMenu.records[11].textColour=&(ownedUnitListRows[11].display).colour;
    if (MwReadNativeWord(data,0x28EB)!=0x2821) return false;
    ownedUnitListMenu.records[12].text=ownedUnitListRows[12].caption.text;
    ownedUnitListMenu.records[12].textColour=&(ownedUnitListRows[12].display).colour;
    for (uint16_t i=0;i<mechServiceMenu.recordCount;++i)
    {
        const uint8_t *encoded=data+0x2D53+i*CAMPAIGN_UI_RECORD_BYTES;
        MwUiRecord *row=&mechServiceMenu.records[i];
        row->enabled=encoded[0];row->textRecord=encoded[1];
        row->x=MwReadNativeWord(encoded,2);row->y=MwReadNativeWord(encoded,4);
        row->activateCodeOffset=MwReadNativeWord(encoded,6);row->pictureIndex=MwReadNativeWord(encoded,8);
        row->upRecordIndex=encoded[10];row->downRecordIndex=encoded[11];
        row->leftRecordIndex=encoded[12];row->rightRecordIndex=encoded[13];
        row->text=NULL;row->textColour=NULL;
    }
    if (MwReadNativeWord(data,0x2D5B)!=0x2CCE) return false;
    mechServiceMenu.records[0].text=mechStatusLabels[1].text;
    mechServiceMenu.records[0].textColour=&(mechStatusLabels[1].display).colour;
    if (MwReadNativeWord(data,0x2D69)!=0x2CDA) return false;
    mechServiceMenu.records[1].text=mechStatusLabels[2].text;
    mechServiceMenu.records[1].textColour=&(mechStatusLabels[2].display).colour;
    if (MwReadNativeWord(data,0x2D77)!=0x2CEA) return false;
    mechServiceMenu.records[2].text=mechStatusLabels[3].text;
    mechServiceMenu.records[2].textColour=&(mechStatusLabels[3].display).colour;
    if (MwReadNativeWord(data,0x2D85)!=0x2CF6) return false;
    mechServiceMenu.records[3].text=mechStatusLabels[4].text;
    mechServiceMenu.records[3].textColour=&(mechStatusLabels[4].display).colour;
    if (MwReadNativeWord(data,0x2D93)!=0x2D00) return false;
    mechServiceMenu.records[4].text=mechStatusLabels[5].text;
    mechServiceMenu.records[4].textColour=&(mechStatusLabels[5].display).colour;
    for (uint16_t i=0;i<dropShipServiceMenu.recordCount;++i)
    {
        const uint8_t *encoded=data+0x2D99+i*CAMPAIGN_UI_RECORD_BYTES;
        MwUiRecord *row=&dropShipServiceMenu.records[i];
        row->enabled=encoded[0];row->textRecord=encoded[1];
        row->x=MwReadNativeWord(encoded,2);row->y=MwReadNativeWord(encoded,4);
        row->activateCodeOffset=MwReadNativeWord(encoded,6);row->pictureIndex=MwReadNativeWord(encoded,8);
        row->upRecordIndex=encoded[10];row->downRecordIndex=encoded[11];
        row->leftRecordIndex=encoded[12];row->rightRecordIndex=encoded[13];
        row->text=NULL;row->textColour=NULL;
    }
    if (MwReadNativeWord(data,0x2DA1)!=0x2D43) return false;
    dropShipServiceMenu.records[0].text=dropShipStatusLabels[1].text;
    dropShipServiceMenu.records[0].textColour=&(dropShipStatusLabels[1].display).colour;
    if (MwReadNativeWord(data,0x2DAF)!=0x2D4D) return false;
    dropShipServiceMenu.records[1].text=dropShipStatusLabels[2].text;
    dropShipServiceMenu.records[1].textColour=&(dropShipStatusLabels[2].display).colour;
    for (uint16_t i=0;i<componentRepairMenu.recordCount;++i)
    {
        const uint8_t *encoded=data+0x3558+i*CAMPAIGN_UI_RECORD_BYTES;
        MwUiRecord *row=&componentRepairMenu.records[i];
        row->enabled=encoded[0];row->textRecord=encoded[1];
        row->x=MwReadNativeWord(encoded,2);row->y=MwReadNativeWord(encoded,4);
        row->activateCodeOffset=MwReadNativeWord(encoded,6);row->pictureIndex=MwReadNativeWord(encoded,8);
        row->upRecordIndex=encoded[10];row->downRecordIndex=encoded[11];
        row->leftRecordIndex=encoded[12];row->rightRecordIndex=encoded[13];
        row->text=NULL;row->textColour=NULL;
    }
    if (MwReadNativeWord(data,0x3560)!=0x3276) return false;
    componentRepairMenu.records[0].text=repairSystemConditionRows[0].caption.text;
    componentRepairMenu.records[0].textColour=&(repairSystemConditionRows[0].display).colour;
    if (MwReadNativeWord(data,0x356E)!=0x3295) return false;
    componentRepairMenu.records[1].text=repairSystemConditionRows[1].caption.text;
    componentRepairMenu.records[1].textColour=&(repairSystemConditionRows[1].display).colour;
    if (MwReadNativeWord(data,0x357C)!=0x32B4) return false;
    componentRepairMenu.records[2].text=repairSystemConditionRows[2].caption.text;
    componentRepairMenu.records[2].textColour=&(repairSystemConditionRows[2].display).colour;
    if (MwReadNativeWord(data,0x358A)!=0x32D3) return false;
    componentRepairMenu.records[3].text=repairSystemConditionRows[3].caption.text;
    componentRepairMenu.records[3].textColour=&(repairSystemConditionRows[3].display).colour;
    if (MwReadNativeWord(data,0x3598)!=0x32F2) return false;
    componentRepairMenu.records[4].text=repairSystemConditionRows[4].caption.text;
    componentRepairMenu.records[4].textColour=&(repairSystemConditionRows[4].display).colour;
    if (MwReadNativeWord(data,0x35A6)!=0x3311) return false;
    componentRepairMenu.records[5].text=repairSystemConditionRows[5].caption.text;
    componentRepairMenu.records[5].textColour=&(repairSystemConditionRows[5].display).colour;
    if (MwReadNativeWord(data,0x35B4)!=0x3330) return false;
    componentRepairMenu.records[6].text=repairSystemConditionRows[6].caption.text;
    componentRepairMenu.records[6].textColour=&(repairSystemConditionRows[6].display).colour;
    if (MwReadNativeWord(data,0x35C2)!=0x334F) return false;
    componentRepairMenu.records[7].text=repairSystemConditionRows[7].caption.text;
    componentRepairMenu.records[7].textColour=&(repairSystemConditionRows[7].display).colour;
    if (MwReadNativeWord(data,0x35D0)!=0x336E) return false;
    componentRepairMenu.records[8].text=repairSystemConditionRows[8].caption.text;
    componentRepairMenu.records[8].textColour=&(repairSystemConditionRows[8].display).colour;
    if (MwReadNativeWord(data,0x35DE)!=0x338D) return false;
    componentRepairMenu.records[9].text=repairSystemConditionRows[9].caption.text;
    componentRepairMenu.records[9].textColour=&(repairSystemConditionRows[9].display).colour;
    if (MwReadNativeWord(data,0x35EC)!=0x33AC) return false;
    componentRepairMenu.records[10].text=repairBodyCondition.caption.text;
    componentRepairMenu.records[10].textColour=&(repairBodyCondition.display).colour;
    if (MwReadNativeWord(data,0x35FA)!=0x3407) return false;
    componentRepairMenu.records[11].text=repairWeaponConditionRows[0].caption.text;
    componentRepairMenu.records[11].textColour=&(repairWeaponConditionRows[0].display).colour;
    if (MwReadNativeWord(data,0x3608)!=0x3426) return false;
    componentRepairMenu.records[12].text=repairWeaponConditionRows[1].caption.text;
    componentRepairMenu.records[12].textColour=&(repairWeaponConditionRows[1].display).colour;
    if (MwReadNativeWord(data,0x3616)!=0x3445) return false;
    componentRepairMenu.records[13].text=repairWeaponConditionRows[2].caption.text;
    componentRepairMenu.records[13].textColour=&(repairWeaponConditionRows[2].display).colour;
    if (MwReadNativeWord(data,0x3624)!=0x3464) return false;
    componentRepairMenu.records[14].text=repairWeaponConditionRows[3].caption.text;
    componentRepairMenu.records[14].textColour=&(repairWeaponConditionRows[3].display).colour;
    if (MwReadNativeWord(data,0x3632)!=0x3483) return false;
    componentRepairMenu.records[15].text=repairWeaponConditionRows[4].caption.text;
    componentRepairMenu.records[15].textColour=&(repairWeaponConditionRows[4].display).colour;
    if (MwReadNativeWord(data,0x3640)!=0x34A2) return false;
    componentRepairMenu.records[16].text=repairWeaponConditionRows[5].caption.text;
    componentRepairMenu.records[16].textColour=&(repairWeaponConditionRows[5].display).colour;
    if (MwReadNativeWord(data,0x364E)!=0x34C1) return false;
    componentRepairMenu.records[17].text=repairWeaponConditionRows[6].caption.text;
    componentRepairMenu.records[17].textColour=&(repairWeaponConditionRows[6].display).colour;
    if (MwReadNativeWord(data,0x365C)!=0x34E0) return false;
    componentRepairMenu.records[18].text=repairWeaponConditionRows[7].caption.text;
    componentRepairMenu.records[18].textColour=&(repairWeaponConditionRows[7].display).colour;
    if (MwReadNativeWord(data,0x366A)!=0x34FF) return false;
    componentRepairMenu.records[19].text=repairWeaponConditionRows[8].caption.text;
    componentRepairMenu.records[19].textColour=&(repairWeaponConditionRows[8].display).colour;
    if (MwReadNativeWord(data,0x3678)!=0x351E) return false;
    componentRepairMenu.records[20].text=repairWeaponConditionRows[9].caption.text;
    componentRepairMenu.records[20].textColour=&(repairWeaponConditionRows[9].display).colour;
    if (MwReadNativeWord(data,0x3686)!=0x353D) return false;
    componentRepairMenu.records[21].text=repairWeaponConditionRows[10].caption.text;
    componentRepairMenu.records[21].textColour=&(repairWeaponConditionRows[10].display).colour;
    for (uint16_t chassis=0;chassis<8;++chassis)
        for (uint16_t region=0;region<10;++region)
        {
            MwMechDiagramRectangle *rectangle=region?&mechBodyDiagramLayouts[chassis].bodyRegions[region-1]:&mechBodyDiagramLayouts[chassis].background;
            uint16_t at=0x3CEE+chassis*0x50+region*8;
            rectangle->x=MwReadNativeWord(data,at);rectangle->y=MwReadNativeWord(data,(uint16_t)(at+2));
            rectangle->width=MwReadNativeWord(data,(uint16_t)(at+4));rectangle->height=MwReadNativeWord(data,(uint16_t)(at+6));
        }
    /* These initial near values have no live object until their callers set them. */
    repairQuoteDamage=NULL;componentRepairDamage=NULL;
    mechDiagramBodyDamage=NULL;mechDiagramRectangle=NULL;uiMenu=NULL;
    return ImportPilotUiData(data) && ImportTravelUiData(data) && MwImportCampaignEventData(data,byteCount)
        && MwImportCampaignOptionsData(data,byteCount) && MwImportCampaignHandoffData(data,byteCount);
}
/* Installation decoder, not an added native gameplay procedure. */
static bool ImportPilotUiData(const uint8_t *data)
{
    companyMenuPanel.x=MwReadNativeWord(data,0x21CE);
    companyMenuPanel.y=MwReadNativeWord(data,0x21D0);
    companyMenuPanel.width=MwReadNativeWord(data,0x21D2);
    companyMenuPanel.bottomInset=MwReadNativeWord(data,0x21D4);
    companyMenuPanel.lineCount=MwReadNativeWord(data,0x21D6);
    companyMenuPanel.borderEnabled=MwReadNativeWord(data,0x21D8);
    companyMenuPanel.lineSpacing=MwReadNativeWord(data,0x21DA);
    companyMenuPanel.background=MwReadNativeWord(data,0x21DC);
    if (companyMenuPanel.lineCount!=4) return false;
    if (MwReadNativeWord(data,0x21DE)!=0x21E6) return false;
    if (!ImportInitialString(data,0x21EB,companyMenuLabels[0].text,sizeof(companyMenuLabels[0].text))) return false;
    ImportInitialText(data,0x21E6,&companyMenuLabels[0].display,companyMenuLabels[0].text,0);
    companyMenuPanel.lines[0]=&companyMenuLabels[0].display;
    if (MwReadNativeWord(data,0x21E0)!=0x21F8) return false;
    if (!ImportInitialString(data,0x21FD,companyMenuLabels[1].text,sizeof(companyMenuLabels[1].text))) return false;
    ImportInitialText(data,0x21F8,&companyMenuLabels[1].display,companyMenuLabels[1].text,0);
    companyMenuPanel.lines[1]=&companyMenuLabels[1].display;
    if (MwReadNativeWord(data,0x21E2)!=0x220A) return false;
    if (!ImportInitialString(data,0x220F,companyMenuLabels[2].text,sizeof(companyMenuLabels[2].text))) return false;
    ImportInitialText(data,0x220A,&companyMenuLabels[2].display,companyMenuLabels[2].text,0);
    companyMenuPanel.lines[2]=&companyMenuLabels[2].display;
    if (MwReadNativeWord(data,0x21E4)!=0x221C) return false;
    if (!ImportInitialString(data,0x2221,companyMenuLabels[3].text,sizeof(companyMenuLabels[3].text))) return false;
    ImportInitialText(data,0x221C,&companyMenuLabels[3].display,companyMenuLabels[3].text,0);
    companyMenuPanel.lines[3]=&companyMenuLabels[3].display;
    commanderProfilePanel.x=MwReadNativeWord(data,0x2258);
    commanderProfilePanel.y=MwReadNativeWord(data,0x225A);
    commanderProfilePanel.width=MwReadNativeWord(data,0x225C);
    commanderProfilePanel.bottomInset=MwReadNativeWord(data,0x225E);
    commanderProfilePanel.lineCount=MwReadNativeWord(data,0x2260);
    commanderProfilePanel.borderEnabled=MwReadNativeWord(data,0x2262);
    commanderProfilePanel.lineSpacing=MwReadNativeWord(data,0x2264);
    commanderProfilePanel.background=MwReadNativeWord(data,0x2266);
    if (commanderProfilePanel.lineCount!=11) return false;
    if (MwReadNativeWord(data,0x2268)!=0x227E) return false;
    if (!ImportInitialString(data,0x2283,commanderProfileLabels[0].text,sizeof(commanderProfileLabels[0].text))) return false;
    ImportInitialText(data,0x227E,&commanderProfileLabels[0].display,commanderProfileLabels[0].text,0);
    commanderProfilePanel.lines[0]=&commanderProfileLabels[0].display;
    if (MwReadNativeWord(data,0x226A)!=0x2290) return false;
    if (!ImportInitialString(data,0x2295,commanderProfileLabels[1].text,sizeof(commanderProfileLabels[1].text))) return false;
    ImportInitialText(data,0x2290,&commanderProfileLabels[1].display,commanderProfileLabels[1].text,0);
    commanderProfilePanel.lines[1]=&commanderProfileLabels[1].display;
    if (MwReadNativeWord(data,0x226C)!=0x22A0) return false;
    if (!ImportInitialString(data,0x22A5,commanderProfileLabels[2].text,sizeof(commanderProfileLabels[2].text))) return false;
    ImportInitialText(data,0x22A0,&commanderProfileLabels[2].display,commanderProfileLabels[2].text,0);
    commanderProfilePanel.lines[2]=&commanderProfileLabels[2].display;
    if (MwReadNativeWord(data,0x226E)!=0x22AA) return false;
    if (!ImportInitialString(data,0x22AF,commanderProfileLabels[3].text,sizeof(commanderProfileLabels[3].text))) return false;
    ImportInitialText(data,0x22AA,&commanderProfileLabels[3].display,commanderProfileLabels[3].text,0);
    commanderProfilePanel.lines[3]=&commanderProfileLabels[3].display;
    if (MwReadNativeWord(data,0x2270)!=0x22BB) return false;
    if (!ImportInitialString(data,0x22C0,commanderProfileLabels[4].text,sizeof(commanderProfileLabels[4].text))) return false;
    ImportInitialText(data,0x22BB,&commanderProfileLabels[4].display,commanderProfileLabels[4].text,0);
    commanderProfilePanel.lines[4]=&commanderProfileLabels[4].display;
    if (MwReadNativeWord(data,0x2272)!=0x22C8) return false;
    if (!ImportInitialString(data,0x22CD,commanderProfileLabels[5].text,sizeof(commanderProfileLabels[5].text))) return false;
    ImportInitialText(data,0x22C8,&commanderProfileLabels[5].display,commanderProfileLabels[5].text,0);
    commanderProfilePanel.lines[5]=&commanderProfileLabels[5].display;
    if (MwReadNativeWord(data,0x2274)!=0x22DF) return false;
    if (!ImportInitialString(data,0x22E4,commanderProfileLabels[6].text,sizeof(commanderProfileLabels[6].text))) return false;
    ImportInitialText(data,0x22DF,&commanderProfileLabels[6].display,commanderProfileLabels[6].text,0);
    commanderProfilePanel.lines[6]=&commanderProfileLabels[6].display;
    if (MwReadNativeWord(data,0x2276)!=0x22ED) return false;
    if (!ImportInitialString(data,0x22F2,commanderProfileLabels[7].text,sizeof(commanderProfileLabels[7].text))) return false;
    ImportInitialText(data,0x22ED,&commanderProfileLabels[7].display,commanderProfileLabels[7].text,0);
    commanderProfilePanel.lines[7]=&commanderProfileLabels[7].display;
    if (MwReadNativeWord(data,0x2278)!=0x22FB) return false;
    if (!ImportInitialString(data,0x2300,commanderProfileLabels[8].text,sizeof(commanderProfileLabels[8].text))) return false;
    ImportInitialText(data,0x22FB,&commanderProfileLabels[8].display,commanderProfileLabels[8].text,0);
    commanderProfilePanel.lines[8]=&commanderProfileLabels[8].display;
    if (MwReadNativeWord(data,0x227A)!=0x2309) return false;
    if (!ImportInitialString(data,0x230E,commanderProfileLabels[9].text,sizeof(commanderProfileLabels[9].text))) return false;
    ImportInitialText(data,0x2309,&commanderProfileLabels[9].display,commanderProfileLabels[9].text,0);
    commanderProfilePanel.lines[9]=&commanderProfileLabels[9].display;
    if (MwReadNativeWord(data,0x227C)!=0x2317) return false;
    if (!ImportInitialString(data,0x231C,commanderProfileLabels[10].text,sizeof(commanderProfileLabels[10].text))) return false;
    ImportInitialText(data,0x2317,&commanderProfileLabels[10].display,commanderProfileLabels[10].text,0);
    commanderProfilePanel.lines[10]=&commanderProfileLabels[10].display;
    crewLabelsPanel.x=MwReadNativeWord(data,0x2325);
    crewLabelsPanel.y=MwReadNativeWord(data,0x2327);
    crewLabelsPanel.width=MwReadNativeWord(data,0x2329);
    crewLabelsPanel.bottomInset=MwReadNativeWord(data,0x232B);
    crewLabelsPanel.lineCount=MwReadNativeWord(data,0x232D);
    crewLabelsPanel.borderEnabled=MwReadNativeWord(data,0x232F);
    crewLabelsPanel.lineSpacing=MwReadNativeWord(data,0x2331);
    crewLabelsPanel.background=MwReadNativeWord(data,0x2333);
    if (crewLabelsPanel.lineCount!=7) return false;
    if (MwReadNativeWord(data,0x2335)!=0x2343) return false;
    if (!ImportInitialString(data,0x2348,crewLabels[0].text,sizeof(crewLabels[0].text))) return false;
    ImportInitialText(data,0x2343,&crewLabels[0].display,crewLabels[0].text,0);
    crewLabelsPanel.lines[0]=&crewLabels[0].display;
    if (MwReadNativeWord(data,0x2337)!=0x234D) return false;
    if (!ImportInitialString(data,0x2352,crewLabels[1].text,sizeof(crewLabels[1].text))) return false;
    ImportInitialText(data,0x234D,&crewLabels[1].display,crewLabels[1].text,0);
    crewLabelsPanel.lines[1]=&crewLabels[1].display;
    if (MwReadNativeWord(data,0x2339)!=0x2358) return false;
    if (!ImportInitialString(data,0x235D,crewLabels[2].text,sizeof(crewLabels[2].text))) return false;
    ImportInitialText(data,0x2358,&crewLabels[2].display,crewLabels[2].text,0);
    crewLabelsPanel.lines[2]=&crewLabels[2].display;
    if (MwReadNativeWord(data,0x233B)!=0x2363) return false;
    if (!ImportInitialString(data,0x2368,crewLabels[3].text,sizeof(crewLabels[3].text))) return false;
    ImportInitialText(data,0x2363,&crewLabels[3].display,crewLabels[3].text,0);
    crewLabelsPanel.lines[3]=&crewLabels[3].display;
    if (MwReadNativeWord(data,0x233D)!=0x236C) return false;
    if (!ImportInitialString(data,0x2371,crewLabels[4].text,sizeof(crewLabels[4].text))) return false;
    ImportInitialText(data,0x236C,&crewLabels[4].display,crewLabels[4].text,0);
    crewLabelsPanel.lines[4]=&crewLabels[4].display;
    if (MwReadNativeWord(data,0x233F)!=0x2376) return false;
    if (!ImportInitialString(data,0x237B,crewLabels[5].text,sizeof(crewLabels[5].text))) return false;
    ImportInitialText(data,0x2376,&crewLabels[5].display,crewLabels[5].text,0);
    crewLabelsPanel.lines[5]=&crewLabels[5].display;
    if (MwReadNativeWord(data,0x2341)!=0x2380) return false;
    if (!ImportInitialString(data,0x2385,crewLabels[6].text,sizeof(crewLabels[6].text))) return false;
    ImportInitialText(data,0x2380,&crewLabels[6].display,crewLabels[6].text,0);
    crewLabelsPanel.lines[6]=&crewLabels[6].display;
    barMenuPanel.x=MwReadNativeWord(data,0x8FF6);
    barMenuPanel.y=MwReadNativeWord(data,0x8FF8);
    barMenuPanel.width=MwReadNativeWord(data,0x8FFA);
    barMenuPanel.bottomInset=MwReadNativeWord(data,0x8FFC);
    barMenuPanel.lineCount=MwReadNativeWord(data,0x8FFE);
    barMenuPanel.borderEnabled=MwReadNativeWord(data,0x9000);
    barMenuPanel.lineSpacing=MwReadNativeWord(data,0x9002);
    barMenuPanel.background=MwReadNativeWord(data,0x9004);
    if (barMenuPanel.lineCount!=4) return false;
    if (MwReadNativeWord(data,0x9006)!=0x900E) return false;
    if (!ImportInitialString(data,0x9013,barMenuLabels[0].text,sizeof(barMenuLabels[0].text))) return false;
    ImportInitialText(data,0x900E,&barMenuLabels[0].display,barMenuLabels[0].text,0);
    barMenuPanel.lines[0]=&barMenuLabels[0].display;
    if (MwReadNativeWord(data,0x9008)!=0x901B) return false;
    if (!ImportInitialString(data,0x9020,barMenuLabels[1].text,sizeof(barMenuLabels[1].text))) return false;
    ImportInitialText(data,0x901B,&barMenuLabels[1].display,barMenuLabels[1].text,0);
    barMenuPanel.lines[1]=&barMenuLabels[1].display;
    if (MwReadNativeWord(data,0x900A)!=0x902C) return false;
    if (!ImportInitialString(data,0x9031,barMenuLabels[2].text,sizeof(barMenuLabels[2].text))) return false;
    ImportInitialText(data,0x902C,&barMenuLabels[2].display,barMenuLabels[2].text,0);
    barMenuPanel.lines[2]=&barMenuLabels[2].display;
    if (MwReadNativeWord(data,0x900C)!=0x903E) return false;
    if (!ImportInitialString(data,0x9043,barMenuLabels[3].text,sizeof(barMenuLabels[3].text))) return false;
    ImportInitialText(data,0x903E,&barMenuLabels[3].display,barMenuLabels[3].text,0);
    barMenuPanel.lines[3]=&barMenuLabels[3].display;
    recruitInterviewPanel.x=MwReadNativeWord(data,0x9076);
    recruitInterviewPanel.y=MwReadNativeWord(data,0x9078);
    recruitInterviewPanel.width=MwReadNativeWord(data,0x907A);
    recruitInterviewPanel.bottomInset=MwReadNativeWord(data,0x907C);
    recruitInterviewPanel.lineCount=MwReadNativeWord(data,0x907E);
    recruitInterviewPanel.borderEnabled=MwReadNativeWord(data,0x9080);
    recruitInterviewPanel.lineSpacing=MwReadNativeWord(data,0x9082);
    recruitInterviewPanel.background=MwReadNativeWord(data,0x9084);
    if (recruitInterviewPanel.lineCount!=3) return false;
    if (MwReadNativeWord(data,0x9086)!=0x908C) return false;
    if (!ImportInitialString(data,0x9091,recruitInterviewLabels[0].text,sizeof(recruitInterviewLabels[0].text))) return false;
    ImportInitialText(data,0x908C,&recruitInterviewLabels[0].display,recruitInterviewLabels[0].text,0);
    recruitInterviewPanel.lines[0]=&recruitInterviewLabels[0].display;
    if (MwReadNativeWord(data,0x9088)!=0x90D9) return false;
    if (!ImportInitialString(data,0x90DE,recruitInterviewLabels[1].text,sizeof(recruitInterviewLabels[1].text))) return false;
    ImportInitialText(data,0x90D9,&recruitInterviewLabels[1].display,recruitInterviewLabels[1].text,0);
    recruitInterviewPanel.lines[1]=&recruitInterviewLabels[1].display;
    if (MwReadNativeWord(data,0x908A)!=0x90E2) return false;
    if (!ImportInitialString(data,0x90E7,recruitInterviewLabels[2].text,sizeof(recruitInterviewLabels[2].text))) return false;
    ImportInitialText(data,0x90E2,&recruitInterviewLabels[2].display,recruitInterviewLabels[2].text,0);
    recruitInterviewPanel.lines[2]=&recruitInterviewLabels[2].display;
    for (uint16_t i=0;i<3;++i)
    {
        uint16_t offset=0x222E+i*CAMPAIGN_UI_RECORD_BYTES;
        MwUiRecord *record=&companyMenu.records[i];
        record->enabled=data[offset];record->textRecord=data[offset+1];
        record->x=MwReadNativeWord(data,offset+2);record->y=MwReadNativeWord(data,offset+4);
        record->activateCodeOffset=MwReadNativeWord(data,offset+6);
        record->upRecordIndex=data[offset+0x0A];record->downRecordIndex=data[offset+0x0B];
        record->leftRecordIndex=data[offset+0x0C];record->rightRecordIndex=data[offset+0x0D];
        if (MwReadNativeWord(data,offset+8)!=MwReadNativeWord(data,0x21E0+i*2)+4) return false;
        record->text=companyMenuLabels[i+1].text;record->textColour=&companyMenuLabels[i+1].display.colour;
    }
    for (uint16_t i=0;i<3;++i)
    {
        uint16_t offset=0x904C+i*CAMPAIGN_UI_RECORD_BYTES;
        MwUiRecord *record=&barMenu.records[i];
        record->enabled=data[offset];record->textRecord=data[offset+1];
        record->x=MwReadNativeWord(data,offset+2);record->y=MwReadNativeWord(data,offset+4);
        record->activateCodeOffset=MwReadNativeWord(data,offset+6);
        record->upRecordIndex=data[offset+0x0A];record->downRecordIndex=data[offset+0x0B];
        record->leftRecordIndex=data[offset+0x0C];record->rightRecordIndex=data[offset+0x0D];
        if (MwReadNativeWord(data,offset+8)!=MwReadNativeWord(data,0x9008+i*2)+4) return false;
        record->text=barMenuLabels[i+1].text;record->textColour=&barMenuLabels[i+1].display.colour;
    }
    for (uint16_t i=0;i<2;++i)
    {
        uint16_t offset=0x90EA+i*CAMPAIGN_UI_RECORD_BYTES;
        MwUiRecord *record=&recruitInterviewMenu.records[i];
        record->enabled=data[offset];record->textRecord=data[offset+1];
        record->x=MwReadNativeWord(data,offset+2);record->y=MwReadNativeWord(data,offset+4);
        record->activateCodeOffset=MwReadNativeWord(data,offset+6);
        record->upRecordIndex=data[offset+0x0A];record->downRecordIndex=data[offset+0x0B];
        record->leftRecordIndex=data[offset+0x0C];record->rightRecordIndex=data[offset+0x0D];
        if (MwReadNativeWord(data,offset+8)!=MwReadNativeWord(data,0x9088+i*2)+4) return false;
        record->text=recruitInterviewLabels[i+1].text;record->textColour=&recruitInterviewLabels[i+1].display.colour;
    }
    for (uint16_t i=0;i<5;++i)
    {
        uint16_t offset=0x238A+i*CAMPAIGN_UI_RECORD_BYTES;
        MwUiRecord *record=&crewMenu.records[i];
        record->enabled=data[offset];record->textRecord=data[offset+1];
        record->x=MwReadNativeWord(data,offset+2);record->y=MwReadNativeWord(data,offset+4);
        record->activateCodeOffset=MwReadNativeWord(data,offset+6);
        record->upRecordIndex=data[offset+0x0A];record->downRecordIndex=data[offset+0x0B];
        record->leftRecordIndex=data[offset+0x0C];record->rightRecordIndex=data[offset+0x0D];
        record->text=i?NULL:crewLabels[6].text;
        record->textColour=i?NULL:&crewLabels[6].display.colour;
        /*273E/28B2 assigns captions before any pilot record is selected. */
    }
    crewScreenRightEdge=MwReadNativeWord(data,0x2438);
    pilotCardRosterSlot=MwReadNativeWord(data,0x243A);
    pilotCardTextX=MwReadNativeWord(data,0x243C);
    displayedPilotId=MwReadNativeWord(data,0x243E);
    pilotCardGunneryTier=MwReadNativeWord(data,0x2440);
    pilotCardPilotingTier=MwReadNativeWord(data,0x2442);
    pilotCardPortraitId=MwReadNativeWord(data,0x2444);
    pilotCardWageTier=MwReadNativeWord(data,0x2446);
    pilotCardOwnedUnitSlot=MwReadNativeWord(data,0x2448);
    assignmentSelectedPilotSlot=MwReadNativeWord(data,0x2581);
    assignmentCandidateOwnedSlot=MwReadNativeWord(data,0x2583);
    assignmentConflictingPilotSlot=MwReadNativeWord(data,0x2586);
    assignmentConflictingOwnedSlot=MwReadNativeWord(data,0x2588);
    recruitCandidateByteOffset=MwReadNativeWord(data,0x911D);
    selectedRecruitPilotId=MwReadNativeWord(data,0x911F);
    assignmentConflictHighlighted=data[0x2585];
    ImportInitialWords(data,0x244A,pilotCardColumnOrigins,4);
    for (uint16_t i=0;i<4;++i)
    {
        if (!ImportInitialString(data,MwReadNativeWord(data,0x23D0+i*2),commanderReputationLabels[i],sizeof(commanderReputationLabels[i]))) return false;
        if (!ImportInitialString(data,MwReadNativeWord(data,0x2452+i*2),pilotSkillLabels[i],sizeof(pilotSkillLabels[i]))) return false;
    }
    const uint16_t attitudeOffsets[5]={0x240D,0x2414,0x241D,0x2425,0x242E};
    for (uint16_t i=0;i<5;++i)
        if (!ImportInitialString(data,attitudeOffsets[i],houseAttitudeLabels[i],sizeof(houseAttitudeLabels[i]))) return false;
    for (uint16_t i=0;i<9;++i)
    {
        uint16_t offset=MwReadNativeWord(data,0x23FB+i*2),label=0;
        while (label<5 && attitudeOffsets[label]!=offset) ++label;
        if (label==5) return false;
        houseAttitudeText[i]=houseAttitudeLabels[label];
    }
    for (uint16_t i=0;i<12;++i)
    {
        uint16_t offset=MwReadNativeWord(data,0x247E+i*2);
        pilotAssignmentCaptionColours[i]=data[offset];
        if (!ImportInitialString(data,offset+1,pilotAssignmentCaptions[i],sizeof(pilotAssignmentCaptions[i]))) return false;
    }
    pilotDefinitions[0]=NULL;pilotDialogue[0]=NULL;
    for (uint16_t pilotId=1;pilotId<CAMPAIGN_PILOT_ID_COUNT;++pilotId)
    {
        uint16_t offset=pilotDefinitionOffsets[pilotId];
        if (offset<PILOT_DEFINITIONS_START || (uint32_t)offset+sizeof(MwPilotDefinitionPrefix)>=PILOT_DEFINITIONS_END) return false;
        if (!memchr(data+offset+3,0,0x0B) || !memchr(data+offset+0x0E,0,PILOT_DEFINITIONS_END-offset-0x0E)) return false;
        pilotDefinitions[pilotId]=(const MwPilotDefinitionPrefix *)&pilotDefinitionStorage[offset-PILOT_DEFINITIONS_START];
        pilotDialogue[pilotId]=(const char *)(pilotDefinitions[pilotId]+1);
    }
    companyNavigationPadding=data[0x3020];
    if (!ImportInitialString(data,0x21C2,commanderProfileName,sizeof(commanderProfileName))) return false;
    if (!ImportInitialString(data,0x9106,recruitStatLabels,sizeof(recruitStatLabels))) return false;
    if (!ImportInitialString(data,0x9C39,noAvailableRecruitsText,sizeof(noAvailableRecruitsText))) return false;
    if (!ImportInitialString(data,0x9C5A,fullFourPilotRosterText,sizeof(fullFourPilotRosterText))) return false;
    if (!ImportInitialString(data,0x9C8D,fullThreePilotRosterText,sizeof(fullThreePilotRosterText))) return false;
    return ImportContractData(data);
}

MwPanelDescriptor contractOfficePanel,contractActionsPanel;
MwServiceCaption contractOfficeLabels[2],contractActionLabels[3];
static MwUiRecord contractOfficeRecords[2],contractActionRecords[6];
static uint8_t contractOfficeNavigationProbe,contractActionsNavigationProbe;
MwUiMenu contractOfficeMenu={contractOfficeRecords,2,&contractOfficeNavigationProbe};
MwUiMenu contractActionsMenu={contractActionRecords,6,&contractActionsNavigationProbe};
uint16_t negotiatingMissionOfferSlot,negotiatingMissionOfferByteOffset;
uint8_t contractRequestEditorInitialized,contractRequestTextColour;
uint16_t contractRequestTermByteOffset,contractNegotiationAttempts;
uint8_t negotiationRequestsSatisfied;
uint16_t contractDateYearXOffsets[CAMPAIGN_MONTH_COUNT];
uint16_t requestedTermX[3],requestedTermY[3],requestedTermDigits[3];
char contractNarrative[0x013C],houseContractNames[CAMPAIGN_HOUSE_COUNT][8];
char submitContractCaption[6],acceptContractCaption[6];
static uint8_t contractOfficePlanetGroupStorage[0x66];
const uint8_t *contractOfficePlanetGroups[CAMPAIGN_HOUSE_COUNT];
uint16_t houseExtraEnemyRollThresholds[CAMPAIGN_HOUSE_COUNT];
char noContractsText[0x2B],negotiationTerminatedText[0x47];
char contractAcceptedText[0x1B],houseContractRefusalText[0x5F];
uint16_t acceptedContractAdvanceCbillsLow,acceptedContractAdvanceCbillsHigh;
uint8_t deploymentRefusalCount,deploymentPenaltySuppressed;
char commanderNeedsMechText[0x37],assignedMechUnreadyText[0x3A],contractCancelledText[0x4F];

/* Actual EXE import boundary: no runtime segment registry or copied live DS. */
static bool ImportContractData(const uint8_t *data)
{
    negotiatingMissionOfferSlot=MwReadNativeWord(data,0x488B);
    negotiatingMissionOfferByteOffset=MwReadNativeWord(data,0x488E);
    contractRequestEditorInitialized=data[0x488D];
    contractRequestTextColour=data[0x4B34];
    contractRequestTermByteOffset=MwReadNativeWord(data,0x4B32);
    contractNegotiationAttempts=MwReadNativeWord(data,0x4B7E);
    negotiationRequestsSatisfied=data[0x4B7D];
    acceptedContractAdvanceCbillsLow=MwReadNativeWord(data,0x4B73);
    acceptedContractAdvanceCbillsHigh=MwReadNativeWord(data,0x4B75);
    deploymentRefusalCount=data[0x0FBF];deploymentPenaltySuppressed=data[0x140B];
    ImportInitialWords(data,0x4B1A,contractDateYearXOffsets,CAMPAIGN_MONTH_COUNT);
    ImportInitialWords(data,0x49CC,requestedTermX,3);
    ImportInitialWords(data,0x49D2,requestedTermY,3);
    ImportInitialWords(data,0x49D8,requestedTermDigits,3);
    ImportInitialWords(data,0x4BF0,houseExtraEnemyRollThresholds,CAMPAIGN_HOUSE_COUNT);
    memcpy(submitContractCaption,data+0x4B35,6);memcpy(acceptContractCaption,data+0x4B3B,6);
    memcpy(contractOfficePlanetGroupStorage,data+0x4B8A,sizeof(contractOfficePlanetGroupStorage));
    for (uint16_t house=0;house<CAMPAIGN_HOUSE_COUNT;++house)
    {
        if (!ImportInitialString(data,MwReadNativeWord(data,0x4A69+house*2),houseContractNames[house],8)) return false;
        uint16_t group=MwReadNativeWord(data,0x4B80+house*2);
        if (group<0x4B8A || group>=0x4BF0 || !memchr(data+group,CONTRACT_REGION_END,0x4BF0-group)) return false;
        contractOfficePlanetGroups[house]=contractOfficePlanetGroupStorage+group-0x4B8A;
    }
    const uint16_t panels[]={0x4822,0x49DE},counts[]={2,3},menus[]={0x486F,0x4A15},menuCounts[]={2,6};
    MwPanelDescriptor *panelOwners[]={&contractOfficePanel,&contractActionsPanel};
    MwServiceCaption *labels[]={contractOfficeLabels,contractActionLabels};
    MwUiMenu *menuOwners[]={&contractOfficeMenu,&contractActionsMenu};
    for (uint16_t group=0;group<2;++group)
    {
        MwPanelDescriptor *panel=panelOwners[group];
        uint16_t header[8];ImportInitialWords(data,panels[group],header,8);
        panel->x=header[0];panel->y=header[1];panel->width=header[2];panel->bottomInset=header[3];
        panel->lineCount=header[4];panel->borderEnabled=header[5];panel->lineSpacing=header[6];panel->background=header[7];
        if (panel->lineCount!=counts[group]) return false;
        for (uint16_t row=0;row<counts[group];++row)
        {
            uint16_t offset=MwReadNativeWord(data,panels[group]+0x10+row*2);
            MwServiceCaption *caption=&labels[group][row];
            if (!ImportInitialString(data,offset+5,caption->text,sizeof(caption->text))) return false;
            ImportInitialText(data,offset,&caption->display,caption->text,0);
            panel->lines[row]=&caption->display;
        }
        for (uint16_t row=0;row<menuCounts[group];++row)
        {
            const uint8_t *encoded=data+menus[group]+row*CAMPAIGN_UI_RECORD_BYTES;
            MwUiRecord *record=&menuOwners[group]->records[row];
            record->enabled=encoded[0];record->textRecord=encoded[1];
            record->x=MwReadNativeWord(encoded,2);record->y=MwReadNativeWord(encoded,4);
            record->activateCodeOffset=MwReadNativeWord(encoded,6);record->pictureIndex=MwReadNativeWord(encoded,8);
            record->upRecordIndex=encoded[10];record->downRecordIndex=encoded[11];
            record->leftRecordIndex=encoded[12];record->rightRecordIndex=encoded[13];
            if (record->textRecord)
            {
                if (row>=counts[group] || record->pictureIndex!=MwReadNativeWord(data,panels[group]+0x10+row*2)+4) return false;
                record->text=labels[group][row].text;record->textColour=&labels[group][row].display.colour;
            }
        }
    }
    contractOfficeNavigationProbe=data[0x5661];contractActionsNavigationProbe=data[0x5807];
    return ImportInitialString(data,0x4890,contractNarrative,sizeof(contractNarrative))
        && ImportInitialString(data,0x4BFA,noContractsText,sizeof(noContractsText))
        && ImportInitialString(data,0x4C25,negotiationTerminatedText,sizeof(negotiationTerminatedText))
        && ImportInitialString(data,0x4C6C,contractAcceptedText,sizeof(contractAcceptedText))
        && ImportInitialString(data,0x4C87,houseContractRefusalText,sizeof(houseContractRefusalText))
        && ImportInitialString(data,0x0FC0,commanderNeedsMechText,sizeof(commanderNeedsMechText))
        && ImportInitialString(data,0x0FF7,assignedMechUnreadyText,sizeof(assignedMechUnreadyText))
        && ImportInitialString(data,0x1031,contractCancelledText,sizeof(contractCancelledText));
}

/* Installation-only near-directory decoding. Shared entries retain ONE text
 * owner; normal campaign code follows C references, not native address tables. */
bool MwImportCampaignHandoffData(const uint8_t *data,size_t byteCount)
{
    if (!data || byteCount<NATIVE_SEGMENT_BYTE_COUNT) return false;
    static const uint16_t failureOffsets[CAMPAIGN_HOUSE_COUNT]={0x149B,0x159B,0x159B,0x1528,0x159B};
    static const uint16_t successOffsets[CAMPAIGN_HOUSE_COUNT]={0x1674,0x1742,0x1742,0x16D6,0x1742};
    static const uint16_t deductionOffsets[3]={0x178C,0x1834,0x18E0};
    for (uint16_t house=0;house<CAMPAIGN_HOUSE_COUNT;++house)
        if (MwReadNativeWord(data,0x1491+house*2)!=failureOffsets[house]
            || MwReadNativeWord(data,0x166A+house*2)!=successOffsets[house]) return false;
    for (uint16_t message=0;message<3;++message)
        if (MwReadNativeWord(data,0x1786+message*2)!=deductionOffsets[message]) return false;
    return true;
}

/* Native reserve-ammunition shop state and shared caption/menu owners. */
uint16_t reserveAmmoShopSelectedRow,reserveAmmoPreviewSelection;
uint8_t reserveAmmoSelling,reserveAmmoMechColours[11];
char reserveAmmoBuyLabel[5],reserveAmmoSellLabel[5];
MwServiceCaption reserveAmmoShopHeading,reserveAmmoFamilyLabels[6],reserveAmmoDoneLabel;
MwReserveAmmoQuantityRow reserveAmmoQuantityRows[6];
MwPanelDescriptor reserveAmmoShopPanel,reserveAmmoCompatibilityPanels[6];
MwServiceCaption reserveAmmoCompatibilityHeadings[6];
uint16_t reserveAmmoCompatibleMechs[6][6];
MwUiRecord reserveAmmoShopRecords[RESERVE_AMMO_MENU_ROWS];
/* Native FF-neighbour probe is 1090:505A, the existing travel table WORD. */
MwUiMenu reserveAmmoShopMenu={reserveAmmoShopRecords,RESERVE_AMMO_MENU_ROWS,NULL,&travelSquareRootValues[0x1BA],0};

static bool ImportReserveAmmoUiData(const uint8_t *data)
{
    reserveAmmoShopSelectedRow=MwReadNativeWord(data,0x4137);
    reserveAmmoPreviewSelection=MwReadNativeWord(data,0x3F6E);
    reserveAmmoSelling=data[0x4139];memcpy(reserveAmmoMechColours,data+0x3F70,sizeof(reserveAmmoMechColours));
    if (!ImportInitialString(data,0x413A,reserveAmmoBuyLabel,sizeof(reserveAmmoBuyLabel))
        || !ImportInitialString(data,0x413F,reserveAmmoSellLabel,sizeof(reserveAmmoSellLabel))) return false;
    for (uint16_t panelIndex=0;panelIndex<=CAMPAIGN_AMMO_FAMILY_COUNT;++panelIndex)
    {
        bool shop=panelIndex==CAMPAIGN_AMMO_FAMILY_COUNT;
        uint16_t offset=shop?0x4144:MwReadNativeWord(data,0x3F7B+panelIndex*2);
        MwPanelDescriptor *panel=shop?&reserveAmmoShopPanel:&reserveAmmoCompatibilityPanels[panelIndex];
        panel->x=MwReadNativeWord(data,offset);panel->y=MwReadNativeWord(data,offset+2);
        panel->width=MwReadNativeWord(data,offset+4);panel->bottomInset=MwReadNativeWord(data,offset+6);
        panel->lineCount=MwReadNativeWord(data,offset+8);panel->borderEnabled=MwReadNativeWord(data,offset+10);
        panel->lineSpacing=MwReadNativeWord(data,offset+12);panel->background=MwReadNativeWord(data,offset+14);
        if (panel->lineCount!=(shop?14:1)) return false;
        for (uint16_t line=0;line<panel->lineCount;++line)
        {
            uint16_t caption=MwReadNativeWord(data,offset+16+line*2);
            MwPanelText *display;char *text;size_t capacity;
            if (!shop)
            {
                display=&reserveAmmoCompatibilityHeadings[panelIndex].display;
                text=reserveAmmoCompatibilityHeadings[panelIndex].text;capacity=sizeof(reserveAmmoCompatibilityHeadings[panelIndex].text);
            }
            else if (line>=7 && line<=12)
            {
                display=&reserveAmmoQuantityRows[line-7].display;
                text=reserveAmmoQuantityRows[line-7].text;capacity=sizeof(reserveAmmoQuantityRows[line-7].text);
            }
            else
            {
                MwServiceCaption *label=line==0?&reserveAmmoShopHeading:
                    line==13?&reserveAmmoDoneLabel:&reserveAmmoFamilyLabels[line-1];
                display=&label->display;text=label->text;capacity=sizeof(label->text);
            }
            if (!ImportInitialString(data,caption+5,text,capacity)) return false;
            ImportInitialText(data,caption,display,text,0);panel->lines[line]=display;
        }
        if (shop) continue;
        uint16_t kinds=MwReadNativeWord(data,0x3F87+panelIndex*2),index=0;
        do
        {
            if (index>=6) return false;
            uint16_t kind=MwReadNativeWord(data,kinds+index*2);
            reserveAmmoCompatibleMechs[panelIndex][index++]=kind;
            if (kind==UINT16_MAX) break;
            if (kind>=CAMPAIGN_MECH_KIND_COUNT) return false;
        } while (true);
    }
    for (uint16_t index=0;index<RESERVE_AMMO_MENU_ROWS;++index)
    {
        const uint8_t *encoded=data+0x4268+index*CAMPAIGN_UI_RECORD_BYTES;
        MwUiRecord *record=&reserveAmmoShopRecords[index];
        record->enabled=encoded[0];record->textRecord=encoded[1];
        record->x=MwReadNativeWord(encoded,2);record->y=MwReadNativeWord(encoded,4);
        record->activateCodeOffset=MwReadNativeWord(encoded,6);record->pictureIndex=0;
        MwPanelText *caption=index<6?&reserveAmmoFamilyLabels[index].display:
            index<12?&reserveAmmoQuantityRows[index-6].display:&reserveAmmoDoneLabel.display;
        record->text=caption->text;record->textColour=&caption->colour;
        record->upRecordIndex=encoded[10];record->downRecordIndex=encoded[11];
        record->leftRecordIndex=encoded[12];record->rightRecordIndex=encoded[13];
    }
    return true;
}
