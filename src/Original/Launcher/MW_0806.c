#include "mechwarrior/port_diagnostics.h"
#include "mechwarrior/launcher.h"
#include <stdlib.h>

MwLauncherSystemCalls mwLauncherSystem;

/* MW0806:09EE..0C25. Hardware detection stays at the BIOS boundary; PSP/FCB
 * pointers are typed launch transport. DOS EXEC stack protection is host call
 * ownership, not a synthetic C frame. Native termination is always code00. */
uint8_t MwLauncherMain(const MwLauncherCommandLine *commandLine)
{
    MwCampaignLaunch launch;
    uint8_t childExitCode;
    if (!mwLauncherSystem.resizeResident || !mwLauncherSystem.allocateMemory
        || !mwLauncherSystem.freeMemory || !mwLauncherSystem.detectDisplayAdapter
        || !mwLauncherSystem.clearTextScreen || !mwLauncherSystem.executeChild
        || !mwLauncherSystem.displayDosString || !mwLauncherSystem.writeConsoleChar
        || !mwCampaignInput.readKeyboard || !mwCampaignInput.currentDefaultDrive) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !mwLauncherSystem.resizeResident || !mwLauncherSystem.allocateMemory || !mwLauncherSystem.freeMemory || !mwLauncherSystem.detectDisplayAdapter || !mwLauncherSystem.clearTextScreen || !mwLauncherSystem.executeChild || !mwLauncherSystem.displayDosString || !mwLauncherSystem.writeConsoleChar || !mwCampaignInput.readKeyboard || !mwCampaignInput.currentDefaultDrive");
    if (mwLauncherSystem.resizeResident(LAUNCHER_RESIDENT_PARAGRAPHS))
    {
        mwLauncherSystem.displayDosString(launcherReleaseError);
        return 0;
    }
    void *memoryProbe=mwLauncherSystem.allocateMemory(LAUNCHER_MINIMUM_PARAGRAPHS);
    if (!memoryProbe)
    {
        mwLauncherSystem.displayDosString(launcherMemoryError);
        return 0;
    }
    mwLauncherSystem.freeMemory(memoryProbe);
    memoryProbe=mwLauncherSystem.allocateMemory(LAUNCHER_LARGER_PARAGRAPHS);
    if (!memoryProbe) childMemoryTier=LAUNCHER_MEMORY_MINIMUM;
    else
    {
        mwLauncherSystem.freeMemory(memoryProbe);
        childMemoryTier=LAUNCHER_MEMORY_512K;
    }
    for (uint16_t i=0;i<commandLine->length;++i)
    {
        switch (commandLine->text[i])
        {
        case '1':goto ega;
        case '2':goto tandy;
        case '3':goto cga;
        }
    }
    uint16_t adapter=mwLauncherSystem.detectDisplayAdapter();
    if ((uint8_t)adapter!=LAUNCHER_ADAPTER_UNSUPPORTED)
    {
        if (adapter==LAUNCHER_ADAPTER_TANDY) goto tandy;
        goto ega;
    }
selectGraphics:
    mwLauncherSystem.displayDosString(launcherGraphicsPrompt);
    for (;;)
    {
        switch (mwCampaignInput.readKeyboard())
        {
        case CAMPAIGN_KEY_ESCAPE:return 0;
        case 0x0231:goto ega;
        case 0x0332:goto tandy;
        case 0x0433:goto cga;
        }
    }
ega:
    combatGraphicsCode='E';campaignGraphicsCode='E';goto selectAudio;
tandy:
    combatGraphicsCode='T';campaignGraphicsCode='T';goto selectAudio;
cga:
    combatGraphicsCode='C';
    adapter=mwLauncherSystem.detectDisplayAdapter();
    if ((uint8_t)adapter==LAUNCHER_ADAPTER_UNSUPPORTED) goto selectGraphics;
    campaignGraphicsCode=adapter==LAUNCHER_ADAPTER_TANDY?'T':'E';
selectAudio:
    for (uint16_t i=0;i<commandLine->length;++i)
    {
        switch (commandLine->text[i])
        {
        case 'S':case 's':childAudioCode='S';goto beginCampaign;
        case 'T':case 't':childAudioCode='T';goto beginCampaign;
        case 'A':case 'a':childAudioCode='A';goto beginCampaign;
        case 'R':case 'r':goto roland;
        }
    }
roland:
    childAudioCode='R';
beginCampaign:
    childPhase=CAMPAIGN_PHASE_NEW;
    mwLauncherSystem.clearTextScreen();
runCampaign:
    childGraphicsCode=campaignGraphicsCode;
    launch=(MwCampaignLaunch){{launcherIdentity,0x07D4,sizeof(launcherIdentity)},
        0x0004,childPhase,childGraphicsCode,childMemoryTier};
    if (mwLauncherSystem.executeChild(launcherCampaignFilename,&launch,childAudioCode,&childExitCode))
    {
        mwLauncherSystem.displayDosString(launcherCampaignDiskPrompt);
        mwLauncherSystem.writeConsoleChar(mwCampaignInput.currentDefaultDrive()+'A');
        mwLauncherSystem.displayDosString(launcherDiskSuffix);
        if (mwCampaignInput.readKeyboard()==CAMPAIGN_KEY_ESCAPE) return 0;
        goto runCampaign;
    }
    if (childExitCode==0) return 0; /* Any NONZERO AL enters combat; AH ignored. */
runCombat:
    childGraphicsCode=combatGraphicsCode;
    launch.graphicsCode=childGraphicsCode;
    if (mwLauncherSystem.executeChild(launcherCombatFilename,&launch,childAudioCode,&childExitCode))
    {
        mwLauncherSystem.displayDosString(launcherCombatDiskPrompt);
        mwLauncherSystem.writeConsoleChar(mwCampaignInput.currentDefaultDrive()+'A');
        mwLauncherSystem.displayDosString(launcherDiskSuffix);
        if (mwCampaignInput.readKeyboard()==CAMPAIGN_KEY_ESCAPE) return 0;
        goto runCombat;
    }
    if (childExitCode==0) return 0;
    childPhase=CAMPAIGN_PHASE_COMBAT_RETURN;
    goto runCampaign;
}
