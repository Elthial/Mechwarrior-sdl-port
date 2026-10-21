#include "mechwarrior/port_diagnostics.h"
#include "mechwarrior/combat_render.h"
#include <stdlib.h>

/*22B6:13EB. Genuine NEAR counterpart to12C4, not a new game helper. */
void BtechTransformWordVector(const uint16_t source[3],const BtechOrientationBasis *basis,uint16_t destination[3])
{
    uint8_t mode=basis->mode;
    for (uint16_t output=0;output<3;++output)
    {
        if (!mode || (mode==1 && output==2)) destination[output]=source[output];
        else
        {
            uint32_t product=0;
            for (uint16_t input=0;input<(mode==1?2:3);++input)
                product+=(uint32_t)((int32_t)(int16_t)source[input]*(int16_t)basis->coefficients[input*3+output]);
            destination[output]=(uint16_t)((product<<2)>>16);
        }
    }
}

/*22B6:1F7C. WORD input, DWORD integer output and three live fractional WORDs.
 * Products wrap to32 bits BEFORE sign extension into DX:AX:BP and shifting.
 * Keep each destination store before its aliased7864 fraction store. */
void BtechTransformWordVectorToDword(const uint16_t source[3],const BtechOrientationBasis *basis,BtechPosition3D *destination)
{
    uint8_t mode=basis->mode;
    uint32_t *components[]={&destination->positionA,&destination->positionB,&destination->positionC};
    for (uint16_t output=0;output<3;++output)
    {
        if (!mode || (mode==1 && output==2))
        {
            uint32_t value=(int16_t)source[output];
            for (uint16_t shifts=resourceScaleShift;shifts;--shifts) value<<=1;
            *components[output]=value;
            geometryTransformFractions[output]=0;
        }
        else
        {
            uint32_t product=0;
            for (uint16_t input=0;input<(mode==1?2:3);++input)
                product+=(uint32_t)((int32_t)(int16_t)source[input]*(int16_t)basis->coefficients[input*3+output]);
            uint64_t shifted=(int32_t)product;
            shifted=(shifted<<2)&UINT64_C(0xFFFFFFFFFFFF);
            for (uint16_t shifts=resourceScaleShift;shifts;--shifts) shifted=(shifted<<1)&UINT64_C(0xFFFFFFFFFFFF);
            *components[output]=(uint32_t)(shifted>>16);
            geometryTransformFractions[output]=(uint16_t)shifted;
        }
    }
}

/*22B6:2175. Any nonzero flag reuses the existing cache, including normals.
 * ADD fraction,0 clears carry: translation is a wrapped DWORD integer add. */
void BtechCacheTransformedGeometryVector(uint8_t vectorIndex)
{
    if (vectorIndex>=GEOMETRY_CACHE_VERTICES) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: vectorIndex>=GEOMETRY_CACHE_VERTICES"); /* FF aliases X-cache bytes in the native code owner. */
    if (vertexNearPlaneState[vectorIndex]) return;
    vertexNearPlaneState[vectorIndex]=1;
    for (uint16_t axis=0;axis<3;++axis) viewVectorCoefficients[axis]=renderGeometry[vectorIndex][axis];
    BtechTransformWordVectorToDword(viewVectorCoefficients,activeRenderBasis,&projectionRatio.vector);
    geometryViewX.wide[vectorIndex]=projectionRatio.vector.positionA+transformedViewPosition.positionA;
    geometryViewDepth.wide[vectorIndex]=projectionRatio.vector.positionB+transformedViewPosition.positionB;
    geometryViewZ.wide[vectorIndex]=projectionRatio.vector.positionC+transformedViewPosition.positionC;
}

/*22B6:2227. WORD normal values occupy LOW WORDs at DWORD strides. The high
 * halves are intentionally stale and share storage with transformed vertices. */
void BtechCacheRotatedGeometryVector(uint8_t vectorIndex)
{
    if (vectorIndex>=GEOMETRY_CACHE_VERTICES) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: vectorIndex>=GEOMETRY_CACHE_VERTICES");
    if (vertexNearPlaneState[vectorIndex]) return;
    vertexNearPlaneState[vectorIndex]=1;
    for (uint16_t axis=0;axis<3;++axis) viewVectorCoefficients[axis]=renderGeometry[vectorIndex][axis];
    BtechTransformWordVector(viewVectorCoefficients,activeRenderBasis,transformedEntryVector);
    geometryViewX.words[vectorIndex*2]=transformedEntryVector[0];
    geometryViewDepth.words[vectorIndex*2]=transformedEntryVector[1];
    geometryViewZ.words[vectorIndex*2]=transformedEntryVector[2];
}

/*22B6:2586, including its24E9 direct-basis tail. Native result DX:AX:CX;
 * uint64_t only transports those48 bits. Callers test signed HIGH WORD. */
uint64_t BtechGetWidePolygonFacingDotHighWord(uint8_t normalIndex,uint8_t firstVertexIndex)
{
    if (useViewBasisDirectly)
    {
        if (normalIndex>=GEOMETRY_CACHE_VERTICES || firstVertexIndex>=GEOMETRY_CACHE_VERTICES) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: normalIndex>=GEOMETRY_CACHE_VERTICES || firstVertexIndex>=GEOMETRY_CACHE_VERTICES"); /* FF geometry overread is outside the recovered array. */
        uint32_t *position[]={&projectionVectorScratch.positionA,&projectionVectorScratch.positionB,&projectionVectorScratch.positionC};
        const uint32_t *relative[]={&cameraRelativePosition.positionA,&cameraRelativePosition.positionB,&cameraRelativePosition.positionC};
        for (uint16_t axis=0;axis<3;++axis)
        {
            uint32_t value=(int16_t)renderGeometry[firstVertexIndex][axis];
            for (uint16_t shifts=resourceScaleShift;shifts;--shifts) value<<=1;
            *position[axis]=value+*relative[axis];
        }
        for (uint16_t axis=0;axis<3;++axis) viewVectorCoefficients[axis]=renderGeometry[normalIndex][axis];
    }
    else
    {
        BtechCacheRotatedGeometryVector(normalIndex);
        BtechCacheTransformedGeometryVector(firstVertexIndex);
        projectionVectorScratch.positionA=geometryViewX.wide[firstVertexIndex];
        projectionVectorScratch.positionB=geometryViewDepth.wide[firstVertexIndex];
        projectionVectorScratch.positionC=geometryViewZ.wide[firstVertexIndex];
        viewVectorCoefficients[0]=geometryViewX.words[normalIndex*2];
        viewVectorCoefficients[1]=geometryViewDepth.words[normalIndex*2];
        viewVectorCoefficients[2]=geometryViewZ.words[normalIndex*2];
    }
    return BtechDotSignedVectorByWordCoefficients(&projectionVectorScratch,viewVectorCoefficients);
}

/*22B6:6473. Shade normals use WORD strides on the narrow path and DWORD
 * strides on the wide path. Neither path clears the other half of a slot. */
void BtechCachePolygonShadeNormal(uint8_t normalIndex)
{
    if (normalIndex>=GEOMETRY_CACHE_VERTICES) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: normalIndex>=GEOMETRY_CACHE_VERTICES");
    uint16_t wordIndex=normalIndex*(widePolygonCoordinates?2:1);
    if (vertexNearPlaneState[normalIndex])
    {
        viewVectorCoefficients[0]=geometryViewX.words[wordIndex];
        viewVectorCoefficients[1]=geometryViewDepth.words[wordIndex];
        viewVectorCoefficients[2]=geometryViewZ.words[wordIndex];
        return;
    }
    vertexNearPlaneState[normalIndex]=1;
    for (uint16_t axis=0;axis<3;++axis) transformedEntryVector[axis]=renderGeometry[normalIndex][axis];
    BtechTransformWordVector(transformedEntryVector,activeRenderBasis,viewVectorCoefficients);
    geometryViewX.words[wordIndex]=viewVectorCoefficients[0];
    geometryViewDepth.words[wordIndex]=viewVectorCoefficients[1];
    geometryViewZ.words[wordIndex]=viewVectorCoefficients[2];
}
