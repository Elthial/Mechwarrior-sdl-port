#include "mechwarrior/port_diagnostics.h"
#include "mechwarrior/combat_render.h"
#include <stdlib.h>

/*0810:D923/F064/F1DA and22B6:65DD/665E. Original normalization,
 * truncation and correction, NOT a host64/32 replacement quotient.
 * CPU divide faults currently terminate this unsupported host path. */
BtechDwordDivisionResult *BtechDivideSignedDwordWithRemainder(uint32_t numerator, uint32_t denominator)
{
    // MW-DS: incoming DS=304E; FAR BP+06/+08 numerator, BP+0A/+0C denominator.
    // BP saved, BX/CX clobbered while copying four WORD arguments to NEAR F064.
    // F064 normalizes only these copied arguments; original FAR arguments survive
    // unless they alias the shared result record (not a disjoint-storage promise).
    return BtechDivideSignedDwordStoreQuotientRemainder((uint16_t)numerator,
        (uint16_t)(numerator>>0x0010),(uint16_t)denominator,
        (uint16_t)(denominator>>0x0010));
    // Native AX=4BA0, DX=DS: the typed owner replaces only this address transport.
}

BtechDwordDivisionResult *BtechDivideSignedDwordStoreQuotientRemainder(uint16_t numeratorLow, uint16_t numeratorHigh, uint16_t denominatorLow, uint16_t denominatorHigh)
{
    // MW-DS: incoming DS=304E. NEAR BP+04/+06 numerator, BP+08/+0A denominator.
    // Native locals normalize by WORD operations; shared result stores remain ordered.
    uint16_t resultSignBits=0x0000; // bit0 remainder negative, bit1 quotient negative.
    if ((int16_t)numeratorHigh<0x0000)
    {
        resultSignBits=0x0003;
        numeratorHigh=(uint16_t)(0x0000-numeratorHigh);
        numeratorLow=(uint16_t)(0x0000-numeratorLow);
        numeratorHigh=(uint16_t)(numeratorHigh-(numeratorLow!=0x0000));
    }
    if ((int16_t)denominatorHigh<0x0000)
    {
        resultSignBits^=0x0002;
        denominatorHigh=(uint16_t)(0x0000-denominatorHigh);
        denominatorLow=(uint16_t)(0x0000-denominatorLow);
        denominatorHigh=(uint16_t)(denominatorHigh-(denominatorLow!=0x0000));
    } // Minimum signed DWORD stays 80000000 under WORD negation.
    uint32_t numerator=((uint32_t)numeratorHigh<<0x0010)|numeratorLow;
    uint32_t denominator=((uint32_t)denominatorHigh<<0x0010)|denominatorLow;
    uint16_t quotientLow,quotientHigh;
    uint32_t remainder;
    if (denominatorHigh==0x0000)
    {
        // F0AC/F0B3: two unsigned WORD DIVs; zero divisor faults before result stores.
        if (!denominatorLow) /* Port emulation of a native CPU fault. */ MW_NATIVE_FAULT("Rejected condition: !denominatorLow");
        quotientHigh=(uint16_t)(numeratorHigh/denominatorLow);
        uint16_t highRemainder=(uint32_t)numeratorHigh-(uint32_t)quotientHigh*denominatorLow;
        uint32_t remainingNumerator=((uint32_t)highRemainder<<0x0010)|numeratorLow;
        if (remainingNumerator/denominatorLow>UINT16_MAX) /* Port emulation of a native CPU fault. */ MW_NATIVE_FAULT("Rejected condition: remainingNumerator/denominatorLow>UINT16_MAX");
        quotientLow=(uint16_t)(remainingNumerator/denominatorLow);
        remainder=(uint32_t)(uint16_t)(remainingNumerator-(uint32_t)quotientLow*denominatorLow);
    }
    else
    {
        uint32_t shiftedNumerator=numerator,shiftedDenominator=denominator;
        do // F0CB..F0D5 SHR/RCR both DWORDs until divisor fits one WORD.
        {
            shiftedDenominator>>=0x0001;
            shiftedNumerator>>=0x0001;
        } while ((shiftedDenominator>>0x0010)!=0x0000);
        if (!(uint16_t)shiftedDenominator || shiftedNumerator/(uint16_t)shiftedDenominator>UINT16_MAX) /* Port emulation of a native CPU fault. */ MW_NATIVE_FAULT("Rejected condition: !(uint16_t)shiftedDenominator || shiftedNumerator/(uint16_t)shiftedDenominator>UINT16_MAX");
        uint16_t quotientCandidate=shiftedNumerator/(uint16_t)shiftedDenominator;
        // Actual F1DA NEAR helper multiplies modulo DWORD and RET0008 cleans its args.
        uint32_t product=BtechMultiplyDwordLow(denominatorLow,denominatorHigh,quotientCandidate,0x0000);
        if ((uint16_t)(product>>0x0010)>numeratorHigh ||
            ((uint16_t)(product>>0x0010)==numeratorHigh && (uint16_t)product>numeratorLow))
        {
            --quotientCandidate;
            product=(uint32_t)(product-denominator); // F0F4..F0F8 single correction.
        }
        remainder=(uint32_t)(product-numerator); // Native reversed subtraction.
        resultSignBits^=0x0001; // F101 reverses its sign below, not a negative-input test.
        quotientHigh=0x0000;
        quotientLow=quotientCandidate;
    }
    if ((resultSignBits&0x0001)!=0x0000) remainder=(uint32_t)(0x00000000-remainder);
    uint32_t quotient=((uint32_t)quotientHigh<<0x0010)|quotientLow;
    if ((resultSignBits&0x0002)!=0x0000) quotient=(uint32_t)(0x00000000-quotient);
    // F122..F12D: four ordered WORD stores, not an atomic aggregate assignment.
    signedDivisionResult.remainderHigh=(uint16_t)(remainder>>0x0010);
    signedDivisionResult.remainderLow=(uint16_t)remainder;
    signedDivisionResult.quotientHigh=(uint16_t)(quotient>>0x0010);
    signedDivisionResult.quotientLow=(uint16_t)quotient;
    return &signedDivisionResult; // AX result offset, DX original DS; NEAR RET, caller cleans four WORDs.
}

uint32_t BtechMultiplyDwordLow(uint16_t leftLow, uint16_t leftHigh, uint16_t rightLow, uint16_t rightHigh)
{
    // NEAR BP+04/+06 left DWORD, BP+08/+0A right DWORD; result DX:AX.
    // BP/SI/DI preserved, BX=right LOW; RET0008 consumes all four WORD arguments.
    // Native cross terms contribute only their LOW WORD; no signed saturation.
    if ((leftHigh|rightHigh)!=0x0000)
    {
        uint16_t crossTerms=(uint32_t)leftHigh*rightLow;
        crossTerms=(uint16_t)(crossTerms+(uint16_t)((uint32_t)leftLow*rightHigh));
        uint32_t lowProduct=(uint32_t)leftLow*rightLow;
        uint16_t resultHigh=(lowProduct>>0x0010)+crossTerms;
        return ((uint32_t)resultHigh<<0x0010)|(uint16_t)lowProduct;
    }
    // F1EA fast path skips cross products and leaves CX unchanged.
    return (uint32_t)leftLow*rightLow;
}

/*0810:F13E. Standalone signed quotient; unlike F064 this does NOT publish
 * the shared4BA0 result. Preserve its unsigned normalization/carry correction. */
uint32_t BtechDivideSignedDword(uint32_t numerator,uint32_t denominator)
{
    uint16_t negativeOperands=0;
    if ((int32_t)numerator<0) {++negativeOperands;numerator=0-numerator;}
    if ((int32_t)denominator<0) {++negativeOperands;denominator=0-denominator;}
    uint32_t quotient;
    if (!(denominator>>16))
    {
        uint16_t divisor=denominator;
        if (!divisor) /* Port emulation of a native CPU fault. */ MW_NATIVE_FAULT("Rejected condition: !divisor");
        uint16_t upper=(numerator>>16)/divisor;
        uint16_t remainder=(numerator>>16)%divisor;
        uint32_t lowerDividend=((uint32_t)remainder<<16)|(uint16_t)numerator;
        quotient=((uint32_t)upper<<16)|(uint16_t)(lowerDividend/divisor);
    }
    else
    {
        uint32_t scaledNumerator=numerator,scaledDenominator=denominator;
        do {scaledNumerator>>=1;scaledDenominator>>=1;} while (scaledDenominator>>16);
        uint32_t candidate=scaledNumerator/(uint16_t)scaledDenominator;
        if (candidate>UINT16_MAX) /* Port emulation of a native CPU fault. */ MW_NATIVE_FAULT("Rejected condition: candidate>UINT16_MAX");
        uint32_t lowProduct=(uint32_t)(uint16_t)denominator*candidate;
        uint32_t highSum=(lowProduct>>16)+(uint16_t)((denominator>>16)*candidate);
        if (highSum>UINT16_MAX || (uint16_t)highSum>(uint16_t)(numerator>>16)
            || ((uint16_t)highSum==(uint16_t)(numerator>>16) && (uint16_t)lowProduct>(uint16_t)numerator)) --candidate;
        quotient=candidate;
    }
    return negativeOperands==1?0-quotient:quotient;
}

/*0810:F20E. Register entry: CL is the full BYTE count, not count&31. */
uint32_t BtechShiftDwordLeft(uint32_t value,uint8_t count)
{
    while (count--) value<<=1;
    return value;
}

/*0810:F21A. SAR DX/RCR AX, repeated for the full incoming CL count. */
uint32_t BtechShiftSignedDwordRight(uint32_t value,uint8_t count)
{
    while (count--) value=(value>>1)|(value&UINT32_C(0x80000000));
    return value;
}

/*0810:D91A. Original NEAR two-WORD signed multiply, DX:AX result. */
int32_t BtechMultiplySignedWords(int16_t left,int16_t right)
{
    return (int32_t)left*right;
}
