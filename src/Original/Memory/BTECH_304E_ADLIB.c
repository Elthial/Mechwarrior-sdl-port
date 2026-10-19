#include "mechwarrior/combat_adlib.h"
#include "mechwarrior/native_address.h"
#include <string.h>

BtechAdlibNoteReferences combatAdlibNoteReferences;
uint8_t combatAdlibDefaultInstruments[BTECH_OPL_DEFAULT_INSTRUMENTS][14];
uint8_t combatAdlibOperatorMapping[28],combatAdlibRegisterOffsets[BTECH_OPL_OPERATORS];
uint8_t combatAdlibCarrierFlags[BTECH_OPL_OPERATORS],combatAdlibOperatorChannels[BTECH_OPL_OPERATORS];
uint8_t combatAdlibTremolo,combatAdlibVibrato,combatAdlibRhythm,combatAdlibNoteSelect;
uint8_t combatAdlibPercussionKeys,combatAdlibNotes[BTECH_OPL_CHANNELS],combatAdlibKeys[BTECH_OPL_CHANNELS];
uint8_t combatAdlibLevels[BTECH_OPL_OPERATORS],combatAdlibPitchClasses[BTECH_OPL_NOTES],combatAdlibOctaves[BTECH_OPL_NOTES];
uint16_t combatAdlibTransposes[BTECH_OPL_TUNING_CHANNELS],combatAdlibWaveforms;
uint16_t combatAdlibFrequencyStep,combatAdlibBendScale;
uint16_t combatAdlibFrequencies[BTECH_OPL_FREQUENCY_ROWS][BTECH_OPL_SEMITONES];
uint16_t *combatAdlibFrequencyRows[BTECH_OPL_TUNING_CHANNELS];
BtechAdlibOperator combatAdlibOperators[BTECH_OPL_OPERATORS];
int32_t combatAdlibCachedScaledBend;
uint16_t combatAdlibCachedTranspose;
uint16_t *combatAdlibCachedFrequencyRow;
_Static_assert(sizeof(BtechAdlibOperator)==14,"Native OPL operator parameters");
_Static_assert(sizeof(BtechAdlibWordInstrument)==0x38,"Native two-operator WORD instrument");
_Static_assert(offsetof(BtechAdlibWordInstrument,carrier)==0x1A,"Native carrier parameter offset");
_Static_assert(offsetof(BtechAdlibNoteReferences,fields.percussionMasks)==14,"Native count/mask overlap");

bool BtechImportAdlibInitialData(const uint8_t *data,size_t bytes)
{
    if (!data || bytes<0x10000) return false;
    memcpy(combatAdlibNoteReferences.bytes,data+0x2B86,25);
    memcpy(combatAdlibDefaultInstruments,data+0x2BA0,sizeof(combatAdlibDefaultInstruments));
    memcpy(combatAdlibOperatorMapping,data+0x2C10,sizeof(combatAdlibOperatorMapping));
    memcpy(combatAdlibRegisterOffsets,data+0x2C2C,sizeof(combatAdlibRegisterOffsets));
    memcpy(combatAdlibCarrierFlags,data+0x2C3E,sizeof(combatAdlibCarrierFlags));
    memcpy(combatAdlibOperatorChannels,data+0x2C50,sizeof(combatAdlibOperatorChannels));
    combatAdlibCachedScaledBend=(int32_t)((uint32_t)MwReadNativeWord(data,0x2C62)
        |((uint32_t)MwReadNativeWord(data,0x2C64)<<16));
    combatAdlibCachedTranspose=MwReadNativeWord(data,0x73F2);
    uint16_t cachedRow=MwReadNativeWord(data,0x73F0);
    combatAdlibCachedFrequencyRow=NULL;
    if (cachedRow>=0x7178 && cachedRow<0x73D0 && (cachedRow-0x7178)%24==0)
        combatAdlibCachedFrequencyRow=combatAdlibFrequencies[(cachedRow-0x7178)/24];
    combatAdlibTremolo=data[0x6F78];combatAdlibRhythm=data[0x6F79];
    combatAdlibNoteSelect=data[0x6F90];combatAdlibBendScale=MwReadNativeWord(data,0x6F92);
    combatAdlibWaveforms=MwReadNativeWord(data,0x6FF4);combatAdlibVibrato=data[0x7056];
    combatAdlibPercussionKeys=data[0x706E];combatAdlibFrequencyStep=MwReadNativeWord(data,0x73D0);
    memcpy(combatAdlibNotes,data+0x7070,sizeof(combatAdlibNotes));
    memcpy(combatAdlibKeys,data+0x73D2,sizeof(combatAdlibKeys));
    memcpy(combatAdlibOperators,data+0x707C,sizeof(combatAdlibOperators));
    memcpy(combatAdlibLevels,data+0x73DE,sizeof(combatAdlibLevels));
    memcpy(combatAdlibPitchClasses,data+0x6F94,sizeof(combatAdlibPitchClasses));
    memcpy(combatAdlibOctaves,data+0x6FF6,sizeof(combatAdlibOctaves));
    for (unsigned row=0;row<BTECH_OPL_FREQUENCY_ROWS;++row)
        for (unsigned note=0;note<BTECH_OPL_SEMITONES;++note)
            combatAdlibFrequencies[row][note]=MwReadNativeWord(data,0x7178+row*24+note*2);
    for (unsigned channel=0;channel<BTECH_OPL_TUNING_CHANNELS;++channel)
    {
        combatAdlibTransposes[channel]=MwReadNativeWord(data,0x6F7A+channel*2);
        /* Fresh BSS row offsets are not dereferenced before table construction. */
        uint16_t offset=MwReadNativeWord(data,0x7058+channel*2);
        combatAdlibFrequencyRows[channel]=NULL;
        if (offset>=0x7178 && offset<0x73D0 && (offset-0x7178)%24==0)
            combatAdlibFrequencyRows[channel]=combatAdlibFrequencies[(offset-0x7178)/24];
    }
    return true;
}
