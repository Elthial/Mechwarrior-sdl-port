#ifndef MECHWARRIOR_CAMPAIGN_HANDOFF_H
#define MECHWARRIOR_CAMPAIGN_HANDOFF_H
#include "mechwarrior/ega_campaign.h"
#include "mechwarrior/campaign_exchange.h"
#include "mechwarrior/campaign_state.h"
#include "mechwarrior/campaign_mechanics.h"

enum { CAMPAIGN_SALVAGE_SLOTS=4, DEPLOYMENT_BYPASS_LAST_SCENARIO=0x0A,
    DEPLOYMENT_NO_BYPASS_SCENARIO=9, DEPLOYMENT_BYPASS_ROLL_LIMIT=0x0F };
typedef struct MwCombatDiskPrompt
{
    uint8_t beforeDrive[0x1E],driveLetter,afterDrive[0x4E];
} MwCombatDiskPrompt;
_Static_assert(sizeof(MwCombatDiskPrompt)==0x6D,"Simulator disk prompt before picture decoder state");
extern char combatExecutableFilename[10];
extern MwCombatDiskPrompt combatDiskPrompt;
extern uint8_t deploymentResolvedWithoutCombat;
extern char afterActionCrewHeading[0x0D],afterActionMechHeading[0x0D];
extern char afterActionSalvageHeading[9],afterActionPaymentHeading[9];
extern char afterActionPercentSuffix[2],afterActionPilotStatusText[3][8];
extern uint16_t afterActionRosterRowOffset;
extern uint16_t campaignMechTonnages[CAMPAIGN_MECH_KIND_COUNT];
extern uint16_t settlementPayoutPercent;
extern uint16_t houseDeductionRollThresholds[CAMPAIGN_HOUSE_COUNT];
extern uint16_t houseReducedPayoutPercentages[CAMPAIGN_HOUSE_COUNT];
enum { SALVAGE_SCRAP_CONDITION_LIMIT=0x0023,SALVAGE_SCRAP_THOUSANDS_PER_TON=10,
    AFTER_ACTION_PAYMENT_X=0x00E8,AFTER_ACTION_SALVAGE_Y=0x006E,AFTER_ACTION_PAYMENT_Y=0x0078 };
enum { COMMANDER_REPUTATION_SECOND_THRESHOLD=6,COMMANDER_REPUTATION_THIRD_THRESHOLD=0x000B,
    COMMANDER_REPUTATION_FOURTH_THRESHOLD=0x0015,DEATH_MUSIC_OFFSET=0x2328 };
void MwStartDeathMusic(void);
enum { DEPLOYMENT_SCENARIO_TIME_COUNT=0x0025,DEPLOYMENT_TRAVEL_MULTIPLIER=0x000E,
    SPECIAL_ASSAULT_DEPLOYMENT_UNITS=0x001C,LAUNCH_MUSIC_OFFSET=0x2AF8,
    AFTER_ACTION_OUTCOME_X=0x000A,AFTER_ACTION_OUTCOME_Y=0x0094 };
extern uint16_t missionDeploymentUnitsByScenario[DEPLOYMENT_SCENARIO_TIME_COUNT];
extern char deploymentUneventfulText[0x56];
extern char afterActionKuritaFailure[0x8D],afterActionLiaoFailure[0x73],afterActionGeneralFailure[0x91];
extern char afterActionKuritaSuccess[0x62],afterActionLiaoSuccess[0x6C],afterActionGeneralSuccess[0x44];
extern char settlementSupplyDeductionText[0xA8],settlementPerformanceDeductionText[0xAC];
extern char settlementTransportDeductionText[0x98],settlementNoPaymentText[0x95],darkWingAttackFailedText[0x49];
extern const char *afterActionFailureMessages[CAMPAIGN_HOUSE_COUNT];
extern const char *afterActionSuccessMessages[CAMPAIGN_HOUSE_COUNT];
extern const char *settlementDeductionMessages[3];
bool MwImportCampaignHandoffData(const uint8_t *data,size_t byteCount);
extern char darkWingVictoryText[0x185];
enum { VICTORY_TRANSITION_PICTURE=0x004D,VICTORY_TRANSITION_TICKS=5,VICTORY_ANIMATION_CYCLES=0x14 };
void MwDrawVictoryTransitionPictureAndWait(void);
bool MwLoadVictoryMusic(void);
enum { AFTER_ACTION_BACKGROUND=Campaign_Pal_EGA_LightGrey,AFTER_ACTION_FOREGROUND=Campaign_Pal_EGA_Black,AFTER_ACTION_FONT=3,
    AFTER_ACTION_TEXT_X=0x00A0,AFTER_ACTION_STATUS_X=0x00F8,
    AFTER_ACTION_FIRST_PILOT_Y=0x0014,AFTER_ACTION_FIRST_MECH_Y=0x0046,
    AFTER_ACTION_ROW_HEIGHT=8,AFTER_ACTION_PERCENT_X=0x0108,
    AFTER_ACTION_SUFFIX_X=0x0120,AFTER_ACTION_SUCCESS_PICTURE=0x004C,
    AFTER_ACTION_LOSS_PICTURE=0x0048,AFTER_ACTION_RETREAT_PICTURE=0x004E,
    AFTER_ACTION_HOUSE_PICTURE_BASE=0x0025 };
void MwDrawCampaignTextColourOne(uint16_t x,uint16_t y,const char *text);
void MwWaitForRequiredResourceFile(void);
/* Host JMP transport, not an AX result or a new native CALL boundary. */
uint16_t MwResolveDeploymentOrExportCombatExchangeContinuation(void);
#endif
