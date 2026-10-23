#ifndef MECHWARRIOR_COMBAT_COCKPIT_H
#define MECHWARRIOR_COMBAT_COCKPIT_H
#include "mechwarrior/combat_video.h"
#include "mechwarrior/combat_world.h"
/* Modal text references replace the unobserved near string offsets. */
typedef struct BtechDialogMessagePair {const char *firstText,*secondText;} BtechDialogMessagePair;
extern char simulationText[11],pausedText[7],retreatQuestionText[14],retreatChoiceText[14];
extern uint16_t savedLineColour,savedFillColour,savedCopySourcePage,savedDrawPage;
extern uint16_t savedTextBackground,savedTextForeground,savedPolygonFilled,savedPolygonClip;
void BtechShowSimulationPausedDialog(void);
void BtechShowSimulationPausedDialogFar(void);
uint16_t BtechConfirmRetreatDialog(void);
uint16_t BtechShowCapturedTwoLineDialog(const BtechDialogMessagePair *message);
void BtechSaveDrawingState(void);
void BtechRestoreDrawingState(void);

enum { COMBAT_COCKPIT_LAYOUTS=3,COMBAT_BRIEFING_MISSIONS=36,COMBAT_PILOT_NAMES=43,
    COMBAT_COCKPIT_PLACEMENTS=6,COMBAT_FRIENDLY_SLOTS=4,
    COMBAT_BRIEFING_MAP_ALTITUDE=0x10000,COMBAT_BRIEFING_TEXT_COLOUR=Combat_Pal_EGA_Yellow,COMBAT_COCKPIT_IMPACT_COLOUR=Combat_Pal_EGA_LightRed,
    COMBAT_SCREEN_BUFFER_BYTES=0x0C80,COMBAT_PACKED_SCREEN_ROW_BYTES=160 };
enum { COMBAT_DECIMAL_FIELD_BYTES=10,COCKPIT_LEFT_LABEL_X=0x3C,COCKPIT_RIGHT_LABEL_X=0xDC,
    COCKPIT_STATUS_TEXT_Y=0x60,COCKPIT_ZOOM_VALUE_X_OFFSET=0x24,COCKPIT_ZOOM_SHIFT_BASE=6,
    COCKPIT_REACTOR_WARNING_X=0x58,COCKPIT_REACTOR_WARNING_Y=0x58,
    COCKPIT_WARNING_FLASH_MASK=4,COCKPIT_REACTOR_WARNING_COLOUR=Combat_Pal_EGA_LightRed };
extern char zoomLabel[7],automaticWeaponsLabel[4],reactorShutdownText[25];
typedef union BtechDecimalFieldAndTargetCache {
    char text[COMBAT_DECIMAL_FIELD_BYTES];
    struct {char prefix[8];uint16_t targetSlot;} fields;
} BtechDecimalFieldAndTargetCache;
extern BtechDecimalFieldAndTargetCache decimalFieldAndTargetCache;
#define decimalFieldText decimalFieldAndTargetCache.text
#define cachedTargetPanelSlot decimalFieldAndTargetCache.fields.targetSlot
const char *BtechFormatSignedDecimalField(int16_t value,uint16_t fieldWidth,uint16_t padding);
const char *BtechFormatSignedDecimalFieldFar(int16_t value,uint16_t fieldWidth,uint16_t padding);
typedef struct BtechScreenRectangle {int16_t x,y,width,height;} BtechScreenRectangle;
enum { COMBAT_RADAR_MARKERS=9,COMBAT_RADAR_CACHE_PAGES=2,COMBAT_RADAR_RANGE_LABELS=4,
    RADAR_MINIMUM_RANGE_SHIFT=7,RADAR_MAXIMUM_RANGE_SHIFT=10,
    RADAR_HEADING_BACKGROUND=Combat_Pal_EGA_LightGreen,RADAR_ALTERNATE_HEADING_BACKGROUND=Combat_Pal_EGA_LightCyan };
extern uint16_t radarPreviousTick,radarMarkerCounts[COMBAT_RADAR_CACHE_PAGES];
extern BtechPooledWorldObject *radarPlayerObject;
extern uint16_t radarMarkerX[COMBAT_RADAR_MARKERS][COMBAT_RADAR_CACHE_PAGES];
extern uint16_t radarMarkerY[COMBAT_RADAR_MARKERS][COMBAT_RADAR_CACHE_PAGES];
extern uint16_t radarCentreX,radarCentreY;
extern uint16_t radarMarkerBackground[COMBAT_RADAR_MARKERS][COMBAT_RADAR_CACHE_PAGES];
extern uint16_t radarCachePage,radarMapOriginX,radarMapOriginY;
extern const BtechScreenRectangle *radarRectangle;
extern const char *radarRangeLabels[COMBAT_RADAR_RANGE_LABELS];
void BtechUpdateRadarPanel(void);
void BtechDrawHeadingRadar(void);
void BtechDrawTerrainRadar(void);
void BtechDrawRadarPositionMarker(const BtechPosition3D *position,uint16_t paletteIndex);
extern const char *weaponLabels[COMBAT_WEAPON_KINDS],*weaponRangeClassLabels[4],*energyAmmoPlaceholder;
void BtechDrawWeaponStatusList(uint16_t selectedMountIndex,uint16_t targetUnitSlot);
void BtechUpdateTargetPanelAndReticle(uint16_t targetUnitSlot);
void BtechDrawTargetDamagePanel(uint16_t targetUnitSlot,const BtechScreenRectangle *panel);
typedef struct BtechHudGaugeAnchor {int16_t x,y;} BtechHudGaugeAnchor;
extern BtechHudGaugeAnchor targetDiagramAnchors[COMBAT_MECH_TEMPLATES];
extern BtechScreenRectangle targetDiagramRegions[COMBAT_MECH_TEMPLATES][COMBAT_ARMOR_LOCATIONS];
typedef struct BtechCockpitBitmapPlacement {uint16_t bitmapIndex;int16_t x,y;} BtechCockpitBitmapPlacement;
typedef struct BtechCockpitLayout {
    BtechScreenRectangle viewport,radarPanel,targetPanel;
    int16_t weaponListX,weaponListY;
    BtechHudGaugeAnchor heatGauge,movementGauge,jumpChargeGauge;
    BtechHudGaugeAnchor firingIndicatorBank0[3],firingIndicatorBank1[3];
    uint8_t unidentifiedField15[4];
    int16_t headingStripY;
    BtechCockpitBitmapPlacement bitmapPlacements[COMBAT_COCKPIT_PLACEMENTS];
} BtechCockpitLayout;
extern uint16_t mechCockpitLayoutIndices[COMBAT_MECH_TEMPLATES];
extern BtechCockpitLayout cockpitLayouts[COMBAT_COCKPIT_LAYOUTS];
extern const char *cockpitScreenFilenames[COMBAT_COCKPIT_LAYOUTS];
extern const char *combatMissionTitles[COMBAT_BRIEFING_MISSIONS];
extern char mechTemplateNames[COMBAT_MECH_TEMPLATES][13],pilotDefinitionNames[COMBAT_PILOT_NAMES][11];
extern const char *mechTemplateNameDirectory[COMBAT_MECH_TEMPLATES],*pilotDefinitionNameDirectory[COMBAT_PILOT_NAMES];
extern const char *inspectionDamageBitmapFilenames[COMBAT_MECH_TEMPLATES];
extern char bodyLocationLabels[COMBAT_BODY_LOCATIONS][3],inspectionConditionLabels[6][11];
extern char inspectionLegendText[2][25],actuatorLabels[COMBAT_ACTUATORS][14];
extern char inspectionSubsystemLabels[5][14],heatSinksOfText[3],jumpJetsText[14];
extern char jumpJetsOfText[3],inspectionWeaponHeading[22],inspectionBlankWeaponRow[15];
extern const char errorInspectionShapeMemory[];
extern uint16_t twoHitSystemConditionLabelIndices[3],engineConditionLabelIndices[4],actuatorConditionLabelIndices[5];
extern BtechHudGaugeAnchor inspectionDiagramOrigins[COMBAT_MECH_TEMPLATES];
extern BtechScreenRectangle inspectionDiagramRegions[COMBAT_MECH_TEMPLATES][COMBAT_ARMOR_LOCATIONS];
extern const BtechMechTemplate *inspectionMechTemplate;
extern const BtechMechState *inspectionMechState;
void BtechUpdateMechInspectionScreen(uint16_t unitSlot);
void BtechClearInspectionBackground(void);
void BtechDrawMechInspectionStatus(void);
void BtechDrawMechInspectionDiagramAndLegend(void);
void BtechDrawMechDamageDiagram(void);
void BtechDrawMechInspectionLegend(void);
extern uint16_t cachedJumpChargeGaugeLevel,cachedHeatGaugeLevel,unidentifiedField25;
extern uint8_t cockpitFullRedrawPending;
typedef struct BtechHudColourScheme {
    uint16_t inspectionBackground,inspectionSubsystemText,inspectionEquipmentText;
    uint16_t inspectionEquipmentHeading,inspectionFooter,inspectionMechName;
    uint16_t orderHighlight,orderReferenceHighlight,orderConditionColours[5];
    uint16_t radarPlayer,radarFriendly,radarOpposing,radarSpecialTarget,radarBackground;
    uint16_t radarTerrainBands[4];
    uint16_t weaponReady,weaponUnavailable,weaponReloading,ammoText;
    uint16_t selectedMount,unselectedMount,forwardTicks,reverseTicks;
    uint16_t movementZeroReference,selectedMovementTick;
    uint16_t jumpRising,jumpFalling,jumpCharged,jumpBelowThreshold,jumpReference;
    uint16_t heatRising,heatFalling,rangeBuckets[5],unidentifiedField17[2];
    uint16_t undamaged,aboveHalfCondition,damaged,destroyed;
} BtechHudColourScheme;
typedef union BtechSubsystemLampColours {
    struct {uint16_t subsystem[COMBAT_HUD_COLOUR_SCHEMES][3],engine[COMBAT_HUD_COLOUR_SCHEMES][4];} tables;
    uint16_t words[14]; /* Unchecked native indices can cross the table boundary. */
} BtechSubsystemLampColours;
extern BtechHudColourScheme hudColourSchemes[COMBAT_HUD_COLOUR_SCHEMES];
extern BtechSubsystemLampColours subsystemLampColourTables;
extern char subsystemLampLabels[4][2];
extern uint16_t rememberedSensorTargetSlot;
void BtechRefreshCockpitAndSensorTarget(void);
void BtechDrawMechSubsystemLamps(BtechMechState *mech);
void BtechUpdateHeatGauge(int16_t heatLevel);
void BtechDrawForwardMovementGauge(int16_t forwardScale);
void BtechUpdateJumpChargeGauge(int16_t jumpCharge);
void BtechPresentHudRectangleIfReady(uint16_t x,uint16_t y,uint16_t width,uint16_t height);
bool BtechImportCockpitInitialData(const uint8_t *data,size_t bytes);
void BtechSetCockpitCameraViewport(BtechCameraContext *camera);
void BtechResetHudGaugeCaches(void);
void BtechRefreshCockpitViewportAndGauges(void);
void BtechRefreshCockpitViewportAndGaugesFar(void);
/* Actual4176 map/markers/display/input and post-wait roster cleanup. */
void BtechShowCombatStageBriefing(void);
void BtechDrawCockpitAimReticle(uint16_t x,uint16_t y);
void BtechDrawCockpitBitmapPlacements(void);
void BtechDrawPendingFiringIndicator(void);
void BtechDrawCockpitHeadingStrip(uint16_t bodyHeading,int16_t torsoTwist);
/* Mapped02D1: final HUD/page continuation compiled; earlier scene gates remain. */
void BtechRenderCockpitSceneAndHud(void);
#endif
