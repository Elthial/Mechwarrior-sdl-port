#include "mechwarrior/combat_mpu.h"
#include "mechwarrior/combat_startup.h"
#include <stdlib.h>

/* Original combat Roland driver. The MPU hardware adapter requests events;
 * these routines retain the game's parsing, interrupt and byte-write order.
 * Card installation does not add MIDI effects to a speaker-only sound bank. */
/* 2176:000C. Reset failure is not an installation failure; the version IRQ is. */
uint16_t BtechInstallMpuInterruptHandler(uint16_t unusedBasePort,uint8_t irq)
{
    (void)unusedBasePort;
    if (combatMpuInstalled!=1)
    {
        combatMpuVector=(uint8_t)(irq+8);combatMpuPicUnmask=(uint8_t)~(irq<8?(1u<<irq):0);
        BtechSendMpuCommandAndWaitAck(BTECH_MPU_RESET);btechHostReadPort(combatMpuDataPort);
        combatMpuPreviousVector=btechHostGetInterruptVector(combatMpuVector);
        btechHostSetInterruptVector(combatMpuVector,BTECH_MPU_HANDLER_VECTOR);
        btechHostWritePort(PIC_MASK_PORT,btechHostReadPort(PIC_MASK_PORT)&combatMpuPicUnmask);
        combatMpuInstalled=1;combatMpuReceivedBytes=0;
        BtechSendMpuCommandAndWaitAck(BTECH_MPU_VERSION);
        for (uint16_t polls=0x4E20;polls;--polls)
        {
            if (btechHostPollInterrupts) btechHostPollInterrupts();
            if (combatMpuReceivedBytes) return combatMpuInstalled;
        }
        BtechRestoreMpuInterruptHandler();
    }
    return combatMpuInstalled;
}
/* 2176:0089. All exits restore entry IF. Non-ACK bytes use the real handler. */
uint16_t BtechSendMpuCommandAndWaitAck(uint8_t command)
{
    if (!btechHostReadInterrupts) abort();
    uint8_t interrupts=btechHostReadInterrupts();
    uint16_t accepted=0;
    for (uint16_t polls=UINT16_MAX;polls;--polls)
    {
        if (btechHostReadPort(combatMpuStatusPort)&BTECH_MPU_BUSY) continue;
        btechHostSetInterrupts(0);btechHostWritePort(combatMpuCommandPort,command);
        for (uint16_t acknowledgement=UINT16_MAX;acknowledgement;--acknowledgement)
        {
            if (btechHostReadPort(combatMpuStatusPort)&BTECH_MPU_INPUT_EMPTY) continue;
            uint8_t response=btechHostReadPort(combatMpuDataPort);
            if (response==BTECH_MPU_ACK) {accepted=1;goto restoreInterrupts;}
            MpuHandleReceivedByte(response);
        }
        btechHostSetInterrupts(1);break;
    }
restoreInterrupts:
    btechHostSetInterrupts(interrupts);return accepted;
}
/* 2176:00CA. Separate native FAR stack wrapper. */
uint16_t BtechSendMpuCommandStack(uint8_t command) {return BtechSendMpuCommandAndWaitAck(command);}
/* 2176:00D5. Always clear installed, even when it was not exactly one. */
void BtechRestoreMpuInterruptHandler(void)
{
    if (combatMpuInstalled==1)
    {
        btechHostWritePort(PIC_MASK_PORT,btechHostReadPort(PIC_MASK_PORT)|(uint8_t)~combatMpuPicUnmask);
        BtechSendMpuCommandAndWaitAck(BTECH_MPU_RESET);
        btechHostSetInterruptVector(combatMpuVector,combatMpuPreviousVector);
    }
    combatMpuInstalled=0;
}
/* 2176:0140. A successful last poll counts; timeout never writes. */
uint16_t MpuWriteDataByte(uint8_t value)
{
    for (uint16_t polls=UINT16_MAX;polls;--polls)
    {
        if (btechHostReadPort(combatMpuStatusPort)&BTECH_MPU_BUSY) continue;
        btechHostWritePort(combatMpuDataPort,value);return 1;
    }
    return 0;
}
/* 2176:0163. Eight request IDs drain one shared staging slot, not eight streams. */
void MpuHandleReceivedByte(uint8_t value)
{
    ++combatMpuReceivedBytes;
    if (value<0xF0) return;
    if (value<0xF8)
    {
        MpuSendQueuedTrackEvent();
        if (!queuedMidiDeltaTicks) BtechDecodeNextMidiEvent(0);
        return;
    }
    if (value!=BTECH_MPU_CONDUCTOR_REQUEST) return;
    MpuWriteDataByte(0x20);MpuWriteDataByte(0xE0);MpuWriteDataByte((uint8_t)(mpuTempoInterval>>1));
}
/* 2176:01B2. Signed delta comparison and failed-write consumption are native. */
void MpuSendQueuedTrackEvent(void)
{
    if ((int16_t)queuedMidiDeltaTicks>BTECH_MPU_MAX_TIME_TAG)
    {
        queuedMidiDeltaTicks-=BTECH_MPU_MAX_TIME_TAG;
        MpuWriteDataByte(BTECH_MPU_MAX_TIME_TAG);MpuWriteDataByte(BTECH_MPU_OVERFLOW);return;
    }
    MpuWriteDataByte((uint8_t)queuedMidiDeltaTicks);queuedMidiDeltaTicks=0;
    switch ((midiRunningStatus>>3)&0x0E)
    {
    case 0:MpuWriteThreeByteChannelEvent();break;
    case 2:MpuWriteNoteOnOrTimingOverflow();break;
    case 8:MpuWriteTwoByteChannelEvent();break;
    default:MpuWriteUnsupportedEventMarker();break;
    }
}
/* 2176:01F5. */
void MpuWriteNoteOnOrTimingOverflow(void)
{
    if (!(soundOutputEnableMask&2)) {MpuWriteDataByte(BTECH_MPU_OVERFLOW);return;}
    MpuWriteThreeByteChannelEvent();
}
/* 2176:0206. Each byte is read live after the preceding write. */
void MpuWriteThreeByteChannelEvent(void)
{MpuWriteDataByte(midiRunningStatus);MpuWriteDataByte(midiMessageData[0]);MpuWriteDataByte(midiMessageData[1]);}
/* 2176:0225. */
void MpuWriteTwoByteChannelEvent(void) {MpuWriteDataByte(midiRunningStatus);MpuWriteDataByte(midiMessageData[0]);}
/* 2176:023A. */
void MpuWriteUnsupportedEventMarker(void) {MpuWriteDataByte(BTECH_MPU_OVERFLOW);}
/* 2176:03C0. Upload a changed native allocation/offset pair, then trigger the
 * instrument audition. Contents can change without invalidating this cache. */
uint16_t BtechUploadMpuInstrumentIfChanged(BtechSoundEvent instrument)
{
    enum { INSTRUMENT_HEADER_BYTES=10,MPU_SYSTEM_MESSAGE=0xDF,MPU_TRACK_TWO=0xD2,
        AUDITION_NOTE_ON=0x95,AUDITION_NOTE=0x45,AUDITION_VELOCITY=0x7F };
    if (instrument.offset!=combatMpuInstrumentOffset || instrument.selector!=combatMpuInstrumentSelector)
    {
        if (!BtechSendMpuCommandStack(MPU_SYSTEM_MESSAGE)) return 0;
        uint16_t byteIndex=INSTRUMENT_HEADER_BYTES;
        while (MwReadNativeViewWord(&instrument.data,(uint16_t)(instrument.offset+2))>byteIndex)
        {
            MpuWriteDataByte(MwReadNativeViewByte(&instrument.data,(uint16_t)(instrument.offset+byteIndex)));
            ++byteIndex;
        }
        combatMpuInstrumentOffset=instrument.offset;
        combatMpuInstrumentSelector=instrument.selector;
    }
    if (!BtechSendMpuCommandStack(MPU_TRACK_TWO)) return 0;
    MpuWriteDataByte(AUDITION_NOTE_ON);MpuWriteDataByte(AUDITION_NOTE);
    return MpuWriteDataByte(AUDITION_VELOCITY);
}
/* 2176:0464. Configure the real MPU command interface; no score replacement. */
uint16_t BtechInitializeRolandMpuBackend(void)
{
    if (!BtechInstallMpuInterruptHandler(0x330,BTECH_MPU_IRQ)) return 0;
    BtechSendMpuCommandStack(0xC4);
    BtechSendMpuCommandStack(0xEE);MpuWriteDataByte(0xFF);
    BtechSendMpuCommandStack(0xEF);MpuWriteDataByte(0xFF);
    BtechSendMpuCommandStack(0xE4);MpuWriteDataByte(0x18);
    BtechSendMpuCommandStack(0xE6);MpuWriteDataByte(4);
    BtechSendMpuCommandStack(0xE7);MpuWriteDataByte(0x80);
    BtechSendMpuCommandStack(0xEC);MpuWriteDataByte(1);
    BtechSendMpuCommandStack(0x8F);return 1;
}
/* 2176:0106. Host only delivers this entry when input is pending. Empty native
 * chaining depends on interrupted DS and is not redirected to a fake handler. */
void BtechMpuInterrupt(void)
{
    if (btechHostReadPort(combatMpuStatusPort)&BTECH_MPU_INPUT_EMPTY) abort();
    MpuHandleReceivedByte(btechHostReadPort(combatMpuDataPort));
    btechHostWritePort(PIC_COMMAND_PORT,PIC_END_OF_INTERRUPT);
}
/* 2176:0244. */
uint16_t BtechStopMpuMusicTrack(void)
{BtechSendMpuCommandStack(5);BtechSendMpuCommandStack(0xB9);return 1;}
/* 2176:0260. */
uint16_t BtechRestartMpuMusicPlayback(void)
{BtechStopMpuMusicTrack();return BtechSendMpuCommandStack(0x0A);}
/* 2176:0272. Preserve the shipped timebase fallthroughs, including failures. */
uint16_t BtechStartMpuMusicTrack(MwNativeByteView header,uint16_t offset)
{
    int16_t timebase=(int16_t)MwReadNativeViewWord(&header,(uint16_t)(offset+4));
    switch (timebase)
    {
    case 24:BtechSendMpuCommandStack(0xC2);break;
    case 48:BtechSendMpuCommandStack(0xC3); /* Native falls through to C8. */
    case 96:BtechSendMpuCommandStack(0xC8);break;
    case 120:BtechSendMpuCommandStack(0xC4);BtechSendMpuCommandStack(0xC5);
    case 144:BtechSendMpuCommandStack(0xC6);
    case 168:BtechSendMpuCommandStack(0xC7);return 0;
    case 72:BtechSendMpuCommandStack(0xC6);BtechSendMpuCommandStack(0xC7);return 0;
    default:return 0;
    }
    BtechSendMpuCommandStack(0xE0);
    MpuWriteDataByte((uint8_t)((int16_t)MwReadNativeViewWord(&header,(uint16_t)(offset+2))/2));
    BtechSendMpuCommandStack(0xEE);MpuWriteDataByte(0xFF);
    BtechSendMpuCommandStack(0xEF);MpuWriteDataByte(0xFF);
    BtechSendMpuCommandStack(0xE7);MpuWriteDataByte(0x10);
    BtechSendMpuCommandStack(0xEC);MpuWriteDataByte(1);
    BtechSendMpuCommandStack(0xB8);BtechSendMpuCommandStack(0x0A);return 1;
}
