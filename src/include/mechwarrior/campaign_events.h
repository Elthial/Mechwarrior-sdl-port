#ifndef MECHWARRIOR_CAMPAIGN_EVENTS_H
#define MECHWARRIOR_CAMPAIGN_EVENTS_H
#include "mechwarrior/campaign_startup.h"
#include "mechwarrior/campaign_ui.h"

enum { CAMPAIGN_EVENT_RECORD_COUNT=0x75, CAMPAIGN_EVENT_COMMAND_COUNT=0xCB,
    CAMPAIGN_EVENT_LIST_COUNT=6, CAMPAIGN_EVENT_LIST_ENTRIES=0x57,
    CAMPAIGN_EVENT_CAPTION_COUNT=0x1C, CAMPAIGN_EVENT_SEEN_FLAGS=0x8C,
    EVENT_DATE_YEAR_BASE=0x0BB8, EVENT_PORT_ARREST=0x75,
    EVENT_DARK_WING_BASE_ATTACK=0x72, EVENT_CONTINUE_OR_QUIT=0x76,
    EVENT_FIRST_HOUSE_REPUTATION_ARRIVAL=0x4F, EVENT_LAST_HOUSE_REPUTATION_ARRIVAL=0x53,
    EVENT_SCRIPT_END=0xFF, EVENT_CLASS_REPEATABLE=3 };
enum MwEventConditionOpcode
{
    EVENT_CONDITION_RANDOM_THRESHOLD,EVENT_CONDITION_SCHEDULED_DATE,
    EVENT_CONDITION_CURRENT_PLANET,EVENT_CONDITION_SEEN_EVENT,
    EVENT_CONDITION_HOUSE_STANDING,EVENT_CONDITION_DIFFERENT_PLANET,
    EVENT_CONDITION_UNCHANGED,EVENT_CONDITION_UNRESOLVED_TARGET,
    EVENT_CONDITION_PLAYER_COMBAT_LOSS,EVENT_CONDITION_CHOICES
};
enum MwEventActionOpcode
{
    EVENT_ACTION_UNCHANGED_FIRST,EVENT_ACTION_QUEUE_RESTART,EVENT_ACTION_DATE_WINDOW,
    EVENT_ACTION_UNCHANGED_SECOND,EVENT_ACTION_ADJUST_FUNDS,EVENT_ACTION_USE_AND,
    EVENT_ACTION_USE_OR,EVENT_ACTION_UNCHANGED_THIRD,EVENT_ACTION_SUBSTITUTE_PLANET,
    EVENT_ACTION_CHOICES
};
typedef struct MwEventDate
{
    uint8_t monthCode,submonthCode,yearOffset;
} MwEventDate;
typedef struct MwEventCaption
{
    uint8_t colour;
    char text[0x20];
} MwEventCaption;
typedef struct MwCampaignEvent MwCampaignEvent;
typedef struct MwEventCommand MwEventCommand;
/* Decoded original command operands, not a new event language. Native BYTE
 * opcodes still select the original two dispatch tables. Near references are
 * resolved only when loading the private installation. */
struct MwEventCommand
{
    uint8_t opcode;
    union
    {
        uint8_t randomThreshold,seenEventId;
        int8_t planetSelector;
        MwEventDate date;
        struct { uint8_t houseSelector;int8_t thresholdHalf; } houseStanding;
        struct { MwEventDate start,end; } dateWindow;
        uint32_t cashDelta;
        struct
        {
            uint8_t count;
            MwEventCaption *captions[3];
            MwCampaignEvent *branches[3];
        } choices;
    } operand;
    MwEventCommand *next;
};
struct MwCampaignEvent
{
    uint8_t eventId,classCode;
    MwEventCommand *script;
};
extern MwCampaignEvent campaignEventRecords[CAMPAIGN_EVENT_RECORD_COUNT];
extern MwEventCommand campaignEventCommands[CAMPAIGN_EVENT_COMMAND_COUNT];
extern MwCampaignEvent *eventListEntries[CAMPAIGN_EVENT_LIST_ENTRIES];
extern MwCampaignEvent **eventLists[CAMPAIGN_EVENT_LIST_COUNT];
extern MwCampaignEvent *fixedCampaignEvent;
extern MwEventCaption eventChoiceCaptions[CAMPAIGN_EVENT_CAPTION_COUNT];
extern uint16_t eventDirectoryIndex,campaignEventClass,currentEventId,dispatchedEventCount;
extern uint8_t eventSeenFlags[CAMPAIGN_EVENT_SEEN_FLAGS];
extern uint8_t eventConditionResult,eventCombineWithAnd;
extern uint16_t eventScheduledMonthScratch,eventExpirySubmonthScratch,eventScheduledYearScratch;
extern uint8_t planetArrivalEventsPending,darkWingBaseVictoryRecorded,campaignRestartPending;
extern const MwEventCommand *eventOperandCursor;
bool MwImportCampaignEventData(const uint8_t *data,size_t byteCount);

/* Original class table, with C references instead of native near code offsets.
 * The result transports the live text-panel registers, not a native struct ABI. */
typedef MwTextPanelReturn (*MwEventClassHandler)(uint16_t textId);
extern MwEventClassHandler eventClassHandlers[7];
/* Shared original opcode09 and action08 table targets. Tests can substitute
 * observers; normal bindings execute the actual converted game bodies. */
extern uint16_t (*campaignEventChoice)(const MwEventCommand *command);
extern void (*campaignEventPlanetSubstitution)(const MwEventCommand *command);

enum { EVENT_TEXT_DIRECTORY_COUNT=0x77, EVENT_TEXT_STORAGE_START=0x00EE,
    EVENT_TEXT_STORAGE_END=0xE089, EVENT_PLANET_NAME_FIELD_BYTES=0x10,
    EVENT_TEXT_PLACEHOLDER=0xFF, EVENT_CHOICE_ROW_PIXELS=7 };
/* This is a writable encoded TEXT resource, like the variable-length planet
 * names. It is not a DS mirror or a second copy of decoded game state. */
typedef struct MwEventTextPageHeader { uint8_t format,rowCount; } MwEventTextPageHeader;
extern uint8_t eventTextStorage[EVENT_TEXT_STORAGE_END-EVENT_TEXT_STORAGE_START];
extern uint8_t *eventTextRecords[EVENT_TEXT_DIRECTORY_COUNT];
bool MwImportCampaignEventText(const uint8_t *data,size_t byteCount);
extern uint8_t eventChoiceCount,eventTextRowCount;
extern MwCampaignEvent *eventChoiceBranches[3];
extern uint16_t eventChoiceSavedFontIndex;
extern MwUiRecord eventTwoChoiceRecords[2],eventThreeChoiceRecords[3];
extern MwUiMenu eventTwoChoiceMenu,eventThreeChoiceMenu;
MwTextPanelReturn MwShowCampaignEventText(uint16_t textId);
MwTextPanelReturn MwShowClassThreeEventPages(uint16_t textId);
void MwEventSubstitutePlanetName(const MwEventCommand *command);
uint16_t MwEventOpenChoiceContinuation(const MwEventCommand *command);
uint16_t MwEventFirstChoiceCallback(void);
uint16_t MwEventSecondChoiceCallback(void);
uint16_t MwEventThirdChoiceCallback(void);
uint16_t MwQuitFromEventContinuation(void);
uint16_t MwRestartCampaignContinuation(void);
uint16_t MwPrepareEventSpecialCombatContinuation(void);

/* Code offsets transport native JMPs, not new AX results. entryOffset6290
 * resumes the original dispatcher after its saved list cursor is restored. */
uint16_t MwDispatchCampaignEvents(uint16_t eventClass,uint16_t entryOffset);
uint16_t MwDispatchFixedCampaignEvent(void);
/* Host transport for639B's suspended native caller, NOT serialized1090 data.
 * Converted6236 supplies6290. Other native callers must bind their own frame. */
extern uint16_t campaignEventReturnContinuation;
uint16_t MwInterpretCampaignEvent(const MwCampaignEvent *event,bool *carry);
void MwCombineEventCondition(uint8_t condition);
void MwEventTestRandomThreshold(const MwEventCommand *command);
void MwEventTestScheduledDate(const MwEventCommand *command);
void MwEventTestCurrentPlanet(const MwEventCommand *command);
void MwEventTestSeenEvent(const MwEventCommand *command);
void MwEventTestControllingHouseStanding(const MwEventCommand *command);
void MwEventTestDifferentPlanet(const MwEventCommand *command);
void MwEventTestPlayerCombatLoss(void);
void MwEventQueueCampaignRestart(void);
void MwEventRestrictToDateWindow(const MwEventCommand *command);
void MwEventAdjustFunds(const MwEventCommand *command);
void MwEventUseAnd(void);
void MwEventUseOr(void);
#endif
