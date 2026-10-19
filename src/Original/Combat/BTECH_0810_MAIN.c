#include "mechwarrior/combat_startup.h"
#include "mechwarrior/combat_input.h"
#include "mechwarrior/combat_runtime.h"
#include "mechwarrior/combat_pose.h"
#include "mechwarrior/combat_movement.h"
#include "mechwarrior/combat_tactics.h"
#include "mechwarrior/combat_projectiles.h"
#include "mechwarrior/combat_effects.h"
#include "mechwarrior/combat_ballistics.h"
#include "mechwarrior/combat_cockpit.h"
#include "mechwarrior/combat_render.h"
#include "mechwarrior/combat_orders.h"
#include <stdlib.h>

/* Run the combat mission: update MechWarriors, BattleMechs, weapons and effects,
 * draw the active cockpit/tactical screen, then resolve the contract outcome.
 *0810:0019. Preserve the connected EGA mission loop and call/write order;
 * remaining foreign-memory aliases are explicit limits, not replacement AI.
 * Typed argv transports the original near vector; no host ABI is seeded. */
void BtechProgramMain(uint16_t argumentCount,const char *const *arguments)
{
    uint8_t outcomeCountdownActive=0;
    uint16_t outcomeStartTick; /* Assigned before every first active-countdown use. */
    savedArgumentCount=argumentCount;savedArguments=arguments;
    BtechInitializeCombatRuntime();
    for (;;)
    {
        /* Host CPU accounting for the completed native loop, not a game
         * wait or replacement simulation clock. Unbound in pure C fixtures. */
        if (btechHostPaceCombatCpu) btechHostPaceCombatCpu();
        if (terminationFlag) BtechTerminateToLauncher(1);
        runtimeDelayRemaining=COMBAT_FRAME_DELAY_TICKS;
        BtechPollCombatCommandInput();
        BtechRefreshOpposingUnitVisibility();
        BtechRefreshUnitDistanceMatrix();
        BtechAdvanceWeaponReloadTimers();
        BtechRechargeJumpCharge();
        BtechUpdateUnitHeatEffects();
        BtechStopSoundEventAndTimer(0);
        if (combatDispatchState==COMBAT_SCREEN_COCKPIT) BtechProcessPlayerControls();
        BtechUpdatePlayerMechMotion();
        if (!aiUpdatesSuppressed) BtechUpdateAiMechUnits();
        BtechUpdateProjectiles();
        BtechAdvanceEffectResourceSequences();
        BtechUpdateBodyDestructionEffects();
        if (mountGroupMode) BtechAdvancePlayerGroupedWeaponSelection();
        if (combatDispatchState==COMBAT_SCREEN_COCKPIT) BtechRenderCockpitSceneAndHud();
        else
        {
            BtechApplyBodyHeadingToMechParts();
            if (!playerUnitBinding || !playerUnitBinding->torso) abort();
            BtechUpdatePlayerMechPose(&playerUnitBinding->torso->object.prefix);
            BtechRemoveBodyHeadingFromMechParts();
            if (combatDispatchState==PLAYER_SCREEN_ORDERS) BtechRefreshLanceOrderScreen();
            else if (combatDispatchState==PLAYER_SCREEN_INSPECTION) BtechUpdateMechInspectionScreen(0);
            else if (combatDispatchState==PLAYER_SCREEN_TARGET_INSPECTION) BtechUpdateMechInspectionScreen(playerTargetSlot);
        }
        BtechUnregisterTransientWorldObjects();
        BtechDampJumpMotion();
        int16_t playerValue=unitControls[0].combatValueOrBoundaryCode;
        if (playerValue<0)
        {
            uint16_t result=UINT16_MAX;
            if (playerValue<-1)
            {
                uint8_t boundaryIndex=(uint8_t)(2-(uint8_t)playerValue);
                if (sideObjectiveRules[0]==OBJECTIVE_WITHDRAW)
                {
                    uint8_t mask;
                    if (boundaryIndex<TACTICAL_BOUNDARY_CODES) mask=boundaryMaskByCode[boundaryIndex];
                    else if (boundaryIndex==4) mask=aiUpdatesSuppressed;
                    else if (boundaryIndex==5) mask=unidentifiedField22;
                    else if (boundaryIndex==6) mask=(uint8_t)bodyLocationReferenceIndices[0];
                    else if (boundaryIndex==7) mask=(uint8_t)(bodyLocationReferenceIndices[0]>>8);
                    else abort(); /* Other native adjacent-owner boundary aliases. */
                    result=(scenarioSelectors.sideBoundaryMasks[0]&mask)?0:2;
                }
                else result=2;
            }
            else
            {
                if (!playerMechState) abort();
                if (playerMechState->lifeSupportHits==2 || playerMechState->cockpitDisabled)
                {BtechFlashPlayerMechLoss();result=1;}
            }
            if (result!=UINT16_MAX)
            {
                BtechExportCombatResultsAndTransition(result);
                combatTickCounter=UINT16_MAX;outcomeCountdownActive=0;
            }
        }
        if (combatTickCounter!=UINT16_MAX && (sideCompletionFlags[0] || sideCompletionFlags[1]))
        {
            if (!outcomeCountdownActive) {outcomeStartTick=combatTickCounter;outcomeCountdownActive=1;}
            uint16_t remaining=(uint16_t)(outcomeStartTick-combatTickCounter+COMBAT_OUTCOME_DELAY_TICKS);
            if (!remaining || (combatDispatchState==COMBAT_SCREEN_COCKPIT && combatInputScanCode==COMBAT_KEY_ESCAPE))
            {
                uint16_t result=(uint16_t)(1-(int16_t)(int8_t)sideCompletionFlags[0]);
                BtechExportCombatResultsAndTransition(result);
                outcomeCountdownActive=0;combatTickCounter=UINT16_MAX;
            }
        }
        playerImpactThisTick=0;
        ++combatTickCounter;
    }
}

/*0810:01C0. Twenty native flashes, not a redraw/damage implementation. */
void BtechFlashPlayerMechLoss(void)
{
    screenPolygonNeedsClip=0;drawPolygonFilled=1;
    BtechPlaySoundIfEnabled(COMBAT_LOSS_SOUND,COMBAT_LOSS_SOUND_PRIORITY);
    for (uint16_t flash=0;flash<COMBAT_LOSS_FLASHES;++flash)
    {
        uint16_t colour=(flash&1)?COMBAT_LOSS_COLOUR_B:COMBAT_LOSS_COLOUR_A;
        if (combatDispatchState==COMBAT_SCREEN_COCKPIT)
        {
            drawFillColour=(uint8_t)colour;drawLineColour=(uint8_t)colour;
            if (!playerPoseCameraContext) abort();
            BtechCameraContext *camera=playerPoseCameraContext;
            BtechDrawStyledRectangle((uint16_t)camera->clipMinimumA,(uint16_t)camera->clipMinimumB,
                (uint16_t)(camera->clipMaximumA-camera->clipMinimumA),(uint16_t)(camera->clipMaximumB-camera->clipMinimumB));
            BtechDrawCockpitBitmapPlacements();
        }
        else BtechSetOverscanColour(colour);
        BtechWaitScaledRuntimeDelay(COMBAT_LOSS_FLASH_DELAY);
    }
}
