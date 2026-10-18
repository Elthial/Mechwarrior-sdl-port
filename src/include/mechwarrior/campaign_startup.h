#ifndef MECHWARRIOR_CAMPAIGN_STARTUP_H
#define MECHWARRIOR_CAMPAIGN_STARTUP_H
#include "mechwarrior/campaign_input.h"
#include "mechwarrior/native_address.h"
#include "mechwarrior/campaign_exchange.h"

enum { CAMPAIGN_PHASE_NEW=1, CAMPAIGN_PHASE_COMBAT_RETURN=2,
    CAMPAIGN_EXIT_QUIT=0, CAMPAIGN_EXIT_COMBAT=1,
    JOYSTICK_DETECT_MINIMUM_SAMPLES=5, JOYSTICK_DETECT_TIMEOUT_SAMPLES=0x03E8 };
/* Host projection of the launcher's PSP/FCB contract, not a serialized struct
 * or additional source arguments. The resident view includes the identity
 * bytes beyond the exchange buffer which0002 genuinely tests. */
typedef struct MwCampaignLaunch
{
    MwNativeByteView resident;
    uint16_t transferOffset;
    uint8_t phase, graphicsCode, memoryTier;
} MwCampaignLaunch;

typedef struct MwCampaignPlatformCalls
{
    bool (*allocateParagraphs)(uint16_t paragraphs,uint16_t *selectorOrError);
    bool (*loadGraphicsOverlay)(const char *filename,uint16_t selector);
    void (*installInterrupts)(void);
    void (*restoreInterrupts)(void);
    uint8_t (*getVideoMode)(void);
    void (*setVideoMode)(uint8_t mode);
    uint32_t (*readBiosTicks)(void);
    uint16_t (*resetMouse)(void);
    void (*displayDosString)(const char *text);
} MwCampaignPlatformCalls;
extern MwCampaignPlatformCalls mwCampaignPlatform;
/* Explicit temporary dependency bindings, never successful no-op defaults.
 * These are transport for the unconverted native continuation/music body. */
extern void (*campaignStopMusic)(void);
extern void (*campaignStartMusic)(uint16_t selector,uint16_t offset);
extern uint16_t campaignMusicBufferSelector, graphicsOverlaySelector;
extern uint16_t launcherMemoryTier;
extern uint8_t launcherExitCode, tandyGraphicsSelected, savedVideoMode;
extern uint8_t keyboardInterruptState;
extern char startupErrorText[0x0120];
extern const char *startupErrorMessages[7];
uint8_t MwRunCampaign(const MwCampaignLaunch *launch);
void MwInstallCampaignInterruptHandlers(void);
void MwRestoreCampaignInterruptHandlers(void);
void MwSaveCurrentVideoMode(void);
void MwRestoreSavedVideoMode(void);
void MwDetectMouseAndJoystick(void);
void MwDetectAndCalibrateInputDevices(void);
void MwMixBiosTicksIntoRandomState(void);
typedef struct MwIntroPositionedText
{
    uint16_t x,y;
    char text[0x80];
} MwIntroPositionedText;
extern char introNarrationLines[10][0x50];
extern MwIntroPositionedText introFirstLeftText,introFirstRightText;
extern MwIntroPositionedText introSecondLeftText,introSecondRightText;
extern MwIntroPositionedText introThirdLeftText,introThirdRightText;
extern MwIntroPositionedText introFourthLeftText,introFourthRightText;
extern MwIntroPositionedText introFifthLeftText,introFifthRightText;
extern MwIntroPositionedText introSixthLeftText,introSixthRightText;
extern MwIntroPositionedText introSeventhLeftText,introSeventhRightText;
extern uint16_t musicFileHandle;
bool MwLoadMechMusic(void);
bool MwOpenAndLoadMusicFile(const char *filename);
bool MwReadMusicStreamIntoBuffer(uint16_t fileHandle);
void MwDrawIntroNarrationLines(uint8_t colour);
bool MwWaitTicksOrInputInterruption(uint16_t delayTicks);
void MwSetIntroTextPaletteColour(uint8_t paletteCode);
void MwDrawPositionedIntroText(const MwIntroPositionedText *text);
/* Host projection of a native nonlocal transfer, NOT an AX result ABI. */
uint16_t MwDrawTimedPictureAndTextTransition(uint16_t delayTicks,
    const MwIntroPositionedText *leftText,const MwIntroPositionedText *rightText);
enum { COPY_PROTECTION_MECH_COUNT=8, COPY_PROTECTION_CALLSIGNS_PER_MECH=3,
    COPY_PROTECTION_QUESTION_COUNT=24, COPY_PROTECTION_TEXT_CAPACITY=0x20 };
extern uint16_t copyProtectionSelection;
extern uint16_t copyProtectionPictureIndices[COPY_PROTECTION_MECH_COUNT];
extern char copyProtectionCallsigns[COPY_PROTECTION_QUESTION_COUNT][COPY_PROTECTION_TEXT_CAPACITY];
extern char copyProtectionAnswers[COPY_PROTECTION_QUESTION_COUNT][COPY_PROTECTION_TEXT_CAPACITY];
extern char copyProtectionPrompt[0x85],copyProtectionAcceptedText[0x33];
extern uint8_t campaignTextInputBuffer[0x29];
bool MwLoadBarDeathAndLaunchMusic(void);
/* Folded166D/181A flow. Returned code offset is host JMP transport, not AX. */
uint16_t MwRunCopyProtectionAndInitializeCampaign(uint16_t entryOffset);
extern uint16_t campaignEventPlanetSelectors[6];
extern uint8_t eventPlanetChoicePool0[8],eventPlanetChoicePool1[3];
extern uint8_t eventPlanetChoicePool2[4],eventPlanetChoicePool3[3];
extern uint8_t eventPlanetChoicePool4[4],eventPlanetChoicePool5[3];
#endif
