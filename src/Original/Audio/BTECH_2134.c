#include "mechwarrior/combat_sound.h"
#include <stdlib.h>

/*2134:02BE. Native speaker bias is CH of a mask already reduced to2: zero. */
void BtechSetToneMusicPriorityAndTranslation(uint16_t priority)
{
    musicPriority=priority;
    if (selectedAudioBackendMask&COMBAT_SOUND_TANDY)
    {musicPitchTableOffset=TONE_TANDY_PITCH_TABLE;musicChannelBias=4;musicNoteBase=0x1E;}
    else
    {musicChannelBias=0;musicPitchTableOffset=TONE_SPEAKER_PITCH_TABLE;musicNoteBase=0x0E;}
}

/*2134:0342. Native table dispatch retains AX residue for229C:00E2.
 * Bend writes through ambient DI, not the BX-selected channel: without a
 * proved DI owner it remains a gate, not an invented channel argument. */
uint16_t BtechDispatchToneMidiMessage(uint16_t statusDisplacement,uint16_t channel,uint16_t packedData)
{
    if ((statusDisplacement&1) || statusDisplacement/2>=8) abort();
    uint16_t handler=musicHandlerOffsets[statusDisplacement/2];
    if (handler==TONE_MUSIC_NO_OPERATION) return packedData;
    if (handler==TONE_MUSIC_PITCH_BEND) abort();
    if (handler!=TONE_MUSIC_NOTE_OFF && handler!=TONE_MUSIC_NOTE_ON) abort();
    channel=(channel&0xFF00)|(uint8_t)((uint8_t)channel+musicChannelBias);
    if (handler==TONE_MUSIC_NOTE_OFF || !(packedData&0xFF00))
    {
        uint16_t residue=musicPriority;
        if ((int16_t)channel>=COMBAT_TONE_CHANNELS) return residue;
        if (channel>=COMBAT_TONE_CHANNELS) abort(); /* Signed-negative adjacent-owner alias. */
        residue=(residue&0xFF00)|(channelFlags[channel].activityMask&2);
        if (residue&2) channelFlags[channel].activityMask=0;
        return residue;
    }
    if ((int8_t)(uint8_t)channel>=COMBAT_TONE_CHANNELS) return packedData;
    if (channel>=COMBAT_TONE_CHANNELS) abort();
    if (soundPriorities[channel]>musicPriority) return packedData;
    channelFlags[channel].activityMask=2;
    BtechToneChannelState *state=toneChannelStates[channel];
    if (!state) abort();
    soundPriorities[channel]=musicPriority;
    state->modulationScale=0;state->periodStep=0;state->bytecodeOffset=0;
    packedData=(packedData&0x00FF)|((packedData&0x7F00)<<1);
    state->outputLevel=packedData;
    uint16_t period;
    if (state==&toneChannels[TONE_NOISE_STATE_INDEX]) /* Native state024E, selected by its address, not channel number. */
    {
        state->levelSequenceOffset=TONE_NOISE_ENVELOPES+packedData*TONE_NOISE_ENVELOPE_BYTES;
        if (musicPriority>=TONE_NOISE_PERIOD_WORDS) abort(); /* Adjacent noise-period/envelope aliases not recovered. */
        period=noiseMusicPeriods[musicPriority];
    }
    else
    {
        uint8_t note=(uint8_t)packedData-musicNoteBase;
        if ((int8_t)note<0) return packedData; /* Activity/priority/level have already changed. */
        if (musicPitchTableOffset==TONE_SPEAKER_PITCH_TABLE) period=speakerMusicPeriods[note];
        else if (musicPitchTableOffset==TONE_TANDY_PITCH_TABLE) period=tandyMusicPeriods[note];
        else abort();
    }
    state->basePeriod=period;state->outputPeriod=period;
    return period;
}

/*2134:0196. Binary commands select even state fields. Signed CMP BL,80/JGE
 * always takes the command path: high-bit opcodes are NOT stop markers. */
void BtechParseToneCommands(BtechToneChannelState *state,MwNativeByteView event,uint16_t cursor,uint16_t channel)
{
    for (;;)
    {
        uint8_t command=MwReadNativeViewByte(&event,cursor++);
        uint16_t field=command&TONE_STATE_FIELD_MASK;
        if (field>0x78)
        {
            uint16_t handler=bytecodeBranchHandlers[(field-0x78)/2];
            if (handler==TONE_BRANCH_STOP) goto stopChannel;
            if (handler==TONE_BRANCH_RETURN) {cursor=bytecodeReturnOffset;continue;}
            uint16_t relative=MwReadNativeViewWord(&event,cursor);cursor+=2;
            if (handler==TONE_BRANCH_CALL) bytecodeReturnOffset=cursor;
            else if (handler==TONE_BRANCH_REPEAT)
            {
                if (state->branchRepeatCount && !--state->branchRepeatCount) continue;
            }
            else abort(); /* Unrecovered modified code-table target. */
            cursor+=relative;continue;
        }
        uint16_t parameter=MwReadNativeViewWord(&event,cursor);cursor+=2;
        if (field<0x2C)
        {
            switch (field)
            {
            case 0x00:state->bytecodeOffset=parameter;break;
            case 0x02:state->commandDelayTicks=parameter;break;
            case 0x04:state->basePeriod=parameter;break;
            case 0x06:state->periodStep=parameter;break;
            case 0x08:state->outputPeriod=parameter;break;
            case 0x0A:state->outputLevel=parameter;break;
            case 0x0C:state->levelStep=parameter;break;
            case 0x0E:case 0x10:case 0x12:case 0x14:state->unidentifiedField8[(field-0x0E)/2]=parameter;break;
            case 0x16:state->levelSequencePosition=parameter;break;
            case 0x18:state->levelSequenceTicks=parameter;break;
            case 0x1A:state->modulationPhase=parameter;break;
            case 0x1C:state->modulationPhaseStep=parameter;break;
            case 0x1E:state->modulationScale=(int16_t)parameter;break;
            case 0x20:state->branchRepeatCount=parameter;break;
            case 0x22:case 0x24:case 0x26:case 0x28:state->unidentifiedField13[(field-0x22)/2]=parameter;break;
            case 0x2A:state->modulationPhaseLimit=parameter;break;
            }
            if (field!=2) continue;
            if (!parameter) goto stopChannel;
            break;
        }
        if (field==0x2C)
        {
            uint16_t entry=levelSequenceDirectoryOffsets[channel]+parameter*2;
            state->levelSequenceOffset=MwReadNativeViewWord(&event,entry);
            state->commandDelayTicks=0;break;
        }
        uint16_t entry=fallbackAllocationOffsets[channel]+parameter*4;
        state->modulationTableOffset=MwReadNativeViewWord(&toneFallbackData[channel],entry);
        state->modulationPhaseLimit=MwReadNativeViewWord(&toneFallbackData[channel],(uint16_t)(entry+2));
    }
    state->bytecodeOffset=cursor;return;
stopChannel:
    channelFlags[channel].activityMask=0;
    soundPriorities[channel]=0;
    state->bytecodeOffset=cursor;
}

/*2134:0101. Zero phase sum does not publish phase; subtraction wraps once.
 * Native SAR4 and signed IMUL high WORD are made explicit without signed UB. */
void BtechAdvanceToneChannel(BtechToneChannelState *state,MwNativeByteView event,uint16_t channel)
{
    if (!--state->commandDelayTicks && state->bytecodeOffset)
        BtechParseToneCommands(state,event,state->bytecodeOffset,channel);
    state->outputLevel+=state->levelStep;
    state->basePeriod+=state->periodStep;
    uint16_t phase=state->modulationPhase+state->modulationPhaseStep;
    uint16_t modulationHigh=0;
    if (phase)
    {
        if (phase>=state->modulationPhaseLimit) phase-=state->modulationPhaseLimit;
        state->modulationPhase=phase;
        uint16_t shiftedPhase=(phase>>4)|((phase&0x8000)?0xF000:0);
        uint16_t entry=shiftedPhase+state->modulationTableOffset;
        int16_t modulation=(uint16_t)MwReadNativeViewByte(&toneFallbackData[channel],entry)<<8;
        int32_t product=(int32_t)modulation*state->modulationScale;
        modulationHigh=(uint16_t)((uint32_t)product>>16);
    }
    state->outputPeriod=modulationHigh+state->basePeriod;
    if (!state->levelSequenceTicks || --state->levelSequenceTicks) return;
    for (;;)
    {
        uint16_t entry=state->levelSequenceOffset+state->levelSequencePosition;
        uint16_t level=MwReadNativeViewWord(&event,entry),ticks=MwReadNativeViewWord(&event,(uint16_t)(entry+2));
        state->levelSequencePosition+=4;
        if (ticks==UINT16_MAX)
        {
            state->outputLevel=level;
            if (!level) state->levelStep=0;
            continue;
        }
        state->levelStep=level;state->levelSequenceTicks=ticks;return;
    }
}

/*2134:0000. Choose BEFORE advancing each active channel. Unsigned priority
 * ties replace in descending traversal, so lower channel wins. */
void BtechTickSpeakerToneChannels(void)
{
    if (!audioTickCallback) abort();
    audioTickCallback();
    soundOutputEnableMask=soundEffectsEnableMask;
    selectedSpeakerState=NULL;selectedSpeakerPriority=0;
    for (int16_t channel=COMBAT_SPEAKER_CHANNELS-1;channel>=0;--channel)
    {
        uint8_t activity=channelFlags[channel].activityMask;
        if (!activity) continue;
        BtechToneChannelState *state=toneChannelStates[channel];
        if ((activity&soundOutputEnableMask) && soundPriorities[channel]>=selectedSpeakerPriority)
        {selectedSpeakerPriority=soundPriorities[channel];selectedSpeakerState=state;}
        currentChannelDisplacement=channel*2;
        BtechAdvanceToneChannel(state,toneEventData[channel],channel);
    }
    if (!btechHostWritePort || !btechHostReadPort) abort();
    uint8_t gateBits=0;
    if (selectedSpeakerState)
    {
        btechHostWritePort(TONE_PIT_DATA_PORT,(uint8_t)selectedSpeakerState->outputPeriod);
        btechHostWritePort(TONE_PIT_DATA_PORT,(uint8_t)(selectedSpeakerState->outputPeriod>>8));
        if (selectedSpeakerState->outputLevel) gateBits=TONE_SPEAKER_GATE_BITS;
    }
    uint8_t gate=btechHostReadPort(TONE_SPEAKER_GATE_PORT);
    btechHostWritePort(TONE_SPEAKER_GATE_PORT,(gate&~TONE_SPEAKER_GATE_BITS)|gateBits);
}

/*2134:0086. Noise inherits transformed period AX for volume; masked/inactive
 * channels inherit flags AX. Neither path reads a fabricated mute level. */
void BtechTickTandyToneChannels(void)
{
    BtechTickSpeakerToneChannels();
    for (uint16_t channel=COMBAT_SPEAKER_CHANNELS;channel<COMBAT_TONE_CHANNELS;++channel)
    {
        uint8_t latch=0x80+(channel-COMBAT_SPEAKER_CHANNELS)*0x20;
        uint16_t output=channelFlags[channel].activityMask;
        if (output)
        {
            BtechToneChannelState *state=toneChannelStates[channel];
            currentChannelDisplacement=channel*2;
            BtechAdvanceToneChannel(state,toneEventData[channel],channel);
            output=channelFlags[channel].activityMask;
            if (output&soundOutputEnableMask)
            {
                output=state->outputPeriod>>2;
                output=(output&0xFF00)|((uint8_t)output>>4);output^=latch;
                btechHostWritePort(TONE_TANDY_DATA_PORT,(uint8_t)output);
                if (latch!=0xE0)
                {btechHostWritePort(TONE_TANDY_DATA_PORT,(uint8_t)(output>>8));output=state->outputLevel;}
            }
        }
        output^=UINT16_MAX;
        btechHostWritePort(TONE_TANDY_DATA_PORT,(uint8_t)(((output>>12)^latch)+0x10));
    }
}

/*2134:0248. Zero state WORDs descending, except cursor until the final store.
 * Bytecode/fallback views borrow real allocations; no copied segment image. */
void BtechStartToneInterpreterChannel(MwNativeByteView event,uint16_t eventOffset,uint16_t channel,MwNativeByteView fallback,uint16_t fallbackOffset)
{
    if (channel>=COMBAT_TONE_CHANNELS || !toneChannelStates[channel]) abort();
    channelFlags[channel].activityMask=1;
    BtechToneChannelState *state=toneChannelStates[channel];
    toneEventData[channel]=event;
    fallbackAllocationOffsets[channel]=fallbackOffset;toneFallbackData[channel]=fallback;
    state->modulationTableOffset=0;state->levelSequenceOffset=0;state->modulationPhaseLimit=0;
    for (uint16_t word=4;word;--word) state->unidentifiedField13[word-1]=0;
    state->branchRepeatCount=0;state->modulationScale=0;state->modulationPhaseStep=0;
    state->modulationPhase=0;state->levelSequenceTicks=0;state->levelSequencePosition=0;
    for (uint16_t word=4;word;--word) state->unidentifiedField8[word-1]=0;
    state->levelStep=0;state->outputLevel=0;state->outputPeriod=0;
    state->periodStep=0;state->basePeriod=0;state->commandDelayTicks=0;
    state->bytecodeOffset=eventOffset+COMBAT_TONE_HEADER_BYTES;
    state->commandDelayTicks=1;
}

/*2134:0302. Descending channels; stop identity OR any requested activity bit.
 * Other interpreter state and the flags' second BYTE remain untouched. */
void BtechStopMatchingToneChannels(uint16_t soundEventIndex,uint8_t activityMask)
{
    for (int16_t channel=COMBAT_TONE_CHANNELS-1;channel>=0;--channel)
    {
        if (soundEventIndices[channel]==soundEventIndex || (channelFlags[channel].activityMask&activityMask))
        {
            soundPriorities[channel]=0;
            channelFlags[channel].activityMask=0;
            toneChannelStates[channel]->outputLevel=0;
            channelFlags[channel].activityMask=0; /* Repeated native store. */
        }
    }
}
