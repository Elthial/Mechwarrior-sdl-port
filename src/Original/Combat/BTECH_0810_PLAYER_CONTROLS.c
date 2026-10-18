#include "mechwarrior/combat_input.h"
#include "mechwarrior/combat_ballistics.h"
#include "mechwarrior/combat_projectiles.h"
#include "mechwarrior/combat_pose.h"
#include "mechwarrior/combat_cockpit.h"
#include "mechwarrior/combat_sound.h"
#include "mechwarrior/combat_render.h"
#include <stdlib.h>

enum { PLAYER_THROTTLE_STEP=4,PLAYER_GROUND_TURN_STEP=0x80,PLAYER_GROUND_TURN_LIMIT=0x280,
    PLAYER_JUMP_TURN_STEP=0xB6,PLAYER_JUMP_FORWARD_STEP=2,PLAYER_JUMP_CHARGE_THRESHOLD=500,
    PLAYER_JUMP_FORWARD_COST=4,PLAYER_JUMP_VERTICAL_COST=2,PLAYER_JUMP_FORWARD_HEAT=20,PLAYER_JUMP_VERTICAL_HEAT=12,
    PLAYER_AIM_STEP=0x140,PLAYER_AIM_LIMIT=0xB40,PLAYER_FIRST_COCKPIT_AIM_MARGIN=0x280,
    PLAYER_TORSO_STEP=0x400,PLAYER_TORSO_LIMIT=0x1C40,PLAYER_WEAPON_SOUND_PRIORITY=10,
    PLAYER_MACHINE_GUN_SOUND=0,PLAYER_LASER_SOUND=1,PLAYER_AC5_SOUND=2,PLAYER_MISSILE_SOUND=3,
    PLAYER_PPC_SOUND=4,PLAYER_JUMP_SOUND=14,PLAYER_JUMP_SOUND_PRIORITY=15,
    PLAYER_RECOVERY_ROLL_BOUND=12,PLAYER_RECOVERY_ROLL_THRESHOLD=5,
    PLAYER_ZOOM_MINIMUM=7,PLAYER_ZOOM_MAXIMUM=9,PLAYER_RADAR_RANGE_MINIMUM=7,PLAYER_RADAR_RANGE_MAXIMUM=10,
    PLAYER_HUD_COLOUR_MASK=0x0F,PLAYER_ENGINE_DISABLED_HITS=3,
    PLAYER_MECH_TARGET_INDEX_MASK=0x07,PLAYER_QUARTER_SIGN_EXTENSION=0xC000 };

/* Apply cockpit commands to the player's BattleMech, accounting for ammunition,
 * heat, jump charge and weapon selection before converging throttle and aim.
 *0810:099A. One original procedure: scan commands, shot accounting, movement,
 * then aim/torso/throttle convergence. SI/DI are preserved, not arguments.
 * Pause and normal target cycling are connected. Retreat/results and special
 * target aliases remain explicit gates, not successful stubs. */
void BtechProcessPlayerControls(void)
{
    if (!playerUnitBinding || !playerUnitBinding->torso || !playerUnitBinding->positionedLeftLeg
        || !playerMechState || !playerMechTemplate) abort();
    BtechWorldObjectPrefix *torso=&playerUnitBinding->torso->object.prefix;
    BtechPooledWorldObject *motion=playerUnitBinding->positionedLeftLeg;
    if (!combatTickCounter)
    {
        unidentifiedField24=previousFireButtonState=previousAimButtonState=0;
        requestedTorsoAngle=requestedAimPitch=fireCycleActive=0;
    }
    if (fireCycleActive && selectedWeaponMount==fireCycleStartMount) fireCycleActive=0;
    uint16_t altHeld=BtechReadHeldKeyState(COMBAT_KEY_ALT);
    int16_t scanCode=(int16_t)combatInputScanCode;
    switch (scanCode)
    {
    case COMBAT_KEY_P:
        BtechShowSimulationPausedDialog();break;
    case COMBAT_KEY_ESCAPE:
        if (!sideCompletionFlags[1] && !sideCompletionFlags[0] && BtechConfirmRetreatDialog())
            BtechExportCombatResultsAndTransition(COMBAT_RESULT_RETREAT);
        break;
    case COMBAT_KEY_TAB:
        fireCycleActive^=1;
        if (fireCycleActive) fireCycleStartMount=selectedWeaponMount;
        break;
    case COMBAT_KEY_W:
        mountGroupMode^=1;
        for (int16_t mount=0;mount<(int16_t)playerMechTemplate->weaponMountCount;++mount)
        {
            if ((uint16_t)mount>COMBAT_WEAPON_MOUNTS) abort();
            /* Native CBW, not Boolean normalization. Mount10 crosses into ammo
             * and the first reference-vector WORD; no eleventh weapon owner. */
            uint8_t operational=(uint16_t)mount==COMBAT_WEAPON_MOUNTS?(uint8_t)playerMechState->ammoSalvosRemaining[0]
                :playerMechState->weapons[mount].operational;
            if ((uint16_t)mount==COMBAT_WEAPON_MOUNTS) resourceReferenceVector[0]=(uint16_t)(int16_t)(int8_t)operational;
            else playerMountFireFlags[mount]=(uint16_t)(int16_t)(int8_t)operational;
        }
        break;
    case COMBAT_KEY_E:
        if (!cockpitFullRedrawPending && (playerTargetSlot&PLAYER_MECH_TARGET_INDEX_MASK) && !playerMechState->sensorHits)
            combatDispatchState=PLAYER_SCREEN_TARGET_INSPECTION;
        break;
    case COMBAT_KEY_R:
        if (headingRadarSelected)
        {
            --radarRangeShift;
            if ((int16_t)radarRangeShift<PLAYER_RADAR_RANGE_MINIMUM) radarRangeShift=PLAYER_RADAR_RANGE_MAXIMUM;
        }
        break;
    case COMBAT_KEY_T: headingRadarSelected=headingRadarSelected<1;break;
    case COMBAT_KEY_U:
        if (unitControls[0].movementMode==MOVEMENT_MODE_FALLEN_POSE
            && playerMechState->currentInternalStructure[COMBAT_BODY_LEFT_LEG]
            && playerMechState->currentInternalStructure[COMBAT_BODY_RIGHT_LEG]
            && playerMechState->actuatorCondition[COMBAT_ACTUATOR_LEFT_LEG] && playerMechState->actuatorCondition[COMBAT_ACTUATOR_RIGHT_LEG])
            unitControls[0].animationRequestFlags=(int16_t)BtechRandomBelowBound(PLAYER_RECOVERY_ROLL_BOUND)<PLAYER_RECOVERY_ROLL_THRESHOLD
                ?ANIMATION_REQUEST_RECOVER:ANIMATION_REQUEST_FAILED_RECOVERY;
        break;
    case COMBAT_KEY_O:
        if (selectedWeaponMount>COMBAT_WEAPON_MOUNTS) abort();
        if (selectedWeaponMount==COMBAT_WEAPON_MOUNTS) resourceReferenceVector[0]^=1;
        else playerMountFireFlags[selectedWeaponMount]^=1;
        break;
    case COMBAT_KEY_H:
    {
        if (combatLandscapeIndex>=COMBAT_LANDSCAPES) abort();
        BtechLandscapeCameraColourRow *colours=&landscapeCameraColourRows[0][combatLandscapeIndex];
        /* Native WORD increment/mask clears the adjacent0017 byte, too. */
        colours->hudReticleColour=(uint8_t)((colours->hudReticleColour+1)&PLAYER_HUD_COLOUR_MASK);
        colours->unidentifiedField11=0;
        colours->headingTickPaletteIndex=(colours->headingTickPaletteIndex+1)&PLAYER_HUD_COLOUR_MASK;
        break;
    }
    case COMBAT_KEY_J:
        if (altHeld) BtechCalibrateJoystickAxisPairs();
        else
        {
            if (playerMechState->templateIndex>=COMBAT_MECH_TEMPLATES) abort();
            if (mechTemplates[playerMechState->templateIndex].jumpJetGroupCount && !heatInhibitFlags[0]
                && unitControls[0].movementMode!=MOVEMENT_MODE_JUMP && (int16_t)playerMechState->jumpCharge>PLAYER_JUMP_CHARGE_THRESHOLD)
            {
                motion->object.prefix.orientationB=0;motion->object.prefix.orientationA=0;
                motion->orientationSteps[2]=0;motion->forwardStepScale=0;
                unitControls[0].animationRequestFlags|=ANIMATION_REQUEST_BEGIN_JUMP;
            }
        }
        break;
    case COMBAT_KEY_Z:
        ++viewScaleShift;
        if ((int16_t)viewScaleShift>PLAYER_ZOOM_MAXIMUM) viewScaleShift=PLAYER_ZOOM_MINIMUM;
        if (!playerPoseCameraContext) abort();
        playerPoseCameraContext->projectionShift=(uint8_t)viewScaleShift;
        break;
    case COMBAT_KEY_C:
        if (!cockpitFullRedrawPending) combatDispatchState=PLAYER_SCREEN_ORDERS;
        break;
    case COMBAT_KEY_KEYPAD_MINUS:
        selectedWeaponMount=BtechFindPreviousReadyPlayerWeaponMount(selectedWeaponMount,1);break;
    case COMBAT_KEY_KEYPAD_PLUS:
        selectedWeaponMount=BtechFindNextReadyPlayerWeaponMount(selectedWeaponMount,1);break;
    case COMBAT_KEY_ENTER:
        if ((int16_t)playerMechState->sensorHits<COMBAT_SENSORS_DESTROYED_HITS)
            playerTargetSlot=BtechCyclePlayerVisibleTarget(playerTargetSlot);
        break;
    case COMBAT_KEY_S: combatSoundEnabled=combatSoundEnabled<1;break;
    case COMBAT_KEY_D:
        if (!cockpitFullRedrawPending) combatDispatchState=PLAYER_SCREEN_INSPECTION;
        break;
    default:
        if (scanCode>=COMBAT_KEY_1 && scanCode<=COMBAT_KEY_0 && BtechIsPlayerWeaponMountReady(scanCode-COMBAT_KEY_1)) selectedWeaponMount=scanCode-COMBAT_KEY_1;
        break;
    }
    uint16_t directions=combatInputDirectionMask,aimButton=combatInputButton1State,fireButton=combatInputButton0State;
    if ((previousFireButtonState && !fireButton) || scanCode==COMBAT_KEY_SPACE || fireCycleActive)
    {
        if (selectedWeaponMount>=COMBAT_WEAPON_MOUNTS) abort(); /* Firing ammo alias is not an installed mount. */
        BtechWeaponState *weapon=&playerMechState->weapons[selectedWeaponMount];
        if (weapon->reloadTicksRemaining || heatInhibitFlags[0] || unitControls[0].combatValueOrBoundaryCode<=0 || !weapon->operational)
            fireCycleActive=0;
        else
        {
            int16_t kind=playerMechTemplate->weaponMounts[selectedWeaponMount].weaponKind;
            if ((uint16_t)kind>=COMBAT_WEAPON_KINDS) abort();
            BtechWeaponDefinition *definition=&weaponDefinitions[kind];
            int16_t ammoFamily=definition->ammoFamily;
            if ((uint16_t)ammoFamily!=COMBAT_WEAPON_NONE && (uint16_t)ammoFamily>=COMBAT_AMMO_FAMILIES) abort();
            /* Empty ammo leaves the cycling latch alone. All other gates above
             * clear it. Reload and heat are written BEFORE shot dispatch. */
            if ((uint16_t)ammoFamily==COMBAT_WEAPON_NONE || playerMechState->ammoSalvosRemaining[ammoFamily])
            {
                weapon->reloadTicksRemaining=definition->reloadTicks;
                playerMechState->heatUnits+=definition->heatUnits;
                uint16_t sound;
                if ((uint16_t)ammoFamily==COMBAT_WEAPON_NONE)
                {
                    BtechFirePlayerDirectWeapon(selectedWeaponMount);
                    sound=kind==COMBAT_WEAPON_PPC?PLAYER_PPC_SOUND:PLAYER_LASER_SOUND;
                }
                else
                {
                    --playerMechState->ammoSalvosRemaining[ammoFamily];
                    if (kind==COMBAT_WEAPON_MACHINE_GUN)
                    {
                        sound=PLAYER_MACHINE_GUN_SOUND;
                        BtechFirePlayerDirectWeapon(selectedWeaponMount);
                        /* Reload live globals after firing, as0B19 does. */
                        if (playerMechState->templateIndex>=COMBAT_MECH_TEMPLATES || selectedWeaponMount>=COMBAT_WEAPON_MOUNTS) abort();
                        uint16_t location=mechTemplates[playerMechState->templateIndex].weaponMounts[selectedWeaponMount].bodyLocation;
                        if (location==COMBAT_BODY_LEFT_ARM || location==COMBAT_BODY_LEFT_TORSO) firingIndicatorBank0Pending=1;
                        else firingIndicatorBank1Pending=1;
                    }
                    else
                    {
                        BtechFirePlayerWeaponProjectile(selectedWeaponMount);
                        sound=kind==COMBAT_WEAPON_AC5?PLAYER_AC5_SOUND:PLAYER_MISSILE_SOUND;
                    }
                }
                BtechPlaySoundIfEnabled(sound,PLAYER_WEAPON_SOUND_PRIORITY);
            }
        }
    }
    else if (fireButton && !aimButton)
    {
        if (directions==COMBAT_DIRECTION_UP) selectedWeaponMount=BtechFindPreviousReadyPlayerWeaponMount(selectedWeaponMount,0);
        else if (directions==COMBAT_DIRECTION_DOWN) selectedWeaponMount=BtechFindNextReadyPlayerWeaponMount(selectedWeaponMount,0);
    }
    previousFireButtonState=fireButton;
    if (!aimButton && !fireButton)
    {
        int16_t turn=motion->orientationSteps[2],forward=motion->forwardStepScale;
        if (unitControls[0].movementMode==MOVEMENT_MODE_JUMP)
        {
            if (!directions || heatInhibitFlags[0])
            {requestedPlayerForwardStepScale=(uint16_t)forward;goto dampTurn;}
            int16_t charge=(int16_t)playerMechState->jumpCharge,vertical=motion->positionSteps[2];
            int16_t turnSign=directions&COMBAT_DIRECTION_LEFT?1:-1;
            if ((directions&COMBAT_DIRECTION_UP) && charge>=PLAYER_JUMP_FORWARD_COST)
            {
                forward=(int16_t)(uint16_t)(forward+PLAYER_JUMP_FORWARD_STEP);
                vertical=(int16_t)(uint16_t)(vertical+playerMechTemplate->jumpAcceleration);
                playerMechState->heatUnits+=PLAYER_JUMP_FORWARD_HEAT;charge-=PLAYER_JUMP_FORWARD_COST;
                BtechPlaySoundIfEnabled(PLAYER_JUMP_SOUND,PLAYER_JUMP_SOUND_PRIORITY);
            }
            else if ((directions&COMBAT_DIRECTION_DOWN) && charge>=PLAYER_JUMP_VERTICAL_COST)
            {
                vertical=(int16_t)(uint16_t)(vertical+playerMechTemplate->jumpAcceleration);
                playerMechState->heatUnits+=PLAYER_JUMP_VERTICAL_HEAT;charge-=PLAYER_JUMP_VERTICAL_COST;
                turnSign=-turnSign;BtechPlaySoundIfEnabled(PLAYER_JUMP_SOUND,PLAYER_JUMP_SOUND_PRIORITY);
            }
            if (directions&(COMBAT_DIRECTION_LEFT|COMBAT_DIRECTION_RIGHT)) turn=(int16_t)(uint16_t)(turn+turnSign*PLAYER_JUMP_TURN_STEP);
            playerMechState->jumpCharge=(uint16_t)charge;
            motion->orientationSteps[2]=turn; /* No airborne turn clamp. */
            if (forward<(int16_t)playerMechTemplate->jumpSpeedLimit)
            {requestedPlayerForwardStepScale=(uint16_t)forward;motion->forwardStepScale=forward;}
            if (vertical<(int16_t)playerMechTemplate->jumpSpeedLimit) motion->positionSteps[2]=vertical;
            goto aiming;
        }
        if (!directions || unitControls[0].movementMode!=MOVEMENT_MODE_GROUNDED)
        {
dampTurn:
            /* Subtract the signed step without a crossing-zero clamp. */
            if (motion->orientationSteps[2]) motion->orientationSteps[2]=(int16_t)(uint16_t)
                (motion->orientationSteps[2]+(motion->orientationSteps[2]<0?PLAYER_GROUND_TURN_STEP:-PLAYER_GROUND_TURN_STEP));
            goto aiming;
        }
        if (playerMechState->templateIndex>=COMBAT_MECH_TEMPLATES) abort();
        int16_t limit=(int16_t)(uint16_t)(mechTemplates[playerMechState->templateIndex].maximumSpeedScale-heatSpeedPenalties[0]);
        int16_t actuator=(int16_t)playerMechState->actuatorCondition[COMBAT_ACTUATOR_LEFT_LEG];
        if (actuator>(int16_t)playerMechState->actuatorCondition[COMBAT_ACTUATOR_RIGHT_LEG]) actuator=(int16_t)playerMechState->actuatorCondition[COMBAT_ACTUATOR_RIGHT_LEG];
        uint16_t product=(uint16_t)((int32_t)limit*actuator),sign=(int16_t)product<0?UINT16_MAX:0;
        uint16_t magnitude=(uint16_t)((product^sign)-sign);
        /* SAR of signed8000 remains negative; use unsigned sign extension. */
        uint16_t quarter=(magnitude>>2)|((magnitude&NATIVE_WORD_SIGN_BIT)?PLAYER_QUARTER_SIGN_EXTENSION:0);
        limit=(int16_t)(uint16_t)((quarter^sign)-sign);
        if (directions&COMBAT_DIRECTION_DOWN)
        {
            uint16_t reverse=(uint16_t)(limit-(limit<0?UINT16_MAX:0));
            reverse=(reverse>>1)|((reverse&NATIVE_WORD_SIGN_BIT)?NATIVE_WORD_SIGN_BIT:0);
            int16_t reverseLimit=(int16_t)(uint16_t)(0-reverse),requested=(int16_t)(uint16_t)(forward-PLAYER_THROTTLE_STEP);
            requestedPlayerForwardStepScale=(uint16_t)(requested<reverseLimit?reverseLimit:requested);
            if (directions&COMBAT_DIRECTION_LEFT) turn=(int16_t)(uint16_t)(turn-PLAYER_GROUND_TURN_STEP);
            else if (directions&COMBAT_DIRECTION_RIGHT) turn=(int16_t)(uint16_t)(turn+PLAYER_GROUND_TURN_STEP);
        }
        else
        {
            if (directions&COMBAT_DIRECTION_UP)
            {
                int16_t requested=(int16_t)(uint16_t)(forward+PLAYER_THROTTLE_STEP);
                requestedPlayerForwardStepScale=(uint16_t)(requested>limit?limit:requested);
            }
            if (directions&COMBAT_DIRECTION_LEFT) turn=(int16_t)(uint16_t)(turn+PLAYER_GROUND_TURN_STEP);
            else if (directions&COMBAT_DIRECTION_RIGHT) turn=(int16_t)(uint16_t)(turn-PLAYER_GROUND_TURN_STEP);
        }
        if (!(directions&(COMBAT_DIRECTION_LEFT|COMBAT_DIRECTION_RIGHT))) motion->orientationSteps[2]=0;
        else
        {
            int16_t turnMagnitude=(uint16_t)turn==NATIVE_WORD_SIGN_BIT?INT16_MAX:turn<0?-turn:turn;
            if (turnMagnitude<PLAYER_GROUND_TURN_LIMIT) motion->orientationSteps[2]=turn;
        }
    }
aiming:;
    int16_t torsoHeading=(int16_t)torso->orientationC,aimPitch=(int16_t)playerMechState->aimPitch;
    if (!aimButton || !directions) requestedAimPitch=(uint16_t)aimPitch;
    else
    {
        unidentifiedField24=0;
        if (directions&COMBAT_DIRECTION_LEFT)
        {
            int16_t requested=(int16_t)(uint16_t)(torsoHeading+PLAYER_TORSO_STEP);
            requestedTorsoAngle=(uint16_t)(requested>PLAYER_TORSO_LIMIT?PLAYER_TORSO_LIMIT:requested);
        }
        else if (directions&COMBAT_DIRECTION_RIGHT)
        {
            int16_t requested=(int16_t)(uint16_t)(torsoHeading-PLAYER_TORSO_STEP);
            requestedTorsoAngle=(uint16_t)(requested< -PLAYER_TORSO_LIMIT?-PLAYER_TORSO_LIMIT:requested);
        }
        if (directions&COMBAT_DIRECTION_DOWN)
        {
            int16_t requested=(int16_t)(uint16_t)(aimPitch+PLAYER_AIM_STEP);
            requestedAimPitch=(uint16_t)(requested>PLAYER_AIM_LIMIT?PLAYER_AIM_LIMIT:requested);
        }
        else if (directions&COMBAT_DIRECTION_UP)
        {
            int16_t minimum=(activeCockpitLayoutIndex<1?PLAYER_FIRST_COCKPIT_AIM_MARGIN:0)-PLAYER_AIM_LIMIT;
            int16_t requested=(int16_t)(uint16_t)(aimPitch-PLAYER_AIM_STEP);
            requestedAimPitch=(uint16_t)(requested<minimum?minimum:requested);
        }
    }
    if (scanCode==COMBAT_KEY_KEYPAD_STOP) {motion->orientationSteps[2]=0;requestedPlayerForwardStepScale=0;}
    previousAimButtonState=aimButton;
    if (scanCode==COMBAT_KEY_A) requestedTorsoAngle=0;
    if (playerMechState->engineHits==PLAYER_ENGINE_DISABLED_HITS || heatInhibitFlags[0] || playerMechState->gyroHits==COMBAT_GYRO_DESTROYED_HITS)
    {requestedPlayerForwardStepScale=0;motion->orientationSteps[2]=0;}
    else
    {
        /* Signed comparisons, WORD differences and8000 special case are native.
         * These are three inline convergence blocks, not invented game helpers. */
        aimPitch=(int16_t)playerMechState->aimPitch;
        torsoHeading=(int16_t)torso->orientationC;
        int16_t target=(int16_t)requestedAimPitch;
        int16_t step=(uint16_t)(aimPitch-target)==NATIVE_WORD_SIGN_BIT?INT16_MAX:(int16_t)(uint16_t)(aimPitch<target?target-aimPitch:aimPitch-target);
        if (step>PLAYER_AIM_STEP) step=PLAYER_AIM_STEP;
        playerMechState->aimPitch=(uint16_t)(aimPitch+(aimPitch>target?-step:step));
        target=(int16_t)requestedTorsoAngle;
        step=(uint16_t)(torsoHeading-target)==NATIVE_WORD_SIGN_BIT?INT16_MAX:(int16_t)(uint16_t)(torsoHeading<target?target-torsoHeading:torsoHeading-target);
        if (step>PLAYER_TORSO_STEP) step=PLAYER_TORSO_STEP;
        torso->orientationC=(uint16_t)(torsoHeading+(torsoHeading>target?-step:step));
    }
    int16_t forward=motion->forwardStepScale,target=(int16_t)requestedPlayerForwardStepScale;
    int16_t step=(uint16_t)(forward-target)==NATIVE_WORD_SIGN_BIT?INT16_MAX:(int16_t)(uint16_t)(forward<target?target-forward:forward-target);
    if (step>PLAYER_THROTTLE_STEP) step=PLAYER_THROTTLE_STEP;
    motion->forwardStepScale=(int16_t)(uint16_t)(forward+(forward>target?-step:step));
    if (!playerMechState->currentInternalStructure[COMBAT_BODY_LEFT_LEG] || !playerMechState->currentInternalStructure[COMBAT_BODY_RIGHT_LEG])
        unitControls[0].animationRequestFlags|=ANIMATION_REQUEST_FALL_POSE;
}
