#include "mechwarrior/port_diagnostics.h"
#include "mechwarrior/campaign_music.h"
#include "mechwarrior/combat_startup.h"
#include <stdlib.h>

enum { MPU_WRITE_BUSY=0x40,MPU_INPUT_EMPTY=0x80,MPU_RESET_COMMAND=0xFF,
    MPU_UART_COMMAND=0x3F,MPU_ACK=0xFE,MPU_IRQ_VECTOR=0x0A,
    MPU_RECEIVE_HANDLER=0x0F410A01 };

/* MW_MAIN0F41:0912. LOOPNE's last-read timeout and unconditional STI remain. */
uint16_t MwProbeMpuBackend(void)
{
    mwMusicHardware.setInterruptsEnabled(false);
    uint16_t remaining=0;uint8_t status;
    do {status=mwMusicHardware.readPort(MPU_STATUS_COMMAND_PORT);--remaining;}
    while (remaining && (status&MPU_WRITE_BUSY));
    if (!remaining) goto failed;
    mwMusicHardware.writePort(MPU_STATUS_COMMAND_PORT,MPU_RESET_COMMAND);
    remaining=0;
    do {status=mwMusicHardware.readPort(MPU_STATUS_COMMAND_PORT);--remaining;}
    while (remaining && (status&MPU_WRITE_BUSY));
    if (!remaining) goto failed;
    mwMusicHardware.writePort(MPU_STATUS_COMMAND_PORT,0);
    remaining=0;
    do {status=mwMusicHardware.readPort(MPU_STATUS_COMMAND_PORT);--remaining;}
    while (remaining && (status&MPU_INPUT_EMPTY));
    if (!remaining || mwMusicHardware.readPort(MPU_DATA_PORT)!=MPU_ACK) goto failed;
    mwMusicHardware.setInterruptsEnabled(true);return 1;
failed:
    mwMusicHardware.setInterruptsEnabled(true);return 0;
}
/* MW_MAIN0F41:0946. Saved vector is a native identity, not a host pointer. */
void MwInitializeMpuUartBackend(void)
{
    if (!mwMusicHardware.readInterruptVector || !mwMusicHardware.writeInterruptVector) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !mwMusicHardware.readInterruptVector || !mwMusicHardware.writeInterruptVector");
    mwMusicHardware.setInterruptsEnabled(false);
    mwMusicHardware.writePort(MPU_STATUS_COMMAND_PORT,MPU_UART_COMMAND);
    mpuQueueWriteIndex=mpuQueueReadIndex=mpuQueueByteCount=0;
    mpuPreviousIrq=mwMusicHardware.readInterruptVector(MPU_IRQ_VECTOR);
    mwMusicHardware.writeInterruptVector(MPU_IRQ_VECTOR,MPU_RECEIVE_HANDLER);
    for (uint16_t reads=10;reads;--reads) mwMusicHardware.readPort(MPU_DATA_PORT);
    mwMusicHardware.writePort(PIC_COMMAND_PORT,PIC_END_OF_INTERRUPT);
    mwMusicHardware.writePort(PIC_MASK_PORT,0);
    mwMusicHardware.setInterruptsEnabled(true);
}
/* MW_MAIN0F41:098A. Waits for IRQ0 progress, never drains bytes synchronously. */
void MwDrainAndShutdownMpuBackend(void)
{
    while (mpuQueueByteCount) mwMusicHardware.waitForInterrupt();
    mwMusicHardware.setInterruptsEnabled(false);
    mwMusicHardware.writeInterruptVector(MPU_IRQ_VECTOR,mpuPreviousIrq);
    mwMusicHardware.setInterruptsEnabled(true);
}
/* MW_MAIN0F41:09A4. Producer's STI does not restore the incoming IF. */
uint16_t MwQueueMpuByte(uint16_t midiWord)
{
    while (mpuQueueByteCount>=MIDI_MPU_QUEUE_CAPACITY) mwMusicHardware.waitForInterrupt();
    mwMusicHardware.setInterruptsEnabled(false);
    if (mpuQueueWriteIndex>=MIDI_MPU_QUEUE_CAPACITY) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: mpuQueueWriteIndex>=MIDI_MPU_QUEUE_CAPACITY"); /* Unrepresented corrupt-index alias. */
    mpuTransmitQueue[mpuQueueWriteIndex]=(uint8_t)midiWord;
    if (++mpuQueueWriteIndex>=MIDI_MPU_QUEUE_CAPACITY) mpuQueueWriteIndex=0;
    ++mpuQueueByteCount;
    mwMusicHardware.setInterruptsEnabled(true);return 1;
}
/* MW_MAIN0F41:09D0. Exactly one byte per ready fast callback. */
void MwTickMpuTransmitQueue(void)
{
    if (!mpuQueueByteCount || (mwMusicHardware.readPort(MPU_STATUS_COMMAND_PORT)&MPU_WRITE_BUSY)) return;
    if (mpuQueueReadIndex>=MIDI_MPU_QUEUE_CAPACITY) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: mpuQueueReadIndex>=MIDI_MPU_QUEUE_CAPACITY");
    mwMusicHardware.writePort(MPU_DATA_PORT,mpuTransmitQueue[mpuQueueReadIndex]);
    if (++mpuQueueReadIndex>=MIDI_MPU_QUEUE_CAPACITY) mpuQueueReadIndex=0;
    --mpuQueueByteCount;
}
/* MW_MAIN0F41:0A01. CPU entry/IRET is owned by the host interrupt boundary. */
void MwMpuReceiveInterrupt(void)
{
    mwMusicHardware.readPort(MPU_DATA_PORT);
    mwMusicHardware.writePort(PIC_COMMAND_PORT,PIC_END_OF_INTERRUPT);
}
