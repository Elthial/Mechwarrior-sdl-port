#include "mechwarrior/campaign_continuations.h"
#include "mechwarrior/campaign_market.h"
#include "mechwarrior/ega_campaign.h"
#include "mechwarrior/campaign_input.h"
#include "mechwarrior/campaign_events.h"
#include <stdlib.h>
#include <string.h>

/* MW_MAIN081B:2DA6..2DF3. POP inherited dispatch IP, reset remembered rows.
 *2DE2 is the folded post-event entry, not a second game procedure. */
uint16_t MwEnterMechServiceMenuContinuation(uint16_t entryOffset)
{
    if (entryOffset==CAMPAIGN_CONTINUE_MECH_COMPLEX_EVENT_RETURN) goto showServicePanel;
    if (entryOffset!=CAMPAIGN_CONTINUE_MECH_COMPLEX) abort();
    hubSelectedRecordIndex=MECH_COMPLEX_HUB_ROW;
    ownedUnitListSelectedRow=0;
    mechServiceMenuSelectedRow=0;
    marketUnitListSelectedRow=0;
    EgaSelectFont(TEXT_PANEL_MONOSPACE_FONT);
    MwDrawMechComplexBackground();
    MwInitializeMechServiceIdle();
    if (unitInspectionContextCode==MECH_COMPLEX_FIRST_VISIT)
    {
        unitInspectionContextCode=MECH_COMPLEX_REVIEW_CONTEXT;
        campaignEventDispatchReturnContinuation=CAMPAIGN_CONTINUE_MECH_COMPLEX_EVENT_RETURN;
        uint16_t transfer=MwDispatchCampaignEvents(MECH_COMPLEX_EVENT_CLASS,CAMPAIGN_CALL_DISPATCH_EVENTS);
        if (transfer) return transfer;
        campaignEventDispatchReturnContinuation=0;
    }
showServicePanel:
    MwDrawDescriptorTextPanel(&mechComplexPanel);
    MwSelectUiRecordTable(unitInspectionContextCode,&mechComplexMenu);
    return CAMPAIGN_CONTINUE_UI;
}
/* MW_MAIN081B:35E3..362E. Unsigned SUB/borrow gate, not elapsed-tick math. */
void MwAnimateMechServiceIdle(void)
{
    if (campaignTimerTicks<mechServiceNextAnimationTick) return;
    mechServiceNextAnimationTick=campaignTimerTicks+1;
    uint16_t frameOffset=mechServiceAnimationByteOffset;
    if ((frameOffset&1)!=0 || frameOffset/sizeof(uint16_t)>=MECH_COMPLEX_ANIMATION_FRAMES) abort(); /* Unrepresented native adjacent/odd reads. */
    uint16_t picture=mechComplexAnimationFrames[frameOffset/sizeof(uint16_t)];
    mechServiceAnimationByteOffset+=sizeof(uint16_t);
    if (picture==0) return;
    if (picture==MECH_COMPLEX_ANIMATION_END)
    {
        mechServiceAnimationByteOffset=0;
        uint16_t pause=MwRandomUpTo(MECH_COMPLEX_ANIMATION_PAUSE_MAXIMUM)+MECH_COMPLEX_ANIMATION_PAUSE_MINIMUM;
        mechServiceNextAnimationTick+=pause;
        return;
    }
    MwDrawGeneralPicture(MECH_COMPLEX_ANIMATION_X,MECH_COMPLEX_ANIMATION_Y,EGA_DISPLAY_PAGE,picture);
}

/* MW_MAIN081B:4674..4769;475A is also the dealer DONE callback entry.
 * entryOffset transports that shared label, not a native pushed argument. */
uint16_t MwPurchaseSelectedMarketUnit(uint16_t entryOffset)
{
    if (entryOffset==CAMPAIGN_CONTINUE_MECH_PURCHASE_FINISHED) goto purchaseFinished;
    uint32_t purchasePrice=((uint32_t)unitTransactionPriceHigh<<NATIVE_WORD_BITS)|unitTransactionPriceLow;
    uint32_t funds=((uint32_t)campaignFundsHigh<<NATIVE_WORD_BITS)|campaignFundsLow;
    if (purchasePrice>funds)
    {
        MwNativeByteView text={(const uint8_t *)purchaseInsufficientFundsText,0,sizeof(purchaseInsufficientFundsText)};
        MwShowMonospaceTextPanel(MARKET_MESSAGE_X,MARKET_MESSAGE_Y,PURCHASE_FUNDS_MESSAGE_COLUMNS,PURCHASE_FUNDS_MESSAGE_ROWS,&text,0);
        return 0;
    }
    if (ownedUnitCount>=CAMPAIGN_OWNED_UNIT_SLOTS)
    {
        MwNativeByteView text={(const uint8_t *)ownedMechCapacityText,0,sizeof(ownedMechCapacityText)};
        MwShowMonospaceTextPanel(OWNERSHIP_LIMIT_MESSAGE_X,OWNERSHIP_LIMIT_MESSAGE_Y,OWNERSHIP_LIMIT_MESSAGE_COLUMNS,OWNERSHIP_LIMIT_MESSAGE_ROWS,&text,0);
        return 0;
    }
    funds-=purchasePrice;
    campaignFundsHigh=(uint16_t)(funds>>NATIVE_WORD_BITS);
    campaignFundsLow=(uint16_t)funds;
    uint16_t ownedUnitSlot=0;
    while (ownedUnitSlot<CAMPAIGN_OWNED_UNIT_SLOTS && ownedUnitKinds[ownedUnitSlot]!=CAMPAIGN_VACANT_UNIT_KIND) ++ownedUnitSlot;
    if (ownedUnitSlot==CAMPAIGN_OWNED_UNIT_SLOTS) return 0; /* Payment is not refunded. */
    purchaseOwnedSlotByteOffset=ownedUnitSlot*sizeof(uint16_t);
    uint16_t marketUnitSlot=selectedMarketUnitByteOffset/sizeof(uint16_t);
    ownedUnitKinds[ownedUnitSlot]=marketUnitKinds[marketUnitSlot];
    ++ownedUnitCount;
    const uint8_t *source=(const uint8_t *)&marketUnitDamage[(uint8_t)selectedMarketUnitSlot];
    uint8_t *destination=(uint8_t *)&ownedUnitDamage[(uint8_t)ownedUnitSlot];
    for (uint16_t index=0;index<CAMPAIGN_DAMAGE_BYTES;++index) destination[index]=source[index];
    for (uint16_t group=0;group<CAMPAIGN_AMMO_GROUPS;++group)
        ownedAmmoQuantitiesByGroup[group*CAMPAIGN_OWNED_UNIT_SLOTS+ownedUnitSlot]=marketAmmoQuantitiesByGroup[group*CAMPAIGN_MARKET_UNIT_SLOTS+marketUnitSlot];
    for (uint16_t group=0;group<CAMPAIGN_AMMO_GROUPS;++group)
        ownedAmmoKindsByGroup[group*CAMPAIGN_OWNED_UNIT_SLOTS+ownedUnitSlot]=marketAmmoKindsByGroup[group*CAMPAIGN_MARKET_UNIT_SLOTS+marketUnitSlot];
    marketUnitKinds[marketUnitSlot]=CAMPAIGN_VACANT_UNIT_KIND;
    --marketUnitCount;
purchaseFinished:
    marketUnitListSelectedRow=0;
    if (marketUnitCount==0) return CAMPAIGN_CONTINUE_MECH_COMPLEX;
    MwDrawMechComplexBackground();
    return CAMPAIGN_CONTINUE_MARKET_MECH_LIST;
}

/* MW_MAIN081B:407C..4316. Discard callback IP, display dealer damage only.
 * Same condition-text shape as owned repair, with separate mutable owners. */
uint16_t MwOpenMarketMechComponentInspectionCallback(void)
{
    unitInspectionContextCode=MECH_COMPLEX_DEALER_CONTEXT;
    EgaSelectFont(TEXT_PANEL_COMPACT_FONT);
    uint16_t kind=selectedMarketUnitKind;
    uint16_t titleCursor=0;
    while (titleCursor!=repairTitleLeadingSpacesByKind[kind])
        marketComponentTitle.text[titleCursor++]=ASCII_SPACE;
    strcpy(marketComponentTitle.text+titleCursor,unitDisplayNames[kind]);
    CampaignUnitDamage *damage=&marketUnitDamage[(uint8_t)selectedMarketUnitSlot];
    uint8_t *damagedComponent=(uint8_t *)damage;
    marketSystemRowIndex=0;
    do
    {
        uint8_t tier=*damagedComponent;
        uint16_t category=marketSystemRowIndex;
        MwComponentConditionTextRow *row=&marketSystemConditionRows[category];
        row->display.colour=tier==0?REPAIR_CONDITION_HEALTHY_COLOUR:REPAIR_CONDITION_DAMAGED_COLOUR;
        if (category==REPAIR_HEAT_SINKS)
        {
            uint16_t kind=selectedMarketUnitKind;
            uint8_t maximum=unitHeatSinkCounts[kind];
            MwBuildDecimalDigitBuffer((uint8_t)(maximum-tier));
            row->caption.columns.condition[0]=numericDecimalDigits[8]==0?ASCII_SPACE:numericDecimalDigits[8]+ASCII_DIGIT_ZERO;
            row->caption.columns.condition[1]=numericDecimalDigits[9]+ASCII_DIGIT_ZERO;
            kind=selectedMarketUnitKind;
            maximum=unitHeatSinkCounts[kind];
            MwBuildDecimalDigitBuffer(maximum);
            row->caption.columns.condition[6]=numericDecimalDigits[8]+ASCII_DIGIT_ZERO;
            row->caption.columns.condition[7]=numericDecimalDigits[9]+ASCII_DIGIT_ZERO;
        }
        else if (category==REPAIR_JUMP_JETS)
        {
            for (uint16_t letter=0;letter<sizeof(repairJumpJetCountTemplateBytes);++letter)
                row->caption.columns.condition[letter]=repairJumpJetCountTemplateBytes[letter];
            uint16_t kind=selectedMarketUnitKind;
            uint8_t maximum=unitJumpJetGroupCounts[kind];
            MwBuildDecimalDigitBuffer((uint8_t)(maximum-tier));
            row->caption.columns.condition[1]=numericDecimalDigits[9]+ASCII_DIGIT_ZERO;
            kind=selectedMarketUnitKind;
            maximum=unitJumpJetGroupCounts[kind];
            if (maximum!=0)
            {MwBuildDecimalDigitBuffer(maximum);row->caption.columns.condition[6]=numericDecimalDigits[9]+ASCII_DIGIT_ZERO;}
            else for (uint16_t letter=0;letter<sizeof(repairNoJumpJetsTextBytes);++letter)
                row->caption.columns.condition[letter]=repairNoJumpJetsTextBytes[letter];
        }
        else
        {
            /* Copy includes NUL, which can occupy the following terminator. */
            strcpy(row->caption.text+REPAIR_COMPONENT_LABEL_COLUMNS,campaignDamageTierLabels[tier]);
        }
        ++marketSystemRowIndex;++damagedComponent;
    } while (marketSystemRowIndex!=CAMPAIGN_SYSTEM_COUNT);
    damagedComponent=marketUnitDamage[(uint8_t)selectedMarketUnitSlot].bodyDamageTiers;
    uint8_t bodyDamage=0;
    for (uint16_t location=0;location<CAMPAIGN_BODY_LOCATION_COUNT;++location,++damagedComponent)
        bodyDamage+=*damagedComponent&3;
    marketBodyCondition.display.colour=bodyDamage==0?REPAIR_CONDITION_HEALTHY_COLOUR:REPAIR_CONDITION_DAMAGED_COLOUR;
    uint16_t bodyProtection=(uint16_t)(REPAIR_BODY_TOTAL_TIERS-bodyDamage)*PERCENT_DIVISOR/REPAIR_BODY_TOTAL_TIERS;
    MwBuildDecimalDigitBuffer(bodyProtection);
    marketBodyCondition.caption.columns.protectionPercent[0]=numericDecimalDigits[7]==0?ASCII_SPACE:numericDecimalDigits[7]+ASCII_DIGIT_ZERO;
    marketBodyCondition.caption.columns.protectionPercent[1]=numericDecimalDigits[8]==0 && numericDecimalDigits[7]==0?ASCII_SPACE:numericDecimalDigits[8]+ASCII_DIGIT_ZERO;
    marketBodyCondition.caption.columns.protectionPercent[2]=numericDecimalDigits[9]+ASCII_DIGIT_ZERO;
    uint16_t y=REPAIR_WEAPON_FIRST_Y;
    for (uint16_t mount=0;mount<CAMPAIGN_WEAPON_SLOTS;++mount)
    {
        marketWeaponConditionRows[mount].display.x=REPAIR_WEAPON_MENU_X;
        marketWeaponConditionRows[mount].display.y=y;
        y+=REPAIR_WEAPON_ROW_HEIGHT;
    }
    kind=selectedMarketUnitKind;
    uint16_t label=0;
    while (repairWeaponLabels[kind][label]!=NULL)
    {
        if (label<CAMPAIGN_WEAPON_SLOTS)
            strcpy(marketWeaponConditionRows[label].caption.text,repairWeaponLabels[kind][label]);
        else strcpy(marketComponentDone.text,repairWeaponLabels[kind][label]);
        ++label;
    }
    MwPanelText *doneCaption=label-1<CAMPAIGN_WEAPON_SLOTS
        ?&marketWeaponConditionRows[label-1].display:&marketComponentDone.display;
    doneCaption->x=REPAIR_DONE_X;doneCaption->y=DEALER_COMPONENT_DONE_Y;
    MwUiRecord *done=&marketComponentMenu.records[0];
    done->text=doneCaption->text;
    done->textColour=&doneCaption->colour; /*4817 aliases this reference, not separate state. */
    kind=selectedMarketUnitKind;
    uint16_t mountsRemaining=unitWeaponCounts[kind],mount=0;
    damagedComponent=marketUnitDamage[(uint8_t)selectedMarketUnitSlot].weaponDamageTiers;
    do
    {
        if (mount>=CAMPAIGN_WEAPON_SLOTS) abort(); /* Native LOOP underflow is not a zero-weapon no-op. */
        uint8_t tier=*damagedComponent++;
        MwComponentConditionTextRow *row=&marketWeaponConditionRows[mount++];
        row->display.colour=tier==0?REPAIR_CONDITION_HEALTHY_COLOUR:REPAIR_CONDITION_DAMAGED_COLOUR;
        strcpy(row->caption.text+REPAIR_COMPONENT_LABEL_COLUMNS,campaignDamageTierLabels[tier]);
    } while (--mountsRemaining!=0);
    done->x=REPAIR_DONE_X;done->y=DEALER_COMPONENT_DONE_Y;done->activateCodeOffset=CAMPAIGN_CONTINUE_MARKET_COMPONENT_INSPECTION_RETURN;
    kind=selectedMarketUnitKind;
    marketComponentPanel.lineCount=unitWeaponCounts[kind]+DEALER_COMPONENT_FIXED_LINES;
    EgaFillDisplayPageRectangle(REPAIR_PANEL_X,REPAIR_PANEL_Y,REPAIR_PANEL_WIDTH,REPAIR_PANEL_HEIGHT,MECH_DETAIL_BACKGROUND);
    MwDrawDescriptorTextPanel(&marketComponentPanel);
    MwSelectUiRecordTable(0,&marketComponentMenu);
    return CAMPAIGN_CONTINUE_UI;
}

/* MW_MAIN081B:4353..4644, with shared4379/43EB and callback43E0. Do not
 * extract the picture/specification/ammo fallthroughs into new procedures. */
uint16_t MwDrawMarketMechDetailContinuation(uint16_t entryOffset)
{
    if (entryOffset==CAMPAIGN_CONTINUE_MARKET_MECH_STATUS) goto marketStatus;
    if (entryOffset==CAMPAIGN_CONTINUE_MARKET_COMPONENT_INSPECTION_RETURN)
    {
        EgaSelectFont(TEXT_PANEL_COMPACT_FONT);
        goto marketStatus; /* Native POP at43E0 is represented by caller transport. */
    }
    if (entryOffset!=CAMPAIGN_CONTINUE_MARKET_DETAIL_KEEP_BACKGROUND)
    {
        EgaSelectFont(TEXT_PANEL_COMPACT_FONT);
        EgaClearDrawPage(MECH_DETAIL_BACKGROUND);
        EgaCopyDrawPageToDisplay(0,0,EGA_COLUMNS,EGA_ROWS);
    }
    MwDrawMechInspectionGrid();
    if (selectedMarketUnitKind!=CAMPAIGN_UNIT_DROPSHIP)
        MwDrawMechBodyDamageDiagram(selectedMarketUnitKind,&marketUnitDamage[(uint8_t)selectedMarketUnitSlot]);
    uint16_t kind=selectedMarketUnitKind;
    uint16_t x=unitDetailPictureXOffsets[kind]-DETAIL_PICTURE_X_INSET;
    uint16_t y=unitDetailPictureYOrigins[kind];
    x+=INSPECTION_GRID_X;
    uint16_t picture=kind==CAMPAIGN_UNIT_DROPSHIP?DROPSHIP_DETAIL_PICTURE:(uint16_t)(kind+MECH_DETAIL_PICTURE_BASE);
    uint16_t selector=MwLoadIndexedPicture(picture);
    EgaDrawPackedSprite(x,y,EGA_DRAW_PAGE,0,selector);
    EgaCopyDrawPageToDisplay(INSPECTION_CAPTURE_X,INSPECTION_PUBLISH_Y,INSPECTION_PUBLISH_WIDTH,INSPECTION_PUBLISH_HEIGHT);
marketStatus:
    MwDrawDescriptorTextPanel(selectedMarketUnitKind==CAMPAIGN_UNIT_DROPSHIP?&marketDropShipStatusPanel:&marketMechStatusPanel);
    MwDrawPicturePanelBorder(0,0,EGA_COLUMNS,EGA_ROWS);
    selectedMarketUnitKind=marketUnitKinds[selectedMarketUnitByteOffset/sizeof(uint16_t)];
    const char *mechName=unitDisplayNames[selectedMarketUnitKind];
    MwNativeByteView name={(const uint8_t *)mechName,0,(uint32_t)strlen(mechName)+1};
    EgaDrawTransparentText(MECH_DETAIL_NAME_X,MECH_DETAIL_NAME_Y,MECH_DETAIL_TEXT_COLOUR,&name,0);
    CampaignUnitDamage *damage=&marketUnitDamage[(uint8_t)selectedMarketUnitSlot];
    /*444A..4484's sum is overwritten at4486, not an overall condition score. */
    const char *conditionText=(const char *)mechNonfunctionalTextBytes;
    if (damage->engineHits<=MECH_FUNCTIONAL_MAXIMUM_TIER && damage->gyroDamageTier<=MECH_FUNCTIONAL_MAXIMUM_TIER
        && damage->lifeSupportDamageTier<=MECH_FUNCTIONAL_MAXIMUM_TIER
        && damage->actuatorDamageTiers[2]<=MECH_FUNCTIONAL_MAXIMUM_TIER
        && damage->actuatorDamageTiers[3]<=MECH_FUNCTIONAL_MAXIMUM_TIER) conditionText=mechFunctionalTextBytes;
    MwNativeByteView condition={(const uint8_t *)conditionText,0,(uint32_t)strlen(conditionText)+1};
    EgaDrawTransparentText(MECH_DETAIL_CONDITION_X,MECH_DETAIL_CONDITION_Y,MECH_DETAIL_TEXT_COLOUR,&condition,0);
    if (selectedMarketUnitKind==CAMPAIGN_UNIT_DROPSHIP) goto purchasePrice;
    damage=&marketUnitDamage[(uint8_t)selectedMarketUnitSlot];
    MwQuoteMechRepairTotal(selectedMarketUnitKind,damage); /* Also writes shared2AD5. */
    uint32_t repairQuote=((uint32_t)unitRepairQuoteHigh<<NATIVE_WORD_BITS)|unitRepairQuoteLow;
    numericGroupingEnabled=1;
    MwDrawSignedDecimalField(repairQuote,MECH_DETAIL_REPAIR_PRICE_X,1,0,MECH_DETAIL_TEXT_COLOUR,MECH_DETAIL_BACKGROUND,MECH_DETAIL_REPAIR_PRICE_Y);
    kind=selectedMarketUnitKind;
    uint8_t tonnage=unitDisplayTonnages[kind];
    MwDrawSignedDecimalField(tonnage,MECH_DETAIL_TONNAGE_X,1,0,MECH_DETAIL_TEXT_COLOUR,MECH_DETAIL_BACKGROUND,MECH_DETAIL_TONNAGE_Y);
    kind=selectedMarketUnitKind;
    uint8_t speed=unitDisplaySpeedsKph[kind];
    MwDrawSignedDecimalField(speed,MECH_DETAIL_SPEED_X,1,0,MECH_DETAIL_TEXT_COLOUR,MECH_DETAIL_BACKGROUND,MECH_DETAIL_SPEED_Y);
    kind=selectedMarketUnitKind;
    uint8_t jumpCapacity=unitDisplayJumpCapacitiesMetres[kind];
    MwDrawSignedDecimalField(jumpCapacity,MECH_DETAIL_JUMP_CAPACITY_X,1,0,MECH_DETAIL_TEXT_COLOUR,MECH_DETAIL_BACKGROUND,MECH_DETAIL_JUMP_CAPACITY_Y);
    for (uint16_t group=0;group<CAMPAIGN_AMMO_GROUPS;++group)
        selectedMarketAmmoFamilies[group]=marketAmmoKindsByGroup[selectedMarketUnitSlot+group*CAMPAIGN_MARKET_UNIT_SLOTS];
    for (uint16_t group=0;group<CAMPAIGN_AMMO_GROUPS;++group)
        selectedMarketAmmoQuantities[group]=marketAmmoQuantitiesByGroup[selectedMarketUnitSlot+group*CAMPAIGN_MARKET_UNIT_SLOTS];
    uint16_t group=0;
    do
    {
        if (group>=CAMPAIGN_AMMO_GROUPS) abort(); /* Native zero count overruns adjacent storage; not a no-op. */
        uint16_t family=selectedMarketAmmoFamilies[group];
        uint16_t quantity=selectedMarketAmmoQuantities[group];
        const char *ammoName=ammoFamilyDisplayNames[family];
        y=MECH_DETAIL_AMMO_FIRST_Y+(uint16_t)(group*DEALER_AMMO_ROW_HEIGHT);
        MwNativeByteView text={(const uint8_t *)ammoName,0,(uint32_t)strlen(ammoName)+1};
        EgaDrawTransparentText(MECH_DETAIL_AMMO_LABEL_X,y,MECH_DETAIL_TEXT_COLOUR,&text,0);
        MwDrawSignedDecimalField(quantity,MECH_DETAIL_AMMO_QUANTITY_X,1,0,MECH_DETAIL_TEXT_COLOUR,MECH_DETAIL_BACKGROUND,y);
        ++group;kind=selectedMarketUnitKind;
    } while ((uint8_t)group!=unitAmmoGroupCounts[kind]);
purchasePrice:
    MwCalculateMarketPurchasePrice(); /* DropShip intentionally uses the stale repair quote. */
    uint32_t price=((uint32_t)unitTransactionPriceHigh<<NATIVE_WORD_BITS)|unitTransactionPriceLow;
    ++numericGroupingEnabled;
    MwDrawSignedDecimalField(price,DEALER_PRICE_X,1,0,MECH_DETAIL_TEXT_COLOUR,MECH_DETAIL_BACKGROUND,DEALER_PRICE_Y);
    uint32_t funds=((uint32_t)campaignFundsHigh<<NATIVE_WORD_BITS)|campaignFundsLow;
    ++numericGroupingEnabled;
    MwDrawSignedDecimalField(funds,DEALER_FUNDS_X,1,0,MECH_DETAIL_TEXT_COLOUR,MECH_DETAIL_BACKGROUND,DEALER_FUNDS_Y);
    if (selectedMarketUnitKind==CAMPAIGN_UNIT_DROPSHIP) MwSelectUiRecordTable(0,&marketDropShipMenu);
    else
    {
        EgaCopyDisplayToDrawPage(0,0,EGA_COLUMNS,EGA_ROWS);
        MwSelectUiRecordTable(0,&marketMechMenu);
    }
    return CAMPAIGN_CONTINUE_UI;
}

/* MW_MAIN081B:2DF4..2E06. Genuine near initializer also used by empty dealer. */
void MwInitializeMechServiceIdle(void)
{
    uiIdleCodeOffset=CAMPAIGN_CALLBACK_MECH_SERVICE_IDLE;
    mechServiceNextAnimationTick=campaignTimerTicks;
    mechServiceAnimationByteOffset=0;
}
/* MW_MAIN081B:2F32..3020. Empty branch RET; otherwise POP2F5F and JMP089B. */
uint16_t MwOpenMarketMechList(void)
{
    unitInspectionContextCode=MECH_COMPLEX_DEALER_CONTEXT;
    if (marketUnitCount==0)
    {
        EgaSelectFont(TEXT_PANEL_MONOSPACE_FONT);
        MwNativeByteView text={(const uint8_t *)noMarketMechsText,0,sizeof(noMarketMechsText)};
        MwShowMonospaceTextPanel(MARKET_MESSAGE_X,DEALER_EMPTY_MESSAGE_Y,DEALER_EMPTY_MESSAGE_COLUMNS,DEALER_EMPTY_MESSAGE_ROWS,&text,0);
        MwInitializeMechServiceIdle();
        return 0;
    }
    EgaSelectFont(TEXT_PANEL_MONOSPACE_FONT);
    uint16_t marketUnitSlot=0,visibleRow=0;
    do
    {
        while (marketUnitSlot<CAMPAIGN_MARKET_UNIT_SLOTS && marketUnitKinds[marketUnitSlot]==CAMPAIGN_VACANT_UNIT_KIND) ++marketUnitSlot;
        if (marketUnitSlot>=CAMPAIGN_MARKET_UNIT_SLOTS || visibleRow>=CAMPAIGN_MARKET_UNIT_SLOTS) abort(); /* Unrepresented native overrun. */
        memcpy(marketUnitListRows[visibleRow].text,shopUnitNames[marketUnitKinds[marketUnitSlot]],DEALER_LIST_NAME_COLUMNS);
        ++marketUnitSlot;++visibleRow;
    } while (visibleRow!=marketUnitCount);
    memcpy(marketUnitListRows[visibleRow].text,marketListDoneText,DEALER_LIST_NAME_COLUMNS);
    uint8_t previousRow=0,nextRow=2;
    for (uint16_t row=1;row<CAMPAIGN_MARKET_UNIT_SLOTS;++row)
    {
        MwUiRecord *record=&marketUnitListMenu.records[row];
        record->upRecordIndex=previousRow++;record->downRecordIndex=nextRow++;
        record->activateCodeOffset=CAMPAIGN_CALLBACK_SELECT_MARKET_MECH;
    }
    MwUiRecord *done=&marketUnitListMenu.records[(uint8_t)marketUnitCount];
    done->downRecordIndex=0;done->activateCodeOffset=CAMPAIGN_CONTINUE_MECH_COMPLEX;
    marketUnitListPanel.lineCount=marketUnitCount+2;
    marketUnitListMenu.records[0].upRecordIndex=(uint8_t)marketUnitCount;
    EgaCopyDrawPageToDisplay(0,0,EGA_COLUMNS,EGA_ROWS);
    MwDrawDescriptorTextPanel(&marketUnitListPanel);
    MwSelectUiRecordTable(marketUnitListSelectedRow,&marketUnitListMenu);
    return CAMPAIGN_CONTINUE_UI;
}
/* MW_MAIN081B:4317..4351. POP dispatch IP; shared4352 POP is bypassed. */
uint16_t MwSelectMarketUnitListRowCallback(void)
{
    uint16_t selectedRow=selectedUiRecordIndex;
    marketUnitListSelectedRow=selectedRow;
    uint16_t marketUnitSlot=0,visibleRow=0;
    for (;;)
    {
        if (marketUnitSlot>=CAMPAIGN_MARKET_UNIT_SLOTS) abort();
        if (marketUnitKinds[marketUnitSlot]!=CAMPAIGN_VACANT_UNIT_KIND)
        {
            if (visibleRow==selectedRow) break;
            ++visibleRow;
        }
        ++marketUnitSlot;
    }
    selectedMarketUnitByteOffset=marketUnitSlot*sizeof(uint16_t);
    selectedMarketUnitSlot=marketUnitSlot;
    selectedMarketUnitKind=marketUnitKinds[selectedMarketUnitByteOffset/sizeof(uint16_t)];
    return CAMPAIGN_CONTINUE_MARKET_MECH_DETAIL;
}

/* MW_MAIN081B:3393..3476. Shared ACCEPT continuation, not a new native CALL.
 * Payment/removal stand even if stock count or vacancy search rejects insertion. */
uint16_t MwSaleContinuation(void)
{
    uint32_t funds=((uint32_t)campaignFundsHigh<<NATIVE_WORD_BITS)|campaignFundsLow;
    funds+=((uint32_t)unitTransactionPriceHigh<<NATIVE_WORD_BITS)|unitTransactionPriceLow;
    campaignFundsLow=(uint16_t)funds;
    campaignFundsHigh=(uint16_t)(funds>>NATIVE_WORD_BITS);
    uint16_t pilotsRemaining=pilotRosterCount, pilotRosterSlot=0;
    do
    {
        if (pilotRosterSlot>=CAMPAIGN_ROSTER_SLOTS) abort(); /* Native adjacent roster reads not represented. */
        if (rosterMechAssignments[pilotRosterSlot]==selectedOwnedUnitSlot)
        {
            rosterMechAssignments[pilotRosterSlot]=CAMPAIGN_NO_MECH_ASSIGNMENT;
            break; /* Only the first assignment is released. */
        }
        ++pilotRosterSlot;
    } while (--pilotsRemaining!=0);
    uint16_t ownedUnitSlot=selectedOwnedUnitByteOffset/sizeof(uint16_t);
    ownedUnitKinds[ownedUnitSlot]=CAMPAIGN_VACANT_UNIT_KIND;
    --ownedUnitCount;
    ++unitsSoldSinceMarketRefresh;
    if (marketUnitCount<CAMPAIGN_MARKET_UNIT_SLOTS)
    {
        uint16_t marketUnitSlot=0;
        while (marketUnitSlot<CAMPAIGN_MARKET_UNIT_SLOTS && marketUnitKinds[marketUnitSlot]!=CAMPAIGN_VACANT_UNIT_KIND) ++marketUnitSlot;
        if (marketUnitSlot==CAMPAIGN_MARKET_UNIT_SLOTS) goto saleFinished;
        saleMarketSlotByteOffset=marketUnitSlot*sizeof(uint16_t);
        marketUnitKinds[marketUnitSlot]=selectedOwnedUnitKind;
        ++marketUnitCount;
        uint8_t *destination=(uint8_t *)&marketUnitDamage[(uint8_t)marketUnitSlot];
        const uint8_t *source=(const uint8_t *)&ownedUnitDamage[(uint8_t)selectedOwnedUnitSlot];
        for (uint16_t index=0;index<CAMPAIGN_DAMAGE_BYTES;++index) destination[index]=source[index];
        for (uint16_t group=0;group<CAMPAIGN_AMMO_GROUPS;++group)
            marketAmmoQuantitiesByGroup[group*CAMPAIGN_MARKET_UNIT_SLOTS+marketUnitSlot]=ownedAmmoQuantitiesByGroup[group*CAMPAIGN_OWNED_UNIT_SLOTS+ownedUnitSlot];
        for (uint16_t group=0;group<CAMPAIGN_AMMO_GROUPS;++group)
            marketAmmoKindsByGroup[group*CAMPAIGN_MARKET_UNIT_SLOTS+marketUnitSlot]=ownedAmmoKindsByGroup[group*CAMPAIGN_OWNED_UNIT_SLOTS+ownedUnitSlot];
    }
saleFinished:
    ownedUnitListSelectedRow=0;
    if (ownedUnitCount==0) return CAMPAIGN_CONTINUE_MECH_COMPLEX;
    MwDrawMechComplexBackground();
    return CAMPAIGN_CONTINUE_OWNED_MECH_LIST;
}
/* MW_MAIN081B:3477..348C. REJECT restores the saved region then JMP3291. */
uint16_t MwCancelOwnedUnitSale(void)
{
    EgaCopyDrawPageToDisplay(SALE_DIALOG_X,SALE_DIALOG_Y,SALE_DIALOG_WIDTH,SALE_DIALOG_HEIGHT);
    return CAMPAIGN_CONTINUE_OWNED_MECH_SERVICE_MENU;
}

/* MW_MAIN081B:32BA..32EA. Unlike32EB there is no condition factor or +500.
 * Repair-quote subtraction wraps; DIV overflow precedes either price store. */
void MwCalculateSelectedUnitNetPrice(void)
{
    if (currentPlanetMarketTier>=UNIT_PRICE_TIER_COUNT || selectedOwnedUnitKind>=CAMPAIGN_UNIT_KIND_COUNT) abort(); /* Unrepresented adjacent tables. */
    uint8_t saleMultiplier=unitPriceTierMultipliers[currentPlanetMarketTier];
    --saleMultiplier;
    uint32_t salePrice=(uint32_t)(saleMultiplier*PERCENT_DIVISOR)*unitBasePrices[selectedOwnedUnitKind];
    salePrice-=((uint32_t)unitRepairQuoteHigh<<NATIVE_WORD_BITS)|unitRepairQuoteLow;
    salePrice/=CBILLS_PER_THOUSAND;
    if (salePrice>UINT16_MAX) abort();
    salePrice*=CBILLS_PER_THOUSAND;
    unitTransactionPriceLow=(uint16_t)salePrice;
    unitTransactionPriceHigh=(uint16_t)(salePrice>>NATIVE_WORD_BITS);
}
/* MW_MAIN081B:3314..3392. Denial RETs, allowed path POPs at3343 then JMP089B. */
uint16_t MwBeginOwnedUnitSaleCallback(void)
{
    mechServiceMenuSelectedRow=MECH_SERVICE_ROW_SELL;
    MwAdvanceCampaignTimeAndChargeWages(SALE_QUOTE_TIME_UNITS);
    if ((int16_t)currentPlanetMarketTier>=DEALER_QUOTA_RESTRICTED_TIER
        && unitsSoldSinceMarketRefresh==dealerUnitPurchaseLimit)
    {
        MwNativeByteView text={(const uint8_t *)dealerPurchaseLimitText,0,sizeof(dealerPurchaseLimitText)};
        MwShowMonospaceTextPanel(MARKET_MESSAGE_X,MARKET_MESSAGE_Y,DEALER_QUOTA_MESSAGE_COLUMNS,DEALER_QUOTA_MESSAGE_ROWS,&text,0);
        return 0;
    }
    EgaSelectFont(TEXT_PANEL_MONOSPACE_FONT);
    EgaCopyDisplayToDrawPage(SALE_DIALOG_X,SALE_DIALOG_Y,SALE_DIALOG_WIDTH,SALE_DIALOG_HEIGHT);
    MwDrawDescriptorTextPanel(&saleConfirmationPanel);
    MwCalculateSelectedUnitNetPrice();
    uint32_t salePrice=((uint32_t)unitTransactionPriceHigh<<NATIVE_WORD_BITS)|unitTransactionPriceLow;
    ++numericGroupingEnabled;
    MwDrawSignedDecimalField(salePrice,SALE_QUOTE_X,0,MARKET_PRICE_FIELD_DIGITS,TEXT_PANEL_FOREGROUND,TEXT_PANEL_BACKGROUND,SALE_QUOTE_Y);
    MwSelectUiRecordTable(0,&saleConfirmationMenu);
    return CAMPAIGN_CONTINUE_UI;
}

/* MW_MAIN081B:3560..35C8. No extra time charge or reserve-ammo transfer.
 * Partial purchases survive; the inherited callback IP is consumed at3091. */
uint16_t MwRearmSelectedMech(void)
{
    uint16_t unitKind=selectedOwnedUnitKind;
    uint16_t groupsRemaining=unitAmmoGroupCounts[unitKind];
    const CampaignAmmoProfileEntry *profile=unitAmmoProfiles[unitKind];
    uint16_t quantityIndex=selectedOwnedUnitSlot;
    selectedMechRearmQuoteLow=0; /* High WORD is deliberately stale. */
    do
    {
        if (!profile || quantityIndex>=CAMPAIGN_AMMO_GROUPS*CAMPAIGN_OWNED_UNIT_SLOTS) abort();
        uint16_t ammoFamily=profile->ammoFamily;
        uint16_t purchasesRemaining=(uint16_t)(profile->quantityMaximum-ownedAmmoQuantitiesByGroup[quantityIndex]);
        if (purchasesRemaining!=0)
        {
            uint16_t pricePerAmmoUnit=ammunitionTransactionPrices[ammoFamily];
            uint32_t funds=((uint32_t)campaignFundsHigh<<NATIVE_WORD_BITS)|campaignFundsLow;
            do
            {
                if (funds<pricePerAmmoUnit) goto rearmFinished;
                funds-=pricePerAmmoUnit;
                campaignFundsLow=(uint16_t)funds;
                campaignFundsHigh=(uint16_t)(funds>>NATIVE_WORD_BITS);
                ++ownedAmmoQuantitiesByGroup[quantityIndex];
            } while (--purchasesRemaining!=0);
        }
        ++profile;quantityIndex+=CAMPAIGN_OWNED_UNIT_SLOTS;
    } while (--groupsRemaining!=0);
rearmFinished:
    MwRestoreMechServiceDialogRegion();
    return CAMPAIGN_CONTINUE_RETURN_TO_OWNED_MECH;
}
/* MW_MAIN081B:35C9..35CE. No repair-category reset on this cancellation tail. */
uint16_t MwCancelSelectedMechRearm(void)
{
    MwRestoreMechServiceDialogRegion();
    return CAMPAIGN_CONTINUE_OWNED_MECH_SERVICE_MENU;
}
/* MW_MAIN081B:35CF..35E2. Genuine near callee shared by action and cancel. */
void MwRestoreMechServiceDialogRegion(void)
{
    EgaCopyDrawPageToDisplay(REARM_DIALOG_X,REARM_DIALOG_Y,REARM_DIALOG_WIDTH,REARM_DIALOG_HEIGHT);
}

/* MW_MAIN081B:348D..355F. Full bay RETs with callback IP intact; the
 * confirmation path discards it at3540 and JMPs089B. Return is JMP transport. */
uint16_t MwQuoteSelectedMechRearm(void)
{
    mechServiceMenuSelectedRow=MECH_SERVICE_ROW_REARM;
    MwAdvanceCampaignTimeAndChargeWages(REARM_QUOTE_TIME_UNITS);
    uint16_t unitKind=selectedOwnedUnitKind;
    uint16_t groupsRemaining=unitAmmoGroupCounts[unitKind];
    const CampaignAmmoProfileEntry *profile=unitAmmoProfiles[unitKind];
    uint16_t quantityIndex=selectedOwnedUnitSlot;
    selectedMechRearmQuoteLow=0;
    selectedMechRearmQuoteHigh=0;
    do
    {
        if (!profile || quantityIndex>=CAMPAIGN_AMMO_GROUPS*CAMPAIGN_OWNED_UNIT_SLOTS) abort(); /* Unrepresented adjacent reads. */
        uint16_t ammoFamily=profile->ammoFamily;
        uint16_t quantityToBuy=(uint16_t)(profile->quantityMaximum-ownedAmmoQuantitiesByGroup[quantityIndex]);
        uint32_t rearmCost=((uint32_t)selectedMechRearmQuoteHigh<<NATIVE_WORD_BITS)|selectedMechRearmQuoteLow;
        rearmCost+=(uint32_t)quantityToBuy*ammunitionTransactionPrices[ammoFamily];
        selectedMechRearmQuoteLow=(uint16_t)rearmCost;
        selectedMechRearmQuoteHigh=(uint16_t)(rearmCost>>NATIVE_WORD_BITS);
        ++profile;quantityIndex+=CAMPAIGN_OWNED_UNIT_SLOTS;
    } while (--groupsRemaining!=0);
    if ((selectedMechRearmQuoteLow|selectedMechRearmQuoteHigh)==0)
    {
        MwNativeByteView text={(const uint8_t *)ammoBayFullText,0,sizeof(ammoBayFullText)};
        MwShowMonospaceTextPanel(REARM_FULL_MESSAGE_X,MARKET_MESSAGE_Y,REARM_FULL_MESSAGE_COLUMNS,REARM_FULL_MESSAGE_ROWS,&text,0);
        return 0;
    }
    EgaCopyDisplayToDrawPage(REARM_DIALOG_X,REARM_DIALOG_Y,REARM_DIALOG_WIDTH,REARM_DIALOG_HEIGHT);
    MwDrawDescriptorTextPanel(&rearmConfirmationPanel);
    uint32_t rearmCost=((uint32_t)selectedMechRearmQuoteHigh<<NATIVE_WORD_BITS)|selectedMechRearmQuoteLow;
    ++numericGroupingEnabled;
    /*09B2 completes its WORD line loop with CX0;3516 changes CL only. */
    MwDrawSignedDecimalField(rearmCost,REARM_QUOTE_X,1,0,TEXT_PANEL_FOREGROUND,TEXT_PANEL_BACKGROUND,REARM_QUOTE_Y);
    uint32_t funds=((uint32_t)campaignFundsHigh<<NATIVE_WORD_BITS)|campaignFundsLow;
    ++numericGroupingEnabled;
    MwDrawSignedDecimalField(funds,REARM_FUNDS_X,1,MARKET_PRICE_FIELD_DIGITS,TEXT_PANEL_FOREGROUND,TEXT_PANEL_BACKGROUND,REARM_FUNDS_Y);
    MwSelectUiRecordTable(0,&rearmConfirmationMenu);
    return CAMPAIGN_CONTINUE_UI;
}

/* MW_MAIN081B:476A..47E7. Reserve hold shop, not selected-Mech reloading.
 * POP discards the menu CALL return; entry charges one campaign time unit. */
uint16_t MwOpenReserveAmmunitionShop(void)
{
    MwAdvanceCampaignTimeAndChargeWages(1);
    reserveAmmoShopSelectedRow=0;unitInspectionContextCode=1;
    reserveAmmoSelling=0;reserveAmmoPreviewSelection=0;
    MwMarkOwnedAmmunitionCompatibleMechs();
    EgaSelectFont(TEXT_PANEL_COMPACT_FONT);MwDrawMechComplexBackground();
    for (uint16_t family=0;family<CAMPAIGN_AMMO_FAMILY_COUNT;++family)
        MwFormatReserveAmmunitionQuantity(family,reserveAmmoQuantitiesByFamily[family]);
    MwDrawDescriptorTextPanel(&reserveAmmoShopPanel);
    EgaCopyDisplayToDrawPage(0,0,RESERVE_AMMO_PREVIEW_WIDTH,RESERVE_AMMO_PREVIEW_HEIGHT);
    MwShowReserveAmmunitionCompatibility(0);
    MwDrawReserveAmmunitionPrices(false);MwDrawAmmunitionShopCompanyFunds();
    uiSelectionChangedCodeOffset=CAMPAIGN_CALLBACK_REFRESH_RESERVE_AMMO;
    MwInitializeMechServiceIdle();MwSelectUiRecordTable(reserveAmmoShopSelectedRow,&reserveAmmoShopMenu);
    return CAMPAIGN_CONTINUE_UI;
}

/* MW_MAIN081B:47E8..4821. Five digits, blank-padded except the last zero. */
void MwFormatReserveAmmunitionQuantity(uint16_t family,uint16_t quantity)
{
    if (family>=CAMPAIGN_AMMO_FAMILY_COUNT) abort();
    MwBuildDecimalDigitBuffer(quantity);
    bool leading=true;
    for (uint16_t digit=0;digit<5;++digit)
    {
        uint8_t value=numericDecimalDigits[DECIMAL_DIGIT_COUNT-5+digit];
        if (digit!=4 && value==0 && leading) reserveAmmoQuantityRows[family].text[digit]=ASCII_SPACE;
        else {leading=false;reserveAmmoQuantityRows[family].text[digit]=(char)(value+ASCII_DIGIT_ZERO);}
    }
}
/* MW_MAIN081B:4822/4827..4859. Two native entries choose buy/sell price rows. */
void MwDrawReserveAmmunitionPrices(bool selling)
{
    for (uint16_t family=0;family<CAMPAIGN_AMMO_FAMILY_COUNT;++family)
        MwDrawSignedDecimalField(ammunitionTransactionPrices[family+(selling?CAMPAIGN_AMMO_FAMILY_COUNT:0)],
            RESERVE_AMMO_PRICE_X,0,4,TEXT_PANEL_FOREGROUND,TEXT_PANEL_BACKGROUND,
            RESERVE_AMMO_ROW_Y+family*RESERVE_AMMO_ROW_HEIGHT);
}
/* MW_MAIN081B:485A..4877. Grouping stays enabled after drawing company funds. */
void MwDrawAmmunitionShopCompanyFunds(void)
{
    ++numericGroupingEnabled;
    MwDrawSignedDecimalField(((uint32_t)campaignFundsHigh<<NATIVE_WORD_BITS)|campaignFundsLow,
        0xA1,0,9,TEXT_PANEL_FOREGROUND,TEXT_PANEL_BACKGROUND,0xB8);
}
/* MW_MAIN081B:4878..48E5. Partial purchases survive insufficient C-bills.
 * Machine-gun cartridges trade in batches of ten, charged individually. */
void MwBuyReserveAmmunition(void)
{
    uiSelectionChangedCodeOffset=CAMPAIGN_CALLBACK_REFRESH_RESERVE_AMMO;MwInitializeMechServiceIdle();
    uint16_t family=selectedUiRecordIndex;
    if (family>=CAMPAIGN_AMMO_FAMILY_COUNT) abort();
    uint16_t remaining=family==RESERVE_AMMO_MACHINE_GUN_FAMILY?RESERVE_AMMO_MACHINE_GUN_BATCH:1;
    do
    {
        uint32_t funds=((uint32_t)campaignFundsHigh<<NATIVE_WORD_BITS)|campaignFundsLow;
        uint16_t price=ammunitionTransactionPrices[family];
        if (funds<price) break;
        funds-=price;campaignFundsLow=(uint16_t)funds;campaignFundsHigh=(uint16_t)(funds>>NATIVE_WORD_BITS);
        ++reserveAmmoQuantitiesByFamily[family];
    } while (--remaining);
    MwFormatReserveAmmunitionQuantity(family,reserveAmmoQuantitiesByFamily[family]);
    MwDrawSignedDecimalField(reserveAmmoQuantitiesByFamily[family],RESERVE_AMMO_QUANTITY_X,0,5,
        TEXT_PANEL_FOREGROUND,TEXT_PANEL_BACKGROUND,RESERVE_AMMO_ROW_Y+family*RESERVE_AMMO_ROW_HEIGHT);
    MwDrawAmmunitionShopCompanyFunds();
}
/* MW_MAIN081B:48E6..49D7, including DONE's49D8..49F1 restore tail. */
void MwRefreshReserveAmmunitionSelection(void)
{
    uint16_t row=selectedUiRecordIndex;
    if (row>=RESERVE_AMMO_MENU_ROWS) abort();
    bool selling=row>=CAMPAIGN_AMMO_FAMILY_COUNT && row!=RESERVE_AMMO_DONE_ROW;
    if (row!=RESERVE_AMMO_DONE_ROW && selling!=(reserveAmmoSelling!=0))
    {
        const char *label=selling?reserveAmmoSellLabel:reserveAmmoBuyLabel;
        MwNativeByteView text={(const uint8_t *)label,0,(uint32_t)strlen(label)+1};
        uint16_t y=0x6A,offset=0;EgaDrawString(0x89,&y,0x0408,&text,&offset);
        reserveAmmoSelling=selling;MwDrawReserveAmmunitionPrices(selling);
    }
    if (row==RESERVE_AMMO_DONE_ROW)
    {
        EgaCopyDrawPageToDisplay(0,0,RESERVE_AMMO_PREVIEW_WIDTH,RESERVE_AMMO_PREVIEW_HEIGHT);
        reserveAmmoPreviewSelection=UINT16_MAX;return;
    }
    uint16_t family=selling?(uint16_t)(row-CAMPAIGN_AMMO_FAMILY_COUNT):row;
    if (family==reserveAmmoPreviewSelection) return;
    if (reserveAmmoPreviewSelection==UINT16_MAX) MwShowReserveAmmunitionCompatibility(family);
    else
    {
        reserveAmmoPreviewSelection=row;
        EgaFillDisplayPageRectangle(6,6,0x58,0x32,TEXT_PANEL_BACKGROUND);
        MwDrawReserveAmmunitionCompatibleMechs(family);
    }
}
/* MW_MAIN081B:497B..49C7, shared rendering tail of48E6 and49C8. */
void MwDrawReserveAmmunitionCompatibleMechs(uint16_t family)
{
    if (family>=CAMPAIGN_AMMO_FAMILY_COUNT) abort();
    const char *heading=reserveAmmoCompatibilityHeadings[family].text;
    MwNativeByteView text={(const uint8_t *)heading,0,(uint32_t)strlen(heading)+1};
    EgaDrawTransparentText(0x0E,0x0C,TEXT_PANEL_FOREGROUND,&text,0);
    uint16_t y=0x13;
    for (uint16_t index=0;index<6;++index)
    {
        uint16_t kind=reserveAmmoCompatibleMechs[family][index];
        if (kind==UINT16_MAX) return;
        const char *name=unitDisplayNames[kind];
        text=(MwNativeByteView){(const uint8_t *)name,0,(uint32_t)strlen(name)+1};
        EgaDrawTransparentText(0x0E,y,reserveAmmoMechColours[kind],&text,0);y+=7;
    }
    abort();
}
/* MW_MAIN081B:49C8..49D6 jumps back into497B after drawing the frame. */
void MwShowReserveAmmunitionCompatibility(uint16_t family)
{
    if (family>=CAMPAIGN_AMMO_FAMILY_COUNT) abort();
    reserveAmmoPreviewSelection=family;
    MwDrawDescriptorTextPanel(&reserveAmmoCompatibilityPanels[family]);
    MwDrawReserveAmmunitionCompatibleMechs(family);
}
/* MW_MAIN081B:49F2..4A76. Selling ends by reselecting the live menu row. */
uint16_t MwSellReserveAmmunition(void)
{
    uiSelectionChangedCodeOffset=CAMPAIGN_CALLBACK_REFRESH_RESERVE_AMMO;MwInitializeMechServiceIdle();
    uint16_t family=(uint16_t)(selectedUiRecordIndex-CAMPAIGN_AMMO_FAMILY_COUNT);
    if (family>=CAMPAIGN_AMMO_FAMILY_COUNT) abort();
    uint16_t remaining=family==RESERVE_AMMO_MACHINE_GUN_FAMILY?RESERVE_AMMO_MACHINE_GUN_BATCH:1;
    do
    {
        if (!reserveAmmoQuantitiesByFamily[family]) break;
        uint32_t funds=((uint32_t)campaignFundsHigh<<NATIVE_WORD_BITS)|campaignFundsLow;
        funds+=ammunitionTransactionPrices[family+CAMPAIGN_AMMO_FAMILY_COUNT];
        campaignFundsLow=(uint16_t)funds;campaignFundsHigh=(uint16_t)(funds>>NATIVE_WORD_BITS);
        --reserveAmmoQuantitiesByFamily[family];
    } while (--remaining);
    MwFormatReserveAmmunitionQuantity(family,reserveAmmoQuantitiesByFamily[family]);
    MwDrawSignedDecimalField(reserveAmmoQuantitiesByFamily[family],RESERVE_AMMO_QUANTITY_X,0,5,
        TEXT_PANEL_FOREGROUND,TEXT_PANEL_BACKGROUND,RESERVE_AMMO_ROW_Y+family*RESERVE_AMMO_ROW_HEIGHT);
    MwDrawAmmunitionShopCompanyFunds();MwSelectUiRecordTable(selectedUiRecordIndex,&reserveAmmoShopMenu);
    return CAMPAIGN_CONTINUE_UI;
}
/* MW_MAIN081B:4A77..4AA1. Highlight chassis already in the company roster. */
void MwMarkOwnedAmmunitionCompatibleMechs(void)
{
    memset(reserveAmmoMechColours,TEXT_PANEL_FOREGROUND,sizeof(reserveAmmoMechColours));
    for (uint16_t slot=0;slot<CAMPAIGN_OWNED_UNIT_SLOTS;++slot)
        if (ownedUnitKinds[slot]!=UINT16_MAX)
        {
            uint16_t kind=ownedUnitKinds[slot];if (kind>=sizeof(reserveAmmoMechColours)) abort();
            reserveAmmoMechColours[kind]=UI_SELECTED_TEXT_COLOUR;
        }
}
