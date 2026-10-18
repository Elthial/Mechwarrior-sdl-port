#include "mechwarrior/campaign_economy.h"
#include <stdlib.h>

/* MW_MAIN 081B:3BF7..3D08. AX/BX/CX/DX inputs, output in shared1090 scratch.
 * Ordered draws overwrite tiers; unused ammunition scratch remains unchanged.
 * Guards identify unrepresented native storage, not added game validation. */
void MwGenerateUnitStock(uint16_t stockKindRangeIndex, uint16_t tierThreeRolls,
    uint16_t tierTwoRolls, uint16_t tierOneRolls)
{
    uint16_t unitKind;
    if (generateStartingJenner == STOCK_FORCE_STARTER_MECH) unitKind = CAMPAIGN_UNIT_JENNER;
    else
    {
        if (stockKindRangeIndex >= STOCK_KIND_RANGE_COUNT) abort();
        unitKind = (uint8_t)(MwRandomUpTo(generatedKindRangeMaxima[stockKindRangeIndex])
            + generatedKindRangeStarts[stockKindRangeIndex]);
        if (unitKind == CAMPAIGN_UNIT_WASP) unitKind = CAMPAIGN_UNIT_LOCUST;
        if (unitKind == CAMPAIGN_UNIT_WOLVERINE) unitKind = CAMPAIGN_UNIT_PHOENIX_HAWK;
    }
    generatedUnitKind = unitKind;
    if (unitKind >= CAMPAIGN_MECH_KIND_COUNT) abort();
    generatedDamageMaximumIndex = (uint8_t)(unitWeaponCounts[unitKind] + CAMPAIGN_SYSTEM_COUNT - 1);
    uint8_t *damageBytes = (uint8_t *)&generatedMechDamage;
    for (uint16_t index = 0; index < sizeof(generatedMechDamage); ++index) damageBytes[index] = 0;
    uint16_t damageRollCounts[3] = {tierThreeRolls, tierTwoRolls, tierOneRolls};
    for (uint16_t pass = 0; pass < 3; ++pass)
        for (uint16_t roll = 0; roll < damageRollCounts[pass]; ++roll)
        {
            uint16_t damageIndex = MwRandomUpTo(generatedDamageMaximumIndex);
            if (damageIndex >= sizeof(generatedMechDamage)) abort();
            damageBytes[damageIndex] = (uint8_t)(STOCK_COMPONENT_TIER_THREE - pass);
        }
    if (unitJumpJetGroupCounts[unitKind] == 0) generatedMechDamage.destroyedJumpJetGroups = 0;
    else generatedMechDamage.destroyedJumpJetGroups >>= 1;
    uint16_t groupsRemaining = unitAmmoGroupCounts[unitKind];
    const CampaignAmmoProfileEntry *profile = unitAmmoProfiles[unitKind];
    uint16_t ammoGroup = 0;
    do
    {
        if (!profile) abort(); /* Unrepresented malformed profile, not native validation. */
        if (ammoGroup >= CAMPAIGN_AMMO_GROUPS) abort();
        generatedAmmoKinds[ammoGroup] = profile->ammoFamily;
        uint16_t quantityMaximum = profile->quantityMaximum;
        ++profile;
        generatedAmmoQuantities[ammoGroup] = MwRandomUpTo(quantityMaximum);
        ++ammoGroup;
    } while (--groupsRemaining != 0); /* Native LOOP executes once even with zero count. */
    uint16_t bodyRollsRemaining = MwRandomUpTo(STOCK_BODY_ROLL_MAXIMUM) + STOCK_BODY_MINIMUM_ROLLS;
    do
    {
        uint16_t bodyLocation = MwRandomUpTo(CAMPAIGN_BODY_LOCATION_COUNT - 1);
        uint8_t damageTier = (uint8_t)((MwRandomUpTo(STOCK_BODY_TIER_MAXIMUM) | 1) & STOCK_BODY_TIER_MAXIMUM);
        generatedMechDamage.bodyDamageTiers[bodyLocation] = damageTier;
    } while (--bodyRollsRemaining != 0);
    if (generateStartingJenner == STOCK_FORCE_STARTER_MECH)
    {
        generatedMechDamage.engineHits = 0;
        generatedMechDamage.gyroDamageTier = 0;
    }
    generateStartingJenner = 0;
}

/* MW_MAIN 081B:3D09..3DF2. Clear kind/damage, NOT ammunition; then fill
 * the live count. Each generator call retains its complete RNG sequence. */
void MwRefreshMarketUnitStock(void)
{
    dealerUnitPurchaseLimit = MwRandomUpTo((uint16_t)(DEALER_PURCHASE_ROLL_BASE - currentPlanetMarketTier)) + 1;
    unitsSoldSinceMarketRefresh = 0;
    marketUnitCount = 0;
    for (uint16_t marketSlot = 0; marketSlot < CAMPAIGN_MARKET_UNIT_SLOTS; ++marketSlot)
    {
        marketUnitKinds[marketSlot] = CAMPAIGN_VACANT_UNIT_KIND;
        uint8_t *damageBytes = (uint8_t *)&marketUnitDamage[marketSlot];
        for (uint16_t index = 0; index < sizeof(CampaignUnitDamage); ++index) damageBytes[index] = 0;
    }
    uint8_t stockDivisor = (uint8_t)(currentPlanetMarketTier + 1);
    if (stockDivisor == 0) abort(); /* Native DIV fault AFTER the clear. */
    marketUnitCount = MwRandomUpTo(CAMPAIGN_MARKET_UNIT_SLOTS / stockDivisor);
    if (marketUnitCount == 0) return;
    uint16_t marketSlot = 0;
    do
    {
        uint16_t damageRollMaximum = currentPlanetMarketTier + 1;
        uint16_t tierOneRolls = MwRandomUpTo(damageRollMaximum);
        uint16_t tierTwoRolls = MwRandomUpTo(damageRollMaximum);
        uint16_t tierThreeRolls = MwRandomUpTo(damageRollMaximum);
        uint16_t rangeRoll = MwRandomUpTo(STOCK_RANGE_ROLL_MAXIMUM);
        uint16_t stockKindRange = rangeRoll < STOCK_FIRST_RANGE_THRESHOLD ? 0
            : rangeRoll < STOCK_SECOND_RANGE_THRESHOLD ? 1 : 2;
        MwGenerateUnitStock(stockKindRange,tierThreeRolls,tierTwoRolls,tierOneRolls);
        marketUnitKinds[marketSlot] = generatedUnitKind;
        uint8_t *destination = (uint8_t *)&marketUnitDamage[(uint8_t)marketSlot];
        const uint8_t *source = (const uint8_t *)&generatedMechDamage;
        for (uint16_t index = 0; index < sizeof(CampaignUnitDamage); ++index) destination[index] = source[index];
        for (uint16_t group = 0; group < CAMPAIGN_AMMO_GROUPS; ++group)
        {
            marketAmmoKindsByGroup[group * CAMPAIGN_MARKET_UNIT_SLOTS + marketSlot] = generatedAmmoKinds[group];
            marketAmmoQuantitiesByGroup[group * CAMPAIGN_MARKET_UNIT_SLOTS + marketSlot] = generatedAmmoQuantities[group];
        }
        ++marketSlot;
    } while (marketSlot != marketUnitCount);
}

/* MW_MAIN 081B:6C0C..6CD4; sole RET at6C59. No source inputs/result.
 * BP is reused for the hired-roster scan, not a monotonic attempt count.
 * Preserve its reset/partial value on rejection; there is no added retry cap. */
void MwRefreshAvailablePilotRecruits(void)
{
    for (uint16_t slot = 0; slot < RECRUIT_OFFER_SLOTS; ++slot) availableRecruitIds[slot] = 0;
    uint16_t recruitRollMaximum = RECRUIT_COUNT_ROLL_BASE - (uint16_t)((currentPlanetMarketTier + 1) << 1);
    uint16_t desiredRecruitCount = MwRandomUpTo(recruitRollMaximum) + 1;
    availableRecruitCount = 0;
    uint16_t searchState = 0;
    for (;;)
    {
        uint16_t sampledPilotIndex = MwRandomUpTo(CAMPAIGN_PILOT_ID_COUNT - 2);
        uint16_t pilotId = sampledPilotIndex + 1;
        if (pilotUnavailableFlags[pilotId] != 0) goto nextCandidate;
        uint16_t gunneryTier = pilotGunneryTiers[pilotId];
        uint16_t commanderReputationTier = rosterPayTiers[0];
        if (gunneryTier > commanderReputationTier || (gunneryTier != commanderReputationTier
            && gunneryTier + 1 != commanderReputationTier)) goto nextCandidate;
        uint16_t portraitPairBaseIndex = sampledPilotIndex & PILOT_PORTRAIT_PAIR_MASK;
        for (uint16_t slot = 0; slot < RECRUIT_OFFER_SLOTS; ++slot)
        {
            uint16_t offeredPilotId = availableRecruitIds[slot];
            if (offeredPilotId == pilotId || ((uint16_t)(offeredPilotId - 1) & PILOT_PORTRAIT_PAIR_MASK) == portraitPairBaseIndex)
                goto nextCandidate;
        }
        searchState = pilotRosterCount;
        if (searchState != 1)
        {
            --searchState;
            uint16_t rosterSlot = 1;
            do
            {
                if (rosterSlot >= CAMPAIGN_ROSTER_SLOTS) abort(); /* Unsupported native adjacent roster. */
                if (((uint16_t)(rosterPilotIds[rosterSlot] - 1) & PILOT_PORTRAIT_PAIR_MASK) == portraitPairBaseIndex)
                    goto nextCandidate;
                ++rosterSlot;
            } while (--searchState != 0);
        }
        if (availableRecruitCount >= RECRUIT_OFFER_SLOTS) abort();
        availableRecruitIds[availableRecruitCount] = pilotId;
        ++availableRecruitCount;
nextCandidate:
        if (desiredRecruitCount == availableRecruitCount) return;
        if (++searchState == RECRUIT_SEARCH_STATE_LIMIT) return;
    }
}

/* MW_MAIN 081B:4F6E..52ED. SI offer slot; true projects CF=refused.
 * Parallel offer arrays are NOT a packed contract record. No retry limits. */
bool MwGenerateMissionOffer(uint16_t missionOfferSlot)
{
    generatingMissionOfferSlot = missionOfferSlot;
    uint16_t employerHouse = currentPlanetHouseIndex;
    if (employerHouse >= CAMPAIGN_HOUSE_COUNT) abort();
    if (houseGrievancePoints[employerHouse] >= HOUSE_GRIEVANCE_REFUSAL) return true;
    uint16_t combatCreditBand = 0;
    if (playerCombatCredit >= COMBAT_CREDIT_SECOND_BAND)
    {
        combatCreditBand = 1;
        if (playerCombatCredit >= COMBAT_CREDIT_THIRD_BAND)
        {
            combatCreditBand = 2;
            if (playerCombatCredit >= COMBAT_CREDIT_FOURTH_BAND) combatCreditBand = 3;
        }
    }
    uint16_t pilotsRemaining = pilotRosterCount, rosterByteOffset = 0;
    uint8_t assignedForceWeight = 0;
    do
    {
        uint16_t rosterSlot = rosterByteOffset / sizeof(uint16_t);
        if (rosterSlot >= CAMPAIGN_ROSTER_SLOTS) abort();
        uint16_t assignedOwnedSlot = rosterMechAssignments[rosterSlot];
        if (assignedOwnedSlot != CAMPAIGN_NO_MECH_ASSIGNMENT)
        {
            uint16_t ownedSlot = (uint16_t)(assignedOwnedSlot << 1) / sizeof(uint16_t);
            if (ownedSlot >= CAMPAIGN_OWNED_UNIT_SLOTS) abort();
            uint16_t unitKind = ownedUnitKinds[ownedSlot];
            if (unitKind < CAMPAIGN_MECH_KIND_COUNT) assignedForceWeight += missionForceWeightByUnitKind[unitKind];
            else if (unitKind == CAMPAIGN_UNIT_DROPSHIP)
                assignedForceWeight += (uint8_t)houseSalvageBasePercent[0]; /* Native adjacent BYTE, not DropShip tonnage. */
            else abort();
        }
        rosterByteOffset += sizeof(uint16_t);
    } while (--pilotsRemaining != 0);
    uint16_t baseForceBudget = (uint16_t)((combatCreditBand + assignedForceWeight) * MISSION_FORCE_BUDGET_SCALE);
    missionForceBudget = baseForceBudget;
    if (generatingMissionOfferSlot == 1 || generatingMissionOfferSlot == 4)
        missionForceBudget += baseForceBudget >> 1;
    else if (generatingMissionOfferSlot == 2 || generatingMissionOfferSlot == 5) missionForceBudget >>= 1;
    uint16_t offerSlot = generatingMissionOfferSlot;
    if (offerSlot >= MISSION_OFFER_SLOTS) abort();
    missionEnemyHeavyCounts[offerSlot] = missionForceBudget / MISSION_HEAVY_FORCE_COST;
    uint16_t remainingForceBudget = missionForceBudget % MISSION_HEAVY_FORCE_COST;
    missionEnemyMediumCounts[offerSlot] = remainingForceBudget / MISSION_MEDIUM_FORCE_COST;
    uint16_t lightEnemies = remainingForceBudget % MISSION_MEDIUM_FORCE_COST / MISSION_LIGHT_FORCE_COST;
    for (;;)
    {
        missionEnemyLightCounts[offerSlot] = lightEnemies;
        int16_t enemyCount = (int16_t)(uint16_t)(lightEnemies + missionEnemyMediumCounts[offerSlot] + missionEnemyHeavyCounts[offerSlot]);
        if (enemyCount <= 0) { missionEnemyLightCounts[offerSlot] = 1; break; }
        if (enemyCount <= MISSION_ENEMY_COUNT_LIMIT) break;
        if (missionEnemyLightCounts[offerSlot] != 0) --lightEnemies;
        else if (missionEnemyMediumCounts[offerSlot] != 0) --missionEnemyMediumCounts[offerSlot];
        else if (missionEnemyHeavyCounts[offerSlot] != 0) --missionEnemyHeavyCounts[offerSlot];
        else break;
    }
    if (missionEnemyHeavyCounts[offerSlot] == MISSION_ENEMY_COUNT_LIMIT)
    {
        uint16_t mixRoll = MwRandomUpTo(MISSION_MIX_ROLL_MAXIMUM);
        if (mixRoll >= MISSION_MIX_FIRST_THRESHOLD)
        {
            missionEnemyHeavyCounts[offerSlot] = 3;
            missionEnemyMediumCounts[offerSlot] = 1;
            if (mixRoll >= MISSION_MIX_SECOND_THRESHOLD)
            {
                missionEnemyHeavyCounts[offerSlot] = 2;
                missionEnemyMediumCounts[offerSlot] = 2;
            }
        }
    }
    missionPaymentBaseThousands = (uint8_t)((missionForceBudget >> 1) + 1) * PERCENT_DIVISOR;
    uint16_t paymentRoll = MwRandomUpTo(MISSION_PAYMENT_ROLL_MAXIMUM);
    employerHouse = currentPlanetHouseIndex;
    uint16_t biasedPaymentRoll = paymentRoll + 1 + housePaymentRollBias[employerHouse];
    uint16_t paymentProductLow = (uint16_t)((uint32_t)biasedPaymentRoll * missionPaymentBaseThousands);
    int32_t paymentAdjustment = (int32_t)(int16_t)paymentProductLow * housePaymentAdjustmentPercent[employerHouse] / PERCENT_DIVISOR;
    if (paymentAdjustment < INT16_MIN || paymentAdjustment > INT16_MAX) abort(); /* Native IDIV overflow, before terms. */
    uint16_t paymentCeiling = (uint16_t)(paymentAdjustment + missionPaymentBaseThousands);
    paymentCeiling = paymentCeiling / MISSION_PAYMENT_ROUNDING_THOUSANDS * MISSION_PAYMENT_ROUNDING_THOUSANDS;
    if (paymentCeiling < MISSION_PAYMENT_MINIMUM_THOUSANDS) paymentCeiling = MISSION_PAYMENT_MINIMUM_THOUSANDS;
    if (paymentCeiling > MISSION_PAYMENT_MAXIMUM_THOUSANDS) paymentCeiling = MISSION_PAYMENT_MAXIMUM_THOUSANDS;
    missionPaymentCeilingsThousands[offerSlot] = paymentCeiling;
    uint16_t openingPayment = (paymentCeiling >> 2) / MISSION_PAYMENT_ROUNDING_THOUSANDS * MISSION_PAYMENT_ROUNDING_THOUSANDS;
    if (openingPayment < MISSION_PAYMENT_MINIMUM_THOUSANDS) openingPayment = MISSION_PAYMENT_MINIMUM_THOUSANDS;
    missionPaymentThousands[offerSlot] = openingPayment;
    uint16_t relationshipDifference = houseFavorPoints[employerHouse] - houseGrievancePoints[employerHouse];
    int16_t relationshipAdjustment = (int16_t)(uint16_t)(relationshipDifference << 2);
    if (relationshipAdjustment > MISSION_RELATIONSHIP_LIMIT) relationshipAdjustment = MISSION_RELATIONSHIP_LIMIT;
    if (relationshipAdjustment < -MISSION_RELATIONSHIP_LIMIT) relationshipAdjustment = -MISSION_RELATIONSHIP_LIMIT;
    uint16_t salvageCeiling = relationshipAdjustment + houseSalvageBasePercent[employerHouse];
    if ((int16_t)salvageCeiling < 0) salvageCeiling = 0;
    missionSalvagePercentageCeilings[offerSlot] = salvageCeiling;
    missionSalvagePercentages[offerSlot] = salvageCeiling >> 2;
    uint16_t advanceCeiling = relationshipAdjustment + houseAdvanceBasePercent[employerHouse];
    if ((int16_t)advanceCeiling < 0) advanceCeiling = 0;
    missionAdvancePercentageCeilings[offerSlot] = advanceCeiling;
    missionAdvancePercentages[offerSlot] = advanceCeiling >> 2;
retryTarget:
    employerHouse = currentPlanetHouseIndex;
    uint16_t policyStart = (uint16_t)(employerHouse << 3), policyOffset = policyStart;
    uint16_t eligibleTargetSteps = MwRandomUpTo(MISSION_TARGET_STEP_ROLL_MAXIMUM) + 1;
    uint16_t columnsRemaining = MISSION_TARGET_COLUMNS;
    for (;;)
    {
        if (policyOffset >= sizeof(houseMissionTypeLimits)) abort();
        if (houseMissionTypeLimits[policyOffset] != 0 && --eligibleTargetSteps == 0) break;
        ++policyOffset;
        if (--columnsRemaining == 0) { columnsRemaining = MISSION_TARGET_COLUMNS; policyOffset = policyStart; }
    }
    uint16_t targetColumn = policyOffset - policyStart;
    missionTargetHouseIndices[offerSlot] = targetColumn;
    uint16_t categoryRollMaximum = (uint16_t)((houseMissionTypeLimits[policyOffset] - 1) << 2);
    uint16_t category = (MwRandomUpTo(categoryRollMaximum) >> 2) + 1;
    for (;;)
    {
        if (majorCampaignMissionRollThreshold != 0 && (int8_t)(uint8_t)MwRandomUpTo(MISSION_STORY_ROLL_MAXIMUM)
            >= (int8_t)majorCampaignMissionRollThreshold) category = MISSION_CATEGORY_MAJOR_CAMPAIGN;
        missionTypeCodes[offerSlot] = category;
        if (category >= MISSION_CATEGORY_COUNT) abort();
        uint16_t descriptionIndex = MwRandomUpTo((uint16_t)(missionDescriptionMaximumIndicesByCategory[category] << 1)) >> 1;
        uint16_t listEntryOffset = missionDescriptionListOffsetsByCategory[category] + descriptionIndex * sizeof(uint16_t);
        if (listEntryOffset < MISSION_DESCRIPTION_LIST_START || (listEntryOffset - MISSION_DESCRIPTION_LIST_START) % sizeof(uint16_t) != 0
            || (listEntryOffset - MISSION_DESCRIPTION_LIST_START) / sizeof(uint16_t) >= MISSION_DESCRIPTION_LIST_WORDS) abort();
        uint16_t descriptionOffset = missionDescriptionListEntries[(listEntryOffset - MISSION_DESCRIPTION_LIST_START) / sizeof(uint16_t)];
        missionDescriptionOffsets[offerSlot] = descriptionOffset;
        if (descriptionOffset < MISSION_DESCRIPTION_STORAGE_START || descriptionOffset >= MISSION_DESCRIPTION_STORAGE_END) abort();
        const CampaignMissionDescriptionHeader *description = (const CampaignMissionDescriptionHeader *)&missionDescriptionStorage[descriptionOffset - MISSION_DESCRIPTION_STORAGE_START];
        uint16_t scenarioCode = description->scenarioCode;
        missionScenarioCodes[offerSlot] = scenarioCode;
        for (uint16_t previousSlot = 0; previousSlot != generatingMissionOfferSlot; ++previousSlot)
            if (scenarioCode == missionScenarioCodes[previousSlot]) goto retryTarget;
        if (scenarioCode != CAMPAIGN_SCENARIO_EXTENDED_OFFENSIVE && scenarioCode != CAMPAIGN_SCENARIO_EXTENDED_DEFENSIVE
            && scenarioCode != CAMPAIGN_SCENARIO_EXTENDED_SIEGE) break;
        if (pilotRosterCount >= 2 && ownedUnitCount >= 2)
        {
            uint32_t tripledPayment = (uint32_t)missionPaymentCeilingsThousands[offerSlot] * 3;
            missionPaymentCeilingsThousands[offerSlot] = (uint16_t)tripledPayment;
            uint32_t openingDividend = (tripledPayment & UINT32_C(0xFFFF0000)) | ((uint16_t)tripledPayment >> 2);
            if (openingDividend / MISSION_PAYMENT_ROUNDING_THOUSANDS > UINT16_MAX) abort();
            missionPaymentThousands[offerSlot] = (uint16_t)((openingDividend / MISSION_PAYMENT_ROUNDING_THOUSANDS) * MISSION_PAYMENT_ROUNDING_THOUSANDS);
            missionEnemyHeavyCounts[offerSlot] *= 3;
            missionEnemyMediumCounts[offerSlot] *= 3;
            missionEnemyLightCounts[offerSlot] *= 3;
            break;
        }
        category = MISSION_CATEGORY_RAID;
    }
    category = missionTypeCodes[offerSlot];
    targetColumn = missionTargetHouseIndices[offerSlot];
    if (category != MISSION_CATEGORY_DEFENCE && targetColumn < CAMPAIGN_HOUSE_COUNT)
    {
        uint16_t rangeStart = housePlanetRangeStarts[targetColumn];
        uint16_t rangeMaximum = housePlanetRangeCounts[targetColumn];
        for (;;)
        {
            uint16_t directoryByteOffset = (uint16_t)((uint16_t)(MwRandomUpTo(rangeMaximum) + rangeStart) << 1);
            uint16_t planetIndex = directoryByteOffset / sizeof(uint16_t);
            if (planetIndex >= CAMPAIGN_PLANET_COUNT) abort();
            uint16_t planetOffset = planetRecordOffsets[planetIndex];
            if (planetOffset < PLANET_RECORDS_START || (uint32_t)planetOffset + sizeof(MwPlanetHeader) > PLANET_RECORDS_END) abort();
            const MwPlanetHeader *planet = (const MwPlanetHeader *)&planetRecordStorage[planetOffset - PLANET_RECORDS_START];
            if (planet->maximumMissionType == 0 || (uint8_t)category > planet->maximumMissionType) continue;
            missionPlanetRecordIndices[offerSlot] = planetIndex;
            break;
        }
    }
    else
    {
        uint16_t policyOffset = houseMissionPlanetTableOffsets[currentPlanetHouseIndex];
        if (policyOffset < HOUSE_PLANET_POLICIES_START || (policyOffset - HOUSE_PLANET_POLICIES_START) % sizeof(MwHouseMissionPlanetPolicy) != 0
            || (policyOffset - HOUSE_PLANET_POLICIES_START) / sizeof(MwHouseMissionPlanetPolicy) >= CAMPAIGN_HOUSE_COUNT) abort();
        const MwHouseMissionPlanetPolicy *policy = &houseMissionPlanetPolicies[(policyOffset - HOUSE_PLANET_POLICIES_START) / sizeof(MwHouseMissionPlanetPolicy)];
        uint16_t choice = MwRandomUpTo(HOUSE_PLANET_CHOICES - 1);
        if (targetColumn >= MISSION_TARGET_COLUMNS) abort();
        uint8_t planetSelector = policy->planetSelectorsByTarget[targetColumn][choice];
        missionPlanetRecordIndices[offerSlot] = (uint16_t)((planetSelector == MISSION_PLANET_SELECTOR_NONE ? 0 : planetSelector) - 1);
    }
    return false;
}

/* MW_MAIN 081B:52EE..5369. Date tests are SIGNED; only count is cleared
 * on generation failure. The six parallel arrays retain partial/stale data. */
void MwRefreshMissionOffers(void)
{
    majorCampaignMissionRollThreshold = 0;
    bool middleStoryWindow = false;
    if ((int16_t)calendarYear >= MAJOR_CAMPAIGN_MIDDLE_YEAR)
    {
        if (calendarYear == MAJOR_CAMPAIGN_MIDDLE_YEAR)
            middleStoryWindow = (int16_t)calendarMonthIndex > MAJOR_CAMPAIGN_START_MONTH
                || ((int16_t)calendarMonthIndex == MAJOR_CAMPAIGN_START_MONTH
                    && (int16_t)calendarSubmonthUnits >= MAJOR_CAMPAIGN_START_SUBMONTH_UNIT);
        else if ((int16_t)calendarYear < MAJOR_CAMPAIGN_LATE_YEAR) middleStoryWindow = true;
        else majorCampaignMissionRollThreshold = MAJOR_CAMPAIGN_LATE_THRESHOLD;
    }
    uint16_t offerCount;
    if (middleStoryWindow)
    {
        majorCampaignMissionRollThreshold = MAJOR_CAMPAIGN_MIDDLE_THRESHOLD;
        offerCount = MwRandomUpTo(MISSION_OFFER_MIDDLE_ROLL_MAXIMUM) + MISSION_OFFER_MIDDLE_MINIMUM;
    }
    else
    {
        missionOfferCount = 0;
        uint16_t skipRoll = MwRandomUpTo(MISSION_STORY_ROLL_MAXIMUM);
        if ((int16_t)skipRoll <= MISSION_OFFER_SKIP_ROLL_LIMIT) return;
        offerCount = MwRandomUpTo(MISSION_OFFER_NORMAL_ROLL_MAXIMUM) + MISSION_OFFER_NORMAL_MINIMUM;
    }
    missionOfferCount = offerCount;
    uint16_t offerSlot = 0, offersRemaining = offerCount;
    if (offersRemaining != 0)
        do
        {
            if (MwGenerateMissionOffer(offerSlot)) { missionOfferCount = 0; return; }
            ++offerSlot;
        } while (--offersRemaining != 0);
}

/* MW_MAIN 081B:0DC6..0DF6. A kind-specific BYTE view of the existing condition
 * owner, NOT a second DropShip allocation. First match only; no saturation. */
void MwAdvanceDropShipConditionDeterioration(void)
{
    for (uint16_t ownedSlot = 0; ownedSlot < CAMPAIGN_OWNED_UNIT_SLOTS; ++ownedSlot)
    {
        if (ownedUnitKinds[ownedSlot] != CAMPAIGN_UNIT_DROPSHIP) continue;
        CampaignDropShipConditionPrefix *condition = (CampaignDropShipConditionPrefix *)&ownedUnitDamage[ownedSlot];
        uint8_t deterioration = condition->deterioration;
        condition->deterioration = (uint8_t)(deterioration + (deterioration >> 2) + 1);
        return;
    }
}

/* MW_MAIN 081B:0C3E..0DC5. Calendar/refresh first, wages then DropShip upkeep.
 * All nested procedures are real Original code; no host calendar or game seams. */
void MwAdvanceCampaignTimeAndChargeWages(uint16_t elapsedUnits)
{
    elapsedBillingMonths = 0;
    marketRefreshUnits += elapsedUnits;
    calendarSubmonthUnits += elapsedUnits;
    if (calendarSubmonthUnits >= CAMPAIGN_UNITS_PER_BILLING_MONTH)
    {
        elapsedBillingMonths = 1; /* Native pre-DIV write survives a fault. */
        uint16_t months = calendarSubmonthUnits / CAMPAIGN_UNITS_PER_BILLING_MONTH;
        if (months > UINT8_MAX) abort();
        uint16_t submonthRemainder = calendarSubmonthUnits % CAMPAIGN_UNITS_PER_BILLING_MONTH;
        elapsedBillingMonths = months;
        calendarMonthIndex += months;
        calendarSubmonthUnits = submonthRemainder;
        if (calendarMonthIndex >= CAMPAIGN_MONTH_COUNT)
        {
            uint16_t years = calendarMonthIndex / CAMPAIGN_MONTH_COUNT;
            if (years > UINT8_MAX) abort();
            uint16_t monthRemainder = calendarMonthIndex % CAMPAIGN_MONTH_COUNT;
            calendarYear += years;
            calendarMonthIndex = monthRemainder;
            if (years != 0) MwAdvanceDropShipConditionDeterioration(); /* Once, not once per year. */
        }
    }
    uint16_t refreshUnits = marketRefreshUnits;
    if (refreshUnits >= MARKET_REFRESH_PERIOD_UNITS)
    {
        if (refreshUnits / MARKET_REFRESH_PERIOD_UNITS > UINT8_MAX) abort();
        marketRefreshUnits = refreshUnits % MARKET_REFRESH_PERIOD_UNITS;
        MwRefreshAvailablePilotRecruits();
        MwRefreshMarketUnitStock();
        MwRefreshMissionOffers();
    }
    if (elapsedBillingMonths == 0) return;
    if (pilotRosterCount != 1)
    {
        uint16_t pilotsRemaining = pilotRosterCount - 1, rosterSlot = 1, monthlyWages = 0;
        do
        {
            if (rosterSlot >= CAMPAIGN_ROSTER_SLOTS) abort();
            uint16_t payTier = (uint16_t)(rosterPayTiers[rosterSlot] << 1) / sizeof(uint16_t);
            if (payTier >= CAMPAIGN_ROSTER_SLOTS) abort();
            monthlyWages += pilotWageAmounts[payTier];
            ++rosterSlot;
        } while (--pilotsRemaining != 0);
        uint16_t wageDebit = (uint16_t)((uint32_t)monthlyWages * elapsedBillingMonths);
        uint16_t fundsLow = campaignFundsLow, fundsHigh = campaignFundsHigh;
        if (fundsHigh == 0 && fundsLow < wageDebit)
        {
            if (elapsedBillingMonths <= 1 && unpaidPilotWageMarker == 0)
            {
                notificationPending[NOTIFICATION_UNPAID_PILOT_WAGES] = 1;
                unpaidPilotWageMarker = wageDebit;
            }
            else
            {
                unpaidPilotWageMarker = 0;
                notificationPending[NOTIFICATION_PILOTS_DEPARTED] = 1;
                notificationPending[NOTIFICATION_UNPAID_PILOT_WAGES] = 0;
                pilotRosterCount = 1;
                for (uint16_t slot = 1; slot < CAMPAIGN_ROSTER_SLOTS; ++slot)
                    rosterMechAssignments[slot] = CAMPAIGN_NO_MECH_ASSIGNMENT;
                for (uint16_t slot = 1; slot < CAMPAIGN_ROSTER_SLOTS; ++slot)
                {
                    uint16_t flagOffset = PILOT_DEPARTURE_NATIVE_BASE + rosterPilotIds[slot];
                    if (flagOffset >= PILOT_AVAILABILITY_NATIVE_BASE
                        && flagOffset < PILOT_AVAILABILITY_NATIVE_BASE + CAMPAIGN_PILOT_ID_COUNT)
                        pilotUnavailableFlags[flagOffset - PILOT_AVAILABILITY_NATIVE_BASE] = 0;
                    else if (flagOffset == PILOT_DEPARTURE_TAIL_OFFSET) pilotDepartureFlagTail = 0;
                    else abort(); /* Unmapped adjacent native byte, not a new ID validation rule. */
                }
            }
        }
        else
        {
            campaignFundsLow = fundsLow - wageDebit;
            campaignFundsHigh = fundsHigh - (fundsLow < wageDebit);
            /* Paid wages do NOT clear the earlier warning or unpaid marker. */
        }
    }
    uint16_t upkeepDebit = (uint16_t)((uint32_t)DROPSHIP_MONTHLY_UPKEEP_CBILLS * elapsedBillingMonths);
    for (uint16_t ownedSlot = 0; ownedSlot < CAMPAIGN_OWNED_UNIT_SLOTS; ++ownedSlot)
    {
        if (ownedUnitKinds[ownedSlot] != CAMPAIGN_UNIT_DROPSHIP) continue;
        uint16_t fundsLow = campaignFundsLow, fundsHigh = campaignFundsHigh;
        if (fundsHigh == 0 && fundsLow < upkeepDebit)
        {
            if (elapsedBillingMonths <= 1)
            {
                ++dropShipMissedUpkeepPayments;
                if (dropShipMissedUpkeepPayments != DROPSHIP_UPKEEP_DEPARTURE_COUNT)
                {
                    notificationPending[NOTIFICATION_UNPAID_DROPSHIP_UPKEEP] = 1;
                    continue;
                }
            }
            notificationPending[NOTIFICATION_DROPSHIP_CREW_DEPARTED] = 1;
            notificationPending[NOTIFICATION_UNPAID_DROPSHIP_UPKEEP] = 0;
            ownedUnitKinds[ownedSlot] = CAMPAIGN_VACANT_UNIT_KIND;
            --ownedUnitCount;
            dropShipMissedUpkeepPayments = 0;
        }
        else
        {
            campaignFundsLow = fundsLow - upkeepDebit;
            campaignFundsHigh = fundsHigh - (fundsLow < upkeepDebit);
            dropShipMissedUpkeepPayments = 0;
        }
    }
}
