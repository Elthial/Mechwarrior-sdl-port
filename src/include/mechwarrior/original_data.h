#ifndef MECHWARRIOR_ORIGINAL_DATA_H
#define MECHWARRIOR_ORIGINAL_DATA_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Installation-file decoding, not native game procedures or a live DS image.
 * Input storage is borrowed only for the duration of each call. */
bool MwImportCampaignInitialData(const uint8_t *data, size_t byteCount);
bool MwHostLoadCampaignInitialData(const char *executablePath);
bool MwHostLoadEgaInitialData(const char *executablePath);
bool MwHostLoadCampaignEventText(const char *executablePath);
bool MwImportCampaignMusicData(const uint8_t *data,size_t byteCount);
bool MwHostLoadCampaignMusicData(const char *executablePath);
#endif
