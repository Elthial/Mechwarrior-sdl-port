#include "mechwarrior/port_diagnostics.h"
#include "mechwarrior/combat_render.h"
#include <stdlib.h>

/*0810:D923/F064/F1DA and22B6:65DD/665E. Original normalization,
 * truncation and correction, NOT a host64/32 replacement quotient.
 * CPU divide faults currently terminate this unsupported host path. */
uint32_t BtechDivideSignedDwordNormalized(uint16_t numeratorLow, int16_t numeratorHigh, uint16_t denominatorLow, int16_t denominatorHigh,uint16_t *cxOut)
{
    // MW-NATIVE-ABI: NEAR BP+04..0A, two split signed DWORDs; result DX:AX.
    // SI/DI/BP preserved. This normalization approximation is NOT host signed division.
    uint32_t numerator=((uint32_t)(uint16_t)numeratorHigh<<0x0010)|numeratorLow;
    uint32_t denominator=((uint32_t)(uint16_t)denominatorHigh<<0x0010)|denominatorLow;
    uint8_t quotientNegative=0x00;
    if (numeratorHigh<0x0000)
    {
        quotientNegative=0x01;
        numerator=(uint32_t)(0x00000000-numerator); // NEG high/low, SBB; minimum wraps.
    }
    if (denominatorHigh<0x0000)
    {
        quotientNegative^=0x01;
        denominator=(uint32_t)(0x00000000-denominator);
    }
    uint16_t quotientLow,quotientHigh;
    if ((denominator>>0x0010)!=0x0000)
    {
        if ((denominator>>0x0018)!=0x00) // 660F..6621: drop one low uint8_t from BOTH values.
        {
            denominator>>=0x0008;
            numerator>>=0x0008;
        }
        do // 6623..662D: two SHR/RCR pairs until denominator upper WORD is zero.
        {
            numerator>>=0x0001;
            denominator>>=0x0001;
        } while ((denominator>>0x0010)!=0x0000);
        if (!(uint16_t)denominator || numerator/(uint16_t)denominator>UINT16_MAX) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !(uint16_t)denominator || numerator/(uint16_t)denominator>UINT16_MAX");
        quotientLow=(uint16_t)(numerator/(uint16_t)denominator);
        quotientHigh=0x0000;
    }
    else if ((int16_t)denominator>(int16_t)(numerator>>0x0010))
    {
        // 6636 JLE is SIGNED WORD comparison, not unsigned quotient-fit reasoning.
        if (!(uint16_t)denominator || numerator/(uint16_t)denominator>UINT16_MAX) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !(uint16_t)denominator || numerator/(uint16_t)denominator>UINT16_MAX");
        quotientLow=(uint16_t)(numerator/(uint16_t)denominator);
        quotientHigh=0x0000;
    }
    else
    {
        // 6641..664A: two actual WORD DIVs, high numerator first then remainder:low.
        uint16_t divisor=denominator;
        uint16_t numeratorHigh=numerator>>0x0010;
        if (!divisor) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !divisor");
        quotientHigh=(uint16_t)(numeratorHigh/divisor);
        uint16_t firstRemainder=(uint32_t)numeratorHigh-(uint32_t)quotientHigh*divisor;
        uint32_t remainingNumerator=((uint32_t)firstRemainder<<0x0010)|(uint16_t)numerator;
        if (remainingNumerator/divisor>UINT16_MAX) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: remainingNumerator/divisor>UINT16_MAX");
        quotientLow=(uint16_t)(remainingNumerator/divisor);
    }
    // Each native WORD DIV rejects zero and overflowing quotients before stores.
    uint32_t quotient=((uint32_t)quotientHigh<<0x0010)|quotientLow;
    if (cxOut) *cxOut=quotientHigh; /*664C copies CX to DX BEFORE sign folding. */
    if (quotientNegative!=0x00) quotient=(uint32_t)(0x00000000-quotient);
    return quotient; // NEAR RET 665D. Result sign overflow wraps exactly as native NEG/SBB.
}

uint32_t BtechDivideSignedWideNumerator(int16_t numeratorHighest, uint16_t numeratorUpper, uint16_t numeratorMiddle, uint16_t numeratorLow, uint16_t denominatorLow, int16_t denominatorHigh,uint16_t *cxOut)
{
    // MW-DS: incoming DS=304E; NEAR six WORD arguments at BP+04..0E, result DX:AX.
    // Numerator is four WORDs, high first; denominator is low/high. SI/DI/BP preserved.
    // Native normalizes, discards low numerator bits and corrects via 65DD/D923;
    // do NOT replace this approximation with a host 64/32 quotient.
    uint16_t numeratorWords[0x0004]={numeratorLow,numeratorMiddle,numeratorUpper,(uint16_t)numeratorHighest};
    uint16_t foldedDenominatorLow=denominatorLow;
    uint16_t foldedDenominatorHigh=denominatorHigh;
    if (numeratorHighest<0x0000)
    {
        // 6679..668A NOT/ADD/ADC across four WORDs; 668D..6691 negates denominator too.
        uint16_t carry=0x0001;
        for (uint16_t wordIndex=0x0000; wordIndex<0x0004; ++wordIndex)
        {
            uint32_t sum=(uint32_t)(uint16_t)~numeratorWords[wordIndex]+carry;
            numeratorWords[wordIndex]=(uint16_t)sum;
            carry=(uint16_t)(sum>>0x0010);
        }
        foldedDenominatorHigh=(uint16_t)(0x0000-foldedDenominatorHigh);
        foldedDenominatorLow=(uint16_t)(0x0000-foldedDenominatorLow);
        foldedDenominatorHigh=(uint16_t)(foldedDenominatorHigh-(foldedDenominatorLow!=0x0000));
    }
    if ((numeratorWords[0x0003]|numeratorWords[0x0002])==0x0000 &&
        (uint8_t)(numeratorWords[0x0001]>>0x0008)<0x80)
        return BtechDivideSignedDwordNormalized(numeratorWords[0x0000],(int16_t)numeratorWords[0x0001],
            foldedDenominatorLow,(int16_t)foldedDenominatorHigh,cxOut); // 669A/669E upper zero AND signed CH>=0.

    uint16_t upper=numeratorWords[0x0002],middle=numeratorWords[0x0001],low=numeratorWords[0x0000];
    uint16_t normalizationShift=0x0000; // Native BP becomes a counter, not a frame or pointer.
    if (numeratorWords[0x0003]!=0x0000)
    {
        low=middle;middle=upper;upper=numeratorWords[0x0003];
        normalizationShift=0x0010; // 66B1..66B7: discard lowest WORD, not a rounded conversion.
    }
    if ((upper>>0x0008)!=0x00)
    {
        low=(uint16_t)((low>>0x0008)|(middle<<0x0008));
        middle=(uint16_t)((middle>>0x0008)|(upper<<0x0008));
        upper=(uint16_t)(upper>>0x0008);
        normalizationShift=(uint16_t)(normalizationShift+0x0008);
    }
    do // 66D6..66DD: SHR BL supplies carry for both RCR WORDs, including the final step.
    {
        ++normalizationShift;
        uint16_t upperCarry=(uint16_t)(upper&0x0001),middleCarry=(uint16_t)(middle&0x0001);
        upper=(uint16_t)((uint8_t)upper>>0x0001);
        middle=(uint16_t)((middle>>0x0001)|(upperCarry<<0x000F));
        low=(uint16_t)((low>>0x0001)|(middleCarry<<0x000F));
    } while ((uint8_t)upper!=0x00);
    if ((int16_t)middle<0x0000) // 66DF..66E6: one more shift to keep normalized numerator signed-positive.
    {
        ++normalizationShift;
        uint16_t middleCarry=middle&0x0001;
        middle=(uint16_t)(middle>>0x0001);
        low=(uint16_t)((low>>0x0001)|(middleCarry<<0x000F));
    }
    uint32_t normalizedNumerator=((uint32_t)middle<<0x0010)|low;
    uint32_t foldedDenominator=((uint32_t)foldedDenominatorHigh<<0x0010)|foldedDenominatorLow;
    BtechDwordDivisionResult *divisionResult=BtechDivideSignedDwordWithRemainder(normalizedNumerator,foldedDenominator);
    // Native D923 returns the existing shared result owner, not a quotient.
    uint16_t quotientLow=divisionResult->quotientLow;
    uint16_t quotientHigh=divisionResult->quotientHigh;
    uint16_t remainderLow=divisionResult->remainderLow;
    uint16_t remainderHigh=divisionResult->remainderHigh; // 66F4..66FC: four ordered WORD reads.
    for (uint16_t shifts=normalizationShift; shifts!=0x0000; --shifts)
    {
        uint16_t carry=quotientLow>>0x000F;
        quotientLow=(uint16_t)(quotientLow<<0x0001);
        quotientHigh=(uint16_t)((quotientHigh<<0x0001)|carry); // Modulo DWORD, even for counts >=32.
    }
    uint16_t correctionDenominatorLow=foldedDenominatorLow;
    uint16_t correctionDenominatorHigh=foldedDenominatorHigh;
    uint16_t correctionShifts=normalizationShift;
    for (;;)
    {
        if ((remainderHigh&0x4000)!=0x0000) // 670B TEST DH,40, not a signed negativity test.
        {
            while (correctionShifts!=0x0000)
            {
                uint16_t carry=correctionDenominatorHigh&0x0001;
                correctionDenominatorHigh=(uint16_t)((correctionDenominatorHigh>>0x0001)|(correctionDenominatorHigh&0x8000));
                correctionDenominatorLow=(uint16_t)((correctionDenominatorLow>>0x0001)|(carry<<0x000F));
                --correctionShifts; // 671B SAR BX / 671D RCR BP, preserves signed denominator.
            }
            break;
        }
        uint16_t carry=remainderLow>>0x000F;
        remainderLow=(uint16_t)(remainderLow<<0x0001);
        remainderHigh=(uint16_t)((remainderHigh<<0x0001)|carry);
        if (--correctionShifts==0x0000) break;
    }
    uint32_t correction=BtechDivideSignedDwordNormalized(remainderLow,(int16_t)remainderHigh,
        correctionDenominatorLow,(int16_t)correctionDenominatorHigh,cxOut);
    uint32_t lowSum=(uint32_t)(uint16_t)correction+quotientLow;
    uint16_t resultLow=lowSum;
    uint16_t resultHigh=(correction>>0x0010)+quotientHigh+(lowSum>>0x0010);
    return ((uint32_t)resultHigh<<0x0010)|resultLow; // 672B ADD / 672D ADC; NEAR RET 6732.
}
