#ifndef MECHWARRIOR_CAMPAIGN_MECHANICS_H
#define MECHWARRIOR_CAMPAIGN_MECHANICS_H

#include "mechwarrior/campaign.h"

enum CampaignUnitKind
{
    CAMPAIGN_UNIT_LOCUST, CAMPAIGN_UNIT_WASP, CAMPAIGN_UNIT_JENNER,
    CAMPAIGN_UNIT_PHOENIX_HAWK, CAMPAIGN_UNIT_SHADOW_HAWK, CAMPAIGN_UNIT_WOLVERINE,
    CAMPAIGN_UNIT_RIFLEMAN, CAMPAIGN_UNIT_WARHAMMER, CAMPAIGN_UNIT_MARAUDER,
    CAMPAIGN_UNIT_BATTLEMASTER, CAMPAIGN_UNIT_DROPSHIP, CAMPAIGN_UNIT_KIND_COUNT
};
enum CampaignRepairCategory
{
    REPAIR_ENGINE, REPAIR_GYRO, REPAIR_SENSORS, REPAIR_LIFE_SUPPORT,
    REPAIR_HEAT_SINKS, REPAIR_LEFT_ARM_ACTUATORS, REPAIR_RIGHT_ARM_ACTUATORS,
    REPAIR_LEFT_LEG_ACTUATORS, REPAIR_RIGHT_LEG_ACTUATORS, REPAIR_JUMP_JETS,
    REPAIR_BODY, REPAIR_FIRST_WEAPON
};
enum CampaignPilotSkillTier
{
    PILOT_SKILL_POOR, PILOT_SKILL_AVERAGE, PILOT_SKILL_GOOD, PILOT_SKILL_EXCELLENT
};
enum
{
    CAMPAIGN_OWNED_UNIT_SLOTS = 0x000C, CAMPAIGN_MARKET_UNIT_SLOTS = 0x000C,
    CAMPAIGN_MECH_KIND_COUNT = 0x000A, CAMPAIGN_SYSTEM_COUNT = 0x000A,
    CAMPAIGN_WEAPON_SLOTS = 0x000A, CAMPAIGN_BODY_LOCATION_COUNT = 0x0009,
    CAMPAIGN_DAMAGE_BYTES = 0x001D, CAMPAIGN_BODY_DAMAGE_OFFSET = 0x0014,
    OWNED_UNIT_DAMAGE_START = 0x054A, MARKET_UNIT_DAMAGE_START = 0x07EC,
    GENERATED_UNIT_DAMAGE_START = 0x3A41,
    REPAIR_WEAPON_PRICES_START = 0x3C2F, REPAIR_WEAPON_PRICE_WORD_COUNT = 0x003E,
    UNIT_PRICE_TIER_COUNT = 0x0004, CBILLS_PER_THOUSAND = 0x03E8,
    UNIT_SALE_ROUNDING_CBILLS = 0x01F4, REPAIR_COMPONENT_CBILLS = 0x014D,
    CAMPAIGN_ROSTER_SLOTS = 0x0004, CAMPAIGN_PILOT_ID_COUNT = 0x002B,
    CAMPAIGN_NO_MECH_ASSIGNMENT = 0xFFFF, PILOT_STATUS_RELEASE_AVAILABILITY = 0x02,
    PILOT_POOR_ADVANCE_COUNT = 0x03, PILOT_AVERAGE_ADVANCE_COUNT = 0x0A,
    PILOT_GOOD_ADVANCE_COUNT = 0x0F,
    NATIVE_BYTE_SIGN_BIT = 0x80,
    PILOT_DEFINITIONS_START = 0x9177, PILOT_DEFINITIONS_END = 0x9C39,
    TRAVEL_ROOT_TABLE_START = 0x4CE6, TRAVEL_ROOT_TABLE_COUNT = 0x03E8,
    TRAVEL_MAP_Y_SCALE_NUMERATOR = 0x0020, TRAVEL_MAP_Y_SCALE_DENOMINATOR = 0x0014,
    TRAVEL_DISTANCE_DIVISOR = 0x0012, TRAVEL_MINIMUM_DISTANCE = 0x0001,
    TRAVEL_ROOT_FINE_BOUNDARY = 0x0064, TRAVEL_ROOT_MEDIUM_BOUNDARY = 0x2710,
    TRAVEL_ROOT_MILLION = 0x000F4240, TRAVEL_ROOT_HUNDRED_MILLION = 0x05F5E100,
    TRAVEL_ROOT_SATURATION_HIGH = 0xFFFE, TRAVEL_ROOT_LARGE_DIVISOR = 0xFFFF,
    TRAVEL_ROOT_LARGE_SECOND_DIVISOR = 0x0099, CAMPAIGN_AMMO_FAMILY_COUNT = 0x0006,
    TRAVEL_PLANET_DIRECTORY_COUNT = 0x0091, TRAVEL_PLANET_NAME_CAPACITY = 0x0018,
    TRAVEL_PLANET_ROW_BYTES = 0x0017,
    TRAVEL_HOUSE_COUNT = 0x0005, TRAVEL_PLANET_ROWS_PER_HOUSE = 0x001E,
    HIRED_TRANSPORT_FRAME_COUNT = 0x0022, DROPSHIP_TRAVEL_FRAME_COUNT = 0x0016
};

typedef struct CampaignUnitDamage
{
    uint8_t engineHits, gyroDamageTier, sensorDamageTier;
    uint8_t lifeSupportDamageTier, destroyedHeatSinks;
    uint8_t actuatorDamageTiers[0x0004];
    uint8_t destroyedJumpJetGroups;
    uint8_t weaponDamageTiers[CAMPAIGN_WEAPON_SLOTS];
    uint8_t bodyDamageTiers[CAMPAIGN_BODY_LOCATION_COUNT];
} CampaignUnitDamage;
_Static_assert(sizeof(CampaignUnitDamage) == CAMPAIGN_DAMAGE_BYTES, "Native campaign damage record");
_Static_assert(offsetof(CampaignUnitDamage, bodyDamageTiers) == CAMPAIGN_BODY_DAMAGE_OFFSET, "Native body tiers");

typedef struct CampaignSystemRepairPriceRow
{
    uint16_t basePriceByMechKind[CAMPAIGN_MECH_KIND_COUNT];
} CampaignSystemRepairPriceRow;
_Static_assert(sizeof(CampaignSystemRepairPriceRow) == 0x0014, "Native system repair row");

typedef struct MwPilotDefinitionPrefix
{
    unsigned char portraitId, initialGunneryTier, initialPilotingTier;
    unsigned char nameBytes[0x000B];
} MwPilotDefinitionPrefix;
_Static_assert(sizeof(MwPilotDefinitionPrefix) == 0x000E, "Native pilot prefix");
_Static_assert(_Alignof(MwPilotDefinitionPrefix) == 1, "Odd pilot prefix offsets");

/* 04BE is BOTH roster ID3's high byte and availability[0]. One canonical
 * owner; releasing definition0 must be visible to the following roster copy. */
typedef union CampaignPilotIdentityStorage
{
    struct
    {
        uint16_t count;
        uint16_t pilotIds[CAMPAIGN_ROSTER_SLOTS];
        uint8_t unavailablePilots1To42[CAMPAIGN_PILOT_ID_COUNT - 1];
    } roster;
    struct
    {
        unsigned char precedingRosterBytes[0x0009];
        uint8_t flags[CAMPAIGN_PILOT_ID_COUNT];
    } availability;
} CampaignPilotIdentityStorage;
_Static_assert(offsetof(CampaignPilotIdentityStorage, roster.pilotIds) == 0x0002, "Roster IDs at04B7");
_Static_assert(offsetof(CampaignPilotIdentityStorage, availability.flags) == 0x0009, "Availability alias at04BE");
_Static_assert(sizeof(CampaignPilotIdentityStorage) == 0x0034, "Canonical04B5..04E8");
extern CampaignPilotIdentityStorage pilotIdentityStorage;
#define pilotRosterCount pilotIdentityStorage.roster.count
#define rosterPilotIds pilotIdentityStorage.roster.pilotIds
#define pilotUnavailableFlags pilotIdentityStorage.availability.flags

extern CampaignUnitDamage ownedUnitDamage[CAMPAIGN_OWNED_UNIT_SLOTS];
extern CampaignUnitDamage marketUnitDamage[CAMPAIGN_MARKET_UNIT_SLOTS];
extern CampaignUnitDamage generatedMechDamage;
extern uint8_t unitWeaponCounts[CAMPAIGN_MECH_KIND_COUNT];
extern uint8_t unitPriceTierMultipliers[UNIT_PRICE_TIER_COUNT];
extern uint16_t unitBasePrices[CAMPAIGN_UNIT_KIND_COUNT];
extern uint16_t unitTransactionPriceLow, unitTransactionPriceHigh;
extern uint16_t selectedOwnedUnitKind, selectedMarketUnitKind;
extern uint16_t unitRepairQuoteLow, unitRepairQuoteHigh;
extern uint16_t repairQuoteMechKind;
extern CampaignUnitDamage *repairQuoteDamage;
extern uint16_t componentRepairPriceLow, componentRepairPriceHigh;
extern uint16_t repairPriceCategory;
extern CampaignUnitDamage *componentRepairDamage;
extern CampaignSystemRepairPriceRow systemRepairBasePrices[CAMPAIGN_SYSTEM_COUNT];
extern uint16_t *weaponRepairPricesByKind[CAMPAIGN_MECH_KIND_COUNT];
extern uint16_t weaponRepairPrices[REPAIR_WEAPON_PRICE_WORD_COUNT];
extern uint8_t bodyRepairBasePricesByKind[CAMPAIGN_MECH_KIND_COUNT];
extern uint8_t rosterStatus[CAMPAIGN_ROSTER_SLOTS];
extern uint16_t rosterPayTiers[CAMPAIGN_ROSTER_SLOTS];
extern uint16_t rosterGunneryTiers[CAMPAIGN_ROSTER_SLOTS];
extern uint16_t rosterPilotingTiers[CAMPAIGN_ROSTER_SLOTS];
extern uint16_t rosterUnknownWords[CAMPAIGN_ROSTER_SLOTS];
extern uint16_t rosterMechAssignments[CAMPAIGN_ROSTER_SLOTS];
extern uint16_t rosterPortraitIds[CAMPAIGN_ROSTER_SLOTS];
extern uint8_t pilotGunneryTiers[CAMPAIGN_PILOT_ID_COUNT];
extern uint8_t pilotPilotingTiers[CAMPAIGN_PILOT_ID_COUNT];
extern uint8_t pilotExperienceCounts[CAMPAIGN_PILOT_ID_COUNT];
extern uint16_t pilotDefinitionOffsets[CAMPAIGN_PILOT_ID_COUNT];
extern uint8_t pilotDefinitionStorage[PILOT_DEFINITIONS_END - PILOT_DEFINITIONS_START];
extern uint16_t travelSquareRootValues[TRAVEL_ROOT_TABLE_COUNT];
extern uint16_t travelDestinationMapPositionX, travelDestinationMapPositionY;
extern uint16_t travelMapDeltaX, travelMapDeltaY, travelDestinationTimeWeight;
extern uint16_t travelDestinationPlanetOffset, travelDistanceUnits;
extern uint16_t travelQuoteCbillsLow, travelQuoteCbillsHigh;
extern uint16_t travelMapSelection, travelDestinationPlanetIndex;
extern uint16_t travelMarkerX, travelMarkerY;
extern uint16_t travelSavedMarkerX, travelSavedMarkerY;
extern uint16_t travelJoystickLastTick, travelAnimationFrameStep;
/* One authored House range table for both planet directories. Mission52C4
 * passes the count unchanged to inclusive RNG; do not silently subtract one. */
extern uint16_t housePlanetRangeStarts[CAMPAIGN_HOUSE_COUNT];
extern uint16_t housePlanetRangeCounts[CAMPAIGN_HOUSE_COUNT];
extern uint8_t travelDropShipSelected;
extern uint16_t hiredTransportReverseTerminator;
extern uint16_t hiredTransportFrames[HIRED_TRANSPORT_FRAME_COUNT];
extern uint16_t dropShipTravelFrames[DROPSHIP_TRAVEL_FRAME_COUNT];
extern char travelDestinationName[TRAVEL_PLANET_NAME_CAPACITY];
extern uint16_t alphabeticPlanetRecordOffsets[TRAVEL_PLANET_DIRECTORY_COUNT];
extern uint16_t travelSelectedHouseByteOffset, travelHousePlanetStart;
extern uint16_t travelHousePlanetCount;
extern uint16_t reserveAmmoQuantitiesByFamily[CAMPAIGN_AMMO_FAMILY_COUNT];
extern uint16_t reserveAmmoFreightRates[CAMPAIGN_AMMO_FAMILY_COUNT];

uint32_t MwCalculateUnitConditionPrice(uint8_t condition, uint16_t unitKind, uint16_t *marketTier);
void MwCalculateMarketPurchasePrice(void);
uint16_t MwHasBodyDamageForRepair(void);
void MwQuoteRepairComponent(CampaignUnitDamage *damage);
void MwQuoteMechRepairTotal(uint16_t unitKind, CampaignUnitDamage *damage);
void MwInitializePilotRatingsFromDefinitions(void);
void MwAdvancePilotSkillFromExperience(uint16_t pilotRosterSlot);
uint16_t MwRemovePilotAndCompactRoster(uint16_t rosterByteOffset);
uint16_t MwLookupScaledSquareRoot(uint16_t tableIndex);
uint16_t MwApproximateSquareRoot(uint32_t squaredMagnitude);
uint16_t MwCalculatePlanetTravelDistance(uint16_t destinationPlanetIndex);
void MwAddReserveAmmoFreightToTravelQuote(void);

#endif
