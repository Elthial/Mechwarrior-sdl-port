#ifndef MECHWARRIOR_CAMPAIGN_OPTIONS_H
#define MECHWARRIOR_CAMPAIGN_OPTIONS_H
#include "mechwarrior/campaign_service.h"
#include "mechwarrior/campaign_events.h"
#include "mechwarrior/campaign_exchange.h"

enum { CAMPAIGN_OPTIONS_ROWS=7, CAMPAIGN_SAVE_ROWS=12, CAMPAIGN_SAVE_STEM_BYTES=8,
    CAMPAIGN_SAVE_FILENAME_BYTES=13, CAMPAIGN_SAVE_BYTES=CAMPAIGN_EXCHANGE_BYTES,
    CAMPAIGN_NEWS_ROWS=3,
    OPTION_SAVE=0, OPTION_RESTORE, OPTION_SOUND, OPTION_DETAIL, OPTION_RESTART,
    OPTION_QUIT, OPTION_CONTINUE };
enum { OPTIONS_HUB_ROW=5,OPTIONS_LANDSCAPE_PICTURE_BASE=3,
    OPTIONS_SOUND_LABEL_COLUMN=11,OPTIONS_DETAIL_LABEL_COLUMN=8,
    OPTIONS_VALUE_FOREGROUND=13,OPTIONS_VALUE_BACKGROUND=4 };
typedef struct MwSaveListRow
{
    MwPanelText display;
    union
    {
        struct { char rowNumber[4],filename[9]; } columns;
        char text[13];
    } caption;
} MwSaveListRow;
typedef struct MwSaveDiskPrompt
{
    uint8_t beforeDrive[0x1E],driveLetter,afterDrive[2];
} MwSaveDiskPrompt;
_Static_assert(sizeof(MwSaveDiskPrompt)==0x21,"Native save-disk prompt endsA134, before numeric flags");
extern MwPanelDescriptor optionsPanel,restorePanel,confirmationPanel,newsMenuPanel;
extern MwServiceCaption optionsLabels[CAMPAIGN_OPTIONS_ROWS],confirmationLabels[4],newsMenuLabels[3];
extern MwServiceCaption restoreHeading,restoreCancel;
extern MwSaveListRow saveListRows[CAMPAIGN_SAVE_ROWS];
extern MwUiMenu optionsMenu,restoreMenu,confirmationMenu,newsMenu;
extern uint16_t saveFileHandle,saveFileCount;
extern const char *saveSearchFilename;
extern MwSaveListRow *saveListWriteRow;
extern char selectedSaveFilename[CAMPAIGN_SAVE_FILENAME_BYTES],saveFindPattern[6];
extern uint8_t saveNamePrompt[0x1B];
extern char audioOptionLabels[2][4],terrainDetailLabels[3][5];
extern uint8_t saveDiskFullText[0x2B],saveDiskErrorText[0x30],loadDiskErrorText[0x31];
extern MwSaveDiskPrompt saveDiskPrompt;
extern uint8_t newsAtStart,newsAtEnd;
extern uint16_t newsLastArticleId;
extern char newsBoundaryText[0x1B];
extern MwCampaignEvent **newsArticleCursor,**newsArticleListStart;
bool MwImportCampaignOptionsData(const uint8_t *data,size_t byteCount);
uint16_t MwEnterOptionsMenuContinuation(uint16_t entryOffset);
void MwUpdateAudioOptionLabel(void);
void MwUpdateTerrainDetailOptionLabel(void);
void MwToggleAudioOption(void);
void MwCycleTerrainDetailOption(void);
uint16_t MwConfirmQuitCampaignContinuation(void);
uint16_t MwConfirmRestartCampaignContinuation(void);
uint16_t MwQuitFromOptionsContinuation(void);
uint16_t MwRestartFromOptionsContinuation(void);
bool MwDosCreateSaveFile(const char *filename,uint16_t *handleOrError);
bool MwDosWriteFile(const uint8_t *buffer,uint16_t byteCount,uint16_t fileHandle,uint16_t *bytesWrittenOrError);
uint16_t MwSaveCampaignCallback(void);
uint16_t MwShowOptionsMessageAndReturnContinuation(const MwNativeByteView *text,uint16_t textOffset);
uint16_t MwShowLoadCampaignCallback(void);
bool MwEnumerateCampaignSaves(void);
uint16_t MwLoadSelectedCampaignCallback(void);
void MwInitializeNewsArticleCursor(void);
bool MwFindNextNewsArticle(uint16_t *articleId);
bool MwFindPreviousNewsArticle(uint16_t *articleId);
uint16_t MwOpenNewsNetContinuation(void);
uint16_t MwNextNewsArticleContinuation(void);
uint16_t MwPreviousNewsArticleContinuation(void);
void MwDrawNewsBoundaryMessage(void);
void MwRestoreNewsArticleRegion(void);
#endif
