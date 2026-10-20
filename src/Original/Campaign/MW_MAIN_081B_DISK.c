#include "mechwarrior/campaign_ui.h"
#include "mechwarrior/campaign_input.h"

/* Original text cursors, not EGA palette indices. */
enum { CHANGE_DISK_TEXT_OFFSET=0x0E71, INSERT_DISK_ONE_TEXT_OFFSET=0x0ED6,
    DISK_PROMPT_INSET=16, CHANGE_DISK_COLUMNS=34, CHANGE_DISK_ROWS=8,
    INSERT_DISK_ONE_COLUMNS=32, INSERT_DISK_ONE_ROWS=5 };

/* MW_MAIN081B:1280..129B.129B is a plain RET: mode1's discarded return
 * bypasses just that RET, reaching the same caller without further work. */
void MwPromptChangeCurrentDriveDisk(void)
{
    if (mwCampaignInput.currentDefaultDrive == NULL) abort();
    changeDiskPrompt.driveLetter = (uint8_t)(mwCampaignInput.currentDefaultDrive() + 'A');
    MwNativeByteView text = { (const uint8_t *)&changeDiskPrompt, CHANGE_DISK_TEXT_OFFSET, sizeof(changeDiskPrompt) };
    (void)MwShowMonospaceTextPanel(DISK_PROMPT_INSET,DISK_PROMPT_INSET,CHANGE_DISK_COLUMNS,CHANGE_DISK_ROWS,&text,text.nativeBase);
}
/* MW_MAIN081B:129C..12B7. Same RET-only continuation rule as1280. */
void MwPromptInsertDiskOne(void)
{
    if (mwCampaignInput.currentDefaultDrive == NULL) abort();
    insertDiskOnePrompt.driveLetter = (uint8_t)(mwCampaignInput.currentDefaultDrive() + 'A');
    MwNativeByteView text = { (const uint8_t *)&insertDiskOnePrompt, INSERT_DISK_ONE_TEXT_OFFSET, sizeof(insertDiskOnePrompt) };
    (void)MwShowMonospaceTextPanel(DISK_PROMPT_INSET,DISK_PROMPT_INSET,INSERT_DISK_ONE_COLUMNS,INSERT_DISK_ONE_ROWS,&text,text.nativeBase);
}
