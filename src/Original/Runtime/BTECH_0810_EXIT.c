#include "mechwarrior/port_diagnostics.h"
#include "mechwarrior/combat_runtime.h"
#include "mechwarrior/combat_startup.h"
#include <stdlib.h>
#include <string.h>

/*0810:E1CD. Near and FAR pointers become typed references only after resolving
 * the authored tables. Their separate reverse walks remain native boundaries. */
void BtechRunNearCallbacksReverse(BtechRuntimeCallback *callbacks,uint16_t count)
{
    while (count) {--count;if (callbacks[count]) callbacks[count]();}
}

/*0810:E1DC. Empty authored FAR ranges are passed with count0, not fabricated
 * callbacks. The pointer representation no longer dictates a four-byte step. */
void BtechRunFarCallbacksReverse(BtechRuntimeCallback *callbacks,uint16_t count)
{
    while (count) {--count;if (callbacks[count]) callbacks[count]();}
}

/*0810:F48A. A query forFFFF matches the sentinel's actual text. Other unknown
 * codes stop there and do not walk into the following runtime storage. */
const char *BtechFindRuntimeMessageByCode(uint16_t messageCode)
{
    for (uint16_t message=0;message<COMBAT_RUNTIME_MESSAGES;++message)
    {
        if (runtimeMessages[message].code==messageCode) return runtimeMessages[message].text;
        if (runtimeMessages[message].code==UINT16_MAX) return NULL;
    }
    /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Unsupported table without its native terminator."); /* Unsupported table without its native terminator. */
}

/*0810:F4B5. Only DOS handle2 output crosses the host boundary. */
void BtechWriteRuntimeMessageToStderr(uint16_t messageCode)
{
    const char *text=BtechFindRuntimeMessageByCode(messageCode);
    if (!text) return;
    if (!btechHostWriteHandle) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !btechHostWriteHandle");
    btechHostWriteHandle(COMBAT_STDERR_HANDLE,text,(uint16_t)strlen(text));
}

/*0810:F246. Callback can change the following message; do not snapshot it. */
void BtechReportRuntimeFatalState(void)
{
    BtechWriteRuntimeMessageToStderr(COMBAT_RUNTIME_MESSAGE_NEWLINE);
    if (runtimeDiagnosticCallback) runtimeDiagnosticCallback();
    BtechWriteRuntimeMessageToStderr(COMBAT_RUNTIME_MESSAGE_PREFIX);
}

/*0810:F26C. Good checksum leaves the LAST BYTE in AL, not bool false. The
 * four fallback WORDs are serialized here solely for this bytewise consumer. */
uint16_t BtechCheckRuntimeDataChecksum(void)
{
    uint8_t checksum=0,lastByte=0;
    for (uint16_t word=0;word<4;++word)
    {
        checksum^=(uint8_t)runtimePrefix.ambientFallbackWords[word];
        checksum^=(uint8_t)(runtimePrefix.ambientFallbackWords[word]>>8);
    }
    for (uint16_t byte=0;byte<sizeof(runtimePrefix.copyrightBytes);++byte)
    {
        lastByte=runtimePrefix.copyrightBytes[byte];checksum^=lastByte;
    }
    checksum^=COMBAT_RUNTIME_CHECKSUM_XOR;
    if (!checksum) return lastByte;
    BtechReportRuntimeFatalState();
    BtechWriteRuntimeMessageToStderr(COMBAT_RUNTIME_MESSAGE_NULL_ASSIGNMENT);
    return 1;
}

/*0810:E1A0. Distinct from1BF5:1F84: this CRT hook REALLY restores INT00.
 * The optional FAR device callback receives native BX=2. */
void BtechRestoreRuntimeVectors(void)
{
    /*4DF0 is the selector HALF of4DEE, not another enable-flag owner. */
    if (runtimeDeviceCleanup) runtimeDeviceCleanup(COMBAT_RUNTIME_DEVICE_EXIT);
    if (!btechHostSetInterruptVector) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !btechHostSetInterruptVector");
    btechHostSetInterruptVector(0,runtimeSavedDivideVector);
    if (runtimeExtraVectorActive) btechHostSetInterruptVector(runtimeExtraInterrupt,runtimeSavedExtraVector);
}

/*0810:E144. Preserve all four callback ranges, checksum exit-code override,
 * raw-handle scan, CRT vectors, then low-BYTE DOS termination. */
void BtechRuntimeExitAndCloseHandles(uint16_t exitCode)
{
    BtechRunNearCallbacksReverse(NULL,0); /*774C..774C*/
    BtechRunNearCallbacksReverse(runtimeExitCallbacks,1);
    BtechRunNearCallbacksReverse(NULL,0); /*4DFC..4DFC*/
    BtechRunFarCallbacksReverse(NULL,0); /*4DFC..4DFC*/
    if (BtechCheckRuntimeDataChecksum() && !exitCode) exitCode=COMBAT_RUNTIME_ERROR_EXIT;
    for (uint16_t handle=COMBAT_STANDARD_HANDLES;handle<COMBAT_DOS_HANDLES;++handle)
    {
        if (!(runtimeHandleFlags[handle]&1)) continue;
        if (!btechHostCloseHandle) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !btechHostCloseHandle");
        btechHostCloseHandle(handle); /* Raw INT21 close: no runtime table write. */
    }
    BtechRestoreRuntimeVectors();
    if (!btechHostTerminateChild) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !btechHostTerminateChild");
    btechHostTerminateChild((uint8_t)exitCode);
    /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("DOS4C never returns into E1A0's following bytes."); /* DOS4C never returns into E1A0's following bytes. */
}
