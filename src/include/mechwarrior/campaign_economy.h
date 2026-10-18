#ifndef MECHWARRIOR_CAMPAIGN_ECONOMY_H
#define MECHWARRIOR_CAMPAIGN_ECONOMY_H

#include "mechwarrior/campaign_mechanics.h"

enum
{
    CAMPAIGN_AMMO_GROUPS = 0x0003, STOCK_KIND_RANGE_COUNT = 0x0003,
    AMMO_PROFILE_ENTRY_COUNT = 0x000F, AMMO_PROFILE_START = 0x2F3C,
    STOCK_BODY_ROLL_MAXIMUM = 0x0005, STOCK_BODY_MINIMUM_ROLLS = 0x0002,
    STOCK_BODY_TIER_MAXIMUM = 0x0003, STOCK_COMPONENT_TIER_THREE = 0x03,
    STOCK_FORCE_STARTER_MECH = 0x01, CAMPAIGN_VACANT_UNIT_KIND = 0xFFFF,
    DEALER_PURCHASE_ROLL_BASE = 0x0004, STOCK_RANGE_ROLL_MAXIMUM = 0x0064,
    STOCK_FIRST_RANGE_THRESHOLD = 0x003C, STOCK_SECOND_RANGE_THRESHOLD = 0x005A,
    RECRUIT_OFFER_SLOTS = 0x0008, RECRUIT_COUNT_ROLL_BASE = 0x0009,
    RECRUIT_SEARCH_STATE_LIMIT = 0x0064, PILOT_PORTRAIT_PAIR_MASK = 0xFFFE
};

enum
{
    MISSION_OFFER_SLOTS = 0x0006, MISSION_TARGET_COLUMNS = 0x0008,
    MISSION_CATEGORY_COUNT = 0x0005, MISSION_DESCRIPTION_LIST_WORDS = 0x0022,
    MISSION_DESCRIPTION_LIST_START = 0x8A25, MISSION_DESCRIPTION_STORAGE_START = 0x8A69,
    MISSION_DESCRIPTION_STORAGE_END = 0x8E46, HOUSE_PLANET_POLICIES_START = 0x8E9A,
    HOUSE_PLANET_CHOICES = 0x0008, HOUSE_GRIEVANCE_REFUSAL = 0x0008,
    COMBAT_CREDIT_SECOND_BAND = 0x0009, COMBAT_CREDIT_THIRD_BAND = 0x0015,
    COMBAT_CREDIT_FOURTH_BAND = 0x0024, MISSION_FORCE_BUDGET_SCALE = 0x000A,
    MISSION_HEAVY_FORCE_COST = 0x0019, MISSION_MEDIUM_FORCE_COST = 0x000F,
    MISSION_LIGHT_FORCE_COST = 0x0007, MISSION_ENEMY_COUNT_LIMIT = 0x0004,
    MISSION_MIX_ROLL_MAXIMUM = 0x000A, MISSION_MIX_FIRST_THRESHOLD = 0x0005,
    MISSION_MIX_SECOND_THRESHOLD = 0x0008, MISSION_PAYMENT_ROLL_MAXIMUM = 0x0009,
    MISSION_PAYMENT_MINIMUM_THOUSANDS = 0x0064, MISSION_PAYMENT_MAXIMUM_THOUSANDS = 0x2706,
    MISSION_PAYMENT_ROUNDING_THOUSANDS = 0x000A, MISSION_RELATIONSHIP_LIMIT = 0x000A,
    MISSION_TARGET_STEP_ROLL_MAXIMUM = 0x000A, MISSION_STORY_ROLL_MAXIMUM = 0x0064,
    MISSION_CATEGORY_DEFENCE = 0x0001, MISSION_CATEGORY_RESCUE_RETRIEVAL,
    MISSION_CATEGORY_RAID, MISSION_CATEGORY_MAJOR_CAMPAIGN,
    CAMPAIGN_SCENARIO_EXTENDED_OFFENSIVE = 0x000F, CAMPAIGN_SCENARIO_EXTENDED_DEFENSIVE = 0x0010,
    CAMPAIGN_SCENARIO_EXTENDED_SIEGE = 0x0012, MISSION_PLANET_SELECTOR_NONE = 0xFF
};
enum
{
    MAJOR_CAMPAIGN_MIDDLE_YEAR = 0x0BD4, MAJOR_CAMPAIGN_LATE_YEAR = 0x0BD6,
    MAJOR_CAMPAIGN_START_MONTH = 0x0007, MAJOR_CAMPAIGN_START_SUBMONTH_UNIT = 0x0028,
    MAJOR_CAMPAIGN_MIDDLE_THRESHOLD = 0x32, MAJOR_CAMPAIGN_LATE_THRESHOLD = 0x4B,
    MISSION_OFFER_SKIP_ROLL_LIMIT = 0x0003, MISSION_OFFER_NORMAL_ROLL_MAXIMUM = 0x0004,
    MISSION_OFFER_NORMAL_MINIMUM = 0x0002, MISSION_OFFER_MIDDLE_ROLL_MAXIMUM = 0x0003,
    MISSION_OFFER_MIDDLE_MINIMUM = 0x0003
};
enum
{
    CAMPAIGN_UNITS_PER_BILLING_MONTH = 0x003C, MARKET_REFRESH_PERIOD_UNITS = 0x000E,
    DROPSHIP_MONTHLY_UPKEEP_CBILLS = 0x1388, DROPSHIP_UPKEEP_DEPARTURE_COUNT = 0x0002,
    PILOT_AVAILABILITY_NATIVE_BASE = 0x04BE, PILOT_DEPARTURE_NATIVE_BASE = 0x04BF,
    PILOT_DEPARTURE_TAIL_OFFSET = 0x04E9
};
enum CampaignNotification
{
    NOTIFICATION_PILOTS_DEPARTED, NOTIFICATION_DROPSHIP_CREW_DEPARTED,
    NOTIFICATION_UNPAID_PILOT_WAGES, NOTIFICATION_UNPAID_DROPSHIP_UPKEEP,
    CAMPAIGN_NOTIFICATION_COUNT
};

typedef struct CampaignMissionDescriptionHeader
{
    uint8_t scenarioCode, unidentifiedField2;
} CampaignMissionDescriptionHeader;
typedef struct MwHouseMissionPlanetPolicy
{
    uint8_t planetSelectorsByTarget[MISSION_TARGET_COLUMNS][HOUSE_PLANET_CHOICES];
} MwHouseMissionPlanetPolicy;
_Static_assert(sizeof(CampaignMissionDescriptionHeader) == 0x0002, "Native description prefix");
_Static_assert(sizeof(MwHouseMissionPlanetPolicy) == 0x0040, "Native House destination policy");

typedef struct CampaignAmmoProfileEntry
{
    uint16_t ammoFamily, quantityMaximum;
} CampaignAmmoProfileEntry;
_Static_assert(sizeof(CampaignAmmoProfileEntry) == 0x0004, "Native ammunition profile pair");
typedef struct CampaignDropShipConditionPrefix
{
    uint8_t deterioration;
} CampaignDropShipConditionPrefix;
_Static_assert(sizeof(CampaignDropShipConditionPrefix) == 1, "DropShip view of first condition byte");

extern uint8_t generatedKindRangeMaxima[STOCK_KIND_RANGE_COUNT];
extern uint8_t generatedKindRangeStarts[STOCK_KIND_RANGE_COUNT];
extern uint16_t generatedDamageMaximumIndex, generatedUnitKind;
extern uint16_t generatedAmmoKinds[CAMPAIGN_AMMO_GROUPS];
extern uint16_t generatedAmmoQuantities[CAMPAIGN_AMMO_GROUPS];
extern uint8_t generateStartingJenner;
extern uint8_t unitAmmoGroupCounts[CAMPAIGN_MECH_KIND_COUNT];
extern CampaignAmmoProfileEntry *unitAmmoProfiles[CAMPAIGN_MECH_KIND_COUNT];
extern CampaignAmmoProfileEntry unitAmmoProfileEntries[AMMO_PROFILE_ENTRY_COUNT];
extern uint8_t unitJumpJetGroupCounts[CAMPAIGN_MECH_KIND_COUNT];
extern uint16_t dealerUnitPurchaseLimit, unitsSoldSinceMarketRefresh;
extern uint16_t marketUnitCount, marketUnitKinds[CAMPAIGN_MARKET_UNIT_SLOTS];
extern uint16_t marketAmmoKindsByGroup[CAMPAIGN_AMMO_GROUPS * CAMPAIGN_MARKET_UNIT_SLOTS];
extern uint16_t marketAmmoQuantitiesByGroup[CAMPAIGN_AMMO_GROUPS * CAMPAIGN_MARKET_UNIT_SLOTS];
extern uint16_t availableRecruitCount, availableRecruitIds[RECRUIT_OFFER_SLOTS];
extern uint16_t ownedUnitCount, ownedUnitKinds[CAMPAIGN_OWNED_UNIT_SLOTS];
extern uint16_t ownedAmmoKindsByGroup[CAMPAIGN_AMMO_GROUPS * CAMPAIGN_OWNED_UNIT_SLOTS];
extern uint16_t ownedAmmoQuantitiesByGroup[CAMPAIGN_AMMO_GROUPS * CAMPAIGN_OWNED_UNIT_SLOTS];
extern uint16_t houseFavorPoints[CAMPAIGN_HOUSE_COUNT], houseGrievancePoints[CAMPAIGN_HOUSE_COUNT];
extern uint16_t playerCombatCredit, generatingMissionOfferSlot;
extern uint16_t missionForceBudget, missionPaymentBaseThousands;
extern uint8_t missionForceWeightByUnitKind[CAMPAIGN_MECH_KIND_COUNT];
extern uint16_t housePaymentRollBias[CAMPAIGN_HOUSE_COUNT];
extern int16_t housePaymentAdjustmentPercent[CAMPAIGN_HOUSE_COUNT];
extern uint16_t houseSalvageBasePercent[CAMPAIGN_HOUSE_COUNT], houseAdvanceBasePercent[CAMPAIGN_HOUSE_COUNT];
extern uint8_t houseMissionTypeLimits[CAMPAIGN_HOUSE_COUNT * MISSION_TARGET_COLUMNS];
extern uint8_t majorCampaignMissionRollThreshold;
extern uint16_t missionDescriptionOffsets[MISSION_OFFER_SLOTS], missionPlanetRecordIndices[MISSION_OFFER_SLOTS];
extern uint16_t missionTargetHouseIndices[MISSION_OFFER_SLOTS], missionTypeCodes[MISSION_OFFER_SLOTS];
extern uint16_t missionEnemyHeavyCounts[MISSION_OFFER_SLOTS], missionEnemyMediumCounts[MISSION_OFFER_SLOTS];
extern uint16_t missionEnemyLightCounts[MISSION_OFFER_SLOTS], missionPaymentThousands[MISSION_OFFER_SLOTS];
extern uint16_t missionSalvagePercentages[MISSION_OFFER_SLOTS], missionAdvancePercentages[MISSION_OFFER_SLOTS];
extern uint16_t missionPaymentCeilingsThousands[MISSION_OFFER_SLOTS], missionSalvagePercentageCeilings[MISSION_OFFER_SLOTS];
extern uint16_t missionAdvancePercentageCeilings[MISSION_OFFER_SLOTS], missionScenarioCodes[MISSION_OFFER_SLOTS];
extern uint8_t missionDescriptionMaximumIndicesByCategory[MISSION_CATEGORY_COUNT];
extern uint16_t missionDescriptionListOffsetsByCategory[MISSION_CATEGORY_COUNT];
extern uint16_t missionDescriptionListEntries[MISSION_DESCRIPTION_LIST_WORDS];
extern uint8_t missionDescriptionStorage[MISSION_DESCRIPTION_STORAGE_END - MISSION_DESCRIPTION_STORAGE_START];
extern uint16_t houseMissionPlanetTableOffsets[CAMPAIGN_HOUSE_COUNT];
extern MwHouseMissionPlanetPolicy houseMissionPlanetPolicies[CAMPAIGN_HOUSE_COUNT];
extern uint16_t calendarSubmonthUnits, missionOfferCount;
extern uint16_t marketRefreshUnits, elapsedBillingMonths;
extern uint16_t campaignFundsLow, campaignFundsHigh;
extern uint16_t dropShipMissedUpkeepPayments, unpaidPilotWageMarker;
extern uint16_t pilotWageAmounts[CAMPAIGN_ROSTER_SLOTS];
extern uint8_t pilotDepartureFlagTail;
extern uint8_t notificationPending[CAMPAIGN_NOTIFICATION_COUNT];

void MwGenerateUnitStock(uint16_t stockKindRangeIndex, uint16_t tierThreeRolls,
    uint16_t tierTwoRolls, uint16_t tierOneRolls);
void MwRefreshMarketUnitStock(void);
void MwRefreshAvailablePilotRecruits(void);
bool MwGenerateMissionOffer(uint16_t missionOfferSlot);
void MwRefreshMissionOffers(void);
void MwAdvanceDropShipConditionDeterioration(void);
void MwAdvanceCampaignTimeAndChargeWages(uint16_t elapsedUnits);

#endif
