#include "mechwarrior/launcher.h"
#include <string.h>

/* Native resident exchange outlives both child executables. Only transport
 * owns these encoded bytes; campaign and combat use their own typed objects. */
MwCampaignTransferStorage campaignTransfer;
uint8_t childGraphicsCode,childAudioCode,childPhase,childMemoryTier;
uint8_t combatGraphicsCode,campaignGraphicsCode;
uint8_t launcherIdentity[11];
char launcherCampaignFilename[12],launcherCombatFilename[10];
char launcherReleaseError[0x17],launcherCampaignDiskPrompt[0x1C];
char launcherCombatDiskPrompt[0x1C],launcherMemoryError[0x21];
char launcherGraphicsPrompt[0xE7],launcherDiskSuffix[0x39];

/* Installation boundary only. No retained launcher code/segment image. */
bool MwImportLauncherInitialData(const uint8_t *data,size_t byteCount)
{
    if (!data || byteCount<0x09EE) return false;
    memcpy(campaignCombatExchange,data+0x0004,sizeof(campaignCombatExchange));
    memcpy(launcherIdentity,data+0x07D4,sizeof(launcherIdentity));
    memcpy(launcherCampaignFilename,data+0x07E3,sizeof(launcherCampaignFilename));
    memcpy(launcherCombatFilename,data+0x07EF,sizeof(launcherCombatFilename));
    memcpy(launcherReleaseError,data+0x085A,sizeof(launcherReleaseError));
    memcpy(launcherCampaignDiskPrompt,data+0x0871,sizeof(launcherCampaignDiskPrompt));
    memcpy(launcherCombatDiskPrompt,data+0x088D,sizeof(launcherCombatDiskPrompt));
    memcpy(launcherMemoryError,data+0x08A9,sizeof(launcherMemoryError));
    memcpy(launcherGraphicsPrompt,data+0x08CA,sizeof(launcherGraphicsPrompt));
    memcpy(launcherDiskSuffix,data+0x09B1,sizeof(launcherDiskSuffix));
    childGraphicsCode=data[0x0808];childAudioCode=data[0x080A];
    childPhase=data[0x080C];childMemoryTier=data[0x0811];
    combatGraphicsCode=data[0x09EA];campaignGraphicsCode=data[0x09EB];
    return launcherCampaignFilename[11]==0 && launcherCombatFilename[9]==0;
}
