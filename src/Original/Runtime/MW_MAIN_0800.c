#include "mechwarrior/campaign_runtime.h"
#include "mechwarrior/campaign_music.h"
#include <stdlib.h>

/* MW_MAIN0800:0000..0185. Native120E checks/allocates the physical 16-bit
 * caller frame. Host C owns that frame; do not manufacture a DOS SP or a fake
 * successful stack-check procedure. DOS overflow/terminate handlers are not
 * converted by this normal-entry projection. Native CBW sign-extends AL. */
int16_t MwRunCampaignProgram(const MwCampaignLaunch *launch,uint8_t audioCode)
{
    /* Original A/a, R/r and T/t select other backends. Do not silently fall
     * back to PC or claim their probes/side effects have been implemented. */
    if (audioCode=='A' || audioCode=='a'
        || audioCode=='T' || audioCode=='t') abort();
    MwCampaignAudioBackend *backend=&speakerBackend;
    if (audioCode=='R' || audioCode=='r') backend=&mpuBackend;
    MwInstallMusicTimer();
    MwTimerSetCallback(1,MwTickMidiTrack,0);
    MwTimerSetCallback(3,MwTickMidiTrack,0);
    MwTimerSetCallback(5,MwTickMidiTrack,0);
    MwTimerSetCallback(7,MwTickMidiTrack,0);
    /* Actual PC0EE4 is unconditionally successful; failed other-adapter probe
     * chains are outside this minimum speaker slice, not success stubs. */
    if (!MwProbeMusicBackend(backend)) abort();
    MwSelectAndInitializeMidiBackend(backend);
    uint8_t campaignResult=MwRunCampaign(launch);
    MwShutdownMusicBackend();
    MwTimerSetCallback(1,NULL,0);
    MwTimerSetCallback(3,NULL,0);
    MwTimerSetCallback(5,NULL,0);
    MwTimerSetCallback(7,NULL,0);
    MwRestoreBiosTimer();
    return campaignResult<0x80?campaignResult:(int16_t)campaignResult-0x100;
}

/* MW_MAIN0800:0186..01A3 /01A4..01B4. Genuine FAR wrappers. Native resource
 * DWORD is split into selector/offset only in this host representation. */
void MwStartMidiTrackFar(uint16_t selector,uint16_t offset)
{(void)MwStartMidiTrack(selector,offset,MIDI_TRACK_ACTIVE);}
void MwStopMidiTrackFar(void) {MwStopMidiTrack();}
