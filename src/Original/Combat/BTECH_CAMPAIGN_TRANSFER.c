/* Actual resident IO decode only. Runtime game code uses typed fields. */
#include "mechwarrior/combat_import.h"
#include "mechwarrior/native_address.h"
#include <string.h>
void BtechDecodeCampaignImport(const uint8_t bytes[CAMPAIGN_EXCHANGE_BYTES],BtechCampaignImport *company)
{
    company->memoryTier=bytes[1];company->landscapeIndex=bytes[2];
    company->enemyHeavyCount=bytes[3];company->enemyMediumCount=bytes[4];company->enemyLightCount=bytes[5];
    company->planetSelector=bytes[6];company->scenarioCode=bytes[7];company->terrainDetailLevel=bytes[8];
    company->soundDisabled=bytes[0x05E2];
    memcpy(company->commanderName,bytes+0x003B,sizeof(company->commanderName));
    for (uint16_t slot=0;slot<4;++slot)
    {
        company->pilotIds[slot]=MwReadNativeWord(bytes,0x006F+slot*2);
        company->gunneryTiers[slot]=MwReadNativeWord(bytes,0x00AA+slot*2);
        company->pilotingTiers[slot]=MwReadNativeWord(bytes,0x00B2+slot*2);
        company->ownedMechSlots[slot]=MwReadNativeWord(bytes,0x00C2+slot*2);
    }
    for (uint16_t slot=0;slot<12;++slot)
    {
        company->ownedUnitKinds[slot]=MwReadNativeWord(bytes,0x00EA+slot*2);
        memcpy(&company->ownedDamage[slot],bytes+0x0102+slot*sizeof(CampaignUnitDamage),sizeof(CampaignUnitDamage));
        for (uint16_t group=0;group<6;++group)
            company->ammoQuantities[group][slot]=MwReadNativeWord(bytes,0x025E+group*0x18+slot*2);
    }
}
