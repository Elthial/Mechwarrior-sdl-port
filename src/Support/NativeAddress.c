#include "mechwarrior/native_address.h"

#include <stdlib.h>

/* Host bindings, not additional original state. Current callers use allocated
 * picture buffers only; this is not a physical-address/PSP model. An unbound
 * selector is an unsupported integration error, never a successful empty read.
 * Do not silently allocate/clear storage to conceal missing native setup. */
static uint8_t *nativeSegmentBytes[NATIVE_SELECTOR_COUNT];

void MwBindNativeSegment(uint16_t selector, uint8_t *segmentBytes)
{
    nativeSegmentBytes[selector] = segmentBytes;
}

uint8_t *MwResolveNativeSegment(uint16_t selector)
{
    if (nativeSegmentBytes[selector] == NULL)
        abort();
    return nativeSegmentBytes[selector];
}
