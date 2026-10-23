#include "mechwarrior/port_diagnostics.h"
#include "mechwarrior/combat_cockpit.h"
#include <stdlib.h>

/*0810:689E. Shared ten-byte field storage, not an allocated string. Native
 * NEG8000 stays8000 and its signed IDIV emits odd negative-remainder digits. */
const char *BtechFormatSignedDecimalField(int16_t value,uint16_t fieldWidth,uint16_t padding)
{
    if (fieldWidth>=COMBAT_DECIMAL_FIELD_BYTES || (!fieldWidth && !value)) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: fieldWidth>=COMBAT_DECIMAL_FIELD_BYTES || (!fieldWidth && !value)"); /* Native adjacent-owner write. */
    decimalFieldText[fieldWidth]=0;
    int16_t digit=fieldWidth-1;
    bool negative=value<0;
    if (negative) value=(int16_t)(0-(uint16_t)value);
    if (!value) decimalFieldText[digit--]='0';
    while (digit>=0)
    {
        uint8_t character;
        if (!value) character=(uint8_t)padding;
        else
        {
            character=(uint8_t)(value%10+'0');
            value/=10;
            if (negative && !value && digit>0)
            {
                decimalFieldText[digit--]=(char)character;
                character='-';
            }
        }
        decimalFieldText[digit--]=(char)character;
    }
    return decimalFieldText;
}

/*0810:6BB6. Genuine FAR forwarding veneer, same three WORD arguments. */
const char *BtechFormatSignedDecimalFieldFar(int16_t value,uint16_t fieldWidth,uint16_t padding)
{
    return BtechFormatSignedDecimalField(value,fieldWidth,padding);
}
