#include "mechwarrior/combat_video.h"
#include "mechwarrior/native_address.h"

const uint8_t glyphTailMasks[8]={0xFF,0x80,0xC0,0xE0,0xF0,0xFF,0xFC,0xFE};
const char errorCardNotFound[]="card not found";

uint8_t textForegroundColour,textBackgroundColour,textTransparentBackground;
int16_t clipMinimumX,clipMaximumX,clipMinimumY,clipMaximumY;
uint8_t screenPolygonNeedsClip,drawPolygonFilled,egaBitmapEnabledPlanes;
uint8_t drawLineColour,drawFillColour;
uint16_t drawColourPattern,spanPrimaryPattern,spanSecondaryPattern,spanAlternationState;
uint8_t egaSpanLeadingMasks[8],egaSpanTrailingMasks[8],egaSpanPrimaryColour,egaSpanSecondaryColour;
uint8_t egaPixelBitMasks[8],egaLinePixelMask;
uint16_t egaLineRunLength,egaLineFractionStep,egaLineRemainingBeforeRun,egaLineFractionAccumulator;
void (*graphicsWritePixel)(uint16_t,uint16_t,uint16_t);
uint16_t (*graphicsReadPixel)(uint16_t,uint16_t);
void (*graphicsSetOverscanColour)(uint16_t);
void (*graphicsSwapScreenPages)(uint16_t);
void (*graphicsCopyScreenRectangle)(uint16_t,uint16_t,uint16_t,uint16_t);
BtechSolidSpan graphicsFillSolidSpan;
BtechAlternatingSpan graphicsFillAlternatingSpan;
BtechShapeSpanCallback shapeSpanCallback;
uint8_t ellipseLeftInsets[COMBAT_ELLIPSE_ROWS],ellipseBorderWidths[COMBAT_ELLIPSE_ROWS],ellipseInteriorWidths[COMBAT_ELLIPSE_ROWS];
uint16_t ellipsePageSelector;
uint16_t videoScratchPageSelector,backPageSelector,frontPageSelector;
uint16_t copySourcePageSelector,drawPageSelector;
uint8_t videoColourCount;
uint8_t pcjrTandyCompatible,combatRendererKind;
uint8_t textForegroundPattern,textBackgroundPattern,glyphTailMask;
uint16_t graphicsRowOffsets[COMBAT_SCREEN_HEIGHT];
uint16_t herculesDisplayRowOffsets[0x15C];
uint16_t palettePrimarySlots[COMBAT_PALETTE_SLOTS][COMBAT_PALETTE_WORDS];
uint16_t paletteSecondarySlots[COMBAT_PALETTE_SLOTS][COMBAT_PALETTE_WORDS];
uint16_t graphicsColourPatterns[COMBAT_PALETTE_SLOTS*COMBAT_PALETTE_WORDS];
uint8_t fontAdvanceWidths[COMBAT_FONT_SLOTS],combatFontHeights[COMBAT_FONT_SLOTS];
uint8_t fontFirstCharacters[COMBAT_FONT_SLOTS],fontCharacterCounts[COMBAT_FONT_SLOTS];
BtechFont fonts[COMBAT_FONT_SLOTS];
uint8_t *egaFontBuffer;
uint16_t egaFontChunkCounter;
uint8_t paletteSlotIds[COMBAT_PALETTE_SLOTS];
void (*graphicsDrawGlyph)(uint16_t,uint16_t,uint16_t,uint16_t);
uint16_t (*graphicsLoadFont)(BtechRuntimeStream *,uint16_t,uint8_t *);
uint16_t (*graphicsLoadPalette)(BtechRuntimeStream *,uint16_t);
void (*graphicsSelectPalette)(uint16_t);
uint8_t videoAdapterAllowedMask;
uint16_t bitmapRegistryCounts[COMBAT_BITMAP_LISTS];
BtechBitmapList *bitmapRegistryLists[COMBAT_BITMAP_LISTS],*promptBitmaps;
BtechBitmapList *pointPictures,*smallMechPictures,*strutPictures;
BtechBitmapList *cockpitPictures,*hudGlyphPictures,*inspectionDamagePictures;
uint8_t graphicsColourMode;
BtechLandscapeCameraColourRow landscapeCameraColourRows[COMBAT_HUD_COLOUR_SCHEMES][COMBAT_LANDSCAPES];
uint16_t landscapePaletteSlots[COMBAT_LANDSCAPES],playerCameraFillASuppressed;
uint16_t damagePaletteSlot;
uint32_t (*graphicsCaptureStorageSize)(uint16_t,uint16_t);
BtechScreenRectangleTransfer graphicsCaptureRectangle,graphicsRestoreRectangle;
BtechScreenRegionUpload graphicsUploadPackedScreenRegion;
void (*graphicsUploadAlternateScreenRegion)(void),(*graphicsPresentRectangle)(void);
uint16_t egaScreenUploadColumns,egaScreenUploadRowSkip,egaScreenUploadRowsRemaining;
uint8_t inspectionReloadPending;
uint16_t inspectionSetupComplete;
_Static_assert(sizeof(BtechLandscapeCameraColourRow)==0x1E,"Native landscape colour row");
uint32_t (*graphicsBitmapStorageSize)(const BtechBitmapList *,uint16_t *);
void (*graphicsPrepareBitmapList)(BtechBitmapList *,BtechResourceAllocation,uint32_t);
void (*graphicsDrawBitmap)(const BtechBitmap *,uint16_t,uint16_t);
uint16_t egaBitmapRoundedEndX,egaBitmapShiftPhase;
uint8_t egaBitmapClipSides;
uint16_t egaBitmapSourceRowSkip,egaBitmapDestinationRowSkip,egaBitmapPlaneStride;

/* EXEPACK data projection, not a runtime segment mirror. The shipped font
 * directory/metrics are ZERO: an early fatal message has no default font.
 * Do not borrow the campaign font or manufacture a BIOS font fallback. */
bool BtechImportVideoInitialData(const uint8_t *data,size_t bytes)
{
    if (!data || bytes<0x4EDC) return false;
    for (uint16_t slot=0;slot<COMBAT_FONT_SLOTS;++slot)
    {
        if (MwReadNativeWord(data,0x2ED4+slot*2)) return false;
        fonts[slot]=(BtechFont){0};
        fontAdvanceWidths[slot]=data[0x2E84+slot];combatFontHeights[slot]=data[0x2E98+slot];
        fontFirstCharacters[slot]=data[0x2EAC+slot];fontCharacterCounts[slot]=data[0x2EC0+slot];
    }
    if (MwReadNativeWord(data,0x3F38)!=0x0006 || MwReadNativeWord(data,0x3F3A)!=0x1BF5) return false;
    graphicsDrawGlyph=BtechGraphicsNoOperation;
    /* Other authored RETF dispatch entries have no typed callable adapter
     * before installation. Startup must select a real backend first. */
    graphicsLoadFont=NULL;graphicsLoadPalette=NULL;graphicsSelectPalette=NULL;
    graphicsBitmapStorageSize=NULL;graphicsPrepareBitmapList=NULL;
    graphicsDrawBitmap=NULL;
    egaBitmapRoundedEndX=MwReadNativeWord(data,0x46E6);egaBitmapShiftPhase=MwReadNativeWord(data,0x46E8);
    egaBitmapClipSides=data[0x46EA];egaBitmapSourceRowSkip=MwReadNativeWord(data,0x46EB);
    egaBitmapDestinationRowSkip=MwReadNativeWord(data,0x46ED);egaBitmapPlaneStride=MwReadNativeWord(data,0x46EF);
    graphicsCaptureStorageSize=NULL;
    graphicsUploadPackedScreenRegion=NULL;graphicsUploadAlternateScreenRegion=NULL;
    graphicsPresentRectangle=NULL;
    egaScreenUploadColumns=MwReadNativeWord(data,0x4724);
    egaScreenUploadRowSkip=MwReadNativeWord(data,0x4726);egaScreenUploadRowsRemaining=MwReadNativeWord(data,0x4728);
    graphicsColourMode=data[0x2E6F];
    graphicsWritePixel=NULL;graphicsReadPixel=NULL;graphicsSetOverscanColour=NULL;
    graphicsCopyScreenRectangle=NULL;
    graphicsSwapScreenPages=NULL;
    for (uint16_t bit=0;bit<8;++bit) egaPixelBitMasks[bit]=data[0x46D4+bit];
    egaLinePixelMask=data[0x46DC];egaLineRunLength=MwReadNativeWord(data,0x46DD);
    egaLineFractionStep=MwReadNativeWord(data,0x46DF);egaLineRemainingBeforeRun=MwReadNativeWord(data,0x46E1);egaLineFractionAccumulator=MwReadNativeWord(data,0x46E3);
    /* Actual image/BSS IO boundary; runtime consumers use typed rows. */
    for (uint16_t scheme=0;scheme<COMBAT_HUD_COLOUR_SCHEMES;++scheme)
        for (uint16_t landscape=0;landscape<COMBAT_LANDSCAPES;++landscape)
        {
            BtechLandscapeCameraColourRow *row=&landscapeCameraColourRows[scheme][landscape];
            uint16_t offset=0x1E88+scheme*0x5A+landscape*0x1E;
            row->playerFillPaletteA=data[offset];row->unidentifiedField2=data[offset+1];
            row->sharedFillPaletteB=data[offset+2];
            row->unidentifiedField3=data[offset+3];row->unidentifiedField4[0]=data[offset+4];row->unidentifiedField4[1]=data[offset+5];
            row->mapBoundaryColour=MwReadNativeWord(data,offset+6);row->mapRetreatBoundaryColour=MwReadNativeWord(data,offset+8);
            for (uint16_t unit=0;unit<5;++unit) row->unitMarkerColours[unit]=MwReadNativeWord(data,offset+0x0A+unit*2);
            row->orderTrackingLineColour=MwReadNativeWord(data,offset+0x14);
            row->hudReticleColour=data[offset+0x16];row->unidentifiedField11=data[offset+0x17];
            row->headingTickPaletteIndex=MwReadNativeWord(data,offset+0x18);
            for (uint16_t byte=0;byte<sizeof(row->unidentifiedField12);++byte) row->unidentifiedField12[byte]=data[offset+0x1A+byte];
        }
    for (uint16_t landscape=0;landscape<COMBAT_LANDSCAPES;++landscape) landscapePaletteSlots[landscape]=0;
    damagePaletteSlot=0;
    playerCameraFillASuppressed=0;inspectionReloadPending=0;inspectionSetupComplete=0;
    for (uint16_t slot=0;slot<COMBAT_BITMAP_LISTS;++slot)
    {bitmapRegistryCounts[slot]=0;bitmapRegistryLists[slot]=NULL;}
    promptBitmaps=NULL; // Native fresh-child BSS, not a selective combat reset.
    pointPictures=NULL;smallMechPictures=NULL;strutPictures=NULL;
    cockpitPictures=NULL;hudGlyphPictures=NULL;inspectionDamagePictures=NULL;
    if (MwReadNativeWord(data,0x4714)) return false;
    egaFontBuffer=NULL;egaFontChunkCounter=MwReadNativeWord(data,0x4716);
    for (uint16_t slot=0;slot<COMBAT_PALETTE_SLOTS;++slot) paletteSlotIds[slot]=data[0x3796+slot];
    textForegroundColour=data[0x2E50];textBackgroundColour=data[0x2E51];
    textTransparentBackground=data[0x2E52];screenPolygonNeedsClip=data[0x2E5B];
    drawPolygonFilled=data[0x2E5C];egaBitmapEnabledPlanes=data[0x2E5F];
    drawLineColour=data[0x2E5D];drawFillColour=data[0x2E5E];
    drawColourPattern=MwReadNativeWord(data,0x2E72);spanPrimaryPattern=MwReadNativeWord(data,0x2E74);
    spanSecondaryPattern=MwReadNativeWord(data,0x2E76);spanAlternationState=MwReadNativeWord(data,0x2E7A);
    for (uint16_t i=0;i<8;++i) {egaSpanLeadingMasks[i]=data[0x4702+i];egaSpanTrailingMasks[i]=data[0x470A+i];}
    egaSpanPrimaryColour=data[0x4712];egaSpanSecondaryColour=data[0x4713];
    graphicsFillSolidSpan=NULL;graphicsFillAlternatingSpan=NULL;shapeSpanCallback=(BtechShapeSpanCallback){0};
    clipMinimumX=(int16_t)MwReadNativeWord(data,0x2E53);clipMaximumX=(int16_t)MwReadNativeWord(data,0x2E55);
    clipMinimumY=(int16_t)MwReadNativeWord(data,0x2E57);clipMaximumY=(int16_t)MwReadNativeWord(data,0x2E59);
    videoScratchPageSelector=MwReadNativeWord(data,0x2E60);backPageSelector=MwReadNativeWord(data,0x2E62);
    frontPageSelector=MwReadNativeWord(data,0x2E64);copySourcePageSelector=MwReadNativeWord(data,0x2E66);
    drawPageSelector=MwReadNativeWord(data,0x2E68);videoColourCount=data[0x2E71];
    pcjrTandyCompatible=data[0x2E6C];combatRendererKind=data[0x2E6D];
    textForegroundPattern=data[0x2E78];textBackgroundPattern=data[0x2E79];
    glyphTailMask=data[0x46D2];videoAdapterAllowedMask=data[0x3FC5];
    ellipsePageSelector=MwReadNativeWord(data,0x4151);
    for (uint16_t row=0;row<COMBAT_ELLIPSE_ROWS;++row)
    {ellipseLeftInsets[row]=data[0x401C+row];ellipseBorderWidths[row]=data[0x4083+row];ellipseInteriorWidths[row]=data[0x40EA+row];}
    for (uint16_t row=0;row<COMBAT_SCREEN_HEIGHT;++row) graphicsRowOffsets[row]=MwReadNativeWord(data,0x334E+row*2);
    for (uint16_t row=0;row<0x15C;++row) herculesDisplayRowOffsets[row]=MwReadNativeWord(data,0x34DE+row*2);
    for (uint16_t slot=0;slot<COMBAT_PALETTE_SLOTS;++slot)
        for (uint16_t colour=0;colour<COMBAT_PALETTE_WORDS;++colour)
        {
            uint16_t offset=(slot*COMBAT_PALETTE_WORDS+colour)*2;
            palettePrimarySlots[slot][colour]=MwReadNativeWord(data,0x37B4+offset);
            paletteSecondarySlots[slot][colour]=MwReadNativeWord(data,0x3A34+offset);
            graphicsColourPatterns[slot*COMBAT_PALETTE_WORDS+colour]=MwReadNativeWord(data,0x3CB4+offset);
        }
    return true;
}
