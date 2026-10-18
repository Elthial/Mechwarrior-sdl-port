#ifndef MECHWARRIOR_CAMPAIGN_EXCHANGE_H
#define MECHWARRIOR_CAMPAIGN_EXCHANGE_H
enum { COMBAT_SCENARIO_DARK_WING_BASE=0x63 };
#include <stdint.h>
#include "mechwarrior/campaign_mechanics.h"

enum { CAMPAIGN_EXCHANGE_BYTES=0x0718 };
/* Resident MW0806:0004. Binary transport between children, NOT live1090 state.
 * The same bytes form GAM files; decoding writes the canonical named owners. */
typedef struct CampaignSalvageEntry
{
    uint8_t unitKind,leadingZeroByte,conditionPercent,trailingZeroByte;
} CampaignSalvageEntry;
typedef struct MwTransferWord {uint8_t low,high;} MwTransferWord;
/* Actual0718-byte wire record, NOT the campaign's runtime data model.
 * Untouched campaign fields remain encoded; combat only observes the named
 * transfer fields. Byte-aligned words retain partial-byte export semantics. */
typedef struct MwCampaignTransferData
{
    uint8_t unidentifiedField1,memoryTier,landscape,enemyHeavyCount,enemyMediumCount,enemyLightCount;
    uint8_t planetSelector,scenarioCode,terrainDetail,pilotStatus[CAMPAIGN_ROSTER_SLOTS];
    CampaignSalvageEntry salvage[CAMPAIGN_ROSTER_SLOTS];
    MwTransferWord disabledEnemyCount;
    uint8_t result,campaignPrefixBytes[0x00CA];
    MwTransferWord exchangeOwnedUnitKinds[CAMPAIGN_OWNED_UNIT_SLOTS];
    CampaignUnitDamage ownedDamage[CAMPAIGN_OWNED_UNIT_SLOTS];
    MwTransferWord ammo[CAMPAIGN_AMMO_FAMILY_COUNT][CAMPAIGN_OWNED_UNIT_SLOTS];
    MwTransferWord reserveAmmo[CAMPAIGN_AMMO_FAMILY_COUNT];
    uint8_t campaignMiddleBytes[0x02E8],soundDisabled,campaignSuffixBytes[0x0135];
} MwCampaignTransferData;
_Static_assert(sizeof(MwCampaignTransferData)==CAMPAIGN_EXCHANGE_BYTES,"Resident transfer/GAM wire size");
_Static_assert(offsetof(MwCampaignTransferData,ownedDamage)==0x0102,"Combat damage transfer offset");
_Static_assert(offsetof(MwCampaignTransferData,ammo)==0x025E,"Packed ammo transfer offset");
_Static_assert(offsetof(MwCampaignTransferData,reserveAmmo)==0x02EE,"Reserve ammo transfer offset");
_Static_assert(offsetof(MwCampaignTransferData,soundDisabled)==0x05E2,"Sound preference transfer offset");
typedef union MwCampaignTransferStorage {uint8_t bytes[CAMPAIGN_EXCHANGE_BYTES];MwCampaignTransferData fields;} MwCampaignTransferStorage;
extern MwCampaignTransferStorage campaignTransfer;
#define campaignCombatExchange campaignTransfer.bytes
#define campaignResultExchange campaignTransfer.fields
void MwEncodeCampaignExchange(uint8_t bytes[CAMPAIGN_EXCHANGE_BYTES]);
void MwDecodeCampaignExchange(const uint8_t bytes[CAMPAIGN_EXCHANGE_BYTES]);
#endif
