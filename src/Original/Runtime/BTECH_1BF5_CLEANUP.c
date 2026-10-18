#include "mechwarrior/combat_startup.h"
#include "mechwarrior/input_hardware.h"
#include "mechwarrior/combat_platform.h"
#include <stdlib.h>

/*1BF5:00D5. Fatal DE52 calls this AFTER text restore; normal1337 does the
 * reverse. Keep the original caller ordering when those roots are linked. */
void BtechShutdownPlatformHandlers(void)
{
    BtechRestoreKeyboardAndTickVectors();
    BtechShutdownMouseDriver();
    BtechRestoreTimerScheduler();
    BtechDeactivateDivideErrorHook();
}

/*1BF5:00EA. FF means no saved adapter, not a request to force mode03. */
void BtechRestoreTextVideoMode(void)
{
    if (videoAdapter!=0xFF) BtechSetBiosTextModeAndAdapterBits(videoAdapter);
}

/*1BF5:0287. Native BIOS equipment BYTE has two ordered read/modify/writes. */
void BtechSetBiosTextModeAndAdapterBits(uint16_t displayBits)
{
    if (!btechHostReadBiosByte || !btechHostWriteBiosByte || !btechHostSetVideoMode) abort();
    btechHostWriteBiosByte(BIOS_VIDEO_EQUIPMENT,btechHostReadBiosByte(BIOS_VIDEO_EQUIPMENT)&0xCF);
    btechHostWriteBiosByte(BIOS_VIDEO_EQUIPMENT,btechHostReadBiosByte(BIOS_VIDEO_EQUIPMENT)|(uint8_t)(displayBits<<4));
    btechHostSetVideoMode(3);
}

/*1BF5:2C8D. Flag clears before discarding queued BIOS keys and restoring IVT. */
void BtechRestoreKeyboardAndTickVectors(void)
{
    if (!keyboardHandlersInstalled) return;
    keyboardHandlersInstalled=0;
    if (!btechHostReadBiosWord || !btechHostWriteBiosWord || !btechHostSetInterruptVector) abort();
    btechHostWriteBiosWord(BIOS_KEYBOARD_HEAD,btechHostReadBiosWord(BIOS_KEYBOARD_TAIL));
    btechHostSetInterruptVector(KEYBOARD_INTERRUPT,savedKeyboardVector);
    btechHostSetInterruptVector(BIOS_TICK_INTERRUPT,savedBiosTickVector);
}

/*1BF5:2428. INT33 reset is a hardware boundary, not a game-input callback. */
void BtechShutdownMouseDriver(void)
{
    if (!mouseDriverInstalled) return;
    mouseDriverInstalled=0;
    if (!btechHostResetMouse) abort();
    btechHostResetMouse();
}

/*1BF5:25AB. Preserve literal PIC mask28 and unconditional STI on active path. */
void BtechRestoreTimerScheduler(void)
{
    if (!runtimeDelayActive) return;
    if (!btechHostSetInterrupts || !btechHostWritePort || !btechHostSetInterruptVector) abort();
    btechHostSetInterrupts(0);
    btechHostWritePort(PIT_CONTROL_PORT,PIT_MODE_3_LOW_HIGH);
    btechHostWritePort(PIT_CHANNEL_ZERO_PORT,0);btechHostWritePort(PIT_CHANNEL_ZERO_PORT,0);
    btechHostWritePort(PIC_MASK_PORT,0x28);
    btechHostSetInterruptVector(8,savedTimerVector);
    runtimeDelayActive=0;
    btechHostSetInterrupts(1);
}

/*1BF5:1F84. The shipped26 A1 instructions READ INT00; they do not restore it.
 * Keep that native defect and the active-path STI, not an idealized unhook. */
void BtechDeactivateDivideErrorHook(void)
{
    if (!divideErrorHookMarkedActive) return;
    divideErrorHookMarkedActive=0;
    if (!btechHostSetInterrupts || !btechHostGetInterruptVector) abort();
    btechHostSetInterrupts(0);
    (void)savedDivideErrorVector;
    (void)btechHostGetInterruptVector(0);
    btechHostSetInterrupts(1);
}
