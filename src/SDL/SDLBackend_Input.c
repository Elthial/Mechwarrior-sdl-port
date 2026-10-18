#include "SDLBackend.h"
#include "mechwarrior/campaign_input.h"
#include "mechwarrior/combat_input.h"
#include <stdio.h>
#include <stdlib.h>

enum { HOST_KEYBOARD_SCAN_QUEUE_BYTES=16,HOST_DEFAULT_DRIVE_C=2 };
static uint16_t keys[BIOS_KEY_BUFFER_WORDS];
static uint8_t keyShifts[BIOS_KEY_BUFFER_WORDS],readIndex,writeIndex,lastShift;
static bool quitRequested;
static uint32_t captureNumber;
static uint16_t mouseReleaseCounts[MOUSE_BUTTON_COUNT];
static float mouseXMotion,mouseYMotion;
static uint8_t keyboardScans[HOST_KEYBOARD_SCAN_QUEUE_BYTES],keyboardRead,keyboardWrite;
bool MwSdlCombatKeyboardPending(void) {return mwSdlCombatActive && keyboardRead!=keyboardWrite;}
uint8_t MwSdlReadKeyboardScanByte(void)
{
    if (!MwSdlCombatKeyboardPending()) MwSdlFail("Keyboard controller has no scan byte");
    return keyboardScans[keyboardRead];
}
void MwSdlDeliverCombatKeyboardInterrupt(void)
{
    if (!btechHostGetInterruptVector) MwSdlFail("Missing keyboard vector binding");
    uint32_t vector=btechHostGetInterruptVector(KEYBOARD_INTERRUPT);
    if (vector==BTECH_KEYBOARD_HANDLER_VECTOR) BtechKeyboardInterrupt();
    else if (!vector)
    {
        /* Opaque0 is the configured pre-child system handler, NOT BIOS bytes
         * at0000:0000. Ordinary releases produce no BIOS key and do not touch
         * BTECH's held-key/last-key state after2C8D restores the system vector.
         * Modifier/lock/Insert releases and all makes still need the complete
         * firmware translation; never pretend they are harmless releases. */
        uint8_t scan=MwSdlReadKeyboardScanByte(),key=scan&KEYBOARD_SCAN_MASK;
        if (!(scan&KEYBOARD_BREAK_FLAG) || !key || key>=COMBAT_KEYBOARD_SCAN_CODES
            || key==COMBAT_KEY_CONTROL || key==COMBAT_KEY_LEFT_SHIFT || key==COMBAT_KEY_RIGHT_SHIFT || key==COMBAT_KEY_ALT
            || key==COMBAT_KEY_CAPS_LOCK || key==COMBAT_KEY_NUM_LOCK || key==COMBAT_KEY_SCROLL_LOCK || key==COMBAT_KEY_INSERT || key==COMBAT_KEY_SYSTEM_REQUEST)
            MwSdlFail("Unconverted pre-child BIOS keyboard scan");
        uint8_t control=MwSdlReadDevicePort(KEYBOARD_CONTROL_PORT);
        MwSdlWriteDevicePort(KEYBOARD_CONTROL_PORT,control|KEYBOARD_ACKNOWLEDGE_BIT);
        MwSdlWriteDevicePort(KEYBOARD_CONTROL_PORT,control);
        MwSdlWriteDevicePort(PIC_COMMAND_PORT,PIC_END_OF_INTERRUPT);
    }
    else MwSdlFail("Unmapped native keyboard vector");
    keyboardRead=(keyboardRead+1)%sizeof(keyboardScans);
}
static void QueueCombatScan(uint8_t scan)
{
    uint8_t next=(keyboardWrite+1)%sizeof(keyboardScans);
    if (next!=keyboardRead) {keyboardScans[keyboardWrite]=scan;keyboardWrite=next;}
}
void MwSdlQueueCombatScan(uint8_t scan) {if(mwSdlCombatActive)QueueCombatScan(scan);}

void MwSdlQueueBiosKey(uint16_t key,uint8_t shift)
{
    if (getenv("MECHWARRIOR_DIAGNOSTICS"))
    {fprintf(stderr,"BIOS key=%04X shift=%02X time=%llu\n",key,shift,(unsigned long long)SDL_GetTicks());fflush(stderr);}
    uint8_t next=(writeIndex+1)%BIOS_KEY_BUFFER_WORDS;
    if (next==readIndex) return; /* BIOS ring is full; no game action mapping. */
    keys[writeIndex]=key;keyShifts[writeIndex]=shift;writeIndex=next;
}
void MwSdlRequestQuit(void) {quitRequested=true;}
static uint8_t ScanCode(SDL_Scancode scan)
{
    static const struct {SDL_Scancode scan;uint8_t bios;} map[]={
        {SDL_SCANCODE_ESCAPE,COMBAT_KEY_ESCAPE},{SDL_SCANCODE_RETURN,COMBAT_KEY_ENTER},{SDL_SCANCODE_KP_ENTER,COMBAT_KEY_ENTER},
        {SDL_SCANCODE_SPACE,COMBAT_KEY_SPACE},{SDL_SCANCODE_TAB,COMBAT_KEY_TAB},{SDL_SCANCODE_BACKSPACE,COMBAT_KEY_BACKSPACE},
        {SDL_SCANCODE_LCTRL,COMBAT_KEY_CONTROL},{SDL_SCANCODE_RCTRL,COMBAT_KEY_CONTROL},{SDL_SCANCODE_LSHIFT,COMBAT_KEY_LEFT_SHIFT},{SDL_SCANCODE_RSHIFT,COMBAT_KEY_RIGHT_SHIFT},
        {SDL_SCANCODE_LALT,COMBAT_KEY_ALT},{SDL_SCANCODE_RALT,COMBAT_KEY_ALT},{SDL_SCANCODE_CAPSLOCK,COMBAT_KEY_CAPS_LOCK},{SDL_SCANCODE_NUMLOCKCLEAR,COMBAT_KEY_NUM_LOCK},
        {SDL_SCANCODE_MINUS,COMBAT_KEY_MINUS},{SDL_SCANCODE_EQUALS,COMBAT_KEY_EQUALS},{SDL_SCANCODE_KP_MINUS,COMBAT_KEY_KEYPAD_MINUS},{SDL_SCANCODE_KP_PLUS,COMBAT_KEY_KEYPAD_PLUS},
        {SDL_SCANCODE_COMMA,COMBAT_KEY_COMMA},{SDL_SCANCODE_PERIOD,COMBAT_KEY_PERIOD},{SDL_SCANCODE_SLASH,COMBAT_KEY_SLASH},
        {SDL_SCANCODE_UP,COMBAT_KEY_UP},{SDL_SCANCODE_DOWN,COMBAT_KEY_DOWN},{SDL_SCANCODE_LEFT,COMBAT_KEY_LEFT},{SDL_SCANCODE_RIGHT,COMBAT_KEY_RIGHT},
        {SDL_SCANCODE_HOME,COMBAT_KEY_HOME},{SDL_SCANCODE_END,COMBAT_KEY_END},{SDL_SCANCODE_PAGEUP,COMBAT_KEY_PAGE_UP},{SDL_SCANCODE_PAGEDOWN,COMBAT_KEY_PAGE_DOWN},
        {SDL_SCANCODE_KP_7,COMBAT_KEY_HOME},{SDL_SCANCODE_KP_8,COMBAT_KEY_UP},{SDL_SCANCODE_KP_9,COMBAT_KEY_PAGE_UP},
        {SDL_SCANCODE_KP_4,COMBAT_KEY_LEFT},{SDL_SCANCODE_KP_5,COMBAT_KEY_KEYPAD_STOP},{SDL_SCANCODE_KP_6,COMBAT_KEY_RIGHT},
        {SDL_SCANCODE_KP_1,COMBAT_KEY_END},{SDL_SCANCODE_KP_2,COMBAT_KEY_DOWN},{SDL_SCANCODE_KP_3,COMBAT_KEY_PAGE_DOWN},
        {SDL_SCANCODE_A,COMBAT_KEY_A},{SDL_SCANCODE_B,COMBAT_KEY_B},{SDL_SCANCODE_C,COMBAT_KEY_C},{SDL_SCANCODE_D,COMBAT_KEY_D},
        {SDL_SCANCODE_E,COMBAT_KEY_E},{SDL_SCANCODE_F,COMBAT_KEY_F},{SDL_SCANCODE_G,COMBAT_KEY_G},{SDL_SCANCODE_H,COMBAT_KEY_H},
        {SDL_SCANCODE_I,COMBAT_KEY_I},{SDL_SCANCODE_J,COMBAT_KEY_J},{SDL_SCANCODE_K,COMBAT_KEY_K},{SDL_SCANCODE_L,COMBAT_KEY_L},
        {SDL_SCANCODE_M,COMBAT_KEY_M},{SDL_SCANCODE_N,COMBAT_KEY_N},{SDL_SCANCODE_O,COMBAT_KEY_O},{SDL_SCANCODE_P,COMBAT_KEY_P},
        {SDL_SCANCODE_Q,COMBAT_KEY_Q},{SDL_SCANCODE_R,COMBAT_KEY_R},{SDL_SCANCODE_S,COMBAT_KEY_S},{SDL_SCANCODE_T,COMBAT_KEY_T},
        {SDL_SCANCODE_U,COMBAT_KEY_U},{SDL_SCANCODE_V,COMBAT_KEY_V},{SDL_SCANCODE_W,COMBAT_KEY_W},{SDL_SCANCODE_X,COMBAT_KEY_X},
        {SDL_SCANCODE_Y,COMBAT_KEY_Y},{SDL_SCANCODE_Z,COMBAT_KEY_Z},{SDL_SCANCODE_1,COMBAT_KEY_1},{SDL_SCANCODE_2,COMBAT_KEY_2},
        {SDL_SCANCODE_3,COMBAT_KEY_3},{SDL_SCANCODE_4,COMBAT_KEY_4},{SDL_SCANCODE_5,COMBAT_KEY_5},{SDL_SCANCODE_6,COMBAT_KEY_6},
        {SDL_SCANCODE_7,COMBAT_KEY_7},{SDL_SCANCODE_8,COMBAT_KEY_8},{SDL_SCANCODE_9,COMBAT_KEY_9},{SDL_SCANCODE_0,COMBAT_KEY_0},
        {SDL_SCANCODE_F1,COMBAT_KEY_F1},{SDL_SCANCODE_F2,COMBAT_KEY_F2},{SDL_SCANCODE_F3,COMBAT_KEY_F3},{SDL_SCANCODE_F4,COMBAT_KEY_F4},
        {SDL_SCANCODE_F5,COMBAT_KEY_F5},{SDL_SCANCODE_F6,COMBAT_KEY_F6},{SDL_SCANCODE_F7,COMBAT_KEY_F7},{SDL_SCANCODE_F8,COMBAT_KEY_F8},
        {SDL_SCANCODE_F9,COMBAT_KEY_F9},{SDL_SCANCODE_F10,COMBAT_KEY_F10}};
    for (uint16_t i=0;i<sizeof(map)/sizeof(map[0]);++i) if (map[i].scan==scan) return map[i].bios;
    return 0;
}
void MwSdlPollEvents(void)
{
    MwSdlPollDebugControl();
    MwSdlPollSupportPilot();
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        if(event.type==SDL_EVENT_KEY_DOWN && event.key.scancode==SDL_SCANCODE_F11)
        {MwSdlStopSupportPilot();continue;}
        if(event.type==SDL_EVENT_KEY_DOWN && (event.key.scancode==SDL_SCANCODE_P || event.key.scancode==SDL_SCANCODE_ESCAPE))
            MwSdlStopSupportPilot(); /* Explicit manual pause/retreat always takes over. */
        if (event.type==SDL_EVENT_QUIT) {MwSdlRequestQuit();continue;}
        if (event.type==SDL_EVENT_WINDOW_EXPOSED) {MwSdlPresentVideo(true);continue;}
        if (event.type==SDL_EVENT_MOUSE_BUTTON_UP)
        {
            uint16_t button=event.button.button==SDL_BUTTON_LEFT?MOUSE_BUTTON_LEFT:
                event.button.button==SDL_BUTTON_RIGHT?MOUSE_BUTTON_RIGHT:UINT16_MAX;
            if (button<MOUSE_BUTTON_COUNT && mouseReleaseCounts[button]!=UINT16_MAX) ++mouseReleaseCounts[button];
            continue;
        }
        if (event.type==SDL_EVENT_MOUSE_MOTION)
        {
            float logicalXMotion,logicalYMotion;
            MwSdlConvertMouseMotion(event.motion.xrel,event.motion.yrel,&logicalXMotion,&logicalYMotion);
            mouseXMotion+=logicalXMotion;mouseYMotion+=logicalYMotion;
            continue;
        }
        if (mwSdlCombatActive && (event.type==SDL_EVENT_KEY_DOWN || event.type==SDL_EVENT_KEY_UP) && event.key.scancode!=SDL_SCANCODE_F12)
        {
            uint8_t scan=ScanCode(event.key.scancode);
            if (scan) QueueCombatScan(scan|(event.type==SDL_EVENT_KEY_UP?KEYBOARD_BREAK_FLAG:0));
            continue;
        }
        if (event.type!=SDL_EVENT_KEY_DOWN) continue;
        if (event.key.scancode==SDL_SCANCODE_F12)
        {
            char stem[48];snprintf(stem,sizeof(stem),"capture-%u",captureNumber++);
            if (!MwSdlCapture(stem)) fprintf(stderr,"Capture failed or file already exists.\n");
            continue;
        }
        uint8_t scan=ScanCode(event.key.scancode);if (!scan) continue;
        uint8_t ascii=event.key.key<ASCII_CHARACTER_LIMIT?(uint8_t)event.key.key:0;
        if ((event.key.mod&SDL_KMOD_NUM) && event.key.scancode>=SDL_SCANCODE_KP_1
            && event.key.scancode<=SDL_SCANCODE_KP_9)
            ascii=(uint8_t)('1'+event.key.scancode-SDL_SCANCODE_KP_1);
        if (scan==COMBAT_KEY_ENTER) ascii='\r';
        if (event.key.mod&SDL_KMOD_CTRL) {if (ascii>='a' && ascii<='z') ascii-=('a'-1);}
        else if (((event.key.mod&SDL_KMOD_SHIFT)!=0)^((event.key.mod&SDL_KMOD_CAPS)!=0))
        {if (ascii>='a' && ascii<='z') ascii-=ASCII_CASE_DISTANCE;}
        if (event.key.mod&SDL_KMOD_ALT) ascii=0;
        uint8_t shift=(event.key.mod&SDL_KMOD_CTRL)?BIOS_SHIFT_CONTROL:0;
        MwSdlQueueBiosKey(((uint16_t)scan<<BIOS_KEY_SCAN_SHIFT)|ascii,shift);
    }
    if (quitRequested)
    {
        if (mwSdlCombatActive) {QueueCombatScan(COMBAT_KEY_CONTROL);QueueCombatScan(COMBAT_KEY_ESCAPE);quitRequested=false;}
        else if (readIndex==writeIndex) MwSdlQueueBiosKey(CAMPAIGN_KEY_ESCAPE,BIOS_SHIFT_CONTROL);
    }
}
static bool Available(void) {MwSdlPump();return readIndex!=writeIndex;}
static uint16_t ReadKey(void)
{
    while (readIndex==writeIndex) MwSdlPump();
    uint16_t key=keys[readIndex];lastShift=keyShifts[readIndex];readIndex=(readIndex+1)%BIOS_KEY_BUFFER_WORDS;
    return key;
}
static uint8_t ShiftFlags(void)
{return lastShift|((SDL_GetModState()&SDL_KMOD_CTRL)?BIOS_SHIFT_CONTROL:0);}
static uint16_t MouseRelease(uint16_t button)
{
    MwSdlPump();
    if (button>=MOUSE_BUTTON_COUNT) return 0;
    uint16_t count=mouseReleaseCounts[button];mouseReleaseCounts[button]=0;return count;
}
static void MouseReset(void)
{mouseReleaseCounts[MOUSE_BUTTON_LEFT]=mouseReleaseCounts[MOUSE_BUTTON_RIGHT]=0;mouseXMotion=mouseYMotion=0;}
static void MouseMotion(uint16_t *x,uint16_t *y)
{
    MwSdlPump();
    int16_t xMotion=(int16_t)mouseXMotion,yMotion=(int16_t)mouseYMotion;
    mouseXMotion-=xMotion;mouseYMotion-=yMotion;
    *x=(uint16_t)xMotion;*y=(uint16_t)yMotion;
}
static uint8_t GamePort(void) {return GAMEPORT_DISCONNECTED;} /* No joystick connected: axes never discharge. */
static void ChargeGamePort(uint8_t value) {(void)value;}
static uint8_t CurrentDrive(void) {return HOST_DEFAULT_DRIVE_C;}
void MwSdlInitializeInput(void)
{
    readIndex=writeIndex=lastShift=0;quitRequested=false;captureNumber=0;MouseReset();
    keyboardRead=keyboardWrite=0;
    MwSdlInitializeDebugControl();
    mwCampaignInput=(MwCampaignInputCalls){MouseRelease,Available,ReadKey,GamePort,ChargeGamePort,
        MwSdlSetInterrupts,MwSdlPump,CurrentDrive,MouseReset,MouseMotion,ShiftFlags,
        MwSdlSetMousePosition,MwSdlReadMousePosition};
}
