#include "SDLBackend.h"
#include "mechwarrior/ega_campaign.h"
#include "mechwarrior/combat_video.h"
#include "mechwarrior/combat_startup.h"
#include <stdio.h>
#include <string.h>

static SDL_Window *window;
static SDL_Renderer *renderer;
static SDL_Texture *texture;
static uint8_t videoMode=3,paletteCodes[EGA_PALETTE_ENTRIES];
static uint8_t displayedIndices[EGA_COLUMNS*EGA_ROWS],lastPalette[EGA_PALETTE_ENTRIES];
static uint32_t rgbaPixels[EGA_COLUMNS*EGA_ROWS];
static uint16_t lastPage;
static char captureDirectory[4096];
static bool haveFrame,presenting;
void (*mwSdlFrameObserver)(void);

static uint32_t Mode0dColour(uint8_t code)
{
    uint32_t intensity=(code&0x10)?85:0;
    uint32_t red=((code&4)?170:0)+intensity,green=((code&2)?170:0)+intensity,blue=((code&1)?170:0)+intensity;
    if ((code&0x17)==6) green=85;
    return 0xFF000000|(red<<16)|(green<<8)|blue;
}
void MwSdlSetVideoMode(uint8_t mode)
{
    if (mode!=3 && mode!=EGA_BIOS_MODE_320X200) MwSdlFail("Unsupported video mode");
    videoMode=mode;
    if (mode==EGA_BIOS_MODE_320X200)
    {
        memset(egaVideoPixels,0,sizeof(egaVideoPixels));egaLogicalFunction=EGA_COPY;
        MwSdlResetCombatVideoHardware();
        for (uint8_t i=0;i<16;++i) paletteCodes[i]=(i&7)|((i&8)?0x10:0);
        paletteCodes[16]=0;haveFrame=false;
        MwSdlResetCombatCpu();
    }
}
uint8_t MwSdlGetVideoMode(void) {return videoMode;}
void MwSdlSetMousePosition(uint16_t horizontalPosition,uint16_t verticalPosition)
{
    float windowX,windowY;
    if (!renderer || !window || !SDL_RenderCoordinatesToWindow(renderer,
        horizontalPosition/2.0f,(float)verticalPosition,&windowX,&windowY))
        MwSdlFail("Position mouse in EGA coordinates");
    SDL_WarpMouseInWindow(window,windowX,windowY);
}
void MwSdlReadMousePosition(uint16_t *horizontalPosition,uint16_t *verticalPosition)
{
    float windowX,windowY,logicalX,logicalY;
    if (!horizontalPosition || !verticalPosition || !renderer) MwSdlFail("Read mouse in EGA coordinates");
    SDL_GetMouseState(&windowX,&windowY);
    if (!SDL_RenderCoordinatesFromWindow(renderer,windowX,windowY,&logicalX,&logicalY))
        MwSdlFail("Read mouse in EGA coordinates");
    if (logicalX<0) logicalX=0;else if (logicalX>EGA_COLUMNS-1) logicalX=EGA_COLUMNS-1;
    if (logicalY<0) logicalY=0;else if (logicalY>EGA_ROWS-1) logicalY=EGA_ROWS-1;
    *horizontalPosition=(uint16_t)logicalX<<1;
    *verticalPosition=(uint16_t)logicalY;
}
void MwSdlConvertMouseMotion(float windowXMotion,float windowYMotion,
    float *logicalXMotion,float *logicalYMotion)
{
    float originX,originY,movedX,movedY;
    if (!logicalXMotion || !logicalYMotion || !renderer
        || !SDL_RenderCoordinatesFromWindow(renderer,0,0,&originX,&originY)
        || !SDL_RenderCoordinatesFromWindow(renderer,windowXMotion,windowYMotion,&movedX,&movedY))
        MwSdlFail("Convert mouse motion to EGA coordinates");
    *logicalXMotion=movedX-originX;*logicalYMotion=movedY-originY;
}
static void PaletteRegister(uint8_t index,uint8_t code)
{if (index>=EGA_PALETTE_ENTRIES) MwSdlFail("Unsupported EGA palette register");paletteCodes[index]=code;}
static void PaletteTable(const uint8_t *codes) {memcpy(paletteCodes,codes,sizeof(paletteCodes));}
static void OverscanRegister(uint8_t code) {paletteCodes[16]=code;}
static void PresentPage(uint16_t page)
{
    if (page!=egaDisplayPage) MwSdlFail("EGA display-page binding mismatch");
    MwSdlPresentVideo(true);
}
bool MwSdlInitializeVideo(const char *directory)
{
    if (!directory) directory=".";
    if (strlen(directory)>=sizeof(captureDirectory)) return false;
    strcpy(captureDirectory,directory);
    window=SDL_CreateWindow("MechWarrior — EGA / PC speaker",960,600,SDL_WINDOW_RESIZABLE);
    if (!window) return false;
    renderer=SDL_CreateRenderer(window,NULL);
    if (!renderer || !SDL_SetRenderLogicalPresentation(renderer,EGA_COLUMNS,EGA_ROWS,SDL_LOGICAL_PRESENTATION_INTEGER_SCALE)) return false;
    texture=SDL_CreateTexture(renderer,SDL_PIXELFORMAT_ARGB8888,SDL_TEXTUREACCESS_STREAMING,EGA_COLUMNS,EGA_ROWS);
    if (!texture || !SDL_SetTextureScaleMode(texture,SDL_SCALEMODE_NEAREST)) return false;
    egaSetVideoMode=MwSdlSetVideoMode;egaSetPaletteRegister=PaletteRegister;
    egaSetPaletteTable=PaletteTable;egaPresentPage=PresentPage;
    btechHostSetPaletteRegister=PaletteRegister;btechHostSetOverscanColour=OverscanRegister;
    MwSdlBindCombatVideo();
    haveFrame=presenting=false;videoMode=3;
    return true;
}
static void Render(void)
{
    if (!SDL_SetRenderDrawColor(renderer,0,0,0,255) || !SDL_RenderClear(renderer)
        || !SDL_RenderTexture(renderer,texture,NULL,NULL)) MwSdlFail("Render EGA texture");
}
void MwSdlPresentVideo(bool force)
{
    if (!renderer || presenting || videoMode!=EGA_BIOS_MODE_320X200) return;
    uint16_t displayOffset=mwSdlCombatActive?MwSdlCombatDisplayByteOffset():(uint16_t)((egaDisplayPage-EGA_FIRST_PAGE_SELECTOR)*0x10);
    uint32_t base=(uint32_t)displayOffset*8;
    bool changed=force || !haveFrame || lastPage!=displayOffset || memcmp(lastPalette,paletteCodes,sizeof(paletteCodes));
    for (uint32_t pixel=0;pixel<EGA_COLUMNS*EGA_ROWS;++pixel)
    {
        uint8_t index=egaVideoPixels[(base+pixel)&0x7FFFF];
        if (index>=16) MwSdlFail("Unrepresented EGA index");
        if (displayedIndices[pixel]!=index) changed=true;
        displayedIndices[pixel]=index;
    }
    if (!changed) return;
    presenting=true;
    for (uint32_t pixel=0;pixel<EGA_COLUMNS*EGA_ROWS;++pixel) rgbaPixels[pixel]=Mode0dColour(paletteCodes[displayedIndices[pixel]]);
    if (!SDL_UpdateTexture(texture,NULL,rgbaPixels,EGA_COLUMNS*sizeof(uint32_t))) MwSdlFail("Upload EGA texture");
    Render();
    if (!SDL_RenderPresent(renderer)) MwSdlFail("Present EGA texture");
    lastPage=displayOffset;memcpy(lastPalette,paletteCodes,sizeof(lastPalette));haveFrame=true;
    if (mwSdlFrameObserver) mwSdlFrameObserver();
    presenting=false;
}
bool MwSdlCapture(const char *stem)
{
    if (!haveFrame || !stem || strpbrk(stem,"/\\:")) return false;
    char path[4096];
    if (snprintf(path,sizeof(path),"%s/%s.bmp",captureDirectory,stem)>=(int)sizeof(path)) return false;
    /* Reserve exclusively: never overwrite an earlier observation. */
    FILE *file=fopen(path,"wbx");if (!file) return false;fclose(file);
    Render();
    SDL_Surface *surface=SDL_RenderReadPixels(renderer,NULL);
    if (!surface) return false;
    bool saved=SDL_SaveBMP(surface,path);SDL_DestroySurface(surface);
    if (!SDL_RenderPresent(renderer)) saved=false;
    if (!saved) return false;
    snprintf(path,sizeof(path),"%s/%s.indices",captureDirectory,stem);
    file=fopen(path,"wbx");if (!file) return false;
    saved=fwrite(displayedIndices,1,sizeof(displayedIndices),file)==sizeof(displayedIndices);if (fclose(file)) saved=false;
    snprintf(path,sizeof(path),"%s/%s.palette",captureDirectory,stem);
    file=fopen(path,"wbx");if (!file) return false;
    saved=saved && fwrite(lastPalette,1,sizeof(lastPalette),file)==sizeof(lastPalette);if (fclose(file)) saved=false;
    snprintf(path,sizeof(path),"%s/%s.json",captureDirectory,stem);
    file=fopen(path,"wx");if (!file) return false;
    fprintf(file,"{\"kind\":\"Actual SDL renderer readback; not a matched original replay\",\"mode\":13,\"logicalWidth\":320,\"logicalHeight\":200,\"combat\":%s,\"displayPage\":%u,\"drawPage\":%u,\"displayByteOffset\":%u,\"SDL\":\"%s\",\"acceptance\":\"Pending\"}\n",mwSdlCombatActive?"true":"false",mwSdlCombatActive?frontPageSelector:egaDisplayPage,mwSdlCombatActive?drawPageSelector:egaDrawPage,lastPage,SDL_GetRevision());
    if (fclose(file)) saved=false;
    return saved;
}
void MwSdlShutdownVideo(void)
{
    SDL_DestroyTexture(texture);SDL_DestroyRenderer(renderer);SDL_DestroyWindow(window);
    texture=NULL;renderer=NULL;window=NULL;
}
