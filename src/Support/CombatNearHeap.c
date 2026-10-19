#include "mechwarrior/combat_heap.h"
#include "mechwarrior/combat_sound.h"
#include "mechwarrior/combat_world.h"
#include "mechwarrior/combat_video.h"
#include <stdlib.h>

/* Only the observed six-byte sound view, two-byte next store and E978 byte
 * write cross this boundary. No persistent DS mirror: numeric fields are read
 * from, and writes immediately applied to, their actual typed C owners. */
typedef struct NearAliasTransfer {
    uint16_t start;
    uint8_t count,covered,bytes[6];
    bool write,markFree;
} NearAliasTransfer;

static uint32_t TransferScalar(NearAliasTransfer *transfer,uint16_t offset,uint32_t value,uint8_t width)
{
    for (uint8_t byte=0;byte<width;++byte)
    {
        uint16_t address=offset+byte;
        if (address<transfer->start || address>=transfer->start+transfer->count) continue;
        uint8_t index=(uint8_t)(address-transfer->start),shift=byte*8;
        transfer->covered|=(uint8_t)(1u<<index);
        if (transfer->markFree) value|=UINT32_C(1)<<shift;
        else if (transfer->write)
            value=(value&~(UINT32_C(0xFF)<<shift))|((uint32_t)transfer->bytes[index]<<shift);
        else transfer->bytes[index]=(uint8_t)(value>>shift);
    }
    return value;
}

/* Foreign pointer writes cannot be replaced with host pointer arithmetic.
 * Reject an actual change to an unrepresented pointer, not an unrelated scalar
 * access in the same record. */
static void TransferReference(NearAliasTransfer *transfer,uint16_t offset,uint16_t identity)
{
    if (TransferScalar(transfer,offset,identity,2)!=identity) abort();
}

static bool TransferTouches(const NearAliasTransfer *transfer,uint16_t offset,size_t bytes)
{
    return offset<transfer->start+transfer->count && offset+bytes>transfer->start;
}
#define FIELD(offset,field) do { if (TransferTouches(transfer,offset,sizeof(field))) \
    (field)=TransferScalar(transfer,offset,(uint32_t)(field),sizeof(field)); } while (0)
static void TransferArchive(BtechArchiveDescriptor *archive,NearAliasTransfer *transfer)
{
    TransferReference(transfer,0,BtechNearAllocationOffset(archive->state));
    if (!archive->workspace || archive->workspaceSelector)
    {
        TransferReference(transfer,2,0);
        TransferReference(transfer,4,archive->workspaceSelector);
    }
    if (!archive->stream || archive->streamIdentity) TransferReference(transfer,6,archive->streamIdentity);
    FIELD(8,archive->sourceSelector);
    FIELD(0x0A,archive->compressedPosition);
    FIELD(0x0E,archive->compressedBytes);
    FIELD(0x12,archive->decodedBytes);
    FIELD(0x16,archive->decodedPosition);
    FIELD(0x1A,archive->pendingEnd);FIELD(0x1B,archive->pendingRead);
    FIELD(0x1C,archive->initialStreamPosition);FIELD(0x20,archive->methodAndFlags);
}

static void TransferWorldObject(BtechHeapWorldObject *object,NearAliasTransfer *transfer)
{
    BtechWorldObjectPrefix *prefix=&object->prefix;
    FIELD(0,prefix->renderFlags);FIELD(1,prefix->flags);FIELD(2,prefix->resourceId);
    FIELD(4,prefix->resourceScaledValue);
    FIELD(8,prefix->position.positionA);FIELD(0x0C,prefix->position.positionB);FIELD(0x10,prefix->position.positionC);
    FIELD(0x14,prefix->orientationA);FIELD(0x16,prefix->orientationB);FIELD(0x18,prefix->orientationC);
    FIELD(0x1A,prefix->updateCallbackOffset);
    if (!object->entryVariants) TransferReference(transfer,0x1C,0);
    FIELD(0x1E,object->projectedScreenA);FIELD(0x20,object->projectedScreenB);FIELD(0x22,object->projectionValue);
    for (uint16_t axis=0;axis<3;++axis) FIELD(0x24+axis*4,object->transformedPosition[axis]);
}

static void TransferOwner(void *owner,BtechNearOwner kind,NearAliasTransfer *transfer)
{
    if ((uint32_t)transfer->start+transfer->count>BtechNearAllocationBytes(owner)) abort();
    if (kind==BTECH_NEAR_ARCHIVE) TransferArchive(owner,transfer);
    else if (kind==BTECH_NEAR_WORLD_OBJECT) TransferWorldObject(owner,transfer);
    else if (kind==BTECH_NEAR_BYTES && BtechNearAllocationBytesDefined(owner))
    {
        uint8_t *bytes=owner;
        for (uint8_t byte=0;byte<transfer->count;++byte)
            FIELD(transfer->start+byte,bytes[transfer->start+byte]);
    }
    else if (kind==BTECH_NEAR_SOUND_NODE)
    {
        BtechSoundAllocationNode *node=owner;
        FIELD(0,node->allocationOffset);FIELD(2,node->allocationSelector);
        if (TransferTouches(transfer,4,2))
        {
            uint16_t next=BtechNearAllocationOffset(node->next);
            next=(uint16_t)TransferScalar(transfer,4,next,2);
            node->next=BtechNearObjectAtOffset(next,BTECH_NEAR_SOUND_NODE);
        }
        for (uint16_t byte=0;byte<sizeof(node->chunkTag);++byte) FIELD(6+byte,node->chunkTag[byte]);
    }
    else if (kind==BTECH_NEAR_BITMAP_LIST)
    {
        BtechBitmapList *list=owner;
        uint16_t images=BtechNearAllocationOffset(list->images);
        for (uint16_t image=0;image<=list->imageCount;++image)
            TransferReference(transfer,image*2,image==list->imageCount?0:(uint16_t)(images+image*BTECH_NEAR_BITMAP_ENTRY_BYTES));
    }
    else if (kind==BTECH_NEAR_BITMAP_IMAGES)
    {
        BtechBitmap *images=owner;
        size_t count=BtechNearAllocationBytes(owner)/BTECH_NEAR_BITMAP_ENTRY_BYTES;
        for (uint16_t image=0;image<count;++image)
        {
            /* Native INF stores widths at +6 and heights at +8. The FAR image
             * pointers need selector/paragraph provenance, not host casts. */
            FIELD(image*BTECH_NEAR_BITMAP_ENTRY_BYTES+6,images[image].widthPixels);
            FIELD(image*BTECH_NEAR_BITMAP_ENTRY_BYTES+8,images[image].heightPixels);
        }
    }
    if (transfer->covered!=(1u<<transfer->count)-1) abort();
}
#undef FIELD

void BtechHostReadNearSoundLink(void *owner,BtechNearOwner kind,uint16_t displacement,uint16_t words[3])
{
    NearAliasTransfer transfer={.start=displacement,.count=6};
    TransferOwner(owner,kind,&transfer);
    for (uint8_t word=0;word<3;++word)
        words[word]=transfer.bytes[word*2]|((uint16_t)transfer.bytes[word*2+1]<<8);
}
void BtechHostWriteNearSoundNext(void *owner,BtechNearOwner kind,uint16_t displacement,uint16_t next)
{
    NearAliasTransfer transfer={.start=(uint16_t)(displacement+4),.count=2,.write=true,
        .bytes={(uint8_t)next,(uint8_t)(next>>8)}};
    TransferOwner(owner,kind,&transfer);
}
void BtechHostMarkNearOwnerReleased(void *owner,BtechNearOwner kind,uint16_t displacement)
{
    NearAliasTransfer transfer={.start=displacement,.count=1,.markFree=true};
    TransferOwner(owner,kind,&transfer);
}

BtechSoundAllocationNode *BtechHostResolveReleasedSoundNode(BtechSoundAllocationNode *node)
{
    if (BtechNearAllocationIsCurrent(node)) return node;
    BtechNearOwner kind;uint16_t displacement;
    void *owner=BtechNearAllocationOwner(node,&kind,&displacement);
    if (kind==BTECH_NEAR_SOUND_NODE && !displacement) return owner;
    uint16_t words[3];BtechHostReadNearSoundLink(owner,kind,displacement,words);
    node->allocationOffset=words[0];node->allocationSelector=words[1];
    node->allocation=(BtechResourceAllocation){0};
    node->next=BtechNearObjectAtOffset(words[2],BTECH_NEAR_SOUND_NODE);
    return node;
}

void BtechHostStoreSoundAllocationNext(BtechSoundAllocationNode *node,BtechSoundAllocationNode *next)
{
    /* Re-resolve after malloc, which may itself reuse this address. The caller
     * handles the embedded bank root directly; it is not a heap allocation. */
    if (BtechNearAllocationIsCurrent(node)) {node->next=next;return;}
    BtechNearOwner kind;uint16_t displacement;
    void *owner=BtechNearAllocationOwner(node,&kind,&displacement);
    BtechHostWriteNearSoundNext(owner,kind,displacement,BtechNearAllocationOffset(next));
    node->next=next;
}
