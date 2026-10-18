#include "../SDL/SDLBackend.h"
#include "mechwarrior/audio_devices.h"

enum { MPU_INPUT_EMPTY=0x80,MPU_OUTPUT_BUSY=0x40,MPU_ACK=0xFE,
    MPU_RESET_COMMAND=0xFF,MPU_UART_COMMAND=0x3F,
    MIDI_BITS_PER_BYTE=10,MIDI_BAUD=31250,MPU_REPLY_CAPACITY=16 };
static uint8_t replies[MPU_REPLY_CAPACITY];
static unsigned replyRead,replyCount;
static bool uartMode;
static double transmitReady;

void MwMpuReset(void)
{replyRead=replyCount=0;uartMode=false;transmitReady=0;}
void MwMpuWritePort(uint16_t port,uint8_t value)
{
    if (port==MPU_STATUS_COMMAND_PORT)
    {
        if (value==MPU_RESET_COMMAND) MwMpuReset();
        else if (value==MPU_UART_COMMAND) uartMode=true;
        else if (value!=0) MwSdlFail("Unimplemented intelligent-mode MPU command");
        if (replyCount==MPU_REPLY_CAPACITY) MwSdlFail("MPU response FIFO overflow");
        replies[(replyRead+replyCount++)%MPU_REPLY_CAPACITY]=MPU_ACK;
        return;
    }
    if (!uartMode) MwSdlFail("MPU MIDI data outside UART mode");
    transmitReady=MwSdlHardwareTime()+(double)HOST_PIT_CLOCK_HZ*MIDI_BITS_PER_BYTE/MIDI_BAUD;
    MwMt32ReceiveByte(value);
}
uint8_t MwMpuReadPort(uint16_t port)
{
    if (port==MPU_STATUS_COMMAND_PORT)
        return (replyCount?0:MPU_INPUT_EMPTY)|(MwSdlHardwareTime()<transmitReady?MPU_OUTPUT_BUSY:0);
    if (!replyCount) return UINT8_MAX;
    uint8_t value=replies[replyRead];replyRead=(replyRead+1)%MPU_REPLY_CAPACITY;--replyCount;
    return value;
}
