#include "mechwarrior/combat_sound.h"
#include "mechwarrior/combat_runtime.h"
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
            stream->flags=0;stream->dosHandle=0xFF;
            return stream;
        }
        if (COMBAT_STREAM_TABLE+slot*COMBAT_STREAM_STRIDE==lastStreamOffset) return NULL;
    }
    abort(); /* Unsupported native table overrun, not a silently extended table. */
}

/*0810:E2C0. Binary-read resource branch. F7E2's other modes and runtime buffer
 * error flags are not yet promoted; do not disguise them as host fopen behavior. */
BtechRuntimeStream *BtechRuntimeStreamOpen(const char *filename,const char *mode)
{
    BtechRuntimeStream *stream=BtechRuntimeStreamFindFreeSlot();
    if (!stream) return NULL;
    if (mode[0]!='r' || mode[1]!='b' || mode[2]) abort();
    uint16_t handle=0;
    while (handle<runtimeHandleLimit && (runtimeHandleFlags[handle]&1)) ++handle;
    if (handle>=runtimeHandleLimit) return NULL;
    if (!btechHostOpenStream) abort();
    stream->file=btechHostOpenStream(filename,mode);
    if (!stream->file) return NULL;
    runtimeHandleFlags[handle]=1;
    stream->dosHandle=(uint8_t)handle;
    stream->flags=COMBAT_STREAM_READ;
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
        if (slot>=COMBAT_STREAM_SLOTS || runtimeTemporaryFileIds[slot]) abort();
        closeStatus=BtechRuntimeStreamFlush(stream);
        int16_t closed;
        if (stream->file) {closed=fclose(stream->file)?-1:0;stream->file=NULL;}
        else
        {
            if (!btechHostCloseHandle) abort();
            closed=btechHostCloseHandle((uint16_t)(int16_t)(int8_t)stream->dosHandle);
        }
        if (closed<0) closeStatus=-1;
        else
        {
            if (stream->dosHandle>=COMBAT_DOS_HANDLES) abort();
            runtimeHandleFlags[stream->dosHandle]=0;
        }
    }
    stream->flags=0;
    return closeStatus;
}

/*0810:F8DA. Host stdio collapses the read-buffer cursor/count reset. Binary
 * resource reads and unbuffered authored standard FILEs need no output flush.
 * Buffered writer/update and auxiliary external-buffer modes remain gated. */
int16_t BtechRuntimeStreamFlush(BtechRuntimeStream *stream)
{
    size_t slot=(size_t)(stream-runtimeStreams);
    if (slot>=COMBAT_STREAM_SLOTS) abort();
    if ((stream->flags&(COMBAT_STREAM_READ|COMBAT_STREAM_WRITE))==COMBAT_STREAM_WRITE &&
        ((stream->flags&COMBAT_STREAM_OWNS_BUFFER) ||
        (runtimeStreamAuxiliaryFlags[slot]&COMBAT_STREAM_AUX_EXTERNAL_BUFFER))) abort();
    return 0;
}

/*0810:FE80. Real authored callback at4DFA, absent from Reko's procedure list.
 * Ascending inclusive FILE scan; count successful FLUSHES. No FILEs close. */
uint16_t BtechFlushAllRuntimeStreams(void)
{
    uint16_t flushed=0;
    for (uint16_t slot=0;COMBAT_STREAM_TABLE+slot*COMBAT_STREAM_STRIDE<=lastStreamOffset;++slot)
    {
        if (slot>=COMBAT_STREAM_SLOTS) abort(); /* Unrepresented native table overrun. */
        if ((runtimeStreams[slot].flags&COMBAT_STREAM_ACTIVE_MASK) &&
            BtechRuntimeStreamFlush(&runtimeStreams[slot])!=-1) ++flushed;
    }
    return flushed;
}
