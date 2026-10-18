#include "SDLBackend.h"
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

MwHostSoundSystem mwSdlSoundSystem=MW_SOUND_SPEAKER;
char mwSdlMt32ControlRom[4096],mwSdlMt32PcmRom[4096];
enum { HOST_MIN_CPU_CYCLES_PER_MS=100,HOST_MAX_CPU_CYCLES_PER_MS=100000 };

/* Parse into temporary configuration: an invalid trailing key must not change
 * the approved CPU rate or partially select an audio device. ROM paths are
 * resolved relative to this INI, not the game's working/save directory. */
bool MwSdlLoadTimingConfiguration(const char *path,bool optional)
{
    FILE *file=fopen(path,"r");
    if (!file) return optional && errno==ENOENT;
    char line[8192],controlRom[4096],pcmRom[4096];
    strcpy(controlRom,mwSdlMt32ControlRom);strcpy(pcmRom,mwSdlMt32PcmRom);
    uint32_t cycles=mwSdlCpuCyclesPerMillisecond;
    MwHostSoundSystem soundSystem=mwSdlSoundSystem;
    bool valid=true;
    while (fgets(line,sizeof(line),file))
    {
        char *key=line;
        while (isspace((unsigned char)*key)) ++key;
        if (!*key || *key==';' || *key=='#') continue;
        char *value=strchr(key,'=');
        if (!value) {valid=false;break;}
        char *keyEnd=value;
        while (keyEnd>key && isspace((unsigned char)keyEnd[-1])) --keyEnd;
        *keyEnd=0;++value;
        while (isspace((unsigned char)*value)) ++value;
        char *end=value+strlen(value);
        while (end>value && isspace((unsigned char)end[-1])) --end;
        *end=0;
        if (!strcmp(key,"cpu_cycles_per_ms"))
        {
            if (!isdigit((unsigned char)*value)) {valid=false;break;}
            errno=0;unsigned long rate=strtoul(value,&end,10);
            if (errno || *end || rate<HOST_MIN_CPU_CYCLES_PER_MS || rate>HOST_MAX_CPU_CYCLES_PER_MS) {valid=false;break;}
            cycles=(uint32_t)rate;
        }
        else if (!strcmp(key,"sound_system"))
        {
            if (!strcmp(value,"speaker")) soundSystem=MW_SOUND_SPEAKER;
            else if (!strcmp(value,"mt32")) soundSystem=MW_SOUND_MT32;
            else if (!strcmp(value,"adlib")) soundSystem=MW_SOUND_ADLIB;
            else {valid=false;break;}
        }
        else if (!strcmp(key,"mt32_control_rom") || !strcmp(key,"mt32_pcm_rom"))
        {
            char *destination=!strcmp(key,"mt32_control_rom")?controlRom:pcmRom;
            if (value[0]=='"' && end>value+1 && end[-1]=='"') {*--end=0;++value;}
            if (!*value || value[0]=='/' || value[0]=='\\' || (strlen(value)>1 && value[1]==':'))
            {
                if (strlen(value)>=4096) {valid=false;break;}
                strcpy(destination,value);
            }
            else
            {
                const char *slash=strrchr(path,'/'),*backslash=strrchr(path,'\\');
                if (backslash && (!slash || backslash>slash)) slash=backslash;
                size_t prefix=slash?(size_t)(slash-path)+1:0;
                if (prefix+strlen(value)>=4096) {valid=false;break;}
                memcpy(destination,path,prefix);strcpy(destination+prefix,value);
            }
        }
        else {valid=false;break;}
    }
    if (ferror(file)) valid=false;
    if (fclose(file)) valid=false;
    if (valid)
    {
        mwSdlCpuCyclesPerMillisecond=cycles;mwSdlSoundSystem=soundSystem;
        strcpy(mwSdlMt32ControlRom,controlRom);strcpy(mwSdlMt32PcmRom,pcmRom);
    }
    return valid;
}
