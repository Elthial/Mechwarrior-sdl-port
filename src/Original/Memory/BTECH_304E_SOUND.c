#include "mechwarrior/combat_sound.h"
#include "mechwarrior/native_address.h"
#include "mechwarrior/combat_runtime.h"
#include "mechwarrior/combat_adlib.h"
#include "mechwarrior/combat_mpu.h"
#include <stdlib.h>

BtechRuntimeStream runtimeStreams[COMBAT_STREAM_SLOTS];
uint16_t lastStreamOffset,streamSetupCounter;
BtechTaggedStream taggedStreams[COMBAT_TAGGED_STREAMS],taggedStreamBackup;
BtechSoundBank soundBanks[COMBAT_AUDIO_BANK_ROOTS];
uint16_t selectedAudioBackendMask,activeSoundBankIndex,soundAllocationError;
BtechResourceAllocation soundAllocationOverride;
uint16_t timerSoundEventIndex,savedSoundTimerDivisor;
uint8_t soundTimerInstalled;
uint8_t musicTimerInstalled;
uint16_t savedMusicTimerDivisor;
uint32_t savedMusicTimerVector;
uint8_t adlibProbeInitialStatus;
uint8_t soundEffectsEnableMask;
void (*audioTickCallback)(void);
BtechMusicMessage audioMusicCallback;
uint8_t softwareMusicRateShift,softwareMusicDivisionScale;
uint16_t midiTrackIndex;
uint8_t midiRunningStatus,midiMessageData[2],midiMessageDataLength,midiChannel;
uint8_t midiMessageLengths[8];
uint16_t musicSequenceIndex,mpuTempoInterval,combatMidiStreamOffset;
uint16_t queuedMidiDeltaTicks,softwareMusicDelayTicks,midiTempoHighBytes;
MwNativeByteView midiStreamData;
bool midiStreamSelectorPresent;
uint16_t activeMusicBankIndex;
int16_t musicSequences[COMBAT_MUSIC_SEQUENCE_ROWS][COMBAT_MUSIC_SEQUENCE_WORDS];
uint16_t mpuProbeDataPort,mpuProbeStatusPort,mpuProbeCommandPort;
uint16_t mpuProbeReceivedByteCount;
uint32_t mpuProbePreviousIrq;
uint16_t *samplePriorityOwner;

/* Child-image IO only. Do not call this to reset an already-running child. */
bool BtechImportSoundInitialData(const uint8_t *data,size_t bytes)
{
    if (!data || bytes<0x10000 || !BtechImportRuntimeInitialData(data,bytes)
        || !BtechImportAdlibInitialData(data,bytes) || !BtechImportMpuInitialData(data,bytes)) return false;
    for (uint16_t bank=0;bank<COMBAT_AUDIO_BANK_ROOTS;++bank)
    {
        free(soundBanks[bank].events);
        soundBanks[bank]=(BtechSoundBank){0};
    }
    for (uint16_t stream=0;stream<COMBAT_TAGGED_STREAMS;++stream) taggedStreams[stream]=(BtechTaggedStream){0};
    taggedStreamBackup=(BtechTaggedStream){0};soundAllocationOverride=(BtechResourceAllocation){0};
    lastStreamOffset=MwReadNativeWord(data,0x4CD8);
    if (lastStreamOffset!=COMBAT_STREAM_TABLE+(COMBAT_STREAM_SLOTS-1)*COMBAT_STREAM_STRIDE) return false;
    for (uint16_t slot=0;slot<COMBAT_STREAM_SLOTS;++slot)
    {
        runtimeStreams[slot].file=NULL;
        runtimeStreams[slot].buffer=NULL;
        runtimeStreams[slot].flags=data[COMBAT_STREAM_TABLE+slot*COMBAT_STREAM_STRIDE+6];
        runtimeStreams[slot].dosHandle=data[COMBAT_STREAM_TABLE+slot*COMBAT_STREAM_STRIDE+7];
    }
    streamSetupCounter=MwReadNativeWord(data,0x4DEA);
    selectedAudioBackendMask=MwReadNativeWord(data,0x28D4);
    savedMusicTimerDivisor=MwReadNativeWord(data,0x28D6);
    savedMusicTimerVector=MwReadNativeWord(data,0x28D8)|((uint32_t)MwReadNativeWord(data,0x28DA)<<16);
    musicTimerInstalled=data[0x28DC];musicTimerCallback=BtechAudioTimerNoOp;
    soundEffectsEnableMask=data[0x28DD];
    /* These are the actual authored pairs, not a replacement empty tick. */
    if (MwReadNativeWord(data,0x2C9B)!=0x0020 || MwReadNativeWord(data,0x2C9D)!=0x229C
        || MwReadNativeWord(data,0x2C9F)!=0x0051 || MwReadNativeWord(data,0x2CA1)!=0x229C) return false;
    audioMusicCallback=BtechMusicMessageNoOperation;
    audioTickCallback=BtechTickSoftwareMusic;
    softwareMusicRateShift=data[0x2C80];midiTrackIndex=MwReadNativeWord(data,0x2C81);
    musicSequenceIndex=MwReadNativeWord(data,0x2C83);softwareMusicDivisionScale=data[0x2C85];
    mpuTempoInterval=MwReadNativeWord(data,0x2C86);
    combatMidiStreamOffset=MwReadNativeWord(data,0x2C88);
    if (combatMidiStreamOffset || MwReadNativeWord(data,0x2C8A)) return false;
    midiStreamData=(MwNativeByteView){0};midiStreamSelectorPresent=false;
    queuedMidiDeltaTicks=MwReadNativeWord(data,0x2C8C);midiRunningStatus=data[0x2C8E];
    midiMessageData[0]=data[0x2C8F];midiMessageData[1]=data[0x2C90];
    softwareMusicDelayTicks=MwReadNativeWord(data,0x2C91);
    for (uint16_t index=0;index<8;++index) midiMessageLengths[index]=data[0x2C93+index];
    midiMessageDataLength=data[0x2CA3];midiChannel=data[0x2CA4];
    midiTempoHighBytes=MwReadNativeWord(data,0x2CA5);
    activeMusicBankIndex=MwReadNativeWord(data,0x2B4E);
    for (uint16_t bank=0;bank<COMBAT_MUSIC_SEQUENCE_ROWS;++bank)
        for (uint16_t index=0;index<COMBAT_MUSIC_SEQUENCE_WORDS;++index)
            musicSequences[bank][index]=(int16_t)MwReadNativeWord(data,0x7D8A+bank*0x14+index*2);
    mpuProbeDataPort=MwReadNativeWord(data,0x2CA8);
    mpuProbeStatusPort=MwReadNativeWord(data,0x2CAA);mpuProbeCommandPort=MwReadNativeWord(data,0x2CAC);
    mpuProbePreviousIrq=MwReadNativeWord(data,0x2CAE)|((uint32_t)MwReadNativeWord(data,0x2CB0)<<16);
    mpuProbeReceivedByteCount=MwReadNativeWord(data,0x2CB2);
    adlibProbeInitialStatus=data[0x2CB4];
    activeSoundBankIndex=MwReadNativeWord(data,0x2B4C);
    soundAllocationError=MwReadNativeWord(data,0x2B56);
    return true;
}
