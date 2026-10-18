#ifndef MECHWARRIOR_CAMPAIGN_RUNTIME_H
#define MECHWARRIOR_CAMPAIGN_RUNTIME_H
#include "mechwarrior/campaign_startup.h"

/* Host launch/audio projection of original argv/PSP, not new native ABI.
 * The present conversion supports the default PC-speaker branch only. */
int16_t MwRunCampaignProgram(const MwCampaignLaunch *launch,uint8_t audioCode);
void MwStartMidiTrackFar(uint16_t selector,uint16_t offset);
void MwStopMidiTrackFar(void);
#endif
