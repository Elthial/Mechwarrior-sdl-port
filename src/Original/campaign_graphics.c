#include "mechwarrior/campaign_graphics.h"

#include <assert.h>
#include <string.h>

/*
 * Campaign graphics overlay reconstruction.
 *
 * Address provenance:
 *   EGA   0800:003D..13DA (MW_EGA.EXE ordinals 0..22)
 *   Tandy 0800:003B..1185 (MW_TANDY.EXE ordinals 0..21)
 *
 * This is the shared logical result of the two original software drivers. It
 * intentionally omits EGA planes/latches and the Tandy B800 page window: those
 * layouts existed only to drive DOS hardware. Packed-sprite decoding, indexed
 * colors, line-rasterizer quirks, page ownership and rectangle dimensions
 * remain game-visible behavior and are preserved here.
 */

static uint8_t ClampColor(uint8_t color)
{
    return color & (MW_CAMPAIGN_COLOR_COUNT - 1);
}

static MwCampaignIndexedPage *GetPage(
    MwCampaignGraphics *graphics,
    MwCampaignPageTarget target)
{
    const uint8_t pageIndex = target == MW_CAMPAIGN_DISPLAY_PAGE
        ? graphics->displayPageIndex
        : graphics->drawPageIndex;
    return &graphics->pages[pageIndex];
}

static const MwCampaignIndexedPage *GetConstPage(
    const MwCampaignGraphics *graphics,
    MwCampaignPageTarget target)
{
    const uint8_t pageIndex = target == MW_CAMPAIGN_DISPLAY_PAGE
        ? graphics->displayPageIndex
        : graphics->drawPageIndex;
    return &graphics->pages[pageIndex];
}

static bool RectangleFitsScreen(
    uint16_t x,
    uint16_t y,
    uint16_t width,
    uint16_t height)
{
    return width != 0 && height != 0
        && x < MW_CAMPAIGN_SCREEN_WIDTH
        && y < MW_CAMPAIGN_SCREEN_HEIGHT
        && width <= MW_CAMPAIGN_SCREEN_WIDTH - x
        && height <= MW_CAMPAIGN_SCREEN_HEIGHT - y;
}

void MwCampaignGraphics_BuildNativeScanlineOffsets(
    MwCampaignGraphics *graphics)
{
    assert(graphics != NULL);

    /* EGA 0800:0233; Tandy 0800:0231. */
    for (uint16_t y = 0; y < MW_CAMPAIGN_SCREEN_HEIGHT; ++y)
    {
        graphics->egaScanlineOffsets[y] = y * MW_EGA_BYTES_PER_SCANLINE;
        graphics->tandyScanlineOffsets[y] =
            (y / MW_TANDY_SCANLINE_BANK_COUNT)
                * MW_TANDY_BYTES_PER_SCANLINE_GROUP
            + (y % MW_TANDY_SCANLINE_BANK_COUNT)
                * MW_TANDY_SCANLINE_BANK_SIZE;
    }
}

void MwCampaignGraphics_Initialize(
    MwCampaignGraphics *graphics,
    uint8_t initialColor)
{
    assert(graphics != NULL);

    memset(graphics, 0, sizeof(*graphics));
    graphics->displayPageIndex = 0;
    graphics->drawPageIndex = 1;
    MwCampaignGraphics_BuildNativeScanlineOffsets(graphics);

    /* Both original ordinal-0 routines clear the backing page and immediately
       present it, leaving the other page ready for the first composed frame. */
    MwCampaignGraphics_ClearDrawPage(graphics, initialColor);
    MwCampaignGraphics_PresentDrawPage(graphics);
}

void MwCampaignGraphics_ClearDrawPage(
    MwCampaignGraphics *graphics,
    uint8_t color)
{
    assert(graphics != NULL);

    MwCampaignIndexedPage *drawPage = GetPage(
        graphics,
        MW_CAMPAIGN_DRAW_PAGE);
    memset(drawPage->pixels, ClampColor(color), sizeof(drawPage->pixels));
}

void MwCampaignGraphics_PresentDrawPage(MwCampaignGraphics *graphics)
{
    assert(graphics != NULL);

    const uint8_t previousDisplayPage = graphics->displayPageIndex;
    graphics->displayPageIndex = graphics->drawPageIndex;
    graphics->drawPageIndex = previousDisplayPage;
}

void MwCampaignGraphics_DrawPixel(
    MwCampaignGraphics *graphics,
    MwCampaignPageTarget target,
    uint16_t x,
    uint16_t y,
    uint8_t color)
{
    assert(graphics != NULL);
    assert(x < MW_CAMPAIGN_SCREEN_WIDTH);
    assert(y < MW_CAMPAIGN_SCREEN_HEIGHT);

    GetPage(graphics, target)->pixels[y][x] = ClampColor(color);
}

bool MwPackedSpriteView_FromRecord(
    const uint8_t *record,
    size_t recordSize,
    MwPackedSpriteView *sprite)
{
    if (record == NULL || sprite == NULL || recordSize < 4)
        return false;

    const uint16_t width = (uint16_t) record[0]
        | (uint16_t) ((uint16_t) record[1] << 8);
    const uint16_t height = (uint16_t) record[2]
        | (uint16_t) ((uint16_t) record[3] << 8);
    const size_t bytesPerRow = ((size_t) width + 1) / 2;

    if (width == 0 || height == 0
        || bytesPerRow > (SIZE_MAX - 4) / height)
        return false;

    const size_t packedPixelByteCount = bytesPerRow * height;
    if (packedPixelByteCount > recordSize - 4)
        return false;

    sprite->width = width;
    sprite->height = height;
    sprite->packedPixels = record + 4;
    sprite->packedPixelByteCount = packedPixelByteCount;
    return true;
}

void MwCampaignGraphics_DrawPackedSprite(
    MwCampaignGraphics *graphics,
    MwCampaignPageTarget target,
    uint16_t x,
    uint16_t y,
    const MwPackedSpriteView *sprite)
{
    assert(graphics != NULL);
    assert(sprite != NULL);
    assert(sprite->packedPixels != NULL);
    assert(x < MW_CAMPAIGN_SCREEN_WIDTH);
    assert(y < MW_CAMPAIGN_SCREEN_HEIGHT);
    assert(sprite->width <= MW_CAMPAIGN_SCREEN_WIDTH - x);
    assert(sprite->height <= MW_CAMPAIGN_SCREEN_HEIGHT - y);

    MwCampaignIndexedPage *page = GetPage(graphics, target);
    const size_t bytesPerRow = ((size_t) sprite->width + 1) / 2;
    assert(sprite->packedPixelByteCount >= bytesPerRow * sprite->height);

    /* EGA 0800:06B5; Tandy 0800:02DC. Source high nibble is the left pixel,
       low nibble the right pixel, and color zero leaves the destination alone. */
    for (uint16_t row = 0; row < sprite->height; ++row)
    {
        const uint8_t *sourceRow = sprite->packedPixels + row * bytesPerRow;
        for (uint16_t column = 0; column < sprite->width; ++column)
        {
            const uint8_t packedPair = sourceRow[column / 2];
            const uint8_t color = (column & 1) == 0
                ? packedPair >> 4
                : packedPair & 0x0F;
            if (color != MW_CAMPAIGN_TRANSPARENT_COLOR)
                page->pixels[y + row][x + column] = color;
        }
    }
}

void MwCampaignGraphics_FillRectangle(
    MwCampaignGraphics *graphics,
    MwCampaignPageTarget target,
    uint16_t x,
    uint16_t y,
    uint16_t width,
    uint16_t height,
    uint8_t color)
{
    assert(graphics != NULL);
    assert(RectangleFitsScreen(x, y, width, height));

    MwCampaignIndexedPage *page = GetPage(graphics, target);
    const uint8_t indexedColor = ClampColor(color);
    for (uint16_t row = 0; row < height; ++row)
        memset(&page->pixels[y + row][x], indexedColor, width);
}

void MwCampaignGraphics_CopyRectangle(
    MwCampaignGraphics *graphics,
    MwCampaignPageTarget source,
    MwCampaignPageTarget destination,
    uint16_t x,
    uint16_t y,
    uint16_t width,
    uint16_t height)
{
    assert(graphics != NULL);
    assert(source != destination);
    assert(RectangleFitsScreen(x, y, width, height));

    const MwCampaignIndexedPage *sourcePage = GetConstPage(graphics, source);
    MwCampaignIndexedPage *destinationPage = GetPage(graphics, destination);
    for (uint16_t row = 0; row < height; ++row)
    {
        memcpy(
            &destinationPage->pixels[y + row][x],
            &sourcePage->pixels[y + row][x],
            width);
    }
}

void MwCampaignGraphics_DrawLine(
    MwCampaignGraphics *graphics,
    uint16_t startX,
    uint16_t startY,
    uint16_t endX,
    uint16_t endY,
    uint8_t color)
{
    assert(graphics != NULL);
    assert(startX < MW_CAMPAIGN_SCREEN_WIDTH);
    assert(endX < MW_CAMPAIGN_SCREEN_WIDTH);
    assert(startY < MW_CAMPAIGN_SCREEN_HEIGHT);
    assert(endY < MW_CAMPAIGN_SCREEN_HEIGHT);

    /* EGA 0800:1219; Tandy 0800:0FE1. The executables first normalize the
       endpoints so X always increases. This is intentionally their midpoint
       accumulator, including the strict 'error > major/2' comparison and the
       subtraction of major+1; it is not replaced with a generic Bresenham. */
    int32_t x = startX;
    int32_t y = startY;
    int32_t finalX = endX;
    int32_t finalY = endY;
    if (x >= finalX)
    {
        const int32_t savedX = x;
        const int32_t savedY = y;
        x = finalX;
        y = finalY;
        finalX = savedX;
        finalY = savedY;
    }

    const int32_t horizontalDistance = finalX - x;
    const int32_t verticalStep = y < finalY ? 1 : -1;
    const int32_t verticalDistance = y < finalY
        ? finalY - y
        : y - finalY;
    const bool xIsMajorAxis = verticalDistance <= horizontalDistance;
    const int32_t majorDistance = xIsMajorAxis
        ? horizontalDistance
        : verticalDistance;
    const int32_t minorDistance = xIsMajorAxis
        ? verticalDistance
        : horizontalDistance;
    const int32_t errorThreshold = majorDistance / 2;
    const int32_t pointCount = majorDistance + 1;
    int32_t error = 0;

    for (int32_t point = 0; point < pointCount; ++point)
    {
        MwCampaignGraphics_DrawPixel(
            graphics,
            MW_CAMPAIGN_DISPLAY_PAGE,
            (uint16_t) x,
            (uint16_t) y,
            color);

        if (xIsMajorAxis)
            ++x;
        else
            y += verticalStep;

        error += minorDistance;
        if (error > errorThreshold)
        {
            error -= pointCount;
            if (xIsMajorAxis)
                y += verticalStep;
            else
                ++x;
        }
    }
}

const MwCampaignIndexedPage *MwCampaignGraphics_GetDisplayPage(
    const MwCampaignGraphics *graphics)
{
    assert(graphics != NULL);
    return GetConstPage(graphics, MW_CAMPAIGN_DISPLAY_PAGE);
}
