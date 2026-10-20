#include "mechwarrior/campaign_music.h"
#include "mechwarrior/native_address.h"
#include <stdlib.h>

MwMusicHardwareCalls mwMusicHardware;

/* MW_MAIN0F41:003E..00B4. The host IRQ boundary owns CPU entry/IRET flags.
 * Valid original code references become typed functions, not a CS registry.
 * BIOS chain performs its own EOI. Only the coarse callback enables nesting. */
void MwCampaignTimerInterrupt(void)
{
    uint16_t divisorAndPhase=pitDivisorHigh|((uint16_t)timerPhase<<8);
    uint8_t previousPhase=divisorAndPhase>>8;
    timerPhase+=(uint8_t)divisorAndPhase;
    if (!(previousPhase&MUSIC_TIMER_PHASE_MASK))
    {
        uint16_t previousLow=timerClockLow;
        timerClockLow+=MUSIC_TIMER_CLOCK_INCREMENT;
        if (previousLow>UINT16_MAX-MUSIC_TIMER_CLOCK_INCREMENT) ++timerClockHigh;
    }
    if (!previousPhase)
    {
        if (!timerCallbackSlots[0]) abort(); /* Unmapped FAR tail, never a no-op. */
        timerCallbackSlots[0]();
        return;
    }
    if (!mwMusicHardware.writePort) abort();
    mwMusicHardware.writePort(PIC_COMMAND_PORT,PIC_END_OF_INTERRUPT);
    if (!(previousPhase&MUSIC_TIMER_PHASE_MASK))
    {
        if (!mwMusicHardware.setInterruptsEnabled) abort();
        mwMusicHardware.setInterruptsEnabled(true);
        uint16_t slot=(previousPhase&MUSIC_TIMER_COARSE_MASK)>>5;
        if (timerCallbackSlots[slot]) timerCallbackSlots[slot]();
    }
    else
    {
        /* Native fast path attempts even0000:0000. That unmapped code target
         * is unsupported here; it is NOT a successful empty callback. */
        if (!timerCallbackSlots[MUSIC_TIMER_FAST_SLOT]) abort();
        timerCallbackSlots[MUSIC_TIMER_FAST_SLOT]();
    }
}

/* MW_MAIN0F41:00B5..00F9. Original DOS vector operations project to typed
 * handler references; their numeric identities are not consumed by this slice. */
void MwInstallMusicTimer(void)
{
    if (!mwMusicHardware.setInterruptsEnabled || !mwMusicHardware.readIrq0Handler
        || !mwMusicHardware.writeIrq0Handler || !mwMusicHardware.writePort) abort();
    mwMusicHardware.setInterruptsEnabled(false);
    for (uint16_t slot=0;slot<MUSIC_TIMER_CALLBACK_COUNT;++slot) timerCallbackSlots[slot]=NULL;
    timerCallbackSlots[0]=mwMusicHardware.readIrq0Handler();
    mwMusicHardware.writeIrq0Handler(MwCampaignTimerInterrupt);
    mwMusicHardware.writePort(PIT_CONTROL_PORT,PIT_CHANNEL0_SQUARE_WAVE);
    mwMusicHardware.writePort(PIT_CHANNEL0_PORT,0);
    timerPhase=0;
    timerClockLow=0;
    timerClockHigh=0;
    mwMusicHardware.writePort(PIT_CHANNEL0_PORT,MUSIC_TIMER_BASE_DIVISOR_HIGH);
    pitDivisorHigh=MUSIC_TIMER_BASE_DIVISOR_HIGH;
    mwMusicHardware.setInterruptsEnabled(true);
}

/* MW_MAIN0F41:00FA..0117. Restore writable slot0, not a second saved vector. */
void MwRestoreBiosTimer(void)
{
    if (!mwMusicHardware.setInterruptsEnabled || !mwMusicHardware.writeIrq0Handler
        || !mwMusicHardware.writePort) abort();
    mwMusicHardware.setInterruptsEnabled(false);
    mwMusicHardware.writeIrq0Handler(timerCallbackSlots[0]);
    mwMusicHardware.writePort(PIT_CONTROL_PORT,PIT_CHANNEL0_SQUARE_WAVE);
    mwMusicHardware.writePort(PIT_CHANNEL0_PORT,0);
    mwMusicHardware.writePort(PIT_CHANNEL0_PORT,0);
    mwMusicHardware.setInterruptsEnabled(true);
}

/* MW_MAIN0F41:0139..018F. Native AX is unused by actual callers. Rate is read
 * only for nonnull slot8; null calls really push only three WORDs. */
void MwTimerSetCallback(uint16_t slot,MwMusicTimerHandler handler,uint16_t rateShift)
{
    if (slot>=MUSIC_TIMER_CALLBACK_COUNT) return;
    if (!mwMusicHardware.setInterruptsEnabled || !mwMusicHardware.writePort) abort();
    mwMusicHardware.setInterruptsEnabled(false);
    timerCallbackSlots[slot]=handler;
    if (slot==MUSIC_TIMER_FAST_SLOT)
    {
        timerPhase=(uint8_t)(timerPhase+MUSIC_TIMER_PHASE_MASK)&MUSIC_TIMER_COARSE_MASK;
        uint8_t divisorHigh=MUSIC_TIMER_BASE_DIVISOR_HIGH;
        if (handler && rateShift && rateShift<=MUSIC_TIMER_MAX_RATE_SHIFT) divisorHigh>>=rateShift;
        pitDivisorHigh=divisorHigh;
        mwMusicHardware.writePort(PIT_CONTROL_PORT,PIT_CHANNEL0_SQUARE_WAVE);
        mwMusicHardware.writePort(PIT_CHANNEL0_PORT,0);
        mwMusicHardware.writePort(PIT_CHANNEL0_PORT,divisorHigh);
    }
    mwMusicHardware.setInterruptsEnabled(true);
}

/* MW_MAIN0F41:019A..01FF. Native unsigned shift/subtract division returns0
 * for zero divisor, unlike the actual DIV instructions elsewhere below. */
uint32_t MwDivideMidiTimingRatio(uint32_t numerator,uint32_t denominator)
{
    if (!numerator || !denominator) return 0;
    return numerator/denominator;
}

/* MW_MAIN0F41:022F..0244. STI is unconditional, not restoration of old IF. */
uint16_t MwSetMidiTickDenominator(uint16_t denominator)
{
    if (denominator)
    {
        if (!mwMusicHardware.setInterruptsEnabled) abort();
        mwMusicHardware.setInterruptsEnabled(false);
        midiTickDenominator=denominator;
        midiTickRemainder=0;
        mwMusicHardware.setInterruptsEnabled(true);
    }
    return 0;
}

/* MW_MAIN0F41:0245..0259. MUL's low WORD becomes the timing numerator. */
void MwSetMidiTickNumerator(uint16_t division,uint16_t tempo)
{
    uint16_t numerator=(uint32_t)division*tempo;
    if (!mwMusicHardware.setInterruptsEnabled) abort();
    mwMusicHardware.setInterruptsEnabled(false);
    midiTickNumerator=numerator;
    midiTickRemainder=0;
    mwMusicHardware.setInterruptsEnabled(true);
}

/* MW_MAIN0F41:025A..0274. The bar store survives a subsequent native DIV
 * fault. abort is the current fatal CPU-fault boundary, not a successful RET. */
void MwSetMidiBeatBarDivisions(uint16_t beatScale,uint16_t denominatorScale,uint16_t barDivision)
{
    midiBarDivision=barDivision;
    uint16_t divisor=(uint32_t)barDivision*denominatorScale;
    uint32_t scaledBeat=(uint32_t)(MIDI_TICKS_PER_QUARTER*4)*beatScale;
    if (!divisor || scaledBeat/divisor>UINT16_MAX) abort();
    midiBeatDivision=(uint16_t)(scaledBeat/divisor);
}

/* MW_MAIN0F41:0275..02A0. Original LOOP emits channels15 down to0. */
void MwMidiAllNotesOff(void)
{
    uint16_t channelsRemaining=MIDI_CHANNEL_COUNT;
    do
    {
        MwDispatchMidiByte(MIDI_CONTROL_CHANGE-1+channelsRemaining);
        MwDispatchMidiByte(MIDI_CONTROL_ALL_NOTES_OFF);
        MwDispatchMidiByte(0);
    } while (--channelsRemaining);
}

/* MW_MAIN0F41:02A1..02B7. Real serialized SMF bytes, wrapping ES:SI cursor.
 * Native MOV DH,DL/SHR DX drops bit31; there is no four-byte VLQ limit. */
uint32_t MwReadMidiVariableLengthQuantity(uint16_t resourceSelector,uint16_t *streamOffset)
{
    const uint8_t *stream=MwResolveNativeSegment(resourceSelector);
    uint32_t quantity=0;
    uint8_t encoded;
    do
    {
        encoded=stream[(*streamOffset)++];
        quantity=((quantity<<7)|(encoded&0x7F))&0x7FFFFFFF;
    } while (encoded&MIDI_STATUS_BYTE);
    /* Native DX is the high WORD of the returned quantity. The AdLib rhythm
     * release callback observably inherits it before another port selection. */
    midiAdlibOutputPort=(uint16_t)(quantity>>16);
    return quantity;
}

/* MW_MAIN0F41:02B8..0350. Resource selector/offset project one native FAR
 * pointer. Header length HIGH and track length are skipped, not validated. */
uint16_t MwStartMidiTrack(uint16_t resourceSelector,uint16_t streamOffset,uint16_t playbackFlags)
{
    enum { MIDI_HEADER_FIRST_WORD=0x544D, MIDI_HEADER_SECOND_WORD=0x6468,
        MIDI_TRACK_SECOND_WORD=0x6B72, MIDI_ENCODED_ONE_TRACK=0x0100,
        MIDI_ENCODED_96_PPQN=0x6000 };
    if (!(playbackFlags&MIDI_TRACK_ACTIVE)) return 0;
    const uint8_t *stream=MwResolveNativeSegment(resourceSelector);
    if (MwReadNativeWord(stream,streamOffset)!=MIDI_HEADER_FIRST_WORD) return 0;
    streamOffset+=2;
    if (MwReadNativeWord(stream,streamOffset)!=MIDI_HEADER_SECOND_WORD) return 0;
    streamOffset+=4;
    uint16_t headerLength=MwReadNativeWord(stream,streamOffset);
    headerLength=(uint16_t)((headerLength<<8)|(headerLength>>8));
    streamOffset+=2;
    if (MwReadNativeWord(stream,streamOffset)!=0
        || MwReadNativeWord(stream,(uint16_t)(streamOffset+2))!=MIDI_ENCODED_ONE_TRACK
        || MwReadNativeWord(stream,(uint16_t)(streamOffset+4))!=MIDI_ENCODED_96_PPQN) return 0;
    streamOffset+=headerLength;
    if (MwReadNativeWord(stream,streamOffset)!=MIDI_HEADER_FIRST_WORD) return 0;
    streamOffset+=2;
    if (MwReadNativeWord(stream,streamOffset)!=MIDI_TRACK_SECOND_WORD) return 0;
    streamOffset+=6;
    midiStateUpdateInProgress=1;
    if (midiPlaybackFlags&MIDI_TRACK_ACTIVE) MwMidiAllNotesOff();
    midiPlaybackFlags=0;
    midiEventTickBalance=(uint16_t)MwReadMidiVariableLengthQuantity(resourceSelector,&streamOffset);
    midiStreamOffset=streamOffset;
    midiStreamSegment=resourceSelector;
    midiPlaybackFlags=playbackFlags;
    midiBeatRemainder=0;
    midiBarRemainder=0;
    midiElapsedBars=0;
    midiStateUpdateInProgress=0;
    return playbackFlags;
}

/* MW_MAIN0F41:0351..036E. No conventional note retrigger on resume. */
void MwPauseMidiTrack(void)
{
    if ((midiPlaybackFlags&(MIDI_TRACK_ACTIVE|MIDI_TRACK_PAUSED))==MIDI_TRACK_ACTIVE)
    {
        midiStateUpdateInProgress=1;
        midiPlaybackFlags|=MIDI_TRACK_PAUSED;
        MwMidiAllNotesOff();
        midiStateUpdateInProgress=0;
    }
}
/* MW_MAIN0F41:036F..0389. */
void MwResumeMidiTrack(void)
{
    if ((midiPlaybackFlags&(MIDI_TRACK_ACTIVE|MIDI_TRACK_PAUSED))==(MIDI_TRACK_ACTIVE|MIDI_TRACK_PAUSED))
    {
        midiStateUpdateInProgress=1;
        midiPlaybackFlags&=(uint16_t)~MIDI_TRACK_PAUSED;
        midiStateUpdateInProgress=0;
    }
}
/* MW_MAIN0F41:038A..03A7. Paused is deliberately absent from this mask. */
void MwMuteMidiTrack(void)
{
    if ((midiPlaybackFlags&(MIDI_TRACK_ACTIVE|MIDI_TRACK_MUTED))==MIDI_TRACK_ACTIVE)
    {
        midiStateUpdateInProgress=1;
        midiPlaybackFlags|=MIDI_TRACK_MUTED;
        MwMidiAllNotesOff();
        midiStateUpdateInProgress=0;
    }
}
/* MW_MAIN0F41:03A8..03C2. */
void MwUnmuteMidiTrack(void)
{
    if ((midiPlaybackFlags&(MIDI_TRACK_ACTIVE|MIDI_TRACK_MUTED))==(MIDI_TRACK_ACTIVE|MIDI_TRACK_MUTED))
    {
        midiStateUpdateInProgress=1;
        midiPlaybackFlags&=(uint16_t)~MIDI_TRACK_MUTED;
        midiStateUpdateInProgress=0;
    }
}
/* MW_MAIN0F41:03C3..03DE. Inactive is a genuine native no-op. */
void MwStopMidiTrack(void)
{
    if (midiPlaybackFlags&MIDI_TRACK_ACTIVE)
    {
        midiStateUpdateInProgress=1;
        midiPlaybackFlags=0;
        MwMidiAllNotesOff();
        midiStateUpdateInProgress=0;
    }
}

/* MW_MAIN0F41:03DF..0573. Shared NEAR targets stay inside this original FAR
 * callback. Code-offset tables remain writable. Native guards compare EXACT1;
 * DIV failures retain prior stores/guard, and zero-delta processing is unbounded. */
void MwTickMidiTrack(void)
{
    if (midiTickInProgress==1) return;
    midiTickInProgress=1;
    if ((midiPlaybackFlags&(MIDI_TRACK_ACTIVE|MIDI_TRACK_PAUSED))!=MIDI_TRACK_ACTIVE) goto tickFinished;
    uint16_t tickUnits=midiTickRemainder+midiTickNumerator;
    uint16_t tickDenominator=midiTickDenominator;
    if (!tickDenominator) abort();
    midiTickRemainder=tickUnits%tickDenominator;
    midiEventTickBalance+=tickUnits/tickDenominator;
    if (midiStateUpdateInProgress==1) goto tickFinished;
    uint16_t streamOffset=midiStreamOffset;
    uint16_t resourceSelector=midiStreamSegment;
    const uint8_t *stream=MwResolveNativeSegment(resourceSelector);
    uint16_t status,handler,payloadLength;
nextEvent:
    if (midiEventTickBalance&0x8000) goto saveStream;
    status=stream[streamOffset++];
    if (status<MIDI_STATUS_BYTE) {status=midiTrackRunningStatus;--streamOffset;}
    if (status<MIDI_SYSTEM_STATUS)
    {
        midiTrackRunningStatus=(uint8_t)status;
        handler=midiTrackChannelHandlers[(status&0x70)>>4];
    }
    else
    {
        if (status<MIDI_REALTIME_STATUS) midiTrackRunningStatus=0;
        handler=midiTrackSystemHandlers[status&0x0F];
    }
    switch (handler)
    {
    case MIDI_FORWARD_SYSEX_STATUS: goto forwardSysexStatus;
    case MIDI_FORWARD_SYSEX_PAYLOAD: goto forwardSysexPayload;
    case MIDI_META_EVENT: goto metaEvent;
    case MIDI_READ_NEXT_DELTA: goto readNextDelta;
    case MIDI_MUTED_NOTE_OR_TWO_DATA: goto mutedNoteOrTwoData;
    case MIDI_FORWARD_STATUS_TWO_DATA: goto forwardStatusAndTwoData;
    case MIDI_FORWARD_STATUS_ONE_DATA: goto forwardStatusAndOneData;
    case MIDI_FORWARD_CURRENT_BYTE: goto forwardCurrentByte;
    default: abort(); /* Unmapped writable code target, not a successful event. */
    }
forwardSysexStatus:
    MwDispatchMidiByte(status);
forwardSysexPayload:
    payloadLength=(uint16_t)MwReadMidiVariableLengthQuantity(resourceSelector,&streamOffset);
    do
    {
        uint8_t midiByte=stream[streamOffset++];
        MwDispatchMidiByte(midiByte);
    } while (--payloadLength); /* Zero starts a full65536-byte native LOOP. */
    goto readNextDelta;
metaEvent: ;
    uint8_t metaType=stream[streamOffset++];
    payloadLength=(uint16_t)MwReadMidiVariableLengthQuantity(resourceSelector,&streamOffset);
    if (metaType==MIDI_META_END_TRACK)
    {
        MwStopMidiTrack();
        goto saveStream; /* No payload skip at end-of-track. */
    }
    if (metaType==MIDI_META_TEMPO)
    {
        uint32_t microseconds=(uint32_t)stream[streamOffset]<<16;
        uint16_t tempoLow=MwReadNativeWord(stream,(uint16_t)(streamOffset+1));
        microseconds|=(uint16_t)((tempoLow<<8)|(tempoLow>>8));
        enum { MICROSECONDS_PER_MINUTE=60000000 };
        uint16_t beatsPerMinute=(uint16_t)MwDivideMidiTimingRatio(MICROSECONDS_PER_MINUTE,microseconds);
        MwSetMidiTickNumerator(MIDI_TICKS_PER_QUARTER,beatsPerMinute);
    }
    else if (metaType==MIDI_META_TIME_SIGNATURE)
    {
        uint8_t numerator=stream[streamOffset];
        uint16_t signature=MwReadNativeWord(stream,(uint16_t)(streamOffset+1));
        uint8_t exponent=signature;
        uint8_t denominator=exponent<8 ? 1u<<exponent : 0;
        uint16_t signatureDivisor=(uint16_t)denominator*(uint8_t)(signature>>8);
        uint16_t signatureUnits=MIDI_TICKS_PER_QUARTER*numerator;
        if (!signatureDivisor) abort();
        uint16_t quotient=signatureUnits/signatureDivisor;
        uint16_t remainder=signatureUnits%signatureDivisor;
        MwSetMidiBeatBarDivisions((remainder&0xFF00)|numerator,
            (remainder&0xFF00)|denominator,quotient); /* Native MOV DL preserves DH. */
    }
    streamOffset+=payloadLength;
    goto readNextDelta;
mutedNoteOrTwoData:
    if (midiPlaybackFlags&MIDI_TRACK_MUTED) {streamOffset+=2;goto readNextDelta;}
forwardStatusAndTwoData:
    MwDispatchMidiByte(status);
    status=stream[streamOffset++];
forwardStatusAndOneData:
    MwDispatchMidiByte(status);
    status=stream[streamOffset++];
forwardCurrentByte:
    MwDispatchMidiByte(status);
readNextDelta: ;
    uint16_t nextDelta=(uint16_t)MwReadMidiVariableLengthQuantity(resourceSelector,&streamOffset);
    if (nextDelta)
    {
        midiEventTickBalance-=nextDelta;
        if (!(midiPlaybackFlags&MIDI_TRACK_SKIP_METRICS))
        {
            uint16_t beatUnits=nextDelta+midiBeatRemainder;
            uint16_t beatDivision=midiBeatDivision;
            if (!beatDivision) abort();
            midiBeatRemainder=beatUnits%beatDivision;
            uint16_t barUnits=beatUnits/beatDivision+midiBarRemainder;
            uint16_t barDivision=midiBarDivision;
            if (!barDivision) abort();
            midiBarRemainder=barUnits%barDivision;
            midiElapsedBars+=barUnits/barDivision;
        }
    }
    goto nextEvent;
saveStream:
    midiStreamOffset=streamOffset;
tickFinished:
    midiTickInProgress=0;
}
