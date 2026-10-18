#ifndef MECHWARRIOR_CAMPAIGN_CONTRACTS_H
#define MECHWARRIOR_CAMPAIGN_CONTRACTS_H

#include "mechwarrior/campaign_service.h"
#include "mechwarrior/campaign_state.h"

/* Native order proven by1090:4A69's five House-name references. */
enum { HOUSE_KURITA, HOUSE_STEINER, HOUSE_MARIK, HOUSE_LIAO, HOUSE_DAVION };

enum { CONTRACT_OFFICE_HUB_ROW=2, CONTRACT_OFFICE_EVENT_CLASS=5,
    CONTRACT_OFFICE_FONT=3, CONTRACT_OFFICE_EMBLEM_BASE=0x20,
    CONTRACT_OFFICE_PORTRAIT_BASE=0x2A, CONTRACT_OFFICE_REGION_BASE=0x2F,
    CONTRACT_REGION_SEPARATOR=0xFF, CONTRACT_REGION_END=0xFE,
    CONTRACT_TERM_COUNT=3, CONTRACT_PAYMENT_TERM=0, CONTRACT_SALVAGE_TERM=1,
    CONTRACT_ADVANCE_TERM=2, CONTRACT_PAYMENT_STEP=0x000A,
    CONTRACT_PAYMENT_EDITOR_MAXIMUM=0x270F, CONTRACT_PERCENT_MAXIMUM=0x0064,
    CONTRACT_EDITING_COLOUR=0x0D, CONTRACT_TERM_COLOUR=9, CONTRACT_TEXT_COLOUR=3,
    CONTRACT_HEADING_COLOUR=0x0C, CONTRACT_ACCEPT_CAPTION_BYTES=6,
    CONTRACT_EDITOR_PROMPT_X=0x37, CONTRACT_EDITOR_PROMPT_Y=0xB9,
    CONTRACT_EDITOR_PROMPT_WIDTH=0x30, CONTRACT_TERM_HEIGHT=8,
    CONTRACT_TERM_COPY_WIDTH=0x28, CONTRACT_REFUSALS_BEFORE_CANCELLATION=3 };

/* Authored screen placement, not BattleTech rules or simulation units. */
enum { CONTRACT_PAGE_BACKGROUND=1, CONTRACT_PROMPT_BACKGROUND=4,
    CONTRACT_FRAME_TOP_PICTURE=0x0C, CONTRACT_FRAME_BOTTOM_PICTURE=0x2C,
    CONTRACT_FRAME_LEFT_PICTURE=0x2D, CONTRACT_FRAME_RIGHT_PICTURE=0x2E,
    CONTRACT_FRAME_BOTTOM_Y=0xB1, CONTRACT_FRAME_SIDE_Y=0x17, CONTRACT_FRAME_RIGHT_X=0x123,
    CONTRACT_OFFICE_EMBLEM_X=0x2C, CONTRACT_OFFICE_EMBLEM_Y=0x25,
    CONTRACT_OFFICE_PORTRAIT_X=0x4F, CONTRACT_OFFICE_PORTRAIT_Y=0x5B,
    CONTRACT_OFFICE_REGION_X=0xBD, CONTRACT_OFFICE_REGIONS_PER_HOUSE=5,
    CONTRACT_TEXT_X=0x26, CONTRACT_NARRATIVE_Y=0x16,
    CONTRACT_EMPLOYER_X=0x56, CONTRACT_EMPLOYER_Y=0x36,
    CONTRACT_OBJECTIVE_Y=0x46, CONTRACT_DESTINATION_Y=0x56,
    CONTRACT_TARGET_HOUSE_X=0x5E, CONTRACT_TARGET_HOUSE_Y=0x4E,
    CONTRACT_HEAVY_COUNT_X=0x3E, CONTRACT_MEDIUM_COUNT_X=0x8E,
    CONTRACT_FORCE_COUNTS_Y=0x66, CONTRACT_LIGHT_COUNT_Y=0x6E,
    CONTRACT_PAYMENT_X=0xAE, CONTRACT_PAYMENT_Y=0x7E, CONTRACT_PAYMENT_DIGITS=4,
    CONTRACT_SALVAGE_X=0x46, CONTRACT_SALVAGE_Y=0x86,
    CONTRACT_ADVANCE_X=0x9E, CONTRACT_ADVANCE_Y=0x8E, CONTRACT_PERCENT_DIGITS=3,
    CONTRACT_MONTH_X=0x56, CONTRACT_DATE_Y=0xA6, CONTRACT_YEAR_BASE_X=0x5E,
    CONTRACT_ACCEPTED_MESSAGE_X=0x38, CONTRACT_ACCEPTED_MESSAGE_Y=0x61 };

extern MwPanelDescriptor contractOfficePanel,contractActionsPanel;
extern MwServiceCaption contractOfficeLabels[2],contractActionLabels[3];
extern MwUiMenu contractOfficeMenu,contractActionsMenu;
extern uint16_t negotiatingMissionOfferSlot,negotiatingMissionOfferByteOffset;
extern uint8_t contractRequestEditorInitialized,contractRequestTextColour;
extern uint16_t contractRequestTermByteOffset,contractNegotiationAttempts;
extern uint8_t negotiationRequestsSatisfied;
extern uint16_t contractDateYearXOffsets[CAMPAIGN_MONTH_COUNT];
extern uint16_t requestedTermX[3],requestedTermY[3],requestedTermDigits[3];
extern char contractNarrative[0x013C],houseContractNames[CAMPAIGN_HOUSE_COUNT][8];
extern char submitContractCaption[6],acceptContractCaption[6];
extern const uint8_t *contractOfficePlanetGroups[CAMPAIGN_HOUSE_COUNT];
extern uint16_t houseExtraEnemyRollThresholds[CAMPAIGN_HOUSE_COUNT];
extern char noContractsText[0x2B],negotiationTerminatedText[0x47];
extern char contractAcceptedText[0x1B],houseContractRefusalText[0x5F];
extern uint16_t acceptedContractAdvanceCbillsLow,acceptedContractAdvanceCbillsHigh;
extern uint8_t deploymentRefusalCount,deploymentPenaltySuppressed;
extern char commanderNeedsMechText[0x37],assignedMechUnreadyText[0x3A],contractCancelledText[0x4F];

void MwDrawCampaignFramedBackground(void);
uint16_t MwEnterContractOfficeContinuation(uint16_t entryOffset);
void MwInitializeContractRequestDisplay(uint16_t missionOfferSlot);
void MwDrawNegotiatingOfferTerms(void);
void MwDrawContractHeading(uint16_t x,uint16_t y,const char *text);
void MwDrawContractText(uint16_t x,uint16_t y,const char *text);
uint16_t MwDrawMissionContractBriefingContinuation(uint16_t missionOfferSlot);
uint16_t MwRequestMissionOfferContinuation(void);
uint16_t MwNextMissionOfferContinuation(void);
void MwRedrawRequestedContractTerm(uint16_t termByteOffset);
uint16_t MwEditContractRequestContinuation(uint16_t termByteOffset);
uint16_t MwEditContractPaymentRequestContinuation(void);
uint16_t MwEditContractSalvageRequestContinuation(void);
uint16_t MwEditContractAdvanceRequestContinuation(void);
uint16_t MwNegotiateAndAcceptContractContinuation(void);
uint16_t MwCreditAcceptedContractAdvanceContinuation(void);
uint16_t MwAdjustAcceptedContractEnemyForceContinuation(void);
uint16_t MwAttemptContractDeployment(void);
uint16_t MwCheckDeploymentWithoutRefusalPenalty(void);
uint16_t MwCheckAssignedMechDeploymentReadiness(void);
uint16_t MwCancelContractAfterDeploymentRefusalsContinuation(void);

#endif
