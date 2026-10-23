#include "mechwarrior/port_diagnostics.h"
#include "mechwarrior/combat_input.h"
#include "mechwarrior/combat_video.h"
#include <stdlib.h>

/*1BF5:2CCB. Genuine IRQ1, no source arguments. Read the make/break byte from
 * port60, acknowledge through61, then execute original translation/state/BIOS
 * ring logic. SDL must not synthesize these game-owned tables or commands. */
void BtechKeyboardInterrupt(void)
{
    if (!btechHostReadPort || !btechHostWritePort || !btechHostReadBiosByte || !btechHostWriteBiosByte
        || !btechHostReadBiosWord || !btechHostWriteBiosWord) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !btechHostReadPort || !btechHostWritePort || !btechHostReadBiosByte || !btechHostWriteBiosByte || !btechHostReadBiosWord || !btechHostWriteBiosWord");
    uint8_t scan=btechHostReadPort(KEYBOARD_DATA_PORT);
    uint8_t keyboardControl=btechHostReadPort(KEYBOARD_CONTROL_PORT);
    btechHostWritePort(KEYBOARD_CONTROL_PORT,keyboardControl|KEYBOARD_ACKNOWLEDGE_BIT);
    btechHostWritePort(KEYBOARD_CONTROL_PORT,keyboardControl);
    if (pcjrTandyCompatible==1)
    {
        if (keyboardBiosIntercept==1)
        {
            if (scan==COMBAT_KEY_GRAVE) scan=COMBAT_KEY_UP;
            if (scan==COMBAT_KEY_BACKSLASH) scan=COMBAT_KEY_LEFT;
        }
        else
            for (uint16_t index=0;index<COMBAT_KEYBOARD_COMPATIBILITY_CODES;++index)
                if (scan==keyboardCompatibilityScans[index]) {scan=keyboardCompatibilityReplacements[index];break;}
    }
    uint8_t scanIndex=scan&KEYBOARD_SCAN_MASK;
    if (scanIndex>=COMBAT_KEYBOARD_SCAN_CODES) goto acknowledge;
    uint8_t attributes=keyboardScanTranslations[scanIndex];
    keyboardScanTranslations[scanIndex]=(attributes
        &((scan&KEYBOARD_BREAK_FLAG)?UINT8_MAX:KEYBOARD_ATTRIBUTE_CLEAR_HELD))^KEYBOARD_ATTRIBUTE_HELD;
    if (keyboardBiosIntercept==1)
    {
        if (scanIndex==COMBAT_KEY_CAPS_LOCK) {scan=scanIndex;attributes&=keyboardScanMask;}
        else if (scanIndex==COMBAT_KEY_NUM_LOCK) {scan=scanIndex;attributes&=keyboardExtendedScanMask;}
    }
    if (scan&KEYBOARD_BREAK_FLAG)
    {
        if (attributes&KEYBOARD_ATTRIBUTE_DIRECTION_MASK)
        {
            uint16_t group=(attributes>>KEYBOARD_ATTRIBUTE_GROUP_SHIFT)&KEYBOARD_ATTRIBUTE_GROUP_MASK;
            uint8_t direction=attributes>>KEYBOARD_ATTRIBUTE_DIRECTION_SHIFT;
            if (heldDirectionKeyMasks[group]==direction) heldDirectionKeyMasks[group]=0;
        }
        keyboardLastKey=0;
        uint8_t character=keyboardUnshiftedCharacters[scan&KEYBOARD_SCAN_MASK];
        if ((character&KEYBOARD_CHARACTER_SPECIAL) && !(character&KEYBOARD_CHARACTER_TOGGLE_MASK))
            btechHostWriteBiosByte(BIOS_KEYBOARD_SHIFT_FLAGS,btechHostReadBiosByte(BIOS_KEYBOARD_SHIFT_FLAGS)&(character^KEYBOARD_CHARACTER_VALUE_MASK));
        goto acknowledge;
    }
    if (attributes&KEYBOARD_ATTRIBUTE_DIRECTION_MASK)
        heldDirectionKeyMasks[(attributes>>KEYBOARD_ATTRIBUTE_GROUP_SHIFT)&KEYBOARD_ATTRIBUTE_GROUP_MASK]
            =attributes>>KEYBOARD_ATTRIBUTE_DIRECTION_SHIFT;
    uint8_t character=keyboardUnshiftedCharacters[scan];
    if (character&KEYBOARD_CHARACTER_SPECIAL)
    {
        character&=KEYBOARD_CHARACTER_VALUE_MASK;
        if (!(character&KEYBOARD_CHARACTER_TOGGLE_MASK)) btechHostWriteBiosByte(BIOS_KEYBOARD_SHIFT_FLAGS,btechHostReadBiosByte(BIOS_KEYBOARD_SHIFT_FLAGS)|character);
        else if (!((character&KEYBOARD_CHARACTER_CAPS_LOCK) && keyboardCapsLockRequested) && !(attributes&KEYBOARD_ATTRIBUTE_HELD))
            btechHostWriteBiosByte(BIOS_KEYBOARD_SHIFT_FLAGS,btechHostReadBiosByte(BIOS_KEYBOARD_SHIFT_FLAGS)^character);
        goto acknowledge;
    }
    if (btechHostReadBiosByte(BIOS_KEYBOARD_SHIFT_FLAGS)&BIOS_SHIFT_CONTROL)
    {character|=ASCII_CONTROL_MARKER;if (attributes&KEYBOARD_ATTRIBUTE_ALPHABETIC) character-=ASCII_CASE_DISTANCE;}
    else if ((btechHostReadBiosByte(BIOS_KEYBOARD_SHIFT_FLAGS)&BIOS_SHIFT_CAPS_LOCK) && (attributes&KEYBOARD_ATTRIBUTE_ALPHABETIC)) character-=ASCII_CASE_DISTANCE;
    else if (btechHostReadBiosByte(BIOS_KEYBOARD_SHIFT_FLAGS)&BIOS_SHIFT_EITHER_SHIFT) character=keyboardShiftedCharacters[scan];
    uint16_t key=((uint16_t)scan<<BIOS_KEY_SCAN_SHIFT)|character;
    keyboardLastKey=key;
    uint16_t head=btechHostReadBiosWord(BIOS_KEYBOARD_HEAD),tail=btechHostReadBiosWord(BIOS_KEYBOARD_TAIL);
    if ((head==BIOS_KEY_BUFFER_LAST && tail!=BIOS_KEY_BUFFER_FIRST)
        || (head!=BIOS_KEY_BUFFER_LAST && (uint16_t)(head+BIOS_KEY_BUFFER_WORD_BYTES)!=tail))
    {
        btechHostWriteBiosWord(tail,key);
        if (tail==BIOS_KEY_BUFFER_LAST) tail=BIOS_KEY_BUFFER_BEFORE_FIRST;
        tail+=BIOS_KEY_BUFFER_WORD_BYTES;btechHostWriteBiosWord(BIOS_KEYBOARD_TAIL,tail);
    }
    if (scan==COMBAT_KEY_D && (btechHostReadBiosByte(BIOS_KEYBOARD_SHIFT_FLAGS)&BIOS_SHIFT_CONTROL)) goto acknowledge;
    if (key==BIOS_CONTROL_ESCAPE_KEY && (btechHostReadBiosByte(BIOS_KEYBOARD_SHIFT_FLAGS)&BIOS_SHIFT_CONTROL))
    {
        tail-=BIOS_KEY_BUFFER_WORD_BYTES;if (tail==BIOS_KEY_BUFFER_BEFORE_FIRST) tail=BIOS_KEY_BUFFER_LAST;
        btechHostWriteBiosWord(tail,0);
        btechHostWriteBiosWord(BIOS_KEYBOARD_HEAD,btechHostReadBiosWord(BIOS_KEYBOARD_TAIL));
        btechHostWritePort(PIC_COMMAND_PORT,PIC_END_OF_INTERRUPT);
        BtechRequestTermination();return;
    }
acknowledge:
    btechHostWritePort(PIC_COMMAND_PORT,PIC_END_OF_INTERRUPT);
}
