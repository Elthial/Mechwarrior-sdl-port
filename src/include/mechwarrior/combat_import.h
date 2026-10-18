#ifndef MECHWARRIOR_COMBAT_IMPORT_H
#define MECHWARRIOR_COMBAT_IMPORT_H
#include "mechwarrior/combat_mechs.h"
#include "mechwarrior/campaign_mechanics.h"
#include "mechwarrior/campaign_exchange.h"

enum { COMBAT_STAGE_PROFILES=9,COMBAT_STAGES_PER_PROFILE=3,COMBAT_MISSION_COUNT=0x25,
    COMBAT_DEFAULT_SKILL_PARAMETER=4,CAMPAIGN_SKILL_TO_COMBAT_BASE=5,
    COMBAT_MEMORY_TIER_MINIMUM=2,COMBAT_ENGINE_HIT_LIMIT=3,COMBAT_LIFE_SUPPORT_HIT_LIMIT=2,
    CAMPAIGN_FULL_BODY_INTEGRITY=3,COMBAT_INPUT_PLAYBACK=1,COMBAT_INPUT_RECORD=2,
    COMBAT_EXTENDED_OFFENSIVE=0x0E,COMBAT_EXTENDED_DEFENSIVE=0x0F,
    COMBAT_PLANETARY_ASSAULT=0x10,COMBAT_EXTENDED_SIEGE=0x11,
    COMBAT_SCENARIO_COUNT=0x14,COMBAT_NO_ASSIGNMENT=0xFFFF,
    COMBAT_PREPARE_DELAY=0x78,COMBAT_CHECKSUM_BYTES=0x12C,
    COMBAT_TEMPLATE_WARHAMMER=5,COMBAT_TEMPLATE_BATTLEMASTER=7,
    COMBAT_BODY_LEFT_ARM=0,COMBAT_BODY_RIGHT_ARM=1,COMBAT_MOVEMENT_FALLEN=1,
    COMBAT_WORLD_OBJECT_HIDDEN=0x80 };
typedef struct BtechCombatStageProfile {uint16_t missionCodes[COMBAT_STAGES_PER_PROFILE];} BtechCombatStageProfile;

/* A decoded INPUT at the real resident transfer boundary, not a second live
 * campaign image.2C40's only resident writes are the disjoint salvage bytes;
 * none of its callees writes the fields decoded here. */
typedef struct BtechCampaignImport
{
    uint8_t memoryTier,landscapeIndex,enemyHeavyCount,enemyMediumCount,enemyLightCount;
    uint8_t planetSelector,scenarioCode,terrainDetailLevel,soundDisabled;
    uint8_t commanderName[10];
    uint16_t pilotIds[4],gunneryTiers[4],pilotingTiers[4],ownedMechSlots[4],ownedUnitKinds[12];
    CampaignUnitDamage ownedDamage[12];
    uint16_t ammoQuantities[6][12]; /* Fourth owned read really enters market group0. */
} BtechCampaignImport;
void BtechDecodeCampaignImport(const uint8_t bytes[CAMPAIGN_EXCHANGE_BYTES],BtechCampaignImport *company);

extern uint16_t resourceDiskPromptNumber,combatContinuationStage;
extern uint16_t salvageEntryCount;
extern uint16_t campaignUnitKindByCombatTemplate[COMBAT_MECH_TEMPLATES];
extern uint16_t actuatorIntegrityTierByCondition[5],systemDamageTierByHits[3],weaponIntegrityTierByOperational[2];
enum { COMBAT_RESULT_SUCCESS=0,COMBAT_RESULT_FAILURE=1,COMBAT_RESULT_RETREAT=2,
    COMBAT_DISABLED_UNIT_VALUE=0xFFFF,COMBAT_FINAL_CONTINUATION_STAGE=2 };
typedef union BtechMissionRules {
    struct {uint16_t rules[36],continuationProfileIndex;} fields;
    uint16_t words[37];
} BtechMissionRules;
extern BtechMissionRules missionRules;
#define combatMissionSideZeroRules missionRules.words
#define combatContinuationProfileIndex missionRules.fields.continuationProfileIndex
extern uint16_t reducedMemoryMode,combatTerrainDetailLevel,combatMissionDefinitionIndex,combatSoundEnabled;
extern uint16_t playbackCombatInputRequested,combatScenarioIndex;
extern uint16_t gunneryTargetNumbers[8],pilotingRecoveryBounds[8],activeCockpitLayoutIndex;
extern uint16_t combatToPilotRosterSlots[4],combatToOwnedMechSlots[4],deployedPilotIds[4];
/* The unchecked sixth/seventh/eighth reservation writes really alias these
 * following WORDs. One canonical owner, not a larger independent ID array. */
typedef union BtechTemplateReservations
{
    struct {
        uint16_t templateIds[5],recordRequested,enemyCount,reservedCount,budgetLow,budgetHigh;
    } fields;
    uint16_t words[10];
} BtechTemplateReservations;
extern BtechTemplateReservations mechTemplateReservations;
#define reservedMechTemplateIds mechTemplateReservations.words
#define recordCombatInputRequested mechTemplateReservations.fields.recordRequested
#define enemyMechPreparationCount mechTemplateReservations.fields.enemyCount
#define reservedMechTemplateCount mechTemplateReservations.fields.reservedCount
#define mechTemplateBudgetBytesLow mechTemplateReservations.fields.budgetLow
#define mechTemplateBudgetBytesHigh mechTemplateReservations.fields.budgetHigh
extern uint32_t mechTemplateBudgetCostsBytes[8];
extern uint16_t hudColourSchemeIndex,specialTargetInteractionBlocked,sideObjectiveRules[2],combatLandscapeIndex;
extern BtechCombatStageProfile combatContinuationProfiles[9];
extern uint16_t opposingObjectiveRules[5];
extern uint16_t combatTemplateByCampaignUnitKind[10],weaponDisabledByDamageTier[4],systemHitsByDamageTier[4],actuatorLossByDamageTier[4];
extern uint16_t armorToBodyLocations[9],campaignBodyTierByArmorLocation[9];
extern uint16_t mechFallbackStartTemplates[8];
extern int16_t mechFallbackTemplateSteps[8];
extern uint8_t combatDefaultCommanderNameBytes[10];
extern uint16_t savedArgumentCount;
extern const char *const *savedArguments;
extern BtechUnitControlState *activeUnitControl;
extern uint16_t startupControlMovementAlias;

bool BtechImportCombatStageInitialData(const uint8_t *data,size_t byteCount);
/* Native has no source arguments. This WORD is explicitly host transport for
 * the observable preceding sound-loader stack residue, NEVER a Reko seed. */
void BtechPrepareCampaignCombatStage(uint16_t inheritedSoundStreamIdentity);
uint16_t BtechPrepareEnemyMech(uint16_t preferredTemplate);
uint16_t BtechTryReserveMechTemplate(uint16_t templateIndex);
uint16_t BtechIsMechOperational(const BtechMechState *mech);
void BtechLoadScenarioAndWorld(uint16_t scenarioIndex);
uint8_t BtechCalculateSalvageConditionPercent(const BtechMechState *mech);
void BtechDistributeCampaignReserveAmmo(void);
void BtechRecoverFriendlyMechCondition(void);
void BtechExportCombatResultsAndTransition(uint16_t resultCode);
void BtechWaitScaledRuntimeDelay(uint16_t delayUnits);
void BtechCalibrateJoystickAxisPairs(void);
void BtechConfigureCombatInputReplay(uint16_t requestedMode,const char *filename);
uint16_t BtechLowercaseCharacterByClassTable(uint16_t character);
#endif
