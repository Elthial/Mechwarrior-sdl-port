#include "mechwarrior/combat_adlib.h"
#include "mechwarrior/combat_sound.h"
#include "mechwarrior/combat_startup.h"
#include <stdlib.h>

/* Original combat AdLib instrument/note driver, independent of the campaign
 * driver. Nuked-OPL3 receives only its register writes in OPL2-compatible mode;
 * it does not select instruments or reinterpret the game's score. */
/* 21CA:000E. Preserve the final OPL status as AX, not synthetic success. */
uint16_t BtechInitializeAdlibMusicBackend(void)
{
    BtechResetAdlibOperatorLevels();BtechBuildAdlibFrequencyTables();
    BtechSetAdlibRhythmMode(1);BtechSetAdlibGlobalDepthControls(0,0,0);
    for (uint16_t channel=0;channel<BTECH_OPL_CHANNELS;++channel) BtechClearAdlibChannelFrequency(channel);
    BtechSetAdlibFrequencyStep(1);
    return BtechSetAdlibWaveformSelection(1);
}
/* 21CA:005C. Rhythm setup writes the old state before publishing the new flag. */
void BtechSetAdlibRhythmMode(uint16_t enabled)
{
    if (enabled)
    {
        BtechClearAdlibChannelFrequency(6);BtechClearAdlibChannelFrequency(7);BtechClearAdlibChannelFrequency(8);
        BtechSetAdlibChannelNoteAndKeyState(8,24,0);BtechSetAdlibChannelNoteAndKeyState(7,31,0);
    }
    combatAdlibRhythm=(uint8_t)enabled;combatAdlibPercussionKeys=0;
    BtechLoadDefaultAdlibInstruments();BtechWriteAdlibRhythmDepthState();
}
/* 21CA:00B8. Reset every operator waveform before enabling waveform selection. */
uint16_t BtechSetAdlibWaveformSelection(uint16_t enabled)
{
    combatAdlibWaveforms=enabled?BTECH_OPL_WAVEFORM_ENABLE:0;
    for (unsigned operatorIndex=0;operatorIndex<BTECH_OPL_OPERATORS;++operatorIndex)
        BtechWriteAdlibRegister((int8_t)combatAdlibRegisterOffsets[operatorIndex]+BTECH_OPL_WAVEFORM_REGISTER,0);
    return BtechWriteAdlibRegister(1,combatAdlibWaveforms);
}
/* 21CA:0102. The unsigned tuning step is clamped to 1..12 semitones. */
void BtechSetAdlibFrequencyStep(uint16_t step)
{
    if (step>12) step=12;
    if (step<1) step=1;
    combatAdlibFrequencyStep=step;combatAdlibBendScale=step*25;
}
/* 21CA:012E. */
void BtechSetAdlibGlobalDepthControls(uint8_t tremolo,uint8_t vibrato,uint8_t noteSelect)
{
    combatAdlibTremolo=tremolo;combatAdlibVibrato=vibrato;combatAdlibNoteSelect=noteSelect;
    BtechWriteAdlibRhythmDepthState();BtechWriteAdlibNoteSelect();
}
/* 21CA:0440. Shipped defaults are two melodic and six percussion operators. */
void BtechLoadDefaultAdlibInstruments(void)
{
    for (uint16_t operatorIndex=0;operatorIndex<BTECH_OPL_OPERATORS;++operatorIndex)
        BtechLoadSignedAdlibInstrumentBytes(operatorIndex,combatAdlibDefaultInstruments[combatAdlibCarrierFlags[operatorIndex]?1:0],0);
    if (combatAdlibRhythm)
    {
        BtechLoadSignedAdlibInstrumentBytes(12,combatAdlibDefaultInstruments[2],0);
        BtechLoadSignedAdlibInstrumentBytes(15,combatAdlibDefaultInstruments[3],0);
        BtechLoadSignedAdlibInstrumentBytes(16,combatAdlibDefaultInstruments[4],0);
        BtechLoadSignedAdlibInstrumentBytes(14,combatAdlibDefaultInstruments[5],0);
        BtechLoadSignedAdlibInstrumentBytes(17,combatAdlibDefaultInstruments[6],0);
        BtechLoadSignedAdlibInstrumentBytes(13,combatAdlibDefaultInstruments[7],0);
    }
}
/* 21CA:04EE. */
void BtechResetAdlibOperatorLevels(void)
{for (unsigned index=0;index<BTECH_OPL_OPERATORS;++index) combatAdlibLevels[index]=0x7F;}
/* 21CA:050E. Low-DWORD arithmetic and signed runtime divisions, not floating tuning. */
uint32_t BtechCalculateAdlibRowBaseFrequency(uint16_t step,int16_t scale)
{
    int32_t scaleProduct=(int16_t)(uint16_t)((uint16_t)scale*100);
    int32_t stepProduct=(int16_t)(uint16_t)(step*6);
    uint32_t numerator=(uint32_t)(scaleProduct+stepProduct)*(0x65BCu*2);
    int32_t denominator=(int32_t)((uint32_t)scaleProduct*25);
    if (!denominator || ((int32_t)numerator==INT32_MIN && denominator==-1)) abort();
    uint32_t ratio=(uint32_t)((int32_t)numerator/denominator);
    numerator=(ratio<<14)*9;
    return (uint32_t)((int32_t)numerator/(0xB3*0x271));
}
/* 21CA:05D8. Each semitone rounds only the low WORD of its accumulator. */
void BtechBuildAdlibSemitoneRow(uint16_t *row,uint16_t step,int16_t scale)
{
    uint32_t frequency=BtechCalculateAdlibRowBaseFrequency(step,scale);
    for (unsigned note=0;note<BTECH_OPL_SEMITONES;++note)
    {
        if (note) frequency=(uint32_t)((int32_t)(frequency*106)/100);
        row[note]=(uint16_t)((uint16_t)((uint16_t)frequency+4)>>3);
    }
}
/* 21CA:0656. The eleven channel row references are ordinary typed references. */
void BtechBuildAdlibFrequencyTables(void)
{
    for (unsigned row=0;row<BTECH_OPL_FREQUENCY_ROWS;++row)
        BtechBuildAdlibSemitoneRow(combatAdlibFrequencies[row],(uint16_t)(row*4),100);
    for (unsigned channel=0;channel<BTECH_OPL_TUNING_CHANNELS;++channel)
    {combatAdlibFrequencyRows[channel]=combatAdlibFrequencies[0];combatAdlibTransposes[channel]=0;}
    for (unsigned note=0;note<BTECH_OPL_NOTES;++note)
    {combatAdlibOctaves[note]=(uint8_t)(note/12);combatAdlibPitchClasses[note]=(uint8_t)(note%12);}
}
/* 21CA:0808. Parameters are thirteen low BYTEs of actual caller WORDs. */
void BtechStoreAdlibInstrumentParameters(uint16_t operatorIndex,const int16_t *parameters,uint16_t waveform)
{
    if (operatorIndex>=BTECH_OPL_OPERATORS) abort();
    uint8_t *destination=(uint8_t *)&combatAdlibOperators[operatorIndex];
    for (unsigned parameter=0;parameter<BTECH_OPL_PARAMETER_BYTES;++parameter) destination[parameter]=(uint8_t)parameters[parameter];
    combatAdlibOperators[operatorIndex].waveform=(uint8_t)(waveform&3);
    BtechApplyAdlibInstrumentRegisters(operatorIndex);
}
/* 21CA:0860. Genuine automatic WORD array and signed BYTE widening. */
void BtechLoadSignedAdlibInstrumentBytes(uint16_t operatorIndex,const uint8_t *parameters,uint16_t waveform)
{
    int16_t widened[BTECH_OPL_PARAMETER_BYTES];
    for (unsigned parameter=0;parameter<BTECH_OPL_PARAMETER_BYTES;++parameter) widened[parameter]=(int8_t)parameters[parameter];
    BtechStoreAdlibInstrumentParameters(operatorIndex,widened,waveform);
}
/* 21CA:0916. Do not reorder the device writes. */
void BtechApplyAdlibInstrumentRegisters(uint16_t operatorIndex)
{
    BtechWriteAdlibRhythmDepthState();BtechWriteAdlibNoteSelect();
    BtechWriteAdlibOperatorLevel(operatorIndex);BtechWriteAdlibFeedbackAndConnection(operatorIndex);
    BtechWriteAdlibAttackDecay(operatorIndex);BtechWriteAdlibSustainRelease(operatorIndex);
    BtechWriteAdlibOperatorModeMultiplier(operatorIndex);BtechWriteAdlibOperatorWaveform(operatorIndex);
}
/* 21CA:0958. MUL discards DX before rounding and unsigned division. */
void BtechWriteAdlibOperatorLevel(uint16_t operatorIndex)
{
    if (operatorIndex>=BTECH_OPL_OPERATORS) abort();
    BtechAdlibOperator *parameters=&combatAdlibOperators[operatorIndex];
    uint16_t amplitude=0x3F-(parameters->attenuation&0x3F);
    uint16_t product=(uint16_t)(amplitude*(uint16_t)(int16_t)(int8_t)combatAdlibLevels[operatorIndex]);
    uint16_t attenuation=(uint16_t)(0x3F-(uint16_t)(product+(uint16_t)(product+0x7F))/0xFE);
    uint16_t scale=(uint16_t)((uint16_t)(int16_t)(int8_t)parameters->keyScaleLevel<<6);
    BtechWriteAdlibRegister((int8_t)combatAdlibRegisterOffsets[operatorIndex]+0x40,attenuation|scale);
}
/* 21CA:09C8. */
void BtechWriteAdlibNoteSelect(void) {BtechWriteAdlibRegister(8,combatAdlibNoteSelect?0x40:0);}
/* 21CA:09E2. Only modulators own the channel connection register. */
void BtechWriteAdlibFeedbackAndConnection(uint16_t operatorIndex)
{
    if (operatorIndex>=BTECH_OPL_OPERATORS) abort();
    if (combatAdlibCarrierFlags[operatorIndex]) return;
    BtechAdlibOperator *parameters=&combatAdlibOperators[operatorIndex];
    uint16_t feedback=(uint16_t)((uint16_t)(int16_t)(int8_t)parameters->feedback<<1);
    feedback|=parameters->fmConnectionEnabled?0:1;
    BtechWriteAdlibRegister((int8_t)combatAdlibOperatorChannels[operatorIndex]+0xC0,feedback);
}
/* 21CA:0A34. */
void BtechWriteAdlibAttackDecay(uint16_t operatorIndex)
{
    if (operatorIndex>=BTECH_OPL_OPERATORS) abort();
    BtechAdlibOperator *parameters=&combatAdlibOperators[operatorIndex];
    uint16_t envelope=(uint16_t)((uint16_t)(int16_t)(int8_t)parameters->attack<<4)|(parameters->decay&0x0F);
    BtechWriteAdlibRegister((int8_t)combatAdlibRegisterOffsets[operatorIndex]+0x60,envelope);
}
/* 21CA:0A7C. */
void BtechWriteAdlibSustainRelease(uint16_t operatorIndex)
{
    if (operatorIndex>=BTECH_OPL_OPERATORS) abort();
    BtechAdlibOperator *parameters=&combatAdlibOperators[operatorIndex];
    uint16_t envelope=(uint16_t)((uint16_t)(int16_t)(int8_t)parameters->sustainLevel<<4)|(parameters->release&0x0F);
    BtechWriteAdlibRegister((int8_t)combatAdlibRegisterOffsets[operatorIndex]+0x80,envelope);
}
/* 21CA:0AC4. */
void BtechWriteAdlibOperatorModeMultiplier(uint16_t operatorIndex)
{
    if (operatorIndex>=BTECH_OPL_OPERATORS) abort();
    BtechAdlibOperator *parameters=&combatAdlibOperators[operatorIndex];
    uint16_t mode=(parameters->tremoloEnabled?0x80:0)+(parameters->vibratoEnabled?0x40:0)
        +(parameters->sustainEnabled?0x20:0)+(parameters->keyScaleRateEnabled?0x10:0)+(parameters->frequencyMultiplier&0x0F);
    BtechWriteAdlibRegister((int8_t)combatAdlibRegisterOffsets[operatorIndex]+0x20,mode);
}
/* 21CA:0B50. */
void BtechWriteAdlibRhythmDepthState(void)
{
    uint16_t rhythm=(combatAdlibTremolo?0x80:0)|(combatAdlibVibrato?0x40:0)|(combatAdlibRhythm?0x20:0);
    rhythm|=(uint16_t)(int16_t)(int8_t)combatAdlibPercussionKeys;
    BtechWriteAdlibRegister(BTECH_OPL_RHYTHM_REGISTER,rhythm);
}
/* 21CA:0BA0. */
void BtechWriteAdlibOperatorWaveform(uint16_t operatorIndex)
{
    if (operatorIndex>=BTECH_OPL_OPERATORS) abort();
    BtechWriteAdlibRegister((int8_t)combatAdlibRegisterOffsets[operatorIndex]+BTECH_OPL_WAVEFORM_REGISTER,
        combatAdlibWaveforms?(combatAdlibOperators[operatorIndex].waveform&3):0);
}
/* 21CA:0BE4. Keep original note/key bytes, then transpose and signed clamp. */
void BtechSetAdlibChannelNoteAndKeyState(uint16_t channel,uint16_t note,uint16_t keyOn)
{
    if (channel>=BTECH_OPL_CHANNELS) abort();
    combatAdlibKeys[channel]=(uint8_t)keyOn;combatAdlibNotes[channel]=(uint8_t)note;
    note+=combatAdlibTransposes[channel];
    if ((int16_t)note>95) note=95;
    if ((int16_t)note<0) note=0;
    int16_t pitch=(int8_t)combatAdlibPitchClasses[note];
    if (!combatAdlibFrequencyRows[channel] || pitch<0 || pitch>=BTECH_OPL_SEMITONES) abort();
    uint16_t frequency=combatAdlibFrequencyRows[channel][pitch];
    BtechWriteAdlibRegister(channel+BTECH_OPL_FREQUENCY_LOW,frequency);
    uint16_t key=keyOn?BTECH_OPL_KEY_ON:0;
    uint16_t octave=(uint16_t)((uint16_t)(int16_t)(int8_t)combatAdlibOctaves[note]<<2);
    BtechWriteAdlibRegister(channel+BTECH_OPL_FREQUENCY_HIGH,(uint16_t)(key+octave+((frequency>>8)&3)));
}
/* 21CA:0C88. */
void BtechClearAdlibChannelFrequency(uint16_t channel)
{BtechWriteAdlibRegister(channel+BTECH_OPL_FREQUENCY_LOW,0);BtechWriteAdlibRegister(channel+BTECH_OPL_FREQUENCY_HIGH,0);}
/* 21CA:0CAE. Index and data precede all six reads; final status is the return. */
uint16_t BtechWriteAdlibRegister(uint16_t index,uint16_t value)
{
    if (!btechHostWritePort || !btechHostReadPort) abort();
    btechHostWritePort(ADLIB_INDEX_PORT,(uint8_t)index);btechHostWritePort(ADLIB_DATA_PORT,(uint8_t)value);
    for (unsigned delay=0;delay<5;++delay) btechHostReadPort(ADLIB_INDEX_PORT);
    return btechHostReadPort(ADLIB_INDEX_PORT);
}
/* 21CA:0284. Ten WORD writes intentionally clear overlapping drum masks. */
void BtechClearAdlibNoteReferenceCounts(void)
{for (unsigned index=0;index<20;++index) combatAdlibNoteReferences.bytes[index]=0;}
/* 21CA:02A8. One-based channels, reference-counted melody, channel-nine drums. */
void BtechSetAdlibMidiNote(uint16_t channel,int16_t note,uint16_t velocity)
{
    if (channel>9) return;
    --channel;
    if (channel>=BTECH_OPL_CHANNELS) abort(); /* Native channel zero aliases another owner. */
    if (!velocity)
    {
        if (combatAdlibRhythm && channel>=6)
        {
            combatAdlibPercussionKeys&=(uint8_t)~combatAdlibNoteReferences.fields.percussionMasks[channel];
            BtechWriteAdlibRhythmDepthState();return;
        }
        uint16_t count=MwReadNativeWord(combatAdlibNoteReferences.bytes,channel*2)-1;
        if ((int16_t)count<0) count=0;
        combatAdlibNoteReferences.bytes[channel*2]=(uint8_t)count;
        combatAdlibNoteReferences.bytes[channel*2+1]=(uint8_t)(count>>8);
        if (!count) BtechSetAdlibChannelNoteAndKeyState(channel,(int16_t)(int8_t)combatAdlibNotes[channel],0);
        return;
    }
    if (note<0) return;
    if (channel<6 || !combatAdlibRhythm)
    {
        note=(int16_t)(uint16_t)(note-12);
        BtechSetAdlibChannelNoteAndKeyState(channel,(int16_t)(int8_t)combatAdlibNotes[channel],0);
        BtechSetAdlibChannelNoteAndKeyState(channel,(uint16_t)note,1);
        uint16_t count=MwReadNativeWord(combatAdlibNoteReferences.bytes,channel*2)+1;
        combatAdlibNoteReferences.bytes[channel*2]=(uint8_t)count;
        combatAdlibNoteReferences.bytes[channel*2+1]=(uint8_t)(count>>8);return;
    }
    if (channel!=8) return;
    unsigned percussion=note>40 && note<51?8:0;
    switch (note) {case 36:percussion=6;break;case 38:percussion=7;break;case 42:percussion=10;break;case 49:percussion=9;break;}
    if (percussion==6) BtechSetAdlibChannelNoteAndKeyState(6,14,0);
    if (percussion==8)
    {BtechSetAdlibChannelNoteAndKeyState(8,24,0);BtechSetAdlibChannelNoteAndKeyState(7,38,0);}
    if (!percussion) return;
    combatAdlibPercussionKeys|=combatAdlibNoteReferences.fields.percussionMasks[percussion];
    BtechWriteAdlibRhythmDepthState();
}
/* 229C:0000. Non-note messages preserve AX; note callbacks return velocity. */
uint16_t BtechDispatchAdlibMidiMessage(uint16_t statusDisplacement,uint16_t channel,uint16_t packedData)
{
    if (!statusDisplacement) packedData&=0x00FF;
    else if (statusDisplacement!=2) return packedData;
    BtechSetAdlibMidiNote(channel,(uint8_t)packedData,packedData>>8);
    return packedData>>8;
}

/* 21CA:014C. Both waveform WORDs are captured before any device calls.
 * The second operator mapping remains live after the first operator's writes. */
void BtechLoadAdlibChannelInstrument(uint16_t channel,const BtechAdlibWordInstrument *instrument)
{
    uint16_t firstWaveform=instrument->modulatorWaveform,secondWaveform=instrument->carrierWaveform;
    if (!combatAdlibRhythm || (int16_t)channel<BTECH_OPL_FIRST_RHYTHM_CHANNEL)
    {
        uint16_t mapping=(uint16_t)(channel*2);
        if (mapping>=sizeof(combatAdlibOperatorMapping)-1) abort();
        BtechStoreAdlibInstrumentParameters((uint16_t)(int16_t)(int8_t)combatAdlibOperatorMapping[mapping],instrument->modulator,firstWaveform);
        BtechStoreAdlibInstrumentParameters((uint16_t)(int16_t)(int8_t)combatAdlibOperatorMapping[mapping+1],instrument->carrier,secondWaveform);
    }
    else if (channel==BTECH_OPL_FIRST_RHYTHM_CHANNEL)
    {
        BtechStoreAdlibInstrumentParameters((uint16_t)(int16_t)(int8_t)combatAdlibOperatorMapping[BTECH_OPL_BASS_DRUM_MAPPING],instrument->modulator,firstWaveform);
        BtechStoreAdlibInstrumentParameters((uint16_t)(int16_t)(int8_t)combatAdlibOperatorMapping[BTECH_OPL_BASS_DRUM_MAPPING+1],instrument->carrier,secondWaveform);
    }
    else
    {
        uint16_t mapping=(uint16_t)(BTECH_OPL_RHYTHM_MAPPING_START+channel*2);
        if (mapping>=sizeof(combatAdlibOperatorMapping)) abort();
        BtechStoreAdlibInstrumentParameters((uint16_t)(int16_t)(int8_t)combatAdlibOperatorMapping[mapping],instrument->modulator,firstWaveform);
    }
}
/* 21CA:01E6. Melodic and bass-drum volume uses the carrier; other drums use
 * their single operator. The unsigned input clamps before the BYTE store. */
void BtechSetAdlibChannelOperatorLevel(uint16_t channel,uint16_t level)
{
    uint16_t mapping;
    if (!combatAdlibRhythm || (int16_t)channel<BTECH_OPL_FIRST_RHYTHM_CHANNEL) mapping=(uint16_t)(1+channel*2);
    else mapping=(uint16_t)(BTECH_OPL_RHYTHM_MAPPING_START+channel*2+(channel==BTECH_OPL_FIRST_RHYTHM_CHANNEL?1:0));
    if (mapping>=sizeof(combatAdlibOperatorMapping)) abort();
    uint16_t operatorIndex=(uint16_t)(int16_t)(int8_t)combatAdlibOperatorMapping[mapping];
    if (operatorIndex>=BTECH_OPL_OPERATORS) abort();
    if (level>BTECH_OPL_LEVEL_MAXIMUM) level=BTECH_OPL_LEVEL_MAXIMUM;
    combatAdlibLevels[operatorIndex]=(uint8_t)level;BtechWriteAdlibOperatorLevel(operatorIndex);
}
/* 21CA:0244. Rhythm skips unsigned channels above six. Key is read before
 * note, after bend/cache publication, and both BYTEs are sign-extended. */
void BtechSetAdlibChannelPitchBend(uint16_t channel,uint16_t bend)
{
    if (combatAdlibRhythm && channel>BTECH_OPL_FIRST_RHYTHM_CHANNEL) return;
    if (bend>BTECH_OPL_BEND_MAXIMUM) bend=BTECH_OPL_BEND_MAXIMUM;
    BtechUpdateAdlibPitchBend(channel,bend);
    if (channel>=BTECH_OPL_CHANNELS) abort();
    uint16_t key=(uint16_t)(int16_t)(int8_t)combatAdlibKeys[channel];
    uint16_t note=(uint16_t)(int16_t)(int8_t)combatAdlibNotes[channel];
    BtechSetAdlibChannelNoteAndKeyState(channel,note,key);
}
/* 21CA:0406. This API deliberately does NOT use the note-on indexing contract:
 * melody reads note[channel] but writes channel-1. Counts are untouched. */
void BtechStopAdlibMidiChannel(uint16_t channel)
{
    if (!combatAdlibRhythm || channel<BTECH_OPL_FIRST_RHYTHM_CHANNEL)
    {
        if (channel>=BTECH_OPL_CHANNELS) abort();
        uint16_t note=(uint16_t)(int16_t)(int8_t)combatAdlibNotes[channel];
        BtechSetAdlibChannelNoteAndKeyState((uint16_t)(channel-1),note,0);
    }
    else
    {
        if (channel>=sizeof(combatAdlibNoteReferences.fields.percussionMasks)) abort();
        combatAdlibPercussionKeys&=(uint8_t)~combatAdlibNoteReferences.fields.percussionMasks[channel];
        BtechWriteAdlibRhythmDepthState();
    }
}
/* 21CA:06F6. The cache key is the signed scaled bend, not the channel. Keep
 * WORD truncation before signed quotient/remainder and all publication order. */
void BtechUpdateAdlibPitchBend(uint16_t channel,uint16_t bend)
{
    int16_t centered=(int16_t)(uint16_t)(bend-BTECH_OPL_BEND_CENTER);
    int32_t scaled=(int32_t)centered*(int16_t)combatAdlibBendScale;
    if (channel>=BTECH_OPL_TUNING_CHANNELS) abort();
    if (scaled==combatAdlibCachedScaledBend)
    {
        combatAdlibFrequencyRows[channel]=combatAdlibCachedFrequencyRow;
        combatAdlibTransposes[channel]=combatAdlibCachedTranspose;return;
    }
    int16_t steps=(int16_t)(uint16_t)(scaled/BTECH_OPL_BEND_CENTER);
    uint16_t transpose,remainder;
    if (steps<0)
    {
        int16_t inverted=(int16_t)(uint16_t)(24-(uint16_t)steps);
        transpose=(uint16_t)-(inverted/BTECH_OPL_BEND_STEPS_PER_NOTE);
        int16_t magnitude=(int16_t)(uint16_t)((uint16_t)inverted-24);
        remainder=(uint16_t)(magnitude%BTECH_OPL_BEND_STEPS_PER_NOTE);
        if (remainder) remainder=(uint16_t)(BTECH_OPL_BEND_STEPS_PER_NOTE-remainder);
    }
    else
    {
        transpose=(uint16_t)(steps/BTECH_OPL_BEND_STEPS_PER_NOTE);
        remainder=(uint16_t)(steps%BTECH_OPL_BEND_STEPS_PER_NOTE);
    }
    combatAdlibTransposes[channel]=transpose;combatAdlibCachedTranspose=transpose;
    if (remainder>=BTECH_OPL_FREQUENCY_ROWS) abort(); /* Unrepresented native row alias, after transpose stores. */
    combatAdlibFrequencyRows[channel]=combatAdlibFrequencies[remainder];
    combatAdlibCachedFrequencyRow=combatAdlibFrequencies[remainder];combatAdlibCachedScaledBend=scaled;
}
