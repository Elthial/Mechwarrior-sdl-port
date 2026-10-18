#include "SDLBackend.h"
#include "mechwarrior/ega_campaign.h"
#include "mechwarrior/combat_video.h"
#include <string.h>

/* Physical EGA controller state only. Campaign and combat share ONE indexed
 * aperture, not duplicate framebuffers. Read/write latches project its planes.
 * This is the lowest byte/port boundary used by the original BTECH rasterizers. */
bool mwSdlCombatActive;
static uint8_t graphicsIndex,sequencerIndex,crtcIndex;
static uint8_t graphicsRegisters[9],sequencerRegisters[5],crtcRegisters[25],latches[4];
enum { EGA_SET_RESET=0,EGA_ENABLE_SET_RESET=1,EGA_DATA_ROTATE=3,EGA_READ_MAP=4,
    EGA_MODE=5,EGA_BIT_MASK=8,EGA_MAP_MASK=2,EGA_START_HIGH=0x0C,EGA_START_LOW=0x0D,
    EGA_CRTC_INDEX=0x3D4,EGA_CRTC_DATA=0x3D5,EGA_SEQUENCE_INDEX=0x3C4,EGA_SEQUENCE_DATA=0x3C5,
    EGA_GRAPHICS_INDEX=0x3CE,EGA_GRAPHICS_DATA=0x3CF,EGA_STATUS=0x3DA };

void MwSdlResetCombatVideoHardware(void)
{
    memset(graphicsRegisters,0,sizeof(graphicsRegisters));memset(sequencerRegisters,0,sizeof(sequencerRegisters));
    memset(crtcRegisters,0,sizeof(crtcRegisters));memset(latches,0,sizeof(latches));
    graphicsIndex=sequencerIndex=crtcIndex=0;
    graphicsRegisters[EGA_BIT_MASK]=0xFF;sequencerRegisters[EGA_MAP_MASK]=0x0F;
}
bool MwSdlWriteCombatVideoPort(uint16_t port,uint8_t value)
{
    switch (port)
    {
    case EGA_GRAPHICS_INDEX:graphicsIndex=value;return true;
    case EGA_GRAPHICS_DATA:
        if (graphicsIndex>=sizeof(graphicsRegisters)) MwSdlFail("Unmapped EGA graphics register");
        graphicsRegisters[graphicsIndex]=value;return true;
    case EGA_SEQUENCE_INDEX:sequencerIndex=value;return true;
    case EGA_SEQUENCE_DATA:
        if (sequencerIndex>=sizeof(sequencerRegisters)) MwSdlFail("Unmapped EGA sequencer register");
        sequencerRegisters[sequencerIndex]=value;return true;
    case EGA_CRTC_INDEX:crtcIndex=value;return true;
    case EGA_CRTC_DATA:
        if (crtcIndex>=sizeof(crtcRegisters)) MwSdlFail("Unmapped EGA CRTC register");
        crtcRegisters[crtcIndex]=value;return true;
    default:return false;
    }
}
bool MwSdlReadCombatVideoPort(uint16_t port,uint8_t *value)
{
    switch (port)
    {
    case EGA_GRAPHICS_DATA:
        if (graphicsIndex>=sizeof(graphicsRegisters)) MwSdlFail("Read unmapped EGA graphics register");
        *value=graphicsRegisters[graphicsIndex];return true;
    case EGA_SEQUENCE_DATA:
        if (sequencerIndex>=sizeof(sequencerRegisters)) MwSdlFail("Read unmapped EGA sequencer register");
        *value=sequencerRegisters[sequencerIndex];return true;
    case EGA_CRTC_DATA:
        if (crtcIndex>=sizeof(crtcRegisters)) MwSdlFail("Read unmapped EGA CRTC register");
        *value=crtcRegisters[crtcIndex];return true;
    case EGA_STATUS:
        /* Independent 60Hz scanout. Reading status is not a simulation tick. */
        *value=SDL_GetTicksNS()%UINT64_C(16666667)>=UINT64_C(15000000)?COMBAT_EGA_VERTICAL_RETRACE:0;
        return true;
    default:return false;
    }
}
static uint32_t AperturePixel(uint16_t selector,uint16_t offset)
{
    uint32_t address=((uint32_t)selector<<4)+offset;
    if (address<0xA0000 || address>=0xB0000) MwSdlFail("Unmapped physical EGA aperture access");
    return (address-0xA0000)*8;
}
static uint8_t ReadVideoByte(uint16_t selector,uint16_t offset)
{
    MwSdlAccountCombatVideoAccess(false);
    uint32_t pixel=AperturePixel(selector,offset);
    for (uint16_t plane=0;plane<4;++plane)
    {
        uint8_t bits=0;
        for (uint16_t bit=0;bit<8;++bit) bits|=((egaVideoPixels[pixel+bit]>>plane)&1)<<(7-bit);
        latches[plane]=bits;
    }
    if (graphicsRegisters[EGA_MODE]&8) MwSdlFail("EGA read mode1 is not connected");
    return latches[graphicsRegisters[EGA_READ_MAP]&3];
}
static void WriteVideoByte(uint16_t selector,uint16_t offset,uint8_t value)
{
    MwSdlAccountCombatVideoAccess(true);
    uint32_t pixel=AperturePixel(selector,offset);
    uint8_t mode=graphicsRegisters[EGA_MODE]&3,rotate=graphicsRegisters[EGA_DATA_ROTATE]&7;
    uint8_t rotated=(uint8_t)((value>>rotate)|(value<<((8-rotate)&7)));
    if (mode==3) MwSdlFail("EGA write mode3 is not connected");
    for (uint16_t plane=0;plane<4;++plane)
    {
        if (!(sequencerRegisters[EGA_MAP_MASK]&(1<<plane))) continue;
        uint8_t painted;
        if (mode==1) painted=latches[plane];
        else
        {
            uint8_t source=mode==2?((value&(1<<plane))?0xFF:0):
                (graphicsRegisters[EGA_ENABLE_SET_RESET]&(1<<plane))?((graphicsRegisters[EGA_SET_RESET]&(1<<plane))?0xFF:0):rotated;
            switch ((graphicsRegisters[EGA_DATA_ROTATE]>>3)&3)
            {
            case 1:source&=latches[plane];break;
            case 2:source|=latches[plane];break;
            case 3:source^=latches[plane];break;
            }
            painted=(source&graphicsRegisters[EGA_BIT_MASK])|(latches[plane]&(uint8_t)~graphicsRegisters[EGA_BIT_MASK]);
        }
        for (uint16_t bit=0;bit<8;++bit)
        {
            uint8_t colour=egaVideoPixels[pixel+bit]&(uint8_t)~(1<<plane);
            egaVideoPixels[pixel+bit]=colour|(((painted>>(7-bit))&1)<<plane);
        }
    }
}
static void WritePortWord(uint16_t port,uint16_t value)
{
    MwSdlWriteDevicePort(port,(uint8_t)value);MwSdlWriteDevicePort(port+1,(uint8_t)(value>>8));
}
uint16_t MwSdlCombatDisplayByteOffset(void)
{return ((uint16_t)crtcRegisters[EGA_START_HIGH]<<8)|crtcRegisters[EGA_START_LOW];}
void MwSdlBindCombatVideo(void)
{
    btechHostReadVideoByte=ReadVideoByte;btechHostWriteVideoByte=WriteVideoByte;btechHostWritePortWord=WritePortWord;
}
