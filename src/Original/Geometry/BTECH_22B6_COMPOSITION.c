#include "mechwarrior/combat_render.h"

/*22B6:0DB1. The planar branches pair the first product's HIGH WORD with
 * incoming CX, NOT its LOW WORD. Keep this original arithmetic quirk and each
 * ordered store: destination may be either complete input object. Normal DF=0. */
void BtechComposeTransformBases(uint16_t incomingCx,const BtechOrientationBasis *left,const BtechOrientationBasis *right,BtechOrientationBasis *destination)
{
    uint8_t leftMode=left->mode,rightMode=right->mode;
    if (!leftMode || !rightMode)
    {
        const BtechOrientationBasis *source=!leftMode?right:left;
        destination->mode=source->mode;
        for (uint16_t coefficient=0;coefficient<9;++coefficient)
            destination->coefficients[coefficient]=source->coefficients[coefficient];
        return;
    }
    bool leftPlanar=leftMode==1,rightPlanar=rightMode==1;
    for (uint16_t row=0;row<(leftPlanar?2:3);++row)
    {
        for (uint16_t column=0;column<(rightPlanar?2:3);++column)
        {
            int16_t firstLeft=left->coefficients[row*3];
            int16_t firstRight=right->coefficients[column];
            uint32_t firstProduct=(int32_t)firstLeft*firstRight;
            int16_t secondLeft=left->coefficients[row*3+1];
            int16_t secondRight=right->coefficients[column+3];
            uint32_t secondProduct=(int32_t)secondLeft*secondRight;
            uint32_t accumulated;
            if (!leftPlanar && !rightPlanar)
            {
                accumulated=firstProduct+secondProduct;
                int16_t thirdLeft=left->coefficients[row*3+2];
                int16_t thirdRight=right->coefficients[column+6];
                accumulated+=(uint32_t)((int32_t)thirdLeft*thirdRight);
            }
            else accumulated=((firstProduct&0xFFFF0000)|incomingCx)+secondProduct;
            destination->coefficients[row*3+column]=(uint16_t)((accumulated<<2)>>16);
        }
        if (!leftPlanar && rightPlanar)
            destination->coefficients[row*3+2]=left->coefficients[row*3+2];
    }
    if (leftPlanar && rightPlanar)
    {
        destination->coefficients[2]=0;
        destination->coefficients[5]=0;
        destination->coefficients[6]=0;
        destination->coefficients[7]=0;
        destination->coefficients[8]=0x4000;
        destination->mode=1;
    }
    else
    {
        if (leftPlanar)
            for (uint16_t column=0;column<3;++column)
                destination->coefficients[column+6]=right->coefficients[column+6];
        destination->mode=3;
    }
}

/*22B6:0843. Signed WORD NEG/SAR, including the minimum-WORD alias into the
 * earlier cosine table. The native inverse lookup is not a clamped asin. */
uint16_t BtechLookupSignedCoefficientAngle(uint16_t coefficient)
{
    bool negative=(int16_t)coefficient<0;
    if (negative) coefficient=(uint16_t)(0-coefficient);
    uint16_t angle=coefficient==0x8000?orientationCosines[0x401]:coefficientAngles[coefficient>>5];
    if (negative) angle=(uint16_t)(0x1000-angle);
    return (uint16_t)(angle<<4);
}

/*22B6:08AD. Smaller/larger ratio, the authored CS table, then quadrant folds. */
uint16_t BtechCalculateWordPairHeading(uint16_t componentA,uint16_t componentB)
{
    bool negativeA=(int16_t)componentA<0,negativeB=(int16_t)componentB<0;
    if (negativeA) componentA=(uint16_t)(0-componentA);
    if (negativeB) componentB=(uint16_t)(0-componentB);
    uint16_t angle;
    if (!componentA && !componentB) return 0;
    if (!componentA) angle=0x400;
    else if (!componentB) angle=0;
    else if (componentA==componentB) angle=0x200;
    else
    {
        uint16_t smaller=componentA<componentB?componentA:componentB;
        uint16_t larger=componentA<componentB?componentB:componentA;
        angle=headingRatioAngles[((uint32_t)smaller<<9)/larger];
        if (componentA<componentB) angle=(uint16_t)(0x400-angle);
    }
    if (negativeA) angle=(uint16_t)(0x800-angle);
    if (negativeB) angle=(uint16_t)(0x1000-angle);
    return (uint16_t)(angle<<4);
}

/*22B6:0D1F. Ordered angle stores, with the original quarter-turn branch. */
void BtechExtractOrientationFromBasis(const BtechOrientationBasis *basis,uint16_t orientation[3])
{
    uint16_t firstAngle=BtechLookupSignedCoefficientAngle(basis->coefficients[5]);
    orientation[0]=firstAngle;
    uint16_t magnitude=(int16_t)firstAngle<0?(uint16_t)(0-firstAngle):firstAngle;
    if (magnitude==0x4000)
    {
        orientation[1]=0;
        uint16_t second=basis->coefficients[1];
        uint16_t first=basis->coefficients[0];
        orientation[2]=BtechCalculateWordPairHeading(first,second);
    }
    else
    {
        uint16_t second=0-basis->coefficients[2];
        uint16_t first=basis->coefficients[8];
        orientation[1]=BtechCalculateWordPairHeading(first,second);
        second=(uint16_t)(0-basis->coefficients[3]);
        first=basis->coefficients[4];
        orientation[2]=BtechCalculateWordPairHeading(first,second);
    }
}
