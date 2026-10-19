#include "mechwarrior/audio_devices.h"
#include <opl3.h>

enum { OPL_TIMER_ONE=2,OPL_TIMER_TWO=3,OPL_TIMER_CONTROL=4,
    OPL_TIMER_IRQ_RESET=0x80,OPL_TIMER_ONE_MASK=0x40,OPL_TIMER_TWO_MASK=0x20,
    OPL_TIMER_ONE_START=1,OPL_TIMER_TWO_START=2,OPL_TIMER_IRQ=0x80,
    OPL_TIMER_ONE_OVERFLOW=0x40,OPL_TIMER_TWO_OVERFLOW=0x20,
    OPL_TIMER_ONE_PERIOD_US=80,OPL_TIMER_TWO_PERIOD_US=320,OPL2_FIXED_STATUS_BITS=6 };
static opl3_chip chip;
static uint8_t address,timerValue[2],timerControl,timerOverflow;
static double busTime,timerDeadline[2];

void MwAdlibReset(uint32_t sampleRate)
{
    OPL3_Reset(&chip,sampleRate); /* Power-on OPL2-compatible mode, no OPL3 extensions. */
    address=timerValue[0]=timerValue[1]=timerControl=timerOverflow=0;
    busTime=timerDeadline[0]=timerDeadline[1]=0;
}
static void AdvanceTimers(double microseconds)
{
    if (microseconds>busTime) busTime=microseconds;
    if ((timerControl&OPL_TIMER_ONE_START) && busTime>=timerDeadline[0]) timerOverflow|=OPL_TIMER_ONE_OVERFLOW;
    if ((timerControl&OPL_TIMER_TWO_START) && busTime>=timerDeadline[1]) timerOverflow|=OPL_TIMER_TWO_OVERFLOW;
}
void MwAdlibWritePort(bool dataPort,uint8_t value,double microseconds)
{
    AdvanceTimers(microseconds);
    if (!dataPort) {address=value;return;}
    if (address==OPL_TIMER_ONE || address==OPL_TIMER_TWO) timerValue[address-OPL_TIMER_ONE]=value;
    else if (address==OPL_TIMER_CONTROL)
    {
        if (value&OPL_TIMER_IRQ_RESET) timerOverflow=0;
        else
        {
            if ((value&OPL_TIMER_ONE_START) && !(timerControl&OPL_TIMER_ONE_START))
                timerDeadline[0]=busTime+(256-timerValue[0])*OPL_TIMER_ONE_PERIOD_US;
            if ((value&OPL_TIMER_TWO_START) && !(timerControl&OPL_TIMER_TWO_START))
                timerDeadline[1]=busTime+(256-timerValue[1])*OPL_TIMER_TWO_PERIOD_US;
            timerControl=value;
            timerOverflow&=~value; /* Masking a timer clears its pending flag. */
        }
    }
    else OPL3_WriteReg(&chip,address,value);
}
uint8_t MwAdlibReadStatus(double microseconds)
{
    /* Original calibration polls with interrupts disabled. ISA reads consume
     * bus time even while no IRQ is delivered. This 1 us access model is local
     * to the card timers; it never accelerates gameplay or the PIT timeline. */
    AdvanceTimers(microseconds);busTime+=1;AdvanceTimers(busTime);
    uint8_t status=timerOverflow&~timerControl&(OPL_TIMER_ONE_OVERFLOW|OPL_TIMER_TWO_OVERFLOW);
    /* YM3812 leaves bits 1/2 high. Combat returns this status from its OPL
     * initializer and tests the complete AX; an OPL3-style zero rejects it. */
    return (status?(status|OPL_TIMER_IRQ):0)|OPL2_FIXED_STATUS_BITS;
}
void MwAdlibRender(float *stereo,uint32_t frames)
{
    for (uint32_t frame=0;frame<frames;++frame)
    {
        int16_t output[2];OPL3_GenerateResampled(&chip,output);
        stereo[frame*2]=(float)output[0]/32768.0f;
        /* YM3812 AdLib has one DAC. Do not expose the core's YMF262 L/R
         * sample staggering as invented stereo on the original mono card. */
        stereo[frame*2+1]=stereo[frame*2];
    }
}
