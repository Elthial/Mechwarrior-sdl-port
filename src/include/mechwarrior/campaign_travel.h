#ifndef MECHWARRIOR_CAMPAIGN_TRAVEL_H
#define MECHWARRIOR_CAMPAIGN_TRAVEL_H

#include "mechwarrior/campaign_service.h"

enum
{
    TRAVEL_HOUSE_PICTURE_BASE = 0x0025,
    TRAVEL_MAP_PICTURE = 0x0001,
    TRAVEL_MARKER_PICTURE = 0x000D,
    TRAVEL_MAP_MARKER_INSET = 0x0002,
    TRAVEL_LANDSCAPE_LABEL_COUNT = 3,
    PASSENGER_TRAVEL_CBILLS_PER_DISTANCE_UNIT = 0x09C4,
    HIRED_MECH_TRAVEL_BASE_CBILLS = 0x4E20,
    HIRED_MECH_TRAVEL_CBILLS_PER_DISTANCE_UNIT = 0x61A8,
    OWNED_DROPSHIP_TRAVEL_CBILLS_PER_DISTANCE_UNIT = 0xC350,
    TRAVEL_QUOTE_X = 0x00EF, TRAVEL_QUOTE_Y = 0x0069,
    TRAVEL_FUNDS_Y = 0x0081,
    TRAVEL_MAP_MINIMUM_X = 0x0006, TRAVEL_MAP_MAXIMUM_X = 0x00DA,
    TRAVEL_MAP_MINIMUM_Y = 0x002A, TRAVEL_MAP_MAXIMUM_Y = 0x00BB,
    TRAVEL_MARKER_HIT_SIZE = 0x0004, TRAVEL_MARKER_RESTORE_WIDTH = 0x0010,
    TRAVEL_MARKER_RESTORE_HEIGHT = 0x0007, TRAVEL_MAP_UI_RECORD = 0x0004,
    TRAVEL_MAP_MENU_RECORD_COUNT = 5,
    TRAVEL_SEARCH_X = 0x0009, TRAVEL_SEARCH_Y = 0x0008,
    TRAVEL_SEARCH_WIDTH = 0x006C, TRAVEL_SEARCH_HEIGHT = 6,
    TRAVEL_SEARCH_CAPACITY = 0x0012,
    TRAVEL_SEARCH_FOREGROUND = 8, TRAVEL_SEARCH_BACKGROUND = 1,
    TRAVEL_HOUSE_BROWSER_LINE_COUNT = 7, TRAVEL_HOUSE_BROWSER_RECORD_COUNT = 6,
    TRAVEL_HOUSE_PLANET_LINE_COUNT = 0x20,
    TRAVEL_HOUSE_PLANET_RECORD_COUNT = 0x1F,
    TRAVEL_HOUSE_LIAO_INDEX = 3, TRAVEL_HOUSE_LIAO_PLANET_COUNT = 0x19,
    TRAVEL_HOUSE_BROWSER_X = 0x007E, TRAVEL_HOUSE_BROWSER_Y = 0x003C,
    TRAVEL_HOUSE_BROWSER_WIDTH = 0x0054, TRAVEL_HOUSE_BROWSER_HEIGHT = 0x004C,
    TRAVEL_HOUSE_PLANET_X = 0x0032, TRAVEL_HOUSE_PLANET_Y = 0x001E,
    TRAVEL_HOUSE_PLANET_WIDTH = 0x00EC, TRAVEL_HOUSE_PLANET_HEIGHT = 0x00AA
};
enum { TRAVEL_ROUTE_COLOUR = 2, TRAVEL_DISTANCE_TIME_UNITS = 0x000E };
enum { HIRED_TRANSPORT_BACKGROUND_PICTURE = 0x0049, DROPSHIP_BACKGROUND_PICTURE = 0x004A,
    DROPSHIP_INITIAL_PICTURE = 0x0017, HIRED_TRANSPORT_X = 0x0035,
    HIRED_TRANSPORT_Y = 0x008D, DROPSHIP_X = 0x004B, DROPSHIP_Y = 0x008D,
    TRAVEL_ANIMATION_TERMINATOR = 0xFFFF, TRAVEL_ANIMATION_FRAME_TICKS = 2,
    TRAVEL_ANIMATION_FINAL_TICKS = 0x000A };

extern MwPanelDescriptor travelDestinationHeaderPanel;
extern MwPanelDescriptor travelCostPanel, travelConfirmPanel, travelBrowsePanel;
extern MwServiceCaption travelDestinationHeaderLabels[2];
extern MwServiceCaption travelCostLabel, travelConfirmLabel, travelBrowseLabel;
extern char travelPopulationSuffix[9];
extern char travelLandscapeLabels[TRAVEL_LANDSCAPE_LABEL_COUNT][9];
extern char travelInsufficientFundsText[0x29];
extern uint16_t travelSelectedHouseRow;
extern char travelHouseSearchNames[TRAVEL_HOUSE_COUNT][0x10];
extern MwPanelDescriptor travelHouseBrowserPanel, travelHousePlanetPanel;
extern MwServiceCaption travelHouseBrowserLabels[TRAVEL_HOUSE_BROWSER_LINE_COUNT];
extern MwServiceCaption travelHouseSearchCaption;
extern MwServiceCaption travelHousePlanetRows[TRAVEL_PLANET_ROWS_PER_HOUSE];
extern MwServiceCaption travelHousePlanetCancelLabel;
extern MwUiMenu travelHouseBrowserMenu, travelHousePlanetMenu;
extern MwUiMenu travelMapMenu;

void MwCopyTravelDestinationName(void);
void MwDrawTravelMarker(void);
void MwRestoreTravelMarkerRegion(void);
int16_t MwFindPlanetAtTravelMarker(void);
uint16_t MwHandleTravelMapSelection(void);
uint16_t MwSearchTravelPlanetContinuation(void);
void MwDrawTravelDestinationDetails(void);
void MwDrawPlanetTravelQuote(void);
uint16_t MwSelectTravelDestinationContinuation(uint16_t planetIndex);
uint16_t MwEnterTravelMapMenuContinuation(void);
uint16_t MwOpenTravelScreenContinuation(void);
uint16_t MwOpenTravelHouseBrowserContinuation(void);
uint16_t MwCloseTravelHouseBrowserContinuation(void);
void MwRestoreTravelHouseBrowserRegion(void);
uint16_t MwOpenSelectedHousePlanetListContinuation(void);
uint16_t MwReturnToTravelHouseBrowserContinuation(void);
void MwRestoreTravelHousePlanetListRegion(void);
uint16_t MwSelectHousePlanetContinuation(void);
uint16_t MwConfirmPlanetTravelContinuation(void);
uint16_t MwAdvancePaidTravelAndJoinPlanetArrivalContinuation(void);
uint16_t MwFinishPlanetArrivalContinuation(void);
void MwPlayHiredTransportAnimation(void);
void MwPlayOwnedDropShipAnimation(void);

#endif
