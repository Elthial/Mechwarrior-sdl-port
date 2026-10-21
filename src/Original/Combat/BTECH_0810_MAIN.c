#include "mechwarrior/port_diagnostics.h"
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

/* Local loop sentinels, not additional campaign result values. 0019 uses FFFF
 * to defer result export or skip the countdown before the final INC wraps to 0. */
enum { COMBAT_RESULT_UNRESOLVED=0xFFFF,COMBAT_TICK_RESTART_SENTINEL=0xFFFF };

/* Run the combat mission: update MechWarriors, BattleMechs, weapons and effects,
 * draw the active cockpit/tactical screen, then resolve the contract outcome.
 *0810:0019. Preserve the connected EGA mission loop and call/write order;
 * remaining foreign-memory aliases are explicit limits, not replacement AI.
 * Typed argv transports the original near vector; no host ABI is seeded. */
void BtechProgramMain(uint16_t argumentCount,const char *const *arguments)
{
    uint8_t outcomeCountdownActive=0;
    uint16_t outcomeCountdownStartTick; /* Assigned before every first active-countdown use. */
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
            if (!playerUnitBinding || !playerUnitBinding->torso) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !playerUnitBinding || !playerUnitBinding->torso");
            BtechUpdatePlayerMechPose(&playerUnitBinding->torso->object.prefix);
            BtechRemoveBodyHeadingFromMechParts();
            if (combatDispatchState==PLAYER_SCREEN_ORDERS) BtechRefreshLanceOrderScreen();
            else if (combatDispatchState==PLAYER_SCREEN_INSPECTION) BtechUpdateMechInspectionScreen(0);
            else if (combatDispatchState==PLAYER_SCREEN_TARGET_INSPECTION) BtechUpdateMechInspectionScreen(playerTargetSlot);
        }
        BtechUnregisterTransientWorldObjects();
        BtechDampJumpMotion();
        int16_t playerConditionOrExitCode=unitControls[0].conditionScoreOrExitCode;
        if (playerConditionOrExitCode<COMBAT_CONDITION_SCORE_MINIMUM)
        {
            uint16_t missionResult=COMBAT_RESULT_UNRESOLVED;
            if (playerConditionOrExitCode<COMBAT_MECH_DISABLED_VALUE)
            {
                uint16_t exitBoundaryIndex=-((int32_t)playerConditionOrExitCode+COMBAT_EXIT_CODE_OFFSET);
                missionResult=COMBAT_RESULT_RETREAT;
                if (sideObjectiveRules[0]==OBJECTIVE_WITHDRAW)
                {
                    /* Use the same four-edge decoding as the withdrawal
                     * objective check. Other game state is not an exit mask. */
                    if (exitBoundaryIndex<TACTICAL_BOUNDARY_CODES
                        && (scenarioSelectors.sideBoundaryMasks[0]&boundaryMaskByCode[exitBoundaryIndex]))
                        missionResult=COMBAT_RESULT_SUCCESS;
                }
            }
            else
            {
                if (!playerMechState) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !playerMechState");
                if (playerMechState->lifeSupportHits==COMBAT_LIFE_SUPPORT_HIT_LIMIT || playerMechState->cockpitDisabled)
                {BtechFlashPlayerMechLoss();missionResult=COMBAT_RESULT_FAILURE;}
            }
            if (missionResult!=COMBAT_RESULT_UNRESOLVED)
            {
                BtechExportCombatResultsAndTransition(missionResult);
                combatTickCounter=COMBAT_TICK_RESTART_SENTINEL;outcomeCountdownActive=0;
            }
        }
        if (combatTickCounter!=COMBAT_TICK_RESTART_SENTINEL && (sideCompletionFlags[0] || sideCompletionFlags[1]))
        {
            if (!outcomeCountdownActive) {outcomeCountdownStartTick=combatTickCounter;outcomeCountdownActive=1;}
            uint16_t outcomeTicksRemaining=outcomeCountdownStartTick-combatTickCounter+COMBAT_OUTCOME_DELAY_TICKS;
            if (!outcomeTicksRemaining || (combatDispatchState==COMBAT_SCREEN_COCKPIT && combatInputScanCode==COMBAT_KEY_ESCAPE))
            {
                uint16_t missionResult=COMBAT_RESULT_FAILURE-(int16_t)(int8_t)sideCompletionFlags[0];
                BtechExportCombatResultsAndTransition(missionResult);
                outcomeCountdownActive=0;combatTickCounter=COMBAT_TICK_RESTART_SENTINEL;
            }
        }
        playerImpactThisTick=0;
        ++combatTickCounter;
    }
}

/*0810:01C0. Twenty native flashes, not a redraw/damage implementation. */
void BtechFlashPlayerMechLoss(void)
{
    screenPolygonNeedsClip=false;drawPolygonFilled=true;
    BtechPlaySoundIfEnabled(COMBAT_LOSS_SOUND,COMBAT_LOSS_SOUND_PRIORITY);
    for (uint16_t flash=0;flash<COMBAT_LOSS_FLASHES;++flash)
    {
        uint16_t colour=(flash&1)?COMBAT_LOSS_COLOUR_B:COMBAT_LOSS_COLOUR_A;
        if (combatDispatchState==COMBAT_SCREEN_COCKPIT)
        {
            drawFillColour=(uint8_t)colour;drawLineColour=(uint8_t)colour;
            if (!playerPoseCameraContext) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !playerPoseCameraContext");
            BtechCameraContext *camera=playerPoseCameraContext;
            BtechDrawStyledRectangle((uint16_t)camera->clipMinimumA,(uint16_t)camera->clipMinimumB,
                (uint16_t)(camera->clipMaximumA-camera->clipMinimumA),(uint16_t)(camera->clipMaximumB-camera->clipMinimumB));
            BtechDrawCockpitBitmapPlacements();
        }
        else BtechSetOverscanColour(colour);
        BtechWaitScaledRuntimeDelay(COMBAT_LOSS_FLASH_DELAY);
    }
}
