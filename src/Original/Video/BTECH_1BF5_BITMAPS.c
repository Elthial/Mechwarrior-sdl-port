#include "mechwarrior/combat_video.h"
#include <stdlib.h>
#include <string.h>

/*1BF5:0ED2. Real BMP:INF:/BIN: loader, normal positive-count EGA path.
 * Typed extent replaces the native zero-terminated near pointer list, not a
 * segment registry. Colour-mode/VGA conversion and destructive heap aliases
 * remain unconverted; they must not be made into successful no-ops. */
BtechBitmapList *BtechLoadBitmapList(const char *filename,BtechRuntimeStream *stream)
{
    BtechBitmapList *list=NULL;
    uint8_t *scratch=NULL;
    BtechResourceAllocation primary={0};
    uint16_t loaded=0,openedHere=0,auxiliaryBytes;
    int16_t archiveCodeOrError=0;
    int16_t slot=BtechFindFreeBitmapRegistrySlot();
    if (slot<0) abort(); // Native cleanup reads an uninitialized opened-here WORD and dereferences list zero.
    if (!stream || !BtechIsRegisteredTaggedStream(stream))
    {stream=BtechOpenTaggedResourceStream(filename);openedHere=1;}
    if (BtechFindResourceTagPosition(stream,"BMP:INF:",0)==UINT32_MAX) goto cleanup;
    uint8_t countBytes[2];
    if (fread(countBytes,2,1,stream->file)!=1) goto cleanup;
    int16_t count=(int16_t)(countBytes[0]|((uint16_t)countBytes[1]<<8));
    if (count<=0 || (uint32_t)(uint16_t)count*0x0A>UINT16_MAX) abort(); // Native wrapped/zero near allocations are not host arrays.
    list=malloc(sizeof(*list));
    if (!list) goto cleanup;
    list->imageCount=(uint16_t)count;list->pixels=(BtechResourceAllocation){0};
    list->images=malloc((size_t)count*sizeof(*list->images));
    if (!list->images) goto cleanup;
    for (uint16_t image=0;image<(uint16_t)count;++image)
    {
        uint8_t width[2];
        if (fread(width,2,1,stream->file)!=1) goto cleanup;
        list->images[image].widthPixels=width[0]|((uint16_t)width[1]<<8);
    }
    for (uint16_t image=0;image<(uint16_t)count;++image)
    {
        uint8_t height[2];
        if (fread(height,2,1,stream->file)!=1) goto cleanup;
        list->images[image].heightPixels=height[0]|((uint16_t)height[1]<<8);
    }
    if (!graphicsBitmapStorageSize) abort();
    primary.byteCount=graphicsBitmapStorageSize(list,&auxiliaryBytes);
    primary.bytes=BtechAllocateDosMemoryBlock(primary.byteCount).bytes;
    if (!primary.bytes) goto cleanup;
    if (auxiliaryBytes) abort(); // EGA1EB0 writes zero; other backend allocation/preparation is not converted.
    scratch=malloc(COMBAT_BITMAP_WORKSPACE_BYTES);
    if (scratch)
    {
        borrowedArchiveWorkspace=(BtechLzwReaderWorkspace *)scratch;
        borrowedArchiveWorkspace=(BtechLzwReaderWorkspace *)(scratch+0x10);
        borrowedArchiveWorkspace=(BtechLzwReaderWorkspace *)((uintptr_t)borrowedArchiveWorkspace&~(uintptr_t)0x0F);
    } // Paragraph alignment is actual borrowed decoder storage, not an address-to-object lookup.
    if (BtechFindResourceTagPosition(stream,"BMP:BIN:",0)==UINT32_MAX) goto cleanup;
    uint32_t archiveBytes=BtechGetTaggedPayloadLength(stream);
    int16_t opened=BtechOpenCompressedArchive(0,stream->file,"r",archiveBytes);
    archiveCodeOrError=opened<0?1:0; // Native bug: initial successful handle is discarded, index0 is used.
    if (archiveCodeOrError) goto cleanup;
    uint32_t cursor=0;
    uint8_t decodedChunk[COMBAT_BITMAP_READ_BYTES]; // Private IO transport, as in native resource DC86's C17 projection.
    do
    {
        if (cursor>primary.byteCount) abort();
        uint32_t remaining=primary.byteCount-cursor;
        uint16_t preserved=(uint16_t)(remaining<COMBAT_BITMAP_READ_BYTES?remaining:COMBAT_BITMAP_READ_BYTES);
        /* RLE literals account their requested size even after short IO.
         * Seed private staging from the actual destination so its unwritten
         * bytes remain allocation residue, not unrelated host stack residue. */
        memcpy(decodedChunk,primary.bytes+cursor,preserved);
        uint16_t read=BtechReadArchiveIntoFarBuffer((uint16_t)archiveCodeOrError,decodedChunk,COMBAT_BITMAP_READ_BYTES);
        if (cursor>primary.byteCount || read>primary.byteCount-cursor) abort();
        memcpy(primary.bytes+cursor,decodedChunk,read);
        if (read!=COMBAT_BITMAP_READ_BYTES) break;
        cursor+=COMBAT_BITMAP_READ_BYTES;
    } while (1);
    uint32_t decodedBytes=BtechGetArchiveDecodedLength((uint16_t)archiveCodeOrError);
    if (!graphicsPrepareBitmapList) abort();
    graphicsPrepareBitmapList(list,primary,decodedBytes);
    BtechFlushAndCloseCompressedArchive((uint16_t)archiveCodeOrError);
    loaded=1;
    if (graphicsColourMode) abort(); // Native optional VGA path includes real leak/double-free quirks, not a stub.
    BtechRegisterBitmapList(list,(uint16_t)count,(uint16_t)slot);
cleanup:
    if (scratch)
    {
        free(scratch);
        borrowedArchiveWorkspace=NULL;
    }
    if (!loaded)
    {
        if (primary.bytes) BtechFreeDosMemorySelector(primary.bytes);
        if (archiveCodeOrError) BtechFlushAndCloseCompressedArchive((uint16_t)archiveCodeOrError);
        BtechReleaseBitmapListNearBlocks(list);
        list=NULL;
    }
    if (openedHere) BtechCloseTaggedResourceStream(stream);
    return list;
}

/*1BF5:1336. Native dereferences list[0] before its null check. */
void BtechReleaseBitmapListNearBlocks(BtechBitmapList *list)
{
    if (!list) abort(); // Null native near-address alias is not a nullable host list.
    if (list->images) free(list->images);
    free(list);
}

/*1BF5:135E. Registry removal precedes DOS release and the two near frees. */
void BtechFreeRegisteredBitmapList(BtechBitmapList *list)
{
    if (BtechIsBitmapListRegistered(list))
    {
        BtechUnregisterBitmapList(list);
        BtechFreeDosMemorySelector(list->pixels.bytes);
        BtechReleaseBitmapListNearBlocks(list);
    }
}

/*1BF5:13AE. Counts and list pointers are deliberately independent. */
int16_t BtechFindFreeBitmapRegistrySlot(void)
{
    for (int16_t slot=COMBAT_BITMAP_LISTS-1;slot>=0;--slot)
        if (!bitmapRegistryCounts[slot]) return slot;
    return -1;
}

/*1BF5:13D8. Highest matching pointer only; clear pointer before count. */
void BtechUnregisterBitmapList(BtechBitmapList *list)
{
    if (!list) return;
    for (int16_t slot=COMBAT_BITMAP_LISTS-1;slot>=0;--slot)
        if (bitmapRegistryLists[slot]==list)
        {bitmapRegistryLists[slot]=NULL;bitmapRegistryCounts[slot]=0;return;}
}

/*1BF5:141C. Count store precedes pointer store. */
void BtechRegisterBitmapList(BtechBitmapList *list,uint16_t count,uint16_t slot)
{
    if (!list) return;
    if (slot>=COMBAT_BITMAP_LISTS) abort(); // Native wrapped adjacent-owner writes are outside typed registry scope.
    bitmapRegistryCounts[slot]=count;
    bitmapRegistryLists[slot]=list;
}

/*1BF5:1440. Highest match wins, even when its count is zero. */
uint16_t BtechGetRegisteredBitmapCount(const BtechBitmapList *list)
{
    if (list)
        for (int16_t slot=COMBAT_BITMAP_LISTS-1;slot>=0;--slot)
            if (bitmapRegistryLists[slot]==list) return bitmapRegistryCounts[slot];
    return 0;
}

/*1BF5:1482. Counts are not consulted. */
uint16_t BtechIsBitmapListRegistered(const BtechBitmapList *list)
{
    if (list)
        for (int16_t slot=COMBAT_BITMAP_LISTS-1;slot>=0;--slot)
            if (bitmapRegistryLists[slot]==list) return 1;
    return 0;
}
