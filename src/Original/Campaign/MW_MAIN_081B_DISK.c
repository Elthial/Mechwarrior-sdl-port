#include "mechwarrior/campaign_ui.h"
#include "mechwarrior/campaign_input.h"

/* MW_MAIN081B:1280..129B.129B is a plain RET: mode1's discarded return
 * bypasses just that RET, reaching the same caller without further work. */
void MwPromptChangeCurrentDriveDisk(void)
{
    if (mwCampaignInput.currentDefaultDrive == NULL) abort();
    changeDiskPrompt.driveLetter = (uint8_t)(mwCampaignInput.currentDefaultDrive() + 'A');
    MwNativeByteView text = { (const uint8_t *)&changeDiskPrompt, 0x0E71, sizeof(changeDiskPrompt) };
    (void)MwShowMonospaceTextPanel(0x0010,0x0010,0x0022,0x0008,&text,0x0E71);
}
/* MW_MAIN081B:129C..12B7. Same RET-only continuation rule as1280. */
void MwPromptInsertDiskOne(void)
{
    if (mwCampaignInput.currentDefaultDrive == NULL) abort();
    insertDiskOnePrompt.driveLetter = (uint8_t)(mwCampaignInput.currentDefaultDrive() + 'A');
    MwNativeByteView text = { (const uint8_t *)&insertDiskOnePrompt, 0x0ED6, sizeof(insertDiskOnePrompt) };
    (void)MwShowMonospaceTextPanel(0x0010,0x0010,0x0020,0x0005,&text,0x0ED6);
}
