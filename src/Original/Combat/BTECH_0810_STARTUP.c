#include "mechwarrior/combat_startup.h"
#include "mechwarrior/combat_import.h"
#include "mechwarrior/combat_runtime.h"
#include "mechwarrior/combat_video.h"
#include "mechwarrior/combat_orders.h"
#include "mechwarrior/combat_cockpit.h"
#include <stdlib.h>

/*0810:1E84. Original EGA startup through briefing wait and cockpit setup.
 * PSP:005D/005F's resident reference is already
 * bound to campaignCombatExchange_0004 by the existing launcher transport.
 * No fake PSP, source arguments or alternative startup loop is introduced. */
void BtechInitializeCombatRuntime(void)
{
    resourceDiskPromptNumber=3;
    BtechInstallDosCriticalErrorHandler();
    combatContinuationStage=UINT16_MAX;
    BtechResetCombatState();
    BtechInitializeVideoAndInput();
    hudColourSchemeIndex=combatRendererKind==1;
    if (hudColourSchemeIndex || combatRendererKind==3) abort(); /* Non-EGA scratch ownership not converted. */
    desertPaletteSlot=BtechLoadPaletteSlot("desert.pal",NULL);
    arcticPaletteSlot=BtechLoadPaletteSlot("arctic.pal",NULL);
    tropicalPaletteSlot=BtechLoadPaletteSlot("tropic.pal",NULL);
    damagePaletteSlot=BtechLoadPaletteSlot("dmgpal.pal",NULL);
    BtechSelectFontSlot((int16_t)BtechLoadFontSlot("6x6b.fnt",NULL));
    BtechSelectPaletteSlot((int16_t)arcticPaletteSlot);
    promptBitmaps=BtechLoadBitmapList("prompt.bmp",NULL);
    BtechSelectAndInstallAudioBackends(0);
    BtechRegisterTimerCallback(BtechSelectTandyOrFallbackAudioCallback(),1);
    uint16_t inheritedSoundStreamIdentity;
    uint16_t soundBank=BtechLoadSoundBank("btechsnd.snd",NULL,&inheritedSoundStreamIdentity);
    if (!soundBank) BtechShowFatalErrorAndExit("Sound file not found...");
    else BtechSelectActiveSoundBank(soundBank);
    BtechPrepareCampaignCombatStage(inheritedSoundStreamIdentity);
    BtechInitializeUnitControlRecords();
    selectedObjectCount=0;
    for (uint16_t object=0;object<COMBAT_WORLD_OBJECTS;++object)
    {
        BtechWorldObjectPrefix *registered=worldObjects[object];
        if (!registered) break;
        if ((int16_t)registered->resourceId<(int16_t)surfaceResourceBank.resourceCount
            && (int16_t)registered->resourceId>=0)
        {
            if (selectedObjectCount>=COMBAT_SELECTED_WORLD_OBJECTS) abort(); /* Next native store aliases7A2E. */
            selectedWorldObjects[selectedObjectCount++]=worldObjects[object];
        }
    }
    playerCameraFillASuppressed=BtechRandomBelowBound(3)<1;
    BtechApplyLandscapeCameraColours();
    BtechLoadCockpitHudResources();
    BtechInitializeOrderPanelState();
    BtechResetMechInspectionState();
    if (!playerMechTemplate) abort();
    for (int16_t mount=0;mount<(int16_t)playerMechTemplate->weaponMountCount;++mount)
    {
        if (mount>=COMBAT_WEAPON_MOUNTS || !playerMechState) abort();
        playerMountFireFlags[mount]=(uint16_t)(int16_t)(int8_t)playerMechState->weapons[mount].operational;
    }
    activeUnitControl=&unitControls[0];
    BtechSynchronizeMechRenderParts();
    BtechShowCombatStageBriefing();
    BtechRefreshCockpitViewportAndGauges();
}

/*0810:22D8. Unknown first-argument characters leaveBP-2 uninitialized in the
 * original; there is deliberately no invented auto-detect default here. */
void BtechInitializeVideoAndInput(void)
{
    uint16_t adapterOverride=COMBAT_VIDEO_AUTO;
    if ((int16_t)savedArgumentCount>1)
    {
        uint8_t option=(uint8_t)savedArguments[1][0];
        /*EAA0 lowercases ASCII upper-case in the shipped character table. */
        if (option>='A' && option<='Z') option+='a'-'A';
        switch (option)
        {
        case 'c':adapterOverride=1;break;
        case 'e':adapterOverride=2;break;
        case 't':adapterOverride=3;break;
        default:abort(); /* Unrecovered native stack residue, not an argument. */
        }
    }
    if (!BtechSelectCombatRenderer((uint8_t)adapterOverride,COMBAT_VIDEO_ALLOWED))
        BtechShowFatalErrorAndExit(errorCardNotFound);
    /* The native failure call falls through here if it unexpectedly returns. */
    drawPolygonFilled=1;textTransparentBackground=0;egaBitmapEnabledPlanes=0x0F;
    BtechInstallKeyboardAndTickHandlers(0);
    BtechInstallDivideErrorHandler();
    BtechInstallTimerScheduler(COMBAT_SCHEDULER_PERIOD);
    BtechResetCameraContextsAndReferenceVector();
}

/*0810:DE52. Fatal is NOT1337: no delay, sound loop or critical-vector restore.
 *02C2 draws to the existing draw page; no invented flip/present is inserted. */
void BtechShowFatalErrorAndExit(const char *errorText)
{
    textForegroundColour=0x0F;textBackgroundColour=0;screenPolygonNeedsClip=0;
    BtechDrawNearText(errorText,COMBAT_FATAL_TEXT_X,COMBAT_FATAL_TEXT_Y);
    BtechRestoreTextVideoMode();
    BtechShutdownPlatformHandlers();
    BtechRuntimeExitAndCloseHandles(1);
}

/*0810:1337. Normal exit differs fromDE52's text-first fatal path. */
void BtechTerminateToLauncher(uint16_t quitWholeGame)
{
    BtechWaitScaledRuntimeDelay(COMBAT_EXIT_DELAY_UNITS);
    for (uint16_t soundEvent=0;soundEvent<COMBAT_SHUTDOWN_SOUND_EVENTS;++soundEvent) BtechStopSoundEventAndTimer(soundEvent);
    BtechRestoreDosCriticalErrorHandler();
    BtechShutdownPlatformHandlers();
    BtechRestoreTextVideoMode();
    BtechRuntimeExitAndCloseHandles(quitWholeGame<1);
}

/*0810:206D. Invalidate ONLY the native state and positioned-left-leg entries;
 * this is not memset(bindings).7752 and remaining part links stay untouched. */
void BtechResetCombatState(void)
{
    combatTickCounter=0;
    mountGroupMode=1;
    headingRadarSelected=0;
    combatDispatchState=COMBAT_SCREEN_COCKPIT;
    playerImpactThisTick=0;
    playerTargetSlot=0;
    selectedWeaponMount=0;
    aimReticleScreenY=0;
    headingRadarSelected=0;
    radarRangeShift=0x0A;
    viewScaleShift=7;
    firingIndicatorBank0Pending=0;
    firingIndicatorBank1Pending=0;
    requestedPlayerForwardStepScale=0;
    unidentifiedField21=0;
    selectedHitObject=NULL;
    sideCompletionFlags[1]=0;sideCompletionFlags[0]=0;
    for (uint16_t slot=0;slot<COMBAT_MECH_SLOTS;++slot) unitLastDrawParts[slot]=NULL;
    for (uint16_t chassis=0;chassis<COMBAT_MECH_TEMPLATES;++chassis) mechResourceBaseIds[chassis]=0;
    for (uint16_t slot=0;slot<COMBAT_MECH_SLOTS;++slot)
    {unitBindings[slot].mech=NULL;unitBindings[slot].positionedLeftLeg=NULL;}
    specialTargetInteractionBlocked=0;
    otherShapeResourceBase=0;firstMechShapeResourceId=0;
    playerSpecialTargetLandingLatch=0;
}

/*0810:CE9E. Flag is written before DOS queries/installs INT24. */
void BtechInstallDosCriticalErrorHandler(void)
{
    criticalErrorHandlerInstalled=1;
    if (!btechHostGetInterruptVector || !btechHostSetInterruptVector) abort();
    savedCriticalErrorVector=btechHostGetInterruptVector(0x24);
    btechHostSetInterruptVector(0x24,0x0810CEC9);
}

/*0810:CED5. No installed-flag test: restore the saved pair unconditionally. */
void BtechRestoreDosCriticalErrorHandler(void)
{
    criticalErrorHandlerInstalled=0;
    if (!btechHostSetInterruptVector) abort();
    btechHostSetInterruptVector(0x24,savedCriticalErrorVector);
}
