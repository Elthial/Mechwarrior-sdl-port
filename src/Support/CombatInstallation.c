#include "mechwarrior/combat_installation.h"
#include "mechwarrior/combat_startup.h"
#include "mechwarrior/combat_import.h"
#include "mechwarrior/combat_sound.h"
#include "mechwarrior/combat_heap.h"
#include "mechwarrior/combat_video.h"
#include "mechwarrior/combat_orders.h"
#include "mechwarrior/combat_cockpit.h"
#include "mechwarrior/combat_render.h"
#include "mechwarrior/combat_pose.h"
#include "mechwarrior/combat_scene.h"
#include "mechwarrior/native_address.h"
#include <stdlib.h>
#include <string.h>

void (*btechHostReleaseChildDosAllocations)(void);

/*04FA overwrites its old near allocation without freeing it. Preserve that
 * lifetime through child disposal, without losing the host allocation owner. */
typedef struct RetiredShapeRegistry {
    BtechResourceBank *banks;
    struct RetiredShapeRegistry *next;
} RetiredShapeRegistry;
static RetiredShapeRegistry *retiredShapeRegistries;

void BtechHostRetainDetachedShapeRegistry(void)
{
    if (!shapeResourceBanks || shapeResourceBanks==&initialShapeResourceBank) return;
    for (uint16_t bank=0;bank<shapeResourceBankCount;++bank)
        if (shapeResourceBanks[bank].resources || shapeResourceBanks[bank].decodedResources) abort();
    RetiredShapeRegistry *retired=malloc(sizeof(*retired));if (!retired) abort();
    *retired=(RetiredShapeRegistry){shapeResourceBanks,retiredShapeRegistries};retiredShapeRegistries=retired;
}

/* Decoded host-only allocations have no counterpart in the native paragraph
 * block. Dispose these at its release boundary or child disposal, once. */
void BtechHostDiscardDecodedShapeBank(BtechResourceBank *bank)
{
    if (bank->decodedResources)
        for (BtechShapeResource *resource=bank->decodedResources;resource<bank->decodedResources+bank->decodedResourceCount;++resource)
        {
            if (resource->entries)
                for (int16_t entryIndex=0;entryIndex<resource->entryCount;++entryIndex)
                {
                    BtechRenderEntry *entry=&resource->entries[entryIndex];
                    if (entry->variants)
                        for (uint32_t variant=0;variant<(entry->variantCount?entry->variantCount:1);++variant)
                        {
                            BtechRenderVariant *lod=&entry->variants[variant];
                            if (!lod->primitives) continue;
                            int32_t count=lod->primitiveCount>0?lod->primitiveCount:1;
                            for (int32_t record=lod->primitiveCount==0?-1:0;record<count;++record)
                                if (lod->primitives[record].kind==RENDER_PRIMITIVE_POLYGON)
                                    free(lod->primitives[record].polygonPayload.polygons);
                            free(lod->primitives-1);
                        }
                    free(entry->variants);free(entry->vectors);
                }
            free(resource->entries);
        }
    free(bank->resources);free(bank->decodedResources);
    bank->resources=NULL;bank->decodedResources=NULL;bank->decodedResourceCount=0;
}

void BtechHostDiscardDecodedSurfaceBank(BtechSurfaceBank *bank)
{
    if (bank->surfaces)
        for (uint16_t index=0;index<bank->resourceCount;++index)
        {
            BtechSurfaceDefinition *surface=bank->surfaces[index];
            bool alias=false;
            for (uint16_t prior=0;prior<index;++prior) alias|=bank->surfaces[prior]==surface;
            if (!surface || alias) continue;
            for (uint16_t patch=0;patch<surface->patchCount;++patch) free(surface->patches[patch].boundaries);
            free(surface->patches);free(surface);
        }
    free(bank->surfaces);bank->surfaces=NULL;
}

/* DOS EXEC process disposal, not a new game cleanup routine. The host owns
 * paragraph allocations (including native orphan allocations); converted
 * references/near allocations are released here before reloading the child.
 * Never run this on campaign owners or reuse it as1337/DE52. */
void BtechHostDiscardChildData(void)
{
    bool paragraphs=false;
    for (uint16_t slot=0;slot<COMBAT_BITMAP_LISTS;++slot)
        if (bitmapRegistryLists[slot])
        {
            BtechBitmapList *list=bitmapRegistryLists[slot];
            paragraphs|=list->pixels.bytes!=NULL;
            for (uint16_t alias=slot;alias<COMBAT_BITMAP_LISTS;++alias)
                if (bitmapRegistryLists[alias]==list) bitmapRegistryLists[alias]=NULL;
            /* Near owners are retained until the whole child heap is disposed. */
        }
    promptBitmaps=NULL;
    pointPictures=NULL;smallMechPictures=NULL;strutPictures=NULL;
    cockpitPictures=NULL;hudGlyphPictures=NULL;inspectionDamagePictures=NULL;
    paragraphs|=orderPanelCaptureBuffer.bytes!=NULL;
    orderPanelCaptureBuffer=(BtechResourceAllocation){0};
    for (uint16_t slot=0;slot<RESOURCE_ARCHIVE_SLOTS;++slot)
        if (archiveDescriptors[slot])
        {
            FILE *stream=archiveDescriptors[slot]->stream;
            bool runtimeOwned=false;
            for (uint16_t file=0;file<COMBAT_STREAM_SLOTS;++file) runtimeOwned|=runtimeStreams[file].file==stream;
            if (stream && !runtimeOwned) fclose(stream);
            BtechReleaseArchiveDescriptor(slot);
        }
    for (uint16_t slot=0;slot<COMBAT_STREAM_SLOTS;++slot)
        if (runtimeStreams[slot].file)
        {fclose(runtimeStreams[slot].file);runtimeStreams[slot].file=NULL;}
    for (uint16_t slot=0;slot<COMBAT_WORLD_OBJECTS;++slot)
        if (heapWorldObjectOwned[slot])
        {worldObjects[slot]=NULL;heapWorldObjectOwned[slot]=0;}
    for (uint16_t bank=0;bank<COMBAT_AUDIO_BANK_ROOTS;++bank)
    {
        /* DOS and near allocator owners include detached/reused nodes. Process
         * disposal must not follow a game's stale or cyclic next chain. */
        paragraphs|=soundBanks[bank].root.allocation.bytes!=NULL;
        free(soundBanks[bank].events);soundBanks[bank]=(BtechSoundBank){0};
    }
    for (uint32_t owner=0;owner<(uint32_t)shapeResourceBankCount+1;++owner)
    {
        BtechResourceBank *bank;
        if (owner<shapeResourceBankCount)
        {
            if (!shapeResourceBanks || shapeResourceBanks==&initialShapeResourceBank) continue;
            bank=&shapeResourceBanks[owner];
            if (bank->resources==initialShapeResourceBank.resources) continue;
        }
        else bank=&initialShapeResourceBank;
        paragraphs|=bank->allocation.bytes!=NULL;
        BtechHostDiscardDecodedShapeBank(bank);
    }
    while (retiredShapeRegistries)
    {
        RetiredShapeRegistry *retired=retiredShapeRegistries;retiredShapeRegistries=retired->next;
        free(retired); /* Bank arrays belong to the shared child near heap. */
    }
    paragraphs|=surfaceResourceBank.allocation.bytes!=NULL;
    BtechHostDiscardDecodedSurfaceBank(&surfaceResourceBank);
    initialShapeResourceBank=(BtechResourceBank){0};surfaceResourceBank=(BtechSurfaceBank){0};
    shapeResourceBanks=NULL;shapeResourceBankCount=0;
    for (uint16_t slot=1;slot<COMBAT_FONT_SLOTS;++slot) paragraphs|=fonts[slot].bytes!=NULL;
    paragraphs|=soundAllocationOverride.bytes!=NULL;
    if (paragraphs && !btechHostReleaseChildDosAllocations) abort();
    if (btechHostReleaseChildDosAllocations) btechHostReleaseChildDosAllocations();
    for (uint16_t slot=0;slot<COMBAT_FONT_SLOTS;++slot) fonts[slot]=(BtechFont){0};
    egaFontBuffer=NULL; // Borrowed staging reference, not a second allocation owner.
    BtechHostDiscardNearHeap();
}

/* Actual installed BTECH.EXE's backward EXEPACK B0/B2 interpreter and sixteen
 * relocation groups. Corresponds to Inspect-NativeVideoImage.py; no dependency
 * on Reko exports. CRC pins the reviewed revision, not cryptographic trust. */
bool BtechDecodeInstallationImage(const uint8_t *packed,size_t packedBytes,uint8_t **image)
{
    *image=NULL;
    if (!packed || packedBytes!=BTECH_PACKED_BYTES) return false;
    uint32_t crc=UINT32_MAX;
    for (size_t byte=0;byte<packedBytes;++byte)
    {
        crc^=packed[byte];
        for (uint16_t bit=0;bit<8;++bit) crc=(crc>>1)^((crc&1)?0xEDB88320u:0);
    }
    if (~crc!=0x36A13E1Au || MwReadNativeWord(packed,0)!=0x5A4D || MwReadNativeWord(packed,6)) return false;
    size_t start=(size_t)MwReadNativeWord(packed,8)*0x10;
    size_t stub=start+(size_t)MwReadNativeWord(packed,0x16)*0x10;
    if (stub+0x12D>packedBytes || MwReadNativeWord(packed,0x14)!=0x12
        || MwReadNativeWord(packed+stub,0x10)!=0x4252 || MwReadNativeWord(packed+stub,0x0E)!=1
        || (uint32_t)MwReadNativeWord(packed+stub,0x0C)*0x10!=BTECH_IMAGE_BYTES) return false;
    size_t stubEnd=stub+MwReadNativeWord(packed+stub,6);
    if (stubEnd>packedBytes) return false;
    uint8_t *decoded=malloc(BTECH_IMAGE_BYTES);
    if (!decoded) return false;
    size_t source=stub,destination=BTECH_IMAGE_BYTES;
    while (source>start && packed[source-1]==0xFF) --source;
    for (;;)
    {
        if (source-start<3) goto failed;
        uint8_t command=packed[--source];
        source-=2;uint16_t count=MwReadNativeWord(packed+source,0);
        if (count>destination) goto failed;
        destination-=count;
        if ((command&0xFE)==0xB0)
        {
            if (source==start) goto failed;
            memset(decoded+destination,packed[--source],count);
        }
        else if ((command&0xFE)==0xB2)
        {
            if (count>source-start) goto failed;
            source-=count;memcpy(decoded+destination,packed+source,count);
        }
        else goto failed;
        if (command&1) break;
    }
    if (destination) goto failed;
    size_t cursor=stub+0x12D;
    for (uint16_t group=0;group<0x10;++group)
    {
        if (cursor+2>stubEnd) goto failed;
        uint16_t count=MwReadNativeWord(packed+cursor,0);cursor+=2;
        while (count--)
        {
            if (cursor+2>stubEnd) goto failed;
            size_t at=(size_t)group*0x10000+MwReadNativeWord(packed+cursor,0);cursor+=2;
            if (at+2>BTECH_IMAGE_BYTES) goto failed;
            uint16_t selector=(uint16_t)(MwReadNativeWord(decoded+at,0)+BTECH_IMAGE_LOAD_SELECTOR);
            decoded[at]=(uint8_t)selector;decoded[at+1]=(uint8_t)(selector>>8);
        }
    }
    *image=decoded;return true;
failed:
    free(decoded);return false;
}

/* Host fresh EXEC image + DFCE BSS projection for converted owners. This is
 * still NOT native0019/1E84 entry and does not open the live SDL BTECH gate.
 * The native stack/tail outside4EDC..D5DF is never supplied as zero residue. */
bool BtechHostLoadInitialData(const char *executablePath)
{
    FILE *file=fopen(executablePath,"rb");
    if (!file) return false;
    uint8_t *packed=malloc(BTECH_PACKED_BYTES),*image=NULL;
    bool loaded=packed && fread(packed,1,BTECH_PACKED_BYTES,file)==BTECH_PACKED_BYTES
        && fgetc(file)==EOF && !ferror(file);
    if (fclose(file)) loaded=false;
    if (loaded) loaded=BtechDecodeInstallationImage(packed,BTECH_PACKED_BYTES,&image);
    free(packed);
    if (!loaded) return false;
    uint8_t *data=malloc(0x10000);
    if (!data) {free(image);return false;}
    memcpy(data,image+BTECH_DATA_IMAGE_OFFSET,BTECH_BSS_START);
    memset(data+BTECH_BSS_START,0,BTECH_BSS_END-BTECH_BSS_START);
    BtechHostDiscardChildData();
    BtechInitializeFreshMechOwners(data);BtechInitializeFreshWorldOwners(data);BtechInitializeFreshStartupOwners(data);
    BtechInitializeFreshSceneOwners();
    /* Reload writable CS owners as well as DS; handlers can patch their bytes. */
    const uint8_t *platform=image+(0x1BF5-BTECH_IMAGE_LOAD_SELECTOR)*0x10;
    const uint8_t *audio=image+(0x1ADF-BTECH_IMAGE_LOAD_SELECTOR)*0x10;
    savedKeyboardVector=(uint32_t)MwReadNativeWord(platform,0x2BC0)|((uint32_t)MwReadNativeWord(platform,0x2BC2)<<16);
    savedBiosTickVector=(uint32_t)MwReadNativeWord(platform,0x2BC4)|((uint32_t)MwReadNativeWord(platform,0x2BC6)<<16);
    savedTimerVector=(uint32_t)MwReadNativeWord(platform,0x253C)|((uint32_t)MwReadNativeWord(platform,0x253E)<<16);
    keyboardScanMask=platform[0x2D51];keyboardExtendedScanMask=platform[0x2D5E];
    weaponListLoopJumpOpcode=image[0x6964]; /* Actual code-as-data byte, not an invented E9 default. */
    hudTargetFlagsLocalDisplacement=image[0x06A5]; /* Native5524C -> Jenner vertices ->0000:9C05 in the supported DOS profile. */
    weaponDisabledColourModRm=image[0x69A8]; /* Native0000:FF03+5 = CS0956:69A8, also present in original RAM. */
    savedCriticalErrorVector=(uint32_t)MwReadNativeWord(image,0xCE9A)|((uint32_t)MwReadNativeWord(image,0xCE9C)<<16);
    savedSampleIrqVector=(uint32_t)MwReadNativeWord(audio,0x0E7D)|((uint32_t)MwReadNativeWord(audio,0x0E7F)<<16);
    loaded=BtechImportCombatStageInitialData(data,0x10000)
        && BtechImportWorldInitialData(data,0x10000,image+BTECH_GEOMETRY_IMAGE_OFFSET,BTECH_IMAGE_BYTES-BTECH_GEOMETRY_IMAGE_OFFSET)
        && BtechImportStartupInitialData(data,0x10000)
        && BtechImportSoundInitialData(data,0x10000)
        && BtechImportToneInitialData(data,0x10000,image+BTECH_TONE_IMAGE_OFFSET,BTECH_IMAGE_BYTES-BTECH_TONE_IMAGE_OFFSET)
        && BtechImportVideoInitialData(data,0x10000)
        && BtechImportOrderInitialData(data,0x10000)
        && BtechImportCockpitInitialData(data,0x10000)
        && BtechImportPoseInitialData(data,0x10000)
        && BtechImportRenderInitialData(data,0x10000)
        && BtechImportPolygonInitialData(data,0x10000);
    free(data);free(image);return loaded;
}
