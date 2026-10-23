#include "mechwarrior/port_diagnostics.h"
#include "mechwarrior/combat_runtime.h"
#include "mechwarrior/combat_resources.h"
#include "mechwarrior/native_address.h"
#include <stdlib.h>
#include <string.h>

/*0810:DBF0. Physical length is only the archive's source limit; the five-byte
 * archive header supplies the allocation length. Two separate opens remain. */
uint32_t BtechGetDecodedResourceLength(const char *filename)
{
    FILE *stream=BtechOpenStreamWithRetry(filename,"rb");
    long savedPosition=ftell(stream);fseek(stream,0,SEEK_END);
    uint32_t physicalBytes=(uint32_t)ftell(stream);fseek(stream,savedPosition,SEEK_SET);
    int16_t archive=BtechOpenCompressedArchive(UINT16_MAX,stream,"r",physicalBytes);
    if (archive==-1) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: archive==-1");
    uint32_t decodedBytes=BtechGetArchiveDecodedLength((uint16_t)archive);
    BtechFlushAndCloseCompressedArchive((uint16_t)archive);BtechCloseStreamFile(stream);
    return decodedBytes;
}

/*0810:DC86. Destination selector carry becomes ordinary contiguous storage;
 * each original44DA request is still7D00, including the final EOF probe. */
void BtechDecodeResourceIntoAllocation(const char *filename,BtechResourceAllocation allocation)
{
    FILE *stream=BtechOpenStreamWithRetry(filename,"rb");
    long savedPosition=ftell(stream);fseek(stream,0,SEEK_END);
    uint32_t physicalBytes=(uint32_t)ftell(stream);fseek(stream,savedPosition,SEEK_SET);
    int16_t archive=BtechOpenCompressedArchive(UINT16_MAX,stream,"r",physicalBytes);
    if (archive==-1) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: archive==-1");
    uint32_t total=0;
    uint16_t decodedBytes;
    /* This local is IO transport only: the allocation is private until return.
     * It lets a malformed oversized stream fail instead of writing host memory
     * outside the native DOS block. No extra decode, length cap or retry. */
    uint8_t decodedChunk[RESOURCE_DECODE_CHUNK];
    while ((decodedBytes=BtechReadArchiveIntoFarBuffer((uint16_t)archive,decodedChunk,RESOURCE_DECODE_CHUNK))!=0)
    {
        if (decodedBytes>allocation.byteCount-total) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: decodedBytes>allocation.byteCount-total");
        memcpy(allocation.bytes+total,decodedChunk,decodedBytes);total+=decodedBytes;
    }
    if (total!=allocation.byteCount) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: total!=allocation.byteCount"); /* Native fatal resource-length mismatch. */
    BtechFlushAndCloseCompressedArchive((uint16_t)archive);BtechCloseStreamFile(stream);
}

/*0810:DDF2. Resource bytes are read only after successful paragraph allocation. */
BtechResourceAllocation BtechAllocateAndDecodeResource(const char *filename)
{
    uint32_t bytes=BtechGetDecodedResourceLength(filename);
    if (bytes>DOS_MAX_PARAGRAPH_ALLOCATION_BYTES) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: bytes>DOS_MAX_PARAGRAPH_ALLOCATION_BYTES"); /* Native paragraph wrap would corrupt foreign owners, not a host buffer of this length. */
    BtechDosAllocation allocation=BtechAllocateDosMemoryBlock(bytes);
    if (!allocation.bytes) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !allocation.bytes");
    BtechResourceAllocation resource={allocation.bytes,bytes};
    BtechDecodeResourceIntoAllocation(filename,resource);
    return resource;
}

/*0810:DD6E. First dot wins; extension is replaced, not concatenated twice. */
const char *BtechBuildFilenameWithExtension(const char *basename,const char *extension)
{
    if (strlen(basename)>=sizeof(resourceFilename)) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: strlen(basename)>=sizeof(resourceFilename)");
    strcpy(resourceFilename,basename);
    char *dot=strchr(resourceFilename,'.');
    if (!dot) dot=resourceFilename+strlen(resourceFilename);
    if ((size_t)(dot-resourceFilename)+strlen(extension)+2>sizeof(resourceFilename)) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: (size_t)(dot-resourceFilename)+strlen(extension)+2>sizeof(resourceFilename)");
    *dot++='.';strcpy(dot,extension);
    return resourceFilename;
}
