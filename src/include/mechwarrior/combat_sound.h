#ifndef MECHWARRIOR_COMBAT_SOUND_H
#define MECHWARRIOR_COMBAT_SOUND_H
#include "mechwarrior/combat_resources.h"
#include "mechwarrior/native_address.h"
#include <stdbool.h>

enum { COMBAT_STREAM_SLOTS=0x14,COMBAT_STREAM_TABLE=0x4BC0,COMBAT_STREAM_STRIDE=8,
    COMBAT_STREAM_ACTIVE_MASK=0x83,COMBAT_STREAM_READ=1,COMBAT_STREAM_WRITE=2,
    COMBAT_STREAM_OWNS_BUFFER=8,COMBAT_STREAM_PROTECTED=0x40,COMBAT_STREAM_AUX_EXTERNAL_BUFFER=1,
    COMBAT_TAGGED_STREAMS=4,COMBAT_TAG_PATH_BYTES=0x1A,COMBAT_TAG_LEVELS=7,
    COMBAT_SOUND_BANKS=0x0B,COMBAT_MUSIC_BANKS=0x10,COMBAT_MUSIC_FIRST_ROOT=0x0A,
    COMBAT_MUSIC_LOAD_ROOTS=0x0F,COMBAT_AUDIO_BANK_ROOTS=COMBAT_MUSIC_FIRST_ROOT+COMBAT_MUSIC_LOAD_ROOTS,
    COMBAT_MUSIC_SEQUENCE_WORDS=0x0A,COMBAT_MUSIC_SEQUENCE_ROWS=0x0A,
    COMBAT_SOUND_SPEAKER=1,COMBAT_SOUND_TANDY=2,
    COMBAT_SOUND_MPU=0x10,COMBAT_SOUND_ADLIB=0x20,COMBAT_SOUND_8SV_MASK=0x0C,COMBAT_SOUND_CHUNK_BYTES=0x4E20 };
enum { ADLIB_INDEX_PORT=0x388,ADLIB_DATA_PORT=0x389,ADLIB_TIMER_CONTROL=4,
    ADLIB_TIMER_RESET=0x60,ADLIB_IRQ_RESET=0x80,ADLIB_TIMER_ONE=2,
    ADLIB_TIMER_START=0x21,ADLIB_TIMER_STATUS_MASK=0xE0,ADLIB_TIMER_EXPECTED_STATUS=0xC0 };
extern uint8_t adlibProbeInitialStatus;
/* Raw INT15 input AX -> Carry, not a hardware-detected/backend policy seam. */
extern bool (*btechHostQuerySystemConfiguration)(uint16_t ax);
void BtechWriteAdlibProbeRegister(uint8_t registerIndex,uint8_t value);
uint16_t BtechProbeAdlibTimerStatus(void);
uint16_t BtechProbePlatformAudioMask(void);
uint16_t BtechSelectAndInstallAudioBackends(uint16_t requestedBackendMask);
uint16_t BtechProbeAudioBackendMask(void);
uint16_t BtechProbeMpuWithTemporaryInterrupt(void);
uint16_t BtechSendMpuProbeCommandAndWaitAck(uint8_t command);
/* The original IRQ does not establish DS. This entry supports interrupted
 * DS304E only; foreign-DS delivery must not be routed here. */
void BtechMpuProbeInterrupt(void);
extern uint16_t mpuProbeDataPort,mpuProbeStatusPort,mpuProbeCommandPort;
extern uint16_t mpuProbeReceivedByteCount;
extern uint32_t mpuProbePreviousIrq;
/* Hardware view, e.g. an instrumented DOS machine's live RAM. No fabricated
 * zero arena, game-memory registry or default IVT is installed by the port. */
extern MwNativeByteView (*btechHostBorrowDosMemory)(uint16_t selector);
enum { COMBAT_TONE_CHANNELS=8,COMBAT_SPEAKER_CHANNELS=4,COMBAT_TONE_HEADER_BYTES=8,
    TONE_PIT_DATA_PORT=0x42,TONE_SPEAKER_GATE_PORT=0x61,TONE_TANDY_DATA_PORT=0xC0,
    TONE_SPEAKER_GATE_BITS=3,TONE_STATE_FIELD_MASK=0x7E,
    TONE_BRANCH_RETURN=0x0232,TONE_BRANCH_CALL=0x0239,TONE_BRANCH_STOP=0x01E5,TONE_BRANCH_REPEAT=0x01B3 };
enum { TONE_SPEAKER_PITCH_TABLE=0x03B4,TONE_TANDY_PITCH_TABLE=0x04F4,
    TONE_SPEAKER_PITCH_WORDS=0xA0,TONE_TANDY_PITCH_WORDS=0x94,TONE_NOISE_PERIOD_WORDS=3,
    TONE_MUSIC_NOTE_OFF=0x0352,TONE_MUSIC_NOTE_ON=0x0374,TONE_MUSIC_PITCH_BEND=0x03F0,
    TONE_MUSIC_NO_OPERATION=0x03FF,TONE_NOISE_STATE_INDEX=7,
    TONE_NOISE_ENVELOPES=0x0622,TONE_NOISE_ENVELOPE_BYTES=0x18 };

typedef struct BtechToneChannelState {
    uint16_t bytecodeOffset,commandDelayTicks,basePeriod,periodStep;
    uint16_t outputPeriod,outputLevel,levelStep;
    uint16_t unidentifiedField8[4];
    uint16_t levelSequencePosition,levelSequenceTicks,modulationPhase,modulationPhaseStep;
    int16_t modulationScale;
    uint16_t branchRepeatCount;
    uint16_t unidentifiedField13[4];
    uint16_t modulationPhaseLimit,levelSequenceOffset,modulationTableOffset;
} BtechToneChannelState;
typedef struct BtechToneChannelFlags {uint8_t activityMask,unidentifiedField2;} BtechToneChannelFlags;
extern uint16_t soundEventIndices[COMBAT_TONE_CHANNELS],soundPriorities[COMBAT_TONE_CHANNELS];
extern BtechToneChannelFlags channelFlags[COMBAT_TONE_CHANNELS];
extern BtechToneChannelState toneChannels[COMBAT_TONE_CHANNELS];
extern BtechToneChannelState *toneChannelStates[COMBAT_TONE_CHANNELS];
extern MwNativeByteView toneEventData[COMBAT_TONE_CHANNELS],toneFallbackData[COMBAT_TONE_CHANNELS];
extern uint16_t fallbackAllocationOffsets[COMBAT_TONE_CHANNELS],levelSequenceDirectoryOffsets[COMBAT_TONE_CHANNELS];
extern uint16_t bytecodeReturnOffset,currentChannelDisplacement,selectedSpeakerPriority;
extern uint16_t bytecodeBranchHandlers[4];
extern BtechToneChannelState *selectedSpeakerState;
extern uint8_t soundOutputEnableMask,soundEffectsEnableMask;
extern void (*audioTickCallback)(void);
typedef uint16_t (*BtechMusicMessage)(uint16_t statusDisplacement,uint16_t channel,uint16_t packedData);
extern BtechMusicMessage audioMusicCallback;
extern uint8_t softwareMusicRateShift,softwareMusicDivisionScale;
extern uint16_t midiTrackIndex;
extern uint8_t midiRunningStatus,midiMessageData[2],midiMessageDataLength,midiChannel;
extern uint8_t midiMessageLengths[8];
extern uint16_t musicSequenceIndex,mpuTempoInterval,combatMidiStreamOffset;
extern uint16_t queuedMidiDeltaTicks,softwareMusicDelayTicks,midiTempoHighBytes;
extern MwNativeByteView midiStreamData;
/* Native stream-pair presence differs from an OFFSET0 parser check. */
extern bool midiStreamSelectorPresent;
extern uint16_t activeMusicBankIndex;
extern int16_t musicSequences[COMBAT_MUSIC_SEQUENCE_ROWS][COMBAT_MUSIC_SEQUENCE_WORDS];
extern uint8_t musicNoteBase,musicChannelBias;
extern uint16_t musicPitchTableOffset,musicPriority,musicHandlerOffsets[8];
extern uint16_t speakerMusicPeriods[TONE_SPEAKER_PITCH_WORDS],tandyMusicPeriods[TONE_TANDY_PITCH_WORDS],noiseMusicPeriods[TONE_NOISE_PERIOD_WORDS];
/* Authored229C:0020 is RETF. AX residue is needed by the parser. */
uint16_t BtechMusicMessageNoOperation(uint16_t statusDisplacement,uint16_t channel,uint16_t packedData);
uint16_t BtechDispatchToneMidiMessage(uint16_t statusDisplacement,uint16_t channel,uint16_t packedData);
void BtechSetToneMusicPriorityAndTranslation(uint16_t priority);
void BtechStartSoftwareMusicTrack(MwNativeByteView musicHeader,uint16_t headerOffset,bool selectorPresent);
void BtechTickSoftwareMusic(void);
uint8_t BtechDecodeNextMidiEvent(uint8_t inheritedDeltaHighByte);
typedef void (*BtechAudioTick)(void);
BtechAudioTick BtechSelectTandyOrFallbackAudioCallback(void);
BtechAudioTick BtechSelectSpeakerAdlibOrMpuCallback(void);
void BtechAudioTimerNoOp(void);
void BtechTickSpeakerToneChannels(void);
void BtechTickTandyToneChannels(void);
void BtechAdvanceToneChannel(BtechToneChannelState *state,MwNativeByteView event,uint16_t channel);
void BtechParseToneCommands(BtechToneChannelState *state,MwNativeByteView event,uint16_t cursor,uint16_t channel);
void BtechStartToneInterpreterChannel(MwNativeByteView event,uint16_t eventOffset,uint16_t channel,MwNativeByteView fallback,uint16_t fallbackOffset);
/*00AF is priority entry8, immediately after the eight tone priorities. */
extern uint16_t sampleSoundPriority;
extern uint16_t timerSoundEventIndex,savedSoundTimerDivisor;
extern uint8_t soundTimerInstalled;
extern uint16_t *samplePriorityOwner;
extern uint32_t savedSampleIrqVector;
bool BtechImportToneInitialData(const uint8_t *data,size_t bytes,const uint8_t *toneData,size_t toneBytes);
void BtechStopMatchingToneChannels(uint16_t soundEventIndex,uint8_t activityMask);
uint16_t BtechStopSoundEventAndTimer(uint16_t soundEventIndex);
/* Ordinary DS304E path. Sample IRQ0DD3's foreign-DS call is not yet converted. */
void BtechRestoreSoundTimerInterrupt(void);

/* Host stdio replaces buffering/OS IO, NOT the observable near FILE identity.
 * This is a typed FILE slot, not a native-memory or address-span registry. */
typedef struct BtechRuntimeStream {
    FILE *file;
    uint8_t flags,dosHandle;
} BtechRuntimeStream;
typedef struct BtechTaggedStream {
    BtechRuntimeStream *stream;
    char tagPath[COMBAT_TAG_PATH_BYTES];
    uint32_t branchEnds[COMBAT_TAG_LEVELS];
    uint16_t tagDepth,occurrence;
    uint32_t payloadPosition,payloadBytes;
} BtechTaggedStream;
typedef struct BtechSoundAllocationNode {
    BtechResourceAllocation allocation;
    struct BtechSoundAllocationNode *next;
    char chunkTag[0x0A];
} BtechSoundAllocationNode;

typedef struct BtechSoundEvent {
    MwNativeByteView data; /* Borrowed selector-relative allocation bytes. */
    uint16_t offset; /* Native near cursor remains observable to bytecode. */
} BtechSoundEvent;
BtechSoundEvent BtechAdvanceMusicSequence(void);
typedef struct BtechSoundBank {
    BtechSoundAllocationNode root;
    uint16_t eventCount,unidentifiedField9;
    BtechSoundAllocationNode *fallback;
    BtechSoundEvent *events; /* Relocated binary directory; no second byte mirror. */
    uint16_t eventReferenceCapacity; /* Host allocation extent, not a native game field. */
} BtechSoundBank;
/* Slot10 is also the first music root in the native image. Both domains must
 * use this owner when music is connected; never allocate a second root there. */
extern BtechSoundBank soundBanks[COMBAT_AUDIO_BANK_ROOTS];
extern BtechRuntimeStream runtimeStreams[COMBAT_STREAM_SLOTS];
extern uint16_t lastStreamOffset,streamSetupCounter;
extern BtechTaggedStream taggedStreams[COMBAT_TAGGED_STREAMS],taggedStreamBackup;
extern uint16_t selectedAudioBackendMask,activeSoundBankIndex,soundAllocationError;
extern BtechResourceAllocation soundAllocationOverride;
bool BtechImportSoundInitialData(const uint8_t *data,size_t bytes);
BtechRuntimeStream *BtechRuntimeStreamFindFreeSlot(void);
BtechRuntimeStream *BtechRuntimeStreamOpen(const char *filename,const char *mode);
BtechTaggedStream *BtechFindTaggedStreamRecord(BtechRuntimeStream *stream);
void BtechResetTaggedStreamAndRewind(BtechTaggedStream *taggedStream);
uint16_t BtechEqualNearStringsBounded(const char *left,const char *right,uint16_t remaining);
BtechRuntimeStream *BtechOpenTaggedResourceStream(const char *filename);
uint32_t BtechRestoreTaggedStreamSnapshotAndSeek(BtechTaggedStream *taggedStream);
uint32_t BtechFindResourceTagPosition(BtechRuntimeStream *stream,const char *tag,uint16_t occurrence);
uint32_t BtechGetTaggedPayloadLength(BtechRuntimeStream *stream);
uint16_t BtechCloseTaggedResourceStream(BtechRuntimeStream *stream);
uint16_t BtechIsRegisteredTaggedStream(BtechRuntimeStream *stream);
/* Filename and existing stream project the original overloaded WORD. The last
 * argument is CPU-stack residue transport ONLY, never a native source argument. */
uint16_t BtechLoadSoundBank(const char *filename,BtechRuntimeStream *stream,uint16_t *inheritedStreamIdentity);
uint16_t BtechMergeSoundBackendDirectory(BtechSoundBank *bank,BtechRuntimeStream *stream,const char *tag);
BtechSoundAllocationNode *BtechLoadTaggedSoundAllocation(BtechSoundBank *bank,BtechRuntimeStream *stream,const char *tag);
uint16_t BtechSelectActiveSoundBank(uint16_t bankHandle);
uint16_t BtechAllocateToneEventChannels(BtechSoundEvent event,uint16_t priority,uint16_t soundEventIndex);
uint16_t BtechDispatchSoundBankEvent(int16_t soundEventIndex,uint16_t priority);
void BtechPlaySoundIfEnabled(int16_t soundEventIndex,uint16_t priority);
#endif
