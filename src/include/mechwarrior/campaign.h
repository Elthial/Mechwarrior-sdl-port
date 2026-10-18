#ifndef MECHWARRIOR_CAMPAIGN_H
#define MECHWARRIOR_CAMPAIGN_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum
{
    PICTURE_RLE_ESCAPE = 0x0F,
    PICTURE_NIBBLE_MASK = 0x0F,
    PICTURE_HIGH_NIBBLE_MASK = 0xF0,
    PICTURE_NIBBLE_SHIFT = 0x04,
    RANDOM_WORD1_SHIFT = 0x02,
    RANDOM_WORD0_ROTATE = 0x05,
    RANDOM_CARRY_INSERT_SHIFT = 0x04,
    RANDOM_ROTATE_REINSERT_SHIFT = 0x0C,
    NATIVE_WORD_HIGH_BIT = 0x8000,
    NATIVE_WORD_BITS = 0x10,
    NATIVE_WORD_HIGH_BYTE_MASK = 0xFF00
};

enum { INDEXED_PICTURE_SLOTS = 0x0064, PICTURE_OFFSET_ENTRY_BYTES = 0x04,
    INDEXED_PICTURE_SIZE_TABLE = 0x0190, INDEXED_PICTURE_DIRECTORY_BYTES = 0x0258,
    GENERAL_PICTURE_READ_BYTES = 0x8000, WHOLE_PICTURE_READ_BYTES = 0xFFFF,
    PICTURE_BANK_LOW_OFFSET_WORD = 0x0002, PLANET_NAME_PANEL_WIDTH = 0x0066,
    PLANET_NAME_HEADER_BYTES = 0x000A, CAMPAIGN_GLYPH_ADVANCE = 0x0006,
    CALENDAR_YEAR_FIELD_WIDTH = 0x001E, CALENDAR_YEAR_GAP = 0x0004 };

extern uint16_t indexedPictureOutputSelector, generalPictureBankSelector;
extern uint16_t cockpitPictureBankSelector, activePictureBankSelector;
extern uint16_t secondaryPictureBankSelector;
extern uint8_t indexedPictureDirectoryLoaded;
extern uint16_t indexedPictureCachedIndex, indexedPictureStoredBytes;
extern uint8_t indexedPictureDirectory[INDEXED_PICTURE_DIRECTORY_BYTES];
extern uint16_t indexedPictureFileHandle, generalPictureFileHandle;
extern uint16_t selectedPictureFileHandle;
extern const char indexedPictureFilename[], generalPictureFilename[];
extern const char cockpitPictureFilename[], primaryPictureFilename[];
extern const char secondaryPictureFilename[], tPictureFilename[];
extern const char alternatePictureFilename[], fPictureFilename[];
extern const char *selectedPictureFilename;

bool MwDosOpenResourceReadWrite(const char *filename, uint16_t *handleOrError);
bool MwDosReadFile(uint8_t *buffer, uint16_t byteCount, uint16_t fileHandle, uint16_t *bytesReadOrError);
bool MwDosCloseFile(uint16_t fileHandle);
bool MwDosSeekAbsolute(uint32_t fileOffset, uint16_t fileHandle);
uint16_t MwLoadIndexedPicture(uint16_t pictureIndex);
void MwIndexedPictureCloseHook(void);
void MwCloseIndexedPictureBank(void);
bool MwLoadGeneralPictureBank(void);
bool MwLoadCockpitPictureBank(void);
bool MwLoadPrimaryPictureBank(void);
bool MwSelectAndLoadPictureBank(const char *filename);
bool MwLoadSelectedPictureBank(void);
bool MwLoadSecondaryPictureBank(void);
bool MwLoadTPictureBank(void);
bool MwLoadAlternatePictureBank(void);
bool MwLoadFPictureBank(void);
void MwPromptChangeCurrentDriveDisk(void);
void MwPromptInsertDiskOne(void);
void MwDrawIndexedPicture(uint16_t x, uint16_t y, uint16_t pictureIndex);
uint16_t MwDrawGeneralPicture(uint16_t x, uint16_t y, uint8_t page, uint16_t pictureIndex);
uint16_t MwDrawAlternateBankPicture(uint16_t x, uint16_t y, uint8_t page, uint16_t pictureIndex);
uint16_t MwDrawActivePicture(uint16_t x, uint16_t y, uint8_t page, uint16_t pictureIndex);
uint16_t MwDrawFourthBankPicture(uint16_t x, uint16_t y, uint8_t page, uint16_t pictureIndex);

enum { CAMPAIGN_PLANET_COUNT = 0x0092, PLANET_RECORDS_START = 0x67B6,
    PLANET_RECORDS_END = 0x8A16, CAMPAIGN_MONTH_COUNT = 0x000C, DECIMAL_DIGIT_COUNT = 0x000A,
    DECIMAL_LAST_DIGIT = 0x0009, ASCII_DIGIT_ZERO = 0x30, ASCII_SPACE = 0x20,
    ASCII_MINUS = 0x2D, ASCII_COMMA = 0x2C,
    DECIMAL_BILLION = 0x3B9ACA00, DECIMAL_HUNDRED_MILLION = 0x05F5E100,
    DECIMAL_TEN_MILLION = 0x00989680, DECIMAL_MILLION = 0x000F4240,
    DECIMAL_HUNDRED_THOUSAND = 0x000186A0, DECIMAL_TEN_THOUSAND = 0x00002710,
    DECIMAL_THOUSAND = 0x03E8, DECIMAL_HUNDRED = 0x0064, DECIMAL_TEN = 0x000A };
extern uint16_t planetRecordOffsets[CAMPAIGN_PLANET_COUNT];
extern uint8_t planetRecordStorage[PLANET_RECORDS_END - PLANET_RECORDS_START];
extern const uint8_t *calendarMonthNames[CAMPAIGN_MONTH_COUNT];
extern uint16_t calendarMonthNameWidths[CAMPAIGN_MONTH_COUNT];
extern uint16_t calendarMonthIndex, calendarYear;
extern uint16_t calendarTextX, calendarTextY;
extern uint8_t calendarBackground, calendarForeground;
extern uint8_t numericLeadingDigitSeen, numericGroupingEnabled;
extern uint8_t numericDecimalDigits[DECIMAL_DIGIT_COUNT];
extern uint16_t numericRemainderHigh, numericRemainderLow;
extern uint8_t numericSuppressLeadingZeros, numericNegativeSign;
extern uint16_t numericFieldDigitCount, numericCursorX, numericCursorY;
extern uint8_t numericBackgroundColour, numericForegroundColour;

enum { CAMPAIGN_HOUSE_COUNT = 0x0005, PLANET_LANDSCAPE_COUNT = 0x0003,
    PLANET_MARKET_PREMIUM_STEP = 0x0012, PLANET_PREMIUM_RANDOM_MAX = 0x0007,
    PLANET_NEGOTIATION_RANDOM_MAX = 0x0002, PERCENT_DIVISOR = 0x0064,
    AMMUNITION_PRICE_COUNT = 0x000C, REPAIR_ATTEMPT_KIND_COUNT = 0x000A,
    REPAIR_ATTEMPT_SLOTS_PER_KIND = 0x0016 };

/* Serialized, variable-length planet records: this prefix is followed by two
 * NUL-terminated strings. Character fields allow odd native record offsets;
 * population is a little-endian WORD, not an aligned host WORD member. */
typedef struct MwPlanetHeader
{
    unsigned char planetNumber;
    unsigned char houseIndex;
    unsigned char landscapeAndMarketCode;
    unsigned char maximumMissionType;
    unsigned char facilityDisplayFlag;
    unsigned char mapPositionX;
    unsigned char mapPositionY;
    unsigned char travelTimeWeight;
    unsigned char populationMillions[0x0002];
} MwPlanetHeader;
_Static_assert(sizeof(MwPlanetHeader) == PLANET_NAME_HEADER_BYTES, "Native planet prefix");
_Static_assert(offsetof(MwPlanetHeader, populationMillions) == 0x0008, "Native population offset");
_Static_assert(_Alignof(MwPlanetHeader) == 1, "Unaligned planet records");

typedef struct CampaignRepairAttemptRow
{
    uint8_t failedByDamageIndex[REPAIR_ATTEMPT_SLOTS_PER_KIND];
} CampaignRepairAttemptRow;
_Static_assert(sizeof(CampaignRepairAttemptRow) == 0x0016, "Native repair-attempt row");

extern uint16_t currentPlanetIndex, currentPlanetLandscapeIndex;
extern uint16_t currentPlanetHouseIndex, currentPlanetMarketTier;
extern uint16_t currentPlanetFacilityDisplayFlag;
extern uint16_t currentPlanetMapPositionX, currentPlanetMapPositionY;
extern uint16_t planetMarketPremiumPercent, contractNegotiationAttemptLimit;
extern uint8_t houseNegotiationAttemptBase[CAMPAIGN_HOUSE_COUNT];
/* Six purchase prices followed by six sale prices, not twelve weapon prices. */
extern uint16_t ammunitionBasePrices[AMMUNITION_PRICE_COUNT];
extern uint16_t ammunitionTransactionPrices[AMMUNITION_PRICE_COUNT];
extern CampaignRepairAttemptRow repairAttemptFailuresByKind[REPAIR_ATTEMPT_KIND_COUNT];
void MwApplyCurrentPlanetState(void);
void MwResetRepairAttemptFailures(void);
void MwDrawPlanetNameCentered(uint16_t panelX, uint16_t y, uint16_t color, uint16_t planetIndex);
void MwDrawCampaignCalendar(uint16_t panelX, uint8_t yearDigits, uint8_t background,
    uint8_t foreground, uint16_t y);
void MwDrawSignedDecimalField(uint32_t signedValue, uint16_t x, uint8_t suppressZeros,
    uint8_t fieldDigits, uint8_t foreground, uint8_t background, uint16_t y);
void MwBuildDecimalDigitBuffer(uint32_t magnitude);
uint16_t MwDivideDecimalWordPlace(uint16_t value, uint16_t place, uint16_t *remainder);
uint32_t MwExtractDecimalDoubleWordPlace(uint32_t value, uint32_t place, uint16_t *digitCount);
void MwDrawNumericDigitsUnpadded(void);
void MwDrawNumericDigitsPadded(void);
void MwDrawFormattedNumericGlyphs(uint16_t digitIndex);

/* Canonical owner: src/Original/Memory/MW_MAIN_1090.c. Selectors remain
 * native WORD values; offsets wrap as WORDs, not host pointer arithmetic.
 * This is only the implemented slice of 1090, not its serialized layout. */
extern uint16_t compressedPictureBufferSelector;
extern uint16_t randomStateWord0;
extern uint16_t randomStateWord1;
extern uint16_t pictureCompressedByteCount;
extern uint16_t pictureOutputSelector;
extern uint8_t pictureReadHighNibbleNext;
extern uint16_t pictureReadByteOffset;
extern uint8_t pictureWriteLowNibbleNext;
extern uint16_t pictureWriteByteOffset;

uint16_t MwRandomUpTo(uint16_t maximum);
/* Original AX=count, BX=output selector. Reads low nibble at source[count]
 * before rejecting its high nibble. Supply complete live segment windows;
 * do not zero a made-up EOF sentinel. */
void MwDecodePictureBank(uint16_t compressedByteCount, uint16_t outputSelector);
bool MwReadPictureNibble(uint8_t *nibble);
void MwWritePictureNibble(uint8_t nibble);

#endif
