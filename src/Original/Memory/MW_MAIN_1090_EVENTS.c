/* Installation-file decoding into typed campaign event data. No runtime
 * address registry, retained executable image or additional game procedure. */
#include "mechwarrior/campaign_events.h"
#include <string.h>

enum { EVENT_RECORD_START=0x6121, EVENT_RECORD_END=0x655C,
    EVENT_LIST_START=0x6073, EVENT_LIST_TERMINATOR=0x611F,
    EVENT_CAPTION_START=0x5CF4, EVENT_CAPTION_END=0x5E1C };
static bool ImportEventReference(const uint8_t *data,uint16_t offset,
    uint16_t *offsets,uint16_t *count,MwCampaignEvent **reference)
{
    if (!offset) {*reference=NULL;return true;} /* Stored zero branch, not an executable no-op. */
    for (uint16_t i=0;i<*count;++i)
        if (offsets[i]==offset) {*reference=&campaignEventRecords[i];return true;}
    if (offset<EVENT_RECORD_START || offset+2>=EVENT_RECORD_END || *count==CAMPAIGN_EVENT_RECORD_COUNT) return false;
    offsets[*count]=offset;
    MwCampaignEvent *event=&campaignEventRecords[(*count)++];
    event->eventId=data[offset];event->classCode=data[offset+1];event->script=NULL;
    *reference=event;return true;
}
static MwEventDate ImportEventDate(const uint8_t *bytes)
{
    MwEventDate date={bytes[0],bytes[1],bytes[2]};return date;
}
bool MwImportCampaignEventData(const uint8_t *data,size_t byteCount)
{
    if (!data || byteCount<NATIVE_SEGMENT_BYTE_COUNT) return false;
    /* The dispatch topology is native code, not editable scenario rules. Reject
     * an unsupported installation here rather than execute the wrong C handler. */
    const uint16_t conditions[10]={0x6425,0x643D,0x64BB,0x64ED,0x6503,0x652F,0x6561,0,0x6562,0x66CC};
    const uint16_t actions[10]={0x6587,0x6588,0x65B1,0x662C,0x662D,0x665B,0x6661,0x6667,0x6668,0x66CC};
    for (uint16_t i=0;i<10;++i)
        if (MwReadNativeWord(data,(uint16_t)(0x5E78+i*2))!=conditions[i]
            || MwReadNativeWord(data,(uint16_t)(0x5E8C+i*2))!=actions[i]) return false;
    uint16_t captionOffsets[CAMPAIGN_EVENT_CAPTION_COUNT];
    uint16_t cursor=EVENT_CAPTION_START;
    for (uint16_t i=0;i<CAMPAIGN_EVENT_CAPTION_COUNT;++i)
    {
        captionOffsets[i]=cursor;
        MwEventCaption *caption=&eventChoiceCaptions[i];
        caption->colour=data[cursor++];
        const uint8_t *end=memchr(data+cursor,0,EVENT_CAPTION_END-cursor);
        if (!end || (size_t)(end-(data+cursor))>=sizeof(caption->text)) return false;
        size_t length=(size_t)(end-(data+cursor))+1;
        memcpy(caption->text,data+cursor,length);cursor+=(uint16_t)length;
    }
    if (cursor!=EVENT_CAPTION_END) return false;
    for (uint16_t menuIndex=0;menuIndex<2;++menuIndex)
    {
        MwUiMenu *menu=menuIndex?&eventThreeChoiceMenu:&eventTwoChoiceMenu;
        uint16_t base=menuIndex?0x5E38:0x5E1C;
        for (uint16_t i=0;i<menu->recordCount;++i)
        {
            uint16_t at=base+i*CAMPAIGN_UI_RECORD_BYTES;
            MwUiRecord *record=&menu->records[i];
            record->enabled=data[at];record->textRecord=data[at+1];
            record->x=MwReadNativeWord(data,at+2);record->y=MwReadNativeWord(data,at+4);
            record->activateCodeOffset=MwReadNativeWord(data,at+6);
            record->pictureIndex=0;record->text=NULL;record->textColour=NULL;
            uint16_t captionOffset=MwReadNativeWord(data,at+8);
            for (uint16_t caption=0;caption<CAMPAIGN_EVENT_CAPTION_COUNT;++caption)
                if (captionOffsets[caption]==captionOffset)
                {
                    record->text=eventChoiceCaptions[caption].text;
                    record->textColour=&eventChoiceCaptions[caption].colour;
                }
            if (captionOffset && !record->text) return false;
            record->upRecordIndex=data[at+10];record->downRecordIndex=data[at+11];
            record->leftRecordIndex=data[at+12];record->rightRecordIndex=data[at+13];
        }
    }
    for (uint16_t i=0;i<3;++i) eventChoiceBranches[i]=NULL;
    for (uint16_t i=0;i<7;++i) eventClassHandlers[i]=i==3?MwShowClassThreeEventPages:MwShowCampaignEventText;
    campaignEventChoice=MwEventOpenChoiceContinuation;
    campaignEventPlanetSubstitution=MwEventSubstitutePlanetName;
    uint16_t recordOffsets[CAMPAIGN_EVENT_RECORD_COUNT],recordCount=0,commandCount=0;
    for (uint16_t i=0;i<CAMPAIGN_EVENT_LIST_ENTRIES;++i)
    {
        uint16_t offset=MwReadNativeWord(data,(uint16_t)(EVENT_LIST_START+i*2));
        if (!ImportEventReference(data,offset,recordOffsets,&recordCount,&eventListEntries[i])) return false;
        if ((i==CAMPAIGN_EVENT_LIST_ENTRIES-1)!=(offset==0)) return false;
    }
    for (uint16_t i=0;i<CAMPAIGN_EVENT_LIST_COUNT;++i)
    {
        uint16_t offset=MwReadNativeWord(data,(uint16_t)(0x6067+i*2));
        if (!offset) {eventLists[i]=NULL;continue;}
        if (offset<EVENT_LIST_START || offset>EVENT_LIST_TERMINATOR || (offset-EVENT_LIST_START)%2) return false;
        eventLists[i]=&eventListEntries[(offset-EVENT_LIST_START)/2];
    }
    if (!ImportEventReference(data,0x6353,recordOffsets,&recordCount,&fixedCampaignEvent)) return false;
    for (uint16_t record=0;record<recordCount;++record)
    {
        cursor=recordOffsets[record]+2;
        MwEventCommand **destination=&campaignEventRecords[record].script;
        bool action=false;
        while (data[cursor]!=EVENT_SCRIPT_END)
        {
            if (cursor>=EVENT_RECORD_END || commandCount==CAMPAIGN_EVENT_COMMAND_COUNT) return false;
            uint8_t opcode=data[cursor++];
            const uint8_t conditionSizes[10]={1,3,1,1,2,1,0,0,0,0};
            const uint8_t actionSizes[10]={0,0,6,0,4,0,0,0,1,0};
            if (opcode>=10) return false;
            uint16_t operandBytes=action?actionSizes[opcode]:conditionSizes[opcode];
            if (opcode==9)
            {
                uint8_t count=data[cursor];
                if (count!=2 && count!=3) return false; /* Supported shipped format, not a runtime choice-count guard. */
                operandBytes=(uint16_t)(count*4+1);
            }
            if (cursor+operandBytes>=EVENT_RECORD_END) return false;
            MwEventCommand *command=&campaignEventCommands[commandCount++];
            memset(command,0,sizeof(*command));command->opcode=opcode;
            *destination=command;destination=&command->next;
            if (opcode==9)
            {
                command->operand.choices.count=data[cursor];
                for (uint16_t i=0;i<command->operand.choices.count;++i)
                {
                    uint16_t captionOffset=MwReadNativeWord(data,(uint16_t)(cursor+1+i*2));
                    for (uint16_t caption=0;caption<CAMPAIGN_EVENT_CAPTION_COUNT;++caption)
                        if (captionOffsets[caption]==captionOffset) command->operand.choices.captions[i]=&eventChoiceCaptions[caption];
                    if (!command->operand.choices.captions[i]) return false;
                    uint16_t branchOffset=MwReadNativeWord(data,(uint16_t)(cursor+1+command->operand.choices.count*2+i*2));
                    if (!ImportEventReference(data,branchOffset,recordOffsets,&recordCount,&command->operand.choices.branches[i])) return false;
                }
            }
            else if (!action)
            {
                switch (opcode)
                {
                case 0:command->operand.randomThreshold=data[cursor];break;
                case 1:command->operand.date=ImportEventDate(data+cursor);break;
                case 2:case 5:command->operand.planetSelector=(int8_t)data[cursor];break;
                case 3:command->operand.seenEventId=data[cursor];break;
                case 4:command->operand.houseStanding.houseSelector=data[cursor];command->operand.houseStanding.thresholdHalf=(int8_t)data[cursor+1];break;
                default:break; /* Native no-operand table entries. */
                }
            }
            else
            {
                switch (opcode)
                {
                case 2:command->operand.dateWindow.start=ImportEventDate(data+cursor);command->operand.dateWindow.end=ImportEventDate(data+cursor+3);break;
                case 4:command->operand.cashDelta=(uint32_t)MwReadNativeWord(data,cursor)|((uint32_t)MwReadNativeWord(data,(uint16_t)(cursor+2))<<16);break;
                case 8:command->operand.planetSelector=(int8_t)data[cursor];break;
                default:break;
                }
            }
            cursor+=operandBytes;action=!action;
        }
        *destination=NULL; /* Decoded FF terminator, not an invented script return opcode. */
    }
    eventOperandCursor=NULL;
    return recordCount==CAMPAIGN_EVENT_RECORD_COUNT && commandCount==CAMPAIGN_EVENT_COMMAND_COUNT;
}
