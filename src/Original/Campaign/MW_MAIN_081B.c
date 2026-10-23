#include "mechwarrior/campaign.h"
#include "mechwarrior/native_address.h"

/* Original MW_MAIN 081B:0C17..0C3D. Used by stock, contracts, recruits and
 * planet selection: CLC; RCR AX twice; RCL BX five; XOR; high WORD of MUL
 * by wrapping (maximum+1). Carry participates in both rotations. */
uint16_t MwRandomUpTo(uint16_t maximum)
{
    uint16_t rotatedWord1 = (randomStateWord1 >> RANDOM_WORD1_SHIFT)
        | ((randomStateWord1 & 1) != 0 ? NATIVE_WORD_HIGH_BIT : 0);
    uint16_t carryIntoWord0 = (randomStateWord1 >> 1) & 1;
    randomStateWord0 = (uint16_t)(((uint32_t)randomStateWord0 << RANDOM_WORD0_ROTATE)
        | (carryIntoWord0 << RANDOM_CARRY_INSERT_SHIFT)
        | (randomStateWord0 >> RANDOM_ROTATE_REINSERT_SHIFT));

    randomStateWord1 = rotatedWord1 ^ randomStateWord0;
    uint16_t rangeSize = maximum + 1;
    return (uint16_t)(((uint32_t)randomStateWord1 * rangeSize) >> NATIVE_WORD_BITS);
}

/* Original MW_MAIN 081B:12B8..12FD. Whole-bank and indexed-picture callers
 * share this decoder. No size clamp, output clear or bounded replacement RLE:
 * native cursor wrap and partial work remain. */
void MwDecodePictureBank(uint16_t compressedByteCount, uint16_t outputSelector)
{
    pictureCompressedByteCount = compressedByteCount;
    pictureOutputSelector = outputSelector;
    pictureReadHighNibbleNext = 0;
    pictureReadByteOffset = 0;
    pictureWriteLowNibbleNext = 0;
    pictureWriteByteOffset = 0;

    uint8_t token;
    while (!MwReadPictureNibble(&token))
    {
        if (token != PICTURE_RLE_ESCAPE)
        {
            MwWritePictureNibble(token);
            continue;
        }
        uint8_t repeatCountMinusOne;
        uint8_t repeatedNibble;
        if (MwReadPictureNibble(&repeatCountMinusOne))
            return;
        uint16_t repeatCount = repeatCountMinusOne + 1;
        if (MwReadPictureNibble(&repeatedNibble))
            return;
        do
        {
            MwWritePictureNibble(repeatedNibble);
            --repeatCount;
        } while (repeatCount != 0);
    }
}

/* Original MW_MAIN 081B:12FE..1338. Carry result becomes bool; AL is written
 * even at EOF. The byte read precedes the check and the WORD cursor wraps. */
bool MwReadPictureNibble(uint8_t *nibble)
{
    uint8_t *compressedData = MwResolveNativeSegment(compressedPictureBufferSelector);
    uint8_t packedNibbles = compressedData[pictureReadByteOffset];
    if (pictureReadHighNibbleNext == 0)
    {
        *nibble = packedNibbles & PICTURE_NIBBLE_MASK;
        pictureReadHighNibbleNext = 1;
        return false;
    }
    *nibble = packedNibbles >> PICTURE_NIBBLE_SHIFT;
    pictureReadHighNibbleNext = 0;
    pictureReadByteOffset++;
    return pictureReadByteOffset > pictureCompressedByteCount;
}

/* Original MW_MAIN 081B:1339..136B. First write replaces the entire byte;
 * second ORs the low nibble. An unfinished byte is high-only, not stale RAM. */
void MwWritePictureNibble(uint8_t nibble)
{
    uint8_t *outputData = MwResolveNativeSegment(pictureOutputSelector);
    if (pictureWriteLowNibbleNext == 0)
    {
        outputData[pictureWriteByteOffset] = (uint8_t)((nibble << PICTURE_NIBBLE_SHIFT)
            & PICTURE_HIGH_NIBBLE_MASK);
        pictureWriteLowNibbleNext = 1;
    }
    else
    {
        outputData[pictureWriteByteOffset] |= nibble & PICTURE_NIBBLE_MASK;
        pictureWriteByteOffset++;
        pictureWriteLowNibbleNext = 0;
    }
}
