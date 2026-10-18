#include "mechwarrior/combat_render.h"
#include "mechwarrior/combat_startup.h"
#include <stdlib.h>

/*1BF5:1F4A. Supported F7 WORD-divide fault path. The actual horizon IDIVs
 * are two bytes followed by non-F6/F7 instructions, so the8086 next-IP check
 * rewinds two bytes and selects SAR DX/RCR AX. This DWORD is that register
 * transport, not a native pushed argument/result. Byte faults/foreign opcodes
 * and delivery without the installed hook are not ordinary C calls. */
uint32_t BtechDivideErrorInterrupt(uint32_t dividend)
{
    if (!divideErrorHookMarkedActive) abort();
    return (dividend>>1)|(dividend&UINT32_C(0x80000000));
}
