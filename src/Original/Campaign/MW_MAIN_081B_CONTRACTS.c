#include "mechwarrior/port_diagnostics.h"
#include "mechwarrior/campaign_continuations.h"
#include "mechwarrior/campaign_contracts.h"
#include "mechwarrior/campaign_events.h"
#include "mechwarrior/campaign_input.h"
#include "mechwarrior/ega_campaign.h"
#include <stdlib.h>
#include <string.h>

/* MW_MAIN081B:2D63..2DA5. Genuine shared framed-background procedure. */
void MwDrawCampaignFramedBackground(void)
{
    EgaClearDrawPage(CONTRACT_PAGE_BACKGROUND);
    MwDrawGeneralPicture(0,0,EGA_DRAW_PAGE,CONTRACT_FRAME_TOP_PICTURE);
    MwDrawGeneralPicture(0,CONTRACT_FRAME_BOTTOM_Y,EGA_DRAW_PAGE,CONTRACT_FRAME_BOTTOM_PICTURE);
    MwDrawGeneralPicture(0,CONTRACT_FRAME_SIDE_Y,EGA_DRAW_PAGE,CONTRACT_FRAME_LEFT_PICTURE);
    MwDrawGeneralPicture(CONTRACT_FRAME_RIGHT_X,CONTRACT_FRAME_SIDE_Y,EGA_DRAW_PAGE,CONTRACT_FRAME_RIGHT_PICTURE);
}

/* MW_MAIN081B:4AA2..4B54.4AA8 POPs inherited UI return;4B43 resumes
 * AFTER6236. No extra menu loop or repeated time/event charge on resumption. */
uint16_t MwEnterContractOfficeContinuation(uint16_t entryOffset)
{
    if (entryOffset==CAMPAIGN_CONTINUE_CONTRACT_OFFICE_MENU) goto officeMenu;
    if (entryOffset==CAMPAIGN_CONTINUE_CONTRACT_OFFICE) MwAdvanceCampaignTimeAndChargeWages(1);
    else if (entryOffset!=CAMPAIGN_CONTINUE_CONTRACT_OFFICE_REDRAW) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: entryOffset!=CAMPAIGN_CONTINUE_CONTRACT_OFFICE_REDRAW");
    hubSelectedRecordIndex=CONTRACT_OFFICE_HUB_ROW;
    EgaSelectFont(CONTRACT_OFFICE_FONT);
    MwLoadIndexedPicture((uint16_t)(CONTRACT_OFFICE_EMBLEM_BASE+currentPlanetHouseIndex));
    MwDrawCampaignFramedBackground();
    MwDrawIndexedPicture(CONTRACT_OFFICE_EMBLEM_X,CONTRACT_OFFICE_EMBLEM_Y,(uint16_t)(CONTRACT_OFFICE_EMBLEM_BASE+currentPlanetHouseIndex));
    MwDrawIndexedPicture(CONTRACT_OFFICE_PORTRAIT_X,CONTRACT_OFFICE_PORTRAIT_Y,(uint16_t)(CONTRACT_OFFICE_PORTRAIT_BASE+currentPlanetHouseIndex));
    /* Native MUL BL uses AL only; the resulting picture base survives the scan. */
    uint16_t regionPicture=CONTRACT_OFFICE_REGION_BASE+(uint8_t)currentPlanetHouseIndex*CONTRACT_OFFICE_REGIONS_PER_HOUSE;
    if (currentPlanetHouseIndex>=CAMPAIGN_HOUSE_COUNT) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: currentPlanetHouseIndex>=CAMPAIGN_HOUSE_COUNT"); /* Unrepresented authored list. */
    const uint8_t *planetGroup=contractOfficePlanetGroups[currentPlanetHouseIndex];
    uint16_t groupIndex=0;
    uint8_t planetSelector=currentPlanetIndex+1;
    for (;;)
    {
        if (*planetGroup==CONTRACT_REGION_SEPARATOR) ++groupIndex;
        else if (*planetGroup==CONTRACT_REGION_END || *planetGroup==planetSelector) break;
        ++planetGroup;
    }
    MwDrawIndexedPicture(CONTRACT_OFFICE_REGION_X,CONTRACT_OFFICE_EMBLEM_Y,(uint16_t)(regionPicture+groupIndex));
    EgaCopyDrawPageToDisplay(0,0,EGA_COLUMNS,EGA_ROWS);
    campaignEventDispatchReturnContinuation=CAMPAIGN_CONTINUE_CONTRACT_OFFICE_MENU;
    uint16_t transfer=MwDispatchCampaignEvents(CONTRACT_OFFICE_EVENT_CLASS,CAMPAIGN_CALL_DISPATCH_EVENTS);
    if (transfer) return transfer;
    campaignEventDispatchReturnContinuation=0;
officeMenu:
    MwDrawDescriptorTextPanel(&contractOfficePanel);
    MwSelectUiRecordTable(0,&contractOfficeMenu);
    return CAMPAIGN_CONTINUE_UI;
}

/* MW_MAIN081B:4C87..4CC7. SI is unscaled slot; requests themselves survive. */
void MwInitializeContractRequestDisplay(uint16_t missionOfferSlot)
{
    contractRequestEditorInitialized=0;
    requestContractPaymentEnabled=0;
    requestContractSalvageEnabled=0;
    requestContractAdvanceEnabled=0;
    for (uint16_t byte=0;byte<sizeof(unidentifiedField20);++byte) unidentifiedField20[byte]=0;
    negotiatingMissionOfferByteOffset=(uint16_t)(missionOfferSlot<<1);
    for (uint16_t byte=0;byte<CONTRACT_ACCEPT_CAPTION_BYTES;++byte)
        contractActionLabels[0].text[byte]=acceptContractCaption[byte];
}
/* MW_MAIN081B:4CC8..4D1C. Reload the live offer displacement before each draw. */
void MwDrawNegotiatingOfferTerms(void)
{
    if ((negotiatingMissionOfferByteOffset&1) || negotiatingMissionOfferByteOffset/2>=MISSION_OFFER_SLOTS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: (negotiatingMissionOfferByteOffset&1) || negotiatingMissionOfferByteOffset/2>=MISSION_OFFER_SLOTS");
    MwDrawSignedDecimalField(missionPaymentThousands[negotiatingMissionOfferByteOffset/2],
        CONTRACT_PAYMENT_X,0,CONTRACT_PAYMENT_DIGITS,CONTRACT_TERM_COLOUR,CONTRACT_PAGE_BACKGROUND,CONTRACT_PAYMENT_Y);
    MwDrawSignedDecimalField(missionSalvagePercentages[negotiatingMissionOfferByteOffset/2],
        CONTRACT_SALVAGE_X,0,CONTRACT_PERCENT_DIGITS,CONTRACT_TERM_COLOUR,CONTRACT_PAGE_BACKGROUND,CONTRACT_SALVAGE_Y);
    MwDrawSignedDecimalField(missionAdvancePercentages[negotiatingMissionOfferByteOffset/2],
        CONTRACT_ADVANCE_X,0,CONTRACT_PERCENT_DIGITS,CONTRACT_TERM_COLOUR,CONTRACT_PAGE_BACKGROUND,CONTRACT_ADVANCE_Y);
}
/* MW_MAIN081B:4D1D..4D28 /4D29..4D34. Text is native DS:DI, not SI. */
void MwDrawContractHeading(uint16_t x,uint16_t y,const char *text)
{
    MwNativeByteView view={(const uint8_t *)text,0,(uint32_t)strlen(text)+1};
    EgaDrawTransparentText(x,y,CONTRACT_HEADING_COLOUR,&view,0);
}
void MwDrawContractText(uint16_t x,uint16_t y,const char *text)
{
    MwNativeByteView view={(const uint8_t *)text,0,(uint32_t)strlen(text)+1};
    EgaDrawTransparentText(x,y,CONTRACT_TEXT_COLOUR,&view,0);
}

/* MW_MAIN081B:4B55..4C86. Callback POP then shared briefing rendering.
 * SI offer slot is a real native register input; returned WORD is JMP transport. */
uint16_t MwDrawMissionContractBriefingContinuation(uint16_t missionOfferSlot)
{
    MwInitializeContractRequestDisplay(missionOfferSlot);
    MwDrawCampaignFramedBackground();
    EgaCopyDrawPageToDisplay(0,0,EGA_COLUMNS,EGA_ROWS);
    MwDrawDescriptorTextPanel(&contractActionsPanel);
    MwDrawContractHeading(CONTRACT_TEXT_X,CONTRACT_NARRATIVE_Y,contractNarrative);
    if (currentPlanetHouseIndex>=CAMPAIGN_HOUSE_COUNT) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: currentPlanetHouseIndex>=CAMPAIGN_HOUSE_COUNT");
    const char *employer=houseContractNames[currentPlanetHouseIndex];
    MwDrawContractText(CONTRACT_EMPLOYER_X,CONTRACT_EMPLOYER_Y,employer);
    MwDrawContractText(CONTRACT_TEXT_X,CONTRACT_PAYMENT_Y,employer);
    uint16_t offerSlot=negotiatingMissionOfferByteOffset/2;
    if ((negotiatingMissionOfferByteOffset&1) || offerSlot>=MISSION_OFFER_SLOTS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: (negotiatingMissionOfferByteOffset&1) || offerSlot>=MISSION_OFFER_SLOTS");
    uint16_t description=missionDescriptionOffsets[offerSlot];
    if (description<MISSION_DESCRIPTION_STORAGE_START || (uint32_t)description+2>=MISSION_DESCRIPTION_STORAGE_END) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: description<MISSION_DESCRIPTION_STORAGE_START || (uint32_t)description+2>=MISSION_DESCRIPTION_STORAGE_END");
    MwDrawContractText(CONTRACT_TEXT_X,CONTRACT_OBJECTIVE_Y,(const char *)&missionDescriptionStorage[description+sizeof(CampaignMissionDescriptionHeader)-MISSION_DESCRIPTION_STORAGE_START]);
    offerSlot=negotiatingMissionOfferByteOffset/2;
    uint16_t planetIndex=missionPlanetRecordIndices[offerSlot];
    if (planetIndex>=sizeof(planetRecordOffsets)/sizeof(planetRecordOffsets[0])) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: planetIndex>=sizeof(planetRecordOffsets)/sizeof(planetRecordOffsets[0])");
    uint16_t planetOffset=planetRecordOffsets[planetIndex];
    if (planetOffset<PLANET_RECORDS_START || (uint32_t)planetOffset+sizeof(MwPlanetHeader)>=PLANET_RECORDS_END) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: planetOffset<PLANET_RECORDS_START || (uint32_t)planetOffset+sizeof(MwPlanetHeader)>=PLANET_RECORDS_END");
    MwDrawContractText(CONTRACT_TEXT_X,CONTRACT_DESTINATION_Y,(const char *)&planetRecordStorage[planetOffset+sizeof(MwPlanetHeader)-PLANET_RECORDS_START]);
    offerSlot=negotiatingMissionOfferByteOffset/2;
    uint16_t house=missionTypeCodes[offerSlot]==MISSION_CATEGORY_DEFENCE || missionTargetHouseIndices[offerSlot]>=CAMPAIGN_HOUSE_COUNT
        ?currentPlanetHouseIndex:missionTargetHouseIndices[offerSlot];
    MwDrawContractText(CONTRACT_TARGET_HOUSE_X,CONTRACT_TARGET_HOUSE_Y,houseContractNames[house]);
    MwDrawSignedDecimalField(missionEnemyHeavyCounts[negotiatingMissionOfferByteOffset/2],CONTRACT_HEAVY_COUNT_X,1,0,CONTRACT_TEXT_COLOUR,CONTRACT_PAGE_BACKGROUND,CONTRACT_FORCE_COUNTS_Y);
    MwDrawSignedDecimalField(missionEnemyMediumCounts[negotiatingMissionOfferByteOffset/2],CONTRACT_MEDIUM_COUNT_X,1,0,CONTRACT_TEXT_COLOUR,CONTRACT_PAGE_BACKGROUND,CONTRACT_FORCE_COUNTS_Y);
    MwDrawSignedDecimalField(missionEnemyLightCounts[negotiatingMissionOfferByteOffset/2],CONTRACT_TEXT_X,1,0,CONTRACT_TEXT_COLOUR,CONTRACT_PAGE_BACKGROUND,CONTRACT_LIGHT_COUNT_Y);
    MwDrawNegotiatingOfferTerms();
    if (calendarMonthIndex>=CAMPAIGN_MONTH_COUNT) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: calendarMonthIndex>=CAMPAIGN_MONTH_COUNT");
    uint16_t yearX=contractDateYearXOffsets[calendarMonthIndex];
    MwDrawContractText(CONTRACT_MONTH_X,CONTRACT_DATE_Y,(const char *)calendarMonthNames[calendarMonthIndex]);
    MwDrawSignedDecimalField(calendarYear,(uint16_t)(yearX+CONTRACT_YEAR_BASE_X),1,0,CONTRACT_TEXT_COLOUR,CONTRACT_PAGE_BACKGROUND,CONTRACT_DATE_Y);
    MwSelectUiRecordTable(1,&contractActionsMenu);
    return CAMPAIGN_CONTINUE_UI;
}

/* MW_MAIN081B:4D35..4DA4. No reset of attempts or requested terms here. */
uint16_t MwRequestMissionOfferContinuation(void)
{
    if (currentPlanetHouseIndex>=CAMPAIGN_HOUSE_COUNT) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: currentPlanetHouseIndex>=CAMPAIGN_HOUSE_COUNT");
    if (houseGrievancePoints[currentPlanetHouseIndex]>=HOUSE_GRIEVANCE_REFUSAL)
    {
        MwDrawCampaignFramedBackground();
        EgaCopyDrawPageToDisplay(0,0,EGA_COLUMNS,EGA_ROWS);
        MwDrawContractHeading(CONTRACT_TEXT_X,CONTRACT_NARRATIVE_Y,houseContractRefusalText);
        MwWaitForInputAcknowledgment();
        return CAMPAIGN_CONTINUE_CONTRACT_OFFICE_REDRAW;
    }
    if (missionOfferCount!=0)
    {
        negotiatingMissionOfferSlot=0;
        return CAMPAIGN_CONTINUE_MISSION_BRIEFING;
    }
    MwDrawCampaignFramedBackground();
    EgaCopyDrawPageToDisplay(0,0,EGA_COLUMNS,EGA_ROWS);
    MwDrawContractHeading(CONTRACT_TEXT_X,CONTRACT_NARRATIVE_Y,noContractsText);
    MwWaitForInputAcknowledgment();
    return CAMPAIGN_CONTINUE_CONTRACT_OFFICE_REDRAW;
}
/* MW_MAIN081B:4DA5..4DE5. NEXT off the end goes to office WITHOUT clearing. */
uint16_t MwNextMissionOfferContinuation(void)
{
    uint16_t nextOffer=negotiatingMissionOfferSlot+1;
    if (nextOffer>=missionOfferCount) return CAMPAIGN_CONTINUE_CONTRACT_OFFICE_REDRAW;
    requestedContractPaymentThousands=0;
    requestedContractSalvagePercent=0;
    requestedContractAdvancePercent=0;
    requestContractPaymentEnabled=0;
    requestContractSalvageEnabled=0;
    requestContractAdvanceEnabled=0;
    contractNegotiationAttempts=0;
    negotiatingMissionOfferSlot=nextOffer;
    return CAMPAIGN_CONTINUE_MISSION_BRIEFING;
}

/* MW_MAIN081B:4F25..4F6D. SI is BYTE displacement00/02/04. The apparent
 * fill setup4F26..4F38 has NO CALL: do not invent a rectangle clear here. */
void MwRedrawRequestedContractTerm(uint16_t termByteOffset)
{
    uint16_t term=termByteOffset/sizeof(uint16_t);
    if ((termByteOffset&1) || term>=CONTRACT_TERM_COUNT) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: (termByteOffset&1) || term>=CONTRACT_TERM_COUNT");
    uint16_t requestedAmount=term==CONTRACT_PAYMENT_TERM?requestedContractPaymentThousands:
        term==CONTRACT_SALVAGE_TERM?requestedContractSalvagePercent:requestedContractAdvancePercent;
    uint16_t x=requestedTermX[term],y=requestedTermY[term];
    MwDrawSignedDecimalField(requestedAmount,x,0,(uint8_t)requestedTermDigits[term],
        contractRequestTextColour,CONTRACT_PAGE_BACKGROUND,y);
    EgaCopyDisplayToDrawPage(x,y,CONTRACT_TERM_COPY_WIDTH,CONTRACT_TERM_HEIGHT);
}

/* MW_MAIN081B:4DE6..4EC7. Shared editor frame; zero means native RET,
 *015F means discard the callback return and enter native campaign shutdown. */
uint16_t MwEditContractRequestContinuation(uint16_t termByteOffset)
{
    contractRequestTermByteOffset=termByteOffset;
    if (contractRequestEditorInitialized!=1)
    {
        contractRequestEditorInitialized=1;
        EgaFillDisplayPageRectangle(CONTRACT_EDITOR_PROMPT_X,CONTRACT_EDITOR_PROMPT_Y,
            CONTRACT_EDITOR_PROMPT_WIDTH,CONTRACT_TERM_HEIGHT,CONTRACT_PROMPT_BACKGROUND);
        for (uint16_t byte=0;byte<CONTRACT_ACCEPT_CAPTION_BYTES;++byte)
            contractActionLabels[0].text[byte]=submitContractCaption[byte];
        MwDrawContractText(CONTRACT_EDITOR_PROMPT_X,CONTRACT_EDITOR_PROMPT_Y,contractActionLabels[0].text);
    }
    contractRequestTextColour=CONTRACT_EDITING_COLOUR;
    MwRedrawRequestedContractTerm(contractRequestTermByteOffset);
    for (;;)
    {
        uint16_t key;
        if (!MwPollNavigationKeyFromMouseOrJoystick(&key))
        {
            if (!mwCampaignInput.keyboardAvailable || !mwCampaignInput.readKeyboard) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !mwCampaignInput.keyboardAvailable || !mwCampaignInput.readKeyboard");
            if (!mwCampaignInput.keyboardAvailable()) continue;
            key=mwCampaignInput.readKeyboard();
        }
        if (key==CAMPAIGN_KEY_ESCAPE)
        {
            if (!mwCampaignInput.readKeyboardShiftFlags) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !mwCampaignInput.readKeyboardShiftFlags");
            if (mwCampaignInput.readKeyboardShiftFlags()&BIOS_SHIFT_CONTROL) return CAMPAIGN_CONTINUE_CLEANUP;
            continue;
        }
        uint16_t term=contractRequestTermByteOffset/sizeof(uint16_t);
        if ((contractRequestTermByteOffset&1) || term>=CONTRACT_TERM_COUNT) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: (contractRequestTermByteOffset&1) || term>=CONTRACT_TERM_COUNT");
        uint16_t *request=term==CONTRACT_PAYMENT_TERM?&requestedContractPaymentThousands:
            term==CONTRACT_SALVAGE_TERM?&requestedContractSalvagePercent:&requestedContractAdvancePercent;
        if (key==CAMPAIGN_KEY_UP)
        {
            if (term==CONTRACT_PAYMENT_TERM)
            {
                uint16_t increasedPayment=*request+CONTRACT_PAYMENT_STEP;
                if ((int16_t)increasedPayment>CONTRACT_PAYMENT_EDITOR_MAXIMUM) continue;
                *request=increasedPayment;
            }
            else {if (*request==CONTRACT_PERCENT_MAXIMUM) continue;++*request;}
        }
        else if (key==CAMPAIGN_KEY_DOWN)
        {
            if (term==CONTRACT_PAYMENT_TERM)
            {
                uint16_t reducedPayment=*request-CONTRACT_PAYMENT_STEP;
                if ((int16_t)reducedPayment<0) continue;
                *request=reducedPayment;
            }
            else {if (*request==0) continue;--*request;}
        }
        else if (key==CAMPAIGN_KEY_ENTER)
        {
            contractRequestTextColour=CONTRACT_TERM_COLOUR;
            MwRedrawRequestedContractTerm(contractRequestTermByteOffset);
            return 0;
        }
        else continue;
        MwRedrawRequestedContractTerm(contractRequestTermByteOffset);
    }
}
/* MW_MAIN081B:4EC8..4EE6,4EE7..4F05,4F06..4F24; shared4EE4 is a JMP,
 *not a fourth callable helper. Exact enable==1 alone preserves the old request. */
uint16_t MwEditContractPaymentRequestContinuation(void)
{
    if (requestContractPaymentEnabled!=1)
    {
        if ((negotiatingMissionOfferByteOffset&1) || negotiatingMissionOfferByteOffset/2>=MISSION_OFFER_SLOTS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: (negotiatingMissionOfferByteOffset&1) || negotiatingMissionOfferByteOffset/2>=MISSION_OFFER_SLOTS");
        requestedContractPaymentThousands=missionPaymentThousands[negotiatingMissionOfferByteOffset/2];
        requestContractPaymentEnabled=1;
    }
    return MwEditContractRequestContinuation(CONTRACT_PAYMENT_TERM*sizeof(uint16_t));
}
uint16_t MwEditContractSalvageRequestContinuation(void)
{
    if (requestContractSalvageEnabled!=1)
    {
        if ((negotiatingMissionOfferByteOffset&1) || negotiatingMissionOfferByteOffset/2>=MISSION_OFFER_SLOTS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: (negotiatingMissionOfferByteOffset&1) || negotiatingMissionOfferByteOffset/2>=MISSION_OFFER_SLOTS");
        requestedContractSalvagePercent=missionSalvagePercentages[negotiatingMissionOfferByteOffset/2];
        requestContractSalvageEnabled=1;
    }
    return MwEditContractRequestContinuation(CONTRACT_SALVAGE_TERM*sizeof(uint16_t));
}
uint16_t MwEditContractAdvanceRequestContinuation(void)
{
    if (requestContractAdvanceEnabled!=1)
    {
        if ((negotiatingMissionOfferByteOffset&1) || negotiatingMissionOfferByteOffset/2>=MISSION_OFFER_SLOTS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: (negotiatingMissionOfferByteOffset&1) || negotiatingMissionOfferByteOffset/2>=MISSION_OFFER_SLOTS");
        requestedContractAdvancePercent=missionAdvancePercentages[negotiatingMissionOfferByteOffset/2];
        requestContractAdvanceEnabled=1;
    }
    return MwEditContractRequestContinuation(CONTRACT_ADVANCE_TERM*sizeof(uint16_t));
}

/* MW_MAIN081B:536A..55A4;5516 acceptance is an inline shared branch.
 * Later terms depend on the earlier satisfaction flag, not independent rolls. */
uint16_t MwNegotiateAndAcceptContractContinuation(void)
{
    if (currentPlanetHouseIndex==HOUSE_STEINER || currentPlanetHouseIndex==HOUSE_DAVION
        || contractNegotiationAttempts<contractNegotiationAttemptLimit)
        ++contractNegotiationAttempts;
    negotiationRequestsSatisfied=1;
    uint16_t offerSlot=(uint16_t)(negotiatingMissionOfferSlot<<1)/sizeof(uint16_t);
    if (offerSlot>=MISSION_OFFER_SLOTS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: offerSlot>=MISSION_OFFER_SLOTS");
    if (requestContractPaymentEnabled!=0)
    {
        if (requestedContractPaymentThousands>missionPaymentThousands[offerSlot])
        {
            negotiationRequestsSatisfied=0;
            uint16_t headroom=missionPaymentCeilingsThousands[offerSlot]-missionPaymentThousands[offerSlot];
            uint16_t concession=MwRandomUpTo(headroom>>1);
            concession+=missionPaymentThousands[offerSlot]; /* Native re-read after RNG. */
            concession=(uint16_t)(concession/CONTRACT_PAYMENT_STEP*CONTRACT_PAYMENT_STEP);
            if (concession<requestedContractPaymentThousands)
            {missionPaymentThousands[offerSlot]=concession;goto salvageRequest;}
        }
        missionPaymentThousands[offerSlot]=requestedContractPaymentThousands;
        negotiationRequestsSatisfied=1;
    }
salvageRequest:
    if (requestContractSalvageEnabled!=0)
    {
        if (requestedContractSalvagePercent>missionSalvagePercentages[offerSlot])
        {
            uint16_t headroom=missionSalvagePercentageCeilings[offerSlot]-missionSalvagePercentages[offerSlot];
            uint16_t concession=MwRandomUpTo(headroom>>1);
            concession+=missionSalvagePercentages[offerSlot];
            if (concession<requestedContractSalvagePercent)
            {
                negotiationRequestsSatisfied=0;
                missionSalvagePercentages[offerSlot]=concession;
                goto advanceRequest;
            }
        }
        if (negotiationRequestsSatisfied!=0)
        {
            missionSalvagePercentages[offerSlot]=requestedContractSalvagePercent;
            negotiationRequestsSatisfied=1;
        }
    }
advanceRequest:
    if (requestContractAdvanceEnabled!=0)
    {
        if (requestedContractAdvancePercent>missionAdvancePercentages[offerSlot])
        {
            uint16_t headroom=missionAdvancePercentageCeilings[offerSlot]-missionAdvancePercentages[offerSlot];
            uint16_t concession=MwRandomUpTo(headroom>>1);
            concession+=missionAdvancePercentages[offerSlot];
            if (concession<requestedContractAdvancePercent)
            {
                missionAdvancePercentages[offerSlot]=concession;
                negotiationRequestsSatisfied=0;
                goto negotiationResult;
            }
        }
        if (negotiationRequestsSatisfied!=0)
        {
            missionAdvancePercentages[offerSlot]=requestedContractAdvancePercent;
            negotiationRequestsSatisfied=1;
        }
    }
negotiationResult:
    if (negotiationRequestsSatisfied!=0) goto acceptContract;
    requestedContractPaymentThousands=0;
    requestedContractSalvagePercent=0;
    requestedContractAdvancePercent=0;
    requestContractPaymentEnabled=0;
    requestContractSalvageEnabled=0;
    requestContractAdvanceEnabled=0;
    contractRequestEditorInitialized=0;
    if (currentPlanetHouseIndex!=HOUSE_STEINER && currentPlanetHouseIndex!=HOUSE_DAVION
        && contractNegotiationAttempts>=contractNegotiationAttemptLimit)
    {
        missionOfferCount=0;
        MwDrawCampaignFramedBackground();
        EgaCopyDrawPageToDisplay(0,0,EGA_COLUMNS,EGA_ROWS);
        MwDrawContractHeading(CONTRACT_TEXT_X,CONTRACT_NARRATIVE_Y,negotiationTerminatedText);
        MwWaitForInputAcknowledgment();
        return CAMPAIGN_CONTINUE_CONTRACT_OFFICE_REDRAW;
    }
    MwInitializeContractRequestDisplay(negotiatingMissionOfferSlot);
    MwDrawNegotiatingOfferTerms();
    MwDrawDescriptorTextPanel(&contractActionsPanel);
    /*54F3 POPs inherited UI return before clearing the editing prompt. */
    EgaFillDisplayPageRectangle(CONTRACT_EDITOR_PROMPT_X,CONTRACT_EDITOR_PROMPT_Y,
        CONTRACT_EDITOR_PROMPT_WIDTH,CONTRACT_TERM_HEIGHT,CONTRACT_PROMPT_BACKGROUND);
    MwSelectUiRecordTable(0,&contractActionsMenu);
    return CAMPAIGN_CONTINUE_UI;
acceptContract:
    /*5516 saves SI over the background/text calls, then restores the offer. */
    MwDrawCampaignFramedBackground();
    EgaCopyDrawPageToDisplay(0,0,EGA_COLUMNS,EGA_ROWS);
    MwDrawContractHeading(CONTRACT_ACCEPTED_MESSAGE_X,CONTRACT_ACCEPTED_MESSAGE_Y,contractAcceptedText);
    acceptedContractActive=1;
    acceptedContractAdvancePercent=missionAdvancePercentages[offerSlot];
    acceptedContractSalvagePercent=missionSalvagePercentages[offerSlot];
    acceptedContractPaymentThousands=missionPaymentThousands[offerSlot];
    missionCategoryOrMemoryTier=(uint8_t)missionTypeCodes[offerSlot];
    combatEnemyHeavyCount=(uint8_t)missionEnemyHeavyCounts[offerSlot];
    combatEnemyMediumCount=(uint8_t)missionEnemyMediumCounts[offerSlot];
    combatEnemyLightCount=(uint8_t)missionEnemyLightCounts[offerSlot];
    uint16_t scenario=missionScenarioCodes[offerSlot];
    combatScenarioCode=(uint8_t)scenario;
    uint16_t planetIndex=missionPlanetRecordIndices[offerSlot];
    combatPlanetSelector=(uint8_t)(planetIndex+1); /* INC BL, not INC BX. */
    planetIndex=(planetIndex&0xFF00)|combatPlanetSelector;
    --planetIndex;
    planetIndex=(uint16_t)(planetIndex<<1)/sizeof(uint16_t);
    if (planetIndex>=sizeof(planetRecordOffsets)/sizeof(planetRecordOffsets[0])) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: planetIndex>=sizeof(planetRecordOffsets)/sizeof(planetRecordOffsets[0])");
    uint16_t planetOffset=planetRecordOffsets[planetIndex];
    if (planetOffset<PLANET_RECORDS_START || (uint32_t)planetOffset+sizeof(MwPlanetHeader)>PLANET_RECORDS_END) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: planetOffset<PLANET_RECORDS_START || (uint32_t)planetOffset+sizeof(MwPlanetHeader)>PLANET_RECORDS_END");
    const MwPlanetHeader *planet=(const MwPlanetHeader *)&planetRecordStorage[planetOffset-PLANET_RECORDS_START];
    /*5590 replaces AL only. Retained scenario AH participates in DIV BL. */
    uint16_t landscapeDividend=(scenario&0xFF00)|(uint8_t)(planet->landscapeAndMarketCode-1);
    if (landscapeDividend/PLANET_LANDSCAPE_COUNT>UINT8_MAX) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: landscapeDividend/PLANET_LANDSCAPE_COUNT>UINT8_MAX");
    deploymentLandscapeIndex=(uint8_t)(landscapeDividend%PLANET_LANDSCAPE_COUNT);
    acceptedContractTargetHouseIndex=missionTargetHouseIndices[offerSlot];
    return CAMPAIGN_CONTINUE_CREDIT_CONTRACT_ADVANCE;
}

/* MW_MAIN081B:55A5..55D7; accepted-terms fallthrough, not a new native CALL. */
uint16_t MwCreditAcceptedContractAdvanceContinuation(void)
{
    uint32_t advanceThousands=(uint32_t)acceptedContractAdvancePercent*acceptedContractPaymentThousands/PERCENT_DIVISOR;
    if (advanceThousands>UINT16_MAX) /* Port emulation of a native CPU fault. */ MW_NATIVE_FAULT("Rejected condition: advanceThousands>UINT16_MAX"); /* Native DIV fault before credit. */
    uint32_t advanceCbills=advanceThousands*CBILLS_PER_THOUSAND;
    acceptedContractAdvanceCbillsLow=(uint16_t)advanceCbills;
    acceptedContractAdvanceCbillsHigh=(uint16_t)(advanceCbills>>16);
    uint32_t funds=((uint32_t)campaignFundsHigh<<16)|campaignFundsLow;
    funds+=advanceCbills;
    campaignFundsLow=(uint16_t)funds;campaignFundsHigh=(uint16_t)(funds>>16);
    remainingContractPaymentThousands=(uint16_t)(acceptedContractPaymentThousands-advanceThousands);
    deploymentRefusalCount=0;
    return CAMPAIGN_CONTINUE_ADJUST_ENEMY_FORCE;
}
/* MW_MAIN081B:55D8..561E. Signed wrapped BYTE total; no tonnage balancing. */
uint16_t MwAdjustAcceptedContractEnemyForceContinuation(void)
{
    uint16_t employerHouse=(uint16_t)(currentPlanetHouseIndex<<1)/sizeof(uint16_t);
    uint16_t roll=MwRandomUpTo(CONTRACT_PERCENT_MAXIMUM);
    if (employerHouse>=CAMPAIGN_HOUSE_COUNT) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: employerHouse>=CAMPAIGN_HOUSE_COUNT");
    if (roll<=houseExtraEnemyRollThresholds[employerHouse])
    {
        uint8_t enemies=combatEnemyHeavyCount+combatEnemyMediumCount+combatEnemyLightCount;
        if ((int8_t)enemies<MISSION_ENEMY_COUNT_LIMIT)
        {
            if (combatEnemyHeavyCount!=0) ++combatEnemyHeavyCount;
            else if (combatEnemyMediumCount!=0) ++combatEnemyMediumCount;
            else ++combatEnemyLightCount;
        }
    }
    return CAMPAIGN_CONTINUE_PLANET_HUB;
}

/* MW_MAIN081B:1B55..1B59 falls1B5A. Host zero is STC/RET, not AX success. */
uint16_t MwAttemptContractDeployment(void)
{
    deploymentPenaltySuppressed=0;
    return MwCheckAssignedMechDeploymentReadiness();
}
/* MW_MAIN081B:1CCC..1CD3 is the special-event JMP entry, not a bypass. */
uint16_t MwCheckDeploymentWithoutRefusalPenalty(void)
{
    deploymentPenaltySuppressed=1;
    return MwCheckAssignedMechDeploymentReadiness();
}
/* MW_MAIN081B:1B5A..1B86,1BD2..1C3D. The1B87 cancellation label is
 * shared, not a new source CALL. Success joins the native deployment chain. */
uint16_t MwCheckAssignedMechDeploymentReadiness(void)
{
    if (rosterMechAssignments[0]==CAMPAIGN_NO_MECH_ASSIGNMENT)
    {
        if (deploymentPenaltySuppressed!=1)
        {
            ++deploymentRefusalCount;
            if (deploymentRefusalCount==CONTRACT_REFUSALS_BEFORE_CANCELLATION) return CAMPAIGN_CONTINUE_CANCEL_CONTRACT;
        }
        MwNativeByteView message={(const uint8_t *)commanderNeedsMechText,0,sizeof(commanderNeedsMechText)};
        if (MwShowMonospaceTextPanel(0x10,0x10,0x24,6,&message,0).discardReturnWord) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: MwShowMonospaceTextPanel(0x10,0x10,0x24,6,&message,0).discardReturnWord");
        return 0;
    }
    uint16_t rosterSlot=0;
    do
    {
        if (rosterSlot>=CAMPAIGN_ROSTER_SLOTS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: rosterSlot>=CAMPAIGN_ROSTER_SLOTS"); /* Unrepresented adjacent roster words, not a zero-count guard. */
        uint16_t ownedSlot=rosterMechAssignments[rosterSlot];
        if (ownedSlot!=CAMPAIGN_NO_MECH_ASSIGNMENT)
        {
            ownedSlot=(uint8_t)ownedSlot; /* Native MUL BL consumes AL. */
            if (ownedSlot>=CAMPAIGN_OWNED_UNIT_SLOTS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: ownedSlot>=CAMPAIGN_OWNED_UNIT_SLOTS");
            const CampaignUnitDamage *damage=&ownedUnitDamage[ownedSlot];
            if (damage->engineHits>MECH_FUNCTIONAL_MAXIMUM_TIER || damage->gyroDamageTier>MECH_FUNCTIONAL_MAXIMUM_TIER
                || damage->lifeSupportDamageTier>MECH_FUNCTIONAL_MAXIMUM_TIER
                || damage->actuatorDamageTiers[2]>MECH_FUNCTIONAL_MAXIMUM_TIER
                || damage->actuatorDamageTiers[3]>MECH_FUNCTIONAL_MAXIMUM_TIER)
            {
                if (deploymentPenaltySuppressed!=1)
                {
                    ++deploymentRefusalCount;
                    if (deploymentRefusalCount==CONTRACT_REFUSALS_BEFORE_CANCELLATION) return CAMPAIGN_CONTINUE_CANCEL_CONTRACT;
                }
                MwNativeByteView message={(const uint8_t *)assignedMechUnreadyText,0,sizeof(assignedMechUnreadyText)};
                if (MwShowMonospaceTextPanel(0x10,0x10,0x21,6,&message,0).discardReturnWord) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: MwShowMonospaceTextPanel(0x10,0x10,0x21,6,&message,0).discardReturnWord");
                return 0;
            }
        }
        ++rosterSlot;
    } while (rosterSlot!=pilotRosterCount);
    return CAMPAIGN_CONTINUE_DEPLOYMENT;
}
/* MW_MAIN081B:1B87..1BD1. Refresh precedes live funds/advance/House reads. */
uint16_t MwCancelContractAfterDeploymentRefusalsContinuation(void)
{
    MwNativeByteView message={(const uint8_t *)contractCancelledText,0,sizeof(contractCancelledText)};
    if (MwShowMonospaceTextPanel(0x10,0x10,0x26,7,&message,0).discardReturnWord) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: MwShowMonospaceTextPanel(0x10,0x10,0x26,7,&message,0).discardReturnWord");
    acceptedContractActive=0;
    MwRefreshMissionOffers();
    deploymentRefusalCount=0;
    uint32_t funds=((uint32_t)campaignFundsHigh<<16)|campaignFundsLow;
    uint32_t advance=((uint32_t)acceptedContractAdvanceCbillsHigh<<16)|acceptedContractAdvanceCbillsLow;
    funds=funds<advance?0:funds-advance;
    campaignFundsLow=(uint16_t)funds;campaignFundsHigh=(uint16_t)(funds>>16);
    uint16_t employerHouse=(uint16_t)(currentPlanetHouseIndex<<1)/sizeof(uint16_t);
    if (employerHouse>=CAMPAIGN_HOUSE_COUNT) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: employerHouse>=CAMPAIGN_HOUSE_COUNT");
    ++houseGrievancePoints[employerHouse];
    return CAMPAIGN_CONTINUE_PLANET_HUB;
}
