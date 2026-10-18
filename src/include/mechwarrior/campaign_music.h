#ifndef MECHWARRIOR_CAMPAIGN_MUSIC_H
#define MECHWARRIOR_CAMPAIGN_MUSIC_H
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "mechwarrior/hardware_ports.h"

enum { MIDI_TRACK_ACTIVE=0x0001, MIDI_TRACK_PAUSED=0x0002,
    MIDI_TRACK_MUTED=0x0004, MIDI_TRACK_SKIP_METRICS=0x0008,
    MIDI_CHANNEL_COUNT=16, MIDI_TRACK_CHANNEL_HANDLERS=7,
    MIDI_TRACK_SYSTEM_HANDLERS=16, MIDI_TICKS_PER_QUARTER=96,
    MIDI_CONTROL_ALL_NOTES_OFF=0x7B, MIDI_CONTROL_CHANGE=0xB0,
    MIDI_STATUS_BYTE=0x80, MIDI_SYSTEM_STATUS=0xF0,
    MIDI_REALTIME_STATUS=0xF8, MIDI_META_END_TRACK=0x2F,
    MIDI_META_TEMPO=0x51, MIDI_META_TIME_SIGNATURE=0x58 };
/* Writable native NEAR-JMP destinations, not invented MIDI event IDs. */
enum { MIDI_FORWARD_SYSEX_STATUS=0x045F, MIDI_FORWARD_SYSEX_PAYLOAD=0x046A,
    MIDI_META_EVENT=0x0486, MIDI_READ_NEXT_DELTA=0x0502,
    MIDI_MUTED_NOTE_OR_TWO_DATA=0x0538, MIDI_FORWARD_STATUS_TWO_DATA=0x0545,
    MIDI_FORWARD_STATUS_ONE_DATA=0x0556, MIDI_FORWARD_CURRENT_BYTE=0x0567 };

typedef void (*MwMusicTimerHandler)(void);
enum { MUSIC_TIMER_CALLBACK_COUNT=9, MUSIC_TIMER_FAST_SLOT=8,
    MUSIC_TIMER_PHASE_MASK=0x1F, MUSIC_TIMER_COARSE_MASK=0xE0,
    MUSIC_TIMER_BASE_DIVISOR_HIGH=0x20, MUSIC_TIMER_MAX_RATE_SHIFT=5,
    MUSIC_TIMER_CLOCK_INCREMENT=0x01C2,
    PIT_CHANNEL0_PORT=0x0040, PIT_CHANNEL2_PORT=0x0042,
    PIT_CHANNEL0_SQUARE_WAVE=0x36, PIT_CHANNEL2_SQUARE_WAVE=0xB6,
    SPEAKER_CONTROL_PORT=0x0061, SPEAKER_GATE_BITS=0x03 };
typedef struct MwMusicHardwareCalls
{
    void (*setInterruptsEnabled)(bool enabled);
    MwMusicTimerHandler (*readIrq0Handler)(void);
    void (*writeIrq0Handler)(MwMusicTimerHandler handler);
    void (*writePort)(uint16_t port,uint8_t value);
    uint8_t (*readPort)(uint16_t port);
    void (*waitForInterrupt)(void);
    uint32_t (*readInterruptVector)(uint8_t interrupt);
    void (*writeInterruptVector)(uint8_t interrupt,uint32_t vector);
} MwMusicHardwareCalls;
extern MwMusicHardwareCalls mwMusicHardware;
extern uint16_t timerClockLow,timerClockHigh;
extern MwMusicTimerHandler volatile timerCallbackSlots[MUSIC_TIMER_CALLBACK_COUNT];
extern uint8_t pitDivisorHigh,timerPhase;
void MwCampaignTimerInterrupt(void);
void MwInstallMusicTimer(void);
void MwRestoreBiosTimer(void);
/* Genuine callers discard AX. Invalid-slot incoming AX is not a C argument;
 * unused0118/0190 register-query APIs are not converted by this projection. */
void MwTimerSetCallback(uint16_t slot,MwMusicTimerHandler handler,uint16_t rateShift);

/* Native runtime DS28BE ownership. No live segment mirror. */
extern uint16_t midiTickDenominator,midiTickNumerator,midiTickRemainder;
extern uint16_t midiBeatDivision,midiBarDivision;
extern uint16_t midiStreamOffset,midiStreamSegment;
extern uint16_t midiPlaybackFlags,midiEventTickBalance;
extern uint16_t midiBeatRemainder,midiBarRemainder,midiElapsedBars;
extern uint8_t midiStateUpdateInProgress,midiTickInProgress,midiTrackRunningStatus;
extern uint16_t midiTrackChannelHandlers[MIDI_TRACK_CHANNEL_HANDLERS];
extern uint16_t midiTrackSystemHandlers[MIDI_TRACK_SYSTEM_HANDLERS];

uint32_t MwDivideMidiTimingRatio(uint32_t numerator,uint32_t denominator);
uint16_t MwSetMidiTickDenominator(uint16_t denominator);
void MwSetMidiTickNumerator(uint16_t division,uint16_t tempo);
void MwSetMidiBeatBarDivisions(uint16_t beatScale,uint16_t denominatorScale,uint16_t barDivision);
void MwMidiAllNotesOff(void);
/* Offset pointer projects live SI, not an extra native stack argument. */
uint32_t MwReadMidiVariableLengthQuantity(uint16_t resourceSelector,uint16_t *streamOffset);
uint16_t MwStartMidiTrack(uint16_t resourceSelector,uint16_t streamOffset,uint16_t playbackFlags);
void MwPauseMidiTrack(void);
void MwResumeMidiTrack(void);
void MwMuteMidiTrack(void);
void MwUnmuteMidiTrack(void);
void MwStopMidiTrack(void);
void MwTickMidiTrack(void);
/* Native byte dispatcher, not an SDL MIDI player. */
void MwDispatchMidiByte(uint16_t midiWord);

enum { MIDI_VOICE_CAPACITY=11, MIDI_CHANNEL_RECORDS=48,
    MIDI_LIST_HEAD_OFFSET=0x03FA, MIDI_LIST_TAIL_OFFSET=0x03FC,
    MIDI_VOICES_OFFSET=0x0400, MIDI_CHANNELS_OFFSET=0x04C6,
    MIDI_DUMMY_CHANNEL_OFFSET=0x0646, MIDI_INSTRUMENTS_OFFSET=0x064E,
    MIDI_INSTRUMENT_BYTES=0x0C, MIDI_SYSEX_BUFFER_OFFSET=0x07CE,
    MIDI_SPEAKER_BACKEND_OFFSET=0x07FC, MIDI_MPU_BACKEND_OFFSET=0x01EE,
    MIDI_MPU_QUEUE_CAPACITY=256, MIDI_DRUM_CHANNEL_BIT=0x40,
    MIDI_DRUM_CHANNEL_FIRST=16, MIDI_DRUM_CHANNEL_COUNT=32,
    SPEAKER_SILENT_PERIOD=0x0014, SPEAKER_FIRST_NOTE=14,
    SPEAKER_PERIOD_WORDS=242, SPEAKER_ROTATION_TICKS=2 };
typedef struct MwMidiChannel
{
    uint8_t backendEligibilityMask,voicePriority,reservedVoiceCount;
    uint8_t instrumentId,transposeOrDrumBase,reservationBalance;
    uint16_t instrumentOffset;
} MwMidiChannel;
typedef struct MwMidiVoiceLinks {uint16_t nextOffset,previousOffset;} MwMidiVoiceLinks;
typedef struct MwMidiVoice
{
    union
    {
        struct {MwMidiVoiceLinks links;uint8_t backendMask,activePriority;uint16_t channelOffset;};
        MwMidiChannel channelAlias;
    };
    uint8_t originalNote,translatedNote;
    uint16_t pitPeriod;
    uint8_t unidentifiedField7[6];
} MwMidiVoice;
typedef union MwMidiListSentinels
{
    struct {MwMidiVoiceLinks head;uint16_t lastOffset;} fromHead;
    struct {uint16_t firstOffset;MwMidiVoiceLinks tail;} fromTail;
} MwMidiListSentinels;
/* SysEx's predecessor+6 reload can also select either sentinel as the
 * eight-byte channel writer, overlapping the first voice. One owner only. */
typedef union MwMidiVoicePool
{
    struct {MwMidiListSentinels sentinels;MwMidiVoice voices[MIDI_VOICE_CAPACITY];} records;
    MwMidiChannel headChannelAlias;
    struct {uint16_t prefix;MwMidiChannel channel;} tailChannelAlias;
} MwMidiVoicePool;
_Static_assert(offsetof(MwMidiVoicePool,records.voices)==6,"Sentinel/first voice alias");
typedef struct MwCampaignAudioBackend
{
    uint16_t nameOffset;
    uint8_t sysexManufacturer,voiceCapacity;
    uint16_t probeOffset,initializeOffset,shutdownOffset,rawByteOffset;
    uint16_t noteOnOffset,noteOffOffset,initializeVoicesOffset;
    uint16_t decodeInstrumentOffset,timerOffset;
    uint8_t timerRateShift,selectedVoiceCount;
    uint16_t voicePoolOffset;
} MwCampaignAudioBackend;
/* One canonical overlapping workspace: unchecked SysEx writes can reach its
 * own parser, backend and tone table. This is not a DS image or span registry.
 * BYTE/WORD union views require the native little-endian host representation. */
typedef struct MwMidiSysexWorkspace
{
    uint8_t recordBuffer[0x20];
    uint16_t parserOffset,handlerDisplacement;
    uint8_t recordLength,bytesRemaining;
    uint16_t writeOffset,recordHandlers[3];
    MwCampaignAudioBackend speakerBackend;
    uint8_t speakerNameAndUnknown[0x12];
    uint16_t currentVoiceOffset;
    uint8_t rotationCountdown,voicesRemaining;
    uint16_t currentPeriod;
    /* Normal notes use the first114 words. Higher BYTE notes intentionally
     * read adjacent authored words, not a clamped or invented frequency. */
    uint16_t tonePeriodsAndAdjacentWords[SPEAKER_PERIOD_WORDS];
} MwMidiSysexWorkspace;
_Static_assert(sizeof(MwMidiChannel)==0x08,"Native MIDI channel width");
_Static_assert(sizeof(MwMidiVoice)==0x12,"Native voice width");
_Static_assert(offsetof(MwMidiVoice,pitPeriod)==0x0A,"Native speaker overlay");
_Static_assert(sizeof(MwMidiListSentinels)==6,"Overlapping list sentinels");
_Static_assert(sizeof(MwCampaignAudioBackend)==0x1A,"Native backend data width");
_Static_assert(offsetof(MwMidiSysexWorkspace,speakerBackend)==0x2E,"SysEx/backend alias");
_Static_assert(offsetof(MwMidiSysexWorkspace,currentVoiceOffset)==0x5A,"Speaker cursor alias");
_Static_assert(offsetof(MwMidiSysexWorkspace,tonePeriodsAndAdjacentWords)==0x60,"Speaker tone alias");
extern MwMidiVoicePool audioVoicePool;
#define audioVoiceList audioVoicePool.records.sentinels
#define audioVoices audioVoicePool.records.voices
extern MwMidiChannel audioChannels[MIDI_CHANNEL_RECORDS],audioDummyChannel;
extern MwMidiSysexWorkspace midiSysex;
extern MwCampaignAudioBackend *selectedAudioBackend;
extern MwCampaignAudioBackend mpuBackend;
extern uint8_t mpuTransmitQueue[MIDI_MPU_QUEUE_CAPACITY];
extern volatile uint16_t mpuQueueWriteIndex,mpuQueueReadIndex,mpuQueueByteCount;
extern uint32_t mpuPreviousIrq;
extern uint8_t musicRuntimeHeader[0x42];
uint16_t MwProbeMpuBackend(void);
void MwInitializeMpuUartBackend(void);
void MwDrainAndShutdownMpuBackend(void);
uint16_t MwQueueMpuByte(uint16_t midiWord);
void MwTickMpuTransmitQueue(void);
void MwMpuReceiveInterrupt(void);
extern uint16_t midiByteHandlerDisplacement,midiDispatchChannel,midiFirstDataByte,midiControllerValue;
extern uint16_t midiByteHandlers[14];
#define audioVoiceListHead audioVoiceList.fromHead.head.nextOffset
#define audioVoiceListTailPrevious audioVoiceList.fromTail.tail.previousOffset
#define audioVoiceCount audioDummyChannel.reservationBalance
#define speakerBackend midiSysex.speakerBackend
#define speakerVoiceCount speakerBackend.selectedVoiceCount
#define speakerVoicePoolOffset speakerBackend.voicePoolOffset
/* Representation bridges ONLY for genuinely observable near-link values.
 * 0DB2 rereads head+6 as the first voice's numeric next link, then uses that
 * value as a channel writer base. Host pointers would change this behavior. */
MwMidiVoice *MwResolveMidiVoice(uint16_t nativeOffset);
MwMidiVoiceLinks *MwResolveMidiVoiceLinks(uint16_t nativeOffset);
MwMidiChannel *MwResolveMidiChannel(uint16_t nativeOffset);
uint16_t MwReleaseCampaignMidiVoice(uint16_t channelIndex,uint16_t note);
uint16_t MwAllocateCampaignMidiVoice(uint16_t channelIndex,uint16_t note,uint16_t velocity);
void MwSilenceCampaignMidiVoices(void);
void MwSetCampaignMidiProgram(uint16_t channelIndex,uint16_t instrumentId);
void MwDispatchMidiByteAndSysexState(uint16_t midiWord);
typedef struct MwMidiChannelConfiguration
{uint8_t channelIndex,eligibilityMask,voicePriority,reservations,instrumentId,transpose;} MwMidiChannelConfiguration;
void MwApplySysexChannelConfiguration(const MwMidiChannelConfiguration *configuration);
void MwApplySysexInstrument(const uint8_t *source);
void MwApplySysexVoiceCount(const uint8_t *source);
enum { MIDI_RESET_PARSER_ENTRY=0x0E82,MIDI_REBUILD_POOL_ENTRY=0x0E9B };
void MwResetMidiParserState(uint16_t entryOffset);
#define MwRebuildCampaignVoicePool() MwResetMidiParserState(MIDI_REBUILD_POOL_ENTRY)
uint16_t MwProbeMusicBackend(MwCampaignAudioBackend *backend);
void MwSelectAndInitializeMidiBackend(MwCampaignAudioBackend *backend);
void MwShutdownMusicBackend(void);
uint16_t MwSendMidiByteToSelectedBackend(uint16_t midiWord);
uint16_t MwProbeSpeakerBackend(void);
void MwInitializeSpeakerBackend(void);
void MwShutdownSpeakerBackend(void);
uint16_t MwIgnoreSpeakerRawMidiByte(void);
uint16_t MwStartSpeakerVoice(MwMidiVoice *voice);
uint16_t MwStopSpeakerVoice(MwMidiVoice *voice);
uint16_t MwWriteSpeakerPeriod(uint16_t period);
void MwInitializeSpeakerVoiceRouting(void);
void MwIgnoreSpeakerInstrument(void);
void MwTickSpeakerVoices(void);
#endif
