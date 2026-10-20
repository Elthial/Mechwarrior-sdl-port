#include "mechwarrior/campaign_continuations.h"
#include "mechwarrior/campaign_options.h"
#include "mechwarrior/ega_campaign.h"
#include <stdlib.h>
enum { OPTIONS_SOUND_X=0x0068,OPTIONS_SOUND_Y=0x006E,OPTIONS_DETAIL_X=0x0070,OPTIONS_DETAIL_Y=0x0078,
    AUDIO_OPTION_LABEL_COUNT=2,TERRAIN_DETAIL_LABEL_COUNT=3 };

/* MW_MAIN081B:6CFA..6D4D;6E6A PUSH/JMP re-enters the same redraw.
 * The return value transports the native JMP, not an AX source result. */
uint16_t MwEnterOptionsMenuContinuation(uint16_t entryOffset)
{
    if (entryOffset==CAMPAIGN_CONTINUE_OPTIONS_REDRAW) goto publishMenu;
    if (entryOffset!=CAMPAIGN_CONTINUE_OPTIONS) abort();
    hubSelectedRecordIndex=OPTIONS_HUB_ROW;
    EgaSelectFont(TEXT_PANEL_MONOSPACE_FONT);
    EgaClearDrawPage(0);
    MwDrawIndexedPicture(0,0,(uint16_t)(currentPlanetLandscapeIndex+OPTIONS_LANDSCAPE_PICTURE_BASE));
publishMenu:
    EgaCopyDrawPageToDisplay(0,0,EGA_COLUMNS,EGA_ROWS);
    MwUpdateAudioOptionLabel();
    MwUpdateTerrainDetailOptionLabel();
    MwDrawDescriptorTextPanel(&optionsPanel);
    MwSelectUiRecordTable(optionsMenuSelection,&optionsMenu);
    return CAMPAIGN_CONTINUE_UI;
}

/* MW_MAIN081B:6FDE..6FF9. The NUL is copied; no clearing of the old tail. */
void MwUpdateAudioOptionLabel(void)
{
    if (audioOptionLabelIndex>=AUDIO_OPTION_LABEL_COUNT) abort(); /* Unrepresented out-of-table native alias. */
    uint16_t character=0;
    do
    {
        optionsLabels[OPTION_SOUND].text[OPTIONS_SOUND_LABEL_COLUMN+character]=audioOptionLabels[audioOptionLabelIndex][character];
    } while (audioOptionLabels[audioOptionLabelIndex][character++]!='\0');
}

/* MW_MAIN081B:702B..7046. BYTE detail times5, including trailing space/NUL. */
void MwUpdateTerrainDetailOptionLabel(void)
{
    if (campaignTerrainDetailLevel>=TERRAIN_DETAIL_LABEL_COUNT) abort(); /* Unrepresented out-of-table native alias. */
    uint16_t character=0;
    do
    {
        optionsLabels[OPTION_DETAIL].text[OPTIONS_DETAIL_LABEL_COLUMN+character]=terrainDetailLabels[campaignTerrainDetailLevel][character];
    } while (terrainDetailLabels[campaignTerrainDetailLevel][character++]!='\0');
}

/* MW_MAIN081B:6FB9..6FDD. Ordinary callback; the flag toggles without
 * an immediate music start/stop call. Opaque ordinal0F uses current font. */
void MwToggleAudioOption(void)
{
    optionsMenuSelection=OPTION_SOUND;
    audioOptionLabelIndex^=1;
    MwUpdateAudioOptionLabel();
    MwNativeByteView text={(const uint8_t *)optionsLabels[OPTION_SOUND].text,0x9D97,sizeof(optionsLabels[OPTION_SOUND].text)};
    uint16_t y=OPTIONS_SOUND_Y,offset=0x9D97;
    EgaDrawString(OPTIONS_SOUND_X,&y,(CAMPAIGN_DARK_GREY<<EGA_BYTE_BITS)|CAMPAIGN_YELLOW,&text,&offset);
}

/* MW_MAIN081B:6FFA..702A. Native JL compares the incremented BYTE signed. */
void MwCycleTerrainDetailOption(void)
{
    optionsMenuSelection=OPTION_DETAIL;
    ++campaignTerrainDetailLevel;
    if ((int8_t)campaignTerrainDetailLevel>=TERRAIN_DETAIL_LABEL_COUNT) campaignTerrainDetailLevel=0;
    MwUpdateTerrainDetailOptionLabel();
    MwNativeByteView text={(const uint8_t *)optionsLabels[OPTION_DETAIL].text,0x9DAB,sizeof(optionsLabels[OPTION_DETAIL].text)};
    uint16_t y=OPTIONS_DETAIL_Y,offset=0x9DAB;
    EgaDrawString(OPTIONS_DETAIL_X,&y,(CAMPAIGN_DARK_GREY<<EGA_BYTE_BITS)|CAMPAIGN_YELLOW,&text,&offset);
}

/* MW_MAIN081B:7047..707E. Mutate the SAME heading and YES-record owner
 * used by restart707F; the NO record rejoins6D23 without rebuilding scenery. */
uint16_t MwConfirmQuitCampaignContinuation(void)
{
    optionsMenuSelection=OPTION_QUIT;
    EgaCopyDrawPageToDisplay(0,0,EGA_COLUMNS,EGA_ROWS);
    confirmationPanel.lines[0]=&confirmationLabels[0].display;
    confirmationMenu.records[0].activateCodeOffset=CAMPAIGN_CONTINUE_QUIT_FROM_OPTIONS;
    MwDrawDescriptorTextPanel(&confirmationPanel);
    MwSelectUiRecordTable(0,&confirmationMenu);
    return CAMPAIGN_CONTINUE_UI;
}

/* MW_MAIN081B:707F..70B6, separate native callback/entry, not a new helper. */
uint16_t MwConfirmRestartCampaignContinuation(void)
{
    optionsMenuSelection=OPTION_RESTART;
    EgaCopyDrawPageToDisplay(0,0,EGA_COLUMNS,EGA_ROWS);
    confirmationPanel.lines[0]=&confirmationLabels[1].display;
    confirmationMenu.records[0].activateCodeOffset=CAMPAIGN_CONTINUE_RESTART_FROM_OPTIONS;
    MwDrawDescriptorTextPanel(&confirmationPanel);
    MwSelectUiRecordTable(0,&confirmationMenu);
    return CAMPAIGN_CONTINUE_UI;
}

/* MW_MAIN081B:6D4E..6D51. POP/JMP cleanup; do not substitute683D. */
uint16_t MwQuitFromOptionsContinuation(void)
{
    return CAMPAIGN_CONTINUE_CLEANUP;
}

/* MW_MAIN081B:70B7..70C1. PUSH balances6855's inherited frame. */
uint16_t MwRestartFromOptionsContinuation(void)
{
    hubSelectedRecordIndex=0;
    return CAMPAIGN_CONTINUE_RESTART;
}
