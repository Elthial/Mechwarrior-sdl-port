#include "mechwarrior/port_diagnostics.h"
#include "mechwarrior/combat_sound.h"
#include "mechwarrior/combat_runtime.h"
#include "mechwarrior/combat_startup.h"
#include "mechwarrior/combat_video.h"
#include "mechwarrior/native_address.h"
#include "mechwarrior/combat_mpu.h"
#include "mechwarrior/combat_adlib.h"
#include "mechwarrior/combat_heap.h"
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

/* Same 1ADF:0F38 RETF, viewed through its register-message call contract.
 * This preserves AX and target identity separately from 229C:0020. */
uint16_t BtechAudioMusicNoOp(uint16_t statusDisplacement,uint16_t channel,uint16_t packedData)
{
    (void)statusDisplacement;(void)channel;return packedData;
}

/*1ADF:01CE. Music root0 aliases sound root10. The sequence comparison is
 * signed and admits track==count; an available directory entry still runs. */
BtechSoundEvent BtechAdvanceMusicSequence(void)
{
    if (activeMusicBankIndex>=COMBAT_MUSIC_SEQUENCE_ROWS || musicSequenceIndex>=COMBAT_MUSIC_SEQUENCE_WORDS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: activeMusicBankIndex>=COMBAT_MUSIC_SEQUENCE_ROWS || musicSequenceIndex>=COMBAT_MUSIC_SEQUENCE_WORDS");
    int16_t next=musicSequences[activeMusicBankIndex][musicSequenceIndex];
    if (next==-1) return (BtechSoundEvent){0};
    BtechSoundBank *bank=&soundBanks[COMBAT_MUSIC_FIRST_ROOT+activeMusicBankIndex];
    if (next>(int16_t)bank->eventCount) return (BtechSoundEvent){0};
    next=musicSequences[activeMusicBankIndex][musicSequenceIndex];
    musicSequenceIndex=(uint16_t)next;
    if (next<0 || !bank->events || (uint16_t)next>=bank->eventReferenceCapacity) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: next<0 || !bank->events || (uint16_t)next>=bank->eventReferenceCapacity");
    BtechSoundEvent block=bank->events[next];block.offset+=COMBAT_TONE_HEADER_BYTES;
    return block;
}

/*1ADF:10D2. Count actual OPL status reads, not a wall-clock delay. */
void BtechWriteAdlibProbeRegister(uint8_t registerIndex,uint8_t value)
{
    if (!btechHostReadPort || !btechHostWritePort) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !btechHostReadPort || !btechHostWritePort");
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
    if (!btechHostReadRomByte) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !btechHostReadRomByte");
    if (btechHostReadRomByte(0xFFFF,0x000E)!=0xFF) return COMBAT_SOUND_SPEAKER;
    if (btechHostReadRomByte(0xFC00,0)!=0x21) return COMBAT_SOUND_SPEAKER;
    if (!btechHostQuerySystemConfiguration) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !btechHostQuerySystemConfiguration");
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
        if (!samplePriorityOwner) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !samplePriorityOwner");
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
    if (!btechHostWritePort || !btechHostSetInterruptVector) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !btechHostWritePort || !btechHostSetInterruptVector");
    btechHostWritePort(PIT_CHANNEL_ZERO_PORT,(uint8_t)savedSoundTimerDivisor);
    btechHostWritePort(PIT_CHANNEL_ZERO_PORT,(uint8_t)(savedSoundTimerDivisor>>8));
    soundTimerInstalled=0;
    btechHostSetInterruptVector(8,savedSampleIrqVector);
}

/*1ADF:0F39. The native private IRQ stack becomes the C call frame. There is
 * no STI or previous-handler chain, and a faulting callback never sends EOI. */
void BtechMusicTimerInterrupt(void)
{
    if (!musicTimerCallback || !btechHostWritePort) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !musicTimerCallback || !btechHostWritePort");
    musicTimerCallback();
    btechHostWritePort(PIC_COMMAND_PORT,PIC_END_OF_INTERRUPT);
}

/*1ADF:0F76. Reinstallation changes the callback/divisor, not the saved vector.
 * STI deliberately precedes the shadow store and the two PIT writes. */
void BtechInstallMusicTimerCallback(BtechAudioTick callback,uint16_t divisor)
{
    if (!btechHostGetInterruptVector || !btechHostSetInterruptVector
        || !btechHostSetInterrupts || !btechHostWritePort) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !btechHostGetInterruptVector || !btechHostSetInterruptVector || !btechHostSetInterrupts || !btechHostWritePort");
    if (!musicTimerInstalled)
    {
        musicTimerInstalled=1;
        savedMusicTimerVector=btechHostGetInterruptVector(TIMER_INTERRUPT);
        btechHostSetInterruptVector(TIMER_INTERRUPT,BTECH_MUSIC_TIMER_VECTOR);
        savedMusicTimerDivisor=pitChannel0DivisorShadow;
    }
    btechHostSetInterrupts(0);musicTimerCallback=callback;btechHostSetInterrupts(1);
    pitChannel0DivisorShadow=divisor;
    btechHostWritePort(PIT_CHANNEL_ZERO_PORT,(uint8_t)divisor);
    btechHostWritePort(PIT_CHANNEL_ZERO_PORT,(uint8_t)(divisor>>8));
}

/*1ADF:0FC9. No CLI/STI or control-port write. The saved divisor returns to
 * the PIT, but the native shadow and callback deliberately remain unchanged. */
void BtechRestoreMusicTimerInterrupt(void)
{
    if (!musicTimerInstalled) return;
    if (!btechHostWritePort || !btechHostSetInterruptVector) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !btechHostWritePort || !btechHostSetInterruptVector");
    btechHostWritePort(PIT_CHANNEL_ZERO_PORT,(uint8_t)savedMusicTimerDivisor);
    btechHostWritePort(PIT_CHANNEL_ZERO_PORT,(uint8_t)(savedMusicTimerDivisor>>8));
    musicTimerInstalled=0;
    btechHostSetInterruptVector(TIMER_INTERRUPT,savedMusicTimerVector);
}

/*1ADF:0290. Timer restoration can change the priority owner; select the final
 * tick callback only after clearing matching tone activity. */
uint16_t BtechStopAllAudioChannels(void)
{
    BtechRestoreSoundTimerInterrupt();
    if (!samplePriorityOwner) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !samplePriorityOwner");
    *samplePriorityOwner=0;
    BtechStopMatchingToneChannels(0,3);
    BtechSelectTandyOrFallbackAudioCallback()();
    return 1;
}

/*1ADF:00F2. Preserve the four native cleanup calls in this order. Normal and
 * fatal program exits do not call this library entry in the reviewed image. */
uint16_t BtechShutdownAudioBackends(void)
{
    BtechRestoreMusicTimerInterrupt();
    BtechStopMusicStreams();
    BtechRestoreMpuInterruptHandler();
    return BtechStopAllAudioChannels();
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
        BtechReadStreamElements(ignoredHeader,2,1,stream->file);
        BtechReadStreamElements(eventCount,2,1,stream->file);bank->eventCount=MwReadNativeWord(eventCount,0);
        BtechReadStreamElements(unknown,2,1,stream->file);bank->unidentifiedField9=MwReadNativeWord(unknown,0);
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
        if (!bank->events) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !bank->events"); /* Host representation, not a new native error branch. */
        bank->eventReferenceCapacity=bank->eventCount;
    }
    if (bank->eventCount>bank->eventReferenceCapacity) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: bank->eventCount>bank->eventReferenceCapacity"); /* Unrecovered old-directory aliases after a count-changing slot10 reuse. */
    for (int16_t event=0;event<(int16_t)bank->eventCount;++event)
    {
        uint32_t entry=(uint16_t)((uint16_t)event<<2);
        if (entry+4>node->allocation.byteCount) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: entry+4>node->allocation.byteCount");
        uint16_t offset=MwReadNativeWord(node->allocation.bytes,(uint16_t)entry);
        uint16_t paragraph=MwReadNativeWord(node->allocation.bytes,(uint16_t)(entry+2));
        if (!(offset|paragraph)) continue;
        uint32_t eventOffset=(uint32_t)paragraph*0x10+offset;
        if (eventOffset>=node->allocation.byteCount) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: eventOffset>=node->allocation.byteCount");
        uint32_t paragraphBase=(uint32_t)paragraph*0x10;
        bank->events[event]=(BtechSoundEvent){{node->allocation.bytes+paragraphBase,0,node->allocation.byteCount-paragraphBase},
            offset,(uint16_t)(node->allocationSelector+paragraph)};
    }
    return 1;
}

/*1ADF:054C. Keep decode/request/close/link order, including the failed-append
 * overwrite of the old tail. A malformed first partial length has unknown
 * stack residue; gate it rather than zero-fill an invented allocation size. */
BtechSoundAllocationNode *BtechLoadTaggedSoundAllocation(BtechSoundBank *bank,BtechRuntimeStream *stream,const char *tag)
{
    if (BtechFindResourceTagPosition(stream,tag,0)&0x80000000) return NULL;
    BtechReadStreamByte(stream->file);
    uint8_t lengthBytes[4];
    if (BtechReadStreamElements(lengthBytes,4,1,stream->file)!=1) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: BtechReadStreamElements(lengthBytes,4,1,stream->file)!=1");
    uint32_t remaining=(uint32_t)MwReadNativeWord(lengthBytes,0)|((uint32_t)MwReadNativeWord(lengthBytes,2)<<16);
    fseek(stream->file,-5,SEEK_CUR);
    BtechResourceAllocation allocation=soundAllocationOverride;
    if (!allocation.bytes)
    {
        /*1EB6 deliberately wraps its paragraph request. A corrupt sound length
         * must not let a smaller native allocation masquerade as a host buffer
         * of the full declared size. Foreign-owner overwrite is not supported. */
        if (remaining>DOS_MAX_PARAGRAPH_ALLOCATION_BYTES) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: remaining>DOS_MAX_PARAGRAPH_ALLOCATION_BYTES");
        allocation.bytes=BtechAllocateDosMemoryBlock(remaining).bytes;
        allocation.byteCount=remaining;
        if (!allocation.bytes) soundAllocationError=1;
    }
    int16_t archive=BtechOpenCompressedArchive(UINT16_MAX,stream->file,"rb",BtechGetTaggedPayloadLength(stream));
    uint16_t failed=archive<0;
    uint32_t cursor=0;
    while (!failed && remaining)
    {
        uint16_t chunk=remaining>COMBAT_SOUND_CHUNK_BYTES?COMBAT_SOUND_CHUNK_BYTES:remaining;
        if (!allocation.bytes || cursor+chunk>allocation.byteCount) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !allocation.bytes || cursor+chunk>allocation.byteCount"); /* Native NULL/foreign writes are not host memory. */
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
            while (node->next)
            {
                node=BtechHostResolveReleasedSoundNode(node->next);
            }
            BtechSoundAllocationNode *next=BtechAllocateNearBytes(BTECH_NEAR_SOUND_NODE_BYTES,sizeof(*node),BTECH_NEAR_SOUND_NODE);
            if (node==&bank->root) node->next=next;
            else BtechHostStoreSoundAllocationNext(node,next);
            if (node->next) {node=node->next;node->next=NULL;}
            else {soundAllocationError=2;failed=2;}
        }
        node->allocation=allocation;
        if (!btechHostDosAllocationSelector) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !btechHostDosAllocationSelector");
        node->allocationSelector=btechHostDosAllocationSelector(allocation.bytes);
        node->allocationOffset=0;
        if (strlen(tag)>=sizeof(node->chunkTag)) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: strlen(tag)>=sizeof(node->chunkTag)");
        strcpy(node->chunkTag,tag);
    }
    if (!failed) return node;
    if (allocation.bytes)
    {
        if (!btechHostFreeDosAllocation) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !btechHostFreeDosAllocation");
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

/*1ADF:075E. A music bank chooses one backend directory, unlike the sound
 * bank's merges. The shipped AdLib branch also requests MUS:ROL:, not ADL. */
uint16_t BtechLoadMusicBank(const char *filename,BtechRuntimeStream *stream)
{
    enum { MUSIC_INFORMATION_MAGIC=0x59D9 };
    uint16_t bankIndex=0;
    while (bankIndex<COMBAT_MUSIC_LOAD_ROOTS && soundBanks[COMBAT_MUSIC_FIRST_ROOT+bankIndex].root.allocation.bytes) ++bankIndex;
    if (bankIndex==COMBAT_MUSIC_LOAD_ROOTS) return 0;
    bool openedHere=!stream || !BtechIsRegisteredTaggedStream(stream);
    if (openedHere) stream=BtechOpenTaggedResourceStream(filename);
    if (!stream)
    {
        if (openedHere) BtechCloseTaggedResourceStream(stream);
        return 0; /* Unwritten count is tested, but failure remains set on this path. */
    }
    /* Missing/partial INF leaves stack WORDs unwritten in the executable.
     * Do not fabricate zeros or reinterpret that residue as a valid bank. */
    if (BtechFindResourceTagPosition(stream,"MUS:INF:",0)&0x80000000) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: BtechFindResourceTagPosition(stream,\"MUS:INF:\",0)&0x80000000");
    uint8_t header[2];
    if (BtechReadStreamElements(header,1,2,stream->file)!=2) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: BtechReadStreamElements(header,1,2,stream->file)!=2");
    if (MwReadNativeWord(header,0)!=MUSIC_INFORMATION_MAGIC) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: MwReadNativeWord(header,0)!=MUSIC_INFORMATION_MAGIC"); /* Count is still stack residue. */
    if (BtechReadStreamElements(header,1,2,stream->file)!=2) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: BtechReadStreamElements(header,1,2,stream->file)!=2");
    uint16_t trackCount=MwReadNativeWord(header,0);
    for (int16_t track=0;track<(int16_t)trackCount;++track)
    {
        uint16_t sequenceWord=bankIndex*COMBAT_MUSIC_SEQUENCE_WORDS+track;
        if (sequenceWord>=COMBAT_MUSIC_SEQUENCE_ROWS*COMBAT_MUSIC_SEQUENCE_WORDS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: sequenceWord>=COMBAT_MUSIC_SEQUENCE_ROWS*COMBAT_MUSIC_SEQUENCE_WORDS");
        /* Native unchecked counts can spill into the next represented row. */
        int16_t *next=&musicSequences[sequenceWord/COMBAT_MUSIC_SEQUENCE_WORDS][sequenceWord%COMBAT_MUSIC_SEQUENCE_WORDS];
        header[0]=(uint8_t)*next;header[1]=(uint8_t)((uint16_t)*next>>8);
        BtechReadStreamElements(header,1,2,stream->file); /* Native partial reads preserve unwritten destination bytes. */
        *next=(int16_t)MwReadNativeWord(header,0);
    }
    BtechSoundBank *bank=&soundBanks[COMBAT_MUSIC_FIRST_ROOT+bankIndex];
    bank->eventCount=trackCount;bank->unidentifiedField9=trackCount;
    uint16_t failed=0;
    if (trackCount)
    {
        failed=1;
        const char *tag=NULL;
        if (selectedAudioBackendMask&COMBAT_SOUND_MPU) tag="MUS:ROL:";
        else if (selectedAudioBackendMask&COMBAT_SOUND_ADLIB) tag="MUS:ROL:";
        else if (selectedAudioBackendMask&COMBAT_SOUND_TANDY) tag="MUS:TAN:";
        else if (selectedAudioBackendMask&COMBAT_SOUND_SPEAKER) tag="MUS:IBM:";
        if (tag) failed&=BtechMergeSoundBackendDirectory(bank,stream,tag)<1;
    }
    if (openedHere) BtechCloseTaggedResourceStream(stream);
    return failed?0:bankIndex+1;
}

/*1ADF:0D08. Selection alone does not stop playback. Native signed bounds admit
 * cross-owner negative indices and slot15; those aliases remain explicit gates. */
uint16_t BtechSelectActiveMusicBank(uint16_t bankHandle)
{
    --bankHandle;
    if ((int16_t)bankHandle>COMBAT_MUSIC_BANKS-1) bankHandle=UINT16_MAX;
    else
    {
        if (bankHandle>=COMBAT_MUSIC_LOAD_ROOTS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: bankHandle>=COMBAT_MUSIC_LOAD_ROOTS");
        if (!soundBanks[COMBAT_MUSIC_FIRST_ROOT+bankHandle].root.allocation.bytes) bankHandle=UINT16_MAX;
    }
    activeMusicBankIndex=bankHandle;
    ++bankHandle;
    return (int16_t)bankHandle>0?bankHandle:0;
}

/*1ADF:024C. Clear the selector before the cursor; MPU stops short-circuit both
 * AdLib note-count cleanup and speaker music-channel cleanup. */
void BtechStopMusicStreams(void)
{
    midiStreamSelectorPresent=false;combatMidiStreamOffset=0;
    if (selectedAudioBackendMask&COMBAT_SOUND_MPU) {BtechStopMpuMusicTrack();return;}
    if (selectedAudioBackendMask&COMBAT_SOUND_ADLIB) BtechClearAdlibNoteReferenceCounts();
    if (selectedAudioBackendMask&COMBAT_SOUND_SPEAKER) BtechStopMatchingToneChannels(0,2);
}

/*1ADF:013C. Stop first, then reload the selected bank. A zero track count
 * deliberately wraps the unsigned DEC comparison rather than rejecting all. */
uint16_t BtechStartSelectedMusicBankTrack(uint16_t trackIndex,uint16_t priority)
{
    if ((int16_t)activeMusicBankIndex<0) return 0;
    if (activeMusicBankIndex>=COMBAT_MUSIC_LOAD_ROOTS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: activeMusicBankIndex>=COMBAT_MUSIC_LOAD_ROOTS");
    if ((uint16_t)(soundBanks[COMBAT_MUSIC_FIRST_ROOT+activeMusicBankIndex].eventCount-1)<trackIndex) return 0;
    BtechStopMusicStreams();
    if (activeMusicBankIndex>=COMBAT_MUSIC_LOAD_ROOTS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: activeMusicBankIndex>=COMBAT_MUSIC_LOAD_ROOTS");
    BtechSoundBank *bank=&soundBanks[COMBAT_MUSIC_FIRST_ROOT+activeMusicBankIndex];
    if (!bank->root.allocation.bytes || !bank->events || trackIndex>=bank->eventReferenceCapacity) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !bank->root.allocation.bytes || !bank->events || trackIndex>=bank->eventReferenceCapacity");
    BtechSoundEvent track=bank->events[trackIndex];
    BtechStartSoftwareMusicTrack(track.data,track.offset,track.selector!=0);
    if (selectedAudioBackendMask&COMBAT_SOUND_MPU) BtechStartMpuMusicTrack(track.data,track.offset);
    else if (!(selectedAudioBackendMask&COMBAT_SOUND_ADLIB)) BtechSetToneMusicPriorityAndTranslation(priority);
    musicSequenceIndex=trackIndex;
    return 1;
}

/*1ADF:0C8A. The embedded root's next link and tag survive release. E96E marks
 * a near-heap header free; CDD9's FFFF probe reaches FAAD->FB7B, changing heap
 * metadata, not the node payload. Retain typed near nodes until child reset so
 * the subsequent native next-field read is valid C, never a use-after-free. */
uint16_t BtechFreeSoundAllocationChain(BtechSoundBank *bank)
{
    BtechSoundAllocationNode *node=&bank->root;
    if (!node->allocation.bytes) return 0;
    if (!soundAllocationOverride.bytes)
    {
        if (!btechHostFreeDosSelector) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !btechHostFreeDosSelector");
        btechHostFreeDosSelector(node->allocationSelector);
    }
    node->allocationSelector=0;node->allocationOffset=0;node->allocation=(BtechResourceAllocation){0};
    while ((node=node->next)!=NULL)
    {
        node=BtechHostResolveReleasedSoundNode(node);
        if (!soundAllocationOverride.bytes && (node->allocationOffset|node->allocationSelector))
        {
            if (!btechHostFreeDosSelector) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !btechHostFreeDosSelector");
            btechHostFreeDosSelector(node->allocationSelector);
        }
        BtechReleaseNearBlockAndProbeAllocation(node);
    }
    /* Decoded directory metadata has no native allocation identity. */
    free(bank->events);bank->events=NULL;bank->eventReferenceCapacity=0;
    return 1;
}

/*1ADF:0106. Active comparison precedes bounds and does not clear selection. */
uint16_t BtechUnloadMusicBank(uint16_t bankHandle)
{
    --bankHandle;
    if (activeMusicBankIndex==bankHandle) BtechStopMusicStreams();
    if ((int16_t)bankHandle<0 || (int16_t)bankHandle>COMBAT_MUSIC_BANKS-1) return 0;
    if (bankHandle>=COMBAT_MUSIC_LOAD_ROOTS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: bankHandle>=COMBAT_MUSIC_LOAD_ROOTS");
    return BtechFreeSoundAllocationChain(&soundBanks[COMBAT_MUSIC_FIRST_ROOT+bankHandle]);
}

/*1ADF:0C52. Sound-bank bounds precede the active comparison, unlike music.
 * Invalidate the active sound bank before stop-all can invoke a callback. */
uint16_t BtechUnloadSoundBank(uint16_t bankHandle)
{
    --bankHandle;
    if ((int16_t)bankHandle<0 || (int16_t)bankHandle>=COMBAT_SOUND_BANKS) return 0;
    if (activeSoundBankIndex==bankHandle)
    {
        activeSoundBankIndex=UINT16_MAX;
        BtechStopAllAudioChannels();
    }
    return BtechFreeSoundAllocationChain(&soundBanks[bankHandle]);
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
    if (activeSoundBankIndex>=COMBAT_SOUND_BANKS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: activeSoundBankIndex>=COMBAT_SOUND_BANKS");
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

/*1ADF:091A. Tone records, native MPU terminal-byte output and real no-op cases.
 * Sample/DAC output retains explicit gates rather than invented playback. */
uint16_t BtechDispatchSoundBankEvent(int16_t soundEventIndex,uint16_t priority)
{
    if ((int16_t)activeSoundBankIndex<0) return 0;
    if (activeSoundBankIndex>=COMBAT_SOUND_BANKS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: activeSoundBankIndex>=COMBAT_SOUND_BANKS");
    BtechSoundBank *bank=&soundBanks[activeSoundBankIndex];
    if ((int16_t)(bank->eventCount-1)<soundEventIndex) return 0;
    if (soundEventIndex<0 || (uint16_t)soundEventIndex>=bank->eventReferenceCapacity || !bank->events) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: soundEventIndex<0 || (uint16_t)soundEventIndex>=bank->eventReferenceCapacity || !bank->events");
    BtechSoundEvent event=bank->events[soundEventIndex];
    uint16_t type=MwReadNativeViewWord(&event.data,event.offset);
    switch (type)
    {
    case 0:case 1:return BtechAllocateToneEventChannels(event,priority,(uint16_t)soundEventIndex);
    case 2:
        if (!(soundEffectsEnableMask&1) || !(selectedAudioBackendMask&COMBAT_SOUND_SPEAKER)) return 0;
        /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Native uninitialized priority-channel WORD and sample IRQ not converted."); /* Native uninitialized priority-channel WORD and sample IRQ not converted. */
    case 3:return 1;
    case 4:
        if (!(soundEffectsEnableMask&1) || !(selectedAudioBackendMask&0x08)) return 0;
        /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Tandy SL/TL DAC path."); /* Tandy SL/TL DAC path. */
    case 0x0B:
        if (!(selectedAudioBackendMask&COMBAT_SOUND_MPU)) return 1;
        if (!BtechSendMpuCommandStack(0xDF)) return 1;
        {
            /* 1ADF:09DA..0A15 advances to the terminal BYTE, not a payload
             * transmit loop. Do not turn it into a plausible SysEx upload. */
            uint16_t index=0;
            while (MwReadNativeViewWord(&event.data,(uint16_t)(event.offset+2))>index) ++index;
            uint32_t payload=(uint32_t)event.offset+10;
            /* Only the first addition carries into the selector. The later
             * payload/index addition wraps its offset without another carry. */
            uint32_t terminal=(payload&0x10000)+(uint16_t)((uint16_t)payload+index);
            if (!event.data.bytes || event.data.nativeBase || terminal>=event.data.byteCount) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !event.data.bytes || event.data.nativeBase || terminal>=event.data.byteCount");
            MpuWriteDataByte(event.data.bytes[terminal]);return 1;
        }
    default:return 0;
    }
}
