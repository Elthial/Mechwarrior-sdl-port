#ifndef MECHWARRIOR_COMBAT_PLATFORM_H
#define MECHWARRIOR_COMBAT_PLATFORM_H
#include <stdint.h>
#include <stdio.h>
enum { DOS_MAX_PARAGRAPH_ALLOCATION_BYTES=0xFFFF0 };
/* Required low-level host boundaries. No combat/game decision callbacks. A
 * missing binding fails explicitly, never simulates successful calibration. */
extern FILE *(*btechHostOpenStream)(const char *filename,const char *mode);
extern uint8_t (*btechHostReadPort)(uint16_t port);
extern void (*btechHostWritePort)(uint16_t port,uint8_t value);
extern uint8_t (*btechHostSetInterrupts)(uint8_t enabled);
extern uint8_t (*btechHostReadInterrupts)(void);
extern void (*btechHostDelayLoops)(uint16_t loopCount);
extern void (*btechHostPollInterrupts)(void);
/* Host-only CPU-work instrumentation at native0019 loop boundary. Does not
 * add a native argument, game tick, decision or source procedure. */
extern void (*btechHostPaceCombatCpu)(void);
extern uint8_t *(*btechHostAllocateParagraphs)(uint16_t paragraphs,uint16_t *largestFreeParagraphs);
extern void (*btechHostFreeDosAllocation)(uint8_t *allocation);
extern void (*btechHostFreeDosSelector)(uint16_t selector);
/* Query the existing DOS MCB owner; this is not a host-pointer cache key. */
extern uint16_t (*btechHostDosAllocationSelector)(const uint8_t *allocation);
/* Physical foreign-memory edge of the signed, wrapping BA56 directory read.
 * Returns a real recovered byte owner, never a synthetic surface/priority. */
extern const uint8_t *(*btechHostResolveWrappedResourcePriority)(const uint8_t *directory,int16_t resourceId);
typedef struct BtechDosAllocation {uint8_t *bytes;uint32_t availableBytes;} BtechDosAllocation;
extern uint8_t runtimeDelayActive;
extern uint16_t runtimeDelayRemaining,joystickTimingLoopCount;
extern uint8_t joystickPairInstalled[2],joystickAxisCentres[4];
extern uint16_t joystickAxisReciprocals[4];
extern const char *combatInputReplayFilename;
extern uint16_t combatInputReplayMode;
typedef union BtechCharacterClassStorage {
    struct {uint8_t precedingBytes[0x80],classes[0x100];} fields;
    uint8_t bytes[0x180];
} BtechCharacterClassStorage;
extern BtechCharacterClassStorage characterClassStorage;
#define characterClasses characterClassStorage.fields.classes
FILE *BtechOpenStreamWithRetry(const char *filename,const char *mode);
uint16_t BtechGetActiveRuntimeDelayFar(void);
uint16_t BtechMeasureJoystickAxisTiming(uint8_t axisMask);
BtechDosAllocation BtechAllocateDosMemoryBlock(uint32_t bytes);
void BtechFreeDosMemorySelector(uint8_t *allocation);
#endif
