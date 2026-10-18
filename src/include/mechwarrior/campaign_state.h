#ifndef MECHWARRIOR_CAMPAIGN_STATE_H
#define MECHWARRIOR_CAMPAIGN_STATE_H
#include "mechwarrior/campaign.h"
#include "mechwarrior/campaign_exchange.h"

/* Named owners from the existing campaign/combat exchange schema. Unknown
 * bytes stay neutral; this header does not introduce a packed memory image. */
enum { COMMANDER_INITIAL_AGE_YEARS=0x12 };
extern uint8_t unidentifiedField18,missionCategoryOrMemoryTier,deploymentLandscapeIndex;
extern uint8_t combatEnemyHeavyCount,combatEnemyMediumCount,combatEnemyLightCount;
extern uint8_t combatPlanetSelector,combatScenarioCode,deploymentTerrainDetailLevel;
extern CampaignSalvageEntry salvageEntries[4];
extern uint16_t returnedDisabledEnemyCount;
extern uint8_t combatTransitionResult,acceptedContractActive;
extern uint8_t unidentifiedField19[2];
extern uint8_t commanderNameBytes[0x000A];
extern uint16_t commanderAgeYears,houseNetStandings[CAMPAIGN_HOUSE_COUNT];
extern uint16_t requestedContractPaymentThousands,requestedContractSalvagePercent;
extern uint16_t requestedContractAdvancePercent,requestContractPaymentEnabled;
extern uint16_t requestContractSalvageEnabled,requestContractAdvanceEnabled;
extern uint8_t unidentifiedField20[6];
extern uint16_t acceptedContractPaymentThousands,acceptedContractSalvagePercent;
extern uint16_t acceptedContractAdvancePercent,remainingContractPaymentThousands;
extern uint8_t audioOptionLabelIndex,campaignTerrainDetailLevel;
extern uint16_t acceptedContractTargetHouseIndex,playerSkillProgress;
extern uint8_t defaultHouseMissionTypeLimits[0x28],campaignDefaultCommanderNameBytes[0x0A];
#endif
