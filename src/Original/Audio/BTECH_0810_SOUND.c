#include "mechwarrior/combat_sound.h"
#include "mechwarrior/combat_import.h"

/*0810:096B. Full-WORD game gate; event identity and unsigned priority, not a
 * weapon kind or hardware note. Disabled path does not touch bank/channel data. */
void BtechPlaySoundIfEnabled(int16_t soundEventIndex,uint16_t priority)
{
    if (combatSoundEnabled) BtechDispatchSoundBankEvent(soundEventIndex,priority);
}
