#include "mechwarrior/campaign.h"
#include "mechwarrior/ega_campaign.h"
#include "mechwarrior/campaign_handoff.h"
#include <stdlib.h>
#include <string.h>

/* MW_MAIN081B:2233..223E. Original DI near-string becomes a typed reference;
 * source text is already NUL-terminated by the native data/input boundary. */
void MwDrawCampaignTextColourOne(uint16_t x,uint16_t y,const char *text)
{
    MwNativeByteView view={(const uint8_t *)text,0,(uint32_t)strlen(text)+1};
    (void)EgaDrawTransparentText(x,y,AFTER_ACTION_FOREGROUND,&view,0);
}

/* MW_MAIN 081B:1A97..1ACD. Native name width uses BYTE(length)*6; unsigned
 * WORD subtraction centers long names by wrapping, not a new layout clamp. */
void MwDrawPlanetNameCentered(uint16_t panelX, uint16_t y, uint16_t color, uint16_t planetIndex)
{
    if (planetIndex >= CAMPAIGN_PLANET_COUNT) abort(); /* Unmapped adjacent DS, not a game validation. */
    uint16_t recordOffset = planetRecordOffsets[planetIndex];
    if (recordOffset < PLANET_RECORDS_START || recordOffset >= PLANET_RECORDS_END) abort();
    uint16_t nameOffset = (uint16_t)(recordOffset - PLANET_RECORDS_START + PLANET_NAME_HEADER_BYTES);
    uint16_t nameCursor = nameOffset, nameLength = 0;
    while (planetRecordStorage[nameCursor] != 0)
    {
        nameCursor++; nameLength++;
        if (nameCursor >= sizeof(planetRecordStorage)) abort();
    }
    uint16_t nameWidth = (uint16_t)((uint8_t)nameLength * CAMPAIGN_GLYPH_ADVANCE);
    uint16_t textX = (uint16_t)(panelX + ((uint16_t)(PLANET_NAME_PANEL_WIDTH - nameWidth) >> 1));
    MwNativeByteView text = { planetRecordStorage, 0, sizeof(planetRecordStorage) };
    EgaDrawTransparentText(textX, y, (uint8_t)color, &text, nameOffset);
}
/* MW_MAIN 081B:1A36..1A96. Month-name width then 30-pixel year field; ordinal08
 * followed by the real signed numeric formatter, not printf() text. */
void MwDrawCampaignCalendar(uint16_t panelX, uint8_t yearDigits, uint8_t background,
    uint8_t foreground, uint16_t y)
{
    calendarTextX = panelX; calendarTextY = y;
    calendarBackground = background; calendarForeground = foreground;
    if (calendarMonthIndex >= CAMPAIGN_MONTH_COUNT) abort();
    uint16_t monthWidth = calendarMonthNameWidths[calendarMonthIndex];
    calendarTextX += (uint16_t)(PLANET_NAME_PANEL_WIDTH - (uint16_t)(monthWidth + CALENDAR_YEAR_FIELD_WIDTH)) >> 1;
    const uint8_t *monthName = calendarMonthNames[calendarMonthIndex];
    if (monthName == NULL) abort();
    MwNativeByteView text = { monthName, 0, NATIVE_SEGMENT_BYTE_COUNT };
    EgaDrawTransparentText(calendarTextX, calendarTextY, calendarForeground, &text, 0);
    MwDrawSignedDecimalField(calendarYear,
        (uint16_t)(monthWidth + calendarTextX + CALENDAR_YEAR_GAP), 1, yearDigits, calendarForeground,
        calendarBackground, calendarTextY);
}

/* MW_MAIN 081B:70C2..710E. Unsigned negation preserves INT32_MIN bit pattern.
 * CH/SI return residues have no source-level result in this converted chain. */
void MwDrawSignedDecimalField(uint32_t signedValue, uint16_t x, uint8_t suppressZeros,
    uint8_t fieldDigits, uint8_t foreground, uint8_t background, uint16_t y)
{
    numericNegativeSign = 0;
    if (signedValue & UINT32_C(0x80000000)) { signedValue = UINT32_C(0) - signedValue; numericNegativeSign = 1; }
    numericCursorX = x; numericCursorY = y;
    numericForegroundColour = foreground; numericBackgroundColour = background;
    numericSuppressLeadingZeros = suppressZeros; numericFieldDigitCount = fieldDigits;
    MwBuildDecimalDigitBuffer(signedValue);
    if (numericSuppressLeadingZeros != 0) MwDrawNumericDigitsUnpadded();
    else MwDrawNumericDigitsPadded();
}
/* MW_MAIN 081B:710F..7195. Keep six native subtract-place calls then four DIVs. */
void MwBuildDecimalDigitBuffer(uint32_t magnitude)
{
    uint16_t digit;
    magnitude = MwExtractDecimalDoubleWordPlace(magnitude, DECIMAL_BILLION, &digit);
    numericDecimalDigits[0] = (uint8_t)digit;
    magnitude = MwExtractDecimalDoubleWordPlace(magnitude, DECIMAL_HUNDRED_MILLION, &digit);
    numericDecimalDigits[1] = (uint8_t)digit;
    magnitude = MwExtractDecimalDoubleWordPlace(magnitude, DECIMAL_TEN_MILLION, &digit);
    numericDecimalDigits[2] = (uint8_t)digit;
    magnitude = MwExtractDecimalDoubleWordPlace(magnitude, DECIMAL_MILLION, &digit);
    numericDecimalDigits[3] = (uint8_t)digit;
    magnitude = MwExtractDecimalDoubleWordPlace(magnitude, DECIMAL_HUNDRED_THOUSAND, &digit);
    numericDecimalDigits[4] = (uint8_t)digit;
    magnitude = MwExtractDecimalDoubleWordPlace(magnitude, DECIMAL_TEN_THOUSAND, &digit);
    numericDecimalDigits[5] = (uint8_t)digit;
    uint16_t remainder = (uint16_t)magnitude;
    numericDecimalDigits[6] = (uint8_t)MwDivideDecimalWordPlace(remainder, DECIMAL_THOUSAND, &remainder);
    numericDecimalDigits[7] = (uint8_t)MwDivideDecimalWordPlace(remainder, DECIMAL_HUNDRED, &remainder);
    numericDecimalDigits[8] = (uint8_t)MwDivideDecimalWordPlace(remainder, DECIMAL_TEN, &remainder);
    numericDecimalDigits[9] = (uint8_t)MwDivideDecimalWordPlace(remainder, 1, &remainder);
}
/* MW_MAIN 081B:7196..719D. CWD then UNSIGNED DIV, not normal value/place. */
uint16_t MwDivideDecimalWordPlace(uint16_t value, uint16_t place, uint16_t *remainder)
{
    uint32_t dividend = (value & NATIVE_WORD_HIGH_BIT) ? UINT32_C(0xFFFF0000) | value : value;
    if (place == 0 || dividend / place > UINT16_MAX) abort(); /* Unbound native INT0 boundary. */
    *remainder = (uint16_t)(dividend % place);
    return (uint16_t)(dividend / place);
}
/* MW_MAIN 081B:719E..71B8. Save last unborrowed value; zero place never terminates. */
uint32_t MwExtractDecimalDoubleWordPlace(uint32_t value, uint32_t place, uint16_t *digitCount)
{
    *digitCount = 0;
    for (;;)
    {
        numericRemainderHigh = (uint16_t)(value >> NATIVE_WORD_BITS);
        numericRemainderLow = (uint16_t)value;
        if (value < place) return value;
        value -= place; (*digitCount)++;
    }
}
/* MW_MAIN 081B:71B9..71BE / 71BF..71C5: native shared-tail wrappers. */
void MwDrawNumericDigitsUnpadded(void) { MwDrawFormattedNumericGlyphs(0); }
void MwDrawNumericDigitsPadded(void)
{ MwDrawFormattedNumericGlyphs((uint16_t)(DECIMAL_DIGIT_COUNT - numericFieldDigitCount)); }
/* MW_MAIN 081B:71C6..7258. Ordinal06 always paints its background; separator
 * spaces/commas and flag cleanup follow the original order, not a host formatter. */
void MwDrawFormattedNumericGlyphs(uint16_t digitIndex)
{
    uint16_t cursorX = numericCursorX, cursorY = numericCursorY;
    numericLeadingDigitSeen = 0;
    do
    {
        if (digitIndex >= DECIMAL_DIGIT_COUNT) abort(); /* Unmapped adjacent DS, not clamp. */
        uint8_t character = numericDecimalDigits[digitIndex];
        if (numericLeadingDigitSeen == 0)
        {
            if (character == 0 && digitIndex != DECIMAL_LAST_DIGIT)
            {
                if (numericSuppressLeadingZeros != 0) goto next;
                character = ASCII_SPACE; goto draw;
            }
            if (character != 0)
            {
                numericLeadingDigitSeen = 1;
                if (numericNegativeSign == 1)
                    cursorX = EgaDrawCharacter(cursorX, &cursorY, ASCII_MINUS,
                        numericForegroundColour, numericBackgroundColour);
            }
        }
        character += ASCII_DIGIT_ZERO;
draw:
        cursorX = EgaDrawCharacter(cursorX, &cursorY, character, numericForegroundColour, numericBackgroundColour);
        if (numericGroupingEnabled != 0 && (digitIndex == 0 || digitIndex == 3 || digitIndex == 6))
            cursorX = EgaDrawCharacter(cursorX, &cursorY, character == ASCII_SPACE ? ASCII_SPACE : ASCII_COMMA,
                numericForegroundColour, numericBackgroundColour);
next:
        digitIndex++;
    } while (digitIndex != DECIMAL_DIGIT_COUNT);
    numericSuppressLeadingZeros = 0; numericGroupingEnabled = 0;
}
