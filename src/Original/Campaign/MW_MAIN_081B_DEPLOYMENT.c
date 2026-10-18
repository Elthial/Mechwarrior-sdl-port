#include "mechwarrior/campaign_continuations.h"
#include "mechwarrior/campaign_handoff.h"
#include "mechwarrior/campaign_startup.h"
#include "mechwarrior/campaign_travel.h"
#include "mechwarrior/campaign_ui.h"
#include <stdlib.h>
#include <string.h>

/* MW_MAIN081B:1423..144D. The successful close result is ignored. */
void MwWaitForRequiredResourceFile(void)
{
    uint16_t fileHandle;
    while (MwDosOpenResourceReadWrite(combatExecutableFilename,&fileHandle))
    {
        if (!mwCampaignInput.currentDefaultDrive) abort();
        combatDiskPrompt.driveLetter=mwCampaignInput.currentDefaultDrive()+'A';
        MwNativeByteView text={(const uint8_t *)&combatDiskPrompt,0x0F00,sizeof(combatDiskPrompt)};
        (void)MwShowMonospaceTextPanel(0x0010,0x0010,0x0024,8,&text,0x0F00);
    }
    (void)MwDosCloseFile(fileHandle);
}

/* MW_MAIN081B:1C3E..1CCB. Readiness falls here; POP AX discards the callback
 * return before JMP1CD4/015F. Return codes below transport those JMPs only. */
uint16_t MwResolveDeploymentOrExportCombatExchangeContinuation(void)
{
    MwPlayHiredTransportAnimation();
    /* Native REP STOSB clears0451..0467, NOT the accepted flag0468. */
    memset(rosterStatus,0,sizeof(rosterStatus));
    memset(salvageEntries,0,sizeof(salvageEntries));
    returnedDisabledEnemyCount=0;
    combatTransitionResult=0;
    if ((int8_t)combatScenarioCode<=DEPLOYMENT_BYPASS_LAST_SCENARIO
        && combatScenarioCode!=DEPLOYMENT_NO_BYPASS_SCENARIO)
    {
        if (MwRandomUpTo(PERCENT_DIVISOR)<=DEPLOYMENT_BYPASS_ROLL_LIMIT)
        {
            for (uint16_t slot=0;slot<CAMPAIGN_SALVAGE_SLOTS;++slot)
            {
                salvageEntries[slot].unitKind=0xFF;
                salvageEntries[slot].leadingZeroByte=0xFF; /* Native WORD FFFF. */
            }
            acceptedContractActive=0;
            deploymentResolvedWithoutCombat=1;
            return CAMPAIGN_CONTINUE_AFTER_ACTION;
        }
    }
    MwWaitForRequiredResourceFile();
    missionCategoryOrMemoryTier=(uint8_t)launcherMemoryTier;
    deploymentTerrainDetailLevel=campaignTerrainDetailLevel;
    /* Real launcher and child allocations are disjoint. Encode at their
     * transfer boundary; do not fabricate a live1090 image for this copy.
     * Native copies forward0718 bytes to resident0806:0004; no cursor wraps
     * for that actual allocation. Arbitrary aliased foreign PSPs are not a
     * supported launcher configuration, nor silently treated as memcpy. */
    MwEncodeCampaignExchange(campaignCombatExchange);
    launcherExitCode=CAMPAIGN_EXIT_COMBAT;
    return CAMPAIGN_CONTINUE_CLEANUP;
}
