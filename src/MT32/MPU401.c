#include "../SDL/SDLBackend.h"
#include "mechwarrior/audio_devices.h"
#include <float.h>
#include <string.h>

/* Hardware protocol adapter, not a MIDI player. Campaign uses UART; BTECH's
 * original IRQ/parser supplies intelligent-mode events. Only the command
 * subset used by these drivers is supported, not full MPU-401 firmware. */
enum { MPU_INPUT_EMPTY=0x80,MPU_OUTPUT_BUSY=0x40,MPU_ACK=0xFE,
    MPU_RESET_COMMAND=0xFF,MPU_UART_COMMAND=0x3F,
    MIDI_BITS_PER_BYTE=10,MIDI_BAUD=31250,MPU_REPLY_CAPACITY=32,
    MPU_VERSION_COMMAND=0xAC,MPU_SEND_SYSTEM=0xDF,MPU_SET_TEMPO=0xE0,
    MPU_ACTIVE_TRACKS=0xEC,MPU_CHANNELS_LOW=0xEE,MPU_CHANNELS_HIGH=0xEF,
    MPU_CONDUCTOR_ENABLE=0x8F,MPU_PLAY=0x0A,MPU_STOP=5,
    MPU_CLEAR_COUNTERS=0xB8,MPU_CLEAR_MAP=0xB9,MPU_TRACK_REQUEST=0xF0,
    MPU_CONDUCTOR_REQUEST=0xF9,MPU_OVERFLOW=0xF8,MPU_END=0xFC,
    MIDI_SYSEX_END=0xF7,MIDI_SONG_POSITION=0xF2,MIDI_SONG_SELECT=0xF3,
    MIDI_TUNE_REQUEST=0xF6,MPU_DIRECT_MESSAGE_FIRST=0xD0,MPU_DIRECT_MESSAGE_LAST=0xD7,
    MPU_METRONOME_RATE=0xE4,MPU_MEASURE_LENGTH=0xE6,MPU_HOST_CLOCK_RATE=0xE7,
    MPU_TIMEBASE_FIRST=0xC2,MPU_TIMEBASE_LAST=0xC8,MPU_TIMEBASE_COMMAND_BASE=0xC0,
    MPU_TIMEBASE_STEP=24,MPU_VERSION=0x15 };
static uint8_t replies[MPU_REPLY_CAPACITY];
static unsigned replyRead,replyCount;
static bool uartMode;
static double transmitReady;
static uint8_t commandData,directBytes;
static uint8_t directChannel,systemBytes;
static bool directSystem,systemStarted;
typedef struct MpuTrack {
    uint16_t delay;
    uint8_t message[3],length,received,runningStatus;
    bool ready,waiting;
} MpuTrack;
static MpuTrack tracks[8];
static uint8_t trackMask,activeTracks,selectedTrack,timebase,tempo;
static uint8_t metronome,measureLength,hostClockRate;
static uint16_t channelMask;
static bool playing,conductorEnabled,conductorActive,conductorWaiting;
static uint16_t conductorDelay;
static uint8_t conductorCommand,conductorData,inputStage;
static bool conductorInput;
static double nextTick,immediateEvent;

static void RequestTrack(void);
static void QueueReply(uint8_t value);
static double TickDuration(void) {return (double)HOST_PIT_CLOCK_HZ*60/((double)tempo*timebase);}

void MwMpuReset(void)
{
    replyRead=replyCount=0;uartMode=false;transmitReady=0;commandData=directBytes=0;directSystem=false;
    directChannel=systemBytes=0;systemStarted=false;
    memset(tracks,0,sizeof(tracks));trackMask=activeTracks=selectedTrack=inputStage=0;
    channelMask=UINT16_MAX;timebase=120;tempo=100;
    metronome=measureLength=hostClockRate=0;
    playing=conductorEnabled=conductorActive=conductorWaiting=conductorInput=false;
    conductorDelay=0;conductorCommand=conductorData=0;nextTick=immediateEvent=DBL_MAX;
}
static void QueueReply(uint8_t value)
{
    if (replyCount==MPU_REPLY_CAPACITY) MwSdlFail("MPU response FIFO overflow");
    replies[(replyRead+replyCount++)%MPU_REPLY_CAPACITY]=value;
}
/* Intelligent-mode hardware requests one event at a time. The game IRQ owns
 * decoding; this card buffer owns only its time tag and MIDI transmission. */
static void RequestTrack(void)
{
    if (replyCount || !playing) return;
    for (unsigned track=0;track<8;++track)
        if (tracks[track].waiting) {QueueReply((uint8_t)(MPU_TRACK_REQUEST+track));return;}
    if (conductorWaiting) QueueReply(MPU_CONDUCTOR_REQUEST);
}
static void TransmitTrack(unsigned index)
{
    MpuTrack *track=&tracks[index];
    if (track->ready)
    {
        if (track->message[0]==MPU_END) activeTracks&=(uint8_t)~(1u<<index);
        else if (track->message[0]<MPU_TRACK_REQUEST && (channelMask&(1u<<(track->message[0]&15))))
            for (unsigned byte=0;byte<track->length;++byte) MwMt32ReceiveByte(track->message[byte]);
    }
    track->ready=false;track->waiting=(activeTracks&(1u<<index))!=0;
    track->delay=0xF0;
}
double MwMpuNextEventTime(void) {return nextTick<immediateEvent?nextTick:immediateEvent;}
void MwMpuAdvance(double time)
{
    if (uartMode || !playing) return;
    if (immediateEvent<=time)
    {
        immediateEvent=DBL_MAX;
        for (unsigned index=0;index<8;++index)
            if (tracks[index].ready && !tracks[index].delay) TransmitTrack(index);
        RequestTrack();
    }
    if (nextTick>time) return;
    if (!replyCount)
    {
        for (unsigned index=0;index<8;++index)
            if ((activeTracks&(1u<<index)) && !tracks[index].waiting
                && (!tracks[index].delay || !--tracks[index].delay)) TransmitTrack(index);
        if (conductorActive && !conductorWaiting && (!conductorDelay || !--conductorDelay))
        {
            if (conductorCommand==MPU_SET_TEMPO) tempo=conductorData<4?4:conductorData>250?250:conductorData;
            conductorCommand=0;conductorWaiting=true;conductorDelay=0xF0;
        }
        RequestTrack();
    }
    nextTick=time+TickDuration();
}
void MwMpuWritePort(uint16_t port,uint8_t value)
{
    if (port==MPU_STATUS_COMMAND_PORT)
    {
        if (value==MPU_RESET_COMMAND) MwMpuReset();
        else if (value==MPU_UART_COMMAND)
        {
            uartMode=true;playing=false;nextTick=immediateEvent=DBL_MAX;
            directBytes=commandData=0;directSystem=false;
        }
        else if (value==MPU_VERSION_COMMAND) {QueueReply(MPU_ACK);QueueReply(MPU_VERSION);return;}
        else if (value==MPU_SEND_SYSTEM) {directSystem=true;directBytes=0;systemStarted=false;systemBytes=0;}
        else if (value>=MPU_DIRECT_MESSAGE_FIRST && value<=MPU_DIRECT_MESSAGE_LAST)
        {directSystem=false;directBytes=UINT8_MAX;directChannel=value&7;}
        else if (value==MPU_SET_TEMPO || value==MPU_CHANNELS_LOW || value==MPU_CHANNELS_HIGH || value==MPU_METRONOME_RATE
            || value==MPU_MEASURE_LENGTH || value==MPU_HOST_CLOCK_RATE || value==MPU_ACTIVE_TRACKS) commandData=value;
        else if (value>=MPU_TIMEBASE_FIRST && value<=MPU_TIMEBASE_LAST)
            timebase=(uint8_t)((value-MPU_TIMEBASE_COMMAND_BASE)*MPU_TIMEBASE_STEP);
        else if (value==MPU_CONDUCTOR_ENABLE) conductorEnabled=true;
        else if (value==MPU_PLAY)
        {playing=true;nextTick=MwSdlHardwareTime()+TickDuration();replyRead=replyCount=0;}
        else if (value==MPU_STOP || value==MPU_CLEAR_COUNTERS || value==MPU_CLEAR_MAP)
        {
            for (uint8_t channel=0;channel<15;++channel)
            {MwMt32ReceiveByte(0xB0|channel);MwMt32ReceiveByte(0x7B);MwMt32ReceiveByte(0);}
            if (value==MPU_STOP) {playing=false;nextTick=immediateEvent=DBL_MAX;}
            else
            {
                memset(tracks,0,sizeof(tracks));activeTracks=trackMask;
                conductorActive=conductorEnabled;conductorDelay=0;conductorCommand=0;conductorWaiting=false;
            }
        }
        else if (value!=0) MwSdlFail("Unimplemented intelligent-mode MPU playback command");
        QueueReply(MPU_ACK);
        return;
    }
    if (uartMode)
    {
        transmitReady=MwSdlHardwareTime()+(double)HOST_PIT_CLOCK_HZ*MIDI_BITS_PER_BYTE/MIDI_BAUD;
        MwMt32ReceiveByte(value);return;
    }
    if (commandData)
    {
        if (commandData==MPU_SET_TEMPO) tempo=value<4?4:value>250?250:value;
        else if (commandData==MPU_ACTIVE_TRACKS) trackMask=value;
        else if (commandData==MPU_CHANNELS_LOW) channelMask=(channelMask&0xFF00)|value;
        else if (commandData==MPU_CHANNELS_HIGH) channelMask=(channelMask&0x00FF)|((uint16_t)value<<8);
        else if (commandData==MPU_METRONOME_RATE) metronome=value;
        else if (commandData==MPU_MEASURE_LENGTH) measureLength=value;
        else if (commandData==MPU_HOST_CLOCK_RATE) hostClockRate=value>>2;
        /* Metronome and clock-to-host interval are stored by the interface but
         * BTECH never enables their output. They produce no synth MIDI. */
        commandData=0;return;
    }
    if (directSystem)
    {
        if (value==MIDI_SYSEX_END) {MwMt32ReceiveByte(value);directSystem=false;return;}
        if (!systemStarted)
        {
            systemStarted=true;
            systemBytes=value==MIDI_SONG_POSITION?3:value==MIDI_SONG_SELECT?2:value==MIDI_TUNE_REQUEST?1:0;
        }
        MwMt32ReceiveByte(value);
        if (systemBytes && !--systemBytes) directSystem=false;
        return;
    }
    if (directBytes)
    {
        if (directBytes==UINT8_MAX)
        {
            if (value>=MPU_TRACK_REQUEST) {directBytes=0;return;} /* Illegal system byte in channel-send mode. */
            if (value&0x80) tracks[directChannel].runningStatus=value;
            else
            {
                uint8_t status=tracks[directChannel].runningStatus;
                if (!status) {directBytes=0;return;}
                MwMt32ReceiveByte(status);
            }
            directBytes=(tracks[directChannel].runningStatus&0xE0)==0xC0?2:3;
            if (!(value&0x80)) --directBytes; /* Re-emitted status consumed one BYTE. */
        }
        MwMt32ReceiveByte(value);--directBytes;return;
    }
    if (!playing) MwSdlFail("MPU data without command or playback request");
    if (conductorInput)
    {
        if (!inputStage) {conductorDelay=value;inputStage=1;return;}
        if (inputStage==1) {conductorCommand=value;inputStage=2;return;}
        conductorData=value;conductorWaiting=false;inputStage=0;conductorInput=false;RequestTrack();return;
    }
    MpuTrack *track=&tracks[selectedTrack];
    if (!inputStage) {track->delay=value;track->received=0;inputStage=1;return;}
    if (!track->received)
    {
        if (value>=0xF0) track->length=1;
        else if (value&0x80)
        {track->runningStatus=value;track->length=(value&0xE0)==0xC0?2:3;}
        else
        {
            if (!track->runningStatus) MwSdlFail("MPU playback running status without status");
            track->message[track->received++]=track->runningStatus;
            track->length=(track->runningStatus&0xE0)==0xC0?2:3;
        }
    }
    if (track->received>=sizeof(track->message)) MwSdlFail("MPU track message overflow");
    track->message[track->received++]=value;
    if (track->received==track->length)
    {
        track->ready=true;track->waiting=false;inputStage=0;
        if (!track->delay) immediateEvent=MwSdlHardwareTime()+(double)HOST_PIT_CLOCK_HZ*0.000060;
        RequestTrack();
    }
}
uint8_t MwMpuReadPort(uint16_t port)
{
    if (port==MPU_STATUS_COMMAND_PORT)
        return (replyCount?0:MPU_INPUT_EMPTY)|(uartMode && MwSdlHardwareTime()<transmitReady?MPU_OUTPUT_BUSY:0);
    if (!replyCount) return UINT8_MAX;
    uint8_t value=replies[replyRead];replyRead=(replyRead+1)%MPU_REPLY_CAPACITY;--replyCount;
    if (!uartMode && value>=MPU_TRACK_REQUEST && value<MPU_OVERFLOW)
    {selectedTrack=value&7;inputStage=0;conductorInput=false;}
    else if (!uartMode && value==MPU_CONDUCTOR_REQUEST) {conductorInput=true;inputStage=0;}
    return value;
}
