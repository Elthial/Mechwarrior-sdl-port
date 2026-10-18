#include "mechwarrior/campaign_music.h"
#include <stdlib.h>
#include <string.h>

/* Writable native callback identities stay observable; synthesis lives below
 * the original byte/register outputs, not in a replacement host sequencer. */

/* MW_MAIN0F41:0B3C..0B45. */
uint16_t MwProbeMusicBackend(MwCampaignAudioBackend *backend)
{
    if (!backend) abort();
    if (backend->probeOffset==0x0912) return MwProbeMpuBackend();
    if (backend->probeOffset!=0x0EE4) abort();
    return MwProbeSpeakerBackend();
}

/* MW_MAIN0F41:0B46..0B7F. Pending timer arguments precede initialization.
 * Speaker preserves BX. MPU's different BX residue is NOT generalized here. */
void MwSelectAndInitializeMidiBackend(MwCampaignAudioBackend *backend)
{
    if (selectedAudioBackend)
    {
        if (selectedAudioBackend->shutdownOffset==0x098A) MwDrainAndShutdownMpuBackend();
        else if (selectedAudioBackend->shutdownOffset==0x0F01) MwShutdownSpeakerBackend();
        else abort();
    }
    selectedAudioBackend=backend;
    if (!backend) abort();
    uint16_t timerRate=backend->timerRateShift;
    uint16_t timerTarget=backend->timerOffset;
    if (backend->initializeOffset==0x0946)
    {
        MwInitializeMpuUartBackend();
        /* 0B6C uses BX left by DOS GET VECTOR, NOT the selected descriptor.
         * The existing host's zero saved-vector identity aliases this header.
         * Do not silently set MPU selectedVoiceCount to voiceCapacity instead. */
        uint16_t previousOffset=(uint16_t)mpuPreviousIrq;
        if ((uint32_t)previousOffset+0x17>=sizeof(musicRuntimeHeader)) abort();
        musicRuntimeHeader[previousOffset+0x17]=musicRuntimeHeader[previousOffset+3];
    }
    else
    {
        if (backend->initializeOffset!=0x0EE8) abort();
        MwInitializeSpeakerBackend();
        backend->selectedVoiceCount=backend->voiceCapacity;
    }
    MwResetMidiParserState(MIDI_RESET_PARSER_ENTRY);
    if (timerTarget==0x09D0) MwTimerSetCallback(MUSIC_TIMER_FAST_SLOT,MwTickMpuTransmitQueue,timerRate);
    else if (timerTarget==0x0F73) MwTimerSetCallback(MUSIC_TIMER_FAST_SLOT,MwTickSpeakerVoices,timerRate);
    else abort();
}

/* MW_MAIN0F41:0B80..0B9F. */
void MwShutdownMusicBackend(void)
{
    if (selectedAudioBackend)
    {
        if (selectedAudioBackend->shutdownOffset==0x098A) MwDrainAndShutdownMpuBackend();
        else if (selectedAudioBackend->shutdownOffset==0x0F01) MwShutdownSpeakerBackend();
        else abort();
    }
    selectedAudioBackend=NULL;
    MwTimerSetCallback(MUSIC_TIMER_FAST_SLOT,NULL,0);
}

/* MW_MAIN0F41:0BA0..0BAD. PC hook deliberately ignores the full input WORD. */
uint16_t MwSendMidiByteToSelectedBackend(uint16_t midiWord)
{
    if (selectedAudioBackend && selectedAudioBackend->rawByteOffset==0x09A4) return MwQueueMpuByte(midiWord);
    if (!selectedAudioBackend || selectedAudioBackend->rawByteOffset!=0x0F0A) abort();
    return MwIgnoreSpeakerRawMidiByte();
}

/* MW_MAIN0F41:0BAE..0C36. Detach/reinsert before the note-off callback. */
uint16_t MwReleaseCampaignMidiVoice(uint16_t channelIndex,uint16_t note)
{
    uint16_t channelOffset=MIDI_CHANNELS_OFFSET+channelIndex*sizeof(MwMidiChannel);
    MwMidiChannel *channel=MwResolveMidiChannel(channelOffset);
    uint8_t eligibility=channel->backendEligibilityMask;
    if (!eligibility) return 0;
    if (eligibility&MIDI_DRUM_CHANNEL_BIT)
    {
        uint8_t drumIndex=(uint8_t)note-channel->transposeOrDrumBase;
        if (drumIndex>=MIDI_DRUM_CHANNEL_COUNT) return 0;
        channelIndex=(note&0xFF00)|(uint8_t)(drumIndex+MIDI_DRUM_CHANNEL_FIRST);
        channelOffset=MIDI_CHANNELS_OFFSET+channelIndex*sizeof(MwMidiChannel);
        note&=0xFF00;
    }
    uint16_t voiceOffset=MIDI_LIST_HEAD_OFFSET;
    for (;;)
    {
        voiceOffset=MwResolveMidiVoiceLinks(voiceOffset)->nextOffset;
        if (!MwResolveMidiVoiceLinks(voiceOffset)->nextOffset) return 0;
        MwMidiVoice *voice=MwResolveMidiVoice(voiceOffset);
        if (voice->channelOffset!=channelOffset || !voice->activePriority
            || voice->originalNote!=(uint8_t)note) continue;
        voice->activePriority=0;
        uint16_t next=voice->links.nextOffset,previous=voice->links.previousOffset;
        MwResolveMidiVoiceLinks(previous)->nextOffset=next;
        MwResolveMidiVoiceLinks(next)->previousOffset=previous;
        next=audioVoiceListHead;
        voice->links.nextOffset=next;
        voice->links.previousOffset=MIDI_LIST_HEAD_OFFSET;
        audioVoiceListHead=voiceOffset;
        MwResolveMidiVoiceLinks(next)->previousOffset=voiceOffset;
        if (!selectedAudioBackend || selectedAudioBackend->noteOffOffset!=0x0F2D) abort();
        MwStopSpeakerVoice(voice);
        return 1;
    }
}

/* MW_MAIN0F41:0C37..0CEC. Stop at the first higher-priority voice; equal
 * priorities are inserted BEFORE existing equals. Reservation balances wrap. */
uint16_t MwAllocateCampaignMidiVoice(uint16_t channelIndex,uint16_t note,uint16_t velocity)
{
    (void)velocity; /* Native speaker hook does not consume velocity AX. */
    uint16_t channelOffset=MIDI_CHANNELS_OFFSET+channelIndex*sizeof(MwMidiChannel);
    MwMidiChannel *channel=MwResolveMidiChannel(channelOffset);
    uint8_t eligibility=channel->backendEligibilityMask;
    if (!eligibility) return 0;
    if (eligibility&MIDI_DRUM_CHANNEL_BIT)
    {
        uint8_t drumIndex=(uint8_t)note-channel->transposeOrDrumBase;
        if (drumIndex>=MIDI_DRUM_CHANNEL_COUNT) return 0;
        channelIndex=(note&0xFF00)|(uint8_t)(drumIndex+MIDI_DRUM_CHANNEL_FIRST);
        channelOffset=MIDI_CHANNELS_OFFSET+channelIndex*sizeof(MwMidiChannel);
        note&=0xFF00;
        channel=MwResolveMidiChannel(channelOffset);
    }
    uint8_t priority=channel->voicePriority;
    eligibility=channel->backendEligibilityMask;
    uint16_t voiceOffset=MIDI_LIST_HEAD_OFFSET;
    MwMidiVoice *voice;
    for (;;)
    {
        voiceOffset=MwResolveMidiVoiceLinks(voiceOffset)->nextOffset;
        if (!MwResolveMidiVoiceLinks(voiceOffset)->nextOffset) return 0;
        voice=MwResolveMidiVoice(voiceOffset);
        if (voice->activePriority>priority) return 0;
        if (!(voice->backendMask&eligibility)) continue;
        MwMidiChannel *donor=MwResolveMidiChannel(voice->channelOffset);
        if (voice->channelOffset==channelOffset) break;
        if (!donor->reservationBalance) continue;
        --donor->reservationBalance;
        ++channel->reservationBalance;
        voice->channelOffset=channelOffset;
        break;
    }
    voice->activePriority=priority;
    uint16_t next=voice->links.nextOffset,previous=voice->links.previousOffset;
    MwResolveMidiVoiceLinks(previous)->nextOffset=next;
    MwResolveMidiVoiceLinks(next)->previousOffset=previous;
    while (MwResolveMidiVoiceLinks(next)->nextOffset
        && MwResolveMidiVoice(next)->activePriority<priority)
        next=MwResolveMidiVoiceLinks(next)->nextOffset;
    previous=MwResolveMidiVoiceLinks(next)->previousOffset;
    voice->links.nextOffset=next;
    voice->links.previousOffset=previous;
    MwResolveMidiVoiceLinks(previous)->nextOffset=voiceOffset;
    MwResolveMidiVoiceLinks(next)->previousOffset=voiceOffset;
    voice->originalNote=(uint8_t)note;
    voice->translatedNote=(uint8_t)(note+channel->transposeOrDrumBase);
    if (!selectedAudioBackend || selectedAudioBackend->noteOnOffset!=0x0F0D) abort();
    MwStartSpeakerVoice(voice);
    return 1;
}

/* MW_MAIN0F41:0CEF..0D16. Follow live links after each callback. */
void MwSilenceCampaignMidiVoices(void)
{
    uint16_t voiceOffset=MIDI_LIST_HEAD_OFFSET;
    for (;;)
    {
        voiceOffset=MwResolveMidiVoiceLinks(voiceOffset)->nextOffset;
        if (!MwResolveMidiVoiceLinks(voiceOffset)->nextOffset) return;
        MwMidiVoice *voice=MwResolveMidiVoice(voiceOffset);
        if (!voice->activePriority) continue;
        voice->activePriority=0;
        if (!selectedAudioBackend || selectedAudioBackend->noteOffOffset!=0x0F2D) abort();
        MwStopSpeakerVoice(voice);
    }
}

/* MW_MAIN0F41:0D17..0D3E. Full WORD program affects the instrument offset. */
void MwSetCampaignMidiProgram(uint16_t channelIndex,uint16_t instrumentId)
{
    MwMidiChannel *channel=MwResolveMidiChannel((uint16_t)(MIDI_CHANNELS_OFFSET+channelIndex*sizeof(MwMidiChannel)));
    channel->instrumentId=(uint8_t)instrumentId;
    channel->instrumentOffset=MIDI_INSTRUMENTS_OFFSET+instrumentId*MIDI_INSTRUMENT_BYTES;
}

/* MW_MAIN0F41:0574..0698. Interior JMP targets share the original FAR frame.
 * The native second-data handlers deliberately DO NOT reset the parser. */
void MwDispatchMidiByte(uint16_t midiWord)
{
    if (MwSendMidiByteToSelectedBackend(midiWord)) return;
    uint8_t midiByte=(uint8_t)midiWord;
    if (midiByte>=MIDI_STATUS_BYTE)
    {
        if (midiByte<MIDI_SYSTEM_STATUS)
        {
            midiDispatchChannel=(midiWord&0xFF00)|(midiByte&0x0F);
            midiByteHandlerDisplacement=(midiWord&0xFF70)>>2;
            return;
        }
        if (midiByte==0xF0)
        {
            MwDispatchMidiByteAndSysexState(midiWord);
            midiByteHandlerDisplacement=0x0016;
            return;
        }
        if (midiByte==0xF7) MwDispatchMidiByteAndSysexState(midiWord);
        else if (midiByte==0xFF) MwResetMidiParserState(MIDI_RESET_PARSER_ENTRY);
        else if (midiByte>=MIDI_REALTIME_STATUS) return;
        midiByteHandlerDisplacement=0x0012;
        return;
    }
    uint16_t displacement=midiByteHandlerDisplacement;
    /* Unrepresented out-of-table code fetches are not a native bounds check. */
    if (displacement%2 || displacement/2>=14) abort();
    switch (midiByteHandlers[displacement/2])
    {
    case 0x05C4:
        midiFirstDataByte=midiWord;
        midiByteHandlerDisplacement=displacement+2;
        return;
    case 0x05CC:
        MwReleaseCampaignMidiVoice(midiDispatchChannel,midiFirstDataByte);return;
    case 0x05DE:
        if (!midiByte) MwReleaseCampaignMidiVoice(midiDispatchChannel,midiFirstDataByte);
        else MwAllocateCampaignMidiVoice(midiDispatchChannel,midiFirstDataByte,midiWord);
        return;
    case 0x0625:
        if (midiFirstDataByte==1)
            midiControllerValue=(uint16_t)(((uint16_t)midiByte<<8)|(midiWord>>8))>>1;
        else if (midiFirstDataByte==33)
            midiControllerValue^=(midiWord^midiControllerValue)&0x7F;
        else if (midiFirstDataByte>=MIDI_CONTROL_ALL_NOTES_OFF) MwSilenceCampaignMidiVoices();
        return; /* Native0CEE is one RETF, no synthesis effect. */
    case 0x0664: MwSetCampaignMidiProgram(midiDispatchChannel,midiWord);return;
    case 0x0594:
        MwDispatchMidiByteAndSysexState(midiWord);
        midiByteHandlerDisplacement=0x0016;return;
    case 0x05F5: /* Native0CED poly-pressure is one RETF. */
    case 0x0673: /* Native0D3F channel-pressure is one RETF. */
    case 0x0682: /* Native0D40 pitch-bend is one RETF, result unused. */
    case 0x0621: return;
    default: abort();
    }
}

/* MW_MAIN0F41:0D41..0DB1. BYTE writes retain the proved workspace aliases;
 * a malformed cursor outside that owned workspace is unsupported execution. */
void MwDispatchMidiByteAndSysexState(uint16_t midiWord)
{
    uint8_t midiByte=(uint8_t)midiWord;
    if (midiByte==0xF0) {midiSysex.parserOffset=0x0D58;return;}
    if (midiByte==0xF7) {midiSysex.parserOffset=0x0DB0;return;}
    switch (midiSysex.parserOffset)
    {
    case 0x0D58:
        if (!selectedAudioBackend) abort();
        midiSysex.parserOffset=midiByte==selectedAudioBackend->sysexManufacturer?0x0D66:0x0DB0;
        return;
    case 0x0D66:
        if (midiByte>=3) {midiSysex.parserOffset=0x0DB0;return;}
        midiSysex.handlerDisplacement=(uint16_t)(midiWord<<1);
        midiSysex.parserOffset=0x0D74;return;
    case 0x0D74: midiSysex.recordLength=midiByte;break;
    case 0x0D7E:
    {
        uint16_t writeOffset=midiSysex.writeOffset;
        uint16_t relative=writeOffset-MIDI_SYSEX_BUFFER_OFFSET;
        if (relative>=sizeof(midiSysex)) abort();
        ((uint8_t *)&midiSysex)[relative]=midiByte;
        midiSysex.writeOffset=writeOffset+1;
        if (--midiSysex.bytesRemaining) return;
        uint16_t displacement=midiSysex.handlerDisplacement;
        if (displacement%2 || displacement/2>=3) abort();
        switch (midiSysex.recordHandlers[displacement/2])
        {
        case 0x0DB2: MwApplySysexChannelConfiguration((const MwMidiChannelConfiguration *)midiSysex.recordBuffer);break;
        case 0x0E57: MwApplySysexInstrument(midiSysex.recordBuffer);break;
        case 0x0E73: MwApplySysexVoiceCount(midiSysex.recordBuffer);break;
        default: abort();
        }
        break;
    }
    case 0x0DB0: return;
    default: abort();
    }
    midiSysex.bytesRemaining=midiSysex.recordLength;
    midiSysex.writeOffset=MIDI_SYSEX_BUFFER_OFFSET;
    midiSysex.parserOffset=0x0D7E;
}

/* MW_MAIN0F41:0DB2..0E56. Six sequential BYTE reads, not a source snapshot.
 * E23 rereads OLD predecessor+6; head+6 is the first voice's NEXT LINK.
 * Consequently the final writer can be a voice prefix, not the requested
 * channel. Numeric links and the canonical channelAlias preserve this bug. */
void MwApplySysexChannelConfiguration(const MwMidiChannelConfiguration *configuration)
{
    const uint8_t *source=(const uint8_t *)configuration;
    uint16_t writerOffset=MIDI_CHANNELS_OFFSET+*source++*sizeof(MwMidiChannel);
    MwMidiChannel *writer=MwResolveMidiChannel(writerOffset);
    writer->backendEligibilityMask=*source++;
    writer->voicePriority=*source++;
    uint8_t delta=*source++-writer->reservedVoiceCount;
    if (delta&0x80)
    {
        writer->reservedVoiceCount+=delta;
        writer->reservationBalance-=delta;
    }
    else if (delta)
    {
        uint16_t budget=delta;
        if (writer->reservationBalance)
        {
            do
            {
                ++writer->reservedVoiceCount;
                --writer->reservationBalance;
                --budget;
            } while (budget && writer->reservationBalance);
        }
        if (budget)
        {
            uint8_t eligibility=writer->backendEligibilityMask;
            uint16_t scan=MIDI_LIST_HEAD_OFFSET,insertionPredecessor=scan;
            for (;;)
            {
                scan=MwResolveMidiVoiceLinks(scan)->nextOffset;
                uint16_t following=MwResolveMidiVoiceLinks(scan)->nextOffset;
                if (!--budget || !following) break;
                MwMidiVoice *voice=MwResolveMidiVoice(scan);
                if (!(voice->backendMask&eligibility)) continue;
                MwMidiChannel *donor=MwResolveMidiChannel(voice->channelOffset);
                if (!donor->reservationBalance) continue;
                --donor->reservationBalance;
                ++writer->reservedVoiceCount;
                voice->channelOffset=writerOffset;
                voice->activePriority=0;
                uint16_t next=voice->links.nextOffset,oldPrevious=voice->links.previousOffset;
                MwResolveMidiVoiceLinks(oldPrevious)->nextOffset=next;
                MwResolveMidiVoiceLinks(next)->previousOffset=oldPrevious;
                uint16_t insertionBase=insertionPredecessor;
                insertionPredecessor=oldPrevious;
                next=MwResolveMidiVoiceLinks(insertionBase)->nextOffset;
                voice->links.nextOffset=next;
                voice->links.previousOffset=insertionBase;
                MwResolveMidiVoiceLinks(insertionBase)->nextOffset=scan;
                MwResolveMidiVoiceLinks(next)->previousOffset=scan;
                scan=insertionPredecessor;
                insertionPredecessor=insertionBase;
                if (scan==MIDI_LIST_HEAD_OFFSET) writerOffset=audioVoices[0].links.nextOffset;
                else if (scan==MIDI_LIST_TAIL_OFFSET) writerOffset=audioVoices[0].links.previousOffset;
                else writerOffset=MwResolveMidiVoice(scan)->channelOffset;
                writer=MwResolveMidiChannel(writerOffset);
            }
        }
    }
    uint8_t instrumentId=*source++;
    writer->instrumentId=instrumentId;
    writer->instrumentOffset=MIDI_INSTRUMENTS_OFFSET+instrumentId*MIDI_INSTRUMENT_BYTES;
    uint8_t transpose=*source++;
    if (transpose&MIDI_DRUM_CHANNEL_BIT) transpose|=0x80;
    writer->transposeOrDrumBase=transpose;
}

/* MW_MAIN0F41:0E57..0E72. PC0F72 is a genuine RET-only decoder. Native
 * source advancement/DI arithmetic have no caller-visible PC synthesis use. */
void MwApplySysexInstrument(const uint8_t *source)
{
    (void)*source;
    if (!selectedAudioBackend || selectedAudioBackend->decodeInstrumentOffset!=0x0F72) abort();
    MwIgnoreSpeakerInstrument();
}
/* MW_MAIN0F41:0E73..0E81. No capacity clamp before the shared rebuild. */
void MwApplySysexVoiceCount(const uint8_t *source)
{
    if (!selectedAudioBackend) abort();
    selectedAudioBackend->selectedVoiceCount=*source;
    MwRebuildCampaignVoicePool();
}
/* MW_MAIN0F41:0E82 /0E9B shared0E9D..0EE3. entryOffset is host JMP
 * transport, not a new native argument. Tail.next is deliberately NOT reset. */
void MwResetMidiParserState(uint16_t entryOffset)
{
    if (entryOffset==MIDI_RESET_PARSER_ENTRY)
    {
        midiSysex.parserOffset=0x0DB0;
        memset(audioChannels,0,sizeof(audioChannels));
    }
    else if (entryOffset!=MIDI_REBUILD_POOL_ENTRY) abort();
    uint16_t previousHead=MIDI_LIST_TAIL_OFFSET;
    audioVoiceListHead=previousHead;
    audioVoiceListTailPrevious=MIDI_LIST_HEAD_OFFSET;
    if (!selectedAudioBackend) abort();
    uint16_t remaining=selectedAudioBackend->selectedVoiceCount;
    audioVoiceCount=(uint8_t)remaining;
    uint16_t voiceOffset=MIDI_VOICES_OFFSET;
    while (remaining)
    {
        MwMidiVoice *voice=MwResolveMidiVoice(voiceOffset);
        voice->channelOffset=MIDI_DUMMY_CHANNEL_OFFSET;
        voice->activePriority=0;
        voice->links.nextOffset=previousHead;
        voice->links.previousOffset=MIDI_LIST_HEAD_OFFSET;
        audioVoiceListHead=voiceOffset;
        MwResolveMidiVoiceLinks(previousHead)->previousOffset=voiceOffset;
        previousHead=voiceOffset;
        voiceOffset+=sizeof(MwMidiVoice);
        --remaining;
    }
    if (!selectedAudioBackend) abort();
    selectedAudioBackend->voicePoolOffset=MIDI_VOICES_OFFSET;
    if (selectedAudioBackend->initializeVoicesOffset==0x09CB) (void)MwQueueMpuByte(0xFF);
    else if (selectedAudioBackend->initializeVoicesOffset==0x0F4F) MwInitializeSpeakerVoiceRouting();
    else abort();
}

/* MW_MAIN0F41:0EE4..0EE7. Native unconditional success, no hardware probe. */
uint16_t MwProbeSpeakerBackend(void) {return 1;}
/* MW_MAIN0F41:0EE8..0F00. */
void MwInitializeSpeakerBackend(void)
{
    if (!mwMusicHardware.setInterruptsEnabled || !mwMusicHardware.writePort || !mwMusicHardware.readPort) abort();
    mwMusicHardware.setInterruptsEnabled(false);
    mwMusicHardware.writePort(PIT_CONTROL_PORT,PIT_CHANNEL2_SQUARE_WAVE);
    midiSysex.currentPeriod=SPEAKER_SILENT_PERIOD;
    mwMusicHardware.writePort(PIT_CHANNEL2_PORT,SPEAKER_SILENT_PERIOD);
    mwMusicHardware.writePort(PIT_CHANNEL2_PORT,0);
    uint8_t control=mwMusicHardware.readPort(SPEAKER_CONTROL_PORT);
    mwMusicHardware.writePort(SPEAKER_CONTROL_PORT,control|SPEAKER_GATE_BITS);
    mwMusicHardware.setInterruptsEnabled(true);
}
/* MW_MAIN0F41:0F01..0F09. */
void MwShutdownSpeakerBackend(void)
{
    if (!mwMusicHardware.setInterruptsEnabled || !mwMusicHardware.writePort || !mwMusicHardware.readPort) abort();
    mwMusicHardware.setInterruptsEnabled(false);
    uint8_t control=mwMusicHardware.readPort(SPEAKER_CONTROL_PORT);
    mwMusicHardware.writePort(SPEAKER_CONTROL_PORT,control&~SPEAKER_GATE_BITS);
    mwMusicHardware.setInterruptsEnabled(true);
}
/* MW_MAIN0F41:0F0A..0F0C. Genuine XOR AX,AX /RET. */
uint16_t MwIgnoreSpeakerRawMidiByte(void) {return 0;}
/* MW_MAIN0F41:0F0D..0F2C. Higher BYTE notes read adjacent authored WORDs. */
uint16_t MwStartSpeakerVoice(MwMidiVoice *voice)
{
    uint16_t noteIndex=(uint16_t)(voice->translatedNote-SPEAKER_FIRST_NOTE);
    uint16_t period=(noteIndex&0x8000)?SPEAKER_SILENT_PERIOD:midiSysex.tonePeriodsAndAdjacentWords[noteIndex];
    voice->pitPeriod=period;
    if (speakerVoiceCount<=1) return MwWriteSpeakerPeriod(period);
    return period;
}
/* MW_MAIN0F41:0F2D..0F3A. */
uint16_t MwStopSpeakerVoice(MwMidiVoice *voice)
{
    voice->pitPeriod=SPEAKER_SILENT_PERIOD;
    if (speakerVoiceCount<=1) return MwWriteSpeakerPeriod(SPEAKER_SILENT_PERIOD);
    return SPEAKER_SILENT_PERIOD;
}
/* MW_MAIN0F41:0F3B..0F4E. Native MOV AL,BH preserves AH on return. */
uint16_t MwWriteSpeakerPeriod(uint16_t period)
{
    if (!mwMusicHardware.setInterruptsEnabled || !mwMusicHardware.writePort) abort();
    mwMusicHardware.setInterruptsEnabled(false);
    midiSysex.currentPeriod=period;
    mwMusicHardware.writePort(PIT_CONTROL_PORT,PIT_CHANNEL2_SQUARE_WAVE);
    mwMusicHardware.writePort(PIT_CHANNEL2_PORT,(uint8_t)period);
    mwMusicHardware.writePort(PIT_CHANNEL2_PORT,(uint8_t)(period>>8));
    mwMusicHardware.setInterruptsEnabled(true);
    return (period&0xFF00)|(period>>8);
}
/* MW_MAIN0F41:0F4F..0F71. */
void MwInitializeSpeakerVoiceRouting(void)
{
    midiSysex.currentVoiceOffset=speakerVoicePoolOffset;
    midiSysex.voicesRemaining=speakerVoiceCount;
    uint16_t remaining=speakerVoiceCount,voiceOffset=speakerVoicePoolOffset;
    while (remaining)
    {
        MwMidiVoice *voice=MwResolveMidiVoice(voiceOffset);
        voice->backendMask=1;
        voice->pitPeriod=SPEAKER_SILENT_PERIOD;
        voiceOffset+=sizeof(MwMidiVoice);
        --remaining;
    }
}
/* MW_MAIN0F41:0F72. Genuine single NEAR RET, not an invented stub. */
void MwIgnoreSpeakerInstrument(void) {}
/* MW_MAIN0F41:0F73..0FBB. No CLI/STI on this IRQ callback. */
void MwTickSpeakerVoices(void)
{
    if (--midiSysex.rotationCountdown) return;
    midiSysex.rotationCountdown=SPEAKER_ROTATION_TICKS;
    if (speakerVoiceCount<=1) return;
    MwMidiVoice *voice=MwResolveMidiVoice(midiSysex.currentVoiceOffset);
    uint16_t period=voice->pitPeriod;
    if (period!=midiSysex.currentPeriod)
    {
        midiSysex.currentPeriod=period;
        if (!mwMusicHardware.writePort) abort();
        mwMusicHardware.writePort(PIT_CONTROL_PORT,PIT_CHANNEL2_SQUARE_WAVE);
        mwMusicHardware.writePort(PIT_CHANNEL2_PORT,(uint8_t)period);
        mwMusicHardware.writePort(PIT_CHANNEL2_PORT,(uint8_t)(period>>8));
    }
    midiSysex.currentVoiceOffset+=sizeof(MwMidiVoice);
    if (!--midiSysex.voicesRemaining)
    {
        midiSysex.voicesRemaining=speakerVoiceCount;
        midiSysex.currentVoiceOffset=speakerVoicePoolOffset;
    }
}
