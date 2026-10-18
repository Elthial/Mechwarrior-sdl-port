#include "mechwarrior/audio_devices.h"
#include <mt32emu/mt32emu.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static mt32emu_context synthesizer;
static mt32emu_report_handler_version MT32EMU_C_CALL ReportVersion(mt32emu_report_handler_i reports)
{(void)reports;return MT32EMU_REPORT_HANDLER_VERSION_0;}
static void MT32EMU_C_CALL DebugMessage(void *unused,const char *format,va_list arguments)
{
    (void)unused;
    if (getenv("MECHWARRIOR_DIAGNOSTICS")) {vfprintf(stderr,format,arguments);fputc('\n',stderr);}
}
static const mt32emu_report_handler_i_v0 reportCallbacks={.getVersionID=ReportVersion,.printDebug=DebugMessage};

bool MwMt32Open(const char *controlRom,const char *pcmRom,uint32_t sampleRate,char *error,size_t errorSize)
{
    mt32emu_report_handler_i reports={.v0=&reportCallbacks};
    synthesizer=mt32emu_create_context(reports,NULL);
    if (!synthesizer) {snprintf(error,errorSize,"Cannot create libmt32emu context");return false;}
    if (!*controlRom || !*pcmRom)
    {
        snprintf(error,errorSize,"MT-32 requires mt32_control_rom and mt32_pcm_rom in MechWarrior.ini");
        MwMt32Close();return false;
    }
    if (mt32emu_add_rom_file(synthesizer,controlRom)<0
        || mt32emu_add_rom_file(synthesizer,pcmRom)<0)
    {
        snprintf(error,errorSize,"Cannot load MT-32 ROM files: %s / %s",controlRom,pcmRom);
        MwMt32Close();return false;
    }
    mt32emu_set_stereo_output_samplerate(synthesizer,sampleRate);
    if (mt32emu_open_synth(synthesizer)<0)
    {
        snprintf(error,errorSize,"Incompatible or incomplete MT-32 CONTROL/PCM ROM pair");
        MwMt32Close();return false;
    }
    /* UART transmission is already paced by the hardware adapter and native
     * byte queue. Do not add a second message-length MIDI cable delay. */
    mt32emu_set_midi_delay_mode(synthesizer,MT32EMU_MDM_IMMEDIATE);
    mt32emu_rom_info roms;mt32emu_get_rom_info(synthesizer,&roms);
    fprintf(stderr,"MT-32: %s; %s; libmt32emu %s\n",roms.control_rom_description,
        roms.pcm_rom_description,mt32emu_get_library_version_string());
    return true;
}
void MwMt32ReceiveByte(uint8_t value)
{
    /* Library parser retains running status and fragmented SysEx between bytes.
     * Original MIDI bytes are never translated to General MIDI programs. */
    mt32emu_parse_stream(synthesizer,&value,1);
}
void MwMt32Render(float *stereo,uint32_t frames)
{mt32emu_render_float(synthesizer,stereo,frames);}
void MwMt32Close(void)
{
    if (synthesizer) mt32emu_free_context(synthesizer);
    synthesizer=NULL;
}
