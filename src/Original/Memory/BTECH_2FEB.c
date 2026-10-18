#include "mechwarrior/combat_sound.h"
#include "mechwarrior/native_address.h"

uint16_t soundEventIndices[COMBAT_TONE_CHANNELS],soundPriorities[COMBAT_TONE_CHANNELS];
uint16_t sampleSoundPriority;
BtechToneChannelFlags channelFlags[COMBAT_TONE_CHANNELS];
BtechToneChannelState toneChannels[COMBAT_TONE_CHANNELS];
BtechToneChannelState *toneChannelStates[COMBAT_TONE_CHANNELS];
MwNativeByteView toneEventData[COMBAT_TONE_CHANNELS],toneFallbackData[COMBAT_TONE_CHANNELS];
uint16_t fallbackAllocationOffsets[COMBAT_TONE_CHANNELS],levelSequenceDirectoryOffsets[COMBAT_TONE_CHANNELS];
uint16_t bytecodeReturnOffset,currentChannelDisplacement,selectedSpeakerPriority;
uint16_t bytecodeBranchHandlers[4];
BtechToneChannelState *selectedSpeakerState;
uint8_t soundOutputEnableMask;
uint8_t musicNoteBase,musicChannelBias;
uint16_t musicPitchTableOffset,musicPriority,musicHandlerOffsets[8];
uint16_t speakerMusicPeriods[TONE_SPEAKER_PITCH_WORDS],tandyMusicPeriods[TONE_TANDY_PITCH_WORDS],noiseMusicPeriods[TONE_NOISE_PERIOD_WORDS];
_Static_assert(sizeof(BtechToneChannelState)==0x30,"Native tone record");

bool BtechImportToneInitialData(const uint8_t *data,size_t bytes,const uint8_t *toneData,size_t toneBytes)
{
    if (!data || bytes<0x4DFE || !toneData || toneBytes<0x622) return false;
    /* Shipped priority owner is2FEB, NOT304E. Other selectors require recovery
     * of their actual owning field, not a new segment address registry. */
    if (MwReadNativeWord(data,0x4DFC)!=0x2FEB) return false;
    samplePriorityOwner=&sampleSoundPriority;
    sampleSoundPriority=MwReadNativeWord(toneData,0x00AF);
    soundOutputEnableMask=toneData[0];
    musicNoteBase=toneData[1];musicPitchTableOffset=MwReadNativeWord(toneData,2);
    musicChannelBias=toneData[4];musicPriority=MwReadNativeWord(toneData,5);
    for (uint16_t index=0;index<8;++index) musicHandlerOffsets[index]=MwReadNativeWord(toneData,7+index*2);
    for (uint16_t index=0;index<TONE_SPEAKER_PITCH_WORDS;++index) speakerMusicPeriods[index]=MwReadNativeWord(toneData,TONE_SPEAKER_PITCH_TABLE+index*2);
    for (uint16_t index=0;index<TONE_TANDY_PITCH_WORDS;++index) tandyMusicPeriods[index]=MwReadNativeWord(toneData,TONE_TANDY_PITCH_TABLE+index*2);
    for (uint16_t index=0;index<TONE_NOISE_PERIOD_WORDS;++index) noiseMusicPeriods[index]=MwReadNativeWord(toneData,0x61C+index*2);
    bytecodeReturnOffset=MwReadNativeWord(toneData,0x00F0);
    currentChannelDisplacement=MwReadNativeWord(toneData,0x003D);
    selectedSpeakerPriority=MwReadNativeWord(toneData,0x001B);
    if (MwReadNativeWord(toneData,0x00F4)) return false;
    selectedSpeakerState=NULL;
    for (uint16_t entry=0;entry<4;++entry) bytecodeBranchHandlers[entry]=MwReadNativeWord(toneData,0x00F6+entry*2);
    for (uint16_t channel=0;channel<COMBAT_TONE_CHANNELS;++channel)
    {
        toneEventData[channel]=(MwNativeByteView){0};toneFallbackData[channel]=(MwNativeByteView){0};
        fallbackAllocationOffsets[channel]=MwReadNativeWord(toneData,0x003F+channel*2);
        levelSequenceDirectoryOffsets[channel]=MwReadNativeWord(toneData,0x007F+channel*2);
        soundEventIndices[channel]=MwReadNativeWord(toneData,0x001D+channel*2);
        soundPriorities[channel]=MwReadNativeWord(toneData,0x009F+channel*2);
        channelFlags[channel]=(BtechToneChannelFlags){toneData[0x00DF+channel*2],toneData[0x00E0+channel*2]};
        uint16_t offset=MwReadNativeWord(toneData,0x039E+channel*2);
        if (offset<0x00FE || (offset-0x00FE)%0x30 || (offset-0x00FE)/0x30>=COMBAT_TONE_CHANNELS) return false;
        toneChannelStates[channel]=&toneChannels[(offset-0x00FE)/0x30];
        /* All authored state WORDs are zero. Reject a different image rather
         * than interpreting an aligned host record as little-endian file data. */
        for (uint16_t byte=0;byte<0x30;++byte) if (toneData[0x00FE+channel*0x30+byte]) return false;
        toneChannels[channel]=(BtechToneChannelState){0};
    }
    soundTimerInstalled=data[0x28D0];
    savedSoundTimerDivisor=MwReadNativeWord(data,0x28D1);
    timerSoundEventIndex=MwReadNativeWord(data,0x28F2);
    return true;
}
