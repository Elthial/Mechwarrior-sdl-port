#include "mechwarrior/combat_cockpit.h"
#include <stdlib.h>

/*1BF5:045D. Genuine FAR tail, one native retrace WORD forwarded unchanged. */
void BtechSwapScreenPages(uint16_t waitForRetrace)
{
    if (!graphicsSwapScreenPages) abort();
    graphicsSwapScreenPages(waitForRetrace);
}

/*1BF5:044D. Original FAR tail forwards the same four rectangle WORDs. */
void BtechCopyScreenRectangle(uint16_t x,uint16_t y,uint16_t width,uint16_t height)
{
    if (!graphicsCopyScreenRectangle) abort();
    graphicsCopyScreenRectangle(x,y,width,height);
}

/*1BF5:3470. No sequencer or enable-set/reset write; setup precedes IO failure. */
void BtechSetEgaSpriteWriteRegisters(void)
{
    if (combatRendererKind==COMBAT_VIDEO_EGA)
    {
        if (!btechHostWritePortWord) abort();
        btechHostWritePortWord(COMBAT_EGA_GRAPHICS_PORT,COMBAT_EGA_WRITE_MODE_TWO);
        btechHostWritePortWord(COMBAT_EGA_GRAPHICS_PORT,COMBAT_EGA_DATA_ROTATE_COPY);
        btechHostWritePortWord(COMBAT_EGA_GRAPHICS_PORT,COMBAT_EGA_ALL_PIXEL_BITS);
    }
}

/*1BF5:3441/3445. Genuine FAR tails, typed source instead of selector:offset. */
void BtechDispatchImageRegionPrimary(BtechResourceAllocation source,uint16_t left,uint16_t top,uint16_t width,uint16_t height)
{
    if (!graphicsUploadPackedScreenRegion) abort();
    graphicsUploadPackedScreenRegion(source,left,top,width,height);
}
void BtechDispatchImageRegionAlternate(BtechResourceAllocation source,uint16_t left,uint16_t top,uint16_t width,uint16_t height)
{
    (void)source;(void)left;(void)top;(void)width;(void)height;
    if (!graphicsUploadAlternateScreenRegion) abort();
    graphicsUploadAlternateScreenRegion(); /* Actual selected000B ignores all incoming arguments. */
}

/*1BF5:0461. FAR dispatch; EGA000B ignores the caller's rectangle entirely. */
void BtechPresentScreenRectangle(int16_t x,int16_t y,uint16_t width,uint16_t height)
{
    (void)x;(void)y;(void)width;(void)height;
    if (!graphicsPresentRectangle) abort();
    graphicsPresentRectangle();
}

/*1BF5:14BE. Real SCR:BIN: archive reads; ignored read counts retain stale tail.
 * Host malloc/free replace the CRT near heap only. CDD9's trailing FFFF probe
 * fails at FAAD INC CX/JZ before heap traversal on this initialized path.
 * Near-heap addresses/residue and arbitrary physical aliases are not emulated. */
uint16_t BtechLoadScreenImage(const char *filename,BtechRuntimeStream *stream)
{
    uint16_t openedHere=0,succeeded=0,bufferBytes=COMBAT_SCREEN_BUFFER_BYTES;
    BtechResourceAllocation buffer={0};
    BtechSetEgaSpriteWriteRegisters();
    if (!stream || !BtechIsRegisteredTaggedStream(stream))
    {openedHere=1;stream=BtechOpenTaggedResourceStream(filename);}
    if (BtechFindResourceTagPosition(stream,"SCR:BIN:",0)==UINT32_MAX) goto streamCleanup;
    int16_t archive=BtechOpenCompressedArchive(0,stream->file,"r",BtechGetTaggedPayloadLength(stream));
    if (archive<0) goto streamCleanup;
    do
    {
        buffer.bytes=malloc(bufferBytes);
        if (buffer.bytes) break;
        bufferBytes>>=1;
    } while (bufferBytes);
    if (!buffer.bytes) goto archiveCleanup;
    buffer.byteCount=bufferBytes;
    uint16_t rowsPerBuffer=bufferBytes/COMBAT_PACKED_SCREEN_ROW_BYTES;
    for (uint16_t firstRow=0;(int16_t)firstRow<COMBAT_SCREEN_HEIGHT;firstRow+=rowsPerBuffer)
    {
        BtechReadArchiveIntoFarBuffer((uint16_t)archive,buffer.bytes,bufferBytes);
        BtechDispatchImageRegionPrimary(buffer,0,firstRow,COMBAT_SCREEN_WIDTH,rowsPerBuffer);
    } /* No EOF guard, zero-row escape or shortened last read/upload. */
    if (graphicsColourMode)
    {
        BtechFlushAndCloseCompressedArchive((uint16_t)archive);
        if (BtechFindResourceTagPosition(stream,"SCR:VGA:",0)!=UINT32_MAX)
        {
            archive=BtechOpenCompressedArchive(0,stream->file,"r",BtechGetTaggedPayloadLength(stream));
            if (archive>=0)
            {
                uint16_t firstRow=0;
                do
                {
                    BtechReadArchiveIntoFarBuffer((uint16_t)archive,buffer.bytes,bufferBytes);
                    BtechDispatchImageRegionAlternate(buffer,0,firstRow,COMBAT_SCREEN_WIDTH,rowsPerBuffer);
                    firstRow+=rowsPerBuffer;
                } while ((int16_t)firstRow<COMBAT_SCREEN_HEIGHT);
            }
        }
    }
    free(buffer.bytes);
    succeeded=1;
archiveCleanup:
    /* Stale first handle or negative second-open result is intentional. */
    BtechFlushAndCloseCompressedArchive((uint16_t)archive);
streamCleanup:
    if (openedHere) BtechCloseTaggedResourceStream(stream);
    return succeeded;
}
