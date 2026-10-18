#ifndef MECHWARRIOR_COMBAT_VIDEO_H
#define MECHWARRIOR_COMBAT_VIDEO_H
#include "mechwarrior/combat_sound.h"

enum { COMBAT_VIDEO_EGA=2,COMBAT_VIDEO_AUTO=0,COMBAT_VIDEO_ALLOWED=7,
    COMBAT_SCREEN_WIDTH=320,COMBAT_SCREEN_HEIGHT=200,COMBAT_SCREEN_ROW_BYTES=40,
    COMBAT_EGA_COLOURS=16,COMBAT_PALETTE_SLOTS=10,COMBAT_PALETTE_WORDS=32,
    COMBAT_FONT_SLOTS=20,COMBAT_FONT_READ_BYTES=0x50,COMBAT_PALETTE_FILE_BYTES=0x80,
    COMBAT_BITMAP_LISTS=40,COMBAT_BITMAP_WORKSPACE_BYTES=0x3AC4,COMBAT_BITMAP_READ_BYTES=0x7FFF,
    COMBAT_EGA_BITMAP_SCRATCH_PAGE=0xA600,COMBAT_EGA_PLANES=4,
    COMBAT_EGA_FRONT_PAGE=0xA000,COMBAT_EGA_BACK_PAGE=0xA200,
    COMBAT_EGA_SCRATCH_PAGE=0xA400,COMBAT_EGA_SEQUENCER_PORT=0x3C4,
    COMBAT_EGA_GRAPHICS_PORT=0x3CE,COMBAT_EGA_GRAPHICS_DATA_PORT=0x3CF,COMBAT_EGA_MAP_ALL_PLANES=0x0F02,
    COMBAT_EGA_WRITE_MODE_TWO=0x0205,COMBAT_EGA_BIT_MASK_REGISTER=8,
    COMBAT_EGA_WRITE_MODE_ZERO=0x0005,COMBAT_EGA_WRITE_MODE_ONE=0x0105,COMBAT_EGA_ALL_PIXEL_BITS=0xFF08,
    COMBAT_EGA_MAP_PLANE_ZERO=0x0102,COMBAT_EGA_READ_MAP_REGISTER=4,
    COMBAT_EGA_DATA_ROTATE_COPY=0x0003,COMBAT_EGA_SEQUENCER_MAP_MASK=2,
    COMBAT_EGA_SEQUENCER_DATA_PORT=0x3C5,COMBAT_EGA_PLANE_MASK_ALL=0x0F,
    COMBAT_SCHEDULER_PERIOD=0x0D,COMBAT_FATAL_TEXT_X=0x32,COMBAT_FATAL_TEXT_Y=0x64 };
enum { COMBAT_BIOS_CRTC_PORT=0x63,COMBAT_EGA_CRTC_START_HIGH=0x0C,COMBAT_COLOUR_CRTC_INDEX_PORT=0x3D4,
    COMBAT_EGA_STATUS_PORT=0x3DA,COMBAT_EGA_VERTICAL_RETRACE=8 };
/* 1BF5:00C4's observable DI after copying35 native FAR dispatch entries. */
enum { COMBAT_NATIVE_GRAPHICS_DISPATCH_START=0x3F34,COMBAT_NATIVE_GRAPHICS_DISPATCH_ENTRIES=0x23,
    COMBAT_NATIVE_FAR_POINTER_BYTES=4,
    COMBAT_NATIVE_GRAPHICS_DISPATCH_END=COMBAT_NATIVE_GRAPHICS_DISPATCH_START
        +COMBAT_NATIVE_GRAPHICS_DISPATCH_ENTRIES*COMBAT_NATIVE_FAR_POINTER_BYTES };
extern void (*graphicsSwapScreenPages)(uint16_t waitForRetrace);
void BtechSwapScreenPages(uint16_t waitForRetrace);
void BtechEgaSwapScreenPages(uint16_t waitForRetrace);
extern const uint8_t glyphTailMasks[8];
extern const char errorCardNotFound[];

typedef struct BtechFont {
    uint8_t *bytes;
    size_t byteCount;
} BtechFont;
/* Four byte-aligned fields at the FNT: payload boundary. */
typedef struct BtechFontMetrics {
    uint8_t advancePixels,heightPixels,firstCharacter,characterCount;
} BtechFontMetrics;
_Static_assert(sizeof(BtechFontMetrics)==4,"Native FNT metrics are four bytes");
/* Runtime references replace the selector-first ten-byte native image entry.
 * The native count+1 pointer list's terminator becomes this typed extent. */
typedef struct BtechBitmap {
    uint8_t *pixelPlanes,*preserveMask;
    uint16_t widthPixels,heightPixels;
    size_t pixelStorageBytes,maskStorageBytes; /* Borrowed allocation extents, not native descriptor fields. */
} BtechBitmap;
typedef struct BtechBitmapList {
    BtechBitmap *images;
    uint16_t imageCount;
    BtechResourceAllocation pixels;
} BtechBitmapList;
extern uint16_t bitmapRegistryCounts[COMBAT_BITMAP_LISTS];
extern BtechBitmapList *bitmapRegistryLists[COMBAT_BITMAP_LISTS],*promptBitmaps;
extern BtechBitmapList *pointPictures,*smallMechPictures,*strutPictures;
extern BtechBitmapList *cockpitPictures,*hudGlyphPictures,*inspectionDamagePictures;
extern uint8_t graphicsColourMode;
extern uint32_t (*graphicsBitmapStorageSize)(const BtechBitmapList *,uint16_t *);
extern void (*graphicsPrepareBitmapList)(BtechBitmapList *,BtechResourceAllocation,uint32_t);
extern void (*graphicsDrawBitmap)(const BtechBitmap *,uint16_t,uint16_t);
extern uint16_t egaBitmapRoundedEndX,egaBitmapShiftPhase;
extern uint8_t egaBitmapClipSides;
extern uint16_t egaBitmapSourceRowSkip,egaBitmapDestinationRowSkip,egaBitmapPlaneStride;
void BtechDrawBitmap(const BtechBitmap *bitmap,int16_t x,int16_t y);
void BtechEgaDrawBitmap(const BtechBitmap *bitmap,uint16_t x,uint16_t y);
extern uint8_t textForegroundColour,textBackgroundColour,textTransparentBackground;
extern int16_t clipMinimumX,clipMaximumX,clipMinimumY,clipMaximumY;
extern uint8_t screenPolygonNeedsClip,drawPolygonFilled,egaBitmapEnabledPlanes;
extern uint8_t drawLineColour,drawFillColour;
extern uint16_t drawColourPattern,spanPrimaryPattern,spanSecondaryPattern,spanAlternationState;
extern uint8_t egaSpanLeadingMasks[8],egaSpanTrailingMasks[8],egaSpanPrimaryColour,egaSpanSecondaryColour;
extern uint8_t egaPixelBitMasks[8],egaLinePixelMask;
extern uint16_t egaLineRunLength,egaLineFractionStep,egaLineRemainingBeforeRun,egaLineFractionAccumulator;
extern void (*graphicsWritePixel)(uint16_t x,uint16_t y,uint16_t colour);
extern uint16_t (*graphicsReadPixel)(uint16_t x,uint16_t y);
extern void (*graphicsSetOverscanColour)(uint16_t colour);
extern void (*btechHostSetOverscanColour)(uint8_t colour);
void BtechWriteClippedPixel(int16_t x,int16_t y,uint16_t colour);
uint16_t BtechReadClippedPixel(int16_t x,int16_t y);
void BtechSetOverscanColour(uint16_t colour);
uint16_t BtechEgaPixelByteOffset(uint16_t x,uint16_t y,uint8_t *shift);
uint16_t BtechEgaReadPixel(uint16_t x,uint16_t y);
void BtechEgaWritePixel(uint16_t x,uint16_t y,uint16_t colour);
void BtechEgaSetOverscanColour(uint16_t colour);
void BtechEgaDrawLine(uint16_t firstX,uint16_t firstY,uint16_t secondX,uint16_t secondY,uint16_t page);
extern void (*graphicsCopyScreenRectangle)(uint16_t,uint16_t,uint16_t,uint16_t);
void BtechCopyScreenRectangle(uint16_t x,uint16_t y,uint16_t width,uint16_t height);
void BtechEgaCopyScreenRectangle(uint16_t x,uint16_t y,uint16_t width,uint16_t height);
typedef void (*BtechSolidSpan)(uint16_t pattern,uint16_t x,int16_t count,uint16_t rowOffset,uint16_t page);
typedef void (*BtechAlternatingSpan)(uint16_t primary,uint16_t x,int16_t count,uint16_t alternate,uint16_t rowOffset,uint16_t page);
/* One native code reference has two proved register interfaces. Exactly one
 * member is active; no unused DX argument or invented drawing adapter. */
typedef struct BtechShapeSpanCallback {BtechSolidSpan solid;BtechAlternatingSpan alternating;} BtechShapeSpanCallback;
extern BtechSolidSpan graphicsFillSolidSpan;
extern BtechAlternatingSpan graphicsFillAlternatingSpan;
extern BtechShapeSpanCallback shapeSpanCallback;
uint16_t BtechDrawStyledRectangle(uint16_t left,uint16_t top,uint16_t width,uint16_t height);
enum { COMBAT_ELLIPSE_MAX_RADIUS=0x66,COMBAT_ELLIPSE_ROWS=COMBAT_ELLIPSE_MAX_RADIUS+1 };
extern uint8_t ellipseLeftInsets[COMBAT_ELLIPSE_ROWS],ellipseBorderWidths[COMBAT_ELLIPSE_ROWS],ellipseInteriorWidths[COMBAT_ELLIPSE_ROWS];
extern uint16_t ellipsePageSelector;
void BtechDrawStyledEllipse(int16_t radius,uint16_t centerX,uint16_t centerY);
void BtechEgaFillSolidSpan(uint16_t pattern,uint16_t x,int16_t count,uint16_t rowOffset,uint16_t page);
void BtechEgaFillAlternatingSpan(uint16_t primary,uint16_t x,int16_t count,uint16_t alternate,uint16_t rowOffset,uint16_t page);
extern uint16_t videoScratchPageSelector,backPageSelector,frontPageSelector;
extern uint16_t copySourcePageSelector,drawPageSelector;
extern uint8_t videoColourCount;
extern uint8_t pcjrTandyCompatible,combatRendererKind;
extern uint8_t textForegroundPattern,textBackgroundPattern,glyphTailMask;
extern uint16_t graphicsRowOffsets[COMBAT_SCREEN_HEIGHT];
extern uint16_t herculesDisplayRowOffsets[0x15C]; /* Adjacent native table, also read after EGA row199. */
extern uint16_t palettePrimarySlots[COMBAT_PALETTE_SLOTS][COMBAT_PALETTE_WORDS];
extern uint16_t paletteSecondarySlots[COMBAT_PALETTE_SLOTS][COMBAT_PALETTE_WORDS];
extern uint16_t graphicsColourPatterns[COMBAT_PALETTE_SLOTS*COMBAT_PALETTE_WORDS];
extern uint8_t fontAdvanceWidths[COMBAT_FONT_SLOTS],combatFontHeights[COMBAT_FONT_SLOTS];
extern uint8_t fontFirstCharacters[COMBAT_FONT_SLOTS],fontCharacterCounts[COMBAT_FONT_SLOTS];
extern BtechFont fonts[COMBAT_FONT_SLOTS];
extern uint8_t *egaFontBuffer;
extern uint16_t egaFontChunkCounter;
extern uint8_t paletteSlotIds[COMBAT_PALETTE_SLOTS];
extern void (*graphicsDrawGlyph)(uint16_t character,uint16_t x,uint16_t y,uint16_t page);
extern uint16_t (*graphicsLoadFont)(BtechRuntimeStream *,uint16_t,uint8_t *);
extern uint16_t (*graphicsLoadPalette)(BtechRuntimeStream *,uint16_t);
extern void (*graphicsSelectPalette)(uint16_t);
extern uint8_t videoAdapterAllowedMask;
extern uint32_t (*graphicsCaptureStorageSize)(uint16_t width,uint16_t height);
typedef void (*BtechScreenRectangleTransfer)(BtechResourceAllocation buffer,uint16_t x,uint16_t y,uint16_t width,uint16_t height);
extern BtechScreenRectangleTransfer graphicsCaptureRectangle,graphicsRestoreRectangle;
void BtechCaptureScreenRectangle(BtechResourceAllocation buffer,uint16_t x,uint16_t y,uint16_t width,uint16_t height);
void BtechRestoreScreenRectangle(BtechResourceAllocation buffer,uint16_t x,uint16_t y,uint16_t width,uint16_t height);
void BtechEgaCaptureScreenRectangle(BtechResourceAllocation buffer,uint16_t x,uint16_t y,uint16_t width,uint16_t height);
void BtechEgaRestoreScreenRectangle(BtechResourceAllocation buffer,uint16_t x,uint16_t y,uint16_t width,uint16_t height);
/* Packed screen bytes are decoded IO, not a native segment-memory registry. */
typedef void (*BtechScreenRegionUpload)(BtechResourceAllocation source,uint16_t left,uint16_t top,uint16_t width,uint16_t height);
extern BtechScreenRegionUpload graphicsUploadPackedScreenRegion;
/* Converted EGA alternate-upload/present targets are the genuine000B RETF.
 * It consumes no inputs; wrapper arguments remain in their original domains. */
extern void (*graphicsUploadAlternateScreenRegion)(void),(*graphicsPresentRectangle)(void);
void BtechPresentScreenRectangle(int16_t x,int16_t y,uint16_t width,uint16_t height);
extern uint16_t egaScreenUploadColumns,egaScreenUploadRowSkip,egaScreenUploadRowsRemaining;
uint16_t BtechLoadScreenImage(const char *filename,BtechRuntimeStream *stream);
void BtechDispatchImageRegionPrimary(BtechResourceAllocation source,uint16_t left,uint16_t top,uint16_t width,uint16_t height);
void BtechDispatchImageRegionAlternate(BtechResourceAllocation source,uint16_t left,uint16_t top,uint16_t width,uint16_t height);
void BtechSetEgaSpriteWriteRegisters(void);
void BtechEgaUploadPackedScreenRegion(BtechResourceAllocation source,uint16_t left,uint16_t top,uint16_t width,uint16_t height);
void BtechEgaNoOperation(void);
uint32_t BtechGetCaptureStorageSize(uint16_t width,uint16_t height);
uint32_t BtechEgaGetCaptureStorageSize(uint16_t width,uint16_t height);
/* BIOS register replies and physical ROM/video accesses only; no renderer or
 * game-decision callbacks. The live SDL BTECH entry remains gated. */
extern uint16_t (*btechHostQueryEgaInfo)(void); /* INT10 AX1200/BX0010 -> BX. */
extern uint8_t (*btechHostReadRomByte)(uint16_t selector,uint16_t offset);
extern uint8_t (*btechHostReadVideoByte)(uint16_t page,uint16_t offset);
extern void (*btechHostWriteVideoByte)(uint16_t page,uint16_t offset,uint8_t value);
extern void (*btechHostWritePortWord)(uint16_t port,uint16_t value);
extern void (*btechHostSetPaletteRegister)(uint8_t index,uint8_t colour); /* INT10 AX1000. */
bool BtechImportVideoInitialData(const uint8_t *data,size_t bytes);
uint16_t BtechSelectCombatRenderer(uint8_t adapterOverride,uint8_t allowedMask);
uint16_t BtechDetectVideoAdapter(void);
uint16_t BtechGetBiosDisplayAdapterBits(void);
void BtechInitializeEgaVideo(uint16_t paletteByteOffset);
void BtechDrawNearText(const char *text,int16_t x,int16_t y);
void BtechEgaDrawGlyph(uint16_t character,uint16_t x,uint16_t y,uint16_t page);
void BtechGraphicsNoOperation(uint16_t character,uint16_t x,uint16_t y,uint16_t page);
void BtechShowFatalErrorAndExit(const char *errorText);
void BtechInitializeVideoAndInput(void);
/* One native filename-or-stream WORD becomes typed alternatives, never host
 * pointer bits in a native identity. Exactly one alternative is supplied. */
uint16_t BtechLoadFontSlot(const char *filename,BtechRuntimeStream *stream);
uint16_t BtechSelectFontSlot(int16_t slot);
uint16_t BtechIsFontSlotLoaded(int16_t slot);
uint16_t BtechLoadPaletteSlot(const char *filename,BtechRuntimeStream *stream);
uint16_t BtechSelectPaletteSlot(int16_t slot);
uint16_t BtechIsPaletteSlotLoaded(int16_t slot);
uint16_t BtechEgaLoadFont(BtechRuntimeStream *stream,uint16_t slot,uint8_t *scratch);
uint16_t BtechEgaLoadPalette(BtechRuntimeStream *stream,uint16_t slot);
void BtechEgaSelectPalette(uint16_t slot);
BtechBitmapList *BtechLoadBitmapList(const char *filename,BtechRuntimeStream *stream);
void BtechReleaseBitmapListNearBlocks(BtechBitmapList *list);
void BtechFreeRegisteredBitmapList(BtechBitmapList *list);
int16_t BtechFindFreeBitmapRegistrySlot(void);
void BtechUnregisterBitmapList(BtechBitmapList *list);
void BtechRegisterBitmapList(BtechBitmapList *list,uint16_t count,uint16_t slot);
uint16_t BtechGetRegisteredBitmapCount(const BtechBitmapList *list);
uint16_t BtechIsBitmapListRegistered(const BtechBitmapList *list);
uint32_t BtechEgaBitmapStorageSize(const BtechBitmapList *list,uint16_t *auxiliaryBytes);
void BtechEgaPrepareBitmapList(BtechBitmapList *list,BtechResourceAllocation pixels,uint32_t packedBytes);
void BtechEgaUnpackNibblesToScratchPlanes(BtechResourceAllocation source,uint16_t planeBytes);
void BtechEgaCopyScratchPlanes(uint16_t sourceOffset,uint8_t *destination,uint16_t planeBytes);
void BtechEgaBuildZeroColourMask(uint16_t sourceOffset,uint8_t *destination,uint16_t planeBytes);
void BtechLoadCockpitHudResources(void);
void BtechUnloadCockpitHudResources(void);
void BtechFreeResourceIfLoaded(BtechBitmapList *list);
enum { COMBAT_LANDSCAPES=3,COMBAT_HUD_COLOUR_SCHEMES=2 };
typedef struct BtechLandscapeCameraColourRow {
    uint8_t playerFillPaletteA,unidentifiedField2,sharedFillPaletteB,unidentifiedField3,unidentifiedField4[2];
    uint16_t mapBoundaryColour,mapRetreatBoundaryColour,unitMarkerColours[5];
    uint16_t orderTrackingLineColour;
    uint8_t hudReticleColour,unidentifiedField11;
    uint16_t headingTickPaletteIndex;
    uint8_t unidentifiedField12[4];
} BtechLandscapeCameraColourRow;
extern BtechLandscapeCameraColourRow landscapeCameraColourRows[COMBAT_HUD_COLOUR_SCHEMES][COMBAT_LANDSCAPES];
extern uint16_t landscapePaletteSlots[COMBAT_LANDSCAPES],playerCameraFillASuppressed;
extern uint16_t damagePaletteSlot;
#define desertPaletteSlot landscapePaletteSlots[0]
#define tropicalPaletteSlot landscapePaletteSlots[1]
#define arcticPaletteSlot landscapePaletteSlots[2]
extern uint8_t inspectionReloadPending;
extern uint16_t inspectionSetupComplete;
void BtechApplyLandscapeCameraColours(void);
void BtechResetMechInspectionState(void);
#endif
