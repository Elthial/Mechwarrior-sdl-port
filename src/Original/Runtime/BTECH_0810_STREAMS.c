#include "mechwarrior/port_diagnostics.h"
#include "mechwarrior/combat_sound.h"
#include "mechwarrior/combat_runtime.h"
#include "mechwarrior/combat_heap.h"
#include <stdlib.h>

/*0810:F948. Last slot is inclusive. Native resets buffer cursor/base/count;
 * those unobserved buffering fields are owned by host stdio in this IO slice. */
BtechRuntimeStream *BtechRuntimeStreamFindFreeSlot(void)
{
    for (uint16_t slot=0;slot<COMBAT_STREAM_SLOTS;++slot)
    {
        BtechRuntimeStream *stream=&runtimeStreams[slot];
        if (!(stream->flags&COMBAT_STREAM_ACTIVE_MASK))
        {
            stream->flags=0;stream->dosHandle=0xFF;stream->buffer=NULL;
            return stream;
        }
        if (COMBAT_STREAM_TABLE+slot*COMBAT_STREAM_STRIDE==lastStreamOffset) return NULL;
    }
    /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Unsupported native table overrun, not a silently extended table."); /* Unsupported native table overrun, not a silently extended table. */
}

/*0810:E2C0. Binary-read resource branch. F7E2's other modes and runtime buffer
 * error flags are not yet promoted; do not disguise them as host fopen behavior. */
BtechRuntimeStream *BtechRuntimeStreamOpen(const char *filename,const char *mode)
{
    BtechRuntimeStream *stream=BtechRuntimeStreamFindFreeSlot();
    if (!stream) return NULL;
    if ((mode[0]!='r' && mode[0]!='w') || mode[1]!='b' || mode[2]) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: (mode[0]!='r' && mode[0]!='w') || mode[1]!='b' || mode[2]");
    uint16_t handle=0;
    while (handle<runtimeHandleLimit && (runtimeHandleFlags[handle]&1)) ++handle;
    if (handle>=runtimeHandleLimit) return NULL;
    if (!btechHostOpenStream) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !btechHostOpenStream");
    stream->file=btechHostOpenStream(filename,mode);
    if (!stream->file) return NULL;
    runtimeHandleFlags[handle]=1;
    stream->dosHandle=(uint8_t)handle;
    stream->flags=mode[0]=='r'?COMBAT_STREAM_READ:COMBAT_STREAM_WRITE;
    ++streamSetupCounter;
    return stream;
}

/*0810:E206. Host stdio owns resource buffering; preserve active/protected
 * tests, flush-before-close status, and unconditional final flags clear.
 * Native temp-file delete and non-resource update modes remain explicit gates. */
int16_t BtechRuntimeStreamClose(BtechRuntimeStream *stream)
{
    int16_t closeStatus=-1;
    if ((stream->flags&COMBAT_STREAM_ACTIVE_MASK) && !(stream->flags&COMBAT_STREAM_PROTECTED))
    {
        size_t slot=(size_t)(stream-runtimeStreams);
        if (slot>=COMBAT_STREAM_SLOTS || runtimeTemporaryFileIds[slot]) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: slot>=COMBAT_STREAM_SLOTS || runtimeTemporaryFileIds[slot]");
        closeStatus=BtechRuntimeStreamFlush(stream);
        BtechRuntimeStreamReleaseBuffer(stream);
        int16_t closed;
        if (stream->file) {closed=fclose(stream->file)?-1:0;stream->file=NULL;}
        else
        {
            if (!btechHostCloseHandle) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !btechHostCloseHandle");
            closed=btechHostCloseHandle((int8_t)stream->dosHandle);
        }
        if (closed<0) closeStatus=-1;
        else
        {
            if (stream->dosHandle>=COMBAT_DOS_HANDLES) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: stream->dosHandle>=COMBAT_DOS_HANDLES");
            runtimeHandleFlags[stream->dosHandle]=0;
        }
    }
    stream->flags=0;
    return closeStatus;
}

/*F776. A failed512-byte allocation sets the original unbuffered flag. Host
 * stdio performs the actual IO; its opaque buffer is not a second game owner. */
void BtechRuntimeStreamAllocateBuffer(BtechRuntimeStream *stream)
{
    stream->buffer=BtechAllocateNearBytes(BTECH_NEAR_STREAM_BUFFER_BYTES,BTECH_NEAR_STREAM_BUFFER_BYTES,BTECH_NEAR_STREAM_BUFFER);
    stream->flags|=stream->buffer?COMBAT_STREAM_OWNS_BUFFER:COMBAT_STREAM_UNBUFFERED;
}
/*F748. Stream close uses plain E96E, NOT CDD9's search-reset probe. */
void BtechRuntimeStreamReleaseBuffer(BtechRuntimeStream *stream)
{
    if ((stream->flags&COMBAT_STREAM_ACTIVE_MASK) && (stream->flags&COMBAT_STREAM_OWNS_BUFFER))
    {
        BtechReleaseNearHeapBlock(stream->buffer);stream->buffer=NULL;
        stream->flags&=(uint8_t)~COMBAT_STREAM_OWNS_BUFFER;
    }
}
/*CD16 and getc's E502 refill boundary. Reserve buffering at the first read,
 * not at fopen: this allocation order is observable to the shared near heap. */
size_t BtechReadStreamElements(void *destination,size_t size,size_t count,FILE *file)
{
    if (size && count)
        for (uint16_t slot=0;slot<COMBAT_STREAM_SLOTS;++slot)
            if (runtimeStreams[slot].file==file)
            {
                if (!(runtimeStreams[slot].flags&(COMBAT_STREAM_OWNS_BUFFER|COMBAT_STREAM_UNBUFFERED)))
                    BtechRuntimeStreamAllocateBuffer(&runtimeStreams[slot]);
                break;
            }
    return fread(destination,size,count,file);
}
int BtechReadStreamByte(FILE *file)
{
    uint8_t value;return BtechReadStreamElements(&value,1,1,file)==1?value:EOF;
}
int BtechCloseStreamFile(FILE *file)
{
    for (uint16_t slot=0;slot<COMBAT_STREAM_SLOTS;++slot)
        if (runtimeStreams[slot].file==file) return BtechRuntimeStreamClose(&runtimeStreams[slot]);
    return fclose(file); /* External FILE supplied by an isolated runtime caller. */
}

/*0810:F8DA. Host stdio collapses the read-buffer cursor/count reset. Binary
 * resource reads and unbuffered authored standard FILEs need no output flush.
 * Buffered writer/update and auxiliary external-buffer modes remain gated. */
int16_t BtechRuntimeStreamFlush(BtechRuntimeStream *stream)
{
    size_t slot=(size_t)(stream-runtimeStreams);
    if (slot>=COMBAT_STREAM_SLOTS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: slot>=COMBAT_STREAM_SLOTS");
    if ((stream->flags&(COMBAT_STREAM_READ|COMBAT_STREAM_WRITE))==COMBAT_STREAM_WRITE &&
        ((stream->flags&COMBAT_STREAM_OWNS_BUFFER) ||
        (runtimeStreamAuxiliaryFlags[slot]&COMBAT_STREAM_AUX_EXTERNAL_BUFFER))) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: (stream->flags&(COMBAT_STREAM_READ|COMBAT_STREAM_WRITE))==COMBAT_STREAM_WRITE && ((stream->flags&COMBAT_STREAM_OWNS_BUFFER) || (runtimeStreamAuxiliaryFlags[slot]&COMBAT_STREAM_AUX_EXTERNAL_BUFFER))");
    return 0;
}

/*0810:FE80. Real authored callback at4DFA, absent from Reko's procedure list.
 * Ascending inclusive FILE scan; count successful FLUSHES. No FILEs close. */
uint16_t BtechFlushAllRuntimeStreams(void)
{
    uint16_t flushed=0;
    for (uint16_t slot=0;COMBAT_STREAM_TABLE+slot*COMBAT_STREAM_STRIDE<=lastStreamOffset;++slot)
    {
        if (slot>=COMBAT_STREAM_SLOTS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: slot>=COMBAT_STREAM_SLOTS"); /* Unrepresented native table overrun. */
        if ((runtimeStreams[slot].flags&COMBAT_STREAM_ACTIVE_MASK) &&
            BtechRuntimeStreamFlush(&runtimeStreams[slot])!=-1) ++flushed;
    }
    return flushed;
}
