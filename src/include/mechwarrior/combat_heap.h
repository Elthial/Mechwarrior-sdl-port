#ifndef MECHWARRIOR_COMBAT_HEAP_H
#define MECHWARRIOR_COMBAT_HEAP_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
enum {
    BTECH_NEAR_POINTER_BYTES=2,BTECH_NEAR_SOUND_NODE_BYTES=0x10,
    BTECH_NEAR_ARCHIVE_BYTES=0x21,BTECH_NEAR_ARCHIVE_STATE_BYTES=0x80,
    BTECH_NEAR_WORLD_OBJECT_BYTES=0x30,BTECH_NEAR_BITMAP_ENTRY_BYTES=0x0A,
    BTECH_NEAR_STREAM_BUFFER_BYTES=0x200,
    BTECH_NEAR_INITIAL_BREAK=0xE196,BTECH_NEAR_GROWTH_ALIGNMENT=0x2000
};

typedef enum BtechNearOwner {
    BTECH_NEAR_BYTES,BTECH_NEAR_ARCHIVE,BTECH_NEAR_SOUND_NODE,
    BTECH_NEAR_BITMAP_LIST,BTECH_NEAR_BITMAP_IMAGES,BTECH_NEAR_WORLD_OBJECT,
    BTECH_NEAR_STREAM_BUFFER,BTECH_NEAR_SHAPE_BANKS
} BtechNearOwner;
/* Native304E CRT control words; definitions stay in the runtime memory owner. */
extern uint16_t nearHeapFirstHeader,nearHeapSearchHeader,nearHeapEnd;
extern uint16_t nearHeapProgramBreak,nearHeapGrowthAlignment;

/* CDBC/CE5D projections. Native lengths drive heap placement; host sizes and
 * owner kinds describe widened C records, never additional game arguments. */
void *BtechAllocateNearBytes(uint16_t nativeBytes,size_t hostBytes,BtechNearOwner owner);
void *BtechAllocateZeroedNearElements(uint16_t count,uint16_t stride,size_t hostBytes,BtechNearOwner owner);
void BtechReleaseNearHeapBlock(void *record);
void BtechReleaseNearBlockAndProbeAllocation(void *record);
uint16_t BtechNearAllocationOffset(const void *record);
bool BtechNearAllocationIsCurrent(const void *record);
void *BtechNearAllocationOwner(const void *record,BtechNearOwner *owner,uint16_t *displacement);
size_t BtechNearAllocationBytes(const void *record);
bool BtechNearAllocationBytesDefined(const void *record);
/* Representation bridge for the observed stale sound link and E96E byte write.
 * These are not game-field accessors or a generic segment-memory API. */
void BtechHostReadNearSoundLink(void *owner,BtechNearOwner kind,uint16_t displacement,uint16_t words[3]);
void BtechHostWriteNearSoundNext(void *owner,BtechNearOwner kind,uint16_t displacement,uint16_t next);
void BtechHostMarkNearOwnerReleased(void *owner,BtechNearOwner kind,uint16_t displacement);
void *BtechNearObjectAtOffset(uint16_t offset,BtechNearOwner owner);
/* Process/image lifetime, not a game cleanup call. */
void BtechHostDiscardNearHeap(void);

/* Allocator-only header words; no game-data or segment-memory access API. */
uint16_t BtechAllocateNearHeapBlock(uint16_t bytes);
void BtechReleaseNearHeapOffset(uint16_t offset);
uint16_t BtechNearHeapHeader(uint16_t offset);
void BtechSetNearHeapHeader(uint16_t offset,uint16_t value);
void BtechInitializeNearHeap(uint16_t first,uint16_t search,uint16_t end,uint16_t programBreak,uint16_t growthAlignment);
#endif
