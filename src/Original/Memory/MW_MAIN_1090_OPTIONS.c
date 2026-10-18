/* Native1090 options, persistence UI and News Net data owners. The installation
 * decoder is boundary code, not a memory registry or an original game helper. */
#include "mechwarrior/campaign_options.h"
#include "mechwarrior/campaign_pilots.h"
#include <string.h>

MwPanelDescriptor optionsPanel,restorePanel,confirmationPanel,newsMenuPanel;
MwServiceCaption optionsLabels[CAMPAIGN_OPTIONS_ROWS],confirmationLabels[4],newsMenuLabels[3];
MwServiceCaption restoreHeading,restoreCancel;
MwSaveListRow saveListRows[CAMPAIGN_SAVE_ROWS];
uint16_t saveFileHandle,saveFileCount;
const char *saveSearchFilename;
MwSaveListRow *saveListWriteRow;
char selectedSaveFilename[CAMPAIGN_SAVE_FILENAME_BYTES],saveFindPattern[6];
uint8_t saveNamePrompt[0x1B];
char audioOptionLabels[2][4],terrainDetailLabels[3][5];
uint8_t saveDiskFullText[0x2B],saveDiskErrorText[0x30],loadDiskErrorText[0x31];
MwSaveDiskPrompt saveDiskPrompt;
uint8_t newsAtStart,newsAtEnd;
uint16_t newsLastArticleId;
char newsBoundaryText[0x1B];
MwCampaignEvent **newsArticleCursor,**newsArticleListStart;
static MwUiRecord optionsRecords[CAMPAIGN_OPTIONS_ROWS],restoreRecords[CAMPAIGN_SAVE_ROWS+1];
static MwUiRecord confirmationRecords[2],newsRecords[CAMPAIGN_NEWS_ROWS];
/* No converted writer owns these out-of-menu native FF-link probes. */
static uint8_t optionsNavigationProbe,restoreNavigationProbe,confirmationNavigationProbe,newsNavigationProbe;
MwUiMenu optionsMenu={optionsRecords,CAMPAIGN_OPTIONS_ROWS,&optionsNavigationProbe};
MwUiMenu restoreMenu={restoreRecords,CAMPAIGN_SAVE_ROWS+1,&restoreNavigationProbe};
MwUiMenu confirmationMenu={confirmationRecords,2,&confirmationNavigationProbe};
MwUiMenu newsMenu={newsRecords,CAMPAIGN_NEWS_ROWS,&newsNavigationProbe};

static bool ImportCaption(const uint8_t *data,uint16_t offset,MwPanelText *line,char *text,size_t capacity)
{
    const uint8_t *end=memchr(data+offset+5,0,capacity);
    if (!end) return false;
    line->x=MwReadNativeWord(data,offset);line->y=MwReadNativeWord(data,offset+2);
    line->colour=data[offset+4];line->text=text;
    memcpy(text,data+offset+5,(size_t)(end-(data+offset+5))+1);return true;
}
static void ImportPanel(const uint8_t *data,uint16_t offset,MwPanelDescriptor *panel)
{
    panel->x=MwReadNativeWord(data,offset);panel->y=MwReadNativeWord(data,offset+2);
    panel->width=MwReadNativeWord(data,offset+4);panel->bottomInset=MwReadNativeWord(data,offset+6);
    panel->lineCount=MwReadNativeWord(data,offset+8);panel->borderEnabled=MwReadNativeWord(data,offset+10);
    panel->lineSpacing=MwReadNativeWord(data,offset+12);panel->background=MwReadNativeWord(data,offset+14);
}
static void ImportMenu(const uint8_t *data,uint16_t offset,MwUiMenu *menu,MwPanelText **captions)
{
    for (uint16_t row=0;row<menu->recordCount;++row)
    {
        const uint8_t *encoded=data+offset+row*CAMPAIGN_UI_RECORD_BYTES;
        MwUiRecord *record=&menu->records[row];
        record->enabled=encoded[0];record->textRecord=encoded[1];
        record->x=MwReadNativeWord(encoded,2);record->y=MwReadNativeWord(encoded,4);
        record->activateCodeOffset=MwReadNativeWord(encoded,6);record->pictureIndex=MwReadNativeWord(encoded,8);
        record->text=captions[row]->text;record->textColour=&captions[row]->colour;
        record->upRecordIndex=encoded[10];record->downRecordIndex=encoded[11];
        record->leftRecordIndex=encoded[12];record->rightRecordIndex=encoded[13];
    }
}
bool MwImportCampaignOptionsData(const uint8_t *data,size_t byteCount)
{
    if (!data || byteCount<NATIVE_SEGMENT_BYTE_COUNT) return false;
    static const uint16_t optionOffsets[CAMPAIGN_OPTIONS_ROWS]={0x9D70,0x9D80,0x9D92,0x9DA6,0x9DB8,0x9DCA,0x9DDB};
    static const uint16_t confirmationOffsets[4]={0x9CD4,0x9CFA,0x9D23,0x9D2C};
    static const uint16_t newsOffsets[3]={0x2516,0x2524,0x252E};
    ImportPanel(data,0x9D52,&optionsPanel);ImportPanel(data,0x9E4D,&restorePanel);
    ImportPanel(data,0x9CBE,&confirmationPanel);ImportPanel(data,0x2500,&newsMenuPanel);
    if (optionsPanel.lineCount!=7 || restorePanel.lineCount!=14
        || confirmationPanel.lineCount!=3 || newsMenuPanel.lineCount!=3) return false;
    for (uint16_t row=0;row<CAMPAIGN_OPTIONS_ROWS;++row)
    {
        MwServiceCaption *caption=&optionsLabels[row];
        if (!ImportCaption(data,optionOffsets[row],&caption->display,caption->text,sizeof(caption->text))) return false;
        optionsPanel.lines[row]=&caption->display;
    }
    for (uint16_t row=0;row<4;++row)
    {
        MwServiceCaption *caption=&confirmationLabels[row];
        if (!ImportCaption(data,confirmationOffsets[row],&caption->display,caption->text,sizeof(caption->text))) return false;
    }
    confirmationPanel.lines[0]=&confirmationLabels[0].display;
    confirmationPanel.lines[1]=&confirmationLabels[2].display;
    confirmationPanel.lines[2]=&confirmationLabels[3].display;
    for (uint16_t row=0;row<3;++row)
    {
        MwServiceCaption *caption=&newsMenuLabels[row];
        if (!ImportCaption(data,newsOffsets[row],&caption->display,caption->text,sizeof(caption->text))) return false;
        newsMenuPanel.lines[row]=&caption->display;
    }
    if (!ImportCaption(data,0x9E79,&restoreHeading.display,restoreHeading.text,sizeof(restoreHeading.text))
        || !ImportCaption(data,0x9F6C,&restoreCancel.display,restoreCancel.text,sizeof(restoreCancel.text))) return false;
    restorePanel.lines[0]=&restoreHeading.display;
    for (uint16_t row=0;row<CAMPAIGN_SAVE_ROWS;++row)
    {
        MwSaveListRow *entry=&saveListRows[row];
        if (!ImportCaption(data,(uint16_t)(0x9E94+row*0x12),&entry->display,entry->caption.text,sizeof(entry->caption.text))) return false;
        restorePanel.lines[row+1]=&entry->display;
    }
    restorePanel.lines[13]=&restoreCancel.display;
    ImportMenu(data,0x9DEB,&optionsMenu,optionsPanel.lines);
    barMenu.sentinelEnabled=&optionsMenu.records[5].rightRecordIndex; /*1090:9E3E*/
    ImportMenu(data,0x9F7B,&restoreMenu,restorePanel.lines+1);
    ImportMenu(data,0x9D34,&confirmationMenu,confirmationPanel.lines+1);
    ImportMenu(data,0x2538,&newsMenu,newsMenuPanel.lines);
    optionsNavigationProbe=data[0xABDD];restoreNavigationProbe=data[0xAD6D];
    confirmationNavigationProbe=data[0xAB26];newsNavigationProbe=data[0x332A];
    saveFileHandle=MwReadNativeWord(data,0xA04C);saveFileCount=MwReadNativeWord(data,0xA04E);
    memcpy(selectedSaveFilename,data+0xA064,sizeof(selectedSaveFilename));
    memcpy(saveFindPattern,data+0xA056,sizeof(saveFindPattern));
    memcpy(saveNamePrompt,data+0xA031,sizeof(saveNamePrompt));
    memcpy(audioOptionLabels,data+0xA071,sizeof(audioOptionLabels));
    memcpy(terrainDetailLabels,data+0xA079,sizeof(terrainDetailLabels));
    memcpy(saveDiskFullText,data+0xA088,sizeof(saveDiskFullText));
    memcpy(saveDiskErrorText,data+0xA0B3,sizeof(saveDiskErrorText));
    memcpy(loadDiskErrorText,data+0xA0E3,sizeof(loadDiskErrorText));
    memcpy(&saveDiskPrompt,data+0xA114,sizeof(saveDiskPrompt));
    newsAtStart=data[0x257F];newsAtEnd=data[0x2580];newsLastArticleId=MwReadNativeWord(data,0x2562);
    memcpy(newsBoundaryText,data+0x2564,sizeof(newsBoundaryText));
    newsArticleCursor=NULL;newsArticleListStart=NULL;
    saveSearchFilename=NULL;saveListWriteRow=NULL;
    return true;
}
