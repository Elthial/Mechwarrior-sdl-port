#include "mechwarrior/combat_render.h"

/*0810:D93E. Native signed extent/coordinate tests, wrapped absolute values,
 * arithmetic shifts and reference-vector staging. No host floating point. */
void BtechPrepareResourceCoordinatePaths(void)
{
    if (projectionScaleMultiplier!=0x100)
    {
        uint32_t product=(int32_t)(int16_t)projectionScaleMultiplier*(int16_t)objectProjectionScale;
        for (uint8_t shifts=8;shifts;--shifts) product=(product>>1)|(product&0x80000000);
        objectProjectionScale=(int32_t)product<0?0:(int32_t)product>0x100?0x100:(uint16_t)product;
    }
    resourceScaleShift=activeRenderHeader->scaleShift;
    uint32_t magnitude=transformedViewPosition.positionA;
    if ((int32_t)magnitude<0) magnitude=0-magnitude;
    projectionVectorScratch.positionA=magnitude;
    magnitude=transformedViewPosition.positionB;
    if ((int32_t)magnitude<0) magnitude=0-magnitude;
    projectionVectorScratch.positionB=magnitude;
    magnitude=transformedViewPosition.positionC;
    if ((int32_t)magnitude<0) magnitude=0-magnitude;
    projectionVectorScratch.positionC=magnitude;
    uint32_t maximumMagnitude=projectionVectorScratch.positionA;
    if ((int32_t)maximumMagnitude<(int32_t)projectionVectorScratch.positionB)
        maximumMagnitude=projectionVectorScratch.positionB;
    if ((int32_t)maximumMagnitude<(int32_t)projectionVectorScratch.positionC)
        maximumMagnitude=projectionVectorScratch.positionC;
    uint32_t maximumPlusExtent=maximumMagnitude+resourceScaledExtent;
    widePolygonCoordinates=resourceScaleShift!=0 || (int32_t)resourceScaledExtent>0x3FFC
        || (int32_t)maximumPlusExtent>0x7FBC;
    useWordProductClipping=(int32_t)resourceScaledExtent<0x3FAC;
    uint16_t coordinateShift=0;
    if (widePolygonCoordinates && (int32_t)maximumMagnitude>0x3FFF)
        while ((int32_t)maximumMagnitude>0x3FFF)
        {maximumMagnitude=(maximumMagnitude>>1)|(maximumMagnitude&0x80000000);++coordinateShift;}
    uint16_t *reference=useViewBasisDirectly?resourceReferenceVector:viewVectorCoefficients;
    for (uint16_t component=0;component<3;++component) reference[component]=activeRenderHeader->referenceVector[component];
    const BtechPosition3D *position=&cameraRelativePosition;
    if (!useViewBasisDirectly)
    {
        BtechTransformWordVectorFar(viewVectorCoefficients,activeRenderBasis,resourceReferenceVector);
        position=&transformedViewPosition;
    }
    const uint32_t *components[]={&position->positionA,&position->positionB,&position->positionC};
    for (uint16_t component=0;component<3;++component)
    {
        uint32_t value=*components[component];
        for (uint8_t shifts=coordinateShift;shifts;--shifts) value=(value>>1)|(value&0x80000000);
        resourceReferenceVector[component]+=(uint16_t)value;
    }
    skipWordPolygonRenderer=0;
}
