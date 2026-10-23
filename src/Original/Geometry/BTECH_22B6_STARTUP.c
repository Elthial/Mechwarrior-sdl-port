#include "mechwarrior/combat_video.h"
#include "mechwarrior/combat_world.h"
#include <stdlib.h>

/*22B6:01D8. Two real orientation arguments, no phantom third word. */
void BtechBuildCameraReferenceVector(uint16_t orientation0,uint16_t orientation2)
{
    uint16_t orientation[3]={orientation0,0,orientation2};
    uint16_t reference[3]={0,0x0200,0};
    BtechBuildOrientationMatrix(orientation,&cameraOrientationBasis,0); /* CX result unused. */
    BtechTransformWordVectorFar(reference,&cameraOrientationBasis,cameraReferenceVector);
}

/*22B6:0224. Only five BYTE flags are cleared, not the camera records. */
void BtechResetCameraContextsAndReferenceVector(void)
{
    for (uint16_t camera=0;camera<COMBAT_CAMERA_CONTEXTS;++camera) cameraContexts[camera].flags=0;
    BtechBuildCameraReferenceVector(0x2000,0xA000);
}

/*22B6:12C4. Ordered reads/stores allow source/destination aliases. Products
 * accumulate modulo2^32 before the original SHL/RCL pair selects DX. */
void BtechTransformWordVectorFar(const uint16_t source[3],const BtechOrientationBasis *basis,uint16_t destination[3])
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

uint16_t BtechBuildOrientationMatrix(const uint16_t orientation[3],BtechOrientationBasis *basis,uint16_t incomingCx)
{
    /*22B6:0960. Ordered Q14 coefficients and native CS cosine table.
     * CX is live into0DB1: low modes retain incoming CH, full modes return
     * the first half of the last sine pair. This is register transport only. */
    uint8_t mode=3;
    if (orientation[0]==0x0000)
    {
        --mode;
        if (orientation[1]==0x0000)
        {
            --mode;
            if (orientation[2]==0x0000) --mode;
        }
    }
    basis->mode=mode;
    if (mode==0x00)
    {
        basis->coefficients[0]=0x4000;
        basis->coefficients[4]=0x4000;
        basis->coefficients[8]=0x4000;
        basis->coefficients[1]=0;
        basis->coefficients[2]=0;
        basis->coefficients[3]=0;
        basis->coefficients[5]=0;
        basis->coefficients[6]=0;
        basis->coefficients[7]=0;
        return incomingCx&0xFF00;
    }
    if (mode==0x01)
    {
        uint16_t headingCosine=orientation[2];
        headingCosine >>= 0x0004;
        if ((headingCosine&0x0800)!=0x0000) headingCosine=(uint16_t)(0x1000-headingCosine);
        headingCosine=orientationCosines[headingCosine];
        basis->coefficients[0]=(uint16_t)(headingCosine);
        basis->coefficients[4]=(uint16_t)(headingCosine);
        uint16_t headingSine = orientation[2] + 0xC000;
        headingSine >>= 0x0004;
        if ((headingSine&0x0800)!=0x0000) headingSine=(uint16_t)(0x1000-headingSine);
        headingSine=orientationCosines[headingSine];
        basis->coefficients[1]=(uint16_t)(headingSine);
        basis->coefficients[3]=(uint16_t)(0x0000-headingSine);
        basis->coefficients[2]=0;
        basis->coefficients[5]=0;
        basis->coefficients[6]=0;
        basis->coefficients[7]=0;
        basis->coefficients[8]=0x4000;
        return (incomingCx&0xFF00)|1;
    }
    uint16_t firstSine = orientation[0] + 0xC000;
    firstSine >>= 0x0004;
    if ((firstSine&0x0800)!=0x0000) firstSine=(uint16_t)(0x1000-firstSine);
    firstSine=orientationCosines[firstSine];
    basis->coefficients[5]=(uint16_t)(firstSine);
    uint16_t cosineSum=orientation[1] - orientation[2];
    cosineSum >>= 0x0004;
    if ((cosineSum&0x0800)!=0x0000) cosineSum=(uint16_t)(0x1000-cosineSum);
    cosineSum=orientationCosines[cosineSum];
    cosineSum=(uint16_t)((cosineSum>>0x0001)|(cosineSum&0x8000)); // SAR WORD, not signed host shift.
    uint16_t sumCosine=orientation[1] + orientation[2];
    sumCosine >>= 0x0004;
    if ((sumCosine&0x0800)!=0x0000) sumCosine=(uint16_t)(0x1000-sumCosine);
    sumCosine=orientationCosines[sumCosine];
    sumCosine=(uint16_t)((sumCosine>>0x0001)|(sumCosine&0x8000)); // SAR WORD, not signed host shift.
    cosineSum=(uint16_t)(cosineSum+sumCosine);
    uint16_t cosineDifference=orientation[1] - orientation[2];
    cosineDifference >>= 0x0004;
    if ((cosineDifference&0x0800)!=0x0000) cosineDifference=(uint16_t)(0x1000-cosineDifference);
    cosineDifference=orientationCosines[cosineDifference];
    cosineDifference=(uint16_t)((cosineDifference>>0x0001)|(cosineDifference&0x8000)); // SAR WORD, not signed host shift.
    uint16_t sumCosineSecond=orientation[1] + orientation[2];
    sumCosineSecond >>= 0x0004;
    if ((sumCosineSecond&0x0800)!=0x0000) sumCosineSecond=(uint16_t)(0x1000-sumCosineSecond);
    sumCosineSecond=orientationCosines[sumCosineSecond];
    sumCosineSecond=(uint16_t)((sumCosineSecond>>0x0001)|(sumCosineSecond&0x8000)); // SAR WORD, not signed host shift.
    cosineDifference=(uint16_t)(cosineDifference-sumCosineSecond);
    uint32_t product=(int32_t)(int16_t)firstSine*(int16_t)cosineDifference;
    basis->coefficients[0]=(uint16_t)(cosineSum-(uint16_t)((product<<0x0002)>>0x0010));
    product=(uint32_t)((int32_t)(int16_t)firstSine*(int16_t)cosineSum);
    basis->coefficients[7]=(uint16_t)(cosineDifference-(uint16_t)((product<<0x0002)>>0x0010));
    uint16_t thirdSineSecondCosine=orientation[2] + orientation[1];
    thirdSineSecondCosine=(uint16_t)(thirdSineSecondCosine+0xC000);
    thirdSineSecondCosine >>= 0x0004;
    if ((thirdSineSecondCosine&0x0800)!=0x0000) thirdSineSecondCosine=(uint16_t)(0x1000-thirdSineSecondCosine);
    thirdSineSecondCosine=orientationCosines[thirdSineSecondCosine];
    thirdSineSecondCosine=(uint16_t)((thirdSineSecondCosine>>0x0001)|(thirdSineSecondCosine&0x8000)); // SAR WORD, not signed host shift.
    uint16_t differenceSine=orientation[2] - orientation[1];
    differenceSine=(uint16_t)(differenceSine+0xC000);
    differenceSine >>= 0x0004;
    if ((differenceSine&0x0800)!=0x0000) differenceSine=(uint16_t)(0x1000-differenceSine);
    differenceSine=orientationCosines[differenceSine];
    differenceSine=(uint16_t)((differenceSine>>0x0001)|(differenceSine&0x8000)); // SAR WORD, not signed host shift.
    thirdSineSecondCosine=(uint16_t)(thirdSineSecondCosine+differenceSine);
    uint16_t secondSineThirdCosine=orientation[1] + orientation[2];
    secondSineThirdCosine=(uint16_t)(secondSineThirdCosine+0xC000);
    secondSineThirdCosine >>= 0x0004;
    if ((secondSineThirdCosine&0x0800)!=0x0000) secondSineThirdCosine=(uint16_t)(0x1000-secondSineThirdCosine);
    secondSineThirdCosine=orientationCosines[secondSineThirdCosine];
    secondSineThirdCosine=(uint16_t)((secondSineThirdCosine>>0x0001)|(secondSineThirdCosine&0x8000)); // SAR WORD, not signed host shift.
    uint16_t differenceSineSecond=orientation[1] - orientation[2];
    differenceSineSecond=(uint16_t)(differenceSineSecond+0xC000);
    differenceSineSecond >>= 0x0004;
    if ((differenceSineSecond&0x0800)!=0x0000) differenceSineSecond=(uint16_t)(0x1000-differenceSineSecond);
    differenceSineSecond=orientationCosines[differenceSineSecond];
    differenceSineSecond=(uint16_t)((differenceSineSecond>>0x0001)|(differenceSineSecond&0x8000)); // SAR WORD, not signed host shift.
    secondSineThirdCosine=(uint16_t)(secondSineThirdCosine+differenceSineSecond);
    product=(uint32_t)((int32_t)(int16_t)firstSine*(int16_t)secondSineThirdCosine);
    basis->coefficients[1]=(uint16_t)((uint16_t)((product<<0x0002)>>0x0010)+thirdSineSecondCosine);
    product=(uint32_t)((int32_t)(int16_t)firstSine*(int16_t)thirdSineSecondCosine);
    basis->coefficients[6]=(uint16_t)((uint16_t)((product<<0x0002)>>0x0010)+secondSineThirdCosine);
    uint16_t firstHalf08=orientation[0] + orientation[2];
    firstHalf08 >>= 0x0004;
    if ((firstHalf08&0x0800)!=0x0000) firstHalf08=(uint16_t)(0x1000-firstHalf08);
    firstHalf08=orientationCosines[firstHalf08];
    firstHalf08=(uint16_t)((firstHalf08>>0x0001)|(firstHalf08&0x8000)); // SAR WORD, not signed host shift.
    uint16_t secondHalf08=orientation[0] - orientation[2];
    secondHalf08 >>= 0x0004;
    if ((secondHalf08&0x0800)!=0x0000) secondHalf08=(uint16_t)(0x1000-secondHalf08);
    secondHalf08=orientationCosines[secondHalf08];
    secondHalf08=(uint16_t)((secondHalf08>>0x0001)|(secondHalf08&0x8000)); // SAR WORD, not signed host shift.
    basis->coefficients[4]=(uint16_t)(firstHalf08+secondHalf08);
    uint16_t firstHalf10=orientation[0] + orientation[1];
    firstHalf10 >>= 0x0004;
    if ((firstHalf10&0x0800)!=0x0000) firstHalf10=(uint16_t)(0x1000-firstHalf10);
    firstHalf10=orientationCosines[firstHalf10];
    firstHalf10=(uint16_t)((firstHalf10>>0x0001)|(firstHalf10&0x8000)); // SAR WORD, not signed host shift.
    uint16_t secondHalf10=orientation[0] - orientation[1];
    secondHalf10 >>= 0x0004;
    if ((secondHalf10&0x0800)!=0x0000) secondHalf10=(uint16_t)(0x1000-secondHalf10);
    secondHalf10=orientationCosines[secondHalf10];
    secondHalf10=(uint16_t)((secondHalf10>>0x0001)|(secondHalf10&0x8000)); // SAR WORD, not signed host shift.
    basis->coefficients[8]=(uint16_t)(firstHalf10+secondHalf10);
    uint16_t firstHalf04=orientation[1] - orientation[0];
    firstHalf04=(uint16_t)(firstHalf04+0xC000);
    firstHalf04 >>= 0x0004;
    if ((firstHalf04&0x0800)!=0x0000) firstHalf04=(uint16_t)(0x1000-firstHalf04);
    firstHalf04=orientationCosines[firstHalf04];
    firstHalf04=(uint16_t)((firstHalf04>>0x0001)|(firstHalf04&0x8000)); // SAR WORD, not signed host shift.
    uint16_t secondHalf04=orientation[1] + orientation[0];
    secondHalf04=(uint16_t)(secondHalf04+0xC000);
    secondHalf04 >>= 0x0004;
    if ((secondHalf04&0x0800)!=0x0000) secondHalf04=(uint16_t)(0x1000-secondHalf04);
    secondHalf04=orientationCosines[secondHalf04];
    secondHalf04=(uint16_t)((secondHalf04>>0x0001)|(secondHalf04&0x8000)); // SAR WORD, not signed host shift.
    basis->coefficients[2]=(uint16_t)(0x0000-(firstHalf04+secondHalf04));
    uint16_t firstHalf06=orientation[2] - orientation[0];
    firstHalf06=(uint16_t)(firstHalf06+0xC000);
    firstHalf06 >>= 0x0004;
    if ((firstHalf06&0x0800)!=0x0000) firstHalf06=(uint16_t)(0x1000-firstHalf06);
    firstHalf06=orientationCosines[firstHalf06];
    firstHalf06=(uint16_t)((firstHalf06>>0x0001)|(firstHalf06&0x8000)); // SAR WORD, not signed host shift.
    uint16_t secondHalf06=orientation[2] + orientation[0];
    secondHalf06=(uint16_t)(secondHalf06+0xC000);
    secondHalf06 >>= 0x0004;
    if ((secondHalf06&0x0800)!=0x0000) secondHalf06=(uint16_t)(0x1000-secondHalf06);
    secondHalf06=orientationCosines[secondHalf06];
    secondHalf06=(uint16_t)((secondHalf06>>0x0001)|(secondHalf06&0x8000)); // SAR WORD, not signed host shift.
    basis->coefficients[3]=(uint16_t)(0x0000-(firstHalf06+secondHalf06));
    return firstHalf06;
}
