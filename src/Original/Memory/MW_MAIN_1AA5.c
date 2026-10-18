#include "mechwarrior/campaign_events.h"
#include <string.h>

uint8_t eventTextStorage[EVENT_TEXT_STORAGE_END-EVENT_TEXT_STORAGE_START];
uint8_t *eventTextRecords[EVENT_TEXT_DIRECTORY_COUNT];

/* Original executable-data loading boundary. Only the authored writable text
 * bytes survive; the directory becomes references into their ONE owner. */
bool MwImportCampaignEventText(const uint8_t *data,size_t byteCount)
{
    if (!data || byteCount<EVENT_TEXT_STORAGE_END) return false;
    memcpy(eventTextStorage,data+EVENT_TEXT_STORAGE_START,sizeof(eventTextStorage));
    for (uint16_t i=0;i<EVENT_TEXT_DIRECTORY_COUNT;++i)
    {
        uint16_t offset=MwReadNativeWord(data,(uint16_t)(i*2));
        if (!offset) {eventTextRecords[i]=NULL;continue;}
        if (offset<EVENT_TEXT_STORAGE_START || offset>=EVENT_TEXT_STORAGE_END) return false;
        eventTextRecords[i]=&eventTextStorage[offset-EVENT_TEXT_STORAGE_START];
    }
    return true;
}
