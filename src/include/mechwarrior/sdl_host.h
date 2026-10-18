#ifndef MECHWARRIOR_SDL_HOST_H
#define MECHWARRIOR_SDL_HOST_H
#include <stdbool.h>
#include <stdint.h>
/* Host boundary only. Original headers/procedures never depend on SDL. */
bool MwSdlInitialize(const char *gameDirectory,const char *captureDirectory);
int MwSdlRunCampaign(void);
void MwSdlShutdown(void);
void MwSdlPump(void);
bool MwSdlCapture(const char *stem);
void MwSdlRequestQuit(void);
/* Observers run after real SDL output, never as alternate game flow. */
extern void (*mwSdlFrameObserver)(void);
extern void (*mwSdlAudioObserver)(const float *samples,uint32_t count);
#endif
