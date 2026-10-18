#include "mechwarrior/combat_startup.h"
#include "mechwarrior/combat_input.h"
#include "mechwarrior/combat_video.h"
#include <stdlib.h>

/*1BF5:24D3. First clear occupancy bit, not first NULL callback. Publish the
 * timing/callback BEFORE CLI; only the mask update is protected natively. */
uint16_t BtechRegisterTimerCallback(BtechTimerCallback callback,uint16_t period)
{
    if (!runtimeDelayActive || schedulerActiveSlots==UINT16_MAX) return 0;
    uint16_t slot=0,slotMask=1,pendingSlots=schedulerActiveSlots;
    while (pendingSlots&1) {++slot;slotMask<<=1;pendingSlots>>=1;}
    schedulerTimings[slot].period=period;
    schedulerTimings[slot].ticksRemaining=period;
    schedulerCallbacks[slot]=callback;
    if (!btechHostSetInterrupts) abort();
    btechHostSetInterrupts(0);
    schedulerActiveSlots|=slotMask;
    btechHostSetInterrupts(1);
    return slot+1;
}

/*1BF5:251D. Native DEC/TEST CL,F0 examines CL only; handles with nonzero
 * high BYTE alias1..16. Removal leaves callback and both timing WORDs stale. */
uint16_t BtechUnregisterTimerCallback(uint16_t callbackHandle)
{
    --callbackHandle;
    if ((uint8_t)callbackHandle&COMBAT_SCHEDULER_INVALID_SLOT_BITS) return 0;
    if (!btechHostSetInterrupts) abort();
    btechHostSetInterrupts(0);
    schedulerActiveSlots&=(uint16_t)~(1u<<(uint8_t)callbackHandle);
    btechHostSetInterrupts(1);
    return 1;
}

/*1BF5:25E2. IRQ0 selects304E, snapshots occupancy once, scans low-to-high.
 * Typed callbacks replace FAR pairs, not native scheduling decisions. Bound
 * callbacks must preserve the original scheduler's SI/BP/DS register owners. */
void BtechTimerSchedulerInterrupt(void)
{
    uint16_t remaining=runtimeDelayRemaining-1;
    runtimeDelayRemaining=remaining^((remaining&NATIVE_WORD_SIGN_BIT)?UINT16_MAX:0);
    uint16_t pendingSlots=schedulerActiveSlots;
    for (uint16_t slot=0;pendingSlots;++slot,pendingSlots>>=1)
    {
        if (!(pendingSlots&1)) continue;
        remaining=schedulerTimings[slot].ticksRemaining-1;
        if (!remaining)
        {
            if (!schedulerCallbacks[slot]) abort(); /* Native NULL FAR fault. */
            schedulerCallbacks[slot]();
            remaining=schedulerTimings[slot].period; /* Live AFTER callback. */
        }
        schedulerTimings[slot].ticksRemaining=remaining;
    }
    remaining=schedulerCountdown-1;
    if (remaining)
    {
        schedulerCountdown=remaining;
        if (!btechHostWritePort) abort();
        btechHostWritePort(PIC_COMMAND_PORT,PIC_END_OF_INTERRUPT);
    }
    else
    {
        schedulerCountdown=schedulerPeriod;
        if (!btechHostChainTimerInterrupt) abort();
        btechHostChainTimerInterrupt(savedTimerVector); /* Old handler owns EOI. */
    }
}

/*1BF5:3449. Compatibility is latched; a later mismatch does not clear it. */
int16_t BtechProbePcjrTandyRomSignatures(void)
{
    if (!btechHostReadRomByte) abort();
    if (btechHostReadRomByte(BIOS_ROM_SELECTOR,BIOS_MODEL_OFFSET)==BIOS_PCJR_MODEL_BYTE && btechHostReadRomByte(BIOS_ROM_SELECTOR,TANDY_ROM_SIGNATURE_OFFSET)==TANDY_ROM_SIGNATURE_BYTE)
        pcjrTandyCompatible=1;
    return (int8_t)pcjrTandyCompatible;
}

/*1BF5:2BC8. Repeated calls still adjust BIOS shift state. The compatibility
 * branch patches two CS immediates and four scan-table bytes in native order. */
void BtechInstallKeyboardAndTickHandlers(uint16_t installBiosTick)
{
    if (!btechHostGetInterruptVector || !btechHostSetInterruptVector
        || !btechHostReadBiosByte || !btechHostWriteBiosByte) abort();
    if (!keyboardHandlersInstalled)
    {
        savedKeyboardVector=btechHostGetInterruptVector(KEYBOARD_INTERRUPT);
        savedBiosTickVector=btechHostGetInterruptVector(BIOS_TICK_INTERRUPT);
        btechHostSetInterruptVector(KEYBOARD_INTERRUPT,BTECH_KEYBOARD_HANDLER_VECTOR);
        if (installBiosTick) btechHostSetInterruptVector(BIOS_TICK_INTERRUPT,BTECH_BIOS_TICK_HANDLER_VECTOR);
        keyboardBiosIntercept=0;
        int16_t compatibility=BtechProbePcjrTandyRomSignatures();
        if (compatibility)
        {
            if (!btechHostQueryKeyboardIntercept) abort();
            /* NEG AX leavesFFFF when3449 returns1; it is not BIOS C0. */
            if (btechHostQueryKeyboardIntercept((uint16_t)-compatibility)
                && btechHostReadBiosByte(BIOS_KEYBOARD_COMPATIBILITY_STATE)==KEYBOARD_COMPATIBILITY_INTERCEPT_STATE) keyboardBiosIntercept=1;
            else
            {
                keyboardScanMask=KEYBOARD_ATTRIBUTE_CLEAR_HELD;keyboardExtendedScanMask=KEYBOARD_ATTRIBUTE_CLEAR_HELD;
                keyboardScanTranslations[COMBAT_KEY_GRAVE]=keyboardScanTranslations[COMBAT_KEY_UP];
                keyboardScanTranslations[COMBAT_KEY_BACKSLASH]=keyboardScanTranslations[COMBAT_KEY_LEFT];
                keyboardScanTranslations[COMBAT_KEY_KEYPAD_PLUS]=keyboardScanTranslations[COMBAT_KEY_RIGHT];
                keyboardScanTranslations[COMBAT_KEY_KEYPAD_MINUS]=keyboardScanTranslations[COMBAT_KEY_DOWN];
            }
        }
        keyboardHandlersInstalled=1;
    }
    btechHostWriteBiosByte(BIOS_KEYBOARD_SHIFT_FLAGS,btechHostReadBiosByte(BIOS_KEYBOARD_SHIFT_FLAGS)&BIOS_SHIFT_CLEAR_NUM_LOCK);
    if (keyboardCapsLockRequested) btechHostWriteBiosByte(BIOS_KEYBOARD_SHIFT_FLAGS,btechHostReadBiosByte(BIOS_KEYBOARD_SHIFT_FLAGS)|BIOS_SHIFT_CAPS_LOCK);
}

/*1BF5:1F20. Saved offset/selector are captured before CLI; reinstallation
 * overwrites them even if INT00 already points at this handler. */
void BtechInstallDivideErrorHandler(void)
{
    if (!btechHostGetInterruptVector || !btechHostSetInterruptVector || !btechHostSetInterrupts) abort();
    divideErrorHookMarkedActive=1;
    savedDivideErrorVector=btechHostGetInterruptVector(DIVIDE_ERROR_INTERRUPT);
    btechHostSetInterrupts(0);btechHostSetInterruptVector(DIVIDE_ERROR_INTERRUPT,BTECH_DIVIDE_HANDLER_VECTOR);btechHostSetInterrupts(1);
}

/*1BF5:2540. JG is SIGNED: high-bit periods are accepted. Zero/positive>FF
 * still probe ROM/save vector before returning. Never impose an unsigned255
 * check or a guard that changes those preceding effects. */
void BtechInstallTimerScheduler(uint16_t period)
{
    if (runtimeDelayActive) return;
    schedulerActiveSlots=0;
    BtechProbePcjrTandyRomSignatures();
    if (!btechHostGetInterruptVector) abort();
    savedTimerVector=btechHostGetInterruptVector(TIMER_INTERRUPT);
    if ((int16_t)period>COMBAT_SCHEDULER_MAX_POSITIVE_PERIOD || !period) return;
    schedulerPeriod=period;schedulerCountdown=period;
    pitChannel0DivisorShadow=UINT16_MAX/period;
    if (!btechHostSetInterrupts || !btechHostWritePort || !btechHostSetInterruptVector) abort();
    btechHostSetInterrupts(0);
    btechHostWritePort(PIT_CONTROL_PORT,PIT_MODE_3_LOW_HIGH);
    btechHostWritePort(PIT_CHANNEL_ZERO_PORT,(uint8_t)pitChannel0DivisorShadow);
    btechHostWritePort(PIT_CHANNEL_ZERO_PORT,(uint8_t)(pitChannel0DivisorShadow>>8));
    btechHostWritePort(PIC_MASK_PORT,COMBAT_PIC_DEVICE_MASK);
    btechHostSetInterruptVector(TIMER_INTERRUPT,BTECH_TIMER_HANDLER_VECTOR);
    btechHostSetInterrupts(1);runtimeDelayActive=1;
}
