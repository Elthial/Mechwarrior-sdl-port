#include "mechwarrior/combat_video.h"
#include "mechwarrior/combat_startup.h"
#include <stdlib.h>

/*2A36:1D00. Exchange display/draw buffers, output CRTC BEFORE optional
 * retrace wait. The separate copy-source and drawing selectors stay stale. */
void BtechEgaSwapScreenPages(uint16_t waitForRetrace)
{
    if (!btechHostReadBiosWord || !btechHostWritePortWord || (waitForRetrace && !btechHostReadPort)) abort();
    uint16_t previousFrontPage=frontPageSelector;
    frontPageSelector=backPageSelector;
    backPageSelector=previousFrontPage;
    uint8_t displayStartHigh=(uint8_t)(frontPageSelector>>4);
    uint16_t crtcPort=btechHostReadBiosWord(COMBAT_BIOS_CRTC_PORT);
    btechHostWritePortWord(crtcPort,(uint16_t)((uint16_t)displayStartHigh<<8)|COMBAT_EGA_CRTC_START_HIGH);
    if (waitForRetrace)
    {
        while (btechHostReadPort(COMBAT_EGA_STATUS_PORT)&COMBAT_EGA_VERTICAL_RETRACE) {}
        while (!(btechHostReadPort(COMBAT_EGA_STATUS_PORT)&COMBAT_EGA_VERTICAL_RETRACE)) {}
    }
}
