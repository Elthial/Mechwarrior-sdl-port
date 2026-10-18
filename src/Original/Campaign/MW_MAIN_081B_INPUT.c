#include "mechwarrior/campaign_continuations.h"
#include "mechwarrior/campaign_input.h"
#include "mechwarrior/campaign_startup.h"
#include "mechwarrior/ega_campaign.h"
#include <stdlib.h>
#include <string.h>

/* Host-installed device entry points, not game input decisions. */
MwCampaignInputCalls mwCampaignInput;

/* MW_MAIN081B:0E10..0E8E. INT33 release, then relative motion; X wins ties. */
bool MwPollMouseNavigationKey(uint16_t *navigationKey)
{
    if (mwCampaignInput.mouseReleaseCount==NULL) abort();
    if (mwCampaignInput.mouseReleaseCount(MOUSE_BUTTON_LEFT)!=0)
    { *navigationKey=CAMPAIGN_KEY_ENTER;return true; }
    if (mwCampaignInput.readMouseMotion==NULL) abort();
    uint16_t motionX,motionY;
    mwCampaignInput.readMouseMotion(&motionX,&motionY);
    mouseMotionX+=motionX;mouseMotionY+=motionY;
    if ((int16_t)mouseMotionX>MOUSE_NAVIGATION_THRESHOLD) *navigationKey=CAMPAIGN_KEY_RIGHT;
    else if ((int16_t)mouseMotionX<-MOUSE_NAVIGATION_THRESHOLD) *navigationKey=CAMPAIGN_KEY_LEFT;
    else if ((int16_t)mouseMotionY>MOUSE_NAVIGATION_THRESHOLD) *navigationKey=CAMPAIGN_KEY_DOWN;
    else if ((int16_t)mouseMotionY<-MOUSE_NAVIGATION_THRESHOLD) *navigationKey=CAMPAIGN_KEY_UP;
    else { *navigationKey=mouseMotionY;return false; } /* Native AX residue. */
    mouseMotionX=0;mouseMotionY=0;
    return true;
}
/* MW_MAIN081B:0DF7..0E0F and shared0E8F..0EE6 joystick tail. CF/AX projection. */
bool MwPollNavigationKeyFromMouseOrJoystick(uint16_t *navigationKey)
{
    if (mouseInstalled!=0 && MwPollMouseNavigationKey(navigationKey)) return true;
    if (joystickInstalled==0) return false;
    MwPollJoystickA();
    if ((joystickButtonOnePressed|joystickButtonTwoPressed)!=0)
    { *navigationKey=CAMPAIGN_KEY_ENTER;return true; }
    if (joystickDirection==JOYSTICK_CENTRED)
    { joystickRepeatActive=0;return false; }
    if (joystickDeflection==JOYSTICK_DEFLECTION_NEAR) return false;
    if (joystickRepeatActive!=0 && joystickRepeatDeadline>campaignTimerTicks) return false;
    joystickRepeatDeadline=campaignTimerTicks+JOYSTICK_REPEAT_TICKS;
    joystickRepeatActive=1;
    uint16_t byteOffset=(uint16_t)(joystickDirection*2);
    if (byteOffset>=sizeof(joystickNavigationKeys)) abort(); /* Unmapped adjacent input storage. */
    *navigationKey=joystickNavigationKeys[byteOffset/2];
    return *navigationKey!=0;
}
/* MW_MAIN081B:03F7..04AC. Original discharge sampling, not a second04AD call:
 * calibration must not reset direction, button edge flags or live poll counts. */
void MwCalibrateJoystickA(void)
{
    if (!mwCampaignInput.setInterruptsEnabled || !mwCampaignInput.writeGamePort || !mwCampaignInput.readGamePort) abort();
    uint16_t tick;
    do
    {
        mwCampaignInput.setInterruptsEnabled(false);
        tick=campaignTimerTicks;
        mwCampaignInput.writeGamePort((uint8_t)tick);
        uint16_t count=0;
        uint8_t port;
        for (;;)
        {
            ++count;port=mwCampaignInput.readGamePort();
            if (!(port&GAMEPORT_X_CHARGED))
            {
                joystickCentreX=count;
                do { ++count;port=mwCampaignInput.readGamePort(); }
                while ((port&GAMEPORT_Y_CHARGED) && !(port&GAMEPORT_X_CHARGED));
                joystickCentreY=count;break;
            }
            if (!(port&GAMEPORT_Y_CHARGED))
            {
                joystickCentreY=count;
                do { ++count;port=mwCampaignInput.readGamePort(); }
                while ((port&GAMEPORT_X_CHARGED) && !(port&GAMEPORT_Y_CHARGED));
                joystickCentreX=count;break;
            }
        }
        mwCampaignInput.setInterruptsEnabled(true);
    } while (tick!=campaignTimerTicks);
    joystickXDeflectionStep=(uint16_t)(joystickCentreX*2)/JOYSTICK_CALIBRATION_DIVISOR;
    joystickXLow=joystickCentreX-(joystickXDeflectionStep>>1);
    joystickXFarLow=joystickXLow-joystickXDeflectionStep;
    joystickXHigh=joystickCentreX+(joystickXDeflectionStep>>1);
    joystickXFarHigh=joystickXHigh+joystickXDeflectionStep;
    joystickYDeflectionStep=(uint16_t)(joystickCentreY*2)/JOYSTICK_CALIBRATION_DIVISOR;
    joystickYLow=joystickCentreY-(joystickYDeflectionStep>>1);
    joystickYFarLow=joystickYLow-joystickYDeflectionStep;
    joystickYHigh=joystickCentreY+(joystickYDeflectionStep>>1);
    joystickYFarHigh=joystickYHigh+joystickYDeflectionStep;
}

/* MW_MAIN081B:0273..02A3. Host delivery replaces only STI/save/IRET plumbing;
 * all game-owned BYTE/WORD increments and inhibition are retained. */
void MwCampaignTimerTick(void)
{
    if (campaignTimerInhibited != 0) return;
    campaignTimerByte++;
    campaignTimerTicks++;
    campaignTimerPhaseTicks++;
    if (campaignTimerPhaseTicks < CAMPAIGN_TIMER_PHASE_PERIOD) return;
    campaignTimerPhaseTicks = 0;
    campaignTimerAlternatingPhase ^= 1;
}

/* MW_MAIN081B:02F6..02FA falls through02FB: this WAITS for an event despite
 * the historical "DiscardPendingInput" name. Do not replace it with a drain. */
void MwWaitForInputAcknowledgment(void)
{ inputSinglePoll = 0; (void)MwPollInputUntilIdleOrEvent(); }

/* MW_MAIN081B:02FB..034C. A mouse release or new joystick press returns before
 * RNG/keyboard. Otherwise every pass consumes a real0C17 draw, even if idle. */
bool MwPollInputUntilIdleOrEvent(void)
{
    do
    {
        if (mouseInstalled != 0)
        {
            if (mwCampaignInput.mouseReleaseCount == NULL) abort();
            if (mwCampaignInput.mouseReleaseCount(MOUSE_BUTTON_LEFT) != 0) return true;
            if (mwCampaignInput.mouseReleaseCount(MOUSE_BUTTON_RIGHT) != 0) return true;
        }
        if (joystickInstalled != 0)
        {
            MwPollJoystickA();
            if ((joystickButtonOnePressed | joystickButtonTwoPressed) != 0) return true;
        }
        (void)MwRandomUpTo(UINT8_MAX);
        if (mwCampaignInput.keyboardAvailable == NULL) abort();
        if (mwCampaignInput.keyboardAvailable())
        {
            if (mwCampaignInput.readKeyboard == NULL) abort();
            (void)mwCampaignInput.readKeyboard();
            return true;
        }
    } while (inputSinglePoll == 0);
    return false;
}
/* MW_MAIN081B:034D..0353 JMP02FB, no nested native CALL. */
bool MwPollInputOnce(void)
{ inputSinglePoll = 1; return MwPollInputUntilIdleOrEvent(); }

/* MW_MAIN081B:070C..0724. */
void MwEraseTextInputCursor(void)
{
    uint16_t cursorY=textInputCursorY;
    (void)EgaDrawCharacter(textInputCursorX,&cursorY,
        ' ',textInputForeground,textInputBackground);
}

/* MW_MAIN081B:064E..075D. The second lowercase test is retained even though
 * the first subtraction makes it unreachable for ordinary ASCII letters. */
uint16_t MwReadCampaignTextInput(uint16_t x,uint16_t y,uint16_t capacity,
    uint8_t foreground,uint8_t background)
{
    textInputCursorX=x;textInputCursorY=y;textInputMaximumIndex=(uint16_t)(capacity-1);
    textInputForeground=foreground;textInputBackground=background;
    memset(campaignTextInputBuffer,0,sizeof(campaignTextInputBuffer));textInputLength=0;
    for (;;)
    {
        if (!mwCampaignInput.keyboardAvailable || !mwCampaignInput.readKeyboard) abort();
        if (!mwCampaignInput.keyboardAvailable())
        {
            if (campaignTimerAlternatingPhase!=textInputLastPhase)
            {
                textInputLastPhase=campaignTimerAlternatingPhase;
                uint16_t cursorY=textInputCursorY;
                (void)EgaDrawCharacter(textInputCursorX,&cursorY,
                    campaignTimerAlternatingPhase?'*':' ',textInputForeground,textInputBackground);
            }
            if (!mwCampaignInput.waitForInterrupt) abort();
            mwCampaignInput.waitForInterrupt();continue;
        }
        uint16_t key=mwCampaignInput.readKeyboard();
        if (key==CAMPAIGN_KEY_ENTER)
        {
            if (mouseInstalled!=0) {if (!mwCampaignInput.resetMouse) abort();mwCampaignInput.resetMouse();}
            MwEraseTextInputCursor();return 0;
        }
        if (key==CAMPAIGN_KEY_BACKSPACE)
        {
            if (textInputLength==0) continue;
            campaignTextInputBuffer[--textInputLength]=0;MwEraseTextInputCursor();
            textInputCursorX=(uint16_t)(textInputCursorX-selectedGlyphAdvance);continue;
        }
        if (key==CAMPAIGN_KEY_ESCAPE)
        {
            if (!mwCampaignInput.readKeyboardShiftFlags) abort();
            if (mwCampaignInput.readKeyboardShiftFlags()&BIOS_SHIFT_CONTROL) return CAMPAIGN_CONTINUE_CLEANUP;
            continue;
        }
        uint8_t character=(uint8_t)key;
        if (character<' ' || character>'z') continue;
        if (character>='a') character=(uint8_t)(character-ASCII_CASE_DISTANCE);
        if (textInputLength==textInputMaximumIndex) continue;
        if (character>='a') character=(uint8_t)(character-CAMPAIGN_TEXT_SECOND_CASE_ADJUSTMENT);
        campaignTextInputBuffer[textInputLength++]=character;
        MwEraseTextInputCursor();
        textInputCursorX=EgaDrawCharacter(textInputCursorX,&textInputCursorY,
            character,textInputForeground,textInputBackground);
    }
}

/* MW_MAIN081B:04AD..05FD. Measure raw201h discharge, retry if the tick changed
 * after STI, detect new active-low button presses, then classify directions. */
void MwPollJoystickA(void)
{
    if (mwCampaignInput.readGamePort == NULL || mwCampaignInput.writeGamePort == NULL
        || mwCampaignInput.setInterruptsEnabled == NULL) abort();
    uint16_t startTick;
    uint8_t port;
    do
    {
        mwCampaignInput.setInterruptsEnabled(false);
        startTick = campaignTimerTicks;
        joystickDirection = JOYSTICK_CENTRED;
        joystickDeflection = JOYSTICK_DEFLECTION_NEAR;
        joystickButtonOnePressed = 0; joystickButtonTwoPressed = 0;
        uint16_t count = 0;
        mwCampaignInput.writeGamePort((uint8_t)startTick);
        for (;;)
        {
            port = mwCampaignInput.readGamePort(); count++;
            if (!(port & GAMEPORT_X_CHARGED))
            {
                joystickXCount = count;
                do { port = mwCampaignInput.readGamePort(); count++; }
                while ((port & GAMEPORT_Y_CHARGED) && !(port & GAMEPORT_X_CHARGED));
                joystickYCount = count;
                break;
            }
            if (!(port & GAMEPORT_Y_CHARGED))
            {
                joystickYCount = count;
                do { port = mwCampaignInput.readGamePort(); count++; }
                while ((port & GAMEPORT_X_CHARGED) && !(port & GAMEPORT_Y_CHARGED));
                joystickXCount = count;
                break;
            }
        }
        mwCampaignInput.setInterruptsEnabled(true);
    } while (startTick != campaignTimerTicks);
    if (!(port & GAMEPORT_BUTTON_ONE) && (joystickPreviousButtons & GAMEPORT_BUTTON_ONE)) joystickButtonOnePressed = 1;
    if (!(port & GAMEPORT_BUTTON_TWO) && (joystickPreviousButtons & GAMEPORT_BUTTON_TWO)) joystickButtonTwoPressed = 1;
    joystickPreviousButtons = port;
    uint16_t y = joystickYCount;
    if (joystickXCount < joystickXLow)
    {
        if (y < joystickYLow)
        { joystickDirection = JOYSTICK_UP_LEFT; if (y < joystickYFarLow) goto farDeflection; }
        else if (y > joystickYHigh)
        { joystickDirection = JOYSTICK_DOWN_LEFT; if (y > joystickYFarHigh) goto farDeflection; }
        else joystickDirection = JOYSTICK_LEFT;
        if (joystickXCount < joystickXFarLow) goto farDeflection;
    }
    else if (joystickXCount > joystickXHigh)
    {
        if (y < joystickYLow)
        { joystickDirection = JOYSTICK_UP_RIGHT; if (y < joystickYFarLow) goto farDeflection; }
        else if (y > joystickYHigh)
        { joystickDirection = JOYSTICK_DOWN_RIGHT; if (y > joystickYFarHigh) goto farDeflection; }
        else joystickDirection = JOYSTICK_RIGHT;
        if (joystickXCount > joystickXFarHigh) goto farDeflection;
    }
    else if (y < joystickYLow)
    { joystickDirection = JOYSTICK_UP; if (y < joystickYFarLow) goto farDeflection; }
    else if (y > joystickYHigh)
    { joystickDirection = JOYSTICK_DOWN; if (y > joystickYFarHigh) goto farDeflection; }
    return;
farDeflection:
    joystickDeflection = JOYSTICK_DEFLECTION_FAR;
}
