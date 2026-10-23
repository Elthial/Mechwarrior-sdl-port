#include "mechwarrior/port_diagnostics.h"
#include "mechwarrior/combat_heap.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* CRT allocator metadata only. Native sizes determine exhaustion/reuse, while
 * widened C records still use host malloc. A logical release retains the host
 * object until child disposal because original sound links can outlive it.
 * This is not a second DS image or a game-field address registry. */
typedef struct NearHeapHeader {
    struct NearHeapHeader *next;
    uint16_t offset,value;
} NearHeapHeader;
typedef struct NearHeapRecord {
    struct NearHeapRecord *next;
    void *object;
    size_t hostBytes;
    uint16_t offset,bytes;
    BtechNearOwner owner;
    bool released,bytesDefined;
} NearHeapRecord;
static NearHeapHeader *headers;
static NearHeapRecord *records;

uint16_t BtechNearHeapHeader(uint16_t offset)
{
    for (NearHeapHeader *header=headers;header;header=header->next)
        if (header->offset==offset) return header->value;
    fprintf(stderr,"Unrepresented near-heap header %04X\n",offset);
    /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("A header that aliases an unrepresented payload is not zero."); /* A header that aliases an unrepresented payload is not zero. */
}
void BtechSetNearHeapHeader(uint16_t offset,uint16_t value)
{
    NearHeapHeader *selected=NULL;
    for (NearHeapHeader *header=headers;header;header=header->next)
    {
        if (header->offset==offset) selected=header;
        else if ((uint16_t)(header->offset+1)==offset)
            header->value=(header->value&0x00FF)|(uint16_t)((value&0x00FF)<<8);
        else if ((uint16_t)(offset+1)==header->offset)
            header->value=(header->value&0xFF00)|(value>>8);
    }
    if (!selected)
    {
        selected=malloc(sizeof(*selected));if (!selected) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !selected");
        *selected=(NearHeapHeader){headers,offset,0};headers=selected;
    }
    selected->value=value;
}
void BtechHostDiscardNearHeap(void)
{
    while (records) {NearHeapRecord *record=records;records=record->next;free(record->object);free(record);}
    while (headers) {NearHeapHeader *header=headers;headers=header->next;free(header);}
    nearHeapFirstHeader=nearHeapSearchHeader=nearHeapEnd=0;
    nearHeapProgramBreak=BTECH_NEAR_INITIAL_BREAK;nearHeapGrowthAlignment=BTECH_NEAR_GROWTH_ALIGNMENT;
}
void BtechInitializeNearHeap(uint16_t first,uint16_t search,uint16_t end,uint16_t nextBreak,uint16_t alignment)
{
    BtechHostDiscardNearHeap();nearHeapFirstHeader=first;nearHeapSearchHeader=search;nearHeapEnd=end;
    nearHeapProgramBreak=nextBreak;nearHeapGrowthAlignment=alignment;
}

/* FBEC->FD38/FDA6, the supported near DS resident block: additions that wrap
 * fail. DFCE has already retained all64KiB, so no DOS resize is needed here. */
static bool ExtendProgramBreak(uint16_t bytes,uint16_t *previous)
{
    if ((uint32_t)nearHeapProgramBreak+bytes>UINT16_MAX) return false;
    *previous=nearHeapProgramBreak;nearHeapProgramBreak+=bytes;return true;
}
/* FBCA. A zero-length query returns the break without touching heap headers.
 * Transient odd/overlapping header writes are significant during alignment. */
static bool ExtendNearHeap(uint16_t bytes,uint16_t *position,uint16_t *payload,uint16_t *oldPayload)
{
    uint16_t previous;
    if (!ExtendProgramBreak(bytes,&previous)) return false;
    if (!bytes) {*oldPayload=previous;return true;} /* FBEC returns ZF from the requested length. */
    *oldPayload=*payload;*payload=*position;*position=previous+bytes;
    BtechSetNearHeapHeader((uint16_t)(*position-2),0xFFFE);
    nearHeapEnd=*position;
    BtechSetNearHeapHeader((uint16_t)(*payload-2),(uint16_t)(*position-*payload-1));
    return true;
}
/* FB90. Round growth using the original descending power-of-two mask, falling
 * back to smaller requests rather than silently granting host-malloc space. */
static bool GrowNearHeap(uint16_t requested,uint16_t *position,uint16_t *payload,uint16_t *oldPayload)
{
    uint16_t needed=requested,header=BtechNearHeapHeader((uint16_t)(*payload-2));
    if (header&1) needed=(uint16_t)(needed-header-1);
    needed+=2;
    uint16_t mask=0x7FFF;
    while (mask>nearHeapGrowthAlignment && mask) mask>>=1;
    for (;;)
    {
        uint32_t end=(uint32_t)needed+*position;
        if (end<=UINT16_MAX && end+mask<=UINT16_MAX)
        {
            uint16_t rounded=(end+mask)&(uint16_t)~mask;
            uint16_t bytes=rounded-*position;
            if (bytes && ExtendNearHeap(bytes,position,payload,oldPayload)) return true;
        }
        if (!mask) return false;
        mask>>=1;
    }
}

static uint16_t FindNearHeapBlock(uint16_t bytes);
/* E980. Initialize the shared heap only on its first allocation request. */
uint16_t BtechAllocateNearHeapBlock(uint16_t bytes)
{
    if (!nearHeapFirstHeader)
    {
        uint16_t previous;
        if (!ExtendProgramBreak(5,&previous)) return 0;
        nearHeapFirstHeader=nearHeapSearchHeader=(uint16_t)((previous+1)&0xFFFE);
        BtechSetNearHeapHeader(nearHeapFirstHeader,1);
        nearHeapEnd=nearHeapFirstHeader+4;BtechSetNearHeapHeader((uint16_t)(nearHeapEnd-2),0xFFFE);
    }
    return FindNearHeapBlock(bytes);
}
/* FAAD. Search from the shared cursor, then restart once. Coalescing is
 * demand-driven; releasing a block alone does not merge it with its neighbour. */
static uint16_t FindNearHeapBlock(uint16_t bytes)
{
    uint16_t requested=bytes+1;
    if (!requested || (requested&0xFFFE)>=0xFFEE) {nearHeapSearchHeader=nearHeapFirstHeader;return 0;}
    requested&=0xFFFE;
    uint16_t position=nearHeapSearchHeader,payload=position+2;
    uint16_t header=BtechNearHeapHeader(position);
    position+=2;
    bool restarted=false;
    for (;;)
    {
        if (header&1)
        {
            uint16_t available=header-1;
            if (available>=requested)
            {
                uint16_t next=position+requested;
                if (available==requested) BtechSetNearHeapHeader((uint16_t)(position-2),(uint16_t)(header-1));
                else
                {
                    BtechSetNearHeapHeader((uint16_t)(position-2),requested);
                    BtechSetNearHeapHeader(next,(uint16_t)(available-requested-1));
                }
                nearHeapSearchHeader=next;return position;
            }
            uint16_t following=position+available;
            uint16_t nextHeader=BtechNearHeapHeader(following);
            if (nextHeader&1)
            {
                header=(uint16_t)(nextHeader+available+2);
                BtechSetNearHeapHeader((uint16_t)(payload-2),header);
                position=payload;continue;
            }
            position=following+2;header=nextHeader;
        }
        if (header!=0xFFFE)
        {
            position+=header;
            header=BtechNearHeapHeader(position);position+=2;payload=position;
            continue;
        }
        if (!restarted)
        {
            restarted=true;position=nearHeapFirstHeader;
            header=BtechNearHeapHeader(position);position+=2;payload=position;continue;
        }
        position=nearHeapEnd;
        uint16_t oldEnd;
        if (!ExtendNearHeap(0,&position,&payload,&oldEnd)) {nearHeapSearchHeader=nearHeapFirstHeader;return 0;}
        if (oldEnd!=position)
        {
            uint16_t alignmentBytes=(oldEnd&1)+2;
            if (!ExtendNearHeap(alignmentBytes,&position,&payload,&oldEnd)) {nearHeapSearchHeader=nearHeapFirstHeader;return 0;}
            uint16_t at=(uint16_t)(payload-2),value=BtechNearHeapHeader(at);
            BtechSetNearHeapHeader(at,(value&0xFF00)|(uint8_t)(value-1));
        }
        if (!GrowNearHeap(requested,&position,&payload,&oldEnd)) {nearHeapSearchHeader=nearHeapFirstHeader;return 0;}
        position=oldEnd-2;header=BtechNearHeapHeader(position);position+=2;payload=position;
    }
}
void BtechReleaseNearHeapOffset(uint16_t offset)
{
    if (offset) BtechSetNearHeapHeader((uint16_t)(offset-2),BtechNearHeapHeader((uint16_t)(offset-2))|1);
}
void *BtechAllocateNearBytes(uint16_t bytes,size_t hostBytes,BtechNearOwner owner)
{
    uint16_t offset=BtechAllocateNearHeapBlock(bytes);
    if (!offset) return NULL;
    NearHeapRecord **link=&records;
    while (*link)
    {
        NearHeapRecord *record=*link;
        if (record->released && record->offset==offset && record->bytes==bytes
            && record->hostBytes==hostBytes && record->owner==owner)
        {
            *link=record->next;record->next=records;records=record;
            record->released=false;return record->object;
        }
        link=&record->next;
    }
    NearHeapRecord *record=malloc(sizeof(*record));void *object=malloc(hostBytes?hostBytes:1);
    if (!record || !object) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !record || !object"); /* Host representation failure is not DOS heap exhaustion. */
    *record=(NearHeapRecord){records,object,hostBytes,offset,bytes,owner,false,false};records=record;
    return object;
}
void *BtechAllocateZeroedNearElements(uint16_t count,uint16_t stride,size_t hostBytes,BtechNearOwner owner)
{
    uint32_t bytes=(uint32_t)count*stride;
    if (bytes>UINT16_MAX) return NULL; /* E9C6 checks the multiply's high WORD. */
    void *object=BtechAllocateNearBytes((uint16_t)bytes,hostBytes,owner);
    if (object) {memset(object,0,hostBytes);records->bytesDefined=true;}return object;
}
size_t BtechNearAllocationBytes(const void *object)
{
    for (NearHeapRecord *record=records;record;record=record->next)
        if (record->object==object) return record->bytes;
    /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Allocation size requested for an unknown near-heap object.");
}
bool BtechNearAllocationBytesDefined(const void *object)
{
    for (NearHeapRecord *record=records;record;record=record->next)
        if (record->object==object) return record->bytesDefined;
    /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Defined-byte count requested for an unknown near-heap object.");
}
uint16_t BtechNearAllocationOffset(const void *object)
{
    if (!object) return 0;
    for (NearHeapRecord *record=records;record;record=record->next)
        if (record->object==object) return record->offset;
    /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Near offset requested for an unknown heap object.");
}
bool BtechNearAllocationIsCurrent(const void *object)
{
    NearHeapRecord *selected=NULL;
    for (NearHeapRecord *record=records;record;record=record->next)
    {
        if (record->object==object) {selected=record;break;}
    }
    if (!selected) return false;
    /* FAAD writes the following split header too. A stale payload beginning
     * exactly there must not keep exposing its old sound-node C fields. */
    for (NearHeapRecord *record=records;record!=selected;record=record->next)
        if ((uint16_t)(record->offset-2)<=selected->offset
            && (uint32_t)selected->offset<(uint32_t)record->offset+((record->bytes+1u)&0xFFFEu)+2) return false;
    return true;
}
void *BtechNearAllocationOwner(const void *object,BtechNearOwner *owner,uint16_t *displacement)
{
    uint16_t offset=BtechNearAllocationOffset(object);
    for (NearHeapRecord *record=records;record;record=record->next)
    {
        if (record->offset<=offset && (uint32_t)offset<(uint32_t)record->offset+record->bytes)
        {
            *owner=record->owner;*displacement=offset-record->offset;return record->object;
        }
        if ((uint16_t)(record->offset-2)<=offset
            && (uint32_t)offset<(uint32_t)record->offset+((record->bytes+1u)&0xFFFEu)+2)
            /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: (uint16_t)(record->offset-2)<=offset && (uint32_t)offset<(uint32_t)record->offset+((record->bytes+1u)&0xFFFEu)+2"); /* Header/padding alias: never fall through to an obsolete C payload. */
    }
    /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Near-heap reference has no matching typed owner.");
}
void *BtechNearObjectAtOffset(uint16_t offset,BtechNearOwner owner)
{
    if (!offset) return NULL;
    for (NearHeapRecord *record=records;record;record=record->next)
        if (record->offset==offset && record->owner==owner) return record->object;
    /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("A corrupt near reference to an unrelated global is not a host pointer."); /* A corrupt near reference to an unrelated global is not a host pointer. */
}
void BtechReleaseNearHeapBlock(void *object)
{
    if (!object) return;
    for (NearHeapRecord *record=records;record;record=record->next)
        if (record->object==object)
        {
            /* E978 is OR BYTE [BX-2],1, even when BX is no longer an
             * allocation base. Check the write address itself: a newer owner
             * can end exactly at BX while still covering its preceding byte. */
            uint16_t address=record->offset-2;
            for (NearHeapRecord *current=records;current!=record;current=current->next)
            {
                if (current->offset<=address && (uint32_t)address<(uint32_t)current->offset+current->bytes)
                {
                    BtechHostMarkNearOwnerReleased(current->object,current->owner,address-current->offset);
                    record->released=true;return;
                }
                if ((uint16_t)(current->offset-2)<=address
                    && (uint32_t)address<(uint32_t)current->offset+((current->bytes+1u)&0xFFFEu)+2)
                {
                    if (address>=current->offset
                        && (uint32_t)address<(uint32_t)current->offset+((current->bytes+1u)&0xFFFEu))
                        /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: address>=current->offset && (uint32_t)address<(uint32_t)current->offset+((current->bytes+1u)&0xFFFEu)"); /* Unrepresented alignment byte. */
                    break; /* Newest allocator header, not an older payload. */
                }
            }
            BtechReleaseNearHeapOffset(record->offset);record->released=true;return;
        }
    /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Released object has no registered near-heap allocation.");
}
void BtechReleaseNearBlockAndProbeAllocation(void *object)
{BtechReleaseNearHeapBlock(object);BtechAllocateNearHeapBlock(UINT16_MAX);}
