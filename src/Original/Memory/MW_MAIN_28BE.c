/* MW_MAIN.EXE runtime data owner28BE; authored values are decoded at EXE load. */
#include "mechwarrior/campaign_music.h"
#include "mechwarrior/original_data.h"
#include "mechwarrior/native_address.h"

uint16_t midiTickDenominator,midiTickNumerator,midiTickRemainder;
uint16_t midiBeatDivision,midiBarDivision;
uint16_t midiStreamOffset,midiStreamSegment;
uint16_t midiPlaybackFlags,midiEventTickBalance;
uint16_t midiBeatRemainder,midiBarRemainder,midiElapsedBars;
uint8_t midiStateUpdateInProgress,midiTickInProgress,midiTrackRunningStatus;
uint16_t midiTrackChannelHandlers[MIDI_TRACK_CHANNEL_HANDLERS];
uint16_t midiTrackSystemHandlers[MIDI_TRACK_SYSTEM_HANDLERS];
MwMidiVoicePool audioVoicePool;
MwMidiChannel audioChannels[MIDI_CHANNEL_RECORDS],audioDummyChannel;
MwMidiSysexWorkspace midiSysex;
MwCampaignAudioBackend *selectedAudioBackend;
MwCampaignAudioBackend mpuBackend;
uint8_t mpuTransmitQueue[MIDI_MPU_QUEUE_CAPACITY];
volatile uint16_t mpuQueueWriteIndex,mpuQueueReadIndex,mpuQueueByteCount;
uint32_t mpuPreviousIrq;
uint8_t musicRuntimeHeader[0x42];
uint16_t midiByteHandlerDisplacement,midiDispatchChannel,midiFirstDataByte,midiControllerValue;
uint16_t midiByteHandlers[14];

/* EXE-loader boundary only. These are authored runtime values, not blanket
 * host defaults; the borrowed binary storage is not retained by game code. */
bool MwImportCampaignMusicData(const uint8_t *data,size_t byteCount)
{
    uint16_t hostEndian=1;
    if (!data || byteCount<0x0A12 || *(uint8_t *)&hostEndian!=1) return false;
    uint16_t backendOffset=MwReadNativeWord(data,0x03F8);
    if (backendOffset && backendOffset!=MIDI_SPEAKER_BACKEND_OFFSET && backendOffset!=MIDI_MPU_BACKEND_OFFSET) return false;
    for (size_t i=0;i<sizeof(musicRuntimeHeader);++i) musicRuntimeHeader[i]=data[i];
    for (size_t i=0;i<sizeof(mpuBackend);++i) ((uint8_t *)&mpuBackend)[i]=data[MIDI_MPU_BACKEND_OFFSET+i];
    for (size_t i=0;i<sizeof(mpuTransmitQueue);++i) mpuTransmitQueue[i]=data[0x0224+i];
    mpuQueueWriteIndex=MwReadNativeWord(data,0x0324);
    mpuQueueReadIndex=MwReadNativeWord(data,0x0326);
    mpuQueueByteCount=MwReadNativeWord(data,0x0328);
    mpuPreviousIrq=(uint32_t)MwReadNativeWord(data,0x0220)|((uint32_t)MwReadNativeWord(data,0x0222)<<16);
    midiTickDenominator=MwReadNativeWord(data,0x0042);
    midiTickNumerator=MwReadNativeWord(data,0x0044);
    midiTickRemainder=MwReadNativeWord(data,0x0046);
    midiBeatDivision=MwReadNativeWord(data,0x0048);
    midiBarDivision=MwReadNativeWord(data,0x004A);
    midiStreamOffset=MwReadNativeWord(data,0x004C);
    midiStreamSegment=MwReadNativeWord(data,0x004E);
    midiPlaybackFlags=MwReadNativeWord(data,0x0050);
    midiEventTickBalance=MwReadNativeWord(data,0x0052);
    midiBeatRemainder=MwReadNativeWord(data,0x0054);
    midiBarRemainder=MwReadNativeWord(data,0x0056);
    midiElapsedBars=MwReadNativeWord(data,0x0058);
    midiStateUpdateInProgress=data[0x005A];
    midiTickInProgress=data[0x005B];
    midiTrackRunningStatus=data[0x005C];
    for (uint16_t i=0;i<MIDI_TRACK_CHANNEL_HANDLERS;++i)
        midiTrackChannelHandlers[i]=MwReadNativeWord(data,(uint16_t)(0x005D+i*2));
    for (uint16_t i=0;i<MIDI_TRACK_SYSTEM_HANDLERS;++i)
        midiTrackSystemHandlers[i]=MwReadNativeWord(data,(uint16_t)(0x006B+i*2));
    for (uint16_t i=0;i<14;++i) midiByteHandlers[i]=MwReadNativeWord(data,(uint16_t)(0x008C+i*2));
    midiByteHandlerDisplacement=MwReadNativeWord(data,0x00A8);
    midiDispatchChannel=MwReadNativeWord(data,0x00AA);
    midiFirstDataByte=MwReadNativeWord(data,0x00AC);
    midiControllerValue=MwReadNativeWord(data,0x00AE);
    /* These typed native records have proved BYTE aliases and exact widths.
     * Import once at IO; ordinary procedures do not keep a backing DS image. */
    for (size_t i=0;i<sizeof(audioVoiceList);++i) ((uint8_t *)&audioVoiceList)[i]=data[0x03FA+i];
    for (size_t i=0;i<sizeof(audioVoices);++i) ((uint8_t *)audioVoices)[i]=data[0x0400+i];
    for (size_t i=0;i<sizeof(audioChannels);++i) ((uint8_t *)audioChannels)[i]=data[0x04C6+i];
    for (size_t i=0;i<sizeof(audioDummyChannel);++i) ((uint8_t *)&audioDummyChannel)[i]=data[0x0646+i];
    for (size_t i=0;i<sizeof(midiSysex);++i) ((uint8_t *)&midiSysex)[i]=data[0x07CE+i];
    selectedAudioBackend=backendOffset==MIDI_MPU_BACKEND_OFFSET?&mpuBackend:backendOffset?&speakerBackend:NULL;
    return true;
}
