#include "mechwarrior/port_diagnostics.h"
#include "mechwarrior/ega_campaign.h"
#include <stddef.h>
#include <stdlib.h>

/* MW_EGA CS/DS 0800 owns these recovered globals and tables. Ordinary game
 * access uses their names, not a synthetic segment pointer or WORD adapter. */
uint16_t egaDisplayPage;
uint16_t egaDrawPage;
uint16_t scanlineOffsets[0x00C8];
uint16_t xByteOffsets[0x0140];
uint8_t xPixelMasks[0x0140];
uint16_t spriteWidth;
uint16_t spriteRowsRemaining;
uint16_t glyphX;
uint16_t glyphY;
uint16_t characterCode;
uint8_t glyphForeground;
uint8_t glyphBackground;
uint8_t glyphBitOffset;
uint16_t selectedFontHeight;
uint16_t selectedGlyphAdvance;
uint16_t selectedFontBitmapOffset;
uint16_t rectangleX, rectangleY, rectangleWidth, rectangleHeight;
uint16_t rectangleColour, firstByteWidth, lastByteWidth;
uint8_t firstByteMask, lastByteMask;
uint8_t rectangleLeadingMasks[8], rectangleTrailingMasks[8];
uint16_t copyRectangleX, copyRectangleY, copyRectangleWidth, copyRectangleHeight;
uint8_t egaLineColour;
uint16_t egaCurrentX,currentY,egaFinalX,egaFinalY;
uint16_t textStartX;
uint16_t textCurrentY;
uint8_t egaTextColour;
uint16_t textExtraLeading;
uint16_t fontReadBufferOffset;
uint16_t fontFileHandle;
char smallFontFilename[0x001A];
char largeFontFilename[0x0015];
uint8_t fontBitmapStorage[0x057B];
uint16_t selectedFontIndex;
uint16_t egaFontHeights[0x0005];
uint16_t fontAdvances[0x0005];
uint16_t fontBitmapOffsets[0x0005];

/* Two native font destinations overlap. Keep ONE bitmap byte owner; do not
 * create separate small/large font copies. Other unidentified/code bytes remain
 * byte ranges only for native-image import and unchecked adjacent reads.
 * These are not executable host code. */
static uint8_t egaEntryCodeBytes[0x0058];
static uint8_t egaPageFlipCodeBytes[0x0047];
static uint8_t egaPaletteCodeBytes[0x004E];
static uint8_t egaPixelReadCodeBytes[0x0070];
static uint8_t egaPackedSpriteCodeBytes[0x00C3];
static uint8_t egaCharacterCursorCodeBytes[0x0004];
static uint8_t egaCharacterCodeBytes[0x0113];
static uint8_t overlayByte;
static uint8_t egaTextCodeBytes[0x00FC];
static uint8_t egaTextColourCodeBytes[0x010E];
static uint8_t egaFontLoadCodeBytes[0x0051];
static uint8_t egaFontSelectCodeBytes[0x0025];
static uint8_t egaRectangleCopyCodeBytes[0x00EA];
static uint8_t egaLineCodeBytes[0x019D];
uint8_t defaultPaletteCodes[EGA_PALETTE_ENTRIES];
uint8_t alternatePaletteCodes[EGA_PALETTE_ENTRIES];
static uint8_t egaTailCodeBytes[0xEC32];

/* Representation bridge only: binary IO and out-of-table native offsets must
 * see the same canonical objects as direct assignments. WORDs are projected
 * little-endian explicitly, independent of host alignment or byte order.
 * This table owns no state and is not an additional game procedure. */
typedef struct
{
    uint16_t offset;
    uint32_t byteCount;
    uint8_t *bytes;
    uint16_t *words;
} EgaOverlayByteSpan;
static const EgaOverlayByteSpan overlayByteSpans[] =
{
    { 0x0000, 0x0058, egaEntryCodeBytes, NULL },
    { 0x0058, 0x0002, NULL, &egaDisplayPage },
    { 0x005A, 0x0002, NULL, &egaDrawPage },
    { 0x005C, 0x0047, egaPageFlipCodeBytes, NULL },
    { 0x00A3, 0x0190, NULL, scanlineOffsets },
    { 0x0233, 0x004E, egaPaletteCodeBytes, NULL },
    { 0x0281, 0x0280, NULL, xByteOffsets },
    { 0x0501, 0x0140, xPixelMasks, NULL },
    { 0x0641, 0x0070, egaPixelReadCodeBytes, NULL },
    { 0x06B1, 0x0002, NULL, &spriteWidth },
    { 0x06B3, 0x0002, NULL, &spriteRowsRemaining },
    { 0x06B5, 0x00C3, egaPackedSpriteCodeBytes, NULL },
    { 0x0778, 0x0002, NULL, &glyphX },
    { 0x077A, 0x0002, NULL, &glyphY },
    { 0x077C, 0x0002, NULL, &characterCode },
    { 0x077E, 0x0004, egaCharacterCursorCodeBytes, NULL },
    { 0x0782, 0x0001, &glyphForeground, NULL },
    { 0x0783, 0x0001, &glyphBackground, NULL },
    { 0x0784, 0x0001, &glyphBitOffset, NULL },
    { 0x0785, 0x0002, NULL, &selectedFontHeight },
    { 0x0787, 0x0002, NULL, &selectedGlyphAdvance },
    { 0x0789, 0x0002, NULL, &selectedFontBitmapOffset },
    { 0x078B, 0x0113, egaCharacterCodeBytes, NULL },
    { 0x089E, 2, NULL, &rectangleX }, { 0x08A0, 2, NULL, &rectangleY },
    { 0x08A2, 2, NULL, &rectangleWidth }, { 0x08A4, 2, NULL, &rectangleHeight },
    { 0x08A6, 2, NULL, &rectangleColour },
    { 0x08A8, 1, &firstByteMask, NULL }, { 0x08A9, 1, &lastByteMask, NULL },
    { 0x08AA, 2, NULL, &firstByteWidth }, { 0x08AC, 2, NULL, &lastByteWidth },
    { 0x08AE, 8, rectangleLeadingMasks, NULL }, { 0x08B6, 1, &overlayByte, NULL },
    { 0x08B7, 8, rectangleTrailingMasks, NULL }, { 0x08BF, 0x00FC, egaTextCodeBytes, NULL },
    { 0x09BB, 0x0002, NULL, &textStartX },
    { 0x09BD, 0x0002, NULL, &textCurrentY },
    { 0x09BF, 0x0001, &egaTextColour, NULL },
    { 0x09C0, 0x0002, NULL, &textExtraLeading },
    { 0x09C2, 0x010E, egaTextColourCodeBytes, NULL },
    { 0x0AD0, 0x0002, NULL, &fontReadBufferOffset },
    { 0x0AD2, 0x0002, NULL, &fontFileHandle },
    { 0x0AD4, 0x001A, (uint8_t *)smallFontFilename, NULL },
    { 0x0AEE, 0x0015, (uint8_t *)largeFontFilename, NULL },
    { 0x0B03, 0x057B, fontBitmapStorage, NULL },
    { 0x107E, 0x0051, egaFontLoadCodeBytes, NULL },
    { 0x10CF, 0x0002, NULL, &selectedFontIndex },
    { 0x10D1, 0x000A, NULL, egaFontHeights },
    { 0x10DB, 0x000A, NULL, fontAdvances },
    { 0x10E5, 0x000A, NULL, fontBitmapOffsets },
    { 0x10EF, 0x0025, egaFontSelectCodeBytes, NULL },
    { 0x1114, 2, NULL, &copyRectangleX }, { 0x1116, 2, NULL, &copyRectangleY },
    { 0x1118, 2, NULL, &copyRectangleWidth }, { 0x111A, 2, NULL, &copyRectangleHeight },
    { 0x111C, 0x00EA, egaRectangleCopyCodeBytes, NULL },
    { 0x1206, 1, &egaLineColour, NULL },
    { 0x1207, 2, NULL, &egaCurrentX },{ 0x1209, 2, NULL, &currentY },
    { 0x120B, 2, NULL, &egaFinalX },{ 0x120D, 2, NULL, &egaFinalY },
    { 0x120F, 0x019D, egaLineCodeBytes, NULL },
    { 0x13AC, EGA_PALETTE_ENTRIES, defaultPaletteCodes, NULL },
    { 0x13BD, EGA_PALETTE_ENTRIES, alternatePaletteCodes, NULL },
    { 0x13CE, 0xEC32, egaTailCodeBytes, NULL },
};

uint8_t EgaReadOverlayByte(uint16_t offset)
{
    for (size_t spanIndex = 0; spanIndex < sizeof(overlayByteSpans) / sizeof(overlayByteSpans[0]); ++spanIndex)
    {
        const EgaOverlayByteSpan *span = &overlayByteSpans[spanIndex];
        if (offset < span->offset || (uint32_t)offset - span->offset >= span->byteCount) continue;
        uint16_t relativeOffset = offset - span->offset;
        if (span->words != NULL)
            return (uint8_t)(span->words[relativeOffset / 2] >> ((relativeOffset & 1) * 8));
        return span->bytes[relativeOffset];
    }
    /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Layout bridge bug, not a new native validation."); /* Layout bridge bug, not a new native validation. */
}
void EgaWriteOverlayByte(uint16_t offset, uint8_t value)
{
    for (size_t spanIndex = 0; spanIndex < sizeof(overlayByteSpans) / sizeof(overlayByteSpans[0]); ++spanIndex)
    {
        const EgaOverlayByteSpan *span = &overlayByteSpans[spanIndex];
        if (offset < span->offset || (uint32_t)offset - span->offset >= span->byteCount) continue;
        uint16_t relativeOffset = offset - span->offset;
        if (span->words != NULL)
        {
            uint16_t *wordPointer = &span->words[relativeOffset / 2];
            uint8_t byteShift = (relativeOffset & 1) * 8;
            *wordPointer = (uint16_t)((*wordPointer & (uint16_t)~((uint16_t)0xFF << byteShift)) | ((uint16_t)value << byteShift));
        }
        else span->bytes[relativeOffset] = value;
        return;
    }
    /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("EGA overlay write has no mapped state owner.");
}
uint16_t EgaReadOverlayWord(uint16_t offset)
{
    return (uint16_t)(EgaReadOverlayByte(offset)
        | ((uint16_t)EgaReadOverlayByte((uint16_t)(offset + 1)) << 8));
}
