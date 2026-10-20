#ifndef MECHWARRIOR_EGA_CAMPAIGN_H
#define MECHWARRIOR_EGA_CAMPAIGN_H
#include "mechwarrior/native_address.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Picture-bank record prefix: two little-endian WORDs, followed by packed
 * pixels. Character fields permit unaligned byte storage without WORD-pointer
 * aliasing or host-endian assumptions. This view owns no duplicate data. */
typedef struct PackedPictureHeader
{
    unsigned char width[0x02];
    unsigned char height[0x02];
} PackedPictureHeader;
_Static_assert(sizeof(PackedPictureHeader) == 0x04, "Native picture header size");
_Static_assert(offsetof(PackedPictureHeader, height) == 0x02, "Native height offset");
_Static_assert(_Alignof(PackedPictureHeader) == 1, "Unaligned picture headers");

enum
{
    EGA_ROWS = 0x00C8, EGA_COLUMNS = 0x0140, EGA_ROW_BYTES = 0x0028, EGA_PIXELS_PER_BYTE = 0x08,
    EGA_DATA_BYTE_COUNT = 0x10000, EGA_VIDEO_PIXEL_COUNT = 0x80000,
    EGA_FIRST_PAGE_SELECTOR = 0xA000, EGA_SECOND_PAGE_SELECTOR = 0xA200,
    EGA_DISPLAY_PAGE = 0x00, EGA_DRAW_PAGE = 0x01,
    EGA_DISPLAY_SELECTOR = 0x0058, EGA_DRAW_SELECTOR = 0x005A,
    EGA_SCANLINE_OFFSETS = 0x00A3, EGA_PIXEL_BYTE_OFFSETS = 0x0281,
    EGA_PIXEL_MASKS = 0x0501, EGA_SPRITE_WIDTH = 0x06B1,
    EGA_SPRITE_ROWS = 0x06B3, EGA_GLYPH_X = 0x0778,
    EGA_GLYPH_Y = 0x077A, EGA_CHARACTER = 0x077C,
    EGA_FOREGROUND = 0x0782, EGA_BACKGROUND = 0x0783,
    EGA_GLYPH_BIT_OFFSET = 0x0784, EGA_FONT_HEIGHT = 0x0785,
    EGA_GLYPH_ADVANCE = 0x0787, EGA_FONT_BITMAP = 0x0789,
    EGA_TEXT_X = 0x09BB, EGA_TEXT_Y = 0x09BD,
    EGA_TEXT_COLOR = 0x09BF, EGA_TEXT_LEADING = 0x09C0,
    EGA_FONT_BUFFER = 0x0AD0, EGA_FONT_HANDLE = 0x0AD2,
    EGA_SMALL_FONT_NAME = 0x0AD4, EGA_LARGE_FONT_NAME = 0x0AEE,
    EGA_SMALL_FONT_BITMAP = 0x0B03, EGA_LARGE_FONT_BITMAP = 0x0D5D,
    EGA_FONT_INDEX = 0x10CF, EGA_FONT_HEIGHTS = 0x10D1,
    EGA_FONT_ADVANCES = 0x10DB, EGA_FONT_BITMAPS = 0x10E5,
    EGA_FONT_READ_BYTES = 0x03E8, EGA_FONT_DATA_END = 0x107E, EGA_GLYPH_WRAP_X = 0x0136,
    EGA_NIBBLE_MASK = 0x0F, EGA_NIBBLE_SHIFT = 0x04,
    EGA_BYTE_HIGH_BIT = 0x80, EGA_BYTE_BITS = 0x08, EGA_BYTE_LAST_BIT = 0x07,
    EGA_PIXEL_BYTE_SHIFT = 0x03, EGA_WORD_BYTES = 0x02, EGA_PACKED_PIXELS_PER_BYTE = 0x02,
    EGA_FIRST_CHARACTER = 0x20, EGA_FONT_HEADER_BYTES = 0x04,
    EGA_GLYPH_COVERAGE_WORD = 0xFF00, EGA_CARRIAGE_RETURN = 0x0D, EGA_SKIP_CHARACTER = 0xFF,
    EGA_SELECTOR_PARAGRAPH_BYTES = 0x10,
    EGA_COPY = 0, EGA_AND = 1, EGA_OR = 2, EGA_XOR = 3
};
enum { EGA_BIOS_MODE_320X200=0x0D, EGA_PALETTE_ENTRIES=0x11 };
/* BIOS palette-register values, not campaign pixel indices. */
enum { EGA_PALETTE_BLACK=0x00, EGA_PALETTE_LIGHT_GREY=0x07,
    EGA_PALETTE_DARK_GREY=0x10, EGA_PALETTE_WHITE=0x17 };
/* Pixel indices in MechWarrior's default campaign palette (MW_EGA:13AC),
 * NOT BIOS colour codes or the standard EGA index order. Palette effects and
 * the alternate palette can temporarily change their displayed colours. */
enum
{
    Campaign_Pal_EGA_Magenta=0x00, Campaign_Pal_EGA_Black=0x01, Campaign_Pal_EGA_White=0x02,
    Campaign_Pal_EGA_LightGrey=0x03, Campaign_Pal_EGA_DarkGrey=0x04, Campaign_Pal_EGA_LightCyan=0x05,
    Campaign_Pal_EGA_LightBlue=0x06, Campaign_Pal_EGA_Blue=0x07, Campaign_Pal_EGA_LightRed=0x08,
    Campaign_Pal_EGA_Red=0x09, Campaign_Pal_EGA_LightGreen=0x0A, Campaign_Pal_EGA_Cyan=0x0B,
    Campaign_Pal_EGA_Green=0x0C, Campaign_Pal_EGA_Yellow=0x0D, Campaign_Pal_EGA_Brown=0x0E,
    Campaign_Pal_EGA_LightMagenta=0x0F
};
extern uint8_t defaultPaletteCodes[EGA_PALETTE_ENTRIES];
extern uint8_t alternatePaletteCodes[EGA_PALETTE_ENTRIES];
extern void (*egaSetVideoMode)(uint8_t mode);
extern void (*egaSetPaletteRegister)(uint8_t index,uint8_t paletteCode);
extern void (*egaSetPaletteTable)(const uint8_t *paletteCodes);
void EgaInitializeGraphics(void);
void EgaLoadDefaultPalette(void);
void EgaLoadAlternatePalette(void);

/* Named canonical owners in Memory/MW_EGA_0800_STATE.c. No raw DS mirror. */
extern uint16_t egaDisplayPage;
extern uint16_t egaDrawPage;
extern uint16_t scanlineOffsets[0x00C8];
extern uint16_t xByteOffsets[0x0140];
extern uint8_t xPixelMasks[0x0140];
extern uint16_t spriteWidth;
extern uint16_t spriteRowsRemaining;
extern uint16_t glyphX;
extern uint16_t glyphY;
extern uint16_t characterCode;
extern uint8_t glyphForeground;
extern uint8_t glyphBackground;
extern uint8_t glyphBitOffset;
extern uint16_t selectedFontHeight;
extern uint16_t selectedGlyphAdvance;
extern uint16_t selectedFontBitmapOffset;
extern uint16_t rectangleX, rectangleY, rectangleWidth, rectangleHeight;
extern uint16_t rectangleColour, firstByteWidth, lastByteWidth;
extern uint8_t firstByteMask, lastByteMask;
extern uint8_t rectangleLeadingMasks[8], rectangleTrailingMasks[8];
extern uint16_t copyRectangleX, copyRectangleY;
extern uint16_t copyRectangleWidth, copyRectangleHeight;
extern uint16_t textStartX;
extern uint16_t textCurrentY;
extern uint8_t egaTextColour;
extern uint16_t textExtraLeading;
extern uint16_t fontReadBufferOffset;
extern uint16_t fontFileHandle;
extern char smallFontFilename[0x001A];
extern char largeFontFilename[0x0015];
extern uint8_t fontBitmapStorage[0x057B];
extern uint16_t selectedFontIndex;
extern uint16_t egaFontHeights[0x0005];
extern uint16_t fontAdvances[0x0005];
extern uint16_t fontBitmapOffsets[0x0005];

/* Only binary transfers and unchecked neighbouring-byte reads use this view.
 * It projects the named owners; it must not become ordinary scalar access. */
uint8_t EgaReadOverlayByte(uint16_t offset);
void EgaWriteOverlayByte(uint16_t offset, uint8_t value);
uint16_t EgaReadOverlayWord(uint16_t offset);
/* Logical aperture/controller projection owned by Video/MW_EGA_0800.c,
 * not additional serialized overlay fields. */
extern uint8_t egaVideoPixels[EGA_VIDEO_PIXEL_COUNT];
extern uint8_t egaLogicalFunction;
extern void (*egaPresentPage)(uint16_t displayPage);

bool EgaLoadFonts(void);
bool EgaLoadFontFile(uint16_t filenameOffset, uint16_t bufferOffset);
bool EgaDosOpenFontFile(uint16_t filenameOffset, uint16_t *handleOrError);
bool EgaDosReadFontFile(uint16_t bufferOffset, uint16_t byteCount, uint16_t fileHandle);
bool EgaDosCloseFontFile(uint16_t fileHandle);
void EgaBuildScanlineOffsets(void);
void EgaSelectFont(uint16_t fontIndex);
uint16_t EgaGetFontIndex(void);
uint16_t EgaDrawPackedSprite(uint16_t x, uint16_t y, uint8_t targetPage,
    uint16_t pictureOffset, uint16_t pictureBufferSelector);
uint16_t EgaDrawCharacter(uint16_t x, uint16_t *y, uint8_t character,
    uint8_t foreground, uint8_t background);
uint16_t EgaDrawTransparentText(uint16_t x, uint16_t y, uint8_t color,
    const MwNativeByteView *text, uint16_t textOffset);
uint16_t EgaDrawTextWithLeading(uint16_t x, uint16_t y, uint8_t color,
    const MwNativeByteView *text, uint16_t textOffset);
uint16_t EgaDrawTransparentCharacter(uint8_t character, uint16_t destinationOffset);
uint16_t EgaDrawTextWithLineSpacing(uint16_t x, uint16_t y, uint8_t color,
    const MwNativeByteView *text, uint16_t textOffset);
void EgaFlipPages(void);
/* Packed palette indices: foreground in the low byte, background in the high
 * byte. These are palette slots, not fixed RGB colours. */
uint16_t EgaDrawString(uint16_t x, uint16_t *y, uint16_t colours,
    const MwNativeByteView *text, uint16_t *textOffset);
void EgaFillDrawPageRectangle(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t colour);
void EgaFillRectangleOnPage(uint16_t x, uint16_t y, uint16_t width, uint16_t height,
    uint16_t colour, uint16_t page);
void EgaFillDisplayPageRectangle(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t colour);
void EgaCopyDrawPageToDisplay(uint16_t x, uint16_t y, uint16_t width, uint16_t height);
void EgaCopyDisplayToDrawPage(uint16_t x, uint16_t y, uint16_t width, uint16_t height);
extern uint8_t egaLineColour;
extern uint16_t egaCurrentX,currentY,egaFinalX,egaFinalY;
void EgaClearDrawPage(uint8_t colour);
void EgaDrawPixel(uint16_t x,uint16_t y,uint8_t targetPage,uint8_t colour);
void EgaDrawLine(uint8_t colour,uint16_t startY,uint16_t endX,uint16_t startX,uint16_t endY);
void EgaDrawSavedLinePixel(void);
void EgaDrawVerticalLine(void);
void EgaDrawHorizontalLine(void);
#endif
