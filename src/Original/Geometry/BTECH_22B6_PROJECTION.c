#include "mechwarrior/combat_render.h"
#include <stdlib.h>

/*22B6:0D8C. Swap the three off-diagonal WORD pairs; mode stays unchanged. */
void BtechTransposeTransformBasis(BtechOrientationBasis *basis)
{
    uint16_t first=basis->coefficients[1],second=basis->coefficients[3];
    basis->coefficients[3]=first;basis->coefficients[1]=second;
    first=basis->coefficients[2];second=basis->coefficients[6];
    basis->coefficients[6]=first;basis->coefficients[2]=second;
    first=basis->coefficients[5];second=basis->coefficients[7];
    basis->coefficients[7]=first;basis->coefficients[5]=second;
}

/*22B6:228D. DX:AX:CX becomes a host unsigned48-bit result transport only.
 * Native first-product and pre-third-product scratch stores remain visible.
 * Inputs are re-read after those stores; zero third coefficient skips its input. */
uint64_t BtechDotSignedVectorByWordCoefficients(const BtechPosition3D *vector,const uint16_t coefficients[3])
{
    const uint32_t *components[]={&vector->positionA,&vector->positionB,&vector->positionC};
    uint64_t total=0;
    for (uint16_t component=0;component<3;++component)
    {
        uint16_t coefficient;
        if (component==2)
        {
            coefficient=coefficients[component];
            if (!coefficient) break;
            dotProductScratchHigh=(uint16_t)(total>>32);
            dotProductScratchMiddle=(uint16_t)(total>>16);
            dotProductScratchLow=(uint16_t)total;
        }
        uint32_t value=*components[component];
        if (component!=2) coefficient=coefficients[component];
        uint64_t product=(uint64_t)((int64_t)(int32_t)value*(int16_t)coefficient)&UINT64_C(0xFFFFFFFFFFFF);
        if (!component)
        {
            total=product;
            dotProductScratchHigh=(uint16_t)(total>>32);
            dotProductScratchMiddle=(uint16_t)(total>>16);
            dotProductScratchLow=(uint16_t)total;
        }
        else total=(product+((uint64_t)dotProductScratchHigh<<32)
            +((uint64_t)dotProductScratchMiddle<<16)+dotProductScratchLow)&UINT64_C(0xFFFFFFFFFFFF);
    }
    return total;
}

/*22B6:2395. Q14 signed48 dot products, then the original two SHL/RCLs.
 * Each destination is stored before reading the next source component. */
void BtechTransformViewVector(const BtechPosition3D *input,const BtechOrientationBasis *basis,BtechPosition3D *output)
{
    const uint32_t *source[]={&input->positionA,&input->positionB,&input->positionC};
    uint32_t *destination[]={&output->positionA,&output->positionB,&output->positionC};
    uint16_t transformedComponents=basis->mode==0?0:basis->mode==1?2:3;
    for (uint16_t component=0;component<3;++component)
    {
        if (component>=transformedComponents) *destination[component]=*source[component];
        else
        {
            viewVectorCoefficients[0]=basis->coefficients[component];
            viewVectorCoefficients[1]=basis->coefficients[component+3];
            viewVectorCoefficients[2]=basis->coefficients[component+6];
            uint64_t product=BtechDotSignedVectorByWordCoefficients(input,viewVectorCoefficients);
            *destination[component]=(uint32_t)(product>>14);
        }
    }
}

/*22B6:1E92. Original sign-extended48 numerator shift; CL compare is signed,
 * its byte subtract differs from full-CX LOOP, and initial CX0 runs65536 shifts. */
uint16_t BtechProjectViewPosition(const BtechPosition3D *input,BtechPosition3D *output)
{
    const uint32_t *source[]={&input->positionA,&input->positionC};
    uint32_t *destination[]={&output->positionA,&output->positionB};
    uint16_t retainedCx;
    for (uint16_t component=0;component<2;++component)
    {
        uint32_t value=*source[component];
        uint16_t low=(uint16_t)value,middle=(uint16_t)(value>>16),high=(int16_t)middle<0?UINT16_MAX:0;
        uint32_t denominator=input->positionB;
        uint16_t shifts=combatProjectionShift;
        if ((uint8_t)shifts>=8 && (uint8_t)shifts<0x80)
        {
            high=(high&0xFF00)|(middle>>8);middle=(uint16_t)((middle<<8)|(low>>8));low=(uint16_t)(low<<8);
            shifts=(shifts&0xFF00)|(uint8_t)((uint8_t)shifts-8);
            if (!(uint8_t)shifts) goto divideNumerator;
        }
        do
        {high=(uint16_t)((high<<1)|(middle>>15));middle=(uint16_t)((middle<<1)|(low>>15));low=(uint16_t)(low<<1);} while (--shifts);
divideNumerator:
        *destination[component]=BtechDivideSignedWideNumerator((int16_t)high<0?-1:0,high,middle,low,(uint16_t)denominator,(int16_t)(denominator>>16),&retainedCx);
    }
    return retainedCx; /* Last665E CX survives2EB2 into the object's0960/0DB1. */
}

/*22B6:1F2A. Separate original entry: only one numerator/depth pair. */
void BtechProjectValueDepthRatio(const BtechProjectionRatio *input,uint32_t *output)
{
    uint32_t value=input->ratio.numerator;
    uint16_t low=(uint16_t)value,middle=(uint16_t)(value>>16),high=(int16_t)middle<0?UINT16_MAX:0;
    uint32_t denominator=input->ratio.denominator;
    uint16_t shifts=combatProjectionShift;
    if ((uint8_t)shifts>=8 && (uint8_t)shifts<0x80)
    {
        high=(high&0xFF00)|(middle>>8);middle=(uint16_t)((middle<<8)|(low>>8));low=(uint16_t)(low<<8);
        shifts=(shifts&0xFF00)|(uint8_t)((uint8_t)shifts-8);
        if (!(uint8_t)shifts) goto divideNumerator;
    }
    do
    {high=(uint16_t)((high<<1)|(middle>>15));middle=(uint16_t)((middle<<1)|(low>>15));low=(uint16_t)(low<<1);} while (--shifts);
divideNumerator:
    *output=BtechDivideSignedWideNumerator((int16_t)high<0?-1:0,high,middle,low,(uint16_t)denominator,(int16_t)(denominator>>16),NULL);
}

/*22B6:2EB2. Ordinary typed pair owners. Partial-WORD aliases into centres or
 * flags are not independent C objects; exact input/output aliases remain live. */
void BtechOffsetProjectedScreenPair(BtechPosition3D *screen)
{
    uint8_t mirrored=mirrorProjectionA;
    uint32_t centre=(uint32_t)screenCentreA;
    screen->positionA=mirrored?centre-screen->positionA:centre+screen->positionA;
    centre=(uint32_t)screenCentreB;
    screen->positionB=centre-screen->positionB;
}
