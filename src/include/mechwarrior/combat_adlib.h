#ifndef MECHWARRIOR_COMBAT_ADLIB_H
#define MECHWARRIOR_COMBAT_ADLIB_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum { BTECH_OPL_OPERATORS=18,BTECH_OPL_CHANNELS=9,BTECH_OPL_TUNING_CHANNELS=11,
    BTECH_OPL_NOTES=96,BTECH_OPL_SEMITONES=12,BTECH_OPL_FREQUENCY_ROWS=25,
    BTECH_OPL_PARAMETER_BYTES=13,BTECH_OPL_DEFAULT_INSTRUMENTS=8,
    BTECH_OPL_KEY_ON=0x20,BTECH_OPL_WAVEFORM_ENABLE=0x20,
    BTECH_OPL_FREQUENCY_LOW=0xA0,BTECH_OPL_FREQUENCY_HIGH=0xB0,
    BTECH_OPL_RHYTHM_REGISTER=0xBD,BTECH_OPL_WAVEFORM_REGISTER=0xE0,
    BTECH_OPL_BEND_CENTER=0x2000,BTECH_OPL_BEND_MAXIMUM=0x3FFF,
    BTECH_OPL_LEVEL_MAXIMUM=0x7F,BTECH_OPL_BEND_STEPS_PER_NOTE=25,
    BTECH_OPL_FIRST_RHYTHM_CHANNEL=6,BTECH_OPL_BASS_DRUM_MAPPING=18,
    BTECH_OPL_RHYTHM_MAPPING_START=6 };
typedef struct BtechAdlibWordInstrument {
    int16_t modulator[BTECH_OPL_PARAMETER_BYTES],carrier[BTECH_OPL_PARAMETER_BYTES];
    uint16_t modulatorWaveform,carrierWaveform;
} BtechAdlibWordInstrument;
typedef struct BtechAdlibOperator {
    uint8_t keyScaleLevel,frequencyMultiplier,feedback,attack,sustainLevel;
    uint8_t sustainEnabled,decay,release,attenuation,tremoloEnabled;
    uint8_t vibratoEnabled,keyScaleRateEnabled,fmConnectionEnabled,waveform;
} BtechAdlibOperator;
/* The last three note-reference WORDs overlap the percussion mask table.
 * Keep one owner, including the five mask bytes beyond those WORDs. */
typedef union BtechAdlibNoteReferences {
    struct {uint16_t melodic[7];uint8_t percussionMasks[11];} fields;
    uint8_t bytes[26];
} BtechAdlibNoteReferences;
extern BtechAdlibNoteReferences combatAdlibNoteReferences;
extern uint8_t combatAdlibDefaultInstruments[BTECH_OPL_DEFAULT_INSTRUMENTS][14];
extern uint8_t combatAdlibOperatorMapping[28],combatAdlibRegisterOffsets[BTECH_OPL_OPERATORS];
extern uint8_t combatAdlibCarrierFlags[BTECH_OPL_OPERATORS],combatAdlibOperatorChannels[BTECH_OPL_OPERATORS];
extern uint8_t combatAdlibTremolo,combatAdlibVibrato,combatAdlibRhythm,combatAdlibNoteSelect;
extern uint8_t combatAdlibPercussionKeys,combatAdlibNotes[BTECH_OPL_CHANNELS],combatAdlibKeys[BTECH_OPL_CHANNELS];
extern uint8_t combatAdlibLevels[BTECH_OPL_OPERATORS],combatAdlibPitchClasses[BTECH_OPL_NOTES],combatAdlibOctaves[BTECH_OPL_NOTES];
extern uint16_t combatAdlibTransposes[BTECH_OPL_TUNING_CHANNELS],combatAdlibWaveforms;
extern uint16_t combatAdlibFrequencyStep,combatAdlibBendScale;
extern uint16_t combatAdlibFrequencies[BTECH_OPL_FREQUENCY_ROWS][BTECH_OPL_SEMITONES];
extern uint16_t *combatAdlibFrequencyRows[BTECH_OPL_TUNING_CHANNELS];
extern BtechAdlibOperator combatAdlibOperators[BTECH_OPL_OPERATORS];
extern int32_t combatAdlibCachedScaledBend;
extern uint16_t combatAdlibCachedTranspose;
extern uint16_t *combatAdlibCachedFrequencyRow;
bool BtechImportAdlibInitialData(const uint8_t *data,size_t bytes);
uint16_t BtechInitializeAdlibMusicBackend(void);
void BtechSetAdlibRhythmMode(uint16_t enabled);
uint16_t BtechSetAdlibWaveformSelection(uint16_t enabled);
void BtechSetAdlibFrequencyStep(uint16_t step);
void BtechSetAdlibGlobalDepthControls(uint8_t tremolo,uint8_t vibrato,uint8_t noteSelect);
uint32_t BtechCalculateAdlibRowBaseFrequency(uint16_t step,int16_t scale);
void BtechBuildAdlibSemitoneRow(uint16_t *row,uint16_t step,int16_t scale);
void BtechBuildAdlibFrequencyTables(void);
void BtechResetAdlibOperatorLevels(void);
void BtechLoadDefaultAdlibInstruments(void);
void BtechLoadSignedAdlibInstrumentBytes(uint16_t operatorIndex,const uint8_t *parameters,uint16_t waveform);
void BtechStoreAdlibInstrumentParameters(uint16_t operatorIndex,const int16_t *parameters,uint16_t waveform);
void BtechApplyAdlibInstrumentRegisters(uint16_t operatorIndex);
void BtechWriteAdlibOperatorLevel(uint16_t operatorIndex);
void BtechWriteAdlibNoteSelect(void);
void BtechWriteAdlibFeedbackAndConnection(uint16_t operatorIndex);
void BtechWriteAdlibAttackDecay(uint16_t operatorIndex);
void BtechWriteAdlibSustainRelease(uint16_t operatorIndex);
void BtechWriteAdlibOperatorModeMultiplier(uint16_t operatorIndex);
void BtechWriteAdlibRhythmDepthState(void);
void BtechWriteAdlibOperatorWaveform(uint16_t operatorIndex);
void BtechSetAdlibChannelNoteAndKeyState(uint16_t channel,uint16_t note,uint16_t keyOn);
void BtechClearAdlibChannelFrequency(uint16_t channel);
uint16_t BtechWriteAdlibRegister(uint16_t index,uint16_t value);
void BtechSetAdlibMidiNote(uint16_t channel,int16_t note,uint16_t velocity);
void BtechClearAdlibNoteReferenceCounts(void);
void BtechLoadAdlibChannelInstrument(uint16_t channel,const BtechAdlibWordInstrument *instrument);
void BtechSetAdlibChannelOperatorLevel(uint16_t channel,uint16_t level);
void BtechSetAdlibChannelPitchBend(uint16_t channel,uint16_t bend);
void BtechUpdateAdlibPitchBend(uint16_t channel,uint16_t bend);
void BtechStopAdlibMidiChannel(uint16_t channel);
uint16_t BtechDispatchAdlibMidiMessage(uint16_t statusDisplacement,uint16_t channel,uint16_t packedData);
#endif
