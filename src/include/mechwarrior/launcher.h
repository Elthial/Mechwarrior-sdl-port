#ifndef MECHWARRIOR_LAUNCHER_H
#define MECHWARRIOR_LAUNCHER_H
#include "mechwarrior/campaign_startup.h"

enum { LAUNCHER_RESIDENT_PARAGRAPHS=0x00EB,LAUNCHER_MINIMUM_PARAGRAPHS=0x704E,
    LAUNCHER_LARGER_PARAGRAPHS=0x8000,LAUNCHER_MEMORY_MINIMUM=2,LAUNCHER_MEMORY_512K=3,
    LAUNCHER_ADAPTER_UNSUPPORTED=0,LAUNCHER_ADAPTER_TANDY=1,LAUNCHER_ADAPTER_EGA=2 };
typedef struct MwLauncherCommandLine {const uint8_t *text;uint8_t length;} MwLauncherCommandLine;
/* DOS EXEC/BIOS transport. Child lifetime recreation belongs inside executeChild,
 * not in campaign mechanics. true means native Carry/error, never success. */
typedef struct MwLauncherSystemCalls
{
    bool (*resizeResident)(uint16_t paragraphs);
    void *(*allocateMemory)(uint16_t paragraphs);
    void (*freeMemory)(void *allocation);
    uint16_t (*detectDisplayAdapter)(void);
    void (*clearTextScreen)(void);
    bool (*executeChild)(const char *filename,const MwCampaignLaunch *launch,uint8_t audioCode,uint8_t *exitCode);
    void (*displayDosString)(const char *text);
    void (*writeConsoleChar)(uint8_t character);
} MwLauncherSystemCalls;
extern MwLauncherSystemCalls mwLauncherSystem;
extern uint8_t childGraphicsCode,childAudioCode,childPhase,childMemoryTier;
extern uint8_t combatGraphicsCode,campaignGraphicsCode;
extern uint8_t launcherIdentity[11];
extern char launcherCampaignFilename[12],launcherCombatFilename[10];
extern char launcherReleaseError[0x17],launcherCampaignDiskPrompt[0x1C];
extern char launcherCombatDiskPrompt[0x1C],launcherMemoryError[0x21];
extern char launcherGraphicsPrompt[0xE7],launcherDiskSuffix[0x39];
bool MwImportLauncherInitialData(const uint8_t *data,size_t byteCount);
/* Host projection of PSP command tail and DOS terminate00; no new native ABI. */
uint8_t MwLauncherMain(const MwLauncherCommandLine *commandLine);
#endif
