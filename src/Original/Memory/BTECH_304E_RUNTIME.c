#include "mechwarrior/combat_runtime.h"
#include "mechwarrior/combat_heap.h"
#include "mechwarrior/native_address.h"
#include <string.h>

BtechRuntimePrefix runtimePrefix;
BtechRuntimeCallback runtimeExitCallbacks[1],runtimeDiagnosticCallback;
void (*runtimeDeviceCleanup)(uint16_t);
uint32_t runtimeSavedDivideVector,runtimeSavedExtraVector;
uint8_t runtimeExtraVectorActive,runtimeExtraInterrupt;
uint16_t runtimeHandleLimit;
uint8_t runtimeHandleFlags[COMBAT_DOS_HANDLES];
uint16_t runtimeTemporaryFileIds[COMBAT_STREAM_SLOTS];
uint8_t runtimeStreamAuxiliaryFlags[COMBAT_STREAM_SLOTS];
uint16_t nearHeapFirstHeader,nearHeapSearchHeader,nearHeapEnd;
uint16_t nearHeapProgramBreak=BTECH_NEAR_INITIAL_BREAK;
uint16_t nearHeapGrowthAlignment=BTECH_NEAR_GROWTH_ALIGNMENT;
BtechRuntimeMessage runtimeMessages[COMBAT_RUNTIME_MESSAGES];

bool BtechImportRuntimeInitialData(const uint8_t *data,size_t bytes)
{
    if (!data || bytes<0x4EDC) return false;
    BtechHostDiscardNearHeap();
    uint16_t callback=MwReadNativeWord(data,0x4DFA);
    if (callback!=0 && callback!=0xFE80) return false;
    if (MwReadNativeWord(data,0x4BA8) || MwReadNativeWord(data,0x4DF0)) return false;
    runtimeExitCallbacks[0]=callback?BtechFlushAllRuntimeStreams:NULL;
    /* FE80's returned flush count is unobserved by E1CD. */
    runtimeDiagnosticCallback=NULL;runtimeDeviceCleanup=NULL;
    for (uint16_t word=0;word<4;++word) runtimePrefix.ambientFallbackWords[word]=MwReadNativeWord(data,word*2);
    memcpy(runtimePrefix.copyrightBytes,data+8,sizeof(runtimePrefix.copyrightBytes));
    runtimeSavedDivideVector=(uint32_t)MwReadNativeWord(data,0x4B4D)|((uint32_t)MwReadNativeWord(data,0x4B4F)<<16);
    runtimeSavedExtraVector=(uint32_t)MwReadNativeWord(data,0x4B8C)|((uint32_t)MwReadNativeWord(data,0x4B8E)<<16);
    runtimeExtraVectorActive=data[0x4B8A];runtimeExtraInterrupt=data[0x4B8B];
    runtimeHandleLimit=MwReadNativeWord(data,0x4B66);
    if (runtimeHandleLimit!=COMBAT_DOS_HANDLES) return false;
    memcpy(runtimeHandleFlags,data+0x4B68,sizeof(runtimeHandleFlags));
    for (uint16_t slot=0;slot<COMBAT_STREAM_SLOTS;++slot)
    {
        runtimeTemporaryFileIds[slot]=MwReadNativeWord(data,0x4C64+slot*6);
        runtimeStreamAuxiliaryFlags[slot]=data[0x4C60+slot*6];
    }
    uint16_t cursor=0x4E0A;
    for (uint16_t message=0;message<COMBAT_RUNTIME_MESSAGES;++message)
    {
        BtechRuntimeMessage *entry=&runtimeMessages[message];
        entry->code=MwReadNativeWord(data,cursor);cursor+=2;
        const uint8_t *end=memchr(data+cursor,0,bytes-cursor);
        if (!end || (size_t)(end-data-cursor)>=sizeof(entry->text)) return false;
        memcpy(entry->text,data+cursor,(size_t)(end-data-cursor)+1);cursor=(uint16_t)(end-data+1);
        if (entry->code==UINT16_MAX) return message==COMBAT_RUNTIME_MESSAGES-1;
    }
    return false;
}
