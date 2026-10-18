#include "mechwarrior/campaign_music.h"
#include <stdlib.h>

/* Stored near-link representation, not game logic or a DS/address registry.
 * Bounds failures expose unrepresented native aliases, not game validation. */
MwMidiVoice *MwResolveMidiVoice(uint16_t nativeOffset)
{
    uint16_t relative=nativeOffset-MIDI_VOICES_OFFSET;
    if (relative%sizeof(MwMidiVoice) || relative/sizeof(MwMidiVoice)>=MIDI_VOICE_CAPACITY) abort();
    return &audioVoices[relative/sizeof(MwMidiVoice)];
}
MwMidiVoiceLinks *MwResolveMidiVoiceLinks(uint16_t nativeOffset)
{
    if (nativeOffset==MIDI_LIST_HEAD_OFFSET) return &audioVoiceList.fromHead.head;
    if (nativeOffset==MIDI_LIST_TAIL_OFFSET) return &audioVoiceList.fromTail.tail;
    return &MwResolveMidiVoice(nativeOffset)->links;
}
MwMidiChannel *MwResolveMidiChannel(uint16_t nativeOffset)
{
    if (nativeOffset==MIDI_LIST_HEAD_OFFSET) return &audioVoicePool.headChannelAlias;
    if (nativeOffset==MIDI_LIST_TAIL_OFFSET) return &audioVoicePool.tailChannelAlias.channel;
    uint16_t relative=nativeOffset-MIDI_CHANNELS_OFFSET;
    if (!(relative%sizeof(MwMidiChannel)) && relative/sizeof(MwMidiChannel)<MIDI_CHANNEL_RECORDS)
        return &audioChannels[relative/sizeof(MwMidiChannel)];
    if (nativeOffset==MIDI_DUMMY_CHANNEL_OFFSET) return &audioDummyChannel;
    /* Proven0DB2 alias: writer base can become a voice's first eight bytes. */
    return &MwResolveMidiVoice(nativeOffset)->channelAlias;
}
