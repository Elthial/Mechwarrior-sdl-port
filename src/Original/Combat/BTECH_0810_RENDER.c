#include "mechwarrior/combat_render.h"
#include "mechwarrior/combat_scene.h"
#include "mechwarrior/combat_startup.h"
#include "mechwarrior/combat_import.h"
#include <stdlib.h>

/*0810:0985. Genuine FAR veneer into the NEAR CF5C entry. */
void BtechRenderObjectListVeneer(BtechCameraContext *camera,BtechWorldObjectPrefix *const *objects,int16_t count)
{
    BtechRenderObjectList(camera,objects,count);
}

/*0810:CF5C. Three native arguments. Incidental preserved DI is not a return
 * value. List/callback fields remain live across every native callee. */
void BtechRenderObjectList(BtechCameraContext *camera,BtechWorldObjectPrefix *const *objects,int16_t count)
{
    BtechPrepareCameraContext(camera);
    if (beforeObjectListCallbackOffset)
    {
        if (beforeObjectListCallbackOffset!=WORLD_LIST_BEFORE_CALLBACK) abort();
        BtechPrepareSelectionPixels(objects,count);
    }
    for (int16_t ordinal=0;ordinal<count;++ordinal)
    {
        BtechHeapWorldObject *object=(BtechHeapWorldObject *)objects[ordinal];
        if (&object->prefix==activeCameraContext->viewObject) continue;
        cameraRelativePosition.positionA=object->prefix.position.positionA-cameraReferencePosition.positionA;
        cameraRelativePosition.positionB=object->prefix.position.positionB-cameraReferencePosition.positionB;
        cameraRelativePosition.positionC=object->prefix.position.positionC-cameraReferencePosition.positionC;
        BtechTransformViewVector(&cameraRelativePosition,&viewTransformBasis,&transformedViewPosition);
        cameraDepthScratch=transformedViewPosition.positionB;
        resourceScaledExtent=object->prefix.resourceScaledValue;
        projectionRatio.ratio.numerator=resourceScaledExtent;
        projectionRatio.ratio.denominator=transformedViewPosition.positionB-resourceScaledExtent;
        uint32_t nearDepth=(int16_t)nearClipDepth;
        if ((int32_t)nearDepth>(int32_t)projectionRatio.ratio.denominator)
            projectionRatio.ratio.denominator=nearDepth;
        BtechProjectValueDepthRatio(&projectionRatio,&projectionVectorScratch.positionA);
        if ((int32_t)projectionVectorScratch.positionA>OBJECT_MAXIMUM_PROJECTION_SCALE)
            projectionVectorScratch.positionA=OBJECT_MAXIMUM_PROJECTION_SCALE;
        objectProjectionScale=(uint16_t)projectionVectorScratch.positionA;
        if (object->prefix.renderFlags&OBJECT_CAPTURE_PROJECTION)
        {
            object->transformedPosition[0]=transformedViewPosition.positionA;
            object->transformedPosition[1]=transformedViewPosition.positionB;
            object->transformedPosition[2]=transformedViewPosition.positionC;
            object->prefix.renderFlags&=(uint8_t)~OBJECT_PROJECTED_VISIBLE;
            if ((int16_t)nearClipDepth<=(int32_t)transformedViewPosition.positionB)
            {
                BtechProjectViewPosition(&transformedViewPosition,&projectionVectorScratch);
                BtechOffsetProjectedScreenPair(&projectionVectorScratch);
                if (clipMinimumX<=(int32_t)projectionVectorScratch.positionA
                    && clipMaximumX>=(int32_t)projectionVectorScratch.positionA
                    && clipMinimumY<=(int32_t)projectionVectorScratch.positionB
                    && clipMaximumY>=(int32_t)projectionVectorScratch.positionB)
                {
                    object->projectionValue=objectProjectionScale;
                    object->projectedScreenA=(int16_t)projectionVectorScratch.positionA;
                    object->projectedScreenB=(int16_t)projectionVectorScratch.positionB;
                    object->prefix.renderFlags|=OBJECT_PROJECTED_VISIBLE;
                }
            }
        }
        if (object->prefix.updateCallbackOffset && (object->prefix.renderFlags&OBJECT_CALLBACK_BEFORE_RENDER))
        {
            if (object->prefix.updateCallbackOffset!=WORLD_OBJECT_SELECTION_PIXEL_CALLBACK) abort();
            BtechUpdateObjectSelectionPixels(object,(uint16_t)ordinal);
        }
        BtechCullAndRenderWorldObject(object);
        if (object->prefix.updateCallbackOffset && !(object->prefix.renderFlags&OBJECT_CALLBACK_BEFORE_RENDER))
        {
            if (object->prefix.updateCallbackOffset!=WORLD_OBJECT_SELECTION_PIXEL_CALLBACK) abort();
            BtechUpdateObjectSelectionPixels(object,(uint16_t)ordinal);
        }
    }
    if (afterObjectListCallbackOffset)
    {
        if (afterObjectListCallbackOffset!=WORLD_LIST_AFTER_CALLBACK) abort();
        BtechRestoreSelectionPixels(objects);
    }
}

/*0810:C1F6/C260/C058. Native marker lifecycle around the actual object draw.
 * The saved pixels alias the same CDD6 owner used by world ordering. */
void BtechPrepareSelectionPixels(BtechWorldObjectPrefix *const *objects,int16_t count)
{
    selectionCount=0;
    if (targetSelectionProbeActive)
    {
        selectionPointBackground=BtechCaptureSelectionPixels((int16_t)selectionPointScreenX,(int16_t)selectionPointScreenY);
        BtechWriteSelectionPixels((int16_t)selectionPointScreenX,(int16_t)selectionPointScreenY,&selectionMarkerPixels);
    }
    for (int16_t ordinal=0;ordinal<count;++ordinal)
        if (objects[ordinal]->renderFlags&OBJECT_CAPTURE_PROJECTION) objects[ordinal]->flags|=WORLD_OBJECT_TARGET_PROBE_VISIBLE;
}

void BtechRestoreSelectionPixels(BtechWorldObjectPrefix *const *objects)
{
    for (int16_t index=0;index<(int16_t)selectionCount;++index)
    {
        if (index>=SELECTION_LIST_PREFIX) abort();
        uint16_t ordinal=selectionObjectListIndices[index];
        if (ordinal>=COMBAT_WORLD_OBJECTS || !objects[ordinal]) abort();
        BtechHeapWorldObject *object=(BtechHeapWorldObject *)objects[ordinal];
        BtechWriteSelectionPixels(object->projectedScreenA,object->projectedScreenB,&objectSortOrSelectionScratch.distances[ordinal]);
    }
    if (targetSelectionProbeActive)
    {
        BtechWriteSelectionPixels((int16_t)selectionPointScreenX,(int16_t)selectionPointScreenY,&selectionPointBackground);
        targetSelectionProbeActive=0;
    }
}

void BtechUpdateObjectSelectionPixels(BtechHeapWorldObject *object,uint16_t ordinal)
{
    int16_t resourceDelta=object->prefix.resourceId-otherShapeResourceBase;
    uint8_t retainCoveredEntry=(resourceDelta>=2 && resourceDelta<=7)
        || (resourceDelta>=10 && resourceDelta<=13) || (resourceDelta>=15 && resourceDelta<=22);
    if (targetSelectionProbeActive)
    {
        selectionCapturedPixels=BtechCaptureSelectionPixels((int16_t)selectionPointScreenX,(int16_t)selectionPointScreenY);
        if (!BtechTestSelectionMarkerPixels(&selectionCapturedPixels))
        {
            if (!retainCoveredEntry) selectedHitObject=&object->prefix;
            selectionPointBackground=selectionCapturedPixels;
            BtechWriteSelectionPixels((int16_t)selectionPointScreenX,(int16_t)selectionPointScreenY,&selectionMarkerPixels);
        }
    }
    for (int16_t index=0;index<(int16_t)selectionCount;++index)
    {
        if (index>=SELECTION_LIST_PREFIX) abort();
        uint16_t selectedOrdinal=selectionObjectListIndices[index];
        if (selectedOrdinal>=COMBAT_WORLD_OBJECTS || !cockpitWorldObjects[selectedOrdinal]) abort();
        BtechHeapWorldObject *selected=(BtechHeapWorldObject *)cockpitWorldObjects[selectedOrdinal];
        selectionCapturedPixels=BtechCaptureSelectionPixels(selected->projectedScreenA,selected->projectedScreenB);
        if (!BtechTestSelectionMarkerPixels(&selectionCapturedPixels))
        {
            if (!retainCoveredEntry)
            {
                if (selectionCount>SELECTION_LIST_PREFIX) abort();
                selectionObjectListIndices[index]=selectionObjectListIndices[selectionCount-1];
                --index;--selectionCount;
                selected->prefix.flags&=(uint8_t)~WORLD_OBJECT_TARGET_PROBE_VISIBLE;
            }
            else
            {
                objectSortOrSelectionScratch.distances[selectedOrdinal]=selectionCapturedPixels;
                BtechWriteSelectionPixels(selected->projectedScreenA,selected->projectedScreenB,&selectionMarkerPixels);
            }
        }
    }
    /* C194/C19A repeat the same first-byte bit08 test. */
    if ((object->prefix.renderFlags&(OBJECT_CAPTURE_PROJECTION|OBJECT_PROJECTED_VISIBLE))
        ==(OBJECT_CAPTURE_PROJECTION|OBJECT_PROJECTED_VISIBLE))
    {
        uint16_t count=selectionCount++;
        if (count>=SELECTION_LIST_PREFIX || ordinal>=COMBAT_WORLD_OBJECTS) abort();
        selectionObjectListIndices[count]=ordinal;
        objectSortOrSelectionScratch.distances[ordinal]=BtechCaptureSelectionPixels(object->projectedScreenA,object->projectedScreenB);
        BtechWriteSelectionPixels(object->projectedScreenA,object->projectedScreenB,&selectionMarkerPixels);
    }
    else object->prefix.flags&=(uint8_t)~WORLD_OBJECT_TARGET_PROBE_VISIBLE;
}

/*0810:C03C. Probe position and latch; no alternative hit-test algorithm. */
void BtechBeginSelectionPixelProbe(uint16_t x,uint16_t y)
{
    selectionPointScreenX=x;selectionPointScreenY=y;
    selectedHitObject=NULL;targetSelectionProbeActive=1;
}

/*0810:C2CE. Single-pixel mode observes only AL. The other three returned
 * stack bytes are undefined native residue, not colours: discarded here,
 * not fabricated DOS RAM. Every mode-zero consumer reads only the low byte. */
uint32_t BtechCaptureSelectionPixels(int16_t x,int16_t y)
{
    if (!hudColourSchemeIndex) return (uint8_t)BtechReadClippedPixel(x,y);
    uint32_t pixels=0;
    for (uint16_t column=0;column<2;++column)
        for (uint16_t row=0;row<2;++row)
            pixels|=(uint32_t)(uint8_t)BtechReadClippedPixel((int16_t)(x+column),(int16_t)(y+row))<<((column*2+row)*8);
    return pixels;
}

/*0810:C33D. Pointer identity selects colour0D only in single-pixel mode. */
void BtechWriteSelectionPixels(int16_t x,int16_t y,const uint32_t *pixels)
{
    if (hudColourSchemeIndex)
    {
        for (uint16_t column=0;column<2;++column)
            for (uint16_t row=0;row<2;++row)
                BtechWriteClippedPixel((int16_t)(x+column),(int16_t)(y+row),(uint8_t)(*pixels>>((column*2+row)*8)));
    }
    else BtechWriteClippedPixel(x,y,pixels==&selectionMarkerPixels?SELECTION_MARKER_COLOUR:(uint8_t)*pixels);
}

/*0810:C3B9. Multi-pixel marker is the authored00010203 column-major pattern. */
uint16_t BtechTestSelectionMarkerPixels(const uint32_t *pixels)
{
    return hudColourSchemeIndex?*pixels==UINT32_C(0x03020100):(uint8_t)*pixels==SELECTION_MARKER_COLOUR;
}
