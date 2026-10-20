#include "mechwarrior/campaign_continuations.h"
#include "mechwarrior/campaign_events.h"
#include "mechwarrior/campaign_mechanics.h"
#include "mechwarrior/campaign_state.h"
#include "mechwarrior/campaign_economy.h"
#include "mechwarrior/ega_campaign.h"
#include <stdlib.h>

/* MW_MAIN081B:61CC..621E. FF asks0EF1 to skip THIS caller's return word:
 * the page remains visible and the actual text origin reaches the choice code.
 * Without FF,6210 overwrites AX with1AA5 before the next-page test. */
MwTextPanelReturn MwShowCampaignEventText(uint16_t textId)
{
    if (textId==8) textPanelYOverride=0x0082;
    if (textId>=EVENT_TEXT_DIRECTORY_COUNT || !eventTextRecords[textId]) abort(); /* Unrepresented zero-directory alias, not a native empty-text test. */
    const uint8_t *page=eventTextRecords[textId];
    MwNativeByteView text={eventTextStorage,EVENT_TEXT_STORAGE_START,sizeof(eventTextStorage)};
    for (;;)
    {
        if (*page==EVENT_TEXT_PLACEHOLDER)
        {
            ++page;
            textPanelContinuationMode=TEXT_PANEL_MODE_DISCARD_RETURN;
        }
        const MwEventTextPageHeader *header=(const MwEventTextPageHeader *)page;
        eventTextRowCount=header->rowCount;
        page+=sizeof(*header);
        MwTextPanelReturn result=MwShowCompactTextPanel(0x000C,0x000A,0x002F,
            eventTextRowCount+3,&text,(uint16_t)(page-eventTextStorage+EVENT_TEXT_STORAGE_START));
        if (result.discardReturnWord) return result; /*100C RET bypasses6210..621E. */
        page=&eventTextStorage[result.textOffset-EVENT_TEXT_STORAGE_START+1];
        if (page>=eventTextStorage+sizeof(eventTextStorage)) abort();
        if (!*page)
        {
            result.textOffset=(uint16_t)(page-eventTextStorage+EVENT_TEXT_STORAGE_START);
            return result;
        }
    }
}

/* MW_MAIN081B:6855..6912. Native byte clear [0448,0B5D) expressed as
 * assignments to its nonoverlapping typed owners. IRQ/timer state is outside
 * this range and there are no intervening calls. Memory tier0B5D is retained. */
uint16_t MwRestartCampaignContinuation(void)
{
    if (!campaignStopMusic) abort();
    campaignStopMusic();
    campaignEventReturnContinuation=campaignEventDispatchReturnContinuation=0; /*6865/66 discard suspended native frames. */
    campaignRestartPending=0;
    for (uint16_t i=0;i<CAMPAIGN_NOTIFICATION_COUNT;++i) notificationPending[i]=0;
    unidentifiedField18=0;
    missionCategoryOrMemoryTier=0;
    deploymentLandscapeIndex=0;
    combatEnemyHeavyCount=0;
    combatEnemyMediumCount=0;
    combatEnemyLightCount=0;
    combatPlanetSelector=0;
    combatScenarioCode=0;
    deploymentTerrainDetailLevel=0;
    for (size_t i=0;i<sizeof(rosterStatus)/sizeof(rosterStatus[0]);++i) rosterStatus[i]=0;
    for (size_t i=0;i<sizeof(salvageEntries)/sizeof(salvageEntries[0]);++i) salvageEntries[i]=(CampaignSalvageEntry){0};
    returnedDisabledEnemyCount=0;
    combatTransitionResult=0;
    acceptedContractActive=0;
    currentPlanetIndex=0;
    currentPlanetLandscapeIndex=0;
    currentPlanetHouseIndex=0;
    currentPlanetMarketTier=0;
    currentPlanetFacilityDisplayFlag=0;
    currentPlanetMapPositionX=0;
    currentPlanetMapPositionY=0;
    planetMarketPremiumPercent=0;
    calendarSubmonthUnits=0;
    calendarMonthIndex=0;
    calendarYear=0;
    marketRefreshUnits=0;
    elapsedBillingMonths=0;
    for (size_t i=0;i<sizeof(commanderNameBytes)/sizeof(commanderNameBytes[0]);++i) commanderNameBytes[i]=0;
    for (size_t i=0;i<sizeof(unidentifiedField19)/sizeof(unidentifiedField19[0]);++i) unidentifiedField19[i]=0;
    commanderAgeYears=0;
    campaignFundsLow=0;
    campaignFundsHigh=0;
    for (size_t i=0;i<sizeof(houseNetStandings)/sizeof(houseNetStandings[0]);++i) houseNetStandings[i]=0;
    for (size_t i=0;i<sizeof(houseFavorPoints)/sizeof(houseFavorPoints[0]);++i) houseFavorPoints[i]=0;
    for (size_t i=0;i<sizeof(houseGrievancePoints)/sizeof(houseGrievancePoints[0]);++i) houseGrievancePoints[i]=0;
    playerCombatCredit=0;
    pilotIdentityStorage=(CampaignPilotIdentityStorage){0};
    pilotDepartureFlagTail=0;
    for (size_t i=0;i<sizeof(rosterPayTiers)/sizeof(rosterPayTiers[0]);++i) rosterPayTiers[i]=0;
    for (size_t i=0;i<sizeof(rosterGunneryTiers)/sizeof(rosterGunneryTiers[0]);++i) rosterGunneryTiers[i]=0;
    for (size_t i=0;i<sizeof(rosterPilotingTiers)/sizeof(rosterPilotingTiers[0]);++i) rosterPilotingTiers[i]=0;
    for (size_t i=0;i<sizeof(rosterUnknownWords)/sizeof(rosterUnknownWords[0]);++i) rosterUnknownWords[i]=0;
    for (size_t i=0;i<sizeof(rosterMechAssignments)/sizeof(rosterMechAssignments[0]);++i) rosterMechAssignments[i]=0;
    for (size_t i=0;i<sizeof(rosterPortraitIds)/sizeof(rosterPortraitIds[0]);++i) rosterPortraitIds[i]=0;
    dropShipMissedUpkeepPayments=0;
    unpaidPilotWageMarker=0;
    availableRecruitCount=0;
    for (size_t i=0;i<sizeof(availableRecruitIds)/sizeof(availableRecruitIds[0]);++i) availableRecruitIds[i]=0;
    ownedUnitCount=0;
    for (size_t i=0;i<sizeof(ownedUnitKinds)/sizeof(ownedUnitKinds[0]);++i) ownedUnitKinds[i]=0;
    for (size_t i=0;i<sizeof(ownedUnitDamage)/sizeof(ownedUnitDamage[0]);++i) ownedUnitDamage[i]=(CampaignUnitDamage){0};
    for (size_t i=0;i<sizeof(ownedAmmoQuantitiesByGroup)/sizeof(ownedAmmoQuantitiesByGroup[0]);++i) ownedAmmoQuantitiesByGroup[i]=0;
    for (size_t i=0;i<sizeof(marketAmmoQuantitiesByGroup)/sizeof(marketAmmoQuantitiesByGroup[0]);++i) marketAmmoQuantitiesByGroup[i]=0;
    for (size_t i=0;i<sizeof(reserveAmmoQuantitiesByFamily)/sizeof(reserveAmmoQuantitiesByFamily[0]);++i) reserveAmmoQuantitiesByFamily[i]=0;
    for (size_t i=0;i<sizeof(ownedAmmoKindsByGroup)/sizeof(ownedAmmoKindsByGroup[0]);++i) ownedAmmoKindsByGroup[i]=0;
    for (size_t i=0;i<sizeof(marketAmmoKindsByGroup)/sizeof(marketAmmoKindsByGroup[0]);++i) marketAmmoKindsByGroup[i]=0;
    marketUnitCount=0;
    for (size_t i=0;i<sizeof(marketUnitKinds)/sizeof(marketUnitKinds[0]);++i) marketUnitKinds[i]=0;
    for (size_t i=0;i<sizeof(marketUnitDamage)/sizeof(marketUnitDamage[0]);++i) marketUnitDamage[i]=(CampaignUnitDamage){0};
    missionOfferCount=0;
    for (size_t i=0;i<sizeof(missionDescriptionOffsets)/sizeof(missionDescriptionOffsets[0]);++i) missionDescriptionOffsets[i]=0;
    for (size_t i=0;i<sizeof(missionPlanetRecordIndices)/sizeof(missionPlanetRecordIndices[0]);++i) missionPlanetRecordIndices[i]=0;
    for (size_t i=0;i<sizeof(missionTargetHouseIndices)/sizeof(missionTargetHouseIndices[0]);++i) missionTargetHouseIndices[i]=0;
    for (size_t i=0;i<sizeof(missionTypeCodes)/sizeof(missionTypeCodes[0]);++i) missionTypeCodes[i]=0;
    for (size_t i=0;i<sizeof(missionEnemyHeavyCounts)/sizeof(missionEnemyHeavyCounts[0]);++i) missionEnemyHeavyCounts[i]=0;
    for (size_t i=0;i<sizeof(missionEnemyMediumCounts)/sizeof(missionEnemyMediumCounts[0]);++i) missionEnemyMediumCounts[i]=0;
    for (size_t i=0;i<sizeof(missionEnemyLightCounts)/sizeof(missionEnemyLightCounts[0]);++i) missionEnemyLightCounts[i]=0;
    for (size_t i=0;i<sizeof(missionPaymentThousands)/sizeof(missionPaymentThousands[0]);++i) missionPaymentThousands[i]=0;
    for (size_t i=0;i<sizeof(missionSalvagePercentages)/sizeof(missionSalvagePercentages[0]);++i) missionSalvagePercentages[i]=0;
    for (size_t i=0;i<sizeof(missionAdvancePercentages)/sizeof(missionAdvancePercentages[0]);++i) missionAdvancePercentages[i]=0;
    for (size_t i=0;i<sizeof(missionPaymentCeilingsThousands)/sizeof(missionPaymentCeilingsThousands[0]);++i) missionPaymentCeilingsThousands[i]=0;
    for (size_t i=0;i<sizeof(missionSalvagePercentageCeilings)/sizeof(missionSalvagePercentageCeilings[0]);++i) missionSalvagePercentageCeilings[i]=0;
    for (size_t i=0;i<sizeof(missionAdvancePercentageCeilings)/sizeof(missionAdvancePercentageCeilings[0]);++i) missionAdvancePercentageCeilings[i]=0;
    contractNegotiationAttemptLimit=0;
    requestedContractPaymentThousands=0;
    requestedContractSalvagePercent=0;
    requestedContractAdvancePercent=0;
    requestContractPaymentEnabled=0;
    requestContractSalvageEnabled=0;
    requestContractAdvanceEnabled=0;
    for (size_t i=0;i<sizeof(unidentifiedField20)/sizeof(unidentifiedField20[0]);++i) unidentifiedField20[i]=0;
    acceptedContractPaymentThousands=0;
    acceptedContractSalvagePercent=0;
    acceptedContractAdvancePercent=0;
    remainingContractPaymentThousands=0;
    for (size_t i=0;i<sizeof(houseMissionTypeLimits)/sizeof(houseMissionTypeLimits[0]);++i) houseMissionTypeLimits[i]=0;
    audioOptionLabelIndex=0;
    for (size_t i=0;i<sizeof(missionScenarioCodes)/sizeof(missionScenarioCodes[0]);++i) missionScenarioCodes[i]=0;
    for (size_t i=0;i<sizeof(pilotGunneryTiers)/sizeof(pilotGunneryTiers[0]);++i) pilotGunneryTiers[i]=0;
    for (size_t i=0;i<sizeof(pilotPilotingTiers)/sizeof(pilotPilotingTiers[0]);++i) pilotPilotingTiers[i]=0;
    for (size_t i=0;i<sizeof(pilotExperienceCounts)/sizeof(pilotExperienceCounts[0]);++i) pilotExperienceCounts[i]=0;
    eventDirectoryIndex=0;
    for (size_t i=0;i<sizeof(eventSeenFlags)/sizeof(eventSeenFlags[0]);++i) eventSeenFlags[i]=0;
    for (size_t i=0;i<sizeof(campaignEventPlanetSelectors)/sizeof(campaignEventPlanetSelectors[0]);++i) campaignEventPlanetSelectors[i]=0;
    majorCampaignMissionRollThreshold=0;
    acceptedContractTargetHouseIndex=0;
    campaignTerrainDetailLevel=0;
    dealerUnitPurchaseLimit=0;
    unitsSoldSinceMarketRefresh=0;
    playerSkillProgress=0;
    darkWingBaseVictoryRecorded=0;
    calendarSubmonthUnits=1;calendarMonthIndex=3;calendarYear=0x0BD0;
    commanderAgeYears=COMMANDER_INITIAL_AGE_YEARS;
    campaignFundsLow=(uint16_t)DECIMAL_MILLION;campaignFundsHigh=DECIMAL_MILLION>>NATIVE_WORD_BITS;
    pilotRosterCount=1;
    for (uint16_t i=0;i<CAMPAIGN_ROSTER_SLOTS;++i) rosterMechAssignments[i]=UINT16_MAX;
    for (uint16_t i=0;i<CAMPAIGN_OWNED_UNIT_SLOTS;++i)
    {ownedUnitKinds[i]=UINT16_MAX;marketUnitKinds[i]=UINT16_MAX;}
    for (size_t i=0;i<sizeof(houseMissionTypeLimits);++i)
        houseMissionTypeLimits[i]=defaultHouseMissionTypeLimits[i];
    for (size_t i=0;i<sizeof(commanderNameBytes);++i)
        commanderNameBytes[i]=campaignDefaultCommanderNameBytes[i];
    eventDirectoryIndex=1;campaignTerrainDetailLevel=2;
    return CAMPAIGN_CONTINUE_INITIALIZE_COMPANY; /* Native JMP into the same166D body, not a new procedure. */
}

/* MW_MAIN081B:6153..61BE. This style deliberately does not parse FF. Only
 * intermediate pages acknowledge input; the final page remains displayed. */
MwTextPanelReturn MwShowClassThreeEventPages(uint16_t textId)
{
    EgaSelectFont(0);
    EgaFillDisplayPageRectangle(0x001D,0x0017,0x0106,0x009A,1);
    if (textId>=EVENT_TEXT_DIRECTORY_COUNT || !eventTextRecords[textId]) abort();
    const uint8_t *page=eventTextRecords[textId];
    MwNativeByteView text={eventTextStorage,EVENT_TEXT_STORAGE_START,sizeof(eventTextStorage)};
    for (;;)
    {
        page+=sizeof(MwEventTextPageHeader);
        uint16_t cursor=EgaDrawTextWithLineSpacing(0x001F,0x0019,CAMPAIGN_GREEN,&text,(uint16_t)(page-eventTextStorage+EVENT_TEXT_STORAGE_START));
        page=&eventTextStorage[cursor-EVENT_TEXT_STORAGE_START+1];
        if (page>=eventTextStorage+sizeof(eventTextStorage)) abort();
        if (!*page)
        {
            MwTextPanelReturn result={(uint16_t)(page-eventTextStorage+EVENT_TEXT_STORAGE_START),0x1AA5,0,false};
            return result; /* Register projection; no class03 choice consumes BX. */
        }
        MwWaitForInputAcknowledgment();
        EgaFillDisplayPageRectangle(0x001D,0x0017,0x0106,0x009A,1);
    }
}

/* MW_MAIN081B:6668..66CB. No positive-selector alternative; the script BYTE
 * is complemented unconditionally. Text writes persist in the resource owner. */
void MwEventSubstitutePlanetName(const MwEventCommand *command)
{
    if (eventConditionResult==1)
    {
        uint16_t reference=(uint16_t)(int16_t)command->operand.planetSelector^UINT16_MAX;
        if (reference>=6 || currentEventId>=EVENT_TEXT_DIRECTORY_COUNT
            || !eventTextRecords[currentEventId]) abort();
        uint16_t planetIndex=campaignEventPlanetSelectors[reference]-1;
        if (planetIndex>=CAMPAIGN_PLANET_COUNT) abort();
        uint16_t planetOffset=planetRecordOffsets[planetIndex];
        if (planetOffset<PLANET_RECORDS_START || planetOffset+PLANET_NAME_HEADER_BYTES>=PLANET_RECORDS_END) abort();
        const uint8_t *planetName=&planetRecordStorage[planetOffset-PLANET_RECORDS_START+PLANET_NAME_HEADER_BYTES];
        uint8_t *write=eventTextRecords[currentEventId]+2;
        while (*write!=EVENT_TEXT_PLACEHOLDER)
        {
            if (++write==eventTextStorage+sizeof(eventTextStorage)) abort();
        }
        ++write;
        uint16_t padding=EVENT_PLANET_NAME_FIELD_BYTES;
        while (*planetName)
        {
            if (write==eventTextStorage+sizeof(eventTextStorage)
                || planetName==planetRecordStorage+sizeof(planetRecordStorage)) abort();
            *write++=*planetName++;
            --padding;
        }
        do /* Native LOOP executes even with CX0. No bounded-copy rewrite. */
        {
            if (write==eventTextStorage+sizeof(eventTextStorage)) abort(); /* Unrepresented wrap into the encoded directory, not a native length clamp. */
            *write++=EVENT_TEXT_PLACEHOLDER;
        } while (--padding!=0);
    }
    eventOperandCursor=command->next;
}

/* MW_MAIN081B:66CC..67C4. Returned code offsets are nonlocal JMP transport,
 * not native AX results. The skipped interpreter call remains suspended. */
uint16_t MwEventOpenChoiceContinuation(const MwEventCommand *command)
{
    if (eventConditionResult!=1)
    {eventOperandCursor=command->next;return 0;}
    eventChoiceCount=command->operand.choices.count;
    MwUiRecord *records=eventChoiceCount==2?eventTwoChoiceRecords:eventThreeChoiceRecords;
    uint16_t choices=eventChoiceCount==2?2:3;
    for (uint16_t i=0;i<choices;++i)
    {
        MwEventCaption *caption=command->operand.choices.captions[i];
        if (!caption) abort();
        records[i].text=caption->text;
        records[i].textColour=&caption->colour;
    }
    for (uint16_t i=0;i<choices;++i) eventChoiceBranches[i]=command->operand.choices.branches[i];
    if (currentEventId!=EVENT_DARK_WING_BASE_ATTACK)
    {
        if (currentEventId>=CAMPAIGN_EVENT_SEEN_FLAGS) abort();
        eventSeenFlags[currentEventId]=1;
    }
    if (campaignEventClass>=7 || !eventClassHandlers[campaignEventClass]) abort();
    MwTextPanelReturn display=eventClassHandlers[campaignEventClass](currentEventId);
    /* Place choices relative to the text panel on either return path. */
    uint16_t x=display.textOriginX+0x000C;
    for (uint16_t i=0;i<2;++i) eventTwoChoiceRecords[i].x=x;
    for (uint16_t i=0;i<3;++i) eventThreeChoiceRecords[i].x=x;
    uint16_t y=(uint8_t)(eventTextRowCount-eventChoiceCount)*EVENT_CHOICE_ROW_PIXELS+display.textOriginY;
    eventTwoChoiceRecords[0].y=eventThreeChoiceRecords[0].y=y;
    y+=EVENT_CHOICE_ROW_PIXELS;
    eventTwoChoiceRecords[1].y=eventThreeChoiceRecords[1].y=y;
    y+=EVENT_CHOICE_ROW_PIXELS;eventThreeChoiceRecords[2].y=y;
    eventChoiceSavedFontIndex=EgaGetFontIndex();
    EgaSelectFont(0);
    MwSelectUiRecordTable(0,eventChoiceCount==2?&eventTwoChoiceMenu:&eventThreeChoiceMenu);
    return CAMPAIGN_CONTINUE_UI;
}

/* MW_MAIN081B:67C5..67F7.67D8 remains a shared label, not a Support method. */
uint16_t MwEventFirstChoiceCallback(void)
{
    EgaSelectFont(eventChoiceSavedFontIndex);
    MwRestoreSavedTextPanelRegion();
    const MwCampaignEvent *branch=eventChoiceBranches[0];
    if (campaignRestartPending==1) return CAMPAIGN_CONTINUE_RESTART;
    if (currentEventId==EVENT_DARK_WING_BASE_ATTACK) return CAMPAIGN_CONTINUE_EVENT_COMBAT;
    if (!campaignEventReturnContinuation) abort(); /* Unrepresented caller frame, not an ordinary UI RET. */
    if (currentEventId==EVENT_CONTINUE_OR_QUIT) return campaignEventReturnContinuation;
    if (!branch) abort(); /* Encoded DS0000 branch is unrepresented, not success. */
    currentEventId=branch->eventId;
    bool carry;
    uint16_t transfer=MwInterpretCampaignEvent(branch,&carry);
    return transfer?transfer:campaignEventReturnContinuation;
}

/* MW_MAIN081B:67F8..6827. Restart quit is tested before panel close; the
 * shared67D8 comparisons are reread AFTER close for the ordinary branch. */
uint16_t MwEventSecondChoiceCallback(void)
{
    EgaSelectFont(eventChoiceSavedFontIndex);
    if (campaignRestartPending==1) return CAMPAIGN_CONTINUE_EVENT_QUIT;
    MwRestoreSavedTextPanelRegion();
    if (currentEventId==EVENT_DARK_WING_BASE_ATTACK) return CAMPAIGN_CONTINUE_EVENT_COMBAT_RETURN;
    if (currentEventId==EVENT_CONTINUE_OR_QUIT) return CAMPAIGN_CONTINUE_EVENT_QUIT;
    const MwCampaignEvent *branch=eventChoiceBranches[1];
    if (campaignRestartPending==1) return CAMPAIGN_CONTINUE_RESTART;
    if (currentEventId==EVENT_DARK_WING_BASE_ATTACK) return CAMPAIGN_CONTINUE_EVENT_COMBAT;
    if (!campaignEventReturnContinuation) abort();
    if (currentEventId==EVENT_CONTINUE_OR_QUIT) return campaignEventReturnContinuation;
    if (!branch) abort();
    currentEventId=branch->eventId;
    bool carry;
    uint16_t transfer=MwInterpretCampaignEvent(branch,&carry);
    return transfer?transfer:campaignEventReturnContinuation;
}

/* MW_MAIN081B:6828..683C, JMP67D8. No invented count guard: branch2 can be
 * stale after a two-choice dialog, just as the original stored WORD can. */
uint16_t MwEventThirdChoiceCallback(void)
{
    EgaSelectFont(eventChoiceSavedFontIndex);
    MwRestoreSavedTextPanelRegion();
    const MwCampaignEvent *branch=eventChoiceBranches[2];
    if (campaignRestartPending==1) return CAMPAIGN_CONTINUE_RESTART;
    if (currentEventId==EVENT_DARK_WING_BASE_ATTACK) return CAMPAIGN_CONTINUE_EVENT_COMBAT;
    if (!campaignEventReturnContinuation) abort();
    if (currentEventId==EVENT_CONTINUE_OR_QUIT) return campaignEventReturnContinuation;
    if (!branch) abort();
    currentEventId=branch->eventId;
    bool carry;
    uint16_t transfer=MwInterpretCampaignEvent(branch,&carry);
    return transfer?transfer:campaignEventReturnContinuation;
}

/* MW_MAIN081B:683D..6854; shared015F cleanup follows the explicit music stop. */
uint16_t MwQuitFromEventContinuation(void)
{
    if (!campaignStopMusic) abort();
    campaignStopMusic();
    launcherExitCode=CAMPAIGN_EXIT_QUIT;
    return CAMPAIGN_CONTINUE_CLEANUP;
}

/* MW_MAIN081B:6913..692A. INC BL wraps independently of the planet WORD. */
uint16_t MwPrepareEventSpecialCombatContinuation(void)
{
    combatPlanetSelector=(uint8_t)(currentPlanetIndex+1);
    deploymentLandscapeIndex=(uint8_t)currentPlanetLandscapeIndex;
    combatScenarioCode=COMBAT_SCENARIO_DARK_WING_BASE;
    return CAMPAIGN_CONTINUE_EVENT_DEPLOYMENT;
}
