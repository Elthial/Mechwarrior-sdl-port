#ifndef MECHWARRIOR_CAMPAIGN_PILOTS_H
#define MECHWARRIOR_CAMPAIGN_PILOTS_H
#include "mechwarrior/ega_campaign.h"
#include "mechwarrior/campaign_service.h"

enum { PILOT_CARD_COUNT=4, PILOT_CARD_COLUMN_WIDTH=0x44,
    PILOT_CARD_FIRST_X=0x2C, PILOT_CARD_TEXT_INSET=4,
    PILOT_CARD_NAME_Y=0x47, PILOT_CARD_GUNNERY_Y=0x4F,
    PILOT_CARD_PILOTING_Y=0x57, PILOT_CARD_WAGE_Y=0x65,
    PILOT_CARD_ASSIGNMENT_Y=0x6D, PILOT_CARD_MECH_Y=0x84,
    PILOT_CARD_COMMANDER_PICTURE=0x16, PILOT_CARD_LABEL_PICTURE=0x1A,
    PILOT_ASSIGNMENT_NONE=0xFFFF, PILOT_ASSIGNMENT_DISMISS=0xFFFE,
    PILOT_CARD_BACKGROUND=Campaign_Pal_EGA_DarkGrey, PILOT_CARD_TEXT_COLOUR=Campaign_Pal_EGA_LightGrey };

extern uint16_t companyMenuSelectedRow,barMenuSelectedRow;
extern MwPanelDescriptor companyMenuPanel,commanderProfilePanel;
extern MwPanelDescriptor crewLabelsPanel,barMenuPanel,recruitInterviewPanel;
extern MwUiMenu companyMenu,crewMenu,barMenu,recruitInterviewMenu;
extern MwServiceCaption companyMenuLabels[4],commanderProfileLabels[11];
extern MwServiceCaption crewLabels[7],barMenuLabels[4],recruitInterviewLabels[3];
extern char commanderProfileName[0x0A];
extern char commanderReputationLabels[4][0x10];
extern char houseAttitudeLabels[5][0x0A];
extern const char *houseAttitudeText[9];
extern char pilotSkillLabels[4][0x0A];
/* Two preceding native table entries are DISMISS and NONE. In C they have
 * proper array indices, not negative subscripts into an unrelated allocation. */
extern char pilotAssignmentCaptions[12][0x0C];
extern uint8_t pilotAssignmentCaptionColours[12];
extern uint16_t crewScreenRightEdge,pilotCardRosterSlot,pilotCardTextX;
extern uint16_t displayedPilotId,pilotCardGunneryTier,pilotCardPilotingTier;
extern uint16_t pilotCardPortraitId,pilotCardWageTier,pilotCardOwnedUnitSlot;
extern uint16_t pilotCardColumnOrigins[4];
extern uint16_t assignmentSelectedPilotSlot,assignmentCandidateOwnedSlot;
extern uint8_t assignmentConflictHighlighted;
extern uint16_t assignmentConflictingPilotSlot,assignmentConflictingOwnedSlot;
extern uint16_t recruitCandidateByteOffset,selectedRecruitPilotId;
extern char recruitStatLabels[0x17],noAvailableRecruitsText[0x21];
extern char fullFourPilotRosterText[0x33],fullThreePilotRosterText[0x35];
extern const MwPilotDefinitionPrefix *pilotDefinitions[CAMPAIGN_PILOT_ID_COUNT];
extern const char *pilotDialogue[CAMPAIGN_PILOT_ID_COUNT];
enum { BAR_BACKGROUND_PICTURE=6, BAR_LANDSCAPE_PICTURE_BASE=8,
    BAR_DRINK_PICTURE=7, BAR_HUB_ROW=4, BAR_ROW_DRINK=0,BAR_ROW_RECRUIT=1,
    BAR_FONT=3, BAR_FIRST_VISIT=0xFFFF, BAR_INITIAL_EVENT_CLASS=1,BAR_DRINK_EVENT_CLASS=2 };
/* entryOffset and returned WORD transport original shared JMPs, not native
 * source arguments/results.69B8 resumes the suspended6236 CALL. */
uint16_t MwEnterBarContinuation(uint16_t entryOffset);
uint16_t MwOrderDrinkContinuation(uint16_t entryOffset);
enum { RECRUIT_STATS_X=0xD2,RECRUIT_NAME_Y=0x8F,RECRUIT_GUNNERY_Y=0x97,
    RECRUIT_PILOTING_Y=0x9F,RECRUIT_WAGE_Y=0xA7,RECRUIT_PORTRAIT_Y=0x3A,
    RECRUIT_DIALOGUE_X=0x1A,RECRUIT_DIALOGUE_Y=0x32,RECRUIT_DIALOGUE_COLOUR=Campaign_Pal_EGA_LightRed,
    RECRUIT_STAT_PANEL_X=0x90,RECRUIT_STAT_PANEL_Y=0x87,
    RECRUIT_STAT_PANEL_WIDTH=0x9C,RECRUIT_STAT_PANEL_HEIGHT=0x30 };
bool MwFindNextRecruitCandidate(void);
uint16_t MwOpenRecruitInterviewContinuation(void);
void MwDrawRecruitStatText(const char *text,uint16_t y);
uint16_t MwHireSelectedRecruitContinuation(void);
uint16_t MwRejectSelectedRecruitContinuation(void);
enum { COMMANDER_REPUTATION_WORTH_WATCHING=1,COMMANDER_AGE_YEAR_BIAS=0x0BBF,
    COMMANDER_AGE_MONTH_THRESHOLD=2,COMPANY_PROFILE_X=0x18,
    COMPANY_LANDSCAPE_PICTURE_BASE=3,COMPANY_MENU_FONT=3,CREW_FONT=0,
    PILOT_MECH_PICTURE_BASE=0x16,COMPANY_PROFILE_WIDTH=0x8A,
    COMPANY_PROFILE_WATCHING_WIDTH=0xA8,COMPANY_PROFILE_NAME_X=0x48,
    COMPANY_PROFILE_NAME_Y=0x24,COMPANY_REPUTATION_X=0x4A,COMPANY_REPUTATION_Y=0x34,
    COMPANY_AGE_X=0x20,COMPANY_AGE_Y=0x2C,COMPANY_WEALTH_X=0x32,COMPANY_WEALTH_Y=0x3C,
    COMPANY_HOUSE_ATTITUDE_X=0x3C,COMPANY_HOUSE_FIRST_Y=0x4C,COMPANY_HOUSE_ROW_HEIGHT=8,
    COMPANY_HOUSE_STANDING_LIMIT=8,CREW_FULL_RIGHT_EDGE=0x13B,CREW_SMALL_RIGHT_EDGE=0xF7,
    CREW_RIGHT_MARGIN_WIDTH=5,CREW_LAST_Y=0xC7,AUDIO_LABEL_OFF=1,BAR_BACKGROUND_COLOUR=Campaign_Pal_EGA_Black };
uint16_t MwEnterCompanyMenuContinuation(uint16_t entryOffset);
void MwDrawCommanderProfile(void);
uint16_t MwEnterCrewMenuContinuation(void);
void MwDrawPilotRosterCard(uint16_t pilotRosterSlot);
void MwDrawPilotCardSeparators(void);
void MwDrawPilotCardText(const char *text,uint16_t y);
const char *MwSetPilotAssignmentCaption(uint16_t unitKind);
enum { ASSIGNMENT_TEXT_WIDTH=0x3C,ASSIGNMENT_TEXT_HEIGHT=8,
    ASSIGNMENT_SELECTED_COLOUR=Campaign_Pal_EGA_LightGreen,ASSIGNMENT_CONFLICT_COLOUR=Campaign_Pal_EGA_Red,
    ASSIGNMENT_CONFIRMED_COLOUR=Campaign_Pal_EGA_Yellow,ASSIGNMENT_MECH_WIDTH=0x42,
    ASSIGNMENT_MECH_HEIGHT=0x43 };
uint16_t MwBeginPilotMechAssignmentContinuation(uint16_t pilotRosterSlot);
uint16_t MwPollPilotMechAssignmentContinuation(void);
void MwFindAssignmentConflict(void);
void MwDrawPilotAssignmentCandidate(uint16_t pilotRosterSlot,uint16_t ownedUnitSlot,uint8_t foreground);
void MwClearAssignmentConflictHighlight(void);
void MwHighlightSelectedPilotAssignment(void);
void MwDrawSelectedPilotAssignment(uint8_t foreground);
void MwDrawAssignmentMechPreview(uint16_t ownedUnitSlot);
uint16_t MwConfirmPilotMechAssignmentContinuation(void);
void MwTransferPreviousMechToConflictingPilot(void);
uint16_t MwDismissSelectedPilotContinuation(uint16_t selectedPilotByteOffset);
#endif
