#include "SDLBackend.h"
#include "mechwarrior/launcher.h"
#include "mechwarrior/combat_installation.h"
#include "mechwarrior/combat_startup.h"
#include "mechwarrior/combat_runtime.h"
#include "mechwarrior/combat_platform.h"
#include "mechwarrior/combat_scene.h"
#include "mechwarrior/combat_video.h"
#include "mechwarrior/combat_sound.h"
#include "mechwarrior/combat_input.h"
#include "mechwarrior/combat_render.h"
#include "mechwarrior/combat_orders.h"
#include <setjmp.h>
#include <stdlib.h>
#include <string.h>

/* Host DOS process ownership only. Game objects and allocation decisions stay
 * in Original. Captured640KiB DOSBox-X machine profile: resident program ends
 * at4194's MCB, code starts0956. AH48 uses DOS first fit, split MCB paragraphs
 * and adjacent-free coalescing, not a host-malloc total-byte approximation. */
enum { COMBAT_DOS_FIRST_MCB=0x4194,COMBAT_DOS_CONVENTIONAL_END=0xA000,
    DOS_MCB_PARAGRAPHS=1,DOS_PARAGRAPH_BYTES=16 };
typedef struct DosAllocation {
    struct DosAllocation *next;
    uint8_t *bytes;
    uint16_t paragraphs,controlBlockSegment;
} DosAllocation;
static DosAllocation *allocations;
static char gameDirectory[4096];
static jmp_buf childReturn;
static bool childActive;
static uint8_t childExitCode;
static uint8_t *AllocateParagraphs(uint16_t paragraphs,uint16_t *largestFreeParagraphs)
{
    if (!allocations)
    {
        allocations=malloc(sizeof(*allocations));
        if (!allocations) {*largestFreeParagraphs=0;return NULL;}
        *allocations=(DosAllocation){NULL,NULL,COMBAT_DOS_CONVENTIONAL_END-COMBAT_DOS_FIRST_MCB-DOS_MCB_PARAGRAPHS,COMBAT_DOS_FIRST_MCB};
    }
    *largestFreeParagraphs=0;
    DosAllocation *allocation=NULL;
    for (DosAllocation *block=allocations;block;block=block->next)
        if (!block->bytes)
        {
            if (block->paragraphs>*largestFreeParagraphs) *largestFreeParagraphs=block->paragraphs;
            if (!allocation && block->paragraphs>=paragraphs) allocation=block;
        }
    if (!allocation) return NULL;
    uint8_t *bytes=malloc(paragraphs?(size_t)paragraphs*DOS_PARAGRAPH_BYTES:1);
    if (!bytes) {*largestFreeParagraphs=0;return NULL;}
    if (allocation->paragraphs>paragraphs)
    {
        DosAllocation *remaining=malloc(sizeof(*remaining));
        if (!remaining) {free(bytes);*largestFreeParagraphs=0;return NULL;}
        *remaining=(DosAllocation){allocation->next,NULL,(uint16_t)(allocation->paragraphs-paragraphs-DOS_MCB_PARAGRAPHS),
            (uint16_t)(allocation->controlBlockSegment+paragraphs+DOS_MCB_PARAGRAPHS)};
        allocation->next=remaining;
    }
    allocation->paragraphs=paragraphs;allocation->bytes=bytes;
    *largestFreeParagraphs=paragraphs; /* Successful DOS AH48 leaves caller BX. */
    return allocation->bytes;
}
static uint16_t DosAllocationSelector(const uint8_t *bytes)
{
    if (!bytes) return 0;
    for (const DosAllocation *block=allocations;block;block=block->next)
        if (block->bytes==bytes) return block->controlBlockSegment+DOS_MCB_PARAGRAPHS;
    MwSdlFail("Unknown BTECH DOS allocation identity");
    return 0;
}
static void FreeDosAllocation(uint8_t *bytes)
{
    /* Native AH49 selector0 fails without freeing anything.1F10 discards CF;
     *228B releases empty bank slots too. Do not turn that DOS error into a
     * host failure or free a fabricated allocation. */
    if (!bytes) return;
    DosAllocation *allocation=allocations;
    while (allocation && allocation->bytes!=bytes) allocation=allocation->next;
    if (!allocation) MwSdlFail("Free of unknown BTECH DOS allocation");
    free(allocation->bytes);allocation->bytes=NULL;
    for (DosAllocation *block=allocations;block;)
    {
        if (!block->bytes && block->next && !block->next->bytes)
        {
            DosAllocation *next=block->next;
            block->paragraphs+=(uint16_t)(DOS_MCB_PARAGRAPHS+next->paragraphs);
            block->next=next->next;free(next);
        }
        else block=block->next;
    }
}
static void ReleaseChildDosAllocations(void)
{
    while (allocations)
    {DosAllocation *block=allocations;allocations=block->next;free(block->bytes);free(block);}
}
static void FreeDosSelector(uint16_t selector)
{
    for (DosAllocation *block=allocations;block;block=block->next)
        if (block->bytes && (uint16_t)(block->controlBlockSegment+DOS_MCB_PARAGRAPHS)==selector)
        {FreeDosAllocation(block->bytes);return;}
    /* DOS AH49 failure, including already-free selectors, is ignored by1F10. */
}
uint16_t MwSdlCombatDosAllocationCount(void)
{
    uint16_t count=0;
    for (const DosAllocation *block=allocations;block;block=block->next) if (block->bytes) ++count;
    return count;
}
static FILE *OpenStream(const char *filename,const char *mode)
{
    /* Original resources are flat installation files. Never change the shared
     * working directory or let a native filename escape the installation. */
    if (!filename[0] || strpbrk(filename,"/\\:")) return NULL;
    char path[4096];
    if (snprintf(path,sizeof(path),"%s/%s",gameDirectory,filename)>=(int)sizeof(path)) return NULL;
    return fopen(path,mode);
}
static int16_t CloseHandle(uint16_t handle)
{
    for (uint16_t slot=0;slot<COMBAT_STREAM_SLOTS;++slot)
    {
        BtechRuntimeStream *stream=&runtimeStreams[slot];
        if (stream->dosHandle!=handle || !stream->file) continue;
        int closed=fclose(stream->file);stream->file=NULL;
        return closed?-1:0; /* Native raw close does not clear FILE flags. */
    }
    return -1;
}
static void WriteHandle(uint16_t handle,const char *text,uint16_t bytes)
{
    FILE *file=handle==COMBAT_STDERR_HANDLE?stderr:handle==1?stdout:NULL;
    if (!file)
        for (uint16_t slot=0;slot<COMBAT_STREAM_SLOTS;++slot)
            if (runtimeStreams[slot].dosHandle==handle && runtimeStreams[slot].file)
            {file=runtimeStreams[slot].file;break;}
    if (!file || fwrite(text,1,bytes,file)!=bytes) MwSdlFail("BTECH DOS handle write failed");
    fflush(file);
}
static void TerminateChild(uint8_t exitCode)
{
    if (!childActive) MwSdlFail("BTECH termination outside EXEC");
    printf("Combat return: DOS exit=%u result=%u tick=%u objective=%u completion=%u/%u protectedTargetRemaining=%u playerValue=%d movementMode=%u lastScan=%02X\n",
        exitCode,campaignResultExchange.result,combatTickCounter,sideObjectiveRules[0],
        sideCompletionFlags[0],sideCompletionFlags[1],specialTargetDamageRemaining,
        unitControls[0].conditionScoreOrExitCode,unitControls[0].movementMode,combatInputScanCode);
    fflush(stdout);
    childExitCode=exitCode;
    longjmp(childReturn,1); /* Actual DOS termination never returns to E144. */
}
bool MwSdlInitializeCombatProcess(const char *directory)
{
    if (strlen(directory)>=sizeof(gameDirectory)) return false;
    strcpy(gameDirectory,directory);
    btechHostOpenStream=OpenStream;btechHostAllocateParagraphs=AllocateParagraphs;
    btechHostFreeDosAllocation=FreeDosAllocation;btechHostReleaseChildDosAllocations=ReleaseChildDosAllocations;
    btechHostDosAllocationSelector=DosAllocationSelector;
    btechHostFreeDosSelector=FreeDosSelector;
    btechHostCloseHandle=CloseHandle;btechHostWriteHandle=WriteHandle;btechHostTerminateChild=TerminateChild;
    return true;
}
bool MwSdlExecuteCombatChild(const char *path,const MwCampaignLaunch *launch,uint8_t audioCode,uint8_t *exitCode)
{
    if (childActive) MwSdlFail("Nested BTECH EXEC");
    if (!BtechHostLoadInitialData(path)) return true;
    MwSdlResetCombatHardware();mwSdlCombatActive=true;
    MwSdlUpdateWindowTitle();
    /* MW.EXE's DOS tail is03,<graphics>,20,<audio>,0D. Phase/transfer live in
     * its private FCB; they are NOT an extra C command-line argument. */
    char graphics[2]={(char)launch->graphicsCode,0},audio[2]={(char)audioCode,0};
    const char *arguments[]={"BTECH.EXE",graphics,audio};
    childActive=true;
    if (setjmp(childReturn)==0)
    {
        BtechProgramMain(3,arguments);
        MwSdlFail("Native BTECH main unexpectedly returned");
    }
    childActive=false;mwSdlCombatActive=false;
    MwSdlUpdateWindowTitle();
    *exitCode=childExitCode;
    BtechHostDiscardChildData();
    return false;
}
