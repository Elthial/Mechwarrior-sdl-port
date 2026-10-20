#include "mechwarrior/campaign_continuations.h"
#include "mechwarrior/campaign_options.h"
#include "mechwarrior/campaign_input.h"
#include "mechwarrior/dos_files.h"
#include "mechwarrior/ega_campaign.h"
#include <stdlib.h>
#include <string.h>

enum { SAVE_NAME_BOX_X=0x0028,SAVE_NAME_BOX_Y=0x0064,SAVE_NAME_BOX_WIDTH=0x00F0,SAVE_NAME_BOX_HEIGHT=0x0038,
    SAVE_NAME_LABEL_X=0x0038,SAVE_NAME_LABEL_Y=0x0074,SAVE_NAME_LABEL_COLOUR=Campaign_Pal_EGA_White,
    SAVE_NAME_FIELD_X=0x007C,SAVE_NAME_FIELD_Y=0x0084,SAVE_NAME_FIELD_COLOUR=Campaign_Pal_EGA_Red,SAVE_PANEL_BACKGROUND=Campaign_Pal_EGA_DarkGrey,
    SAVE_MESSAGE_INSET=0x0010,SAVE_MESSAGE_COLUMNS=0x001B,SAVE_MESSAGE_ROWS=6,DOS_LAST_FLOPPY_DRIVE=1 };
/* Text cursors in the original campaign data; not drawing colours. */
enum { SAVE_NAME_TEXT_OFFSET=0xA031, SAVE_DISK_TEXT_OFFSET=0xA114,
    SAVE_ERROR_TEXT_OFFSET=0xA0B3, SAVE_FULL_TEXT_OFFSET=0xA088,
    LOAD_ERROR_TEXT_OFFSET=0xA0E3 };

/* MW_MAIN081B:104C..1055. Attributes CX=0 at the DOS boundary. */
bool MwDosCreateSaveFile(const char *filename,uint16_t *handleOrError)
{
    if (!mwDosFiles.create) abort();
    return mwDosFiles.create(filename,handleOrError);
}
/* MW_MAIN081B:1066..106C. True means native CF set; no retries. */
bool MwDosWriteFile(const uint8_t *buffer,uint16_t byteCount,uint16_t fileHandle,uint16_t *bytesWrittenOrError)
{
    if (!mwDosFiles.write) abort();
    return mwDosFiles.write(buffer,byteCount,fileHandle,bytesWrittenOrError);
}

/* MW_MAIN081B:6D52..6E6D; invalid embedded/leading spaces return6D53.
 * Typed serialization occurs only at the actual1066 binary IO boundary. */
uint16_t MwSaveCampaignCallback(void)
{
readName:
    optionsMenuSelection=OPTION_SAVE;
    EgaCopyDrawPageToDisplay(0,0,EGA_COLUMNS,EGA_ROWS);
    EgaFillDisplayPageRectangle(SAVE_NAME_BOX_X,SAVE_NAME_BOX_Y,SAVE_NAME_BOX_WIDTH,SAVE_NAME_BOX_HEIGHT,SAVE_PANEL_BACKGROUND);
    MwDrawPicturePanelBorder(SAVE_NAME_BOX_X,SAVE_NAME_BOX_Y,SAVE_NAME_BOX_WIDTH,SAVE_NAME_BOX_HEIGHT);
    EgaSelectFont(TEXT_PANEL_MONOSPACE_FONT);
    MwNativeByteView prompt={saveNamePrompt,SAVE_NAME_TEXT_OFFSET,sizeof(saveNamePrompt)};
    EgaDrawTransparentText(SAVE_NAME_LABEL_X,SAVE_NAME_LABEL_Y,SAVE_NAME_LABEL_COLOUR,&prompt,prompt.nativeBase);
    uint16_t transfer=MwReadCampaignTextInput(SAVE_NAME_FIELD_X,SAVE_NAME_FIELD_Y,CAMPAIGN_SAVE_STEM_BYTES+1,SAVE_NAME_FIELD_COLOUR,SAVE_PANEL_BACKGROUND);
    if (transfer) return transfer;
    uint16_t leadingSpaces=0;
    while (campaignTextInputBuffer[leadingSpaces]==' ') ++leadingSpaces;
    if (!campaignTextInputBuffer[leadingSpaces]) return CAMPAIGN_CONTINUE_OPTIONS;
    uint16_t nameLength=0;
    while (campaignTextInputBuffer[nameLength])
    {
        if (campaignTextInputBuffer[nameLength]==' ') goto readName;
        ++nameLength;
    }
    memcpy(campaignTextInputBuffer+nameLength,".GAM",sizeof(".GAM"));
    if (!mwCampaignInput.currentDefaultDrive) abort();
    uint8_t drive=mwCampaignInput.currentDefaultDrive();
    if (drive<=DOS_LAST_FLOPPY_DRIVE)
    {
        saveDiskPrompt.driveLetter=(uint8_t)(drive+'A');
        MwIndexedPictureCloseHook();
        EgaCopyDrawPageToDisplay(0,0,EGA_COLUMNS,EGA_ROWS);
        MwNativeByteView text={(const uint8_t *)&saveDiskPrompt,SAVE_DISK_TEXT_OFFSET,sizeof(saveDiskPrompt)};
        (void)MwShowMonospaceTextPanel(SAVE_MESSAGE_INSET,SAVE_MESSAGE_INSET,SAVE_MESSAGE_COLUMNS,SAVE_MESSAGE_ROWS,&text,text.nativeBase);
    }
    uint16_t handleOrError;
    if (MwDosCreateSaveFile((const char *)campaignTextInputBuffer+leadingSpaces,&handleOrError))
    {
        MwNativeByteView text={saveDiskErrorText,SAVE_ERROR_TEXT_OFFSET,sizeof(saveDiskErrorText)};
        return MwShowOptionsMessageAndReturnContinuation(&text,text.nativeBase);
    }
    saveFileHandle=handleOrError;
    uint8_t bytes[CAMPAIGN_SAVE_BYTES];MwEncodeCampaignExchange(bytes);
    uint16_t bytesWrittenOrError;
    (void)MwDosWriteFile(bytes,CAMPAIGN_SAVE_BYTES,handleOrError,&bytesWrittenOrError);
    (void)MwDosCloseFile(saveFileHandle);
    if (bytesWrittenOrError!=CAMPAIGN_SAVE_BYTES) /* Native ignores write CF and close result. */
    {
        MwNativeByteView text={saveDiskFullText,SAVE_FULL_TEXT_OFFSET,sizeof(saveDiskFullText)};
        return MwShowOptionsMessageAndReturnContinuation(&text,text.nativeBase);
    }
    return CAMPAIGN_CONTINUE_OPTIONS;
}

/* MW_MAIN081B:6E7D..6EA2. DI is saved across page copy.6E6A rejoins6CFA. */
uint16_t MwShowOptionsMessageAndReturnContinuation(const MwNativeByteView *text,uint16_t textOffset)
{
    EgaCopyDrawPageToDisplay(0,0,EGA_COLUMNS,EGA_ROWS);
    (void)MwShowMonospaceTextPanel(SAVE_MESSAGE_INSET,SAVE_MESSAGE_INSET,SAVE_MESSAGE_COLUMNS,SAVE_MESSAGE_ROWS,text,textOffset);
    return CAMPAIGN_CONTINUE_OPTIONS;
}

/* MW_MAIN081B:6EA3..6EEA. Clear exactly eight filename bytes per0012 row;
 * neither the row number nor ninth filename byte is initialized here. */
uint16_t MwShowLoadCampaignCallback(void)
{
    optionsMenuSelection=OPTION_RESTORE;
    EgaCopyDrawPageToDisplay(0,0,EGA_COLUMNS,EGA_ROWS);
    for (uint16_t row=0;row<CAMPAIGN_SAVE_ROWS;++row)
        for (uint16_t column=0;column<CAMPAIGN_SAVE_STEM_BYTES;++column)
            saveListRows[row].caption.columns.filename[column]=' ';
    (void)MwEnumerateCampaignSaves(); /* Caller ignores CF. */
    MwDrawDescriptorTextPanel(&restorePanel);
    MwSelectUiRecordTable(0,&restoreMenu);
    return CAMPAIGN_CONTINUE_UI;
}

/* MW_MAIN081B:6EEB..6F4B. One DOS DTA filename owner is updated by every
 * FindNext. Keep its reference, not copies of selector/offset artifacts. */
bool MwEnumerateCampaignSaves(void)
{
    if (!mwDosFiles.getDtaFilename || !mwDosFiles.findFirst || !mwDosFiles.findNext) abort();
    saveSearchFilename=mwDosFiles.getDtaFilename();
    saveFileCount=0;
    if (mwDosFiles.findFirst(saveFindPattern,0)) return true;
    saveListWriteRow=saveListRows;
    do
    {
        const char *filename=saveSearchFilename;
        char *stem=saveListWriteRow->caption.columns.filename;
        while (*filename!='.') *stem++=*filename++;
        *stem='\0';
        ++saveListWriteRow;
        if (++saveFileCount==CAMPAIGN_SAVE_ROWS) break;
    } while (!mwDosFiles.findNext());
    return false; /* Native swallows every FindNext end/error. */
}

/* Restore a complete GAM record without replacing the company on failed IO.
 * The extra read byte detects oversized files; it is not campaign data. */
uint16_t MwLoadSelectedCampaignCallback(void)
{
    uint16_t row=(uint8_t)selectedUiRecordIndex; /* Native MUL BL uses AL only. */
    if (row>=CAMPAIGN_SAVE_ROWS) abort(); /* Out-of-row native storage is unrepresented. */
    const char *stem=saveListRows[row].caption.columns.filename;
    if (*stem==' ') return 0;
    uint16_t character=0;
    while (stem[character])
    {
        selectedSaveFilename[character]=stem[character];++character;
    }
    memcpy(selectedSaveFilename+character,".GAM",sizeof(".GAM"));
    uint16_t handleOrError;
    if (MwDosOpenResourceReadWrite(selectedSaveFilename,&handleOrError)) goto diskError;
    saveFileHandle=handleOrError;
    uint8_t bytes[CAMPAIGN_SAVE_BYTES+1];
    uint16_t bytesReadOrError=0;
    bool readError=MwDosReadFile(bytes,sizeof(bytes),handleOrError,&bytesReadOrError);
    bool closeError=MwDosCloseFile(saveFileHandle);
    if (readError || closeError || bytesReadOrError!=CAMPAIGN_SAVE_BYTES) goto diskError;
    MwDecodeCampaignExchange(bytes);
    hubSelectedRecordIndex=0;
    MwIndexedPictureCloseHook(); /* RET-only: retain actual directory/cache state. */
    return CAMPAIGN_CONTINUE_PLANET_HUB;
diskError:
    {
        MwNativeByteView text={loadDiskErrorText,LOAD_ERROR_TEXT_OFFSET,sizeof(loadDiskErrorText)};
        return MwShowOptionsMessageAndReturnContinuation(&text,text.nativeBase);
    }
}
