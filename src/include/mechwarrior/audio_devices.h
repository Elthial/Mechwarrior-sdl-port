#ifndef MECHWARRIOR_AUDIO_DEVICES_H
#define MECHWARRIOR_AUDIO_DEVICES_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Hardware synthesis only. The original game owns scores, instruments and ticks. */
bool MwMt32Open(const char *controlRom,const char *pcmRom,uint32_t sampleRate,char *error,size_t errorSize);
void MwMt32Close(void);
void MwMt32ReceiveByte(uint8_t value);
void MwMt32Render(float *stereo,uint32_t frames);
#endif
