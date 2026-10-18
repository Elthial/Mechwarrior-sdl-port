#include "mechwarrior/combat_sound.h"
#include "mechwarrior/combat_runtime.h"
#include "mechwarrior/native_address.h"
#include <stdlib.h>
#include <string.h>

/*1BF5:26B4. Reverse lookup matters: the first free record is slot3. */
BtechTaggedStream *BtechFindTaggedStreamRecord(BtechRuntimeStream *stream)
{
    for (int16_t slot=COMBAT_TAGGED_STREAMS-1;slot>=0;--slot)
        if (taggedStreams[slot].stream==stream) return &taggedStreams[slot];
    return NULL;
}

/*1BF5:26E8. Clear the entire record except its stream and outer length. */
void BtechResetTaggedStreamAndRewind(BtechTaggedStream *taggedStream)
{
    BtechRuntimeStream *stream=taggedStream->stream;
    uint32_t fileLength=taggedStream->branchEnds[0];
    *taggedStream=(BtechTaggedStream){0};
    taggedStream->branchEnds[0]=fileLength;
    taggedStream->stream=stream;
    rewind(stream->file);
}

/*1BF5:273A. Compare simultaneous NUL before testing the OLD count. */
uint16_t BtechEqualNearStringsBounded(const char *left,const char *right,uint16_t remaining)
{
    for (;;)
    {
        uint8_t leftByte=(uint8_t)*left,rightByte=(uint8_t)*right;
        if (!leftByte && !rightByte) return 1;
        if (!remaining--) return 1;
        ++left;++right;
        if (leftByte!=rightByte) return 0;
    }
}

/*1BF5:2812. IO errors from seek/tell are deliberately not repaired. Native
 * CC6F retries via2777; its missing-disk UI remains an explicit gate here. */
BtechRuntimeStream *BtechOpenTaggedResourceStream(const char *filename)
{
    BtechTaggedStream *taggedStream=BtechFindTaggedStreamRecord(NULL);
    if (!taggedStream) return NULL;
    BtechRuntimeStream *stream=BtechRuntimeStreamOpen(filename,"rb");
    taggedStream->stream=stream;
    if (!stream) abort(); /* Native CC6F would enter its unconverted disk prompt. */
    fseek(stream->file,0,SEEK_END);
    taggedStream->branchEnds[0]=(uint32_t)ftell(stream->file)|0x80000000;
    BtechResetTaggedStreamAndRewind(taggedStream);
    return taggedStream->stream;
}

/*1BF5:2878. Snapshot is descriptor state, not actual current FILE position. */
uint32_t BtechRestoreTaggedStreamSnapshotAndSeek(BtechTaggedStream *taggedStream)
{
    *taggedStream=taggedStreamBackup;
    fseek(taggedStream->stream->file,(int32_t)taggedStream->payloadPosition,SEEK_SET);
    return UINT32_MAX;
}

/*1BF5:28A8. Retain the native forward hierarchy scan, occurrence rules and
 * rollback. File length bit31 marks branches; it is not a compressed size. */
uint32_t BtechFindResourceTagPosition(BtechRuntimeStream *stream,const char *tag,uint16_t occurrence)
{
    if (!stream) return UINT32_MAX;
    BtechTaggedStream *taggedStream=BtechFindTaggedStreamRecord(stream);
    if (!taggedStream) return UINT32_MAX;
    uint16_t tagLength=(uint16_t)strlen(tag),resultOrdinal;
    if (!tagLength || (tagLength&3)) return UINT32_MAX;
    if (tagLength>=COMBAT_TAG_PATH_BYTES) abort(); /* Native path spill is not host memory. */
    taggedStreamBackup=*taggedStream;
    if (!BtechEqualNearStringsBounded(tag,taggedStream->tagPath,0x19))
    {
        if ((int16_t)occurrence>0)
        {BtechResetTaggedStreamAndRewind(taggedStream);resultOrdinal=occurrence;}
        else {occurrence=1;resultOrdinal=0;}
        goto scanFromCurrent;
    }
    if (!occurrence && (uint32_t)ftell(stream->file)==taggedStream->payloadPosition)
        return taggedStream->payloadPosition;
    if (occurrence==UINT16_MAX) goto seekExisting;
    if (taggedStream->occurrence)
    {
        if (!occurrence)
        {resultOrdinal=taggedStream->occurrence+1;occurrence=1;goto scanFromCurrent;}
        resultOrdinal=occurrence;
        if ((int16_t)taggedStream->occurrence<(int16_t)occurrence)
        {occurrence-=taggedStream->occurrence;goto scanFromCurrent;}
        if ((int16_t)taggedStream->occurrence<=(int16_t)occurrence) goto seekExisting;
    }
    else
    {
        resultOrdinal=occurrence;
        if (!occurrence) {occurrence=1;goto scanFromCurrent;}
    }
    BtechResetTaggedStreamAndRewind(taggedStream);
scanFromCurrent:
    if (!(taggedStream->branchEnds[taggedStream->tagDepth/4]&0x80000000))
        taggedStream->payloadPosition+=taggedStream->payloadBytes;
    fseek(stream->file,(int32_t)taggedStream->payloadPosition,SEEK_SET);
    while (occurrence--)
    {
        for (;;)
        {
            uint32_t parentEnd=taggedStream->branchEnds[taggedStream->tagDepth/4];
            if ((parentEnd&0x7FFFFFFF)==taggedStream->payloadPosition)
            {
                if (!taggedStream->tagDepth) goto restoreBackup;
                taggedStream->tagDepth-=4;
                continue;
            }
            if (!(parentEnd&0x80000000))
            {
                taggedStream->payloadPosition+=taggedStream->payloadBytes;
                fseek(stream->file,(int32_t)taggedStream->payloadPosition,SEEK_SET);
                continue;
            }
            if (fread(taggedStream->tagPath+taggedStream->tagDepth,1,4,stream->file)!=4) goto restoreBackup;
            taggedStream->tagDepth+=4;
            if ((int16_t)taggedStream->tagDepth>=0x18) goto restoreBackup;
            taggedStream->tagPath[taggedStream->tagDepth]=0;
            taggedStream->payloadPosition+=8;
            uint8_t lengthBytes[4];
            if (fread(lengthBytes,4,1,stream->file)!=1) goto restoreBackup;
            taggedStream->payloadBytes=(uint32_t)MwReadNativeWord(lengthBytes,0)|((uint32_t)MwReadNativeWord(lengthBytes,2)<<16);
            taggedStream->branchEnds[taggedStream->tagDepth/4]=taggedStream->payloadPosition+taggedStream->payloadBytes;
            taggedStream->payloadBytes&=0x7FFFFFFF;
            if (!taggedStream->payloadBytes ||
                (taggedStream->branchEnds[0]&0x7FFFFFFF)<=taggedStream->payloadBytes) goto restoreBackup;
            if (taggedStream->tagDepth==tagLength &&
                BtechEqualNearStringsBounded(taggedStream->tagPath,tag,tagLength)) break;
        }
    }
    taggedStream->occurrence=resultOrdinal;
    return taggedStream->payloadPosition;
seekExisting:
    fseek(stream->file,(int32_t)taggedStream->payloadPosition,SEEK_SET);
    return taggedStream->payloadPosition;
restoreBackup:
    return BtechRestoreTaggedStreamSnapshotAndSeek(taggedStream);
}

/*1BF5:2B3C. DX:AX is a byte count, not an address pair. */
uint32_t BtechGetTaggedPayloadLength(BtechRuntimeStream *stream)
{
    if (!stream) return UINT32_MAX;
    BtechTaggedStream *taggedStream=BtechFindTaggedStreamRecord(stream);
    return taggedStream?taggedStream->payloadBytes:UINT32_MAX;
}

/*1BF5:2B6E. Release registration BEFORE close; all other search state stays. */
uint16_t BtechCloseTaggedResourceStream(BtechRuntimeStream *stream)
{
    if (!stream) return 0;
    BtechTaggedStream *taggedStream=BtechFindTaggedStreamRecord(stream);
    if (!taggedStream) return 0;
    taggedStream->stream=NULL;
    BtechRuntimeStreamClose(stream); /* Return status is deliberately ignored. */
    return 1;
}

/*1BF5:2BAA. */
uint16_t BtechIsRegisteredTaggedStream(BtechRuntimeStream *stream)
{
    return BtechFindTaggedStreamRecord(stream)!=NULL;
}
