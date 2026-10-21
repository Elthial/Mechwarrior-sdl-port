#include "mechwarrior/port_diagnostics.h"
#include "mechwarrior/campaign_continuations.h"
#include "mechwarrior/campaign_service.h"
#include "mechwarrior/ega_campaign.h"
#include "mechwarrior/campaign_input.h"
#include <stdlib.h>
#include <string.h>

/* MW_MAIN081B:3EE8..3FFC. Early RET or JMP38FC. Saved system/weapon AX
 * retains AH from BYTE slot*001D; body path explicitly clears it. */
uint16_t MwRepairSelectedMechComponentCallback(void)
{
    repairMenuSelectedCategory=selectedUiRecordIndex;
    uiSelectionChangedCodeOffset=CAMPAIGN_CALLBACK_REFRESH_COMPONENT_QUOTE;
    uint16_t slotProduct=(uint8_t)selectedOwnedUnitSlot*CAMPAIGN_DAMAGE_BYTES;
    CampaignUnitDamage *damage=&ownedUnitDamage[(uint8_t)selectedOwnedUnitSlot];
    uint8_t *damagedComponent=(uint8_t *)damage+selectedUiRecordIndex;
    uint16_t savedDamage;
    if (selectedUiRecordIndex==REPAIR_BODY)
    {
        damagedComponent=damage->bodyDamageTiers;
        uint16_t remaining=CAMPAIGN_BODY_LOCATION_COUNT;
        while (*damagedComponent==0)
        {++damagedComponent;if (--remaining==0) return 0;}
        savedDamage=*damagedComponent;
    }
    else
    {
        if (selectedUiRecordIndex>REPAIR_BODY) --damagedComponent;
        uint8_t tier=*damagedComponent;
        if (tier==0) return 0;
        savedDamage=(slotProduct&0xFF00)|tier;
        uint16_t damageIndex=selectedUiRecordIndex;
        if (selectedUiRecordIndex>REPAIR_BODY) --damageIndex;
        bool failed=repairAttemptFailuresByKind[(uint8_t)selectedOwnedUnitKind].failedByDamageIndex[damageIndex]!=0;
        if (!failed)
        {
            uint16_t roll=MwRandomUpTo(REPAIR_ATTEMPT_ROLL_MAXIMUM);
            uint16_t marketTier=currentPlanetMarketTier;
            uint8_t limit=repairSuccessRollLimitsByMarketTier[marketTier];
            if (limit==0 || roll/limit>UINT8_MAX) /* Port emulation of a native CPU fault. */ MW_NATIVE_FAULT("Rejected condition: limit==0 || roll/limit>UINT8_MAX"); /* Native DIV BYTE fault. */
            failed=roll/limit!=0;
        }
        if (failed)
        {
            uint16_t category=selectedUiRecordIndex;
            if (category!=REPAIR_BODY)
            {
                damageIndex=category;
                if (category>REPAIR_BODY) --damageIndex;
                repairAttemptFailuresByKind[(uint8_t)selectedOwnedUnitKind].failedByDamageIndex[damageIndex]=1;
            }
            MwNativeByteView text={(const uint8_t *)repairFailedText,0,sizeof(repairFailedText)};
            MwShowMonospaceTextPanel(REPAIR_MESSAGE_X,REPAIR_MESSAGE_Y,REPAIR_FAILURE_COLUMNS,REPAIR_MESSAGE_ROWS,&text,0);
            return 0;
        }
    }
    uint32_t funds=((uint32_t)campaignFundsHigh<<NATIVE_WORD_BITS)|campaignFundsLow;
    uint32_t price=((uint32_t)componentRepairPriceHigh<<NATIVE_WORD_BITS)|componentRepairPriceLow;
    if (funds<price) return 0; /* Existing highlighted quote, not a recomputation. */
    funds-=price;campaignFundsLow=(uint16_t)funds;campaignFundsHigh=(uint16_t)(funds>>NATIVE_WORD_BITS);
    /* Native3FC1 also loads CX from3A6F;0C3E consumes saved AX, not CX.
     * That discarded ordinary-RAM read is not represented as a C lookup. */
    MwAdvanceCampaignTimeAndChargeWages(savedDamage);
    if (selectedUiRecordIndex!=REPAIR_BODY)
    {
        --*damagedComponent; /* Captured address, even if calendar changed its BYTE to zero. */
        return CAMPAIGN_CONTINUE_COMPONENT_REPAIR_MENU;
    }
    damagedComponent=ownedUnitDamage[(uint8_t)selectedOwnedUnitSlot].bodyDamageTiers;
    for (uint16_t location=0;location<CAMPAIGN_BODY_LOCATION_COUNT;++location,++damagedComponent)
        if (*damagedComponent!=0)
        {
            *damagedComponent=0;
            MwRefreshSelectedMechDetailPicture();
            return CAMPAIGN_CONTINUE_COMPONENT_REPAIR_MENU;
        }
    return 0; /* Calendar left no live body damage: payment/time remain spent. */
}
/* MW_MAIN081B:3802..38FA. Early RET; successful work JMP3091. No calendar. */
uint16_t MwRepairAllSelectedMechComponentsCallback(void)
{
    mechServiceMenuSelectedRow=MECH_SERVICE_ROW_REPAIR_ALL;
    uint32_t quote=((uint32_t)unitRepairQuoteHigh<<NATIVE_WORD_BITS)|unitRepairQuoteLow;
    if (quote==0)
    {
        MwNativeByteView text={(const uint8_t *)noRepairWorkText,0,sizeof(noRepairWorkText)};
        MwShowMonospaceTextPanel(REPAIR_MESSAGE_X,REPAIR_MESSAGE_Y,REPAIR_NO_WORK_COLUMNS,REPAIR_MESSAGE_ROWS,&text,0);
        return 0;
    }
    uint32_t funds=((uint32_t)campaignFundsHigh<<NATIVE_WORD_BITS)|campaignFundsLow;
    if (funds<quote)
    {
        MwNativeByteView text={(const uint8_t *)cannotAffordRepairText,0,sizeof(cannotAffordRepairText)};
        MwShowMonospaceTextPanel(REPAIR_MESSAGE_X,REPAIR_MESSAGE_Y,REPAIR_CANNOT_AFFORD_COLUMNS,REPAIR_CANNOT_AFFORD_ROWS,&text,0);
        return 0;
    }
    funds-=quote;campaignFundsLow=(uint16_t)funds;campaignFundsHigh=(uint16_t)(funds>>NATIVE_WORD_BITS);
    CampaignUnitDamage *damage=&ownedUnitDamage[(uint8_t)selectedOwnedUnitSlot];
    uint8_t *damagedComponent=(uint8_t *)damage;
    CampaignRepairAttemptRow *failedRepairs=&repairAttemptFailuresByKind[(uint8_t)selectedOwnedUnitKind];
    repairWorkIncomplete=0;
    for (uint16_t damageIndex=0;damageIndex<CAMPAIGN_DAMAGE_BYTES;++damageIndex,++damagedComponent)
        if (*damagedComponent!=0)
        {
            bool failed=false;
            if ((int16_t)damageIndex<CAMPAIGN_BODY_DAMAGE_OFFSET)
            {
                failed=failedRepairs->failedByDamageIndex[damageIndex]==1;
                if (!failed)
                {
                    uint16_t roll=MwRandomUpTo(REPAIR_ATTEMPT_ROLL_MAXIMUM);
                    uint16_t marketTier=currentPlanetMarketTier;
                    uint8_t limit=repairSuccessRollLimitsByMarketTier[marketTier];
                    if (limit==0 || roll/limit>UINT8_MAX) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: limit==0 || roll/limit>UINT8_MAX");
                    if (roll/limit!=0)
                    {
                        failedRepairs->failedByDamageIndex[damageIndex]=1;
                        failed=true;
                    }
                }
            }
            if (failed) repairWorkIncomplete=1;
            else
            {
                *damagedComponent=0;
            }
        }
    damage=&ownedUnitDamage[(uint8_t)selectedOwnedUnitSlot];
    MwQuoteMechRepairTotal(selectedOwnedUnitKind,damage);
    uint32_t refund=((uint32_t)unitRepairQuoteHigh<<NATIVE_WORD_BITS)|unitRepairQuoteLow;
    funds=((uint32_t)campaignFundsHigh<<NATIVE_WORD_BITS)|campaignFundsLow;
    funds+=refund;campaignFundsLow=(uint16_t)funds;campaignFundsHigh=(uint16_t)(funds>>NATIVE_WORD_BITS);
    MwRefreshSelectedMechDetailPicture();
    const char *message=repairWorkIncomplete==0?repairCompleteText:repairIncompleteText;
    MwNativeByteView text={(const uint8_t *)message,0,(uint32_t)strlen(message)+1};
    MwTextPanelReturn shown=MwShowMonospaceTextPanel(REPAIR_MESSAGE_X,REPAIR_MESSAGE_Y,REPAIR_RESULT_COLUMNS,REPAIR_MESSAGE_ROWS,&text,0);
    return shown.discardReturnWord?0:CAMPAIGN_CONTINUE_RETURN_TO_OWNED_MECH; /* Native100C can skip38F8's JMP3091. */
}

/* MW_MAIN081B:38FC..3BF6. Discards the dispatch IP; all3946..3BEB entries
 * are fallthroughs in this same component-menu body. */
uint16_t MwOpenMechComponentRepairMenuCallback(void)
{
    mechServiceMenuSelectedRow=MECH_SERVICE_ROW_COMPONENT_REPAIR;
    EgaSelectFont(TEXT_PANEL_COMPACT_FONT);
    uint16_t kind=selectedOwnedUnitKind;
    uint16_t titleCursor=0;
    while (titleCursor!=repairTitleLeadingSpacesByKind[kind])
        repairTitle.text[titleCursor++]=ASCII_SPACE;
    strcpy(repairTitle.text+titleCursor,unitDisplayNames[kind]);
    CampaignUnitDamage *damage=&ownedUnitDamage[(uint8_t)selectedOwnedUnitSlot];
    uint8_t *damagedComponent=(uint8_t *)damage;
    repairSystemRowIndex=0;
    do
    {
        uint8_t tier=*damagedComponent;
        uint16_t category=repairSystemRowIndex;
        MwComponentConditionTextRow *row=&repairSystemConditionRows[category];
        row->display.colour=tier==0?REPAIR_CONDITION_HEALTHY_COLOUR:REPAIR_CONDITION_DAMAGED_COLOUR;
        if (category==REPAIR_HEAT_SINKS)
        {
            uint16_t kind=selectedOwnedUnitKind;
            uint8_t maximum=unitHeatSinkCounts[kind];
            MwBuildDecimalDigitBuffer((uint8_t)(maximum-tier));
            row->caption.columns.condition[0]=numericDecimalDigits[8]==0?ASCII_SPACE:numericDecimalDigits[8]+ASCII_DIGIT_ZERO;
            row->caption.columns.condition[1]=numericDecimalDigits[9]+ASCII_DIGIT_ZERO;
            kind=selectedOwnedUnitKind;
            maximum=unitHeatSinkCounts[kind];
            MwBuildDecimalDigitBuffer(maximum);
            row->caption.columns.condition[6]=numericDecimalDigits[8]+ASCII_DIGIT_ZERO;
            row->caption.columns.condition[7]=numericDecimalDigits[9]+ASCII_DIGIT_ZERO;
        }
        else if (category==REPAIR_JUMP_JETS)
        {
            for (uint16_t letter=0;letter<sizeof(repairJumpJetCountTemplateBytes);++letter)
                row->caption.columns.condition[letter]=repairJumpJetCountTemplateBytes[letter];
            uint16_t kind=selectedOwnedUnitKind;
            uint8_t maximum=unitJumpJetGroupCounts[kind];
            MwBuildDecimalDigitBuffer((uint8_t)(maximum-tier));
            row->caption.columns.condition[1]=numericDecimalDigits[9]+ASCII_DIGIT_ZERO;
            kind=selectedOwnedUnitKind;
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
        ++repairSystemRowIndex;++damagedComponent;
    } while (repairSystemRowIndex!=CAMPAIGN_SYSTEM_COUNT);
    damagedComponent=ownedUnitDamage[(uint8_t)selectedOwnedUnitSlot].bodyDamageTiers;
    uint8_t bodyDamage=0;
    for (uint16_t location=0;location<CAMPAIGN_BODY_LOCATION_COUNT;++location,++damagedComponent)
        bodyDamage+=*damagedComponent&3;
    repairBodyCondition.display.colour=bodyDamage==0?REPAIR_CONDITION_HEALTHY_COLOUR:REPAIR_CONDITION_DAMAGED_COLOUR;
    uint16_t bodyProtection=(uint16_t)(REPAIR_BODY_TOTAL_TIERS-bodyDamage)*PERCENT_DIVISOR/REPAIR_BODY_TOTAL_TIERS;
    MwBuildDecimalDigitBuffer(bodyProtection);
    repairBodyCondition.caption.columns.protectionPercent[0]=numericDecimalDigits[7]==0?ASCII_SPACE:numericDecimalDigits[7]+ASCII_DIGIT_ZERO;
    repairBodyCondition.caption.columns.protectionPercent[1]=numericDecimalDigits[8]==0 && numericDecimalDigits[7]==0?ASCII_SPACE:numericDecimalDigits[8]+ASCII_DIGIT_ZERO;
    repairBodyCondition.caption.columns.protectionPercent[2]=numericDecimalDigits[9]+ASCII_DIGIT_ZERO;
    uint16_t y=REPAIR_WEAPON_FIRST_Y;
    for (uint16_t mount=0;mount<CAMPAIGN_WEAPON_SLOTS;++mount)
    {
        repairWeaponConditionRows[mount].display.x=REPAIR_WEAPON_MENU_X;
        repairWeaponConditionRows[mount].display.y=y;
        MwUiRecord *record=&componentRepairUiRecords[REPAIR_FIRST_WEAPON+mount];
        record->x=REPAIR_WEAPON_MENU_X;record->y=y;
        record->activateCodeOffset=CAMPAIGN_CALLBACK_REPAIR_MECH_COMPONENT;
        y+=REPAIR_WEAPON_ROW_HEIGHT;
    }
    uint8_t previous=REPAIR_BODY,next=REPAIR_FIRST_WEAPON+1;
    for (uint16_t mount=0;mount<CAMPAIGN_WEAPON_SLOTS+1;++mount)
    {
        MwUiRecord *record=&componentRepairUiRecords[REPAIR_FIRST_WEAPON+mount];
        record->upRecordIndex=previous++;record->downRecordIndex=next++;
    }
    kind=selectedOwnedUnitKind;
    uint16_t label=0;
    while (repairWeaponLabels[kind][label]!=NULL)
    {
        /* Fourteen padded characters plus NUL temporarily overwrite the first
         * two condition columns; the following damage-label copy overlays them. */
        strcpy(repairWeaponConditionRows[label].caption.text,repairWeaponLabels[kind][label]);
        ++label;
    }
    repairWeaponConditionRows[label-1].display.x=REPAIR_DONE_X;
    repairWeaponConditionRows[label-1].display.y=REPAIR_DONE_Y;
    kind=selectedOwnedUnitKind;
    uint16_t mountsRemaining=unitWeaponCounts[kind];
    damagedComponent=ownedUnitDamage[(uint8_t)selectedOwnedUnitSlot].weaponDamageTiers;
    uint16_t mount=0;
    do
    {
        if (mount>=CAMPAIGN_WEAPON_SLOTS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: mount>=CAMPAIGN_WEAPON_SLOTS"); /* Zero underflows natively; never a zero-weapon no-op. */
        uint8_t tier=*damagedComponent++;
        MwComponentConditionTextRow *row=&repairWeaponConditionRows[mount++];
        row->display.colour=tier==0?REPAIR_CONDITION_HEALTHY_COLOUR:REPAIR_CONDITION_DAMAGED_COLOUR;
        strcpy(row->caption.text+REPAIR_COMPONENT_LABEL_COLUMNS,campaignDamageTierLabels[tier]);
    } while (--mountsRemaining!=0);
    kind=selectedOwnedUnitKind;
    uint16_t doneRow=unitWeaponCounts[kind]+REPAIR_FIRST_WEAPON;
    componentRepairUiRecords[0].upRecordIndex=(uint8_t)doneRow;
    MwUiRecord *done=&componentRepairUiRecords[doneRow];
    done->x=REPAIR_DONE_X;done->y=REPAIR_DONE_Y;
    done->activateCodeOffset=CAMPAIGN_CONTINUE_RETURN_TO_OWNED_MECH;
    done->downRecordIndex=0;done->leftRecordIndex=0; /* Native WORD store at+0B. */
    kind=selectedOwnedUnitKind;
    componentRepairPanel.lineCount=unitWeaponCounts[kind]+REPAIR_FIRST_WEAPON+3;
    EgaFillDisplayPageRectangle(REPAIR_PANEL_X,REPAIR_PANEL_Y,REPAIR_PANEL_WIDTH,REPAIR_PANEL_HEIGHT,MECH_DETAIL_BACKGROUND);
    MwDrawDescriptorTextPanel(&componentRepairPanel);
    uiSelectionChangedCodeOffset=CAMPAIGN_CALLBACK_REFRESH_COMPONENT_QUOTE;
    uint32_t funds=((uint32_t)campaignFundsHigh<<NATIVE_WORD_BITS)|campaignFundsLow;
    ++numericGroupingEnabled;
    MwDrawSignedDecimalField(funds,REPAIR_FUNDS_X,1,0,REPAIR_CONDITION_HEALTHY_COLOUR,MECH_DETAIL_BACKGROUND,REPAIR_FUNDS_Y);
    MwSelectUiRecordTable(repairMenuSelectedCategory,&componentRepairMenu);
    return CAMPAIGN_CONTINUE_UI;
}
/* MW_MAIN081B:3DF3..3E27. Ordinary NEAR selection-change callback, no args. */
void MwRefreshSelectedComponentRepairQuote(void)
{
    repairPriceCategory=selectedUiRecordIndex;
    CampaignUnitDamage *damage=&ownedUnitDamage[(uint8_t)selectedOwnedUnitSlot];
    MwQuoteRepairComponent(damage);
    uint32_t quote=((uint32_t)componentRepairPriceHigh<<NATIVE_WORD_BITS)|componentRepairPriceLow;
    ++numericGroupingEnabled;
    MwDrawSignedDecimalField(quote,REPAIR_COMPONENT_PRICE_X,0,REPAIR_PRICE_DIGITS,REPAIR_CONDITION_HEALTHY_COLOUR,MECH_DETAIL_BACKGROUND,REPAIR_COMPONENT_PRICE_Y);
}

/* MW_MAIN081B:3063..3090; native shared detail entry, JMP30A2. */
uint16_t MwOpenSelectedMechDetailContinuation(void)
{
    EgaSelectFont(TEXT_PANEL_COMPACT_FONT);
    EgaClearDrawPage(MECH_DETAIL_BACKGROUND);
    EgaCopyDrawPageToDisplay(0,0,EGA_COLUMNS,EGA_ROWS);
    MwDrawMechInspectionGrid();
    MwRefreshSelectedMechDetailPicture();
    return CAMPAIGN_CONTINUE_OWNED_MECH_DETAIL;
}
/* MW_MAIN081B:3091..30A1. Font/reset precede the discarded callback IP. */
uint16_t MwReturnToSelectedMechDetailCallback(void)
{
    EgaSelectFont(TEXT_PANEL_COMPACT_FONT);
    repairMenuSelectedCategory=0;
    return CAMPAIGN_CONTINUE_OWNED_MECH_DETAIL;
}
/* MW_MAIN081B:30A2..32B9, including shared3213/3291/32AE labels. No additional
 * callee boundaries for fallthroughs. Returned WORD is native JMP transport. */
uint16_t MwDrawSelectedMechDetailContinuation(uint16_t entryOffset)
{
    if (entryOffset==CAMPAIGN_CONTINUE_OWNED_MECH_SERVICE_MENU) goto serviceMenu; /* Shared cancel tail; not a new native argument. */
    MwDrawDescriptorTextPanel(selectedOwnedUnitKind==CAMPAIGN_UNIT_DROPSHIP
        ?&dropShipStatusPanel:&mechStatusPanel);
    MwDrawPicturePanelBorder(0,0,EGA_COLUMNS,EGA_ROWS);
    selectedOwnedUnitKind=ownedUnitKinds[selectedOwnedUnitByteOffset/2];
    const char *mechName=unitDisplayNames[selectedOwnedUnitKind];
    MwNativeByteView name={(const uint8_t *)mechName,0,(uint32_t)strlen(mechName)+1};
    EgaDrawTransparentText(MECH_DETAIL_NAME_X,MECH_DETAIL_NAME_Y,MECH_DETAIL_TEXT_COLOUR,&name,0);
    uint16_t kind=selectedOwnedUnitKind;
    CampaignUnitDamage *damage=&ownedUnitDamage[(uint8_t)selectedOwnedUnitSlot];
    /*30D9..313A's damage sum is discarded by MOV AX at313D, not a condition score. */
    const char *conditionText=(const char *)mechNonfunctionalTextBytes;
    if (damage->engineHits<=MECH_FUNCTIONAL_MAXIMUM_TIER && damage->gyroDamageTier<=MECH_FUNCTIONAL_MAXIMUM_TIER
        && damage->lifeSupportDamageTier<=MECH_FUNCTIONAL_MAXIMUM_TIER
        && damage->actuatorDamageTiers[2]<=MECH_FUNCTIONAL_MAXIMUM_TIER
        && damage->actuatorDamageTiers[3]<=MECH_FUNCTIONAL_MAXIMUM_TIER) conditionText=mechFunctionalTextBytes;
    MwNativeByteView condition={(const uint8_t *)conditionText,0,(uint32_t)strlen(conditionText)+1};
    EgaDrawTransparentText(MECH_DETAIL_CONDITION_X,MECH_DETAIL_CONDITION_Y,MECH_DETAIL_TEXT_COLOUR,&condition,0);
    if (selectedOwnedUnitKind==CAMPAIGN_UNIT_DROPSHIP)
    { MwSelectUiRecordTable(0,&dropShipServiceMenu);return CAMPAIGN_CONTINUE_UI; } /* JMP32AE skips3291's font select. */
    damage=&ownedUnitDamage[(uint8_t)selectedOwnedUnitSlot];
    MwQuoteMechRepairTotal(selectedOwnedUnitKind,damage);
    uint32_t quote=((uint32_t)unitRepairQuoteHigh<<NATIVE_WORD_BITS)|unitRepairQuoteLow;
    numericGroupingEnabled=1;
    /*3763 ends with CX0; decimal division and saved-CX glyph calls retain CH0.
     * Native callers set CL1 only. No guessed fixed-width formatting. */
    MwDrawSignedDecimalField(quote,MECH_DETAIL_REPAIR_PRICE_X,1,0,MECH_DETAIL_TEXT_COLOUR,MECH_DETAIL_BACKGROUND,MECH_DETAIL_REPAIR_PRICE_Y);
    kind=selectedOwnedUnitKind;
    uint8_t tonnage=unitDisplayTonnages[kind];
    MwDrawSignedDecimalField(tonnage,MECH_DETAIL_TONNAGE_X,1,0,MECH_DETAIL_TEXT_COLOUR,MECH_DETAIL_BACKGROUND,MECH_DETAIL_TONNAGE_Y);
    kind=selectedOwnedUnitKind;
    uint8_t speed=unitDisplaySpeedsKph[kind];
    MwDrawSignedDecimalField(speed,MECH_DETAIL_SPEED_X,1,0,MECH_DETAIL_TEXT_COLOUR,MECH_DETAIL_BACKGROUND,MECH_DETAIL_SPEED_Y);
    kind=selectedOwnedUnitKind;
    uint8_t jumpCapacity=unitDisplayJumpCapacitiesMetres[kind];
    MwDrawSignedDecimalField(jumpCapacity,MECH_DETAIL_JUMP_CAPACITY_X,1,0,MECH_DETAIL_TEXT_COLOUR,MECH_DETAIL_BACKGROUND,MECH_DETAIL_JUMP_CAPACITY_Y);
    /* The native store order snapshots all families before any quantities. */
    for (uint16_t group=0;group<CAMPAIGN_AMMO_GROUPS;++group)
        selectedMechAmmoFamilies[group]=ownedAmmoKindsByGroup[selectedOwnedUnitSlot+group*CAMPAIGN_OWNED_UNIT_SLOTS];
    for (uint16_t group=0;group<CAMPAIGN_AMMO_GROUPS;++group)
        selectedMechAmmoQuantities[group]=ownedAmmoQuantitiesByGroup[selectedOwnedUnitSlot+group*CAMPAIGN_OWNED_UNIT_SLOTS];
    uint16_t group=0;
    do
    {
        if (group>=CAMPAIGN_AMMO_GROUPS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: group>=CAMPAIGN_AMMO_GROUPS"); /* Unconverted adjacent reads, not a native zero-count no-op. */
        uint16_t family=selectedMechAmmoFamilies[group];
        uint16_t quantity=selectedMechAmmoQuantities[group];
        const char *ammoName=ammoFamilyDisplayNames[family];
        uint16_t y=MECH_DETAIL_AMMO_FIRST_Y+(uint16_t)(group*MECH_DETAIL_AMMO_ROW_HEIGHT);
        MwNativeByteView text={(const uint8_t *)ammoName,0,(uint32_t)strlen(ammoName)+1};
        EgaDrawTransparentText(MECH_DETAIL_AMMO_LABEL_X,y,MECH_DETAIL_TEXT_COLOUR,&text,0);
        MwDrawSignedDecimalField(quantity,MECH_DETAIL_AMMO_QUANTITY_X,1,0,MECH_DETAIL_TEXT_COLOUR,MECH_DETAIL_BACKGROUND,y);
        ++group;
        kind=selectedOwnedUnitKind;
    } while ((uint8_t)group!=unitAmmoGroupCounts[kind]);
serviceMenu:
    EgaSelectFont(TEXT_PANEL_COMPACT_FONT);
    if (selectedOwnedUnitKind==CAMPAIGN_UNIT_DROPSHIP) MwSelectUiRecordTable(0,&dropShipServiceMenu);
    else MwSelectUiRecordTable(mechServiceMenuSelectedRow,&mechServiceMenu);
    return CAMPAIGN_CONTINUE_UI;
}

/* MW_MAIN081B:362F..3700. Inclusive grid lines; capture display into draw. */
void MwDrawMechInspectionGrid(void)
{
    EgaFillDisplayPageRectangle(INSPECTION_GRID_X,0,INSPECTION_GRID_WIDTH,INSPECTION_GRID_LAST_Y,INSPECTION_GRID_BACKGROUND);
    EgaDrawLine(INSPECTION_GRID_LINE_COLOUR,0,INSPECTION_GRID_LAST_X,INSPECTION_GRID_X,0);
    EgaDrawLine(INSPECTION_GRID_LINE_COLOUR,INSPECTION_GRID_LAST_Y,INSPECTION_GRID_LAST_X,INSPECTION_GRID_X,INSPECTION_GRID_LAST_Y);
    EgaDrawLine(INSPECTION_GRID_LINE_COLOUR,0,INSPECTION_GRID_X,INSPECTION_GRID_X,INSPECTION_GRID_LAST_Y);
    EgaDrawLine(INSPECTION_GRID_LINE_COLOUR,0,INSPECTION_GRID_LAST_X,INSPECTION_GRID_LAST_X,INSPECTION_GRID_LAST_Y);
    uint16_t x=INSPECTION_GRID_X;
    do
    {
        x+=INSPECTION_GRID_SPACING;
        EgaDrawLine(INSPECTION_GRID_LINE_COLOUR,0,x,x,INSPECTION_GRID_LAST_Y);
    } while (x!=INSPECTION_GRID_LAST_INTERIOR_X);
    uint16_t y=0;
    do
    {
        y+=INSPECTION_GRID_SPACING;
        EgaDrawLine(INSPECTION_GRID_LINE_COLOUR,y,INSPECTION_GRID_LAST_X,INSPECTION_GRID_X,y);
    } while (y!=INSPECTION_GRID_LAST_INTERIOR_Y);
    EgaCopyDisplayToDrawPage(INSPECTION_CAPTURE_X,0,INSPECTION_CAPTURE_WIDTH,EGA_ROWS);
}
/* MW_MAIN081B:400C..407B. Native background plus eight visible body regions.
 * Rear CT advances both cursors without reading its damage byte. */
void MwDrawMechBodyDamageDiagram(uint16_t unitKind, CampaignUnitDamage *damage)
{
    mechDiagramUnitKind=unitKind;
    mechDiagramBodyDamage=damage->bodyDamageTiers;
    const MwMechBodyDiagramLayout *layout=mechBodyDiagramLayoutsByKind[unitKind];
    if (!layout) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !layout"); /* Native null-directory aliases have not been reconstructed. */
    mechDiagramRectangle=(MwMechDiagramRectangle *)&layout->background;
    EgaFillDrawPageRectangle(layout->background.x,layout->background.y,
        layout->background.width,layout->background.height,MECH_BODY_DIAGRAM_BACKGROUND);
    for (uint16_t location=0;location<CAMPAIGN_BODY_LOCATION_COUNT;++location)
    {
        mechDiagramRectangle=(MwMechDiagramRectangle *)&layout->bodyRegions[location];
        const uint8_t *tier=mechDiagramBodyDamage++;
        if (location==CAMPAIGN_BODY_REAR_CENTRE_TORSO) continue;
        if (*tier!=0)
        {
            const MwMechDiagramRectangle *rectangle=mechDiagramRectangle;
            EgaFillDrawPageRectangle(rectangle->x,rectangle->y,rectangle->width,rectangle->height,bodyDamageTierFillColours[*tier]);
        }
    }
    mechDiagramRectangle=(MwMechDiagramRectangle *)&layout->bodyRegions[CAMPAIGN_BODY_LOCATION_COUNT];
}
/* MW_MAIN081B:3701..3762. Capture origins before loading, reload live kind
 * after diagram drawing. Only the native publication rectangle is copied. */
void MwRefreshSelectedMechDetailPicture(void)
{
    if (selectedOwnedUnitKind!=CAMPAIGN_UNIT_DROPSHIP)
        MwDrawMechBodyDamageDiagram(selectedOwnedUnitKind,
            &ownedUnitDamage[(uint8_t)selectedOwnedUnitSlot]);
    uint16_t kind=selectedOwnedUnitKind;
    uint16_t x=unitDetailPictureXOffsets[kind];
    x-=DETAIL_PICTURE_X_INSET;
    uint16_t y=unitDetailPictureYOrigins[kind];
    x+=INSPECTION_GRID_X;
    uint16_t picture=kind==CAMPAIGN_UNIT_DROPSHIP?DROPSHIP_DETAIL_PICTURE:(uint16_t)(kind+MECH_DETAIL_PICTURE_BASE);
    uint16_t selector=MwLoadIndexedPicture(picture);
    EgaDrawPackedSprite(x,y,EGA_DRAW_PAGE,0,selector);
    EgaCopyDrawPageToDisplay(INSPECTION_CAPTURE_X,INSPECTION_PUBLISH_Y,INSPECTION_PUBLISH_WIDTH,INSPECTION_PUBLISH_HEIGHT);
}

/* MW_MAIN081B:2E07..2E27. Native background picture, then draw->display copy. */
void MwDrawMechComplexBackground(void)
{
    MwDrawActivePicture(0,0,EGA_DRAW_PAGE,2);
    EgaCopyDrawPageToDisplay(0,0,EGA_COLUMNS,EGA_ROWS);
}
/* MW_MAIN081B:2E28..2F31.2E47/2ED8 are fallthrough labels, not extra callees.
 * Native nonempty entry POPs its callback return and eventually JMPs089B. */
uint16_t MwOpenOwnedMechList(void)
{
    if (ownedUnitCount==0) return 0;
    unitInspectionContextCode=0;mechServiceMenuSelectedRow=0;
    EgaSelectFont(TEXT_PANEL_MONOSPACE_FONT);
    uint16_t ownedSlot=0,visibleRow=0;
    do
    {
        while (ownedSlot<CAMPAIGN_OWNED_UNIT_SLOTS && ownedUnitKinds[ownedSlot]==CAMPAIGN_VACANT_UNIT_KIND) ++ownedSlot;
        if (ownedSlot>=CAMPAIGN_OWNED_UNIT_SLOTS || visibleRow>=CAMPAIGN_OWNED_UNIT_SLOTS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: ownedSlot>=CAMPAIGN_OWNED_UNIT_SLOTS || visibleRow>=CAMPAIGN_OWNED_UNIT_SLOTS"); /* Unconverted native overrun. */
        ownedListRowUnitByteOffset=ownedSlot*2;
        MwOwnedUnitListRow *row=&ownedUnitListRows[visibleRow];
        memcpy(row->caption.columns.mechName,shopUnitNames[ownedUnitKinds[ownedSlot]],sizeof(row->caption.columns.mechName));
        uint16_t pilotSlot=0;
        while (pilotSlot<CAMPAIGN_ROSTER_SLOTS && rosterMechAssignments[pilotSlot]!=ownedSlot) ++pilotSlot;
        if (pilotSlot==CAMPAIGN_ROSTER_SLOTS)
            memcpy(row->caption.columns.mechWarriorName,unassignedMechWarriorText,sizeof(row->caption.columns.mechWarriorName));
        else
        {
            uint16_t pilotId=rosterPilotIds[pilotSlot];
            if (pilotId==0) memcpy(row->caption.columns.mechWarriorName,commanderNameBytes,sizeof(row->caption.columns.mechWarriorName));
            else
            {
                uint16_t pilotOffset=pilotDefinitionOffsets[pilotId]-PILOT_DEFINITIONS_START;
                const MwPilotDefinitionPrefix *pilot=(const MwPilotDefinitionPrefix *)&pilotDefinitionStorage[pilotOffset];
                memcpy(row->caption.columns.mechWarriorName,pilot->nameBytes,sizeof(row->caption.columns.mechWarriorName));
            }
        }
        ++ownedSlot;++visibleRow;
    } while (visibleRow!=ownedUnitCount);
    memcpy(ownedUnitListRows[visibleRow].caption.text,ownedListDoneText,sizeof(ownedListDoneText));
    uint8_t previousRow=0,nextRow=2;
    for (uint16_t row=1;row<CAMPAIGN_OWNED_UNIT_SLOTS;++row)
    {
        ownedUnitListUiRecords[row].upRecordIndex=previousRow++;
        ownedUnitListUiRecords[row].downRecordIndex=nextRow++;
        ownedUnitListUiRecords[row].activateCodeOffset=CAMPAIGN_CALLBACK_SELECT_OWNED_MECH;
    }
    MwUiRecord *done=&ownedUnitListUiRecords[(uint8_t)ownedUnitCount];
    done->downRecordIndex=0;
    done->activateCodeOffset=CAMPAIGN_CONTINUE_MECH_COMPLEX;
    ownedUnitListPanel.lineCount=ownedUnitCount+2;
    ownedUnitListUiRecords[0].upRecordIndex=(uint8_t)ownedUnitCount;
    MwDrawMechComplexBackground();
    MwDrawDescriptorTextPanel(&ownedUnitListPanel);
    MwSelectUiRecordTable(ownedUnitListSelectedRow,&ownedUnitListMenu);
    return CAMPAIGN_CONTINUE_UI;
}
/* MW_MAIN081B:3021..3062. POP callback IP, map visible row, JMP3063. */
uint16_t MwSelectOwnedUnitListRowCallback(void)
{
    ownedUnitListSelectedRow=selectedUiRecordIndex;
    uint16_t ownedUnitSlot=0,visibleRow=0;
    for (;;)
    {
        if (ownedUnitSlot>=CAMPAIGN_OWNED_UNIT_SLOTS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: ownedUnitSlot>=CAMPAIGN_OWNED_UNIT_SLOTS");
        if (ownedUnitKinds[ownedUnitSlot]!=CAMPAIGN_VACANT_UNIT_KIND)
        {
            if (visibleRow==ownedUnitListSelectedRow) break;
            ++visibleRow;
        }
        ++ownedUnitSlot;
    }
    selectedOwnedUnitByteOffset=ownedUnitSlot*sizeof(uint16_t);
    selectedOwnedUnitSlot=ownedUnitSlot;
    selectedOwnedUnitKind=ownedUnitKinds[ownedUnitSlot];
    return CAMPAIGN_CONTINUE_OPEN_OWNED_MECH;
}

/* MW_MAIN081B:09B2..0A3B. Serialized panel header and near text-position list.
 * MUL BL consumes only AL and BL, not the complete WORD line count/spacing. */
void MwDrawDescriptorTextPanel(const MwPanelDescriptor *textPanel)
{
    uint16_t height=(uint8_t)textPanel->lineCount*(uint8_t)(textPanel->lineSpacing+2);
    height+=textPanel->bottomInset;
    panelBorderEnabled=textPanel->borderEnabled;
    uint16_t x=textPanel->x;
    uint16_t y=textPanel->y;
    uint16_t width=textPanel->width;
    uint16_t background=textPanel->background;
    panelOriginX=x;panelOriginY=y;panelWidth=width;panelHeight=height;
    if (textPanel->bottomInset!=0)
    {
        uint16_t bottom=height+y;
        if (bottom>=EGA_ROWS) bottom=EGA_ROWS;
        EgaFillDisplayPageRectangle(x,y,width,(uint16_t)(bottom-y),background);
    }
    if (panelBorderEnabled!=0) MwDrawStoredStandardPanelBorder();
    uint16_t line=0;
    do
    {
        if (line>=sizeof(textPanel->lines)/sizeof(textPanel->lines[0])) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: line>=sizeof(textPanel->lines)/sizeof(textPanel->lines[0])"); /* Unconverted overrun, not zero lines. */
        const MwPanelText *caption=textPanel->lines[line];
        x=textPanel->x+caption->x;
        y=textPanel->y+caption->y;
        MwNativeByteView text={(const uint8_t *)caption->text,0,(uint32_t)strlen(caption->text)+1};
        EgaDrawTransparentText(x,y,caption->colour,&text,0);
        ++line;
    } while (line!=textPanel->lineCount);
}
