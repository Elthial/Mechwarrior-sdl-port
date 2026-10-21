#include "mechwarrior/port_diagnostics.h"
#include "mechwarrior/combat_render.h"
#include "mechwarrior/combat_world.h"
#include <stdlib.h>

/*22B6:1503. Signed IMUL products accumulated as a wrapped DWORD, DF-clear. */
uint32_t BtechDotProductSignedWordVectors(const uint16_t first[3],const uint16_t second[3])
{
    uint32_t product=0;
    for (uint16_t component=0;component<3;++component)
    {
        int16_t firstComponent=first[component];
        int16_t secondComponent=second[component];
        product+=(uint32_t)((int32_t)firstComponent*secondComponent);
    }
    return product;
}

/*22B6:272A. Code-owned BYTE flags, normal forward REP STOSW and odd tail. */
void BtechClearGeometryVectorCacheFlags(void)
{
    if (renderGeometryVectorCount>sizeof(vertexNearPlaneState)) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: renderGeometryVectorCount>sizeof(vertexNearPlaneState)"); /* Unrelated CS-owner overwrite, not extra cache slots. */
    uint16_t destination=0;
    for (uint16_t words=renderGeometryVectorCount>>1;words;--words)
    {vertexNearPlaneState[destination++]=0;vertexNearPlaneState[destination++]=0;}
    if (renderGeometryVectorCount&1) vertexNearPlaneState[destination]=0;
}

/*22B6:19CE. Native entry ordering, per-entry variant bytes and LOD walk.
 * Binary bank offsets have already become typed references at05DE. Keep the
 * three original parallel tables, signed counts and live shared working owners. */
void BtechPrepareRenderResourceEntries(uint16_t resourceId,const uint8_t *entryVariants)
{
    activeRenderHeader=BtechLookupBankResource(resourceId);
    if (!activeRenderHeader) return;
    BtechPrepareResourceCoordinatePaths();
    activeRenderEntry=activeRenderHeader->entries;
    int16_t entryCount=activeRenderHeader->entryCount;
    for (int16_t entryIndex=0;entryIndex<entryCount;++entryIndex)
    {
        if (entryIndex>=RENDER_ENTRY_PREFIX_CAPACITY) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: entryIndex>=RENDER_ENTRY_PREFIX_CAPACITY"); /* Native keys overlap7538; no padded allocation. */
        orderedRenderEntries[entryIndex]=activeRenderEntry;
        activeEntryGeometry=activeRenderEntry->vectors;
        const uint16_t *vector=activeEntryGeometry[activeRenderEntry->firstVectorIndex];
        for (uint16_t component=0;component<3;++component) viewVectorCoefficients[component]=vector[component];
        const uint16_t *keyVector=viewVectorCoefficients;
        if (!useViewBasisDirectly)
        {
            BtechTransformWordVectorFar(viewVectorCoefficients,activeRenderBasis,transformedEntryVector);
            keyVector=transformedEntryVector;
        }
        renderEntryOrderingKeys[entryIndex]=BtechDotProductSignedWordVectors(resourceReferenceVector,keyVector);
        renderEntryVariants[entryIndex]=entryVariants?entryVariants[entryIndex]:0;
        if (entryVariants && renderEntryVariants[entryIndex]>=activeRenderEntry->variantCount)
            renderEntryVariants[entryIndex]=0;
        ++activeRenderEntry;
    }
    for (int16_t entryIndex=0;entryIndex<entryCount;++entryIndex)
    {
        int16_t greatestIndex=entryIndex;
        for (int16_t candidate=entryIndex+1;candidate<entryCount;++candidate)
            if ((int32_t)renderEntryOrderingKeys[greatestIndex]<(int32_t)renderEntryOrderingKeys[candidate]) greatestIndex=candidate;
        if (greatestIndex!=entryIndex)
        {
            uint8_t savedVariant=renderEntryVariants[entryIndex];
            renderEntryVariants[entryIndex]=renderEntryVariants[greatestIndex];
            renderEntryVariants[greatestIndex]=savedVariant;
            uint32_t savedKey=renderEntryOrderingKeys[entryIndex];
            renderEntryOrderingKeys[entryIndex]=renderEntryOrderingKeys[greatestIndex];
            renderEntryOrderingKeys[greatestIndex]=savedKey;
            activeRenderEntry=orderedRenderEntries[entryIndex];
            orderedRenderEntries[entryIndex]=orderedRenderEntries[greatestIndex];
            orderedRenderEntries[greatestIndex]=activeRenderEntry;
        }
    }
    uint8_t sharedGeometry=activeRenderHeader->flags&RESOURCE_SHARED_GEOMETRY;
    if (sharedGeometry)
    {
        activeRenderEntry=orderedRenderEntries[0]; /* Retained entry zero even when count<=0. */
        if (!activeRenderEntry) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !activeRenderEntry"); /* Unsupported native null-near alias. */
        renderGeometry=activeRenderEntry->vectors;
        renderGeometryVectorCount=activeRenderEntry->vectorCount;
        BtechClearGeometryVectorCacheFlags();
    }
    for (int16_t entryIndex=0;entryIndex<entryCount;++entryIndex)
    {
        activeRenderEntry=orderedRenderEntries[entryIndex];
        activeEntryGeometry=activeRenderEntry->vectors;
        activeVariant=&activeRenderEntry->variants[renderEntryVariants[entryIndex]];
        int16_t recordCount=activeVariant->primitiveCount;
        int16_t recordIndex=0;
        const BtechRenderPrimitive *selected=activeVariant->primitives;
        while (recordIndex<recordCount && selected->scaleLimit<objectProjectionScale)
        {++recordIndex;++selected;}
        activePrimitiveRecord=selected;
        if (recordIndex==recordCount) --activePrimitiveRecord; /* Count0 backs up too; negative doesn't. */
        if (!sharedGeometry)
        {
            renderGeometry=activeEntryGeometry;
            renderGeometryVectorCount=activeRenderEntry->vectorCount;
            BtechClearGeometryVectorCacheFlags();
        }
        switch (activePrimitiveRecord->kind)
        {
        case RENDER_PRIMITIVE_POLYGON: BtechDrawPolygonPayload(&activePrimitiveRecord->polygonPayload);break;
        case RENDER_PRIMITIVE_ELLIPSE: BtechDrawReferencePointEllipse(&activePrimitiveRecord->ellipsePayload);break;
        case RENDER_PRIMITIVE_ORIGIN_PICTURE: /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Render resource uses an unsupported origin-picture primitive."); /*176E. */
        case RENDER_PRIMITIVE_REFERENCE_PICTURE: /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Render resource uses an unsupported reference-picture primitive."); /*1862. */
        } /* Other native BYTE kinds really are skipped. No returning renderer seam. */
    }
}

/*22B6:1596. FAR payload pair becomes a typed six-byte primitive at bank IO.
 * Preserve shared projection scratch and the signed upper-only radius cap. */
void BtechDrawReferencePointEllipse(const BtechRenderEllipse *ellipse)
{
    const uint16_t *reference=activeEntryGeometry[ellipse->referenceVectorIndex];
    for (uint16_t component=0;component<3;++component) viewVectorCoefficients[component]=reference[component];
    BtechTransformWordVectorFar(viewVectorCoefficients,activeRenderBasis,renderTransformedReference);
    projectionRatio.vector.positionA=transformedViewPosition.positionA+(uint32_t)(int16_t)renderTransformedReference[0];
    projectionRatio.vector.positionB=transformedViewPosition.positionB+(uint32_t)(int16_t)renderTransformedReference[1];
    projectionRatio.vector.positionC=transformedViewPosition.positionC+(uint32_t)(int16_t)renderTransformedReference[2];
    if ((int32_t)(int16_t)nearClipDepth>(int32_t)projectionRatio.ratio.denominator) return;
    projectionVectorScratch.positionA=(uint32_t)ellipse->radiusScale;
    projectionVectorScratch.positionB=projectionRatio.ratio.denominator;
    uint32_t radius;
    BtechProjectionRatio radiusProjection={.ratio={projectionVectorScratch.positionA,projectionVectorScratch.positionB,0}};
    BtechProjectValueDepthRatio(&radiusProjection,&radius);
    if ((int32_t)radius>0x0100) radius=0x0100;
    BtechProjectViewPosition(&projectionRatio.vector,&projectionVectorScratch);
    BtechOffsetProjectedScreenPair(&projectionVectorScratch);
    if ((int32_t)(int16_t)(clipMinimumX-0x0100)>(int32_t)projectionVectorScratch.positionA
        || (int32_t)(int16_t)(clipMaximumX+0x0100)<(int32_t)projectionVectorScratch.positionA
        || (int32_t)(int16_t)(clipMinimumY-0x0100)>(int32_t)projectionVectorScratch.positionB
        || (int32_t)(int16_t)(clipMaximumY+0x0100)<(int32_t)projectionVectorScratch.positionB) return;
    screenPolygonNeedsClip=true;drawPolygonFilled=true;
    uint8_t colour=graphicsColourMode?ellipse->colour:ellipse->alternateColour;
    drawLineColour=activePaletteRemap?activePaletteRemap[colour]:colour;
    if (!graphicsColourMode) drawLineColour&=0x1F;
    drawFillColour=drawLineColour;
    BtechDrawStyledEllipse((int16_t)radius,(uint16_t)projectionVectorScratch.positionA,(uint16_t)projectionVectorScratch.positionB);
}
