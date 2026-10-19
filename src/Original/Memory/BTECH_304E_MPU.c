#include "mechwarrior/combat_mpu.h"
uint16_t combatMpuDataPort,combatMpuStatusPort,combatMpuCommandPort,combatMpuReceivedBytes;
uint8_t combatMpuInstalled,combatMpuPicUnmask,combatMpuVector;
uint32_t combatMpuPreviousVector;
uint16_t combatMpuInstrumentOffset,combatMpuInstrumentSelector;
bool BtechImportMpuInitialData(const uint8_t *data,size_t bytes)
{
    if (!data || bytes<0x10000) return false;
    combatMpuReceivedBytes=MwReadNativeWord(data,0x2B76);
    combatMpuDataPort=MwReadNativeWord(data,0x2B78);combatMpuStatusPort=MwReadNativeWord(data,0x2B7A);
    combatMpuCommandPort=MwReadNativeWord(data,0x2B7C);combatMpuInstalled=data[0x2B7E];
    combatMpuPicUnmask=data[0x2B7F];combatMpuVector=data[0x2B80];
    combatMpuPreviousVector=MwReadNativeWord(data,0x2B81)|((uint32_t)MwReadNativeWord(data,0x2B83)<<16);
    combatMpuInstrumentOffset=MwReadNativeWord(data,0x6F74);
    combatMpuInstrumentSelector=MwReadNativeWord(data,0x6F76);
    return true;
}
