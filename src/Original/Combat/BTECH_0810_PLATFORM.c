#include "mechwarrior/combat_runtime.h"
#include "mechwarrior/combat_import.h"
#include "mechwarrior/combat_platform.h"
#include "mechwarrior/combat_sound.h"
#include <stdlib.h>

/*0810:CC6F successful-open branch.2777's missing-disk screen/retry is game
 * code, not a host disk-prompt callback. Fail explicitly until it is linked. */
FILE *BtechOpenStreamWithRetry(const char *filename,const char *mode)
{
    BtechRuntimeStream *stream=BtechRuntimeStreamOpen(filename,mode);
    if (!stream) abort();
    return stream->file;
}

/*0810:293C/2953. IRQ delivery is the host boundary, not a replacement delay. */
void BtechWaitScaledRuntimeDelay(uint16_t delayUnits)
{
    runtimeDelayRemaining=(uint16_t)(delayUnits<<2);
    while (BtechGetActiveRuntimeDelayFar())
    {
        if (!btechHostPollInterrupts) abort();
        btechHostPollInterrupts();
    }
}
uint16_t BtechGetActiveRuntimeDelayFar(void)
{
    return runtimeDelayActive?runtimeDelayRemaining:0;
}

/*0810:29CD. Filename changes even when requested00 leaves the old mode. */
void BtechConfigureCombatInputReplay(uint16_t requestedMode,const char *filename)
{
    combatInputReplayFilename=filename;
    if (!requestedMode) return;
    combatInputReplayMode=requestedMode;
    if (requestedMode==COMBAT_INPUT_RECORD) BtechCloseStreamFile(BtechOpenStreamWithRetry(filename,"wb"));
}

/*0810:EAA0.2CBC sign-extends the command BYTE. Negative characters address
 * the preceding authored bytes, not an unsigned-ASCII classification entry. */
uint16_t BtechLowercaseCharacterByClassTable(uint16_t character)
{
    uint16_t index=character+0x80;
    if (index>=sizeof(characterClassStorage.bytes)) abort();
    return characterClassStorage.bytes[index]&1?character+0x20:character;
}
