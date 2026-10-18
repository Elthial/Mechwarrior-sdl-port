#include "mechwarrior/combat_world.h"
#include <stdlib.h>

/*22B6:0256. First free of five; no blanket context reset. */
BtechCameraContext *BtechAllocateCameraContext(BtechWorldObjectPrefix *object)
{
    uint16_t slot=0;
    while (slot<COMBAT_CAMERA_CONTEXTS && (cameraContexts[slot].flags&CAMERA_OCCUPIED)) ++slot;
    if (slot==COMBAT_CAMERA_CONTEXTS) return NULL;
    BtechCameraContext *camera=&cameraContexts[slot];
    camera->flags=CAMERA_OCCUPIED;camera->projectionShift=CAMERA_PROJECTION_SHIFT;
    camera->viewObject=object;
    for (uint16_t axis=0;axis<3;++axis) camera->relativeOrientation[axis]=0;
    camera->fillPaletteA=0;camera->fillPaletteB=0;
    camera->clipMinimumA=0;camera->clipMaximumA=0x013F;
    camera->clipMinimumB=0;camera->clipMaximumB=0x00C7;
    camera->colourShadeClasses=NULL;camera->paletteRemap=NULL;
    return camera;
}


/*22B6:0953. The original CS table, not floating-point atan. */
uint16_t BtechLookupHeadingRatio(uint16_t ratioIndex)
{
    if (ratioIndex>=0x200) abort();
    return headingRatioAngles[ratioIndex];
}
