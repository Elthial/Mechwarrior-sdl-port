#include "mechwarrior/port_diagnostics.h"
#include "mechwarrior/combat_sound.h"
#include <stdlib.h>

enum { MIDI_STATUS_FLAG=0x80,MIDI_META=0xFF,MIDI_END_OF_TRACK=0x2F,
    MIDI_SET_TEMPO=0x51,MIDI_IDLE=0xF0,MUSIC_TICK_MULTIPLIER=0x024E,
    MIDI_TEMPO_DIVIDEND=0x00039390 };

/*229C:0020. Authored one-byte RETF; preserve AX for the parser's AH input. */
uint16_t BtechMusicMessageNoOperation(uint16_t statusDisplacement,uint16_t channel,uint16_t packedData)
{
    (void)statusDisplacement;(void)channel;return packedData;
}

/*229C:0021. DIV BYTE leaves its remainder in AH before the whole-WORD SHR.
 * The selector-presence flag transports a real native pair, not a game flag. */
void BtechStartSoftwareMusicTrack(MwNativeByteView musicHeader,uint16_t headerOffset,bool selectorPresent)
{
    uint16_t division=MwReadNativeViewWord(&musicHeader,(uint16_t)(headerOffset+4));
    if (!softwareMusicDivisionScale || division/softwareMusicDivisionScale>UINT8_MAX) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !softwareMusicDivisionScale || division/softwareMusicDivisionScale>UINT8_MAX");
    uint16_t divided=((division%softwareMusicDivisionScale)<<8)|(division/softwareMusicDivisionScale);
    divided>>=1;softwareMusicRateShift=(uint8_t)divided;
    combatMidiStreamOffset=headerOffset+COMBAT_TONE_HEADER_BYTES;
    midiStreamData=musicHeader;midiStreamSelectorPresent=selectorPresent;
    BtechDecodeNextMidiEvent((uint8_t)(divided>>8));
    softwareMusicDelayTicks=1;
}

/*229C:0051. Drain zero/quantized-zero deltas in this tick; no invented cap.
 * Callback AX residue is observable when parsing an idle next block. */
void BtechTickSoftwareMusic(void)
{
    if (--softwareMusicDelayTicks) return;
    if (!combatMidiStreamOffset && !midiStreamSelectorPresent) return;
    for (;;)
    {
        uint16_t statusDisplacement=(midiRunningStatus>>3)&0x0E;
        uint16_t channel=midiChannel;
        uint16_t message=midiMessageData[0]|((uint16_t)midiMessageData[1]<<8);
        if (!audioMusicCallback) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !audioMusicCallback");
        uint16_t callbackResidue=audioMusicCallback(statusDisplacement,channel,message);
        BtechDecodeNextMidiEvent((uint8_t)(callbackResidue>>8));
        uint16_t delta=queuedMidiDeltaTicks;
        if (!delta) continue;
        uint16_t delay;
        uint8_t shift=softwareMusicRateShift;
        if (!shift)
        {
            delay=delta<<3;delay+=delta;delay<<=1;delay+=delta;
            delay<<=1;delay+=delta;delay<<=1;
            delay=(delay&0x00FF)|((uint16_t)(uint8_t)((delay>>8)+(uint8_t)delta)<<8);
            delay<<=1;
            if (!mpuTempoInterval) /* Port emulation of a native CPU fault. */ MW_NATIVE_FAULT("Rejected condition: !mpuTempoInterval"); /* Parser's earlier writes survive the native DIV fault. */
            delay/=mpuTempoInterval;
        }
        else
        {
            uint32_t product=(uint32_t)delta*MUSIC_TICK_MULTIPLIER;
            if (!mpuTempoInterval || product/mpuTempoInterval>UINT16_MAX) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !mpuTempoInterval || product/mpuTempoInterval>UINT16_MAX");
            delay=(uint16_t)(product/mpuTempoInterval);
            while (shift--) delay>>=1;
        }
        if (!delay) continue;
        softwareMusicDelayTicks=delay;return;
    }
}

/*229C:00E2. Binary stream cursor wraps as a WORD. Known staging state is
 * ordinary typed data; malformed unchecked writes into other owners gate. */
uint8_t BtechDecodeNextMidiEvent(uint8_t inheritedDeltaHighByte)
{
nextTrack:
    if (!combatMidiStreamOffset)
    {
        midiRunningStatus=MIDI_IDLE;
        queuedMidiDeltaTicks=((uint16_t)inheritedDeltaHighByte<<8)|MIDI_IDLE;
        return inheritedDeltaHighByte;
    }
    uint16_t cursor=combatMidiStreamOffset;
    uint8_t deltaHighByte;
    for (;;)
    {
        uint8_t first=MwReadNativeViewByte(&midiStreamData,cursor++);
        uint16_t delta=first;
        if (first&MIDI_STATUS_FLAG)
            delta=((uint16_t)(first&0x7F)<<7)|MwReadNativeViewByte(&midiStreamData,cursor++);
        queuedMidiDeltaTicks=delta;deltaHighByte=(uint8_t)(delta>>8);
        uint8_t status=MwReadNativeViewByte(&midiStreamData,cursor);
        if (!(status&MIDI_STATUS_FLAG)) break; /* Running status retains this first data BYTE. */
        ++cursor;
        if (status!=MIDI_META)
        {
            midiRunningStatus=status;midiChannel=status&0x0F;
            midiMessageDataLength=midiMessageLengths[(status>>4)&7];
            deltaHighByte=0;break;
        }
        uint8_t meta=MwReadNativeViewByte(&midiStreamData,cursor);
        if (meta==MIDI_END_OF_TRACK)
        {
            BtechSoundEvent next=BtechAdvanceMusicSequence();
            combatMidiStreamOffset=next.offset;midiStreamData=next.data;
            midiStreamSelectorPresent=next.data.bytes!=NULL;
            ++midiTrackIndex;
            inheritedDeltaHighByte=(uint8_t)(next.offset>>8);goto nextTrack;
        }
        if (meta==MIDI_SET_TEMPO)
        {
            uint16_t tempo=MwReadNativeViewWord(&midiStreamData,(uint16_t)(cursor+2));
            midiTempoHighBytes=(tempo<<8)|(tempo>>8);
            if (!midiTempoHighBytes || MIDI_TEMPO_DIVIDEND/midiTempoHighBytes>UINT16_MAX) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !midiTempoHighBytes || MIDI_TEMPO_DIVIDEND/midiTempoHighBytes>UINT16_MAX");
            mpuTempoInterval=MIDI_TEMPO_DIVIDEND/midiTempoHighBytes;
            cursor+=5; /* Third tempo BYTE intentionally ignored. */
        }
        else
        {
            uint8_t firstLength=MwReadNativeViewByte(&midiStreamData,(uint16_t)(cursor+1));
            uint16_t payload=cursor+2,length=firstLength;
            if (firstLength&MIDI_STATUS_FLAG)
            {
                length=((uint16_t)(firstLength>>1)<<8)|MwReadNativeViewByte(&midiStreamData,(uint16_t)(cursor+2));
                ++payload; /* Native AND clears Carry: no OR80 on this length path. */
            }
            cursor=payload+length;
        }
    }
    /* Native LOOP with length0 writes10000 bytes. Do not silently turn it
     * into an empty message or overwrite unrelated host objects. */
    if (!midiMessageDataLength || midiMessageDataLength>sizeof(midiMessageData)) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !midiMessageDataLength || midiMessageDataLength>sizeof(midiMessageData)");
    for (uint16_t byte=0;byte<midiMessageDataLength;++byte)
        midiMessageData[byte]=MwReadNativeViewByte(&midiStreamData,cursor++);
    combatMidiStreamOffset=cursor;return deltaHighByte;
}
