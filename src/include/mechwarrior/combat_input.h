#ifndef MECHWARRIOR_COMBAT_INPUT_H
#define MECHWARRIOR_COMBAT_INPUT_H
#include "mechwarrior/combat_startup.h"
#include "mechwarrior/combat_import.h"
#include "mechwarrior/combat_platform.h"
#include "mechwarrior/input_hardware.h"
enum { COMBAT_DIRECTION_UP=1,COMBAT_DIRECTION_DOWN=2,COMBAT_DIRECTION_LEFT=4,COMBAT_DIRECTION_RIGHT=8,
    COMBAT_KEY_TAB=0x0F,COMBAT_KEY_Q=0x10,COMBAT_KEY_W=0x11,COMBAT_KEY_E=0x12,COMBAT_KEY_R=0x13,COMBAT_KEY_T=0x14,
    COMBAT_KEY_U=0x16,COMBAT_KEY_O=0x18,COMBAT_KEY_P=0x19,COMBAT_KEY_ENTER=0x1C,COMBAT_KEY_A=0x1E,
    COMBAT_KEY_S=0x1F,COMBAT_KEY_D=0x20,COMBAT_KEY_H=0x23,COMBAT_KEY_J=0x24,COMBAT_KEY_Z=0x2C,
    COMBAT_KEY_GRAVE=0x29,COMBAT_KEY_BACKSLASH=0x2B,
    COMBAT_KEY_C=0x2E,COMBAT_KEY_ALT=0x38,COMBAT_KEY_SPACE=0x39,
    COMBAT_KEY_CAPS_LOCK=0x3A,COMBAT_KEY_NUM_LOCK=0x45,
    COMBAT_KEY_CONTROL=0x1D,COMBAT_KEY_LEFT_SHIFT=0x2A,COMBAT_KEY_RIGHT_SHIFT=0x36,
    COMBAT_KEY_SCROLL_LOCK=0x46,COMBAT_KEY_INSERT=0x52,COMBAT_KEY_SYSTEM_REQUEST=0x54,
    COMBAT_KEY_BACKSPACE=0x0E,COMBAT_KEY_MINUS=0x0C,COMBAT_KEY_EQUALS=0x0D,COMBAT_KEY_SLASH=0x35,
    COMBAT_KEY_B=0x30,COMBAT_KEY_F=0x21,COMBAT_KEY_G=0x22,COMBAT_KEY_I=0x17,
    COMBAT_KEY_K=0x25,COMBAT_KEY_L=0x26,COMBAT_KEY_V=0x2F,COMBAT_KEY_X=0x2D,COMBAT_KEY_Y=0x15,
    COMBAT_KEY_1=0x02,COMBAT_KEY_2=0x03,COMBAT_KEY_3=0x04,COMBAT_KEY_4=0x05,COMBAT_KEY_5=0x06,
    COMBAT_KEY_6=0x07,COMBAT_KEY_7=0x08,COMBAT_KEY_8=0x09,COMBAT_KEY_9=0x0A,COMBAT_KEY_0=0x0B,
    COMBAT_KEY_F1=0x3B,COMBAT_KEY_F2=0x3C,COMBAT_KEY_F3=0x3D,COMBAT_KEY_F4=0x3E,COMBAT_KEY_F5=0x3F,
    COMBAT_KEY_F6=0x40,COMBAT_KEY_F7=0x41,COMBAT_KEY_F8=0x42,COMBAT_KEY_F9=0x43,COMBAT_KEY_F10=0x44,
    COMBAT_KEY_KEYPAD_MINUS=0x4A,COMBAT_KEY_KEYPAD_STOP=0x4C,COMBAT_KEY_KEYPAD_PLUS=0x4E,
    COMBAT_KEY_ESCAPE=1,COMBAT_KEY_N=0x31,COMBAT_KEY_M=0x32,COMBAT_KEY_COMMA=0x33,COMBAT_KEY_PERIOD=0x34,
    COMBAT_KEY_HOME=0x47,COMBAT_KEY_UP=0x48,COMBAT_KEY_PAGE_UP=0x49,
    COMBAT_KEY_LEFT=0x4B,COMBAT_KEY_RIGHT=0x4D,COMBAT_KEY_END=0x4F,COMBAT_KEY_DOWN=0x50,COMBAT_KEY_PAGE_DOWN=0x51,
    BIOS_KEYBOARD_START=0x80,BIOS_KEYBOARD_END=0x82,
    BIOS_CONTROL_ESCAPE_KEY=(COMBAT_KEY_ESCAPE<<BIOS_KEY_SCAN_SHIFT)|('\x1B'|ASCII_CONTROL_MARKER) };
extern uint16_t terminationFlag,combatInputScanCode,combatInputDirectionMask;
extern uint16_t combatInputButton1State,combatInputButton0State,combatInputReplayCancelLatched;
extern uint8_t heldDirectionKeyMasks[2],heldDirectionMasks[0x10];
extern uint16_t keyboardLastKey;
extern uint8_t keyboardUnshiftedCharacters[COMBAT_KEYBOARD_SCAN_CODES],keyboardShiftedCharacters[COMBAT_KEYBOARD_SCAN_CODES];
extern uint8_t keyboardCompatibilityScans[COMBAT_KEYBOARD_COMPATIBILITY_CODES],keyboardCompatibilityReplacements[COMBAT_KEYBOARD_COMPATIBILITY_CODES];
void BtechKeyboardInterrupt(void);
void BtechRequestTermination(void);
uint16_t BtechPopBiosKeyboardBuffer(void);
uint16_t BtechReadHeldDirectionMask(uint16_t keyGroup);
uint16_t BtechReadHeldKeyState(uint16_t scanCode);
uint8_t BtechReadJoystickDirectionMask(uint16_t joystick);
uint16_t BtechReadJoystickPresenceState(uint16_t button);
void BtechPollCombatCommandInput(void);
void BtechWaitForCombatInputReleaseThenPress(void);
extern uint16_t fireCycleStartMount,requestedTorsoAngle,previousAimButtonState;
extern uint16_t previousFireButtonState,fireCycleActive,requestedAimPitch,unidentifiedField24;
void BtechProcessPlayerControls(void);
#endif
