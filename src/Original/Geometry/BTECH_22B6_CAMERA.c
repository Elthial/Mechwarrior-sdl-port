#include "mechwarrior/combat_render.h"
#include <stdlib.h>

/*22B6:02D2. Clear ONLY the occupied flags BYTE. Camera object/pose stay stale. */
void BtechReleaseCameraContextFar(BtechCameraContext *camera)
{
    if (!camera) abort(); /* Unrepresented native near-zero corruption. */
    camera->flags=0;
}

/*22B6:07FA/0820. Original folded Q14 table, WORD angle wrap before indexing. */
uint16_t BtechLookupSine(uint16_t angle)
{
    uint16_t index=(uint16_t)(angle+0xC000)>>4;
    if (index&0x0800) index=(uint16_t)(0x1000-index);
    return orientationCosines[index];
}

uint16_t BtechLookupCosine(uint16_t angle)
{
    uint16_t index=angle>>4;
    if (index&0x0800) index=(uint16_t)(0x1000-index);
    return orientationCosines[index];
}

/*22B6:0008. Typed camera owners and actual relative/view basis composition.
 * EGA horizon rectangles and tilted polygon regions use their native raster paths. */
void BtechPrepareCameraContext(BtechCameraContext *camera)
{
    activeCameraContext=camera;
    cameraReferencePosition.positionA=camera->viewObject->position.positionA;
    cameraReferencePosition.positionB=camera->viewObject->position.positionB;
    cameraReferencePosition.positionC=camera->viewObject->position.positionC;
    uint16_t builderCx=BtechBuildOrientationMatrix(camera->relativeOrientation,&cameraOrientationBasis,0); /* REP MOVSW exhausted CX. */
    BtechWorldObjectPrefix *viewObject=activeCameraContext->viewObject;
    if (cameraOrientationBasis.mode)
    {
        uint16_t orientation[]={viewObject->orientationA,viewObject->orientationB,viewObject->orientationC};
        builderCx=BtechBuildOrientationMatrix(orientation,&composedTransformBasis,builderCx);
        BtechComposeTransformBases(builderCx,&cameraOrientationBasis,&composedTransformBasis,&viewTransformBasis);
        BtechExtractOrientationFromBasis(&viewTransformBasis,cameraViewOrientation);
    }
    else
    {
        cameraViewOrientation[0]=viewObject->orientationA;
        cameraViewOrientation[1]=viewObject->orientationB;
        cameraViewOrientation[2]=viewObject->orientationC;
        BtechBuildOrientationMatrix(cameraViewOrientation,&viewTransformBasis,builderCx);
    }
    BtechTransposeTransformBasis(&viewTransformBasis);
    forceObjectScreenClip=0;
    projectionMode10=camera->flags&CAMERA_PROJECTION_MODE_10;
    mirrorProjectionA=camera->flags&CAMERA_MIRROR_HORIZONTAL;
    if (projectionMode10 || mirrorProjectionA || (activeCameraContext->flags&CAMERA_KEEP_SCREEN_CENTRE))
        forceObjectScreenClip=1;
    combatProjectionShift=camera->projectionShift;
    uint16_t projectionUnit=1;
    for (uint8_t shifts=(uint8_t)combatProjectionShift;shifts;--shifts) projectionUnit=(uint16_t)(projectionUnit<<1);
    combatProjectionUnit=projectionUnit;
    nearClipDepth=projectionUnit;
    clipMinimumX=camera->clipMinimumA;
    clipMaximumX=camera->clipMaximumA;
    clipMinimumY=camera->clipMinimumB;
    clipMaximumY=camera->clipMaximumB;
    BtechTransformWordVectorFar(cameraReferenceVector,&viewTransformBasis,transformedReferenceVector);
    activeColourShadeClasses=camera->colourShadeClasses;
    activePaletteRemap=camera->paletteRemap;
    if (!(camera->flags&CAMERA_KEEP_SCREEN_CENTRE))
    {
        uint16_t difference=(uint16_t)(clipMaximumX-clipMinimumX);
        screenCentreA=(int16_t)(uint16_t)(clipMinimumX+((difference>>1)|(difference&0x8000)));
        difference=(uint16_t)(clipMaximumY-clipMinimumY);
        screenCentreB=(int16_t)(uint16_t)(clipMinimumY+((difference>>1)|(difference&0x8000)));
    }
    if (!(camera->flags&CAMERA_NO_BACKGROUND))
    {
        uint8_t fillA=activeCameraContext->fillPaletteA;
        if (activePaletteRemap) fillA=activePaletteRemap[fillA];
        uint8_t fillB=activeCameraContext->fillPaletteB;
        if (activePaletteRemap) fillB=activePaletteRemap[fillB];
        if ((camera->flags&CAMERA_GRADIENT_BACKGROUND) && graphicsColourMode) abort(); /*2D03 alternate adapter. */
        BtechFillTiltedCameraHorizonRegions(fillA,fillB,cameraViewOrientation[0],cameraViewOrientation[1]);
    }
}

/*22B6:2EEE. Native culling and basis selection. Stops before the unconverted
 * resource primitives; hidden/near/outside returns really return. */
void BtechCullAndRenderWorldObject(BtechHeapWorldObject *object)
{
    if (object->prefix.renderFlags&OBJECT_HIDDEN) return;
    uint32_t farthestDepth=transformedViewPosition.positionB+resourceScaledExtent;
    uint16_t builderCx=(uint16_t)farthestDepth; /*2F00/2F08, retained on forced-clip branch. */
    if ((int32_t)farthestDepth<(int16_t)nearClipDepth || !objectProjectionScale) return;
    if (forceObjectScreenClip) objectNeedsScreenClip=1;
    else
    {
        for (uint16_t phase=0;phase<2;++phase)
        {
            uint32_t magnitude=transformedViewPosition.positionA;
            if ((int32_t)magnitude<0) magnitude=0-magnitude;
            projectionRatio.ratio.numerator=phase?magnitude+resourceScaledExtent:magnitude-resourceScaledExtent;
            projectionRatio.ratio.denominator=phase?transformedViewPosition.positionB-resourceScaledExtent
                :transformedViewPosition.positionB+resourceScaledExtent;
            uint32_t nearDepth=(uint32_t)(int32_t)(int16_t)nearClipDepth;
            if (phase && (int32_t)nearDepth>(int32_t)projectionRatio.ratio.denominator)
                projectionRatio.ratio.denominator=nearDepth;
            magnitude=transformedViewPosition.positionC;
            if ((int32_t)magnitude<0) magnitude=0-magnitude;
            projectionRatio.ratio.third=phase?magnitude+resourceScaledExtent:magnitude-resourceScaledExtent;
            builderCx=BtechProjectViewPosition(&projectionRatio.vector,&projectionVectorScratch);
            BtechOffsetProjectedScreenPair(&projectionVectorScratch);
            uint8_t outside=clipMaximumX<(int32_t)projectionVectorScratch.positionA
                || clipMinimumY>(int32_t)projectionVectorScratch.positionB;
            if (!phase && outside) return;
            if (phase) objectNeedsScreenClip=outside;
        }
    }
    useViewBasisDirectly=1;
    activeRenderBasis=&viewTransformBasis;
    if (!(object->prefix.renderFlags&OBJECT_SKIP_ORIENTATION))
    {
        uint16_t orientation[]={object->prefix.orientationA,object->prefix.orientationB,object->prefix.orientationC};
        builderCx=BtechBuildOrientationMatrix(orientation,&cameraOrientationBasis,builderCx);
        if (cameraOrientationBasis.mode)
        {
            BtechComposeTransformBases(builderCx,&cameraOrientationBasis,&viewTransformBasis,&composedTransformBasis);
            useViewBasisDirectly=0;
            activeRenderBasis=&composedTransformBasis;
        }
    }
    BtechPrepareRenderResourceEntries(object->prefix.resourceId,object->entryVariants);
}
