/* GAM/resident transfer boundary only. Offsets are native0448-relative; never
 * use this encoded buffer as live game state. Generated from canonical
 * owners by Generate-CampaignSaveCodec.py, checked for exact0718 coverage. */
#include "mechwarrior/campaign_options.h"
#include "mechwarrior/campaign_economy.h"
#include <string.h>

void MwEncodeCampaignExchange(uint8_t bytes[CAMPAIGN_SAVE_BYTES])
{
    bytes[0x0000]=unidentifiedField18;
    bytes[0x0001]=missionCategoryOrMemoryTier;
    bytes[0x0002]=deploymentLandscapeIndex;
    bytes[0x0003]=combatEnemyHeavyCount;
    bytes[0x0004]=combatEnemyMediumCount;
    bytes[0x0005]=combatEnemyLightCount;
    bytes[0x0006]=combatPlanetSelector;
    bytes[0x0007]=combatScenarioCode;
    bytes[0x0008]=deploymentTerrainDetailLevel;
    memcpy(bytes+0x0009,rosterStatus,sizeof(rosterStatus));
    memcpy(bytes+0x000D,salvageEntries,sizeof(salvageEntries));
    bytes[0x001D]=(uint8_t)returnedDisabledEnemyCount;
    bytes[0x001D+1]=(uint8_t)(returnedDisabledEnemyCount>>8);
    bytes[0x001F]=combatTransitionResult;
    bytes[0x0020]=acceptedContractActive;
    bytes[0x0021]=(uint8_t)currentPlanetIndex;
    bytes[0x0021+1]=(uint8_t)(currentPlanetIndex>>8);
    bytes[0x0023]=(uint8_t)currentPlanetLandscapeIndex;
    bytes[0x0023+1]=(uint8_t)(currentPlanetLandscapeIndex>>8);
    bytes[0x0025]=(uint8_t)currentPlanetHouseIndex;
    bytes[0x0025+1]=(uint8_t)(currentPlanetHouseIndex>>8);
    bytes[0x0027]=(uint8_t)currentPlanetMarketTier;
    bytes[0x0027+1]=(uint8_t)(currentPlanetMarketTier>>8);
    bytes[0x0029]=(uint8_t)currentPlanetFacilityDisplayFlag;
    bytes[0x0029+1]=(uint8_t)(currentPlanetFacilityDisplayFlag>>8);
    bytes[0x002B]=(uint8_t)currentPlanetMapPositionX;
    bytes[0x002B+1]=(uint8_t)(currentPlanetMapPositionX>>8);
    bytes[0x002D]=(uint8_t)currentPlanetMapPositionY;
    bytes[0x002D+1]=(uint8_t)(currentPlanetMapPositionY>>8);
    bytes[0x002F]=(uint8_t)planetMarketPremiumPercent;
    bytes[0x002F+1]=(uint8_t)(planetMarketPremiumPercent>>8);
    bytes[0x0031]=(uint8_t)calendarSubmonthUnits;
    bytes[0x0031+1]=(uint8_t)(calendarSubmonthUnits>>8);
    bytes[0x0033]=(uint8_t)calendarMonthIndex;
    bytes[0x0033+1]=(uint8_t)(calendarMonthIndex>>8);
    bytes[0x0035]=(uint8_t)calendarYear;
    bytes[0x0035+1]=(uint8_t)(calendarYear>>8);
    bytes[0x0037]=(uint8_t)marketRefreshUnits;
    bytes[0x0037+1]=(uint8_t)(marketRefreshUnits>>8);
    bytes[0x0039]=(uint8_t)elapsedBillingMonths;
    bytes[0x0039+1]=(uint8_t)(elapsedBillingMonths>>8);
    memcpy(bytes+0x003B,commanderNameBytes,sizeof(commanderNameBytes));
    memcpy(bytes+0x0045,unidentifiedField19,sizeof(unidentifiedField19));
    bytes[0x0047]=(uint8_t)commanderAgeYears;
    bytes[0x0047+1]=(uint8_t)(commanderAgeYears>>8);
    bytes[0x0049]=(uint8_t)campaignFundsLow;
    bytes[0x0049+1]=(uint8_t)(campaignFundsLow>>8);
    bytes[0x004B]=(uint8_t)campaignFundsHigh;
    bytes[0x004B+1]=(uint8_t)(campaignFundsHigh>>8);
    for (size_t i=0;i<sizeof(houseNetStandings)/sizeof(houseNetStandings[0]);++i)
    {
        bytes[0x004D+i*2]=(uint8_t)houseNetStandings[i];
        bytes[0x004D+i*2+1]=(uint8_t)(houseNetStandings[i]>>8);
    }
    for (size_t i=0;i<sizeof(houseFavorPoints)/sizeof(houseFavorPoints[0]);++i)
    {
        bytes[0x0057+i*2]=(uint8_t)houseFavorPoints[i];
        bytes[0x0057+i*2+1]=(uint8_t)(houseFavorPoints[i]>>8);
    }
    for (size_t i=0;i<sizeof(houseGrievancePoints)/sizeof(houseGrievancePoints[0]);++i)
    {
        bytes[0x0061+i*2]=(uint8_t)houseGrievancePoints[i];
        bytes[0x0061+i*2+1]=(uint8_t)(houseGrievancePoints[i]>>8);
    }
    bytes[0x006B]=(uint8_t)playerCombatCredit;
    bytes[0x006B+1]=(uint8_t)(playerCombatCredit>>8);
    bytes[0x006D]=(uint8_t)pilotRosterCount;
    bytes[0x006D+1]=(uint8_t)(pilotRosterCount>>8);
    for (size_t i=0;i<sizeof(rosterPilotIds)/sizeof(rosterPilotIds[0]);++i)
    {
        bytes[0x006F+i*2]=(uint8_t)rosterPilotIds[i];
        bytes[0x006F+i*2+1]=(uint8_t)(rosterPilotIds[i]>>8);
    }
    memcpy(bytes+0x0077,pilotIdentityStorage.roster.unavailablePilots1To42,sizeof(pilotIdentityStorage.roster.unavailablePilots1To42));
    bytes[0x00A1]=pilotDepartureFlagTail;
    for (size_t i=0;i<sizeof(rosterPayTiers)/sizeof(rosterPayTiers[0]);++i)
    {
        bytes[0x00A2+i*2]=(uint8_t)rosterPayTiers[i];
        bytes[0x00A2+i*2+1]=(uint8_t)(rosterPayTiers[i]>>8);
    }
    for (size_t i=0;i<sizeof(rosterGunneryTiers)/sizeof(rosterGunneryTiers[0]);++i)
    {
        bytes[0x00AA+i*2]=(uint8_t)rosterGunneryTiers[i];
        bytes[0x00AA+i*2+1]=(uint8_t)(rosterGunneryTiers[i]>>8);
    }
    for (size_t i=0;i<sizeof(rosterPilotingTiers)/sizeof(rosterPilotingTiers[0]);++i)
    {
        bytes[0x00B2+i*2]=(uint8_t)rosterPilotingTiers[i];
        bytes[0x00B2+i*2+1]=(uint8_t)(rosterPilotingTiers[i]>>8);
    }
    for (size_t i=0;i<sizeof(rosterUnknownWords)/sizeof(rosterUnknownWords[0]);++i)
    {
        bytes[0x00BA+i*2]=(uint8_t)rosterUnknownWords[i];
        bytes[0x00BA+i*2+1]=(uint8_t)(rosterUnknownWords[i]>>8);
    }
    for (size_t i=0;i<sizeof(rosterMechAssignments)/sizeof(rosterMechAssignments[0]);++i)
    {
        bytes[0x00C2+i*2]=(uint8_t)rosterMechAssignments[i];
        bytes[0x00C2+i*2+1]=(uint8_t)(rosterMechAssignments[i]>>8);
    }
    for (size_t i=0;i<sizeof(rosterPortraitIds)/sizeof(rosterPortraitIds[0]);++i)
    {
        bytes[0x00CA+i*2]=(uint8_t)rosterPortraitIds[i];
        bytes[0x00CA+i*2+1]=(uint8_t)(rosterPortraitIds[i]>>8);
    }
    bytes[0x00D2]=(uint8_t)dropShipMissedUpkeepPayments;
    bytes[0x00D2+1]=(uint8_t)(dropShipMissedUpkeepPayments>>8);
    bytes[0x00D4]=(uint8_t)unpaidPilotWageMarker;
    bytes[0x00D4+1]=(uint8_t)(unpaidPilotWageMarker>>8);
    bytes[0x00D6]=(uint8_t)availableRecruitCount;
    bytes[0x00D6+1]=(uint8_t)(availableRecruitCount>>8);
    for (size_t i=0;i<sizeof(availableRecruitIds)/sizeof(availableRecruitIds[0]);++i)
    {
        bytes[0x00D8+i*2]=(uint8_t)availableRecruitIds[i];
        bytes[0x00D8+i*2+1]=(uint8_t)(availableRecruitIds[i]>>8);
    }
    bytes[0x00E8]=(uint8_t)ownedUnitCount;
    bytes[0x00E8+1]=(uint8_t)(ownedUnitCount>>8);
    for (size_t i=0;i<sizeof(ownedUnitKinds)/sizeof(ownedUnitKinds[0]);++i)
    {
        bytes[0x00EA+i*2]=(uint8_t)ownedUnitKinds[i];
        bytes[0x00EA+i*2+1]=(uint8_t)(ownedUnitKinds[i]>>8);
    }
    memcpy(bytes+0x0102,ownedUnitDamage,sizeof(ownedUnitDamage));
    for (size_t i=0;i<sizeof(ownedAmmoQuantitiesByGroup)/sizeof(ownedAmmoQuantitiesByGroup[0]);++i)
    {
        bytes[0x025E+i*2]=(uint8_t)ownedAmmoQuantitiesByGroup[i];
        bytes[0x025E+i*2+1]=(uint8_t)(ownedAmmoQuantitiesByGroup[i]>>8);
    }
    for (size_t i=0;i<sizeof(marketAmmoQuantitiesByGroup)/sizeof(marketAmmoQuantitiesByGroup[0]);++i)
    {
        bytes[0x02A6+i*2]=(uint8_t)marketAmmoQuantitiesByGroup[i];
        bytes[0x02A6+i*2+1]=(uint8_t)(marketAmmoQuantitiesByGroup[i]>>8);
    }
    for (size_t i=0;i<sizeof(reserveAmmoQuantitiesByFamily)/sizeof(reserveAmmoQuantitiesByFamily[0]);++i)
    {
        bytes[0x02EE+i*2]=(uint8_t)reserveAmmoQuantitiesByFamily[i];
        bytes[0x02EE+i*2+1]=(uint8_t)(reserveAmmoQuantitiesByFamily[i]>>8);
    }
    for (size_t i=0;i<sizeof(ownedAmmoKindsByGroup)/sizeof(ownedAmmoKindsByGroup[0]);++i)
    {
        bytes[0x02FA+i*2]=(uint8_t)ownedAmmoKindsByGroup[i];
        bytes[0x02FA+i*2+1]=(uint8_t)(ownedAmmoKindsByGroup[i]>>8);
    }
    for (size_t i=0;i<sizeof(marketAmmoKindsByGroup)/sizeof(marketAmmoKindsByGroup[0]);++i)
    {
        bytes[0x0342+i*2]=(uint8_t)marketAmmoKindsByGroup[i];
        bytes[0x0342+i*2+1]=(uint8_t)(marketAmmoKindsByGroup[i]>>8);
    }
    bytes[0x038A]=(uint8_t)marketUnitCount;
    bytes[0x038A+1]=(uint8_t)(marketUnitCount>>8);
    for (size_t i=0;i<sizeof(marketUnitKinds)/sizeof(marketUnitKinds[0]);++i)
    {
        bytes[0x038C+i*2]=(uint8_t)marketUnitKinds[i];
        bytes[0x038C+i*2+1]=(uint8_t)(marketUnitKinds[i]>>8);
    }
    memcpy(bytes+0x03A4,marketUnitDamage,sizeof(marketUnitDamage));
    bytes[0x0500]=(uint8_t)missionOfferCount;
    bytes[0x0500+1]=(uint8_t)(missionOfferCount>>8);
    for (size_t i=0;i<sizeof(missionDescriptionOffsets)/sizeof(missionDescriptionOffsets[0]);++i)
    {
        bytes[0x0502+i*2]=(uint8_t)missionDescriptionOffsets[i];
        bytes[0x0502+i*2+1]=(uint8_t)(missionDescriptionOffsets[i]>>8);
    }
    for (size_t i=0;i<sizeof(missionPlanetRecordIndices)/sizeof(missionPlanetRecordIndices[0]);++i)
    {
        bytes[0x050E+i*2]=(uint8_t)missionPlanetRecordIndices[i];
        bytes[0x050E+i*2+1]=(uint8_t)(missionPlanetRecordIndices[i]>>8);
    }
    for (size_t i=0;i<sizeof(missionTargetHouseIndices)/sizeof(missionTargetHouseIndices[0]);++i)
    {
        bytes[0x051A+i*2]=(uint8_t)missionTargetHouseIndices[i];
        bytes[0x051A+i*2+1]=(uint8_t)(missionTargetHouseIndices[i]>>8);
    }
    for (size_t i=0;i<sizeof(missionTypeCodes)/sizeof(missionTypeCodes[0]);++i)
    {
        bytes[0x0526+i*2]=(uint8_t)missionTypeCodes[i];
        bytes[0x0526+i*2+1]=(uint8_t)(missionTypeCodes[i]>>8);
    }
    for (size_t i=0;i<sizeof(missionEnemyHeavyCounts)/sizeof(missionEnemyHeavyCounts[0]);++i)
    {
        bytes[0x0532+i*2]=(uint8_t)missionEnemyHeavyCounts[i];
        bytes[0x0532+i*2+1]=(uint8_t)(missionEnemyHeavyCounts[i]>>8);
    }
    for (size_t i=0;i<sizeof(missionEnemyMediumCounts)/sizeof(missionEnemyMediumCounts[0]);++i)
    {
        bytes[0x053E+i*2]=(uint8_t)missionEnemyMediumCounts[i];
        bytes[0x053E+i*2+1]=(uint8_t)(missionEnemyMediumCounts[i]>>8);
    }
    for (size_t i=0;i<sizeof(missionEnemyLightCounts)/sizeof(missionEnemyLightCounts[0]);++i)
    {
        bytes[0x054A+i*2]=(uint8_t)missionEnemyLightCounts[i];
        bytes[0x054A+i*2+1]=(uint8_t)(missionEnemyLightCounts[i]>>8);
    }
    for (size_t i=0;i<sizeof(missionPaymentThousands)/sizeof(missionPaymentThousands[0]);++i)
    {
        bytes[0x0556+i*2]=(uint8_t)missionPaymentThousands[i];
        bytes[0x0556+i*2+1]=(uint8_t)(missionPaymentThousands[i]>>8);
    }
    for (size_t i=0;i<sizeof(missionSalvagePercentages)/sizeof(missionSalvagePercentages[0]);++i)
    {
        bytes[0x0562+i*2]=(uint8_t)missionSalvagePercentages[i];
        bytes[0x0562+i*2+1]=(uint8_t)(missionSalvagePercentages[i]>>8);
    }
    for (size_t i=0;i<sizeof(missionAdvancePercentages)/sizeof(missionAdvancePercentages[0]);++i)
    {
        bytes[0x056E+i*2]=(uint8_t)missionAdvancePercentages[i];
        bytes[0x056E+i*2+1]=(uint8_t)(missionAdvancePercentages[i]>>8);
    }
    for (size_t i=0;i<sizeof(missionPaymentCeilingsThousands)/sizeof(missionPaymentCeilingsThousands[0]);++i)
    {
        bytes[0x057A+i*2]=(uint8_t)missionPaymentCeilingsThousands[i];
        bytes[0x057A+i*2+1]=(uint8_t)(missionPaymentCeilingsThousands[i]>>8);
    }
    for (size_t i=0;i<sizeof(missionSalvagePercentageCeilings)/sizeof(missionSalvagePercentageCeilings[0]);++i)
    {
        bytes[0x0586+i*2]=(uint8_t)missionSalvagePercentageCeilings[i];
        bytes[0x0586+i*2+1]=(uint8_t)(missionSalvagePercentageCeilings[i]>>8);
    }
    for (size_t i=0;i<sizeof(missionAdvancePercentageCeilings)/sizeof(missionAdvancePercentageCeilings[0]);++i)
    {
        bytes[0x0592+i*2]=(uint8_t)missionAdvancePercentageCeilings[i];
        bytes[0x0592+i*2+1]=(uint8_t)(missionAdvancePercentageCeilings[i]>>8);
    }
    bytes[0x059E]=(uint8_t)contractNegotiationAttemptLimit;
    bytes[0x059E+1]=(uint8_t)(contractNegotiationAttemptLimit>>8);
    bytes[0x05A0]=(uint8_t)requestedContractPaymentThousands;
    bytes[0x05A0+1]=(uint8_t)(requestedContractPaymentThousands>>8);
    bytes[0x05A2]=(uint8_t)requestedContractSalvagePercent;
    bytes[0x05A2+1]=(uint8_t)(requestedContractSalvagePercent>>8);
    bytes[0x05A4]=(uint8_t)requestedContractAdvancePercent;
    bytes[0x05A4+1]=(uint8_t)(requestedContractAdvancePercent>>8);
    bytes[0x05A6]=(uint8_t)requestContractPaymentEnabled;
    bytes[0x05A6+1]=(uint8_t)(requestContractPaymentEnabled>>8);
    bytes[0x05A8]=(uint8_t)requestContractSalvageEnabled;
    bytes[0x05A8+1]=(uint8_t)(requestContractSalvageEnabled>>8);
    bytes[0x05AA]=(uint8_t)requestContractAdvanceEnabled;
    bytes[0x05AA+1]=(uint8_t)(requestContractAdvanceEnabled>>8);
    memcpy(bytes+0x05AC,unidentifiedField20,sizeof(unidentifiedField20));
    bytes[0x05B2]=(uint8_t)acceptedContractPaymentThousands;
    bytes[0x05B2+1]=(uint8_t)(acceptedContractPaymentThousands>>8);
    bytes[0x05B4]=(uint8_t)acceptedContractSalvagePercent;
    bytes[0x05B4+1]=(uint8_t)(acceptedContractSalvagePercent>>8);
    bytes[0x05B6]=(uint8_t)acceptedContractAdvancePercent;
    bytes[0x05B6+1]=(uint8_t)(acceptedContractAdvancePercent>>8);
    bytes[0x05B8]=(uint8_t)remainingContractPaymentThousands;
    bytes[0x05B8+1]=(uint8_t)(remainingContractPaymentThousands>>8);
    memcpy(bytes+0x05BA,houseMissionTypeLimits,sizeof(houseMissionTypeLimits));
    bytes[0x05E2]=audioOptionLabelIndex;
    for (size_t i=0;i<sizeof(missionScenarioCodes)/sizeof(missionScenarioCodes[0]);++i)
    {
        bytes[0x05E3+i*2]=(uint8_t)missionScenarioCodes[i];
        bytes[0x05E3+i*2+1]=(uint8_t)(missionScenarioCodes[i]>>8);
    }
    memcpy(bytes+0x05EF,pilotGunneryTiers,sizeof(pilotGunneryTiers));
    memcpy(bytes+0x061A,pilotPilotingTiers,sizeof(pilotPilotingTiers));
    memcpy(bytes+0x0645,pilotExperienceCounts,sizeof(pilotExperienceCounts));
    bytes[0x0670]=(uint8_t)eventDirectoryIndex;
    bytes[0x0670+1]=(uint8_t)(eventDirectoryIndex>>8);
    memcpy(bytes+0x0672,eventSeenFlags,sizeof(eventSeenFlags));
    for (size_t i=0;i<sizeof(campaignEventPlanetSelectors)/sizeof(campaignEventPlanetSelectors[0]);++i)
    {
        bytes[0x06FE+i*2]=(uint8_t)campaignEventPlanetSelectors[i];
        bytes[0x06FE+i*2+1]=(uint8_t)(campaignEventPlanetSelectors[i]>>8);
    }
    bytes[0x070A]=majorCampaignMissionRollThreshold;
    bytes[0x070B]=(uint8_t)acceptedContractTargetHouseIndex;
    bytes[0x070B+1]=(uint8_t)(acceptedContractTargetHouseIndex>>8);
    bytes[0x070D]=campaignTerrainDetailLevel;
    bytes[0x070E]=(uint8_t)dealerUnitPurchaseLimit;
    bytes[0x070E+1]=(uint8_t)(dealerUnitPurchaseLimit>>8);
    bytes[0x0710]=(uint8_t)unitsSoldSinceMarketRefresh;
    bytes[0x0710+1]=(uint8_t)(unitsSoldSinceMarketRefresh>>8);
    bytes[0x0712]=(uint8_t)playerSkillProgress;
    bytes[0x0712+1]=(uint8_t)(playerSkillProgress>>8);
    bytes[0x0714]=darkWingBaseVictoryRecorded;
    bytes[0x0715]=(uint8_t)launcherMemoryTier;
    bytes[0x0715+1]=(uint8_t)(launcherMemoryTier>>8);
    bytes[0x0717]=savedVideoMode;
}

void MwDecodeCampaignExchange(const uint8_t bytes[CAMPAIGN_SAVE_BYTES])
{
    unidentifiedField18=bytes[0x0000];
    missionCategoryOrMemoryTier=bytes[0x0001];
    deploymentLandscapeIndex=bytes[0x0002];
    combatEnemyHeavyCount=bytes[0x0003];
    combatEnemyMediumCount=bytes[0x0004];
    combatEnemyLightCount=bytes[0x0005];
    combatPlanetSelector=bytes[0x0006];
    combatScenarioCode=bytes[0x0007];
    deploymentTerrainDetailLevel=bytes[0x0008];
    memcpy(rosterStatus,bytes+0x0009,sizeof(rosterStatus));
    memcpy(salvageEntries,bytes+0x000D,sizeof(salvageEntries));
    returnedDisabledEnemyCount=(uint16_t)(bytes[0x001D]|((uint16_t)bytes[0x001D+1]<<8));
    combatTransitionResult=bytes[0x001F];
    acceptedContractActive=bytes[0x0020];
    currentPlanetIndex=(uint16_t)(bytes[0x0021]|((uint16_t)bytes[0x0021+1]<<8));
    currentPlanetLandscapeIndex=(uint16_t)(bytes[0x0023]|((uint16_t)bytes[0x0023+1]<<8));
    currentPlanetHouseIndex=(uint16_t)(bytes[0x0025]|((uint16_t)bytes[0x0025+1]<<8));
    currentPlanetMarketTier=(uint16_t)(bytes[0x0027]|((uint16_t)bytes[0x0027+1]<<8));
    currentPlanetFacilityDisplayFlag=(uint16_t)(bytes[0x0029]|((uint16_t)bytes[0x0029+1]<<8));
    currentPlanetMapPositionX=(uint16_t)(bytes[0x002B]|((uint16_t)bytes[0x002B+1]<<8));
    currentPlanetMapPositionY=(uint16_t)(bytes[0x002D]|((uint16_t)bytes[0x002D+1]<<8));
    planetMarketPremiumPercent=(uint16_t)(bytes[0x002F]|((uint16_t)bytes[0x002F+1]<<8));
    calendarSubmonthUnits=(uint16_t)(bytes[0x0031]|((uint16_t)bytes[0x0031+1]<<8));
    calendarMonthIndex=(uint16_t)(bytes[0x0033]|((uint16_t)bytes[0x0033+1]<<8));
    calendarYear=(uint16_t)(bytes[0x0035]|((uint16_t)bytes[0x0035+1]<<8));
    marketRefreshUnits=(uint16_t)(bytes[0x0037]|((uint16_t)bytes[0x0037+1]<<8));
    elapsedBillingMonths=(uint16_t)(bytes[0x0039]|((uint16_t)bytes[0x0039+1]<<8));
    memcpy(commanderNameBytes,bytes+0x003B,sizeof(commanderNameBytes));
    memcpy(unidentifiedField19,bytes+0x0045,sizeof(unidentifiedField19));
    commanderAgeYears=(uint16_t)(bytes[0x0047]|((uint16_t)bytes[0x0047+1]<<8));
    campaignFundsLow=(uint16_t)(bytes[0x0049]|((uint16_t)bytes[0x0049+1]<<8));
    campaignFundsHigh=(uint16_t)(bytes[0x004B]|((uint16_t)bytes[0x004B+1]<<8));
    for (size_t i=0;i<sizeof(houseNetStandings)/sizeof(houseNetStandings[0]);++i)
    {
        houseNetStandings[i]=(uint16_t)(bytes[0x004D+i*2]|((uint16_t)bytes[0x004D+i*2+1]<<8));
    }
    for (size_t i=0;i<sizeof(houseFavorPoints)/sizeof(houseFavorPoints[0]);++i)
    {
        houseFavorPoints[i]=(uint16_t)(bytes[0x0057+i*2]|((uint16_t)bytes[0x0057+i*2+1]<<8));
    }
    for (size_t i=0;i<sizeof(houseGrievancePoints)/sizeof(houseGrievancePoints[0]);++i)
    {
        houseGrievancePoints[i]=(uint16_t)(bytes[0x0061+i*2]|((uint16_t)bytes[0x0061+i*2+1]<<8));
    }
    playerCombatCredit=(uint16_t)(bytes[0x006B]|((uint16_t)bytes[0x006B+1]<<8));
    pilotRosterCount=(uint16_t)(bytes[0x006D]|((uint16_t)bytes[0x006D+1]<<8));
    for (size_t i=0;i<sizeof(rosterPilotIds)/sizeof(rosterPilotIds[0]);++i)
    {
        rosterPilotIds[i]=(uint16_t)(bytes[0x006F+i*2]|((uint16_t)bytes[0x006F+i*2+1]<<8));
    }
    memcpy(pilotIdentityStorage.roster.unavailablePilots1To42,bytes+0x0077,sizeof(pilotIdentityStorage.roster.unavailablePilots1To42));
    pilotDepartureFlagTail=bytes[0x00A1];
    for (size_t i=0;i<sizeof(rosterPayTiers)/sizeof(rosterPayTiers[0]);++i)
    {
        rosterPayTiers[i]=(uint16_t)(bytes[0x00A2+i*2]|((uint16_t)bytes[0x00A2+i*2+1]<<8));
    }
    for (size_t i=0;i<sizeof(rosterGunneryTiers)/sizeof(rosterGunneryTiers[0]);++i)
    {
        rosterGunneryTiers[i]=(uint16_t)(bytes[0x00AA+i*2]|((uint16_t)bytes[0x00AA+i*2+1]<<8));
    }
    for (size_t i=0;i<sizeof(rosterPilotingTiers)/sizeof(rosterPilotingTiers[0]);++i)
    {
        rosterPilotingTiers[i]=(uint16_t)(bytes[0x00B2+i*2]|((uint16_t)bytes[0x00B2+i*2+1]<<8));
    }
    for (size_t i=0;i<sizeof(rosterUnknownWords)/sizeof(rosterUnknownWords[0]);++i)
    {
        rosterUnknownWords[i]=(uint16_t)(bytes[0x00BA+i*2]|((uint16_t)bytes[0x00BA+i*2+1]<<8));
    }
    for (size_t i=0;i<sizeof(rosterMechAssignments)/sizeof(rosterMechAssignments[0]);++i)
    {
        rosterMechAssignments[i]=(uint16_t)(bytes[0x00C2+i*2]|((uint16_t)bytes[0x00C2+i*2+1]<<8));
    }
    for (size_t i=0;i<sizeof(rosterPortraitIds)/sizeof(rosterPortraitIds[0]);++i)
    {
        rosterPortraitIds[i]=(uint16_t)(bytes[0x00CA+i*2]|((uint16_t)bytes[0x00CA+i*2+1]<<8));
    }
    dropShipMissedUpkeepPayments=(uint16_t)(bytes[0x00D2]|((uint16_t)bytes[0x00D2+1]<<8));
    unpaidPilotWageMarker=(uint16_t)(bytes[0x00D4]|((uint16_t)bytes[0x00D4+1]<<8));
    availableRecruitCount=(uint16_t)(bytes[0x00D6]|((uint16_t)bytes[0x00D6+1]<<8));
    for (size_t i=0;i<sizeof(availableRecruitIds)/sizeof(availableRecruitIds[0]);++i)
    {
        availableRecruitIds[i]=(uint16_t)(bytes[0x00D8+i*2]|((uint16_t)bytes[0x00D8+i*2+1]<<8));
    }
    ownedUnitCount=(uint16_t)(bytes[0x00E8]|((uint16_t)bytes[0x00E8+1]<<8));
    for (size_t i=0;i<sizeof(ownedUnitKinds)/sizeof(ownedUnitKinds[0]);++i)
    {
        ownedUnitKinds[i]=(uint16_t)(bytes[0x00EA+i*2]|((uint16_t)bytes[0x00EA+i*2+1]<<8));
    }
    memcpy(ownedUnitDamage,bytes+0x0102,sizeof(ownedUnitDamage));
    for (size_t i=0;i<sizeof(ownedAmmoQuantitiesByGroup)/sizeof(ownedAmmoQuantitiesByGroup[0]);++i)
    {
        ownedAmmoQuantitiesByGroup[i]=(uint16_t)(bytes[0x025E+i*2]|((uint16_t)bytes[0x025E+i*2+1]<<8));
    }
    for (size_t i=0;i<sizeof(marketAmmoQuantitiesByGroup)/sizeof(marketAmmoQuantitiesByGroup[0]);++i)
    {
        marketAmmoQuantitiesByGroup[i]=(uint16_t)(bytes[0x02A6+i*2]|((uint16_t)bytes[0x02A6+i*2+1]<<8));
    }
    for (size_t i=0;i<sizeof(reserveAmmoQuantitiesByFamily)/sizeof(reserveAmmoQuantitiesByFamily[0]);++i)
    {
        reserveAmmoQuantitiesByFamily[i]=(uint16_t)(bytes[0x02EE+i*2]|((uint16_t)bytes[0x02EE+i*2+1]<<8));
    }
    for (size_t i=0;i<sizeof(ownedAmmoKindsByGroup)/sizeof(ownedAmmoKindsByGroup[0]);++i)
    {
        ownedAmmoKindsByGroup[i]=(uint16_t)(bytes[0x02FA+i*2]|((uint16_t)bytes[0x02FA+i*2+1]<<8));
    }
    for (size_t i=0;i<sizeof(marketAmmoKindsByGroup)/sizeof(marketAmmoKindsByGroup[0]);++i)
    {
        marketAmmoKindsByGroup[i]=(uint16_t)(bytes[0x0342+i*2]|((uint16_t)bytes[0x0342+i*2+1]<<8));
    }
    marketUnitCount=(uint16_t)(bytes[0x038A]|((uint16_t)bytes[0x038A+1]<<8));
    for (size_t i=0;i<sizeof(marketUnitKinds)/sizeof(marketUnitKinds[0]);++i)
    {
        marketUnitKinds[i]=(uint16_t)(bytes[0x038C+i*2]|((uint16_t)bytes[0x038C+i*2+1]<<8));
    }
    memcpy(marketUnitDamage,bytes+0x03A4,sizeof(marketUnitDamage));
    missionOfferCount=(uint16_t)(bytes[0x0500]|((uint16_t)bytes[0x0500+1]<<8));
    for (size_t i=0;i<sizeof(missionDescriptionOffsets)/sizeof(missionDescriptionOffsets[0]);++i)
    {
        missionDescriptionOffsets[i]=(uint16_t)(bytes[0x0502+i*2]|((uint16_t)bytes[0x0502+i*2+1]<<8));
    }
    for (size_t i=0;i<sizeof(missionPlanetRecordIndices)/sizeof(missionPlanetRecordIndices[0]);++i)
    {
        missionPlanetRecordIndices[i]=(uint16_t)(bytes[0x050E+i*2]|((uint16_t)bytes[0x050E+i*2+1]<<8));
    }
    for (size_t i=0;i<sizeof(missionTargetHouseIndices)/sizeof(missionTargetHouseIndices[0]);++i)
    {
        missionTargetHouseIndices[i]=(uint16_t)(bytes[0x051A+i*2]|((uint16_t)bytes[0x051A+i*2+1]<<8));
    }
    for (size_t i=0;i<sizeof(missionTypeCodes)/sizeof(missionTypeCodes[0]);++i)
    {
        missionTypeCodes[i]=(uint16_t)(bytes[0x0526+i*2]|((uint16_t)bytes[0x0526+i*2+1]<<8));
    }
    for (size_t i=0;i<sizeof(missionEnemyHeavyCounts)/sizeof(missionEnemyHeavyCounts[0]);++i)
    {
        missionEnemyHeavyCounts[i]=(uint16_t)(bytes[0x0532+i*2]|((uint16_t)bytes[0x0532+i*2+1]<<8));
    }
    for (size_t i=0;i<sizeof(missionEnemyMediumCounts)/sizeof(missionEnemyMediumCounts[0]);++i)
    {
        missionEnemyMediumCounts[i]=(uint16_t)(bytes[0x053E+i*2]|((uint16_t)bytes[0x053E+i*2+1]<<8));
    }
    for (size_t i=0;i<sizeof(missionEnemyLightCounts)/sizeof(missionEnemyLightCounts[0]);++i)
    {
        missionEnemyLightCounts[i]=(uint16_t)(bytes[0x054A+i*2]|((uint16_t)bytes[0x054A+i*2+1]<<8));
    }
    for (size_t i=0;i<sizeof(missionPaymentThousands)/sizeof(missionPaymentThousands[0]);++i)
    {
        missionPaymentThousands[i]=(uint16_t)(bytes[0x0556+i*2]|((uint16_t)bytes[0x0556+i*2+1]<<8));
    }
    for (size_t i=0;i<sizeof(missionSalvagePercentages)/sizeof(missionSalvagePercentages[0]);++i)
    {
        missionSalvagePercentages[i]=(uint16_t)(bytes[0x0562+i*2]|((uint16_t)bytes[0x0562+i*2+1]<<8));
    }
    for (size_t i=0;i<sizeof(missionAdvancePercentages)/sizeof(missionAdvancePercentages[0]);++i)
    {
        missionAdvancePercentages[i]=(uint16_t)(bytes[0x056E+i*2]|((uint16_t)bytes[0x056E+i*2+1]<<8));
    }
    for (size_t i=0;i<sizeof(missionPaymentCeilingsThousands)/sizeof(missionPaymentCeilingsThousands[0]);++i)
    {
        missionPaymentCeilingsThousands[i]=(uint16_t)(bytes[0x057A+i*2]|((uint16_t)bytes[0x057A+i*2+1]<<8));
    }
    for (size_t i=0;i<sizeof(missionSalvagePercentageCeilings)/sizeof(missionSalvagePercentageCeilings[0]);++i)
    {
        missionSalvagePercentageCeilings[i]=(uint16_t)(bytes[0x0586+i*2]|((uint16_t)bytes[0x0586+i*2+1]<<8));
    }
    for (size_t i=0;i<sizeof(missionAdvancePercentageCeilings)/sizeof(missionAdvancePercentageCeilings[0]);++i)
    {
        missionAdvancePercentageCeilings[i]=(uint16_t)(bytes[0x0592+i*2]|((uint16_t)bytes[0x0592+i*2+1]<<8));
    }
    contractNegotiationAttemptLimit=(uint16_t)(bytes[0x059E]|((uint16_t)bytes[0x059E+1]<<8));
    requestedContractPaymentThousands=(uint16_t)(bytes[0x05A0]|((uint16_t)bytes[0x05A0+1]<<8));
    requestedContractSalvagePercent=(uint16_t)(bytes[0x05A2]|((uint16_t)bytes[0x05A2+1]<<8));
    requestedContractAdvancePercent=(uint16_t)(bytes[0x05A4]|((uint16_t)bytes[0x05A4+1]<<8));
    requestContractPaymentEnabled=(uint16_t)(bytes[0x05A6]|((uint16_t)bytes[0x05A6+1]<<8));
    requestContractSalvageEnabled=(uint16_t)(bytes[0x05A8]|((uint16_t)bytes[0x05A8+1]<<8));
    requestContractAdvanceEnabled=(uint16_t)(bytes[0x05AA]|((uint16_t)bytes[0x05AA+1]<<8));
    memcpy(unidentifiedField20,bytes+0x05AC,sizeof(unidentifiedField20));
    acceptedContractPaymentThousands=(uint16_t)(bytes[0x05B2]|((uint16_t)bytes[0x05B2+1]<<8));
    acceptedContractSalvagePercent=(uint16_t)(bytes[0x05B4]|((uint16_t)bytes[0x05B4+1]<<8));
    acceptedContractAdvancePercent=(uint16_t)(bytes[0x05B6]|((uint16_t)bytes[0x05B6+1]<<8));
    remainingContractPaymentThousands=(uint16_t)(bytes[0x05B8]|((uint16_t)bytes[0x05B8+1]<<8));
    memcpy(houseMissionTypeLimits,bytes+0x05BA,sizeof(houseMissionTypeLimits));
    audioOptionLabelIndex=bytes[0x05E2];
    for (size_t i=0;i<sizeof(missionScenarioCodes)/sizeof(missionScenarioCodes[0]);++i)
    {
        missionScenarioCodes[i]=(uint16_t)(bytes[0x05E3+i*2]|((uint16_t)bytes[0x05E3+i*2+1]<<8));
    }
    memcpy(pilotGunneryTiers,bytes+0x05EF,sizeof(pilotGunneryTiers));
    memcpy(pilotPilotingTiers,bytes+0x061A,sizeof(pilotPilotingTiers));
    memcpy(pilotExperienceCounts,bytes+0x0645,sizeof(pilotExperienceCounts));
    eventDirectoryIndex=(uint16_t)(bytes[0x0670]|((uint16_t)bytes[0x0670+1]<<8));
    memcpy(eventSeenFlags,bytes+0x0672,sizeof(eventSeenFlags));
    for (size_t i=0;i<sizeof(campaignEventPlanetSelectors)/sizeof(campaignEventPlanetSelectors[0]);++i)
    {
        campaignEventPlanetSelectors[i]=(uint16_t)(bytes[0x06FE+i*2]|((uint16_t)bytes[0x06FE+i*2+1]<<8));
    }
    majorCampaignMissionRollThreshold=bytes[0x070A];
    acceptedContractTargetHouseIndex=(uint16_t)(bytes[0x070B]|((uint16_t)bytes[0x070B+1]<<8));
    campaignTerrainDetailLevel=bytes[0x070D];
    dealerUnitPurchaseLimit=(uint16_t)(bytes[0x070E]|((uint16_t)bytes[0x070E+1]<<8));
    unitsSoldSinceMarketRefresh=(uint16_t)(bytes[0x0710]|((uint16_t)bytes[0x0710+1]<<8));
    playerSkillProgress=(uint16_t)(bytes[0x0712]|((uint16_t)bytes[0x0712+1]<<8));
    darkWingBaseVictoryRecorded=bytes[0x0714];
    launcherMemoryTier=(uint16_t)(bytes[0x0715]|((uint16_t)bytes[0x0715+1]<<8));
    savedVideoMode=bytes[0x0717];
}
