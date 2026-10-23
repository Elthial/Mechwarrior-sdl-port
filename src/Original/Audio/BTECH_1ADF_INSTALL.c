#include "mechwarrior/port_diagnostics.h"
#include "mechwarrior/combat_sound.h"
#include "mechwarrior/combat_startup.h"
#include "mechwarrior/combat_adlib.h"
#include "mechwarrior/combat_mpu.h"
#include <stdlib.h>

enum { MPU_WRITE_BUSY=0x40,MPU_INPUT_EMPTY=0x80,MPU_ACK=0xFE,
    MPU_RESET=0xFF,MPU_VERSION=0xAC,MPU_PROBE_READY_POLLS=0x1F40,
    MPU_PROBE_ACK_POLLS=0xFFFF,MPU_PROBE_IRQ_POLLS=0x4E20,
    MPU_IRQ_VECTOR=0x0A,MPU_IRQ_MASK=4 };

/*1ADF:0032. A zero request means all detected backends. Failed device
 * initializers do not remove bits or restore the previously stored callbacks. */
uint16_t BtechSelectAndInstallAudioBackends(uint16_t requestedBackendMask)
{
    if (!requestedBackendMask) requestedBackendMask=UINT16_MAX;
    requestedBackendMask&=BtechProbeAudioBackendMask();
    if (requestedBackendMask&(COMBAT_SOUND_SPEAKER|COMBAT_SOUND_TANDY))
        audioMusicCallback=BtechDispatchToneMidiMessage;
    if ((requestedBackendMask&COMBAT_SOUND_ADLIB) && BtechInitializeAdlibMusicBackend())
        audioMusicCallback=BtechDispatchAdlibMidiMessage;
    audioTickCallback=BtechTickSoftwareMusic;
    if ((requestedBackendMask&COMBAT_SOUND_MPU) && BtechInitializeRolandMpuBackend())
    {audioMusicCallback=BtechAudioMusicNoOp;audioTickCallback=BtechAudioTimerNoOp;}
    selectedAudioBackendMask=requestedBackendMask;
    return requestedBackendMask;
}

/*1ADF:00BE. Probe all devices, even when the eventual request is speaker-only. */
uint16_t BtechProbeAudioBackendMask(void)
{
    uint16_t available=0;
    if (!BtechProbeAdlibTimerStatus()) available|=COMBAT_SOUND_ADLIB;
    if (BtechProbeMpuWithTemporaryInterrupt()) available|=COMBAT_SOUND_MPU;
    available|=BtechProbePlatformAudioMask();
    return available;
}

/*1ADF:109D. Busy timeout retains IF; after a command, both ACK outcomes
 * enable interrupts. Non-ACK input consumes a second byte without dispatch. */
uint16_t BtechSendMpuProbeCommandAndWaitAck(uint8_t command)
{
    if (!btechHostReadPort || !btechHostWritePort || !btechHostSetInterrupts) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !btechHostReadPort || !btechHostWritePort || !btechHostSetInterrupts");
    uint16_t statusPort=mpuProbeStatusPort;
    for (uint16_t ready=MPU_PROBE_READY_POLLS;ready;--ready)
    {
        if (btechHostReadPort(statusPort)&MPU_WRITE_BUSY) continue;
        btechHostSetInterrupts(0);
        btechHostWritePort(mpuProbeCommandPort,command);
        for (uint16_t acknowledgement=MPU_PROBE_ACK_POLLS;acknowledgement;--acknowledgement)
        {
            if (btechHostReadPort(mpuProbeStatusPort)&MPU_INPUT_EMPTY) continue;
            uint8_t response=btechHostReadPort(mpuProbeDataPort);
            if (response==MPU_ACK)
            {
                btechHostSetInterrupts(1);
                return ((uint16_t)command<<8)|response;
            }
            btechHostReadPort(mpuProbeDataPort);
        }
        btechHostSetInterrupts(1);return 0;
    }
    return 0;
}

/*1ADF:1010. Save IF after resets. A missing input IRQ does not turn an
 * acknowledged version command into failure. Final PIC write masks IRQ2. */
uint16_t BtechProbeMpuWithTemporaryInterrupt(void)
{
    BtechSendMpuProbeCommandAndWaitAck(MPU_RESET);
    uint16_t response=BtechSendMpuProbeCommandAndWaitAck(MPU_RESET);
    if (!(uint8_t)response) return 0;
    btechHostReadPort(mpuProbeDataPort);
    uint8_t savedInterrupts=btechHostSetInterrupts(1);
    if (!btechHostGetInterruptVector || !btechHostSetInterruptVector) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !btechHostGetInterruptVector || !btechHostSetInterruptVector");
    mpuProbePreviousIrq=btechHostGetInterruptVector(MPU_IRQ_VECTOR);
    btechHostSetInterruptVector(MPU_IRQ_VECTOR,0x1ADF1082);
    btechHostWritePort(PIC_MASK_PORT,btechHostReadPort(PIC_MASK_PORT)&~MPU_IRQ_MASK);
    mpuProbeReceivedByteCount=0;
    response=BtechSendMpuProbeCommandAndWaitAck(MPU_VERSION);
    if (response)
        for (uint16_t remaining=MPU_PROBE_IRQ_POLLS;remaining;--remaining)
        {
            /* Interrupt delivery only; the original loop reads this WORD live. */
            if (btechHostPollInterrupts) btechHostPollInterrupts();
            if (mpuProbeReceivedByteCount) break;
        }
    btechHostWritePort(PIC_MASK_PORT,btechHostReadPort(PIC_MASK_PORT)|MPU_IRQ_MASK);
    btechHostSetInterruptVector(MPU_IRQ_VECTOR,mpuProbePreviousIrq);
    btechHostSetInterrupts(savedInterrupts);
    return (uint8_t)response;
}

/*1ADF:1082. Hardware adapter must deliver this version only with DS304E.
 * The original neither saves DS nor selects its own data segment. */
void BtechMpuProbeInterrupt(void)
{
    if (!btechHostReadPort || !btechHostWritePort) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !btechHostReadPort || !btechHostWritePort");
    if (!(btechHostReadPort(mpuProbeStatusPort)&MPU_INPUT_EMPTY))
    {
        btechHostReadPort(mpuProbeDataPort);
        ++mpuProbeReceivedByteCount;
    }
    btechHostWritePort(PIC_COMMAND_PORT,PIC_END_OF_INTERRUPT);
}
