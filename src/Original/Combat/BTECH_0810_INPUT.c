#include "mechwarrior/combat_input.h"
#include "mechwarrior/combat_runtime.h"
#include <stdlib.h>

/*0810:0010. FAR termination callback reached by keyboard Ctrl-Escape. */
void BtechRequestTermination(void)
{
    terminationFlag=1;
}

/*0810:026B. Keep native release phase, then press phase and their original
 * short-circuit order. Timer/key interrupts arrive through hardware bindings. */
void BtechWaitForCombatInputReleaseThenPress(void)
{
    while (combatInputScanCode || BtechReadJoystickPresenceState(JOYSTICK_A_BUTTON_ONE) || BtechReadJoystickPresenceState(JOYSTICK_A_BUTTON_TWO))
        BtechPollCombatCommandInput();
    while (!combatInputScanCode && !BtechReadJoystickPresenceState(JOYSTICK_A_BUTTON_ONE) && !BtechReadJoystickPresenceState(JOYSTICK_A_BUTTON_TWO))
    {
        if (terminationFlag) BtechTerminateToLauncher(1);
        BtechPollCombatCommandInput();
    }
}

/*0810:2A17. Native SI/DI are preserved machine registers, not arguments or
 * source outputs. Publish the four WORDs only after the complete poll. */
void BtechPollCombatCommandInput(void)
{
    uint16_t scanCode=BtechPopBiosKeyboardBuffer()>>BIOS_KEY_SCAN_SHIFT;
    uint16_t direction=BtechReadJoystickDirectionMask(JOYSTICK_A);
    if (!direction)
    {
        direction=BtechReadHeldDirectionMask(0);
        if (!direction)
            switch (scanCode)
            {
            case COMBAT_KEY_HOME:direction=COMBAT_DIRECTION_UP|COMBAT_DIRECTION_LEFT;break;
            case COMBAT_KEY_UP:direction=COMBAT_DIRECTION_UP;break;
            case COMBAT_KEY_PAGE_UP:direction=COMBAT_DIRECTION_UP|COMBAT_DIRECTION_RIGHT;break;
            case COMBAT_KEY_LEFT:direction=COMBAT_DIRECTION_LEFT;break;
            case COMBAT_KEY_RIGHT:direction=COMBAT_DIRECTION_RIGHT;break;
            case COMBAT_KEY_END:direction=COMBAT_DIRECTION_DOWN|COMBAT_DIRECTION_LEFT;break;
            case COMBAT_KEY_DOWN:direction=COMBAT_DIRECTION_DOWN;break;
            case COMBAT_KEY_PAGE_DOWN:direction=COMBAT_DIRECTION_DOWN|COMBAT_DIRECTION_RIGHT;break;
            }
    }
    uint16_t button1=BtechReadJoystickPresenceState(JOYSTICK_A_BUTTON_TWO);
    uint16_t button0=BtechReadJoystickPresenceState(JOYSTICK_A_BUTTON_ONE);
    switch (scanCode)
    {
    case COMBAT_KEY_N:direction=COMBAT_DIRECTION_DOWN;button1=1;break;
    case COMBAT_KEY_M:direction=COMBAT_DIRECTION_UP;button1=1;break;
    case COMBAT_KEY_COMMA:direction=COMBAT_DIRECTION_LEFT;button1=1;break;
    case COMBAT_KEY_PERIOD:direction=COMBAT_DIRECTION_RIGHT;button1=1;break;
    }
    if (combatInputReplayMode==COMBAT_INPUT_PLAYBACK)
    {
        if (scanCode|button1|button0|combatInputReplayCancelLatched)
        {scanCode=COMBAT_KEY_ESCAPE;combatInputReplayCancelLatched=1;}
        else abort(); /* Native replay IO/short-read stack residue is not converted. */
    }
    else if (combatInputReplayMode==COMBAT_INPUT_RECORD) abort(); /* Native append-stream writer dependency. */
    combatInputScanCode=scanCode;
    combatInputDirectionMask=direction;
    combatInputButton1State=button1;
    combatInputButton0State=button0;
}
