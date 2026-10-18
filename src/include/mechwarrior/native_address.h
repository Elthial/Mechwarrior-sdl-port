#ifndef MECHWARRIOR_NATIVE_ADDRESS_H
#define MECHWARRIOR_NATIVE_ADDRESS_H

#include <stdint.h>
#include <stdlib.h>

enum { NATIVE_SEGMENT_BYTE_COUNT = 0x10000, NATIVE_SELECTOR_COUNT = 0x10000,
    NATIVE_WORD_SIGN_BIT = 0x8000 };

/* Zero-copy native BYTE window. Named text storage need not be padded into
 * a second64K data segment merely to preserve a wrapping near cursor. */
typedef struct MwNativeByteView
{
    const uint8_t *bytes;
    uint16_t nativeBase;
    uint32_t byteCount;
    /* Only a proved physical-memory boundary uses this instead of allocation
     * bytes. Ordinary file/string/record views remain direct borrowed bytes. */
    uint8_t (*readPhysicalByte)(uint16_t offset);
} MwNativeByteView;

static inline uint8_t MwReadNativeViewByte(const MwNativeByteView *view,uint16_t offset)
{
    if (view->readPhysicalByte) return view->readPhysicalByte(offset);
    uint16_t relative=offset-view->nativeBase;
    if (relative>=view->byteCount) abort();
    return view->bytes[relative];
}

/* Binary stream WORD, including a real FFFF->0000 cursor wrap. Not an
 * accessor for known C record fields; use their members directly. */
static inline uint16_t MwReadNativeViewWord(const MwNativeByteView *view,uint16_t offset)
{
    uint16_t low=MwReadNativeViewByte(view,offset);
    return low|((uint16_t)MwReadNativeViewByte(view,(uint16_t)(offset+1))<<8);
}

/* Host representation only. Bind a complete native offset window; aliases
 * must resolve to the same underlying storage. This is not a DOS allocator. */
void MwBindNativeSegment(uint16_t selector, uint8_t *segmentBytes);
uint8_t *MwResolveNativeSegment(uint16_t selector);

/* Byte-order/wrap adapters, not additional game procedures. */
static inline uint16_t MwReadNativeWord(const uint8_t *bytes, uint16_t offset)
{
    return (uint16_t)(bytes[offset] | ((uint16_t)bytes[(uint16_t)(offset + 1)] << 8));
}
#endif
