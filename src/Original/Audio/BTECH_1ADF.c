#include "mechwarrior/combat_sound.h"
#include "mechwarrior/combat_runtime.h"
#include "mechwarrior/combat_startup.h"
#include "mechwarrior/combat_video.h"
#include "mechwarrior/native_address.h"
#include <stdlib.h>
#include <string.h>

/*1ADF:0000. Genuine DX:AX code pair becomes a typed callback, not an SDL
 * audio decision. Tandy selection precedes the speaker/fallback selector. */
BtechAudioTick BtechSelectTandyOrFallbackAudioCallback(void)
{
    if (selectedAudioBackendMask&COMBAT_SOUND_TANDY) return BtechTickTandyToneChannels;
    return BtechSelectSpeakerAdlibOrMpuCallback();
}

/*1ADF:000E. Speaker precedes AdLib; the remaining case is actual0F38. */
BtechAudioTick BtechSelectSpeakerAdlibOrMpuCallback(void)
{
    if (selectedAudioBackendMask&COMBAT_SOUND_SPEAKER) return BtechTickSpeakerToneChannels;
    if (selectedAudioBackendMask&COMBAT_SOUND_ADLIB) return BtechTickSoftwareMusic;
    return BtechAudioTimerNoOp;
}

/*1ADF:0F38. Actual one-byte RETF target, not a replacement music scheduler. */
void BtechAudioTimerNoOp(void)
{
}

/*1ADF:01CE. Music root0 aliases sound root10. The sequence comparison is
 * signed and admits track==count; an available directory entry still runs. */
BtechSoundEvent BtechAdvanceMusicSequence(void)
{
    if (activeMusicBankIndex>=COMBAT_MUSIC_SEQUENCE_ROWS || musicSequenceIndex>=COMBAT_MUSIC_SEQUENCE_WORDS) abort();
    int16_t next=musicSequences[activeMusicBankIndex][musicSequenceIndex];
    if (next==-1) return (BtechSoundEvent){0};
    BtechSoundBank *bank=&soundBanks[COMBAT_MUSIC_FIRST_ROOT+activeMusicBankIndex];
    if (next>(int16_t)bank->eventCount) return (BtechSoundEvent){0};
    next=musicSequences[activeMusicBankIndex][musicSequenceIndex];
    musicSequenceIndex=(uint16_t)next;
    if (next<0 || !bank->events || (uint16_t)next>=bank->eventReferenceCapacity) abort();
    BtechSoundEvent block=bank->events[next];block.offset+=COMBAT_TONE_HEADER_BYTES;
    return block;
}

/*1ADF:10D2. Count actual OPL status reads, not a wall-clock delay. */
void BtechWriteAdlibProbeRegister(uint8_t registerIndex,uint8_t value)
{
    if (!btechHostReadPort || !btechHostWritePort) abort();
    btechHostWritePort(ADLIB_INDEX_PORT,registerIndex);
    for (uint16_t delay=7;delay;--delay) btechHostReadPort(ADLIB_INDEX_PORT);
    btechHostWritePort(ADLIB_DATA_PORT,value);
    for (uint16_t delay=0x1E;delay;--delay) btechHostReadPort(ADLIB_INDEX_PORT);
}

/*1ADF:10EC. Failure paths do NOT perform the final timer reset writes. */
uint16_t BtechProbeAdlibTimerStatus(void)
{
    BtechWriteAdlibProbeRegister(ADLIB_TIMER_CONTROL,ADLIB_TIMER_RESET);
    BtechWriteAdlibProbeRegister(ADLIB_TIMER_CONTROL,ADLIB_IRQ_RESET);
    adlibProbeInitialStatus=btechHostReadPort(ADLIB_INDEX_PORT);
    BtechWriteAdlibProbeRegister(ADLIB_TIMER_ONE,0xFF);
    BtechWriteAdlibProbeRegister(ADLIB_TIMER_CONTROL,ADLIB_TIMER_START);
    for (uint16_t delay=0x190;delay;--delay) btechHostReadPort(ADLIB_INDEX_PORT);
    uint8_t status=(btechHostReadPort(ADLIB_INDEX_PORT)&ADLIB_TIMER_STATUS_MASK)^ADLIB_TIMER_EXPECTED_STATUS;
    if (status) return status;
    status=adlibProbeInitialStatus&ADLIB_TIMER_STATUS_MASK;
    if (status) return status;
    BtechWriteAdlibProbeRegister(ADLIB_TIMER_CONTROL,ADLIB_TIMER_RESET);
    BtechWriteAdlibProbeRegister(ADLIB_TIMER_CONTROL,ADLIB_IRQ_RESET);
    return 0;
}

/*1ADF:1136. ROM BYTEs then INT15; the signature leaves AL21, so native AX
 * is C021, not C000. BIOS Carry alone selects the two platform masks. */
uint16_t BtechProbePlatformAudioMask(void)
{
    if (!btechHostReadRomByte) abort();
    if (btechHostReadRomByte(0xFFFF,0x000E)!=0xFF) return COMBAT_SOUND_SPEAKER;
    if (btechHostReadRomByte(0xFC00,0)!=0x21) return COMBAT_SOUND_SPEAKER;
    if (!btechHostQuerySystemConfiguration) abort();
    return btechHostQuerySystemConfiguration(0xC021)?0x03:0x07;
}

/*1ADF:02CA. No port silence is invented: the tone scheduler observes the
 * cleared activity/level on its next tick. Sample IRQ restore precedes priority. */
uint16_t BtechStopSoundEventAndTimer(uint16_t soundEventIndex)
{
    BtechStopMatchingToneChannels(soundEventIndex,0);
    if (soundEventIndex==timerSoundEventIndex)
    {
        BtechRestoreSoundTimerInterrupt();
        if (!samplePriorityOwner) abort();
        *samplePriorityOwner=0;
    }
    return 0;
}

/*1ADF:0DA0. DS304E callers only. No CLI/STI, control-port write or speaker-gate
 * write. IRQ0DD3's call with sample DS selected remains a separate live gate. */
void BtechRestoreSoundTimerInterrupt(void)
{
    if (!soundTimerInstalled) return;
    pitChannel0DivisorShadow=savedSoundTimerDivisor;
    if (!btechHostWritePort || !btechHostSetInterruptVector) abort();
    btechHostWritePort(PIT_CHANNEL_ZERO_PORT,(uint8_t)savedSoundTimerDivisor);
    btechHostWritePort(PIT_CHANNEL_ZERO_PORT,(uint8_t)(savedSoundTimerDivisor>>8));
    soundTimerInstalled=0;
    btechHostSetInterruptVector(8,savedSampleIrqVector);
}

/*1ADF:02F8. Stream-local WORD survives close and is the stack alias read by
 *0810:2C40. Do NOT derive it from FILE*, a DOS handle or the tagged-record slot. */
uint16_t BtechLoadSoundBank(const char *filename,BtechRuntimeStream *stream,uint16_t *inheritedStreamIdentity)
{
    int16_t bankIndex=0;
    while (bankIndex<COMBAT_SOUND_BANKS-1 && soundBanks[bankIndex].root.allocation.bytes) ++bankIndex;
    uint16_t openedHere=0;
    if (!stream || !BtechIsRegisteredTaggedStream(stream))
    {stream=BtechOpenTaggedResourceStream(filename);openedHere=1;}
    *inheritedStreamIdentity=stream?(uint16_t)(COMBAT_STREAM_TABLE+(stream-runtimeStreams)*COMBAT_STREAM_STRIDE):0;
    if (!stream) return 0;
    BtechSoundBank *bank=&soundBanks[bankIndex];
    uint16_t failed=(BtechFindResourceTagPosition(stream,"SND:INF:",0)&0x80000000)!=0;
    if (!failed)
    {
        uint8_t ignoredHeader[2],eventCount[2],unknown[2];
        /* Partial reads store into the root; the ignored local is never used. */
        eventCount[0]=(uint8_t)bank->eventCount;eventCount[1]=(uint8_t)(bank->eventCount>>8);
        unknown[0]=(uint8_t)bank->unidentifiedField9;unknown[1]=(uint8_t)(bank->unidentifiedField9>>8);
        fread(ignoredHeader,2,1,stream->file);
        fread(eventCount,2,1,stream->file);bank->eventCount=MwReadNativeWord(eventCount,0);
        fread(unknown,2,1,stream->file);bank->unidentifiedField9=MwReadNativeWord(unknown,0);
        failed=1;
        if (selectedAudioBackendMask&COMBAT_SOUND_SPEAKER)
            failed&=BtechMergeSoundBackendDirectory(bank,stream,"SND:IBM:")==0;
        if (selectedAudioBackendMask&COMBAT_SOUND_TANDY)
            failed&=BtechMergeSoundBackendDirectory(bank,stream,"SND:TAN:")==0;
        if (selectedAudioBackendMask&COMBAT_SOUND_8SV_MASK)
            failed&=BtechMergeSoundBackendDirectory(bank,stream,"SND:8SV:")==0;
        if (selectedAudioBackendMask&COMBAT_SOUND_MPU)
            failed&=BtechLoadTaggedSoundAllocation(bank,stream,"SND:ROL:")==NULL;
    }
    if (!failed) bank->fallback=BtechLoadTaggedSoundAllocation(bank,stream,"SND:ITM:");
    if (openedHere) BtechCloseTaggedResourceStream(stream);
    return failed?0:(uint16_t)(bankIndex+1);
}

/*1ADF:04D2. Decode each directory pair at the real binary boundary. Runtime
 * playback receives typed event references, not repeated offset accessors. */
uint16_t BtechMergeSoundBackendDirectory(BtechSoundBank *bank,BtechRuntimeStream *stream,const char *tag)
{
    BtechSoundAllocationNode *node=BtechLoadTaggedSoundAllocation(bank,stream,tag);
    if (!node) return 0;
    if ((int16_t)bank->eventCount<=0) return 1;
    if (!bank->events)
    {
        bank->events=calloc(bank->eventCount,sizeof(*bank->events));
        if (!bank->events) abort(); /* Host representation, not a new native error branch. */
        bank->eventReferenceCapacity=bank->eventCount;
    }
    if (bank->eventCount>bank->eventReferenceCapacity) abort(); /* Unrecovered old-directory aliases after a count-changing slot10 reuse. */
    for (int16_t event=0;event<(int16_t)bank->eventCount;++event)
    {
        uint32_t entry=(uint16_t)((uint16_t)event<<2);
        if (entry+4>node->allocation.byteCount) abort();
        uint16_t offset=MwReadNativeWord(node->allocation.bytes,(uint16_t)entry);
        uint16_t paragraph=MwReadNativeWord(node->allocation.bytes,(uint16_t)(entry+2));
        if (!(offset|paragraph)) continue;
        uint32_t eventOffset=(uint32_t)paragraph*0x10+offset;
        if (eventOffset>=node->allocation.byteCount) abort();
        uint32_t paragraphBase=(uint32_t)paragraph*0x10;
        bank->events[event]=(BtechSoundEvent){{node->allocation.bytes+paragraphBase,0,node->allocation.byteCount-paragraphBase},offset};
    }
    return 1;
}

/*1ADF:054C. Keep decode/request/close/link order, including the failed-append
 * overwrite of the old tail. A malformed first partial length has unknown
 * stack residue; gate it rather than zero-fill an invented allocation size. */
BtechSoundAllocationNode *BtechLoadTaggedSoundAllocation(BtechSoundBank *bank,BtechRuntimeStream *stream,const char *tag)
{
    if (BtechFindResourceTagPosition(stream,tag,0)&0x80000000) return NULL;
    fgetc(stream->file);
    uint8_t lengthBytes[4];
    if (fread(lengthBytes,4,1,stream->file)!=1) abort();
    uint32_t remaining=(uint32_t)MwReadNativeWord(lengthBytes,0)|((uint32_t)MwReadNativeWord(lengthBytes,2)<<16);
    fseek(stream->file,-5,SEEK_CUR);
    BtechResourceAllocation allocation=soundAllocationOverride;
    if (!allocation.bytes)
    {
        /*1EB6 deliberately wraps its paragraph request. A corrupt sound length
         * must not let a smaller native allocation masquerade as a host buffer
         * of the full declared size. Foreign-owner overwrite is not supported. */
        if (remaining>DOS_MAX_PARAGRAPH_ALLOCATION_BYTES) abort();
        allocation.bytes=BtechAllocateDosMemoryBlock(remaining).bytes;
        allocation.byteCount=remaining;
        if (!allocation.bytes) soundAllocationError=1;
    }
    int16_t archive=BtechOpenCompressedArchive(UINT16_MAX,stream->file,"rb",BtechGetTaggedPayloadLength(stream));
    uint16_t failed=archive<0;
    uint32_t cursor=0;
    while (!failed && remaining)
    {
        uint16_t chunk=(uint16_t)(remaining>COMBAT_SOUND_CHUNK_BYTES?COMBAT_SOUND_CHUNK_BYTES:remaining);
        if (!allocation.bytes || cursor+chunk>allocation.byteCount) abort(); /* Native NULL/foreign writes are not host memory. */
        failed=BtechReadArchiveIntoFarBuffer((uint16_t)archive,allocation.bytes+cursor,chunk)!=chunk;
        remaining-=chunk;cursor+=chunk;
    }
    if (archive>=0) BtechFlushAndCloseCompressedArchive((uint16_t)archive);
    BtechSoundAllocationNode *node=NULL;
    if (!failed)
    {
        node=&bank->root;
        if (node->allocation.bytes)
        {
            while (node->next) node=node->next;
            node->next=malloc(sizeof(*node));
            if (node->next) {node=node->next;node->next=NULL;}
            else {soundAllocationError=2;failed=2;}
        }
        node->allocation=allocation;
        if (strlen(tag)>=sizeof(node->chunkTag)) abort();
        strcpy(node->chunkTag,tag);
    }
    if (!failed) return node;
    if (allocation.bytes)
    {
        if (!btechHostFreeDosAllocation) abort();
        btechHostFreeDosAllocation(allocation.bytes);
    }
    return NULL;
}

/*1ADF:0C1E. Handle is one-based; signed DEC/bounds checks are native. */
uint16_t BtechSelectActiveSoundBank(uint16_t bankHandle)
{
    --bankHandle;
    if ((int16_t)bankHandle<0 || (int16_t)bankHandle>COMBAT_SOUND_BANKS-1) return 0;
    if (!soundBanks[bankHandle].root.allocation.bytes) return 0;
    activeSoundBankIndex=bankHandle;
    return 1;
}

/*1ADF:0AAC. Native minimum-priority search excludes the last channel. The
 * busy paired-channel6 path retains channel7; do not repair that quirk. */
uint16_t BtechAllocateToneEventChannels(BtechSoundEvent event,uint16_t priority,uint16_t soundEventIndex)
{
    uint16_t type=MwReadNativeViewWord(&event.data,event.offset),first,last;
    if (!type)
    {
        if (!(selectedAudioBackendMask&COMBAT_SOUND_SPEAKER)) return 0;
        first=0;last=3;
    }
    else
    {
        if (type!=1 || !(selectedAudioBackendMask&COMBAT_SOUND_TANDY)) return 0;
        if (MwReadNativeViewWord(&event.data,(uint16_t)(event.offset+6))) first=last=6;
        else {first=4;last=6;}
    }
    uint16_t selected=first;
    while (selected<=last && soundPriorities[selected]) ++selected;
    if (selected>last)
    {
        uint16_t minimumPriority=soundPriorities[first];
        for (uint16_t channel=first;channel<last;++channel)
            if (minimumPriority>=soundPriorities[channel])
            {minimumPriority=soundPriorities[channel];selected=channel;}
    }
    if (soundPriorities[selected]>priority) return 0;
    if (activeSoundBankIndex>=COMBAT_SOUND_BANKS) abort();
    BtechSoundAllocationNode *fallback=soundBanks[activeSoundBankIndex].fallback;
    /* A missing ITM still starts native effects. Preserve its ambient WORD
     * offset; physical selector0/unknown fallback memory is gated ONLY if read. */
    uint16_t fallbackOffset=fallback?0:missingSoundFallbackOffsetAlias;
    MwNativeByteView fallbackData=fallback?(MwNativeByteView){fallback->allocation.bytes,0,fallback->allocation.byteCount}:(MwNativeByteView){0};
    if (!fallback && btechHostBorrowDosMemory) fallbackData=btechHostBorrowDosMemory(missingSoundFallbackSelectorAlias);
    BtechStartToneInterpreterChannel(event.data,event.offset,selected,fallbackData,fallbackOffset);
    if (MwReadNativeViewWord(&event.data,(uint16_t)(event.offset+6)) && MwReadNativeViewWord(&event.data,event.offset)==1)
    {
        fallback=soundBanks[activeSoundBankIndex].fallback;
        fallbackOffset=fallback?0:missingSoundFallbackOffsetAlias;
        fallbackData=fallback?(MwNativeByteView){fallback->allocation.bytes,0,fallback->allocation.byteCount}:(MwNativeByteView){0};
        if (!fallback && btechHostBorrowDosMemory) fallbackData=btechHostBorrowDosMemory(missingSoundFallbackSelectorAlias);
        uint16_t factor=MwReadNativeViewWord(&event.data,(uint16_t)(event.offset+6));
        uint16_t secondaryOffset=factor*0x0A+event.offset-0x50;
        BtechStartToneInterpreterChannel(event.data,secondaryOffset,7,fallbackData,fallbackOffset);
    }
    soundPriorities[selected]=priority;
    soundEventIndices[selected]=soundEventIndex;
    return 1;
}

/*1ADF:091A. Speaker/Tandy tone records and genuine no-op/reject cases.
 * Sample/DAC/MPU output is explicitly unsupported here, never reported played. */
uint16_t BtechDispatchSoundBankEvent(int16_t soundEventIndex,uint16_t priority)
{
    if ((int16_t)activeSoundBankIndex<0) return 0;
    if (activeSoundBankIndex>=COMBAT_SOUND_BANKS) abort();
    BtechSoundBank *bank=&soundBanks[activeSoundBankIndex];
    if ((int16_t)(uint16_t)(bank->eventCount-1)<soundEventIndex) return 0;
    if (soundEventIndex<0 || (uint16_t)soundEventIndex>=bank->eventReferenceCapacity || !bank->events) abort();
    BtechSoundEvent event=bank->events[soundEventIndex];
    uint16_t type=MwReadNativeViewWord(&event.data,event.offset);
    switch (type)
    {
    case 0:case 1:return BtechAllocateToneEventChannels(event,priority,(uint16_t)soundEventIndex);
    case 2:
        if (!(soundEffectsEnableMask&1) || !(selectedAudioBackendMask&COMBAT_SOUND_SPEAKER)) return 0;
        abort(); /* Native uninitialized priority-channel WORD and sample IRQ not converted. */
    case 3:return 1;
    case 4:
        if (!(soundEffectsEnableMask&1) || !(selectedAudioBackendMask&0x08)) return 0;
        abort(); /* Tandy SL/TL DAC path. */
    case 0x0B:
        if (!(selectedAudioBackendMask&COMBAT_SOUND_MPU)) return 1;
        abort(); /* MPU terminal-byte transport. */
    default:return 0;
    }
}
