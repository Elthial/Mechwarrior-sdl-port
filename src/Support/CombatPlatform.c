#include "mechwarrior/combat_platform.h"
#include "mechwarrior/combat_startup.h"
#include "mechwarrior/combat_runtime.h"
#include "mechwarrior/combat_video.h"
#include "mechwarrior/combat_sound.h"
MwNativeByteView (*btechHostBorrowDosMemory)(uint16_t);
FILE *(*btechHostOpenStream)(const char *,const char *);
uint8_t (*btechHostReadPort)(uint16_t);
void (*btechHostWritePort)(uint16_t,uint8_t);
uint8_t (*btechHostSetInterrupts)(uint8_t);
void (*btechHostDelayLoops)(uint16_t);
void (*btechHostPollInterrupts)(void);
void (*btechHostPaceCombatCpu)(void);
void (*btechHostChainTimerInterrupt)(uint32_t);
uint8_t *(*btechHostAllocateParagraphs)(uint16_t,uint16_t *);
void (*btechHostFreeDosAllocation)(uint8_t *);
const uint8_t *(*btechHostResolveWrappedResourcePriority)(const uint8_t *,int16_t);
uint32_t (*btechHostGetInterruptVector)(uint8_t);
void (*btechHostSetInterruptVector)(uint8_t,uint32_t);
uint16_t (*btechHostReadBiosWord)(uint16_t);
void (*btechHostWriteBiosWord)(uint16_t,uint16_t);
uint8_t (*btechHostReadBiosByte)(uint16_t);
void (*btechHostWriteBiosByte)(uint16_t,uint8_t);
void (*btechHostResetMouse)(void);
void (*btechHostSetVideoMode)(uint8_t);
int16_t (*btechHostCloseHandle)(uint16_t);
void (*btechHostWriteHandle)(uint16_t,const char *,uint16_t);
void (*btechHostTerminateChild)(uint8_t);
uint16_t (*btechHostQueryEgaInfo)(void);
uint8_t (*btechHostReadRomByte)(uint16_t,uint16_t);
uint8_t (*btechHostReadVideoByte)(uint16_t,uint16_t);
void (*btechHostWriteVideoByte)(uint16_t,uint16_t,uint8_t);
void (*btechHostWritePortWord)(uint16_t,uint16_t);
void (*btechHostSetPaletteRegister)(uint8_t,uint8_t);
void (*btechHostSetOverscanColour)(uint8_t);
bool (*btechHostQueryKeyboardIntercept)(uint16_t);
bool (*btechHostQuerySystemConfiguration)(uint16_t);
