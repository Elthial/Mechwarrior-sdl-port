#include "mechwarrior/combat_render.h"
#include <stdlib.h>

/*22B6:2908. Native endpoint construction, signed WORD clipping and fill
 * decisions. Non-horizontal split polygons still need0475; no flat substitute. */
void BtechFillTiltedCameraHorizonRegions(uint16_t fillA,uint16_t fillB,uint16_t pitch,uint16_t roll)
{
    drawPolygonFilled=1;
    if (pitch>0x4000 && pitch<=0xC000)
    {
        uint16_t saved=fillA;fillA=fillB;fillB=saved;
        pitch+=0x8000;
    }
    if (roll>0x4000 && roll<=0xC000)
    {
        uint16_t saved=fillA;fillA=fillB;fillB=saved;
        roll+=0x8000;pitch+=0x8000;
    }
    uint16_t angles[]={(uint16_t)(pitch+0xC000),pitch,(uint16_t)(roll+0xC000),roll};
    int16_t trigonometry[4];
    for (uint16_t component=0;component<4;++component)
    {
        uint16_t index=angles[component]>>4;
        if (index&0x0800) index=(uint16_t)(0x1000-index);
        trigonometry[component]=(int16_t)orientationCosines[index];
        if (component==1)
        {
            if (!trigonometry[component]) trigonometry[component]=(int16_t)orientationCosines[0x03FF];
            if (trigonometry[component]<0) trigonometry[component]=(int16_t)(uint16_t)(0-(uint16_t)trigonometry[component]);
        }
    }
    uint16_t rollSin=(uint16_t)trigonometry[2],rollCos=(uint16_t)trigonometry[3];
    for (uint16_t shift=0;shift<6;++shift)
    {rollSin=(rollSin>>1)|(rollSin&0x8000);rollCos=(rollCos>>1)|(rollCos&0x8000);}
    cameraDepthScratch=(cameraDepthScratch&UINT32_C(0xFFFF0000))|(uint16_t)trigonometry[1];
    uint32_t numerator=(uint32_t)(int32_t)trigonometry[0];
    uint16_t shifts=combatProjectionShift;
    if ((uint8_t)shifts>=8 && (uint8_t)shifts<0x80)
    {
        numerator<<=8;
        shifts=(shifts&0xFF00)|(uint8_t)((uint8_t)shifts-8);
        if (!(uint8_t)shifts) goto dividePitch;
    }
    do {numerator<<=1;} while (--shifts);
dividePitch: ;
    int16_t projectedPitch;
    for (;;)
    {
        if (!trigonometry[1]) abort(); /* Native zero-divisor retry cannot progress. */
        int64_t quotient=(int64_t)(int32_t)numerator/trigonometry[1];
        if (quotient>=INT16_MIN && quotient<=INT16_MAX) {projectedPitch=(int16_t)quotient;break;}
        numerator=BtechDivideErrorInterrupt(numerator);
    }
    int16_t offsetX=(int16_t)(uint16_t)((uint32_t)((int32_t)(int16_t)rollSin*projectedPitch)>>8);
    int16_t offsetY=(int16_t)(uint16_t)((uint32_t)((int32_t)(int16_t)(uint16_t)(0-rollCos)*projectedPitch)>>8);
    int16_t left=(int16_t)(uint16_t)(screenCentreA-rollCos+offsetX);
    int16_t right=(int16_t)(uint16_t)(screenCentreA+rollCos+offsetX);
    int16_t leftY=(int16_t)(uint16_t)(screenCentreB-((uint16_t)(0-rollSin)+offsetY));
    int16_t rightY=(int16_t)(uint16_t)(screenCentreB-(rollSin+offsetY));
    uint16_t wholeColour;
    if (right<clipMinimumX) {wholeColour=roll>=0x8000?fillA:fillB;goto fillWhole;}
    if (left<clipMinimumX)
    {
        numerator=(uint32_t)((int32_t)(int16_t)(uint16_t)(rightY-leftY)*(int16_t)(uint16_t)(clipMinimumX-left));
        int16_t divisor=(int16_t)(uint16_t)(right-left);
        for (;;)
        {
            if (!divisor) abort();
            int64_t quotient=(int64_t)(int32_t)numerator/divisor;
            if (quotient>=INT16_MIN && quotient<=INT16_MAX) {leftY=(int16_t)(uint16_t)(leftY+quotient);break;}
            numerator=BtechDivideErrorInterrupt(numerator);
        }
        left=clipMinimumX;
    }
    else if (left>clipMaximumX) {wholeColour=roll>=0x8000?fillB:fillA;goto fillWhole;}
    if (right>clipMaximumX)
    {
        numerator=(uint32_t)((int32_t)(int16_t)(uint16_t)(leftY-rightY)*(int16_t)(uint16_t)(clipMaximumX-right));
        int16_t divisor=(int16_t)(uint16_t)(left-right);
        for (;;)
        {
            if (!divisor) abort();
            int64_t quotient=(int64_t)(int32_t)numerator/divisor;
            if (quotient>=INT16_MIN && quotient<=INT16_MAX) {rightY=(int16_t)(uint16_t)(rightY+quotient);break;}
            numerator=BtechDivideErrorInterrupt(numerator);
        }
        right=clipMaximumX;
    }
    if (rightY<clipMinimumY)
    {
        if (leftY<clipMinimumY) {wholeColour=fillB;goto fillWhole;}
        numerator=(uint32_t)((int32_t)(int16_t)(uint16_t)(left-right)*(int16_t)(uint16_t)(clipMinimumY-rightY));
        int16_t divisor=(int16_t)(uint16_t)(leftY-rightY);
        for (;;)
        {
            if (!divisor) abort();
            int64_t quotient=(int64_t)(int32_t)numerator/divisor;
            if (quotient>=INT16_MIN && quotient<=INT16_MAX) {right=(int16_t)(uint16_t)(right+quotient);break;}
            numerator=BtechDivideErrorInterrupt(numerator);
        }
        rightY=clipMinimumY;
        goto clipLeftBottom;
    }
    if (leftY<clipMinimumY)
    {
        numerator=(uint32_t)((int32_t)(int16_t)(uint16_t)(right-left)*(int16_t)(uint16_t)(clipMinimumY-leftY));
        int16_t divisor=(int16_t)(uint16_t)(rightY-leftY);
        for (;;)
        {
            if (!divisor) abort();
            int64_t quotient=(int64_t)(int32_t)numerator/divisor;
            if (quotient>=INT16_MIN && quotient<=INT16_MAX) {left=(int16_t)(uint16_t)(left+quotient);break;}
            numerator=BtechDivideErrorInterrupt(numerator);
        }
        leftY=clipMinimumY;
    }
    if (rightY>clipMaximumY)
    {
        if (leftY>clipMaximumY) {wholeColour=fillA;goto fillWhole;}
        numerator=(uint32_t)((int32_t)(int16_t)(uint16_t)(left-right)*(int16_t)(uint16_t)(clipMaximumY-rightY));
        int16_t divisor=(int16_t)(uint16_t)(leftY-rightY);
        for (;;)
        {
            if (!divisor) abort();
            int64_t quotient=(int64_t)(int32_t)numerator/divisor;
            if (quotient>=INT16_MIN && quotient<=INT16_MAX) {right=(int16_t)(uint16_t)(right+quotient);break;}
            numerator=BtechDivideErrorInterrupt(numerator);
        }
        rightY=clipMaximumY;
        goto fillRegions;
    }
clipLeftBottom:
    if (leftY>clipMaximumY)
    {
        numerator=(uint32_t)((int32_t)(int16_t)(uint16_t)(right-left)*(int16_t)(uint16_t)(clipMaximumY-leftY));
        int16_t divisor=(int16_t)(uint16_t)(rightY-leftY);
        for (;;)
        {
            if (!divisor) abort();
            int64_t quotient=(int64_t)(int32_t)numerator/divisor;
            if (quotient>=INT16_MIN && quotient<=INT16_MAX) {left=(int16_t)(uint16_t)(left+quotient);break;}
            numerator=BtechDivideErrorInterrupt(numerator);
        }
        leftY=clipMaximumY;
    }
fillRegions: ;
    int16_t top=leftY<rightY?leftY:rightY,bottom=leftY>rightY?leftY:rightY;
    if (top!=clipMinimumY)
    {
        drawLineColour=(uint8_t)fillA;drawFillColour=(uint8_t)fillA;screenPolygonNeedsClip=0;
        BtechDrawStyledRectangle((uint16_t)clipMinimumX,(uint16_t)clipMinimumY,
            (uint16_t)(clipMaximumX-clipMinimumX+1),(uint16_t)(top-clipMinimumY+1));
    }
    if (bottom<clipMaximumY)
    {
        drawLineColour=(uint8_t)fillB;drawFillColour=(uint8_t)fillB;screenPolygonNeedsClip=0;
        BtechDrawStyledRectangle((uint16_t)clipMinimumX,(uint16_t)bottom,
            (uint16_t)(clipMaximumX-clipMinimumX+1),(uint16_t)(clipMaximumY-bottom+1));
    }
    if (leftY!=rightY)
    {
        uint16_t leftColour=leftY>rightY?fillA:fillB,rightColour=leftY>rightY?fillB:fillA;
        if (left==right)
        {
            drawLineColour=(uint8_t)leftColour;drawFillColour=(uint8_t)leftColour;
            BtechDrawStyledRectangle((uint16_t)clipMinimumX,(uint16_t)clipMinimumY,
                (uint16_t)(right+1-clipMinimumX),(uint16_t)(clipMaximumY-clipMinimumY+1));
            drawLineColour=(uint8_t)rightColour;drawFillColour=(uint8_t)rightColour;
            BtechDrawStyledRectangle((uint16_t)(right+1),(uint16_t)clipMinimumY,
                (uint16_t)(clipMaximumX-right),(uint16_t)(clipMaximumY-clipMinimumY+1));
        }
        else
        {
            horizonPolygonY[0]=rightY;horizonPolygonY[1]=rightY;
            horizonPolygonY[2]=leftY;horizonPolygonY[3]=leftY;
            horizonPolygonX[1]=right;horizonPolygonX[2]=left;
            horizonPolygonX[0]=clipMinimumX;horizonPolygonX[3]=clipMinimumX;
            drawLineColour=(uint8_t)leftColour;drawFillColour=(uint8_t)leftColour;
            BtechDrawScreenPolygon(4,horizonPolygonX,horizonPolygonY);
            horizonPolygonX[0]=clipMaximumX;horizonPolygonX[3]=clipMaximumX;
            drawLineColour=(uint8_t)rightColour;drawFillColour=(uint8_t)rightColour;
            BtechDrawScreenPolygon(4,horizonPolygonX,horizonPolygonY);
        }
    }
    screenPolygonNeedsClip=1;
    return;
fillWhole:
    drawLineColour=(uint8_t)wholeColour;drawFillColour=(uint8_t)wholeColour;screenPolygonNeedsClip=0;
    BtechDrawStyledRectangle((uint16_t)clipMinimumX,(uint16_t)clipMinimumY,
        (uint16_t)(clipMaximumX-clipMinimumX+1),(uint16_t)(clipMaximumY-clipMinimumY+1));
    screenPolygonNeedsClip=1;
}
