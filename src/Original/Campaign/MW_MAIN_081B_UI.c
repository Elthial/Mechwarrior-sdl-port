#include "mechwarrior/campaign_continuations.h"
#include "mechwarrior/campaign_ui.h"
#include "mechwarrior/ega_campaign.h"
#include "mechwarrior/campaign_input.h"
#include "mechwarrior/campaign_economy.h"
#include "mechwarrior/campaign_service.h"
#include "mechwarrior/campaign_market.h"
#include "mechwarrior/campaign_events.h"
#include "mechwarrior/campaign_pilots.h"
#include "mechwarrior/campaign_contracts.h"
#include "mechwarrior/campaign_travel.h"
#include "mechwarrior/campaign_options.h"
#include "mechwarrior/campaign_handoff.h"
#include <stdlib.h>
#include <string.h>
void (*campaignNearCallback)(uint16_t codeOffset);

/* MW_MAIN081B:089B..0950, entered by JMP, never an additional native CALL.
 * The code-offset variable below transports native CALL/JMP destinations, not
 * application action IDs. Converted nonlocal service tails join here without
 * recursively calling the menu. Unknown nonzero code bindings must fail. */
uint16_t MwCampaignUiEventLoop(void)
{
    for (;;)
    {
        if (uiIdleCodeOffset!=0)
        {
            if (uiIdleCodeOffset==0x35E3) MwAnimateMechServiceIdle();
            else
            {
                if (!campaignNearCallback) abort();
                campaignNearCallback(uiIdleCodeOffset);
            }
        }
        uint16_t key;
        if (!MwPollNavigationKeyFromMouseOrJoystick(&key))
        {
            if (!mwCampaignInput.keyboardAvailable) abort();
            if (!mwCampaignInput.keyboardAvailable()) continue;
            if (!mwCampaignInput.readKeyboard) abort();
            key=mwCampaignInput.readKeyboard();
        }
        if (key==CAMPAIGN_KEY_CALIBRATE) { MwCalibrateJoystickA();continue; }
        if (key==CAMPAIGN_KEY_ENTER)
        {
            const MwUiRecord *record=MwGetSelectedUiRecord();
            uint16_t codeOffset=record->activateCodeOffset;
            if (codeOffset==0) continue;
            MwFlashActivatedUiRecord(record);
            uiSelectionChangedCodeOffset=0;uiIdleCodeOffset=0;
            do
            {
                switch (codeOffset)
                {
                case CAMPAIGN_CONTINUE_CLEANUP:case CAMPAIGN_CONTINUE_AFTER_ACTION:case CAMPAIGN_CONTINUE_VICTORY_EVENT_RETURN:return codeOffset; /* Shared original0002 frame, including victory's suspended6293 CALL. */
                case 0x247A:case 0x24B6:codeOffset=MwEnterCompanyMenuContinuation(codeOffset);break;
                case CAMPAIGN_CONTINUE_OPTIONS:case CAMPAIGN_CONTINUE_OPTIONS_REDRAW:case CAMPAIGN_CONTINUE_OPTIONS_FROM_SUBMENU:codeOffset=MwEnterOptionsMenuContinuation(codeOffset==CAMPAIGN_CONTINUE_OPTIONS_REDRAW?CAMPAIGN_CONTINUE_OPTIONS_REDRAW:CAMPAIGN_CONTINUE_OPTIONS);break;
                case 0x6FB9:MwToggleAudioOption();codeOffset=0;break;
                case 0x6D52:codeOffset=MwSaveCampaignCallback();break;
                case 0x6EA3:codeOffset=MwShowLoadCampaignCallback();break;
                case 0x6F4C:codeOffset=MwLoadSelectedCampaignCallback();break;
                case 0x2C13:codeOffset=MwOpenNewsNetContinuation();break;
                case 0x2C77:codeOffset=MwNextNewsArticleContinuation();break;
                case 0x2D16:codeOffset=MwPreviousNewsArticleContinuation();break;
                case 0x6FFA:MwCycleTerrainDetailOption();codeOffset=0;break;
                case 0x7047:codeOffset=MwConfirmQuitCampaignContinuation();break;
                case 0x707F:codeOffset=MwConfirmRestartCampaignContinuation();break;
                case CAMPAIGN_CONTINUE_QUIT_FROM_OPTIONS:codeOffset=MwQuitFromOptionsContinuation();break;
                case CAMPAIGN_CONTINUE_RESTART_FROM_OPTIONS:codeOffset=MwRestartFromOptionsContinuation();break;
                case CAMPAIGN_CONTINUE_CREW_MENU:codeOffset=MwEnterCrewMenuContinuation();break;
                case 0x692E:case CAMPAIGN_CONTINUE_BAR_BACKGROUND:case CAMPAIGN_CONTINUE_BAR_REFRESH:case 0x69B8:codeOffset=MwEnterBarContinuation(codeOffset);break;
                case 0x69CA:case 0x6A03:codeOffset=MwOrderDrinkContinuation(codeOffset);break;
                case 0x6A06:codeOffset=MwOpenRecruitInterviewContinuation();break;
                case 0x6B69:codeOffset=MwHireSelectedRecruitContinuation();break;
                case 0x6BFA:codeOffset=MwRejectSelectedRecruitContinuation();break;
                case 0x28E7:case 0x28EC:case 0x28F1:case 0x28F6:
                    codeOffset=MwBeginPilotMechAssignmentContinuation((codeOffset-0x28E7)/5);break;
                case CAMPAIGN_CONTINUE_PILOT_ASSIGNMENT_INPUT:codeOffset=MwPollPilotMechAssignmentContinuation();break;
                case CAMPAIGN_CONTINUE_CONFIRM_PILOT_ASSIGNMENT:codeOffset=MwConfirmPilotMechAssignmentContinuation();break;
                case CAMPAIGN_CONTINUE_PLANET_HUB:case CAMPAIGN_CONTINUE_HUB_MENU:codeOffset=MwEnterCampaignPlanetHub(codeOffset);break;
                case 0x5620:codeOffset=MwOpenTravelScreenContinuation();break;
                case CAMPAIGN_CONTINUE_SELECT_TRAVEL_DESTINATION:codeOffset=MwSelectTravelDestinationContinuation(travelDestinationPlanetIndex);break;
                case CAMPAIGN_CONTINUE_TRAVEL_MAP:codeOffset=MwEnterTravelMapMenuContinuation();break;
                case 0x5C5D:codeOffset=MwSearchTravelPlanetContinuation();break;
                case 0x5CF2:codeOffset=MwOpenTravelHouseBrowserContinuation();break;
                case 0x5D21:codeOffset=MwCloseTravelHouseBrowserContinuation();break;
                case 0x5D45:codeOffset=MwOpenSelectedHousePlanetListContinuation();break;
                case 0x5E26:codeOffset=MwReturnToTravelHouseBrowserContinuation();break;
                case 0x5E41:codeOffset=MwSelectHousePlanetContinuation();break;
                case 0x5E5D:codeOffset=MwConfirmPlanetTravelContinuation();break;
                case CAMPAIGN_CONTINUE_PAID_TRAVEL:codeOffset=MwAdvancePaidTravelAndJoinPlanetArrivalContinuation();break;
                case CAMPAIGN_CONTINUE_PLANET_ARRIVAL:codeOffset=MwFinishPlanetArrivalContinuation();break;
                case 0x4AA2:case CAMPAIGN_CONTINUE_CONTRACT_OFFICE_REDRAW:case 0x4B43:codeOffset=MwEnterContractOfficeContinuation(codeOffset);break;
                case 0x4D35:codeOffset=MwRequestMissionOfferContinuation();break;
                case 0x4DA5:codeOffset=MwNextMissionOfferContinuation();break;
                case CAMPAIGN_CONTINUE_MISSION_BRIEFING:codeOffset=MwDrawMissionContractBriefingContinuation(negotiatingMissionOfferSlot);break;
                case 0x4EC8:codeOffset=MwEditContractPaymentRequestContinuation();break;
                case 0x4EE7:codeOffset=MwEditContractSalvageRequestContinuation();break;
                case 0x4F06:codeOffset=MwEditContractAdvanceRequestContinuation();break;
                case 0x536A:codeOffset=MwNegotiateAndAcceptContractContinuation();break;
                case CAMPAIGN_CONTINUE_CREDIT_CONTRACT_ADVANCE:codeOffset=MwCreditAcceptedContractAdvanceContinuation();break;
                case CAMPAIGN_CONTINUE_ADJUST_ENEMY_FORCE:codeOffset=MwAdjustAcceptedContractEnemyForceContinuation();break;
                case 0x1B55:codeOffset=MwAttemptContractDeployment();break;
                case 0x1B5A:codeOffset=MwCheckAssignedMechDeploymentReadiness();break;
                case CAMPAIGN_CONTINUE_DEPLOYMENT:codeOffset=MwResolveDeploymentOrExportCombatExchangeContinuation();break;
                case CAMPAIGN_CONTINUE_CANCEL_CONTRACT:codeOffset=MwCancelContractAfterDeploymentRefusalsContinuation();break;
                case CAMPAIGN_CONTINUE_EVENT_DEPLOYMENT:
                    codeOffset=MwCheckDeploymentWithoutRefusalPenalty();
                    if (!codeOffset) codeOffset=CAMPAIGN_CONTINUE_EVENT_COMBAT_RETURN; /* Special event inherited RET, then6236 caller ignores CF. */
                    break;
                case 0x67C5:codeOffset=MwEventFirstChoiceCallback();break;
                case 0x67F8:codeOffset=MwEventSecondChoiceCallback();break;
                case 0x6828:codeOffset=MwEventThirdChoiceCallback();break;
                case CAMPAIGN_CONTINUE_EVENT_QUIT:codeOffset=MwQuitFromEventContinuation();break;
                case CAMPAIGN_CONTINUE_RESTART:codeOffset=MwRestartCampaignContinuation();break;
                case CAMPAIGN_CONTINUE_EVENT_COMBAT:codeOffset=MwPrepareEventSpecialCombatContinuation();break;
                case CAMPAIGN_CONTINUE_INITIALIZE_COMPANY:codeOffset=MwRunCopyProtectionAndInitializeCampaign(codeOffset);break;
                case CAMPAIGN_CONTINUE_EVENT_COMBAT_RETURN: /* STC/RET; the mapped6236 caller ignores CF. */
                    if (campaignEventReturnContinuation!=CAMPAIGN_CONTINUE_NEXT_EVENT) abort();
                    codeOffset=campaignEventReturnContinuation;break;
                case CAMPAIGN_CONTINUE_NEXT_EVENT:
                    codeOffset=MwDispatchCampaignEvents(campaignEventClass,codeOffset);
                    if (!codeOffset)
                    {
                        codeOffset=campaignEventDispatchReturnContinuation;
                        campaignEventDispatchReturnContinuation=0;
                        if (!codeOffset) abort(); /* Unrepresented outer caller frame. */
                    }
                    break;
                case CAMPAIGN_CONTINUE_OWNED_MECH_LIST:codeOffset=MwOpenOwnedMechList();break;
                case CAMPAIGN_CONTINUE_MECH_COMPLEX:case 0x2DE2:codeOffset=MwEnterMechServiceMenuContinuation(codeOffset);break;
                case CAMPAIGN_CONTINUE_MARKET_MECH_LIST:codeOffset=MwOpenMarketMechList();break;
                case 0x3314:codeOffset=MwBeginOwnedUnitSaleCallback();break;
                case 0x3393:codeOffset=MwSaleContinuation();break;
                case 0x3477:codeOffset=MwCancelOwnedUnitSale();break;
                case 0x348D:codeOffset=MwQuoteSelectedMechRearm();break;
                case 0x3560:codeOffset=MwRearmSelectedMech();break;
                case 0x35C9:codeOffset=MwCancelSelectedMechRearm();break;
                case 0x407C:codeOffset=MwOpenMarketMechComponentInspectionCallback();break;
                case 0x4317:codeOffset=MwSelectMarketUnitListRowCallback();break;
                case 0x4352:case CAMPAIGN_CONTINUE_MARKET_MECH_DETAIL:case 0x4379:case 0x43E0:case 0x43EB:
                    codeOffset=MwDrawMarketMechDetailContinuation(codeOffset);break;
                case 0x4674:case 0x475A:codeOffset=MwPurchaseSelectedMarketUnit(codeOffset);break;
                case 0x3021:codeOffset=MwSelectOwnedUnitListRowCallback();break;
                case CAMPAIGN_CONTINUE_OPEN_OWNED_MECH:codeOffset=MwOpenSelectedMechDetailContinuation();break;
                case CAMPAIGN_CONTINUE_RETURN_TO_OWNED_MECH:codeOffset=MwReturnToSelectedMechDetailCallback();break;
                case CAMPAIGN_CONTINUE_OWNED_MECH_DETAIL:case CAMPAIGN_CONTINUE_OWNED_MECH_SERVICE_MENU:codeOffset=MwDrawSelectedMechDetailContinuation(codeOffset);break;
                case CAMPAIGN_CONTINUE_COMPONENT_REPAIR_MENU:codeOffset=MwOpenMechComponentRepairMenuCallback();break;
                case 0x3EE8:codeOffset=MwRepairSelectedMechComponentCallback();break;
                case 0x3802:codeOffset=MwRepairAllSelectedMechComponentsCallback();break;
                default:
                    if (!campaignNearCallback) abort();
                    campaignNearCallback(codeOffset); /* A bound ordinary native CALL. */
                    codeOffset=0;
                    break;
                }
            } while (codeOffset!=0 && codeOffset!=CAMPAIGN_CONTINUE_UI);
            continue;
        }
        switch (key)
        {
        case CAMPAIGN_KEY_UP:case CAMPAIGN_KEY_DOWN:
        case CAMPAIGN_KEY_LEFT:case CAMPAIGN_KEY_RIGHT:break;
        default:
            if (key==CAMPAIGN_KEY_ESCAPE)
            {
                if (!mwCampaignInput.readKeyboardShiftFlags) abort();
                if (mwCampaignInput.readKeyboardShiftFlags()&BIOS_SHIFT_CONTROL)
                {
                    return CAMPAIGN_CONTINUE_CLEANUP;
                }
            }
            continue;
        }
        const MwUiRecord *record=MwGetSelectedUiRecord();
        uint16_t nextIndex=key==CAMPAIGN_KEY_UP?record->upRecordIndex:
            key==CAMPAIGN_KEY_DOWN?record->downRecordIndex:
            key==CAMPAIGN_KEY_LEFT?record->leftRecordIndex:record->rightRecordIndex;
        if (MwIsUiRecordDisabled(nextIndex)) continue;
        if ((uint8_t)nextIndex==0xFF) continue; /* After the native enabled read. */
        MwPresentUiRecord(record->x,record->y,record);
        uint16_t selectionTransfer=MwSelectUiRecord(nextIndex);
        if (selectionTransfer==CAMPAIGN_CONTINUE_CLEANUP) return selectionTransfer;
        if (selectionTransfer==CAMPAIGN_CONTINUE_TRAVEL_MAP)
        {
            selectionTransfer=MwEnterTravelMapMenuContinuation();
            if (selectionTransfer!=CAMPAIGN_CONTINUE_UI) abort();
        }
        else if (selectionTransfer!=0) abort();
    }
}

/* MW_MAIN081B:0A3C..0A50 falls through0A51, no additional native frame. */
void MwDrawPicturePanelBorder(uint16_t x, uint16_t y, uint16_t width, uint16_t height)
{
    panelOriginX = x; panelOriginY = y;
    panelWidth = width; panelHeight = height;
    panelBorderPictureBase = 0;
    MwDrawStoredPicturePanelBorder();
}

/* MW_MAIN081B:0A4B shared CALL target, also fallthrough from0A3C. */
void MwDrawStoredStandardPanelBorder(void)
{
    panelBorderPictureBase=0;
    MwDrawStoredPicturePanelBorder();
}

/* MW_MAIN081B:0A51..0BF1 shared entry.0A5A is its width-gated interior,
 * not another source method. No caller return-IP is inspected in this tail. */
void MwDrawStoredPicturePanelBorder(void)
{
    if (panelWidth < PANEL_HORIZONTAL_TILE_PIXELS) return;
    uint16_t horizontalTiles = panelWidth / PANEL_HORIZONTAL_TILE_PIXELS;
    if (horizontalTiles > UINT8_MAX) abort(); /* Native DIV BL faults before drawing. */
    uint16_t x = panelOriginX, tilesRemaining = horizontalTiles;
    do
    {
        MwDrawGeneralPicture(x,panelOriginY,EGA_DISPLAY_PAGE,panelBorderPictureBase + PANEL_TOP_TILE);
        x += PANEL_HORIZONTAL_TILE_PIXELS;
    } while (--tilesRemaining != 0);
    MwDrawGeneralPicture((uint16_t)(panelOriginX + panelWidth - PANEL_HORIZONTAL_TILE_PIXELS),
        panelOriginY,EGA_DISPLAY_PAGE,panelBorderPictureBase + PANEL_TOP_TILE);
    x = panelOriginX; tilesRemaining = horizontalTiles;
    do
    {
        MwDrawGeneralPicture(x,(uint16_t)(panelOriginY + panelHeight - PANEL_BOTTOM_EDGE_INSET),
            EGA_DISPLAY_PAGE,panelBorderPictureBase + PANEL_BOTTOM_TILE);
        x += PANEL_HORIZONTAL_TILE_PIXELS;
    } while (--tilesRemaining != 0);
    MwDrawGeneralPicture((uint16_t)(panelOriginX + panelWidth - PANEL_HORIZONTAL_TILE_PIXELS),
        (uint16_t)(panelOriginY + panelHeight - PANEL_BOTTOM_EDGE_INSET),
        EGA_DISPLAY_PAGE,panelBorderPictureBase + PANEL_BOTTOM_TILE);
    if (panelHeight < PANEL_VERTICAL_TILE_PIXELS) return;
    uint16_t verticalTiles = panelHeight / PANEL_VERTICAL_TILE_PIXELS;
    if (verticalTiles > UINT8_MAX) abort(); /* Horizontal writes survive native DIV fault. */
    uint16_t y = panelOriginY; tilesRemaining = verticalTiles;
    do
    {
        MwDrawGeneralPicture(panelOriginX,y,EGA_DISPLAY_PAGE,panelBorderPictureBase + PANEL_LEFT_TILE);
        y += PANEL_VERTICAL_TILE_PIXELS;
    } while (--tilesRemaining != 0);
    MwDrawGeneralPicture(panelOriginX,(uint16_t)(panelOriginY + panelHeight - PANEL_VERTICAL_TILE_PIXELS),
        EGA_DISPLAY_PAGE,panelBorderPictureBase + PANEL_LEFT_TILE);
    y = panelOriginY; tilesRemaining = verticalTiles;
    do
    {
        MwDrawGeneralPicture((uint16_t)(panelOriginX + panelWidth - PANEL_RIGHT_EDGE_INSET),y,
            EGA_DISPLAY_PAGE,panelBorderPictureBase + PANEL_RIGHT_TILE);
        y += PANEL_VERTICAL_TILE_PIXELS;
    } while (--tilesRemaining != 0);
    MwDrawGeneralPicture((uint16_t)(panelOriginX + panelWidth - PANEL_RIGHT_EDGE_INSET),
        (uint16_t)(panelOriginY + panelHeight - PANEL_VERTICAL_TILE_PIXELS),
        EGA_DISPLAY_PAGE,panelBorderPictureBase + PANEL_RIGHT_TILE);
    MwDrawGeneralPicture(panelOriginX,panelOriginY,EGA_DISPLAY_PAGE,panelBorderPictureBase + PANEL_TOP_LEFT);
    MwDrawGeneralPicture(panelOriginX,(uint16_t)(panelOriginY + panelHeight - PANEL_BOTTOM_CORNER_INSET),
        EGA_DISPLAY_PAGE,panelBorderPictureBase + PANEL_BOTTOM_LEFT);
    MwDrawGeneralPicture((uint16_t)(panelOriginX + panelWidth - PANEL_RIGHT_CORNER_INSET),panelOriginY,
        EGA_DISPLAY_PAGE,panelBorderPictureBase + PANEL_TOP_RIGHT);
    MwDrawGeneralPicture((uint16_t)(panelOriginX + panelWidth - PANEL_RIGHT_CORNER_INSET),
        (uint16_t)(panelOriginY + panelHeight - PANEL_BOTTOM_CORNER_INSET),
        EGA_DISPLAY_PAGE,panelBorderPictureBase + PANEL_BOTTOM_RIGHT);
}
/* MW_MAIN081B:0BF2..0C09; JMP0A51 retains the original caller's return. */
void MwDrawAlternatePicturePanelBorder(uint16_t x, uint16_t y, uint16_t width, uint16_t height)
{
    panelBorderPictureBase = PANEL_ALTERNATE_PICTURE_BASE;
    panelOriginX = x; panelOriginY = y;
    panelWidth = width; panelHeight = height;
    MwDrawStoredPicturePanelBorder();
}

/* MW_MAIN081B:0EE7/0F27..0F42. PUSH DS:DI is the saved text source;
 * the C view borrows those bytes without allocating a padded segment copy. */
MwTextPanelReturn MwShowMonospaceTextPanel(uint16_t insetX, uint16_t insetY,
    uint16_t columns, uint16_t rows, const MwNativeByteView *text, uint16_t textOffset)
{
    textPanelFont = TEXT_PANEL_MONOSPACE_FONT;
    textPanelInsetX = insetX; textPanelInsetY = insetY;
    textPanelWidthPixels = (uint16_t)(columns << 3);
    textPanelHeightPixels = (uint16_t)(rows << 3);
    return MwCenterAndShowTextPanel(text,textOffset);
}
/* MW_MAIN081B:0EF1..0F25. ES:DI text, not DS:DI. */
MwTextPanelReturn MwShowCompactTextPanel(uint16_t insetX, uint16_t insetY,
    uint16_t columns, uint16_t rows, const MwNativeByteView *text, uint16_t textOffset)
{
    textPanelFont = TEXT_PANEL_COMPACT_FONT;
    textPanelInsetX = insetX; textPanelInsetY = insetY;
    textPanelWidthPixels = (uint16_t)(columns << 1);
    textPanelWidthPixels += (uint16_t)(columns << 2);
    textPanelHeightPixels = rows;
    textPanelHeightPixels += (uint16_t)(rows << 1);
    textPanelHeightPixels += (uint16_t)(rows << 2);
    return MwCenterAndShowTextPanel(text,textOffset);
}
/* MW_MAIN081B:0F43..1031 shared continuation.0F55/0F5C are interior labels,
 * not callable helpers. The C boundary projects the common fallthrough only. */
MwTextPanelReturn MwCenterAndShowTextPanel(const MwNativeByteView *text, uint16_t textOffset)
{
    textPanelOriginX = (uint16_t)(EGA_COLUMNS - textPanelWidthPixels) >> 1;
    uint16_t y = textPanelYOverride;
    if (y == 0) y = (uint16_t)(EGA_ROWS - textPanelHeightPixels) >> 1;
    textPanelYOverride = 0;
    textPanelOriginY = y;
    EgaCopyDisplayToDrawPage(textPanelOriginX,textPanelOriginY,
        (uint16_t)(textPanelWidthPixels + TEXT_PANEL_CAPTURE_MARGIN),textPanelHeightPixels);
    EgaFillDisplayPageRectangle(textPanelOriginX,textPanelOriginY,
        textPanelWidthPixels,textPanelHeightPixels,TEXT_PANEL_BACKGROUND);
    MwDrawPicturePanelBorder(textPanelOriginX,textPanelOriginY,
        textPanelWidthPixels,textPanelHeightPixels);
    textPanelPreviousFont = EgaGetFontIndex();
    EgaSelectFont(textPanelFont);
    uint16_t textX = textPanelOriginX + textPanelInsetX;
    uint16_t textY = textPanelOriginY + textPanelInsetY;
    if (textPanelFont == TEXT_PANEL_COMPACT_FONT)
        textOffset = EgaDrawTextWithLineSpacing(textX,textY,TEXT_PANEL_FOREGROUND,text,textOffset);
    else textOffset = EgaDrawTransparentText(textX,textY,TEXT_PANEL_FOREGROUND,text,textOffset);
    EgaSelectFont(textPanelPreviousFont);
    if (textPanelContinuationMode == TEXT_PANEL_MODE_DISCARD_RETURN)
    {
        textPanelContinuationMode = 0;
        MwTextPanelReturn result = { textOffset,
            (uint16_t)(textPanelOriginX + textPanelInsetX),
            (uint16_t)(textPanelOriginY + textPanelInsetY),true };
        return result; /* Native POP DI / POP AX / RET skips the saved return word. */
    }
    if (textPanelContinuationMode != TEXT_PANEL_MODE_LEAVE_VISIBLE)
    {
        MwWaitForInputAcknowledgment();
        EgaCopyDrawPageToDisplay(textPanelOriginX,textPanelOriginY,
            (uint16_t)(textPanelWidthPixels + TEXT_PANEL_CAPTURE_MARGIN),textPanelHeightPixels);
    }
    MwTextPanelReturn result = { textOffset, textX, textY, false };
    return result;
}

/* MW_MAIN081B:1032..104B. Close the live text panel, not a copied dialog. */
void MwRestoreSavedTextPanelRegion(void)
{
    EgaCopyDrawPageToDisplay(textPanelOriginX,textPanelOriginY,
        textPanelWidthPixels+TEXT_PANEL_CAPTURE_MARGIN,textPanelHeightPixels);
}

/* MW_MAIN081B:6122..6152. Four fixed slots, clear only after actual display.
 * The text view aliases the canonical text owner; there is no copied DS image. */
void MwShowPendingNotifications(void)
{
    for (uint16_t slot = 0; slot < CAMPAIGN_NOTIFICATION_COUNT; ++slot)
    {
        if (notificationPending[slot] == 0) continue;
        MwNativeByteView text = { notificationTextStorage, NOTIFICATION_TEXT_START,
            sizeof(notificationTextStorage) };
        MwTextPanelReturn result = MwShowMonospaceTextPanel(notificationTextInsetX[slot],
            notificationTextInsetY[slot],notificationColumns[slot],notificationRows[slot],
            &text,notificationTextOffsets[slot]);
        /* Native mode1 would RET through the saved slot WORD instead of6146.
         * That startup/code continuation is outside this compiled slice. */
        if (result.discardReturnWord) abort();
        notificationPending[slot] = 0;
    }
}
/* MW_MAIN081B:07FE..0811 falls0812; reset preserves incoming AX/SI. */
uint16_t MwSelectUiRecordTable(uint16_t selectedIndex, MwUiMenu *menu)
{
    if (mouseInstalled != 0)
    {
        if (mwCampaignInput.resetMouse == NULL) abort();
        mwCampaignInput.resetMouse();
    }
    uiMenu = menu;
    return MwSelectUiRecord(selectedIndex);
}
/* MW_MAIN081B:0812..0837. Selection callback is loaded AFTER drawing. The C
 * result transports only598A's native nonlocal exits; ordinary RET is zero. */
uint16_t MwSelectUiRecord(uint16_t selectedIndex)
{
    selectedUiRecordIndex = selectedIndex;
    const MwUiRecord *record = MwGetSelectedUiRecord();
    MwCaptureAndDrawUiRecord(record->x,record->y,record->pictureIndex,record);
    uint16_t callback = uiSelectionChangedCodeOffset;
    if (callback != 0)
    {
        if (callback==0x3DF3) MwRefreshSelectedComponentRepairQuote();
        else if (callback==0x598A) return MwHandleTravelMapSelection();
        else
        {
            if (campaignNearCallback == NULL) abort();
            campaignNearCallback(callback);
        }
    }
    return 0;
}
/* MW_MAIN081B:0838..0871. Save34x35 before selected display-page picture. */
void MwCaptureAndDrawUiRecord(uint16_t x, uint16_t y, uint16_t pictureIndex, const MwUiRecord *record)
{
    if (record->textRecord != 0)
    {
        if (record->text != NULL)
        {
            MwNativeByteView text = { (const uint8_t *)record->text, 0, (uint32_t)strlen(record->text)+1 };
            EgaDrawTransparentText(x,y,UI_SELECTED_TEXT_COLOUR,&text,0);
        }
    }
    else
    {
        EgaCopyDisplayToDrawPage(x,y,UI_HIGHLIGHT_WIDTH,UI_HIGHLIGHT_HEIGHT);
        MwDrawGeneralPicture(x,y,EGA_DISPLAY_PAGE,pictureIndex);
    }
}
/* MW_MAIN081B:0872..089A. Restore authored colour or saved page region. */
void MwPresentUiRecord(uint16_t x, uint16_t y, const MwUiRecord *record)
{
    if (record->textRecord != 0)
    {
        if (!record->text || !record->textColour) abort();
        MwNativeByteView text = { (const uint8_t *)record->text, 0, (uint32_t)strlen(record->text)+1 };
        EgaDrawTransparentText(x,y,*record->textColour,&text,0);
    }
    else EgaCopyDrawPageToDisplay(x,y,UI_HIGHLIGHT_WIDTH,UI_HIGHLIGHT_HEIGHT);
}
/* MW_MAIN081B:0951..095F. Native MUL retains the low WORD before adding SI.
 * Valid record accesses are aligned multiples of0E; unknown byte-interior
 * aliases are an unconverted boundary, never an out-of-bounds C pointer. */
MwUiRecord *MwGetSelectedUiRecord(void)
{
    uint16_t byteOffset = (uint16_t)(selectedUiRecordIndex * CAMPAIGN_UI_RECORD_BYTES);
    if (!uiMenu || byteOffset % CAMPAIGN_UI_RECORD_BYTES != 0
        || byteOffset / CAMPAIGN_UI_RECORD_BYTES >= uiMenu->recordCount) abort();
    return &uiMenu->records[byteOffset / CAMPAIGN_UI_RECORD_BYTES];
}
/* MW_MAIN081B:0960..0979. FF probing precedes the caller's sentinel test. */
bool MwIsUiRecordDisabled(uint16_t recordIndex)
{
    if (recordIndex==0xFF)
    {
        if (uiMenu->sentinelEnabled) return *uiMenu->sentinelEnabled==0;
        if (!uiMenu->sentinelEnabledWord) abort();
        return (uint8_t)(*uiMenu->sentinelEnabledWord>>uiMenu->sentinelEnabledShift)==0;
    }
    uint16_t byteOffset=(uint16_t)(recordIndex*CAMPAIGN_UI_RECORD_BYTES);
    if (byteOffset%CAMPAIGN_UI_RECORD_BYTES!=0 || byteOffset/CAMPAIGN_UI_RECORD_BYTES>=uiMenu->recordCount) abort();
    return uiMenu->records[byteOffset/CAMPAIGN_UI_RECORD_BYTES].enabled==0;
}
/* MW_MAIN081B:097A..09B1. Retain tick equality and live fields after waits. */
void MwFlashActivatedUiRecord(const MwUiRecord *record)
{
    for (uint16_t flashesRemaining = UI_ACTIVATION_FLASH_COUNT; flashesRemaining != 0; --flashesRemaining)
    {
        uint16_t targetTick = campaignTimerTicks + UI_FLASH_FIRST_WAIT_TICKS;
        MwPresentUiRecord(record->x,record->y,record);
        while (campaignTimerTicks != targetTick)
        {
            if (mwCampaignInput.waitForInterrupt == NULL) abort();
            mwCampaignInput.waitForInterrupt();
        }
        targetTick++;
        MwCaptureAndDrawUiRecord(record->x,record->y,record->pictureIndex,record);
        while (campaignTimerTicks != targetTick)
        {
            if (mwCampaignInput.waitForInterrupt == NULL) abort();
            mwCampaignInput.waitForInterrupt();
        }
    }
}
