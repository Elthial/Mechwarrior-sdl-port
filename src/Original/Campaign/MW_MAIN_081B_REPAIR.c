#include "mechwarrior/campaign_mechanics.h"
#include <stdlib.h>

/* MW_MAIN 081B:32EB..3313. DI is an observable register result: the salvage
 * caller uses it as an accumulator, which this native procedure overwrites. */
uint32_t MwCalculateUnitConditionPrice(uint8_t condition, uint16_t unitKind, uint16_t *marketTier)
{
    *marketTier = currentPlanetMarketTier;
    if (*marketTier >= UNIT_PRICE_TIER_COUNT) abort(); /* Unmapped adjacent DS. */
    uint8_t priceMultiplier = unitPriceTierMultipliers[*marketTier];
    priceMultiplier--;
    uint16_t conditionFactor = priceMultiplier * condition;
    uint16_t kindByteOffset = unitKind * sizeof(uint16_t);
    if (kindByteOffset >= sizeof(unitBasePrices)) abort();
    uint32_t price = (uint32_t)conditionFactor * unitBasePrices[kindByteOffset / sizeof(uint16_t)];
    price += UNIT_SALE_ROUNDING_CBILLS;
    price /= CBILLS_PER_THOUSAND;
    if (price > UINT16_MAX) abort(); /* Native DIV fault precedes both price stores. */
    price *= CBILLS_PER_THOUSAND;
    unitTransactionPriceLow = (uint16_t)price;
    unitTransactionPriceHigh = (uint16_t)(price >> NATIVE_WORD_BITS);
    return price;
}

/* MW_MAIN 081B:4645..4673. Purchase floors; sale above rounds with +01F4.
 * Repair discount subtraction wraps as a DWORD, with no zero-price clamp. */
void MwCalculateMarketPurchasePrice(void)
{
    if (currentPlanetMarketTier >= UNIT_PRICE_TIER_COUNT) abort();
    uint16_t priceFactor = unitPriceTierMultipliers[currentPlanetMarketTier] * PERCENT_DIVISOR;
    uint16_t kindByteOffset = selectedMarketUnitKind * sizeof(uint16_t);
    if (kindByteOffset >= sizeof(unitBasePrices)) abort();
    uint32_t price = (uint32_t)priceFactor * unitBasePrices[kindByteOffset / sizeof(uint16_t)];
    price -= ((uint32_t)unitRepairQuoteHigh << NATIVE_WORD_BITS) | unitRepairQuoteLow;
    price /= CBILLS_PER_THOUSAND;
    if (price > UINT16_MAX) abort();
    price *= CBILLS_PER_THOUSAND;
    unitTransactionPriceLow = (uint16_t)price;
    unitTransactionPriceHigh = (uint16_t)(price >> NATIVE_WORD_BITS);
}

/* MW_MAIN 081B:3ED0..3EE7. Read the stored near cursor one BYTE at a time.
 * Character views below inspect canonical records, never a second DS mirror. */
uint16_t MwHasBodyDamageForRepair(void)
{
    if (!componentRepairDamage) abort(); /* Unconverted reference, not a clean record. */
    for (uint16_t location = 0; location < CAMPAIGN_BODY_LOCATION_COUNT; ++location)
        if (componentRepairDamage->bodyDamageTiers[location] != 0) return 1;
    return 0;
}

/* MW_MAIN 081B:3E28..3ECF. A category is not a damage BYTE index: body0A
 * occupies a price-only slot, shifting weapon prices by one. */
void MwQuoteRepairComponent(CampaignUnitDamage *damage)
{
    componentRepairDamage = damage;
    componentRepairPriceLow = 0;
    componentRepairPriceHigh = 0;
    uint16_t category = repairPriceCategory;
    if (category != REPAIR_BODY)
    {
        if (!damage) abort(); /* Both quote words have already been cleared. */
        uint16_t component = category;
        if (category > REPAIR_BODY) component--;
        uint8_t tier = ((const unsigned char *)damage)[component];
        if (tier == 0) return;
    }
    uint16_t basePrice;
    if (category < REPAIR_BODY)
    {
        if (selectedOwnedUnitKind >= CAMPAIGN_MECH_KIND_COUNT) abort();
        basePrice = systemRepairBasePrices[category].basePriceByMechKind[selectedOwnedUnitKind];
    }
    else if (category == REPAIR_BODY)
    {
        if (MwHasBodyDamageForRepair() == 0) return;
        if (selectedOwnedUnitKind >= CAMPAIGN_MECH_KIND_COUNT) abort();
        basePrice = bodyRepairBasePricesByKind[selectedOwnedUnitKind];
    }
    else
    {
        if (selectedOwnedUnitKind >= CAMPAIGN_MECH_KIND_COUNT) abort();
        if (!weaponRepairPricesByKind[selectedOwnedUnitKind]) abort();
        basePrice = weaponRepairPricesByKind[selectedOwnedUnitKind][category - REPAIR_FIRST_WEAPON];
    }
    uint32_t surcharge = (uint32_t)basePrice * planetMarketPremiumPercent / PERCENT_DIVISOR;
    if (surcharge > UINT16_MAX) abort();
    uint16_t adjustedPrice = surcharge + basePrice;
    uint16_t priceMultiplier = category == REPAIR_HEAT_SINKS || category == REPAIR_JUMP_JETS || category == REPAIR_BODY
        ? CBILLS_PER_THOUSAND : REPAIR_COMPONENT_CBILLS;
    uint32_t price = (uint32_t)adjustedPrice * priceMultiplier;
    componentRepairPriceLow = (uint16_t)price;
    componentRepairPriceHigh = (uint16_t)(price >> NATIVE_WORD_BITS);
}

/* MW_MAIN 081B:3763..3801. Repeated ADD/ADC per component tier, but only one
 * body charge per nonzero location. This never repairs or debits C-bills. */
void MwQuoteMechRepairTotal(uint16_t unitKind, CampaignUnitDamage *damage)
{
    repairQuoteDamage = damage;
    repairQuoteMechKind = unitKind;
    selectedOwnedUnitKind = unitKind;
    unitRepairQuoteHigh = 0;
    unitRepairQuoteLow = 0;
    if (unitKind >= CAMPAIGN_MECH_KIND_COUNT) abort();
    if (!damage) abort();
    uint16_t fieldsRemaining = CAMPAIGN_SYSTEM_COUNT + unitWeaponCounts[unitKind];
    uint16_t category = 0;
    uint16_t component = 0;
    do
    {
        uint8_t tier = ((const unsigned char *)damage)[component];
        if (tier != 0)
        {
            repairPriceCategory = category;
            MwQuoteRepairComponent(repairQuoteDamage);
            uint32_t tierPrice = ((uint32_t)componentRepairPriceHigh << NATIVE_WORD_BITS) | componentRepairPriceLow;
            do
            {
                uint32_t total = ((uint32_t)unitRepairQuoteHigh << NATIVE_WORD_BITS) | unitRepairQuoteLow;
                total += tierPrice;
                unitRepairQuoteLow = (uint16_t)total;
                unitRepairQuoteHigh = (uint16_t)(total >> NATIVE_WORD_BITS);
            } while (--tier != 0);
        }
        component++;
        category++;
        if (category == REPAIR_BODY) category++;
    } while (--fieldsRemaining != 0);
    repairPriceCategory = REPAIR_BODY;
    MwQuoteRepairComponent(repairQuoteDamage); /* Even for zero body damage. */
    uint16_t damagedLocations = 0;
    for (uint16_t location = 0; location < CAMPAIGN_BODY_LOCATION_COUNT; ++location)
        if (damage->bodyDamageTiers[location] != 0) damagedLocations++;
    if (damagedLocations != 0)
    {
        uint32_t bodyPrice = ((uint32_t)componentRepairPriceHigh << NATIVE_WORD_BITS) | componentRepairPriceLow;
        do
        {
            uint32_t total = ((uint32_t)unitRepairQuoteHigh << NATIVE_WORD_BITS) | unitRepairQuoteLow;
            total += bodyPrice;
            unitRepairQuoteLow = (uint16_t)total;
            unitRepairQuoteHigh = (uint16_t)(total >> NATIVE_WORD_BITS);
        } while (--damagedLocations != 0);
    }
}
