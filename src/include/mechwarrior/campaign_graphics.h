#ifndef MECHWARRIOR_CAMPAIGN_GRAPHICS_H
#define MECHWARRIOR_CAMPAIGN_GRAPHICS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum
{
    MW_CAMPAIGN_SCREEN_WIDTH = 320,
    MW_CAMPAIGN_SCREEN_HEIGHT = 200,
    MW_CAMPAIGN_COLOR_COUNT = 16,
    MW_CAMPAIGN_TRANSPARENT_COLOR = 0,
    MW_EGA_PIXELS_PER_BYTE = 8,
    MW_EGA_BYTES_PER_SCANLINE = 40,
    MW_EGA_PAGE_BYTE_COUNT = 8000,
    MW_TANDY_PIXELS_PER_BYTE = 2,
    MW_TANDY_BYTES_PER_SCANLINE_GROUP = 160,
    MW_TANDY_SCANLINE_BANK_COUNT = 4,
    MW_TANDY_SCANLINE_BANK_SIZE = 0x2000,
    MW_CAMPAIGN_FONT_FILE_SIZE = 1000
};

typedef enum MwCampaignGraphicsOrdinal
{
    MW_GRAPHICS_INITIALIZE = 0,
    MW_GRAPHICS_BUILD_SCANLINE_OFFSETS = 1,
    MW_GRAPHICS_CLEAR_DRAW_PAGE = 2,
    MW_GRAPHICS_PRESENT_DRAW_PAGE = 3,
    MW_GRAPHICS_DRAW_PIXEL = 4,
    MW_GRAPHICS_DRAW_PACKED_SPRITE = 5,
    MW_GRAPHICS_DRAW_GLYPH = 6,
    MW_GRAPHICS_FILL_DRAW_RECTANGLE = 7,
    MW_GRAPHICS_DRAW_TEXT = 8,
    MW_GRAPHICS_LOAD_FONTS = 9,
    MW_GRAPHICS_SELECT_FONT = 10,
    MW_GRAPHICS_MOVE_CURSOR_LEFT_ONE_GLYPH = 11,
    MW_GRAPHICS_COPY_DRAW_TO_DISPLAY = 12,
    MW_GRAPHICS_FILL_DISPLAY_RECTANGLE = 13,
    MW_GRAPHICS_COPY_DISPLAY_TO_DRAW = 14,
    MW_GRAPHICS_DRAW_STRING = 15,
    MW_GRAPHICS_DRAW_LINE = 16,
    MW_GRAPHICS_DRAW_VERTICAL_LINE = 17,
    MW_GRAPHICS_DRAW_HORIZONTAL_LINE = 18,
    MW_GRAPHICS_LOAD_DEFAULT_PALETTE = 19,
    MW_GRAPHICS_GET_SELECTED_FONT_INDEX = 20,
    MW_GRAPHICS_DRAW_TEXT_SPACED = 21,
    MW_EGA_GRAPHICS_LOAD_ALTERNATE_PALETTE = 22
} MwCampaignGraphicsOrdinal;

typedef enum MwCampaignPageTarget
{
    MW_CAMPAIGN_DISPLAY_PAGE,
    MW_CAMPAIGN_DRAW_PAGE
} MwCampaignPageTarget;

typedef struct MwCampaignIndexedPage
{
    uint8_t pixels[MW_CAMPAIGN_SCREEN_HEIGHT][MW_CAMPAIGN_SCREEN_WIDTH];
} MwCampaignIndexedPage;

typedef struct MwCampaignGraphics
{
    MwCampaignIndexedPage pages[2];
    uint8_t displayPageIndex;
    uint8_t drawPageIndex;

    /* Native offsets are retained as format/ABI evidence. Logical drawing
       indexes pixels directly and does not recreate adapter memory layouts. */
    uint16_t egaScanlineOffsets[MW_CAMPAIGN_SCREEN_HEIGHT];
    uint16_t tandyScanlineOffsets[MW_CAMPAIGN_SCREEN_HEIGHT];
} MwCampaignGraphics;

typedef struct MwPackedSpriteView
{
    uint16_t width;
    uint16_t height;
    const uint8_t *packedPixels;
    size_t packedPixelByteCount;
} MwPackedSpriteView;

void MwCampaignGraphics_Initialize(
    MwCampaignGraphics *graphics,
    uint8_t initialColor);

void MwCampaignGraphics_BuildNativeScanlineOffsets(
    MwCampaignGraphics *graphics);

void MwCampaignGraphics_ClearDrawPage(
    MwCampaignGraphics *graphics,
    uint8_t color);

void MwCampaignGraphics_PresentDrawPage(MwCampaignGraphics *graphics);

void MwCampaignGraphics_DrawPixel(
    MwCampaignGraphics *graphics,
    MwCampaignPageTarget target,
    uint16_t x,
    uint16_t y,
    uint8_t color);

bool MwPackedSpriteView_FromRecord(
    const uint8_t *record,
    size_t recordSize,
    MwPackedSpriteView *sprite);

void MwCampaignGraphics_DrawPackedSprite(
    MwCampaignGraphics *graphics,
    MwCampaignPageTarget target,
    uint16_t x,
    uint16_t y,
    const MwPackedSpriteView *sprite);

void MwCampaignGraphics_FillRectangle(
    MwCampaignGraphics *graphics,
    MwCampaignPageTarget target,
    uint16_t x,
    uint16_t y,
    uint16_t width,
    uint16_t height,
    uint8_t color);

void MwCampaignGraphics_CopyRectangle(
    MwCampaignGraphics *graphics,
    MwCampaignPageTarget source,
    MwCampaignPageTarget destination,
    uint16_t x,
    uint16_t y,
    uint16_t width,
    uint16_t height);

void MwCampaignGraphics_DrawLine(
    MwCampaignGraphics *graphics,
    uint16_t startX,
    uint16_t startY,
    uint16_t endX,
    uint16_t endY,
    uint8_t color);

const MwCampaignIndexedPage *MwCampaignGraphics_GetDisplayPage(
    const MwCampaignGraphics *graphics);

#endif
