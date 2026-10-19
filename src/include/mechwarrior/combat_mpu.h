#ifndef MECHWARRIOR_COMBAT_MPU_H
#define MECHWARRIOR_COMBAT_MPU_H
#include "mechwarrior/combat_sound.h"
enum { BTECH_MPU_IRQ=2,BTECH_MPU_VECTOR=0x0A,BTECH_MPU_IRQ_MASK=4,
    BTECH_MPU_BUSY=0x40,BTECH_MPU_INPUT_EMPTY=0x80,BTECH_MPU_ACK=0xFE,
    BTECH_MPU_RESET=0xFF,BTECH_MPU_VERSION=0xAC,BTECH_MPU_MAX_TIME_TAG=0xEF,
    BTECH_MPU_OVERFLOW=0xF8,BTECH_MPU_CONDUCTOR_REQUEST=0xF9 };
#define BTECH_MPU_PROBE_VECTOR UINT32_C(0x1ADF1082)
#define BTECH_MPU_HANDLER_VECTOR UINT32_C(0x21760106)
extern uint16_t combatMpuDataPort,combatMpuStatusPort,combatMpuCommandPort,combatMpuReceivedBytes;
extern uint8_t combatMpuInstalled,combatMpuPicUnmask,combatMpuVector;
extern uint32_t combatMpuPreviousVector;
extern uint16_t combatMpuInstrumentOffset,combatMpuInstrumentSelector;
bool BtechImportMpuInitialData(const uint8_t *data,size_t bytes);
uint16_t BtechInstallMpuInterruptHandler(uint16_t unusedBasePort,uint8_t irq);
uint16_t BtechSendMpuCommandAndWaitAck(uint8_t command);
uint16_t BtechSendMpuCommandStack(uint8_t command);
void BtechRestoreMpuInterruptHandler(void);
uint16_t MpuWriteDataByte(uint8_t value);
void MpuHandleReceivedByte(uint8_t value);
void MpuSendQueuedTrackEvent(void);
void MpuWriteNoteOnOrTimingOverflow(void);
void MpuWriteThreeByteChannelEvent(void);
void MpuWriteTwoByteChannelEvent(void);
void MpuWriteUnsupportedEventMarker(void);
uint16_t BtechInitializeRolandMpuBackend(void);
void BtechMpuInterrupt(void);
uint16_t BtechStopMpuMusicTrack(void);
uint16_t BtechStartMpuMusicTrack(MwNativeByteView header,uint16_t offset);
uint16_t BtechRestartMpuMusicPlayback(void);
uint16_t BtechUploadMpuInstrumentIfChanged(BtechSoundEvent instrument);
#endif
