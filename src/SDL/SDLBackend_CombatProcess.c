#include "SDLBackend.h"
#include "mechwarrior/launcher.h"
#include "mechwarrior/combat_installation.h"
#include "mechwarrior/combat_startup.h"
#include "mechwarrior/combat_runtime.h"
#include "mechwarrior/combat_platform.h"
#include "mechwarrior/combat_scene.h"
#include "mechwarrior/combat_video.h"
#include "mechwarrior/combat_sound.h"
#include <setjmp.h>
#include <stdlib.h>
#include <string.h>

/* Host DOS process ownership only. Game objects and allocation decisions stay
 * in Original. Captured640KiB DOSBox-X machine profile: resident program ends
 * at4194's MCB, code starts0956. AH48 uses DOS first fit, split MCB paragraphs
 * and adjacent-free coalescing, not a host-malloc total-byte approximation. */
enum { COMBAT_DOS_FIRST_MCB=0x4194,COMBAT_DOS_CONVENTIONAL_END=0xA000,
    COMBAT_DOS_CODE_SELECTOR=HOST_DOS_COMBAT_CODE_SELECTOR,DOS_MCB_PARAGRAPHS=1,DOS_PARAGRAPH_BYTES=16,
    NATIVE_RESOURCE_ENTRY_BYTES=4,NATIVE_SURFACE_PRIORITY_OFFSET=5,NATIVE_PHYSICAL_ADDRESS_MASK=0xFFFFF,
    RECOVERED_INTERRUPT_VECTOR_BYTES=0x18 };
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
static uint16_t retainedCampaignPictureWord;
static bool hasRetainedCampaignPictureWord;

void MwSdlRetainCampaignPictureTail(const uint8_t *pictureBank)
{
    /* Supported DOS placement: MW_2PICS at3B16:0000 survives campaign free.
     * BTECH3194:D5F2 aliases bank3DD2. Original RAM matches2550 consecutive
     * decoded bytes from3DC0 before live stack writes. Keep actual pixels,
     * not captured4144 or an invented special-target condition. */
    enum { RETAINED_PICTURE_WORD_OFFSET=0x3DD2 };
    retainedCampaignPictureWord=(uint16_t)(pictureBank[RETAINED_PICTURE_WORD_OFFSET]
        |((uint16_t)pictureBank[RETAINED_PICTURE_WORD_OFFSET+1]<<8));
    hasRetainedCampaignPictureWord=true;
}

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
static const uint8_t *ResolveWrappedResourcePriority(const uint8_t *directory,int16_t resourceId)
{
    static uint8_t physicalPriorityByte; /* Immediate return-value transport, not another native memory owner. */
    const DosAllocation *surface=allocations;
    while (surface && surface->bytes!=directory) surface=surface->next;
    if (!surface) return NULL;
    uint16_t cursor=(uint16_t)((uint16_t)resourceId*NATIVE_RESOURCE_ENTRY_BYTES);
    uint32_t directoryAddress=((uint32_t)(surface->controlBlockSegment+DOS_MCB_PARAGRAPHS)<<4)+cursor;
    const uint8_t *entry=NULL;
    /* Different chassis/resource sets change DOS first-fit placement. Search
     * the existing native bitmap owners, not just sm_mechs.bmp. Only prepared
     * payload bytes are readable here: never host malloc padding or gaps. */
    for (uint16_t list=0;list<COMBAT_BITMAP_LISTS && !entry;++list)
    {
        const BtechBitmapList *bitmap=bitmapRegistryLists[list];
        if (!bitmap) continue;
        const DosAllocation *pictures=allocations;
        while (pictures && pictures->bytes!=bitmap->pixels.bytes) pictures=pictures->next;
        if (!pictures) continue;
        uint32_t picturesAddress=(uint32_t)(pictures->controlBlockSegment+DOS_MCB_PARAGRAPHS)<<4;
        uint32_t offset=directoryAddress-picturesAddress;
        if (offset<=bitmap->pixels.byteCount && bitmap->pixels.byteCount-offset>=NATIVE_RESOURCE_ENTRY_BYTES)
            entry=bitmap->pixels.bytes+offset;
    }
    /* With both light chassis loaded,5524C is jenpck.tbl+102C, inside
     * resource17's immutable vertex payload.05DE relocates only the directory;
     * keep that mutable prefix excluded from this encoded-payload view. */
    for (uint16_t index=0;index<shapeResourceBankCount && !entry;++index)
    {
        const BtechResourceBank *bank=&shapeResourceBanks[index];
        const DosAllocation *shapes=allocations;
        while (shapes && shapes->bytes!=bank->allocation.bytes) shapes=shapes->next;
        if (!shapes) continue;
        uint32_t start=(uint32_t)(shapes->controlBlockSegment+DOS_MCB_PARAGRAPHS)<<4;
        uint32_t offset=directoryAddress-start;
        uint32_t directoryBytes=((uint32_t)bank->decodedResourceCount+1)*NATIVE_RESOURCE_ENTRY_BYTES;
        if (offset>=directoryBytes && offset<=bank->allocation.byteCount
            && bank->allocation.byteCount-offset>=NATIVE_RESOURCE_ENTRY_BYTES)
            entry=bank->allocation.bytes+offset;
    }
    if (!entry)
    {
        fprintf(stderr,"BA56 foreign directory %05lX has no recovered resource owner\n",(unsigned long)directoryAddress);
        for (const DosAllocation *block=allocations;block;block=block->next)
        {
            uint32_t start=(uint32_t)(block->controlBlockSegment+DOS_MCB_PARAGRAPHS)<<4;
            if (block->bytes && directoryAddress>=start && directoryAddress-start+4<=(uint32_t)block->paragraphs*DOS_PARAGRAPH_BYTES)
                fprintf(stderr,"BA56 containing DOS allocation %05lX bytes=%lu offset=%04lX entry=%02X%02X%02X%02X\n",
                    (unsigned long)start,(unsigned long)block->paragraphs*DOS_PARAGRAPH_BYTES,(unsigned long)(directoryAddress-start),
                    block->bytes[directoryAddress-start],block->bytes[directoryAddress-start+1],block->bytes[directoryAddress-start+2],block->bytes[directoryAddress-start+3]);
        }
        return NULL;
    }
    /* These are live resource payload bytes, not an authored surface record.
     * Decode only at this physical alias boundary; game code uses a byte ref. */
    uint16_t targetOffset=(uint16_t)(entry[0]|((uint16_t)entry[1]<<8));
    uint16_t targetSelector=(uint16_t)(entry[2]|((uint16_t)entry[3]<<8));
    uint32_t priorityAddress=(((uint32_t)targetSelector<<4)+(uint16_t)(targetOffset+NATIVE_SURFACE_PRIORITY_OFFSET))&NATIVE_PHYSICAL_ADDRESS_MASK;
    if (priorityAddress==((uint32_t)COMBAT_DOS_CODE_SELECTOR<<4)+0x6964) return &weaponListLoopJumpOpcode;
    if (priorityAddress==((uint32_t)COMBAT_DOS_CODE_SELECTOR<<4)+0x06A5) return &hudTargetFlagsLocalDisplacement;
    if (priorityAddress==((uint32_t)COMBAT_DOS_CODE_SELECTOR<<4)+0x69A8) return &weaponDisabledColourModRm;
    if (priorityAddress<RECOVERED_INTERRUPT_VECTOR_BYTES)
    {
        if (!btechHostBorrowDosMemory) return NULL;
        MwNativeByteView vectors=btechHostBorrowDosMemory(0);
        physicalPriorityByte=MwReadNativeViewByte(&vectors,(uint16_t)priorityAddress);
        return &physicalPriorityByte;
    }
    fprintf(stderr,"BA56 resource alias %04X:%04X+5 resolves to unrecovered byte %05lX\n",targetSelector,targetOffset,(unsigned long)priorityAddress);
    return NULL;
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
    btechHostResolveWrappedResourcePriority=ResolveWrappedResourcePriority;
    btechHostCloseHandle=CloseHandle;btechHostWriteHandle=WriteHandle;btechHostTerminateChild=TerminateChild;
    return true;
}
bool MwSdlExecuteCombatChild(const char *path,const MwCampaignLaunch *launch,uint8_t audioCode,uint8_t *exitCode)
{
    if (childActive) MwSdlFail("Nested BTECH EXEC");
    if (!BtechHostLoadInitialData(path)) return true;
    if (hasRetainedCampaignPictureWord) specialTargetControlAlias=&retainedCampaignPictureWord;
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
