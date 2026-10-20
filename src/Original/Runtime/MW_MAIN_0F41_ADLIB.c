#include "mechwarrior/campaign_music.h"
#include "mechwarrior/combat_sound.h"
#include <stdlib.h>

enum { OPL_TIMER_ONE=2,OPL_TIMER_CONTROL=4,OPL_TIMER_STATUS_MASK=0xE0,
    OPL_TIMER_ONE_OVERFLOW=0xC0,OPL_WAVEFORM_ENABLE=0x2001,
    OPL_NOTE_SELECT=0x0008,OPL_RHYTHM_REGISTER=0xBD,OPL_RHYTHM_INITIAL=0xE0,
    OPL_MELODIC_VOICE=0x20,OPL_BASS_DRUM_VOICE=0x10,OPL_KEY_ON=0x2000,
    OPL_KEY_REGISTER=0xB0,OPL_REGISTER_BANK_STEP=0x20,OPL_WAVEFORM_STEP=0x60,
    OPL_CARRIER_REGISTER_WRAP=0x43,OPL_CHANNEL_REGISTER_STEP=0x10,OPL_FREQUENCY_REGISTER_WRAP=0xE0,
    OPL_MASK_TIMERS=0x6004,OPL_RESET_TIMERS=0x8004,OPL_CALIBRATION_LOAD=0xC002,
    OPL_CALIBRATION_START=0x21,OPL_CALIBRATION_ADDRESS_READS=20,
    OPL_ADDRESS_CALIBRATION_SCALE=0x060F,OPL_DATA_CALIBRATION_SCALE=0x00E4 };

/* MW_MAIN0F41:069A. Calibrated reads, including native zero-count LOOP wrap. */
void MwWriteAdlibRegisterPair(uint16_t registerAndValue,uint16_t port)
{
    midiAdlibOutputPort=port;
    mwMusicHardware.writePort(port,(uint8_t)registerAndValue);
    uint16_t remaining=adlibAddressDelayReads;
    do {mwMusicHardware.readPort(port);} while (--remaining);
    mwMusicHardware.writePort((uint16_t)(port+1),(uint8_t)(registerAndValue>>8));
    remaining=adlibDataDelayReads;
    do {mwMusicHardware.readPort(port);} while (--remaining);
}
/* MW_MAIN0F41:06B9. Timer overflow calibrates the original bus-read delays. */
uint16_t MwProbeAdlibAndCalibrateDelays(void)
{
    MwWriteAdlibRegisterPair(OPL_MASK_TIMERS,ADLIB_INDEX_PORT);
    MwWriteAdlibRegisterPair(OPL_RESET_TIMERS,ADLIB_INDEX_PORT);
    if (mwMusicHardware.readPort(ADLIB_INDEX_PORT)&OPL_TIMER_STATUS_MASK) return 0;
    MwWriteAdlibRegisterPair(OPL_CALIBRATION_LOAD,ADLIB_INDEX_PORT);
    mwMusicHardware.writePort(ADLIB_INDEX_PORT,OPL_TIMER_CONTROL);
    for (uint16_t reads=OPL_CALIBRATION_ADDRESS_READS;reads;--reads) mwMusicHardware.readPort(ADLIB_INDEX_PORT);
    mwMusicHardware.setInterruptsEnabled(false);
    mwMusicHardware.writePort(ADLIB_DATA_PORT,OPL_CALIBRATION_START);
    uint16_t remaining=0;uint8_t status;
    do {status=mwMusicHardware.readPort(ADLIB_INDEX_PORT)&OPL_TIMER_STATUS_MASK;--remaining;}
    while (remaining && status!=OPL_TIMER_ONE_OVERFLOW);
    mwMusicHardware.setInterruptsEnabled(true);
    if (status!=OPL_TIMER_ONE_OVERFLOW) return 0;
    uint16_t measuredReads=0-remaining;
    adlibDataDelayReads=measuredReads/OPL_DATA_CALIBRATION_SCALE+1;
    adlibAddressDelayReads=measuredReads/OPL_ADDRESS_CALIBRATION_SCALE+1;
    return 1;
}
/* MW_MAIN0F41:0712. Rhythm mode and waveform selection are game-owned. */
void MwInitializeAdlibBackend(void)
{
    MwWriteAdlibRegisterPair(OPL_WAVEFORM_ENABLE,ADLIB_INDEX_PORT);
    MwWriteAdlibRegisterPair(OPL_NOTE_SELECT,ADLIB_INDEX_PORT);
    MwWriteAdlibRegisterPair(0x00B7,ADLIB_INDEX_PORT);
    MwWriteAdlibRegisterPair(0x00B8,ADLIB_INDEX_PORT);
    MwWriteAdlibRegisterPair((OPL_RHYTHM_INITIAL<<8)|OPL_RHYTHM_REGISTER,ADLIB_INDEX_PORT);
    adlibRhythmRegister=OPL_RHYTHM_INITIAL;
}
/* MW_MAIN0F41:073A. The first register write clears native CX: both later
 * velocity/attenuation calculations therefore use zero. Preserve that quirk. */
void MwStartAdlibVoice(MwMidiVoice *voice,MwMidiChannel *channel,uint16_t velocity)
{
    (void)velocity;
    if (voice->translatedNote>=MIDI_ADLIB_NOTE_COUNT) {MwStopAdlibVoice(voice);return;}
    MwAdlibVoice *opl=&voice->adlib;
    opl->packedFrequency=adlibPackedNoteFrequencies[voice->translatedNote];
    opl->instrumentOffset=channel->instrumentOffset;
    uint16_t relative=opl->instrumentOffset-MIDI_INSTRUMENTS_OFFSET;
    if (relative%sizeof(MwAdlibInstrument) || relative/sizeof(MwAdlibInstrument)>=MIDI_INSTRUMENT_COUNT) abort();
    const MwAdlibInstrument *instrument=&adlibInstruments[relative/sizeof(MwAdlibInstrument)];
    uint8_t reg=opl->operatorIndex;
    reg+=OPL_REGISTER_BANK_STEP;MwWriteAdlibRegisterPair(((uint16_t)instrument->modulatorFlags<<8)|reg,ADLIB_INDEX_PORT);
    if (voice->backendMask==OPL_MELODIC_VOICE || voice->backendMask==OPL_BASS_DRUM_VOICE)
    {
        reg+=OPL_REGISTER_BANK_STEP;MwWriteAdlibRegisterPair(((uint16_t)instrument->modulatorLevel<<8)|reg,ADLIB_INDEX_PORT);
        reg+=OPL_REGISTER_BANK_STEP;MwWriteAdlibRegisterPair(((uint16_t)instrument->modulatorAttackDecay<<8)|reg,ADLIB_INDEX_PORT);
        reg+=OPL_REGISTER_BANK_STEP;MwWriteAdlibRegisterPair(((uint16_t)instrument->modulatorSustainRelease<<8)|reg,ADLIB_INDEX_PORT);
        reg+=OPL_WAVEFORM_STEP;MwWriteAdlibRegisterPair(((uint16_t)instrument->modulatorWaveform<<8)|reg,ADLIB_INDEX_PORT);
        reg+=OPL_CARRIER_REGISTER_WRAP;MwWriteAdlibRegisterPair(((uint16_t)instrument->carrierFlags<<8)|reg,ADLIB_INDEX_PORT);
        opl->carrierLevel=instrument->carrierLevel&0xC0;
        reg+=OPL_REGISTER_BANK_STEP;MwWriteAdlibRegisterPair(((uint16_t)opl->carrierLevel<<8)|reg,ADLIB_INDEX_PORT);
        reg+=OPL_REGISTER_BANK_STEP;MwWriteAdlibRegisterPair(((uint16_t)instrument->carrierAttackDecay<<8)|reg,ADLIB_INDEX_PORT);
        reg+=OPL_REGISTER_BANK_STEP;MwWriteAdlibRegisterPair(((uint16_t)instrument->carrierSustainRelease<<8)|reg,ADLIB_INDEX_PORT);
        reg+=OPL_WAVEFORM_STEP;MwWriteAdlibRegisterPair(((uint16_t)instrument->carrierWaveform<<8)|reg,ADLIB_INDEX_PORT);
        reg=OPL_KEY_REGISTER+opl->channelIndex;MwWriteAdlibRegisterPair(reg,ADLIB_INDEX_PORT);
        reg+=OPL_CHANNEL_REGISTER_STEP;MwWriteAdlibRegisterPair(((uint16_t)instrument->feedbackConnection<<8)|reg,ADLIB_INDEX_PORT);
        uint16_t keyedFrequency=opl->packedFrequency|OPL_KEY_ON;
        reg+=OPL_FREQUENCY_REGISTER_WRAP;MwWriteAdlibRegisterPair(((uint16_t)(uint8_t)keyedFrequency<<8)|reg,ADLIB_INDEX_PORT);
        if (voice->backendMask!=OPL_BASS_DRUM_VOICE)
        {
            reg+=OPL_CHANNEL_REGISTER_STEP;MwWriteAdlibRegisterPair((keyedFrequency&0xFF00)|reg,ADLIB_INDEX_PORT);return;
        }
    }
    else
    {
        reg+=OPL_REGISTER_BANK_STEP;MwWriteAdlibRegisterPair(reg,ADLIB_INDEX_PORT);
        reg+=OPL_REGISTER_BANK_STEP;MwWriteAdlibRegisterPair(((uint16_t)instrument->modulatorAttackDecay<<8)|reg,ADLIB_INDEX_PORT);
        reg+=OPL_REGISTER_BANK_STEP;MwWriteAdlibRegisterPair(((uint16_t)instrument->modulatorSustainRelease<<8)|reg,ADLIB_INDEX_PORT);
        reg+=OPL_WAVEFORM_STEP;MwWriteAdlibRegisterPair(((uint16_t)instrument->modulatorWaveform<<8)|reg,ADLIB_INDEX_PORT);
    }
    uint8_t rhythm=(uint8_t)~voice->backendMask&adlibRhythmRegister;
    MwWriteAdlibRegisterPair(((uint16_t)rhythm<<8)|OPL_RHYTHM_REGISTER,ADLIB_INDEX_PORT);
    rhythm|=voice->backendMask;
    MwWriteAdlibRegisterPair(((uint16_t)rhythm<<8)|OPL_RHYTHM_REGISTER,ADLIB_INDEX_PORT);
    adlibRhythmRegister=rhythm;
}
/* MW_MAIN0F41:0841. Rhythm release inherits DX; melodic release selects OPL. */
void MwStopAdlibVoice(MwMidiVoice *voice)
{
    if (voice->backendMask==OPL_MELODIC_VOICE)
        MwWriteAdlibRegisterPair((voice->adlib.packedFrequency&0xFF00)|(uint8_t)(OPL_KEY_REGISTER+voice->adlib.channelIndex),ADLIB_INDEX_PORT);
    else
    {
        uint8_t rhythm=(uint8_t)~voice->backendMask&adlibRhythmRegister;
        MwWriteAdlibRegisterPair(((uint16_t)rhythm<<8)|OPL_RHYTHM_REGISTER,midiAdlibOutputPort);
        adlibRhythmRegister=rhythm;
    }
}
/* MW_MAIN0F41:0869. Authored route triples assign chip channels/operators. */
void MwInitializeAdlibVoiceRouting(void)
{
    for (uint16_t index=0;index<adlibBackend.selectedVoiceCount;++index)
    {
        if (index>=MIDI_VOICE_CAPACITY) abort();
        MwMidiVoice *voice=MwResolveMidiVoice(adlibBackend.voicePoolOffset+index*sizeof(MwMidiVoice));
        voice->adlib.channelIndex=adlibVoiceRoutes[index][0];
        voice->adlib.operatorIndex=adlibVoiceRoutes[index][1];
        voice->backendMask=adlibVoiceRoutes[index][2];
    }
}
/* MW_MAIN0F41:0890. Eleven ordered packed-byte reads; padding stays untouched. */
void MwDecodeAdlibInstrument(const uint8_t *source,MwAdlibInstrument *instrument)
{
    instrument->modulatorFlags=*source++;
    uint8_t packed=*source++;
    uint8_t attackDecay=(packed&0x78)<<1;
    instrument->modulatorWaveform=packed&3;
    packed=*source++;attackDecay|=packed&0x0F;instrument->modulatorLevel=(packed&0x60)<<1;
    packed=*source++;uint8_t sustainRelease=(packed&0x78)<<1;
    instrument->feedbackConnection=(packed&7)<<1;
    packed=*source++;sustainRelease|=packed&0x0F;instrument->modulatorFlags|=(packed&0x40)<<1;
    instrument->modulatorSustainRelease=sustainRelease;instrument->modulatorAttackDecay=attackDecay;
    packed=*source++;instrument->modulatorLevel|=packed&0x3F;
    instrument->carrierFlags=*source++;
    packed=*source++;attackDecay=(packed&0x78)<<1;instrument->carrierWaveform=packed&3;
    packed=*source++;attackDecay|=packed&0x0F;instrument->carrierLevel=(packed&0x60)<<1;
    packed=*source++;sustainRelease=(packed&0x78)<<1;
    packed=*source++;sustainRelease|=packed&0x0F;instrument->carrierFlags|=(packed&0x40)<<1;
    instrument->carrierSustainRelease=sustainRelease;instrument->carrierAttackDecay=attackDecay;
}
