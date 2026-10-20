#include "mechwarrior/combat_resources.h"
#include "mechwarrior/combat_heap.h"
#include "mechwarrior/native_address.h"
#include "mechwarrior/combat_video.h"
#include "mechwarrior/combat_installation.h"
#include <stdlib.h>
#include <string.h>

/*22B6:05DE. ADD/ADC of selector:offset from a paragraph-aligned allocation
 * becomes typed resource references. Internal near addresses must still use
 * paragraphBase, NOT the header pointer. No count-prefix is invented. */
BtechResourceBank BtechDecodeAndRelocateResourceBank(const char *filename)
{
    BtechResourceBank bank={0};
    bank.allocation=BtechAllocateAndDecodeResource(filename);
    uint16_t count=0;
    for (;;)
    {
        uint16_t offset=count*4;
        if ((uint32_t)offset+4>bank.allocation.byteCount) abort();
        if (!(MwReadNativeWord(bank.allocation.bytes,offset)|MwReadNativeWord(bank.allocation.bytes,offset+2))) break;
        if (++count==0x4000) abort(); /* Missing terminator would wrap the native cursor. */
    }
    bank.resources=calloc((size_t)count+1,sizeof(*bank.resources));
    bank.decodedResources=calloc(count?count:1,sizeof(*bank.decodedResources));
    bank.decodedResourceCount=count;
    if (!bank.resources || !bank.decodedResources) abort();
    for (uint16_t index=0;index<count;++index)
    {
        uint16_t offset=MwReadNativeWord(bank.allocation.bytes,index*4);
        uint16_t paragraph=MwReadNativeWord(bank.allocation.bytes,index*4+2);
        uint32_t paragraphBytes=(uint32_t)paragraph<<4;
        if (offset>0xFFF2 || paragraphBytes+offset+14>bank.allocation.byteCount) abort();
        BtechShapeResource *resource=&bank.decodedResources[index];
        bank.resources[index]=resource;
        resource->paragraphBase=bank.allocation.bytes+paragraphBytes;
        resource->header=resource->paragraphBase+offset;
        resource->flags=resource->header[0];
        resource->scaleShift=resource->header[1];
        resource->baseScale=(int16_t)MwReadNativeWord(resource->header,2);
        for (uint16_t axis=0;axis<3;++axis) resource->referenceVector[axis]=MwReadNativeWord(resource->header,4+axis*2);
        resource->entryCount=(int16_t)MwReadNativeWord(resource->header,0x0A);
        uint16_t entryOffset=MwReadNativeWord(resource->header,0x0C);
        if (resource->entryCount<=0) continue;
        resource->entries=calloc((uint16_t)resource->entryCount,sizeof(*resource->entries));
        if (!resource->entries) abort();
        for (int16_t entryIndex=0;entryIndex<resource->entryCount;++entryIndex,entryOffset+=8)
        {
            if (entryOffset>0xFFF8 || paragraphBytes+entryOffset+8>bank.allocation.byteCount) abort();
            const uint8_t *encodedEntry=resource->paragraphBase+entryOffset;
            BtechRenderEntry *entry=&resource->entries[entryIndex];
            entry->firstVectorIndex=encodedEntry[0];entry->vectorCount=encodedEntry[1];
            uint16_t vectorOffset=MwReadNativeWord(encodedEntry,2);
            entry->variantCount=MwReadNativeWord(encodedEntry,4);
            uint16_t variantOffset=MwReadNativeWord(encodedEntry,6);
            uint16_t vectorCount=entry->vectorCount;
            if (vectorCount<=entry->firstVectorIndex) vectorCount=(uint16_t)entry->firstVectorIndex+1;
            entry->vectors=calloc(vectorCount,sizeof(*entry->vectors));
            if (!entry->vectors) abort();
            for (uint16_t vector=0;vector<vectorCount;++vector,vectorOffset+=6)
            {
                if (vectorOffset>0xFFFA || paragraphBytes+vectorOffset+6>bank.allocation.byteCount) abort();
                for (uint16_t axis=0;axis<3;++axis) entry->vectors[vector][axis]=MwReadNativeWord(resource->paragraphBase,vectorOffset+axis*2);
            }
            /* A zero variant limit still selects row zero in19CE. Its count
             * zero LOD backs up one eight-byte record; retain that neighbour. */
            uint32_t variants=entry->variantCount?entry->variantCount:1;
            entry->variants=calloc(variants,sizeof(*entry->variants));
            if (!entry->variants) abort();
            for (uint32_t variantIndex=0;variantIndex<variants;++variantIndex,variantOffset+=4)
            {
                if (variantOffset>0xFFFC || paragraphBytes+variantOffset+4>bank.allocation.byteCount) abort();
                BtechRenderVariant *variant=&entry->variants[variantIndex];
                variant->primitiveCount=(int16_t)MwReadNativeWord(resource->paragraphBase,variantOffset);
                uint16_t primitiveOffset=MwReadNativeWord(resource->paragraphBase,variantOffset+2);
                uint16_t primitives=variant->primitiveCount>0?(uint16_t)variant->primitiveCount:1;
                BtechRenderPrimitive *storage=calloc((size_t)primitives+1,sizeof(*storage));
                if (!storage) abort();
                variant->primitives=storage+1;
                for (int32_t primitiveIndex=variant->primitiveCount==0?-1:0;primitiveIndex<primitives;++primitiveIndex)
                {
                    uint16_t at=primitiveOffset+primitiveIndex*8;
                    if (at>0xFFF8 || paragraphBytes+at+8>bank.allocation.byteCount) abort();
                    BtechRenderPrimitive *primitive=&variant->primitives[primitiveIndex];
                    primitive->scaleLimit=resource->paragraphBase[at];
                    primitive->kind=resource->paragraphBase[(uint16_t)(at+1)];
                    const uint8_t *encodedPayload=resource->paragraphBase+(uint16_t)(at+2);
                    if (primitive->kind==RENDER_PRIMITIVE_ELLIPSE)
                        primitive->ellipsePayload=(BtechRenderEllipse){(int16_t)MwReadNativeWord(encodedPayload,0),encodedPayload[2],encodedPayload[3],encodedPayload[4],encodedPayload[5]};
                    else if (primitive->kind!=RENDER_PRIMITIVE_POLYGON) primitive->encodedPayload=encodedPayload;
                    if (primitive->kind==RENDER_PRIMITIVE_POLYGON)
                    {
                        primitive->polygonPayload.polygonCount=(int16_t)MwReadNativeWord(encodedPayload,0);
                        uint16_t polygonOffset=MwReadNativeWord(encodedPayload,2);
                        /* DEC/JGE uses overflow as well as sign. Count8000
                         * exits even though the decremented WORD is7FFF. */
                        uint16_t polygons=primitive->polygonPayload.polygonCount>0?(uint16_t)primitive->polygonPayload.polygonCount:0;
                        if (!polygons) continue;
                        primitive->polygonPayload.polygons=calloc(polygons,sizeof(*primitive->polygonPayload.polygons));
                        if (!primitive->polygonPayload.polygons) abort();
                        for (uint16_t polygonIndex=0;polygonIndex<polygons;++polygonIndex,polygonOffset+=8)
                        {
                            if (polygonOffset>0xFFF8 || paragraphBytes+polygonOffset+8>bank.allocation.byteCount) abort();
                            const uint8_t *encodedPolygon=resource->paragraphBase+polygonOffset;
                            BtechRenderPolygon *polygon=&primitive->polygonPayload.polygons[polygonIndex];
                            polygon->flags=encodedPolygon[0];polygon->fillColour=encodedPolygon[1];polygon->lineColour=encodedPolygon[2];
                            polygon->alternateFillColour=encodedPolygon[3];polygon->alternateLineColour=encodedPolygon[4];polygon->normalVectorIndex=encodedPolygon[5];
                            uint16_t indicesOffset=MwReadNativeWord(encodedPolygon,6);
                            uint32_t available=bank.allocation.byteCount-paragraphBytes;
                            if (available>0x10000) available=0x10000;
                            if (indicesOffset>=available || !memchr(resource->paragraphBase+indicesOffset,POLYGON_VERTEX_LIST_END,available-indicesOffset)) abort();
                            polygon->vertexIndices=resource->paragraphBase+indicesOffset;
                        }
                    }
                }
            }
        }
    }
    /* Count is established by the original bind/count callees, not guessed
     * from resource sizes. Additional allocations only decode binary views;
     * no new game entries, geometry or variants are synthesized. Malformed
     * physical overreads/field-straddling near wraps remain unsupported. */
    return bank;
}

/*22B6:02DE. Same relocation kernel, but independent collision-bank ownership.
 * Keep this original boundary rather than alias collision and render banks. */
void BtechLoadCollisionResourceBank(const char *filename)
{
    BtechResourceAllocation allocation=BtechAllocateAndDecodeResource(filename);
    surfaceResourceBank.allocation=allocation;
    uint16_t count=0;
    for (;;)
    {
        uint16_t cursor=count*4;
        if ((uint32_t)cursor+4>allocation.byteCount) abort();
        if (!(MwReadNativeWord(allocation.bytes,cursor)|MwReadNativeWord(allocation.bytes,cursor+2))) break;
        if (++count==0x4000) abort();
    }
    surfaceResourceBank.surfaces=calloc((size_t)count+1,sizeof(BtechSurfaceDefinition *));
    if (!surfaceResourceBank.surfaces) abort();
    for (uint16_t index=0;index<count;++index)
    {
        uint16_t offset=MwReadNativeWord(allocation.bytes,index*4),paragraph=MwReadNativeWord(allocation.bytes,index*4+2);
        uint32_t paragraphBytes=(uint32_t)paragraph<<4;
        if ((uint32_t)offset+10>0x10000 || paragraphBytes+offset+10>allocation.byteCount) abort();
        /* Duplicate relocation pairs denote ONE immutable surface, preserving
         * descriptor identity in cached surface queries. This is IO only. */
        for (uint16_t prior=0;prior<index;++prior)
            if (MwReadNativeWord(allocation.bytes,prior*4)==offset && MwReadNativeWord(allocation.bytes,prior*4+2)==paragraph)
            {surfaceResourceBank.surfaces[index]=surfaceResourceBank.surfaces[prior];break;}
        if (surfaceResourceBank.surfaces[index]) continue;
        const uint8_t *base=allocation.bytes+paragraphBytes,*header=base+offset;
        BtechSurfaceDefinition *surface=calloc(1,sizeof(*surface));if (!surface) abort();
        surfaceResourceBank.surfaces[index]=surface;
        surface->extentA=MwReadNativeWord(header,0);surface->extentB=MwReadNativeWord(header,2);
        surface->directResultFlag=header[4];surface->priorityClass=header[5];
        surface->surfaceClass=header[6];surface->patchCount=header[7];
        uint16_t table=MwReadNativeWord(header,8);
        if (surface->patchCount)
        {
            surface->patches=calloc(surface->patchCount,sizeof(BtechSurfacePatch));
            if (!surface->patches) abort();
        }
        for (uint16_t patchIndex=0;patchIndex<surface->patchCount;++patchIndex)
        {
            uint16_t at=table+patchIndex*0x10;
            if ((uint32_t)at+0x10>0x10000 || paragraphBytes+at+0x10>allocation.byteCount) abort();
            BtechSurfacePatch *patch=&surface->patches[patchIndex];
            for (uint16_t axis=0;axis<3;++axis) patch->orderingPlaneCoefficients[axis]=(int16_t)MwReadNativeWord(base,at+axis*2);
            patch->heightProductScale=base[at+6];patch->orientationResponseMagnitude=base[at+7];
            patch->orientationResponseHeading=base[at+8];patch->boundaryCount=base[at+9];
            patch->baseHeight=(int16_t)MwReadNativeWord(base,at+0x0C);
            uint16_t boundaryOffset=MwReadNativeWord(base,at+0x0A);
            uint16_t boundaryCount=patch->boundaryCount?patch->boundaryCount:1; /* C8C0 reads first even when count0. */
            patch->boundaries=calloc(boundaryCount,sizeof(BtechSurfaceBoundary));if (!patch->boundaries) abort();
            for (uint16_t boundary=0;boundary<=boundaryCount;++boundary)
            {
                uint16_t cursor=boundary==boundaryCount?MwReadNativeWord(base,at+0x0E):(uint16_t)(boundaryOffset+boundary*8);
                if ((uint32_t)cursor+8>0x10000 || paragraphBytes+cursor+8>allocation.byteCount) abort();
                BtechSurfaceBoundary *target=boundary==boundaryCount?&patch->heightBoundary:&patch->boundaries[boundary];
                *target=(BtechSurfaceBoundary){(int16_t)MwReadNativeWord(base,cursor),(int16_t)MwReadNativeWord(base,cursor+2),
                    (int16_t)MwReadNativeWord(base,cursor+4),(int16_t)MwReadNativeWord(base,cursor+6)};
            }
        }
    }
    surfaceResourceBank.resourceCount=count;
}

/*22B6:04FA. Initial bank is copied; remaining slots are all cleared. */
void BtechAllocateShapeBankRegistry(uint16_t count)
{
    BtechHostRetainDetachedShapeRegistry(); /* Host ownership of the native orphan, no early free. */
    shapeResourceBankCount=count;
    if (!count || (uint32_t)count*6>UINT16_MAX) abort();
    enum { NATIVE_SHAPE_BANK_BYTES=6 };
    shapeResourceBanks=BtechAllocateNearBytes((uint16_t)(count*NATIVE_SHAPE_BANK_BYTES),(size_t)count*sizeof(*shapeResourceBanks),BTECH_NEAR_SHAPE_BANKS);
    if (!shapeResourceBanks) abort();
    shapeResourceBanks[0]=initialShapeResourceBank;
    for (uint16_t slot=1;slot<count;++slot) shapeResourceBanks[slot]=(BtechResourceBank){0};
}

/*22B6:034E. Native leaves its far pair/count stale until the next load.
 * Host decoded references become invalid at paragraph free; no live surface
 * query between release/reload is supported. Count is deliberately retained. */
void BtechReleaseCollisionBankAllocation(void)
{
    BtechFreeDosMemorySelector(surfaceResourceBank.allocation.bytes);
    BtechHostDiscardDecodedSurfaceBank(&surfaceResourceBank);
    surfaceResourceBank.allocation.bytes=NULL; /* Released host ownership only. */
}

/*22B6:064A. Native forwards the original two WORD far pair;1F10 consumes
 * only its selector. The wider C bank also carries host decoded ownership. */
void BtechReleaseResourceBankSelector(BtechResourceBank bank)
{
    BtechFreeDosMemorySelector(bank.allocation.bytes);
    BtechHostDiscardDecodedShapeBank(&bank);
}

/*22B6:06C4. Return allocation before clearing live directory/count. The host
 * decoded owner travels with that detached allocation, not another live bank. */
BtechResourceBank BtechDetachShapeResourceBank(uint16_t bankSlot)
{
    if ((int16_t)bankSlot<0 || (int16_t)bankSlot>=(int16_t)shapeResourceBankCount)
        BtechShowFatalErrorAndExit("bad shape list"); /*304E:2D83*/
    BtechResourceBank bank=shapeResourceBanks[bankSlot];
    shapeResourceBanks[bankSlot]=(BtechResourceBank){0};
    return bank;
}

/*22B6:056A. Counts until the zero far pair, independent of cached count. */
uint16_t BtechCountShapeResourceBankEntries(uint16_t bankSlot)
{
    if (bankSlot&0x8000 || bankSlot>=shapeResourceBankCount) abort();
    const BtechResourceBank *bank=&shapeResourceBanks[bankSlot];
    uint16_t count=0;
    if (bank->resources) while (bank->resources[count]) ++count;
    return count;
}

/*22B6:065E. Validating and storing the directory precedes counting it. */
void BtechBindShapeBankDirectory(BtechResourceBank bank,uint16_t bankSlot)
{
    if (bankSlot&0x8000 || bankSlot>=shapeResourceBankCount || shapeResourceBanks[bankSlot].resources) abort();
    shapeResourceBanks[bankSlot]=bank;
    shapeResourceBanks[bankSlot].resourceCount=BtechCountShapeResourceBankEntries(bankSlot);
}

/*22B6:0714. Null banks still subtract their counts. Signed live bank bound. */
const BtechShapeResource *BtechLookupBankResource(uint16_t resourceId)
{
    for (uint16_t slot=0;!(shapeResourceBankCount&0x8000) && slot<shapeResourceBankCount;++slot)
    {
        const BtechResourceBank *bank=&shapeResourceBanks[slot];
        if (bank->resources && resourceId<bank->resourceCount) return bank->resources[resourceId];
        resourceId-=bank->resourceCount;
    }
    return NULL;
}

/*22B6:0780. Replace the live directory reference, not a copy of its header.
 * Null banks still consume their counts; selector:offset becomes a typed ref. */
void BtechSetShapeResourceEntryAddress(uint16_t resourceId,BtechShapeResource *resource)
{
    for (uint16_t slot=0;(int16_t)slot<(int16_t)shapeResourceBankCount;++slot)
    {
        BtechResourceBank *bank=&shapeResourceBanks[slot];
        if (bank->resources && resourceId<bank->resourceCount)
        {bank->resources[resourceId]=resource;return;}
        resourceId-=bank->resourceCount;
    }
    BtechShowFatalErrorAndExit("bad shpno"); /*304E:2D92. */
}

/*22B6:1E28. Signed WORD scale, repeated DWORD SHL with wrap. */
uint32_t BtechGetResourceScaledValue(uint16_t resourceId)
{
    const BtechShapeResource *resource=BtechLookupBankResource(resourceId);
    uint32_t scale=0;
    if (resource)
    {
        scale=(uint32_t)resource->baseScale;
        for (uint8_t shifts=resource->scaleShift;shifts;--shifts) scale<<=1;
    }
    return scale;
}

/*22B6:1E64. Missing resource returns zero; the shift is an unsigned BYTE. */
uint16_t BtechGetResourceScaleShift(uint16_t resourceId)
{
    const BtechShapeResource *resource=BtechLookupBankResource(resourceId);
    return resource?resource->scaleShift:0;
}
