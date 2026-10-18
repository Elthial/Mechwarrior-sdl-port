#include "mechwarrior/combat_import.h"
#include "mechwarrior/combat_platform.h"
#include "mechwarrior/combat_input.h"
#include <stdlib.h>

/*1BF5:1EB6. Sentinel returns BX paragraphs even on DOS success. Ordinary
 * quotient truncates BEFORE rounding. Host pointers replace selector:0000. */
BtechDosAllocation BtechAllocateDosMemoryBlock(uint32_t bytes)
{
    if (!btechHostAllocateParagraphs) abort();
    uint16_t paragraphs=bytes==UINT32_MAX?UINT16_MAX:(uint16_t)((bytes>>4)+((bytes&0x0F)!=0));
    uint16_t largestFreeParagraphs=0;
    uint8_t *allocation=btechHostAllocateParagraphs(paragraphs,&largestFreeParagraphs);
    BtechDosAllocation result={allocation,(uint32_t)largestFreeParagraphs<<4};
    return result;
}

/*1BF5:1F10. DOS49 boundary; native reads the selector WORD at BP+08 only,
 * preserving ES. BP+06 offset is unused. A typed allocation replaces that
 * unobserved address transport, not a one-WORD native signature. */
void BtechFreeDosMemorySelector(uint8_t *allocation)
{
    if (!btechHostFreeDosAllocation) abort();
    btechHostFreeDosAllocation(allocation);
}

/*1BF5:218B. PUSHF/POPF, not an unconditional STI; axis result complements
 * only AL. Delay loops have no game effects; port read order is retained. */
uint16_t BtechMeasureJoystickAxisTiming(uint8_t axisMask)
{
    if (!btechHostReadPort || !btechHostWritePort || !btechHostSetInterrupts || !btechHostDelayLoops) abort();
    uint8_t previousInterrupts=btechHostSetInterrupts(0);
    uint16_t count=JOYSTICK_AXIS_TIMING_LIMIT;
    uint8_t gamePort;
    do {btechHostDelayLoops(joystickTimingLoopCount);gamePort=btechHostReadPort(GAME_PORT);--count;} while ((gamePort&axisMask) && count);
    if (count)
    {
        btechHostWritePort(GAME_PORT,gamePort);
        btechHostReadPort(PIT_CHANNEL_ZERO_PORT);btechHostReadPort(PIT_CHANNEL_ZERO_PORT);
        count=JOYSTICK_AXIS_TIMING_LIMIT;
        do {btechHostDelayLoops(joystickTimingLoopCount);--count;gamePort=btechHostReadPort(GAME_PORT);} while ((gamePort&axisMask) && count);
    }
    btechHostSetInterrupts(previousInterrupts);
    return (uint16_t)((count&JOYSTICK_TIMING_HIGH_BYTE_MASK)|(uint8_t)~count);
}

/*1BF5:21FF. PIT/game-port arithmetic and all four reciprocal writes stay
 * original; host supplies raw hardware replies, never calibrated game values. */
void BtechCalibrateJoystickAxisPairs(void)
{
    if (!btechHostReadPort || !btechHostWritePort || !btechHostSetInterrupts || !btechHostDelayLoops) abort();
    btechHostSetInterrupts(0);btechHostWritePort(PIT_CONTROL_PORT,PIT_CHANNEL_ZERO_LATCH);
    uint16_t firstCounter=btechHostReadPort(PIT_CHANNEL_ZERO_PORT);
    firstCounter|=(uint16_t)btechHostReadPort(PIT_CHANNEL_ZERO_PORT)<<8;
    for (uint16_t sample=0;sample<JOYSTICK_CALIBRATION_PORT_SAMPLES;++sample) btechHostReadPort(GAME_PORT);
    btechHostWritePort(PIT_CONTROL_PORT,PIT_CHANNEL_ZERO_MODE3_LATCH);
    uint16_t lastCounter=btechHostReadPort(PIT_CHANNEL_ZERO_PORT);
    lastCounter|=(uint16_t)btechHostReadPort(PIT_CHANNEL_ZERO_PORT)<<8;
    btechHostSetInterrupts(1);
    uint16_t elapsed=firstCounter-lastCounter;
    if (!elapsed) abort(); /* Native DIV fault, after hardware reads. */
    joystickTimingLoopCount=JOYSTICK_LOOP_CALIBRATION_NUMERATOR/elapsed;
    if (!joystickTimingLoopCount) joystickTimingLoopCount=1;
    uint8_t lastOutput=0;
    for (uint16_t axis=0;axis<JOYSTICK_AXIS_COUNT;++axis)
    {
        uint16_t timing=BtechMeasureJoystickAxisTiming((uint8_t)(1<<axis));
        joystickAxisCentres[axis]=(uint8_t)timing;
        if (!(axis&JOYSTICK_AXIS_PAIR_POSITION_MASK)) joystickPairInstalled[axis/JOYSTICK_AXES_PER_PAIR]=timing!=JOYSTICK_AXIS_TIMING_LIMIT;
        lastOutput=(uint8_t)timing;
        if (timing) {joystickAxisReciprocals[axis]=JOYSTICK_AXIS_RECIPROCAL_NUMERATOR/timing;lastOutput=(uint8_t)joystickAxisReciprocals[axis];}
    }
    btechHostWritePort(GAME_PORT,lastOutput);
    btechHostDelayLoops(JOYSTICK_PRESENCE_DELAY_LOOPS);
    uint8_t axisChargeState=btechHostReadPort(GAME_PORT);
    if (!(axisChargeState&GAMEPORT_JOYSTICK_A_AXES_MASK)) joystickPairInstalled[JOYSTICK_A]=0;
    if (!(axisChargeState&GAMEPORT_JOYSTICK_B_AXES_MASK)) joystickPairInstalled[JOYSTICK_B]=0;
}
