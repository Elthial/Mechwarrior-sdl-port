#include "SDLBackend.h"
#include "mechwarrior/combat_input.h"
#include "mechwarrior/combat_video.h"
#include "mechwarrior/campaign_input.h"
#include "mechwarrior/combat_installation.h"
#include <string.h>

/* Physical BIOS/interrupt transport, not another game state or replacement
 * timer. Native numeric vectors are opaque host identities at this boundary;
 * zero denotes the retained pre-child host handler, not invented BIOS bytes. */
static uint8_t biosData[0x100];
static uint32_t interruptVectors[0x100];
static MwMusicTimerHandler previousTimerHandler;

/* Actual DOSBox-X captures before EXEC, briefing, first scene and post-fire
 * agree on these bytes. This is the missing-ITM physical read edge, NOT a
 * fabricated zero instrument bank or a general DOS memory image. */
enum { DOS_TONE_ZERO_WINDOW_START=0x1F4A,DOS_TONE_ZERO_WINDOW_BYTES=0x1D4,
    DOS_DEFAULT_IRET_VECTOR=0x0070000E,DOS_TONE_VECTOR_PREFIX=6,
    HOST_REAL_MODE_ADDRESS_MASK=0xFFFFF,HOST_BIOS_MODEL_ADDRESS=0xFFFFE,HOST_BIOS_AT_MODEL=0xFC };
static const uint32_t dosDefaultDivideVector=UINT32_C(0xF000CA60),dosPrintScreenVector=UINT32_C(0xF000FF54);
static const uint8_t dosToneZeroWindow[DOS_TONE_ZERO_WINDOW_BYTES]={0};
static uint8_t ReadToneDosByte(uint16_t offset)
{
    if (offset<DOS_TONE_VECTOR_PREFIX*4)
    {
        uint16_t interrupt=offset/4;uint32_t vector;
        if (!interrupt)
        {
            vector=interruptVectors[0];
            if (!vector) vector=dosDefaultDivideVector;
            else if (vector==BTECH_DIVIDE_HANDLER_VECTOR)
                vector+=((uint32_t)(HOST_DOS_COMBAT_CODE_SELECTOR-BTECH_IMAGE_LOAD_SELECTOR)<<16);
            else MwSdlFail("Unrecovered physical divide vector for tone fallback");
        }
        else
        {
            if (interruptVectors[interrupt]) MwSdlFail("Unrecovered modified physical DOS vector for tone fallback");
            vector=interrupt<5?DOS_DEFAULT_IRET_VECTOR:dosPrintScreenVector;
        }
        return (uint8_t)(vector>>((offset&3)*8));
    }
    if (offset>=DOS_TONE_ZERO_WINDOW_START
        && (uint16_t)(offset-DOS_TONE_ZERO_WINDOW_START)<sizeof(dosToneZeroWindow))
        return dosToneZeroWindow[offset-DOS_TONE_ZERO_WINDOW_START];
    MwSdlFail("Tone fallback read outside recovered physical DOS owners");
}
static MwNativeByteView BorrowToneDosMemory(uint16_t selector)
{
    if (selector) MwSdlFail("Unrecovered physical DOS selector for tone fallback");
    return (MwNativeByteView){NULL,0,0,ReadToneDosByte};
}
static uint8_t ReadBiosByte(uint16_t offset)
{
    if (offset>=sizeof(biosData)) MwSdlFail("Read outside represented BIOS data");
    return biosData[offset];
}
static void WriteBiosByte(uint16_t offset,uint8_t value)
{
    if (offset>=sizeof(biosData)) MwSdlFail("Write outside represented BIOS data");
    biosData[offset]=value;
}
static uint16_t ReadBiosWord(uint16_t offset)
{
    if (offset==BIOS_TIMER_TICKS_LOW) return (uint16_t)MwSdlBiosTicksValue();
    if (offset==BIOS_TIMER_TICKS_HIGH) return (uint16_t)(MwSdlBiosTicksValue()>>16);
    return ReadBiosByte(offset)|((uint16_t)ReadBiosByte(offset+1)<<8);
}
static void WriteBiosWord(uint16_t offset,uint16_t value)
{WriteBiosByte(offset,(uint8_t)value);WriteBiosByte(offset+1,(uint8_t)(value>>8));}
static uint32_t ReadInterruptVector(uint8_t interrupt) {return interruptVectors[interrupt];}
static void UnmappedTimerInterrupt(void) {MwSdlFail("Unconverted native IRQ0 vector");}
static void WriteInterruptVector(uint8_t interrupt,uint32_t vector)
{
    interruptVectors[interrupt]=vector;
    if (interrupt==TIMER_INTERRUPT)
        MwSdlWriteTimerHandler(vector==0?previousTimerHandler:vector==BTECH_TIMER_HANDLER_VECTOR?BtechTimerSchedulerInterrupt:UnmappedTimerInterrupt);
}
static void ChainTimerInterrupt(uint32_t vector)
{
    if (vector || !previousTimerHandler) MwSdlFail("Unmapped saved native timer vector");
    previousTimerHandler();
}
static uint8_t ReadRomByte(uint16_t selector,uint16_t offset)
{
    uint32_t address=(((uint32_t)selector<<4)+offset)&HOST_REAL_MODE_ADDRESS_MASK;
    return address==HOST_BIOS_MODEL_ADDRESS?HOST_BIOS_AT_MODEL:UINT8_MAX; /* Configured AT/EGA, not PCjr/Tandy. */
}
static uint16_t QueryEgaInfo(void) {return 0;}
static void ResetMouse(void) {mwCampaignInput.resetMouse();}
void MwSdlResetCombatHardware(void)
{
    MwSdlResetCombatCpu();
    previousTimerHandler=MwSdlReadTimerHandler();
    memset(interruptVectors,0,sizeof(interruptVectors));memset(biosData,0,sizeof(biosData));
    WriteBiosWord(BIOS_KEYBOARD_HEAD,BIOS_KEY_BUFFER_FIRST);WriteBiosWord(BIOS_KEYBOARD_TAIL,BIOS_KEY_BUFFER_FIRST);
    WriteBiosWord(BIOS_KEYBOARD_START,BIOS_KEY_BUFFER_FIRST);WriteBiosWord(BIOS_KEYBOARD_END,BIOS_KEY_BUFFER_END);
    WriteBiosWord(COMBAT_BIOS_CRTC_PORT,COMBAT_COLOUR_CRTC_INDEX_PORT);
}
void MwSdlInitializeCombatHardware(void)
{
    btechHostReadBiosByte=ReadBiosByte;btechHostWriteBiosByte=WriteBiosByte;
    btechHostReadBiosWord=ReadBiosWord;btechHostWriteBiosWord=WriteBiosWord;
    btechHostGetInterruptVector=ReadInterruptVector;btechHostSetInterruptVector=WriteInterruptVector;
    btechHostBorrowDosMemory=BorrowToneDosMemory;
    btechHostSetInterrupts=MwSdlExchangeInterrupts;btechHostChainTimerInterrupt=ChainTimerInterrupt;
    btechHostReadRomByte=ReadRomByte;btechHostQueryEgaInfo=QueryEgaInfo;btechHostSetVideoMode=MwSdlSetVideoMode;
    btechHostDelayLoops=MwSdlDelayCpuLoops;btechHostPollInterrupts=MwSdlPump;btechHostResetMouse=ResetMouse;
    btechHostPaceCombatCpu=MwSdlPaceCombatCpu;
}
