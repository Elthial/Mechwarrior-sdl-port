/* MW_MAIN CS0F41 writable timer state. Each byte has one canonical owner;
 * composing the003C WORD snapshot does not introduce another live copy. */
#include "mechwarrior/campaign_music.h"
uint16_t timerClockLow,timerClockHigh;
MwMusicTimerHandler volatile timerCallbackSlots[MUSIC_TIMER_CALLBACK_COUNT];
uint8_t pitDivisorHigh,timerPhase;
