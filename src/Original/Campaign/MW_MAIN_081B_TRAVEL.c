#include "mechwarrior/campaign_continuations.h"
#include "mechwarrior/campaign_mechanics.h"
#include "mechwarrior/campaign_travel.h"
#include "mechwarrior/campaign_ui.h"
#include "mechwarrior/campaign_economy.h"
#include "mechwarrior/campaign_events.h"
#include "mechwarrior/campaign_input.h"
#include "mechwarrior/campaign_startup.h"
#include "mechwarrior/ega_campaign.h"
#include <stdlib.h>
#include <string.h>

enum { TRAVEL_NAME_PANEL_X=0x0008, TRAVEL_HEADER_PANEL_Y=0x0006,
    TRAVEL_NAME_PANEL_WIDTH=0x0074, TRAVEL_HEADER_PANEL_HEIGHT=0x0008,
    TRAVEL_LANDSCAPE_PANEL_X=0x0087, TRAVEL_LANDSCAPE_PANEL_WIDTH=0x0035,
    TRAVEL_POPULATION_PANEL_X=0x00E5, TRAVEL_POPULATION_PANEL_WIDTH=0x0052,
    TRAVEL_DESCRIPTION_PANEL_Y=0x0015, TRAVEL_DESCRIPTION_PANEL_WIDTH=0x0130,
    TRAVEL_DESTINATION_TEXT_X=0x000A, TRAVEL_HEADER_TEXT_Y=0x0008,
    TRAVEL_HOUSE_PICTURE_X=0x00EF, TRAVEL_HOUSE_PICTURE_Y=0x0021,
    TRAVEL_LANDSCAPE_TEXT_X=0x008D, TRAVEL_POPULATION_TEXT_X=0x00E6,
    TRAVEL_POPULATION_DIGITS=4, TRAVEL_POPULATION_SUFFIX_X=0x0104,
    TRAVEL_DESCRIPTION_TEXT_Y=0x0017, TRAVEL_QUOTE_CLEAR_X=0x00ED,
    TRAVEL_QUOTE_CLEAR_Y=0x0067, TRAVEL_QUOTE_CLEAR_WIDTH=0x0049, TRAVEL_QUOTE_CLEAR_HEIGHT=0x0007 };

/* MW_MAIN 081B:5FEF..600B. Copies through the NUL; shipped names fit the
 * existing0018-byte mutable owner. This is not a new string-cache policy. */
void MwCopyTravelDestinationName(void)
{
    uint16_t directoryByteOffset=travelDestinationPlanetIndex*sizeof(uint16_t);
    if (directoryByteOffset>=sizeof(planetRecordOffsets)) abort();
    uint16_t planetOffset=planetRecordOffsets[directoryByteOffset/sizeof(uint16_t)];
    if (planetOffset<PLANET_RECORDS_START || planetOffset>=PLANET_RECORDS_END) abort();
    const char *name=(const char *)&planetRecordStorage[planetOffset-PLANET_RECORDS_START+sizeof(MwPlanetHeader)];
    uint16_t nameByte=0;
    do
    {
        if (nameByte>=sizeof(travelDestinationName)) abort(); /* Unrepresented adjacent native storage. */
        travelDestinationName[nameByte]=name[nameByte];
    } while (travelDestinationName[nameByte++]!='\0');
}

/* MW_MAIN 081B:5AF5..5B0B. The old marker coordinates are captured before
 * drawing sprite0D on the display page. */
void MwDrawTravelMarker(void)
{
    travelSavedMarkerX=travelMarkerX;
    travelSavedMarkerY=travelMarkerY;
    MwDrawGeneralPicture(travelMarkerX,travelMarkerY,EGA_DISPLAY_PAGE,TRAVEL_MARKER_PICTURE);
}

/* MW_MAIN 081B:5B0C..5B20. The draw page holds the clean map captured by5620. */
void MwRestoreTravelMarkerRegion(void)
{
    EgaCopyDrawPageToDisplay(travelSavedMarkerX,travelSavedMarkerY,
        TRAVEL_MARKER_RESTORE_WIDTH,TRAVEL_MARKER_RESTORE_HEIGHT);
}

/* MW_MAIN 081B:5BDD..5C5A. DI runs through primary indices00..90; the point
 * test is inclusive against the marker's four-pixel box. */
int16_t MwFindPlanetAtTravelMarker(void)
{
    uint16_t right=travelMarkerX+TRAVEL_MARKER_HIT_SIZE;
    uint16_t bottom=travelMarkerY+TRAVEL_MARKER_HIT_SIZE;
    for (uint16_t planetIndex=0;planetIndex<TRAVEL_PLANET_DIRECTORY_COUNT;++planetIndex)
    {
        uint16_t planetOffset=planetRecordOffsets[planetIndex];
        if (planetOffset<PLANET_RECORDS_START || (uint32_t)planetOffset+sizeof(MwPlanetHeader)>PLANET_RECORDS_END) abort();
        const MwPlanetHeader *planet=(const MwPlanetHeader *)&planetRecordStorage[planetOffset-PLANET_RECORDS_START];
        if (planet->mapPositionX<travelMarkerX || planet->mapPositionX>right) continue;
        if (planet->mapPositionY<travelMarkerY || planet->mapPositionY>bottom) continue;
        return (int16_t)planetIndex;
    }
    return -1;
}

/* MW_MAIN 081B:598A..5C5A. Keyboard, joystick and INT33 policy remains game
 * code. The host supplies only mouse position/release and BIOS/game-port IO.
 * INT33 horizontal positions are twice the 320-column EGA coordinates. */
uint16_t MwHandleTravelMapSelection(void)
{
    if (selectedUiRecordIndex!=TRAVEL_MAP_UI_RECORD) return 0;
positionMouse:
    if (mouseInstalled!=0)
    {
        if (!mwCampaignInput.setMousePosition) abort();
        mwCampaignInput.setMousePosition((uint16_t)(travelMarkerX<<1),travelMarkerY);
    }
    for (;;)
    {
        uint16_t key=0;
        if (!mwCampaignInput.keyboardAvailable || !mwCampaignInput.readKeyboard) abort();
        if (mwCampaignInput.keyboardAvailable()) key=mwCampaignInput.readKeyboard();
        else if (joystickInstalled!=0 && campaignTimerTicks!=travelJoystickLastTick)
        {
            travelJoystickLastTick=campaignTimerTicks; /*5B81 saved tick. */
            MwPollJoystickA();
            if (joystickButtonOnePressed!=0)
            {
                do {MwPollJoystickA();} while (joystickButtonOnePressed!=0);
                key=CAMPAIGN_KEY_ENTER;
            }
            else if (joystickDirection==JOYSTICK_CENTRED) goto pollMouse;
            else if (joystickDirection<JOYSTICK_UP_LEFT)
                key=joystickNavigationKeys[joystickDirection];
            else key=CAMPAIGN_KEY_UP_LEFT; /* Native default5BDA. */
        }
        else
        {
pollMouse:
            if (mouseInstalled==0) continue;
            if (!mwCampaignInput.readMousePosition || !mwCampaignInput.mouseReleaseCount) abort();
            uint16_t mouseX,mouseY;
            mwCampaignInput.readMousePosition(&mouseX,&mouseY);
            mouseX>>=1;
            if (mouseX!=travelMarkerX || mouseY!=travelMarkerY)
            {
                travelMarkerX=mouseX;travelMarkerY=mouseY;
                if (travelMarkerX<TRAVEL_MAP_MINIMUM_X || travelMarkerX>TRAVEL_MAP_MAXIMUM_X
                    || travelMarkerY<TRAVEL_MAP_MINIMUM_Y || travelMarkerY>TRAVEL_MAP_MAXIMUM_Y)
                    goto restoreCurrentPlanet;
                MwRestoreTravelMarkerRegion();MwDrawTravelMarker();
            }
            if (mwCampaignInput.mouseReleaseCount(MOUSE_BUTTON_LEFT)==0) continue;
            key=CAMPAIGN_KEY_ENTER;
        }
        if (key==CAMPAIGN_KEY_ESCAPE)
        {
            if (!mwCampaignInput.readKeyboardShiftFlags) abort();
            if (mwCampaignInput.readKeyboardShiftFlags()&BIOS_SHIFT_CONTROL) return CAMPAIGN_CONTINUE_CLEANUP;
            continue;
        }
        if (key==CAMPAIGN_KEY_ENTER)
        {
            int16_t planetIndex=MwFindPlanetAtTravelMarker();
            if (planetIndex<0) continue;
            uint16_t planetOffset=planetRecordOffsets[(uint16_t)planetIndex];
            const MwPlanetHeader *planet=(const MwPlanetHeader *)&planetRecordStorage[planetOffset-PLANET_RECORDS_START];
            travelMarkerX=(uint16_t)(planet->mapPositionX-TRAVEL_MAP_MARKER_INSET);
            travelMarkerY=(uint16_t)(planet->mapPositionY-TRAVEL_MAP_MARKER_INSET);
            MwRestoreTravelMarkerRegion();MwDrawTravelMarker();
            travelDestinationPlanetIndex=(uint16_t)planetIndex;
            MwCopyTravelDestinationName();
            MwDrawDescriptorTextPanel(&travelDestinationHeaderPanel);
            MwDrawTravelDestinationDetails();MwDrawPlanetTravelQuote();
            travelMapSelection=0;
            return CAMPAIGN_CONTINUE_TRAVEL_MAP;
        }
        switch (key)
        {
        case CAMPAIGN_KEY_UP:--travelMarkerY;break;
        case CAMPAIGN_KEY_DOWN:++travelMarkerY;break;
        case CAMPAIGN_KEY_LEFT:--travelMarkerX;break;
        case CAMPAIGN_KEY_RIGHT:++travelMarkerX;break;
        case CAMPAIGN_KEY_UP_LEFT:--travelMarkerX;--travelMarkerY;break;
        case CAMPAIGN_KEY_UP_RIGHT:++travelMarkerX;--travelMarkerY;break;
        case CAMPAIGN_KEY_DOWN_LEFT:--travelMarkerX;++travelMarkerY;break;
        case CAMPAIGN_KEY_DOWN_RIGHT:++travelMarkerX;++travelMarkerY;break;
        default:continue;
        }
        if (travelMarkerX<TRAVEL_MAP_MINIMUM_X || travelMarkerX>TRAVEL_MAP_MAXIMUM_X
            || travelMarkerY<TRAVEL_MAP_MINIMUM_Y || travelMarkerY>TRAVEL_MAP_MAXIMUM_Y)
        {
restoreCurrentPlanet:
            travelMapSelection=0;MwRestoreTravelMarkerRegion();
            uint16_t planetOffset=planetRecordOffsets[currentPlanetIndex];
            const MwPlanetHeader *planet=(const MwPlanetHeader *)&planetRecordStorage[planetOffset-PLANET_RECORDS_START];
            travelMarkerX=(uint16_t)(planet->mapPositionX-TRAVEL_MAP_MARKER_INSET);
            travelMarkerY=(uint16_t)(planet->mapPositionY-TRAVEL_MAP_MARKER_INSET);
            MwDrawTravelMarker();return CAMPAIGN_CONTINUE_TRAVEL_MAP;
        }
        MwRestoreTravelMarkerRegion();MwDrawTravelMarker();
        if (mouseInstalled!=0) goto positionMouse; /*5A37 ->5999. */
    }
}

/* MW_MAIN 081B:56F9..5815. Record text is borrowed directly from the one
 * canonical variable-length planet store. The four fills precede validation
 * and every later draw in the same order as the executable. */
void MwDrawTravelDestinationDetails(void)
{
    EgaFillDisplayPageRectangle(TRAVEL_NAME_PANEL_X,TRAVEL_HEADER_PANEL_Y,TRAVEL_NAME_PANEL_WIDTH,TRAVEL_HEADER_PANEL_HEIGHT,Campaign_Pal_EGA_Black);
    EgaFillDisplayPageRectangle(TRAVEL_LANDSCAPE_PANEL_X,TRAVEL_HEADER_PANEL_Y,TRAVEL_LANDSCAPE_PANEL_WIDTH,TRAVEL_HEADER_PANEL_HEIGHT,Campaign_Pal_EGA_LightGrey);
    EgaFillDisplayPageRectangle(TRAVEL_POPULATION_PANEL_X,TRAVEL_HEADER_PANEL_Y,TRAVEL_POPULATION_PANEL_WIDTH,TRAVEL_HEADER_PANEL_HEIGHT,Campaign_Pal_EGA_LightGrey);
    EgaFillDisplayPageRectangle(TRAVEL_NAME_PANEL_X,TRAVEL_DESCRIPTION_PANEL_Y,TRAVEL_DESCRIPTION_PANEL_WIDTH,TRAVEL_HEADER_PANEL_HEIGHT,Campaign_Pal_EGA_LightGrey);
    uint16_t directoryByteOffset=travelDestinationPlanetIndex*sizeof(uint16_t);
    if (directoryByteOffset>=sizeof(planetRecordOffsets)) abort();
    uint16_t planetOffset=planetRecordOffsets[directoryByteOffset/sizeof(uint16_t)];
    travelDestinationPlanetOffset=planetOffset;
    if (planetOffset<PLANET_RECORDS_START || (uint32_t)planetOffset+sizeof(MwPlanetHeader)>=PLANET_RECORDS_END) abort();
    const uint8_t *record=&planetRecordStorage[planetOffset-PLANET_RECORDS_START];
    const MwPlanetHeader *planet=(const MwPlanetHeader *)record;
    size_t bytesRemaining=PLANET_RECORDS_END-planetOffset-sizeof(MwPlanetHeader);
    const uint8_t *name=record+sizeof(MwPlanetHeader);
    const uint8_t *nameEnd=memchr(name,0,bytesRemaining);
    if (!nameEnd) abort();
    MwNativeByteView nameView={name,0,(uint32_t)(nameEnd-name+1)};
    EgaDrawTransparentText(TRAVEL_DESTINATION_TEXT_X,TRAVEL_HEADER_TEXT_Y,Campaign_Pal_EGA_White,&nameView,0);
    uint16_t pictureSelector=MwLoadIndexedPicture((uint16_t)(TRAVEL_HOUSE_PICTURE_BASE+planet->houseIndex));
    EgaDrawPackedSprite(TRAVEL_HOUSE_PICTURE_X,TRAVEL_HOUSE_PICTURE_Y,EGA_DISPLAY_PAGE,0,pictureSelector);
    uint8_t classification=planet->landscapeAndMarketCode-1;
    const char *landscape=travelLandscapeLabels[classification%TRAVEL_LANDSCAPE_LABEL_COUNT];
    MwNativeByteView landscapeView={(const uint8_t *)landscape,0,(uint32_t)strlen(landscape)+1};
    EgaDrawTransparentText(TRAVEL_LANDSCAPE_TEXT_X,TRAVEL_HEADER_TEXT_Y,Campaign_Pal_EGA_White,&landscapeView,0);
    uint16_t population=MwReadNativeWord(planet->populationMillions,0);
    numericGroupingEnabled++;
    MwDrawSignedDecimalField(population,TRAVEL_POPULATION_TEXT_X,0,TRAVEL_POPULATION_DIGITS,Campaign_Pal_EGA_White,Campaign_Pal_EGA_LightGrey,TRAVEL_HEADER_TEXT_Y);
    MwNativeByteView suffix={(const uint8_t *)travelPopulationSuffix,0,sizeof(travelPopulationSuffix)};
    EgaDrawTransparentText(TRAVEL_POPULATION_SUFFIX_X,TRAVEL_HEADER_TEXT_Y,Campaign_Pal_EGA_White,&suffix,0);
    const uint8_t *description=nameEnd+1;
    size_t descriptionCapacity=bytesRemaining-(size_t)(description-name);
    const uint8_t *descriptionEnd=memchr(description,0,descriptionCapacity);
    if (!descriptionEnd) abort();
    MwNativeByteView descriptionView={description,0,(uint32_t)(descriptionEnd-description+1)};
    EgaDrawTransparentText(TRAVEL_DESTINATION_TEXT_X,TRAVEL_DESCRIPTION_TEXT_Y,Campaign_Pal_EGA_White,&descriptionView,0);
}

/* MW_MAIN 081B:569F..56C2, then the fallthrough56C3..56E6. The genuine5816
 * procedure returns here before the destination marker is repositioned. */
uint16_t MwSelectTravelDestinationContinuation(uint16_t planetIndex)
{
    travelDestinationPlanetIndex=planetIndex;
    MwCopyTravelDestinationName();
    MwDrawDescriptorTextPanel(&travelDestinationHeaderPanel);
    MwDrawDescriptorTextPanel(&travelCostPanel);
    MwDrawDescriptorTextPanel(&travelConfirmPanel);
    MwDrawDescriptorTextPanel(&travelBrowsePanel);
    MwDrawTravelDestinationDetails();
    MwDrawPlanetTravelQuote();
    uint16_t planetOffset=planetRecordOffsets[travelDestinationPlanetIndex];
    if (planetOffset<PLANET_RECORDS_START || (uint32_t)planetOffset+sizeof(MwPlanetHeader)>PLANET_RECORDS_END) abort();
    const MwPlanetHeader *planet=(const MwPlanetHeader *)&planetRecordStorage[planetOffset-PLANET_RECORDS_START];
    travelMarkerX=(uint16_t)(planet->mapPositionX-TRAVEL_MAP_MARKER_INSET);
    travelMarkerY=(uint16_t)(planet->mapPositionY-TRAVEL_MAP_MARKER_INSET);
    MwDrawTravelMarker(); /* Native56E4, before shared56E7. */
    return MwEnterTravelMapMenuContinuation();
}

/* MW_MAIN 081B:56E7..56FA. Shared map re-entry after destination, search,
 * browser cancellation and marker-boundary paths. */
uint16_t MwEnterTravelMapMenuContinuation(void)
{
    uiSelectionChangedCodeOffset=CAMPAIGN_CALLBACK_SELECT_TRAVEL_MAP_PLANET;
    uint16_t selectionTransfer=MwSelectUiRecordTable(travelMapSelection,&travelMapMenu);
    return selectionTransfer!=0?selectionTransfer:CAMPAIGN_CONTINUE_UI;
}

/* MW_MAIN 081B:5816..58F3. Arithmetic widths follow the native MULs: the
 * passenger base loses DX before distance multiplication, and the hired-Mech
 * distance product is BYTE by BYTE. Same-planet travel leaves the DropShip
 * selection byte untouched. */
void MwDrawPlanetTravelQuote(void)
{
    EgaFillDisplayPageRectangle(TRAVEL_QUOTE_CLEAR_X,TRAVEL_QUOTE_CLEAR_Y,TRAVEL_QUOTE_CLEAR_WIDTH,TRAVEL_QUOTE_CLEAR_HEIGHT,Campaign_Pal_EGA_LightGrey);
    travelQuoteCbillsLow=0;
    travelQuoteCbillsHigh=0;
    if (travelDestinationPlanetIndex!=currentPlanetIndex)
    {
        uint16_t distance=MwCalculatePlanetTravelDistance(travelDestinationPlanetIndex);
        if (ownedUnitCount==0)
        {
            uint16_t passengerBase=pilotRosterCount*PASSENGER_TRAVEL_CBILLS_PER_DISTANCE_UNIT;
            uint32_t quote=(uint32_t)passengerBase*distance;
            travelQuoteCbillsLow=(uint16_t)quote;
            travelQuoteCbillsHigh=(uint16_t)(quote>>NATIVE_WORD_BITS);
            MwAddReserveAmmoFreightToTravelQuote();
        }
        else
        {
            travelDropShipSelected=0;
            uint16_t ownedSlot=0;
            while (ownedSlot!=CAMPAIGN_OWNED_UNIT_SLOTS
                && ownedUnitKinds[ownedSlot]!=CAMPAIGN_UNIT_DROPSHIP) ++ownedSlot;
            if (ownedSlot!=CAMPAIGN_OWNED_UNIT_SLOTS)
            {
                travelDropShipSelected=1;
                uint32_t quote=(uint32_t)travelDistanceUnits*OWNED_DROPSHIP_TRAVEL_CBILLS_PER_DISTANCE_UNIT;
                travelQuoteCbillsLow=(uint16_t)quote;
                travelQuoteCbillsHigh=(uint16_t)(quote>>NATIVE_WORD_BITS);
            }
            else
            {
                uint32_t base=(uint32_t)ownedUnitCount*HIRED_MECH_TRAVEL_BASE_CBILLS;
                uint16_t unitDistance=(uint8_t)ownedUnitCount*(uint8_t)travelDistanceUnits;
                uint32_t quote=(uint32_t)unitDistance*HIRED_MECH_TRAVEL_CBILLS_PER_DISTANCE_UNIT+base;
                travelQuoteCbillsLow=(uint16_t)quote;
                travelQuoteCbillsHigh=(uint16_t)(quote>>NATIVE_WORD_BITS);
                MwAddReserveAmmoFreightToTravelQuote();
            }
        }
    }
    ++numericGroupingEnabled;
    MwDrawSignedDecimalField(((uint32_t)travelQuoteCbillsHigh<<NATIVE_WORD_BITS)|travelQuoteCbillsLow,
        TRAVEL_QUOTE_X,1,0,2,3,TRAVEL_QUOTE_Y);
    ++numericGroupingEnabled;
    MwDrawSignedDecimalField(((uint32_t)campaignFundsHigh<<NATIVE_WORD_BITS)|campaignFundsLow,
        TRAVEL_QUOTE_X,1,0,2,3,TRAVEL_FUNDS_Y);
}

/* MW_MAIN 081B:5620..569E. Entry discards the UI callback return natively;
 * C represents the following569F continuation explicitly. */
uint16_t MwOpenTravelScreenContinuation(void)
{
    hubSelectedRecordIndex=3;
    travelMapSelection=0;
    EgaSelectFont(0);
    (void)MwLoadIndexedPicture((uint16_t)(TRAVEL_HOUSE_PICTURE_BASE+currentPlanetHouseIndex));
    MwDrawActivePicture(0,0,EGA_DRAW_PAGE,TRAVEL_MAP_PICTURE);
    MwDrawIndexedPicture(0x00EF,0x0021,(uint16_t)(TRAVEL_HOUSE_PICTURE_BASE+currentPlanetHouseIndex));
    EgaFlipPages();
    uint16_t directoryByteOffset=currentPlanetIndex*sizeof(uint16_t);
    if (directoryByteOffset>=sizeof(planetRecordOffsets)) abort();
    uint16_t planetOffset=planetRecordOffsets[directoryByteOffset/sizeof(uint16_t)];
    if (planetOffset<PLANET_RECORDS_START || (uint32_t)planetOffset+sizeof(MwPlanetHeader)>PLANET_RECORDS_END) abort();
    const MwPlanetHeader *planet=(const MwPlanetHeader *)&planetRecordStorage[planetOffset-PLANET_RECORDS_START];
    travelMarkerX=(uint16_t)(planet->mapPositionX-TRAVEL_MAP_MARKER_INSET);
    travelMarkerY=(uint16_t)(planet->mapPositionY-TRAVEL_MAP_MARKER_INSET);
    MwDrawTravelMarker();
    EgaCopyDisplayToDrawPage(0,0,EGA_COLUMNS,EGA_ROWS);
    return MwSelectTravelDestinationContinuation(currentPlanetIndex);
}

/* MW_MAIN 081B:5C5D..5CF1. The following569F/56E7 transfers remain explicit
 * because both are shared native continuations, not application return values. */
uint16_t MwSearchTravelPlanetContinuation(void)
{
    uiSelectionChangedCodeOffset=CAMPAIGN_CALLBACK_SELECT_TRAVEL_MAP_PLANET;
    EgaFillDisplayPageRectangle(TRAVEL_SEARCH_X,TRAVEL_SEARCH_Y,
        TRAVEL_SEARCH_WIDTH,TRAVEL_SEARCH_HEIGHT,TRAVEL_SEARCH_BACKGROUND);
    uint16_t transfer=MwReadCampaignTextInput(TRAVEL_SEARCH_X,TRAVEL_SEARCH_Y,
        TRAVEL_SEARCH_CAPACITY,TRAVEL_SEARCH_FOREGROUND,TRAVEL_SEARCH_BACKGROUND);
    if (transfer!=0) return transfer;
    /* 5C89 is an inline loop, not a callable5C87 procedure. Only leading
     * spaces are skipped; a mismatch does not retry at later input bytes. */
    uint16_t inputIndex=0;
    while (campaignTextInputBuffer[inputIndex]==' ') ++inputIndex;
    if (campaignTextInputBuffer[inputIndex]!=0)
    {
        for (uint16_t planetIndex=0;planetIndex<TRAVEL_PLANET_DIRECTORY_COUNT;++planetIndex)
        {
            uint16_t planetOffset=planetRecordOffsets[planetIndex];
            const uint8_t *name=&planetRecordStorage[planetOffset-PLANET_RECORDS_START+sizeof(MwPlanetHeader)];
            uint16_t nameIndex=0,compareIndex=inputIndex;
            while (name[nameIndex]!=0 && name[nameIndex]==campaignTextInputBuffer[compareIndex])
            {++nameIndex;++compareIndex;}
            if (name[nameIndex]==0)
            {
                MwRestoreTravelMarkerRegion();
                travelDestinationPlanetIndex=planetIndex;
                return CAMPAIGN_CONTINUE_SELECT_TRAVEL_DESTINATION;
            }
        }
    }
    /* 5CC4..5CE9 restores details without requoting or moving the marker. */
    EgaFillDisplayPageRectangle(TRAVEL_SEARCH_X+1,TRAVEL_SEARCH_Y,
        TRAVEL_SEARCH_WIDTH,TRAVEL_SEARCH_HEIGHT,3);
    MwDrawTravelDestinationDetails();
    uint16_t selectionTransfer=MwSelectUiRecordTable(0,&travelMapMenu);
    return selectionTransfer!=0?selectionTransfer:CAMPAIGN_CONTINUE_UI;
}

/* MW_MAIN 081B:5CF2..5D20. The native POP discards the invoking UI return;
 * this host return transports the following089B jump. */
uint16_t MwOpenTravelHouseBrowserContinuation(void)
{
    MwRestoreTravelMarkerRegion();
    travelSelectedHouseRow=0;
    EgaCopyDisplayToDrawPage(TRAVEL_HOUSE_BROWSER_X,TRAVEL_HOUSE_BROWSER_Y,
        TRAVEL_HOUSE_BROWSER_WIDTH,TRAVEL_HOUSE_BROWSER_HEIGHT);
    MwDrawDescriptorTextPanel(&travelHouseBrowserPanel);
    MwSelectUiRecordTable(travelSelectedHouseRow,&travelHouseBrowserMenu);
    return CAMPAIGN_CONTINUE_UI;
}

/* MW_MAIN 081B:5D21 POPs the UI dispatch word;5D22..5D31 restores the browser
 * rectangle and rejoins the map with native selection state02. */
uint16_t MwCloseTravelHouseBrowserContinuation(void)
{
    MwRestoreTravelHouseBrowserRegion();
    MwDrawTravelMarker(); /* Native5D25, before shared56E7. */
    travelMapSelection=2;
    return CAMPAIGN_CONTINUE_TRAVEL_MAP;
}

/* MW_MAIN 081B:5D32..5D44. */
void MwRestoreTravelHouseBrowserRegion(void)
{
    EgaCopyDrawPageToDisplay(TRAVEL_HOUSE_BROWSER_X,TRAVEL_HOUSE_BROWSER_Y,
        TRAVEL_HOUSE_BROWSER_WIDTH,TRAVEL_HOUSE_BROWSER_HEIGHT);
}

/* MW_MAIN 081B:5D45..5E24. Alphabetic entries still point at canonical
 * variable-length planet records; no second planet schema is invented. */
uint16_t MwOpenSelectedHousePlanetListContinuation(void)
{
    MwRestoreTravelHouseBrowserRegion();
    travelSelectedHouseRow=selectedUiRecordIndex;
    if (travelSelectedHouseRow>=TRAVEL_HOUSE_COUNT) abort();
    travelSelectedHouseByteOffset=(uint16_t)(travelSelectedHouseRow*sizeof(uint16_t));
    travelHousePlanetStart=housePlanetRangeStarts[travelSelectedHouseRow];
    travelHousePlanetCount=housePlanetRangeCounts[travelSelectedHouseRow];
    strcpy(travelHouseSearchCaption.text,travelHouseSearchNames[travelSelectedHouseRow]);
    if ((uint32_t)travelHousePlanetStart+travelHousePlanetCount>TRAVEL_PLANET_DIRECTORY_COUNT) abort();
    for (uint16_t row=0;row<travelHousePlanetCount;++row)
    {
        uint16_t planetOffset=alphabeticPlanetRecordOffsets[travelHousePlanetStart+row];
        if (planetOffset<PLANET_RECORDS_START || (uint32_t)planetOffset+sizeof(MwPlanetHeader)>=PLANET_RECORDS_END) abort();
        const char *name=(const char *)&planetRecordStorage[planetOffset-PLANET_RECORDS_START+sizeof(MwPlanetHeader)];
        if (strlen(name)>=sizeof(travelHousePlanetRows[row].text)) abort();
        strcpy(travelHousePlanetRows[row].text,name);
    }
    for (uint16_t row=0;row<5;++row)
        travelHousePlanetMenu.records[10+row].rightRecordIndex=(uint8_t)(0x19+row);
    travelHousePlanetMenu.records[0x18].downRecordIndex=0x19;
    if (travelSelectedHouseRow==TRAVEL_HOUSE_LIAO_INDEX)
    {
        for (uint16_t row=0;row<5;++row)
        {
            travelHousePlanetMenu.records[10+row].rightRecordIndex=0xFF;
            travelHousePlanetRows[TRAVEL_HOUSE_LIAO_PLANET_COUNT+row].text[0]='\0';
        }
        travelHousePlanetMenu.records[0x18].downRecordIndex=TRAVEL_PLANET_ROWS_PER_HOUSE;
    }
    EgaCopyDisplayToDrawPage(TRAVEL_HOUSE_PLANET_X,TRAVEL_HOUSE_PLANET_Y,
        TRAVEL_HOUSE_PLANET_WIDTH,TRAVEL_HOUSE_PLANET_HEIGHT);
    MwDrawDescriptorTextPanel(&travelHousePlanetPanel);
    MwSelectUiRecordTable(0,&travelHousePlanetMenu);
    return CAMPAIGN_CONTINUE_UI;
}

/* MW_MAIN 081B:5E26 is the authored UI entry. It discards the dispatch word
 * before rejoining the browser with the previously selected House row. */
uint16_t MwReturnToTravelHouseBrowserContinuation(void)
{
    MwRestoreTravelHousePlanetListRegion();
    EgaCopyDisplayToDrawPage(TRAVEL_HOUSE_BROWSER_X,TRAVEL_HOUSE_BROWSER_Y,
        TRAVEL_HOUSE_BROWSER_WIDTH,TRAVEL_HOUSE_BROWSER_HEIGHT);
    MwDrawDescriptorTextPanel(&travelHouseBrowserPanel);
    MwSelectUiRecordTable(travelSelectedHouseRow,&travelHouseBrowserMenu);
    return CAMPAIGN_CONTINUE_UI;
}

/* MW_MAIN 081B:5E2C..5E40. */
void MwRestoreTravelHousePlanetListRegion(void)
{
    EgaCopyDrawPageToDisplay(TRAVEL_HOUSE_PLANET_X,TRAVEL_HOUSE_PLANET_Y,
        TRAVEL_HOUSE_PLANET_WIDTH,TRAVEL_HOUSE_PLANET_HEIGHT);
}

/* MW_MAIN 081B:5E41 discards the dispatch word and restores the list region;
 * the first record byte is the primary planet index plus one for569F. */
uint16_t MwSelectHousePlanetContinuation(void)
{
    MwRestoreTravelHousePlanetListRegion();
    uint16_t alphabeticIndex=travelHousePlanetStart+selectedUiRecordIndex;
    if (alphabeticIndex>=TRAVEL_PLANET_DIRECTORY_COUNT) abort();
    uint16_t planetOffset=alphabeticPlanetRecordOffsets[alphabeticIndex];
    if (planetOffset<PLANET_RECORDS_START || planetOffset>=PLANET_RECORDS_END) abort();
    uint8_t primaryPlanetNumber=planetRecordStorage[planetOffset-PLANET_RECORDS_START];
    if (primaryPlanetNumber==0) abort();
    travelDestinationPlanetIndex=(uint16_t)(primaryPlanetNumber-1);
    return CAMPAIGN_CONTINUE_SELECT_TRAVEL_DESTINATION;
}

/* MW_MAIN 081B:5E5D..5E9F. SUB/SBB supplies one unsigned affordability
 * decision; the successful halves are stored only after the borrow test. */
uint16_t MwConfirmPlanetTravelContinuation(void)
{
    if (travelDestinationPlanetIndex==currentPlanetIndex)
    {
        uiSelectionChangedCodeOffset=CAMPAIGN_CALLBACK_SELECT_TRAVEL_MAP_PLANET;
        return 0;
    }
    uint32_t funds=((uint32_t)campaignFundsHigh<<NATIVE_WORD_BITS)|campaignFundsLow;
    uint32_t quote=((uint32_t)travelQuoteCbillsHigh<<NATIVE_WORD_BITS)|travelQuoteCbillsLow;
    if (funds<quote)
    {
        MwRestoreTravelMarkerRegion();
        MwNativeByteView message={(const uint8_t *)travelInsufficientFundsText,0,
            sizeof(travelInsufficientFundsText)};
        MwTextPanelReturn result=MwShowMonospaceTextPanel(0x0010,0x0010,0x0019,6,&message,0);
        if (result.discardReturnWord) abort();
        MwDrawTravelMarker();
        uiSelectionChangedCodeOffset=CAMPAIGN_CALLBACK_SELECT_TRAVEL_MAP_PLANET;
        return 0;
    }
    funds-=quote;
    campaignFundsLow=(uint16_t)funds;
    campaignFundsHigh=(uint16_t)(funds>>NATIVE_WORD_BITS);
    return CAMPAIGN_CONTINUE_PAID_TRAVEL;
}

/* MW_MAIN 081B:5EA0..5F14. Both CWD/IDIV operations divide signed map
 * deltas toward zero. Calendar calls reload live planet indices afterwards. */
uint16_t MwAdvancePaidTravelAndJoinPlanetArrivalContinuation(void)
{
    int16_t distance=travelDistanceUnits;
    int16_t deltaX=travelMapDeltaX;
    if (distance==0 || (deltaX==INT16_MIN && distance==-1)) abort();
    travelMapDeltaX=(uint16_t)(deltaX/distance);
    distance=(int16_t)travelDistanceUnits;
    int16_t deltaY=travelMapDeltaY;
    if (distance==0 || (deltaY==INT16_MIN && distance==-1)) abort();
    travelMapDeltaY=(uint16_t)(deltaY/distance);
    EgaDrawLine(TRAVEL_ROUTE_COLOUR,currentPlanetMapPositionY,
        travelDestinationMapPositionX,currentPlanetMapPositionX,
        travelDestinationMapPositionY);
    uint16_t elapsedUnits=(uint16_t)(travelDistanceUnits-1)*TRAVEL_DISTANCE_TIME_UNITS;
    MwAdvanceCampaignTimeAndChargeWages(elapsedUnits);
    uint16_t planetIndex=currentPlanetIndex;
    if (planetIndex>=TRAVEL_PLANET_DIRECTORY_COUNT) abort();
    uint16_t planetOffset=planetRecordOffsets[planetIndex];
    if (planetOffset<PLANET_RECORDS_START || (uint32_t)planetOffset+sizeof(MwPlanetHeader)>PLANET_RECORDS_END) abort();
    const MwPlanetHeader *planet=(const MwPlanetHeader *)&planetRecordStorage[planetOffset-PLANET_RECORDS_START];
    MwAdvanceCampaignTimeAndChargeWages((uint16_t)(planet->travelTimeWeight<<1));
    planetIndex=travelDestinationPlanetIndex;
    if (planetIndex>=TRAVEL_PLANET_DIRECTORY_COUNT) abort();
    planetOffset=planetRecordOffsets[planetIndex];
    if (planetOffset<PLANET_RECORDS_START || (uint32_t)planetOffset+sizeof(MwPlanetHeader)>PLANET_RECORDS_END) abort();
    planet=(const MwPlanetHeader *)&planetRecordStorage[planetOffset-PLANET_RECORDS_START];
    MwAdvanceCampaignTimeAndChargeWages((uint16_t)(planet->travelTimeWeight<<1));
    return CAMPAIGN_CONTINUE_PLANET_ARRIVAL;
}

/* MW_MAIN 081B:5F15..5F3D. This is the paid-travel fall-through, not an
 * independently called status function. The returned code transports JMP1909
 * for the host dispatcher; native AX is not a result. */
uint16_t MwFinishPlanetArrivalContinuation(void)
{
    currentPlanetIndex=travelDestinationPlanetIndex;
    MwApplyCurrentPlanetState();
    MwRefreshMarketUnitStock();
    MwRefreshMissionOffers();
    MwRefreshAvailablePilotRecruits();
    if (travelDropShipSelected!=0)
        MwPlayOwnedDropShipAnimation();
    else
        MwPlayHiredTransportAnimation();
    planetArrivalEventsPending=1;
    return CAMPAIGN_CONTINUE_PLANET_HUB;
}

/* MW_MAIN 081B:6095..60AC joins the shared6031 reverse-animation loop.
 * Timer delivery is a host boundary; input polling remains native game code. */
void MwPlayHiredTransportAnimation(void)
{
    travelAnimationFrameStep=0xFFFF;
    uint16_t frameIndex=0x0020;
    MwDrawIndexedPicture(0,0,HIRED_TRANSPORT_BACKGROUND_PICTURE);
    EgaCopyDrawPageToDisplay(0,0,EGA_COLUMNS,EGA_ROWS);
    uint16_t nextFrameTick=campaignTimerTicks+1;
    for (;;)
    {
        for (;;)
        {
            if (MwPollInputOnce()) return;
            if (nextFrameTick<=campaignTimerTicks) break;
            if (!mwCampaignInput.waitForInterrupt) abort();
            mwCampaignInput.waitForInterrupt();
        }
        nextFrameTick=(uint16_t)(campaignTimerTicks+TRAVEL_ANIMATION_FRAME_TICKS);
        uint16_t pictureIndex;
        if (frameIndex==UINT16_MAX)
            pictureIndex=hiredTransportReverseTerminator;
        else
        {
            if (frameIndex>=HIRED_TRANSPORT_FRAME_COUNT) abort();
            pictureIndex=hiredTransportFrames[frameIndex];
        }
        if (pictureIndex==TRAVEL_ANIMATION_TERMINATOR) break;
        MwDrawFourthBankPicture(HIRED_TRANSPORT_X,HIRED_TRANSPORT_Y,EGA_DRAW_PAGE,pictureIndex);
        EgaFlipPages();
        frameIndex=(uint16_t)(frameIndex+travelAnimationFrameStep);
    }
    uint16_t finalTick=campaignTimerTicks+TRAVEL_ANIMATION_FINAL_TICKS;
    while (finalTick>campaignTimerTicks)
    {
        if (!mwCampaignInput.waitForInterrupt) abort();
        mwCampaignInput.waitForInterrupt();
    }
}

/* MW_MAIN 081B:60AD..6121. Unlike the hired animation this path never polls
 * input and walks its DropShip frame table only in the forward direction. */
void MwPlayOwnedDropShipAnimation(void)
{
    MwDrawIndexedPicture(0,0,DROPSHIP_BACKGROUND_PICTURE);
    MwDrawAlternateBankPicture(DROPSHIP_X,DROPSHIP_Y,EGA_DRAW_PAGE,DROPSHIP_INITIAL_PICTURE);
    EgaCopyDrawPageToDisplay(0,0,EGA_COLUMNS,EGA_ROWS);
    uint16_t frameIndex=0;
    uint16_t nextFrameTick=campaignTimerTicks+1;
    for (;;)
    {
        while (nextFrameTick>campaignTimerTicks)
        {
            if (!mwCampaignInput.waitForInterrupt) abort();
            mwCampaignInput.waitForInterrupt();
        }
        nextFrameTick=(uint16_t)(campaignTimerTicks+TRAVEL_ANIMATION_FRAME_TICKS);
        if (frameIndex>=DROPSHIP_TRAVEL_FRAME_COUNT) abort();
        uint16_t pictureIndex=dropShipTravelFrames[frameIndex];
        if (pictureIndex==TRAVEL_ANIMATION_TERMINATOR) break;
        MwDrawAlternateBankPicture(DROPSHIP_X,DROPSHIP_Y,EGA_DRAW_PAGE,pictureIndex);
        EgaFlipPages();
        ++frameIndex;
    }
    uint16_t finalTick=campaignTimerTicks+TRAVEL_ANIMATION_FINAL_TICKS;
    while (finalTick>campaignTimerTicks)
    {
        if (!mwCampaignInput.waitForInterrupt) abort();
        mwCampaignInput.waitForInterrupt();
    }
}

/* MW_MAIN 081B:5FE6..5FEE. Authored WORDs, not a computed square root.
 * Both SHL and the final near address wrap before resolving owned storage. */
uint16_t MwLookupScaledSquareRoot(uint16_t tableIndex)
{
    uint16_t byteOffset = tableIndex * sizeof(uint16_t);
    uint16_t nativeOffset = TRAVEL_ROOT_TABLE_START + byteOffset;
    uint16_t tableByteOffset = nativeOffset - TRAVEL_ROOT_TABLE_START;
    if (tableByteOffset >= sizeof(travelSquareRootValues)) abort();
    return travelSquareRootValues[tableByteOffset / sizeof(uint16_t)];
}

/* MW_MAIN 081B:5F5C..5FE5. Decimal magnitude bands deliberately lose
 * precision. Large inputs use consecutive DIV FFFF/DIV0099, not ten million. */
uint16_t MwApproximateSquareRoot(uint32_t squaredMagnitude)
{
    uint16_t highWord = squaredMagnitude >> NATIVE_WORD_BITS;
    if (highWord == 0)
    {
        if (squaredMagnitude >= TRAVEL_ROOT_MEDIUM_BOUNDARY)
            return MwLookupScaledSquareRoot((uint16_t)(squaredMagnitude / DECIMAL_THOUSAND)) * DECIMAL_TEN;
        if (squaredMagnitude < TRAVEL_ROOT_FINE_BOUNDARY)
            return MwLookupScaledSquareRoot((uint16_t)(squaredMagnitude * DECIMAL_TEN)) / DECIMAL_TEN;
        return MwLookupScaledSquareRoot((uint16_t)(squaredMagnitude / DECIMAL_TEN));
    }
    if (squaredMagnitude < TRAVEL_ROOT_MILLION)
        return MwLookupScaledSquareRoot((uint16_t)(squaredMagnitude / DECIMAL_THOUSAND)) * DECIMAL_TEN;
    if (squaredMagnitude < TRAVEL_ROOT_HUNDRED_MILLION)
    {
        uint16_t reducedMagnitude = squaredMagnitude / DECIMAL_TEN_THOUSAND;
        return MwLookupScaledSquareRoot(reducedMagnitude / DECIMAL_TEN) * DECIMAL_HUNDRED;
    }
    if (highWord >= TRAVEL_ROOT_SATURATION_HIGH) return UINT16_MAX;
    uint16_t reducedMagnitude = squaredMagnitude / TRAVEL_ROOT_LARGE_DIVISOR;
    return MwLookupScaledSquareRoot(reducedMagnitude / TRAVEL_ROOT_LARGE_SECOND_DIVISOR) * DECIMAL_THOUSAND;
}

/* MW_MAIN 081B:5912..5989. Original map-coordinate arithmetic, NOT elapsed
 * days or a tabletop movement cost. Destination snapshots precede division. */
uint16_t MwCalculatePlanetTravelDistance(uint16_t destinationPlanetIndex)
{
    uint16_t directoryByteOffset = destinationPlanetIndex * sizeof(uint16_t);
    if (directoryByteOffset >= sizeof(planetRecordOffsets)) abort();
    uint16_t planetOffset = planetRecordOffsets[directoryByteOffset / sizeof(uint16_t)];
    travelDestinationPlanetOffset = planetOffset;
    if (planetOffset < PLANET_RECORDS_START
        || (uint32_t)planetOffset + sizeof(MwPlanetHeader) > PLANET_RECORDS_END) abort();
    const MwPlanetHeader *planet = (const MwPlanetHeader *)&planetRecordStorage[planetOffset - PLANET_RECORDS_START];
    travelDestinationMapPositionX = planet->mapPositionX;
    travelDestinationMapPositionY = planet->mapPositionY;
    travelDestinationTimeWeight = planet->travelTimeWeight;
    uint16_t deltaX = planet->mapPositionX - currentPlanetMapPositionX;
    travelMapDeltaX = deltaX;
    if (planet->mapPositionX < currentPlanetMapPositionX) deltaX = (uint16_t)(0 - deltaX);
    uint16_t deltaY = planet->mapPositionY - currentPlanetMapPositionY;
    travelMapDeltaY = deltaY;
    if (planet->mapPositionY < currentPlanetMapPositionY) deltaY = (uint16_t)(0 - deltaY);
    uint32_t scaledY = (uint32_t)deltaY * TRAVEL_MAP_Y_SCALE_NUMERATOR / TRAVEL_MAP_Y_SCALE_DENOMINATOR;
    if (scaledY > UINT16_MAX) abort(); /* Native DIV faults after both delta stores. */
    uint32_t squaredDistance = (uint32_t)deltaX * deltaX + scaledY * scaledY;
    uint16_t distance = MwApproximateSquareRoot(squaredDistance) / TRAVEL_DISTANCE_DIVISOR;
    if (distance == 0) distance = TRAVEL_MINIMUM_DISTANCE;
    travelDistanceUnits = distance;
    return distance;
}

/* MW_MAIN 081B:58F4..5911. Two ADDs, deliberately NOT ADD/ADC. A low
 * WORD carry is lost for every family; do not combine the quote as a DWORD. */
void MwAddReserveAmmoFreightToTravelQuote(void)
{
    for (uint16_t ammoFamily = 0; ammoFamily < CAMPAIGN_AMMO_FAMILY_COUNT; ++ammoFamily)
    {
        uint32_t charge = (uint32_t)reserveAmmoQuantitiesByFamily[ammoFamily] * reserveAmmoFreightRates[ammoFamily];
        travelQuoteCbillsLow += (uint16_t)charge;
        travelQuoteCbillsHigh += (uint16_t)(charge >> NATIVE_WORD_BITS);
    }
}
