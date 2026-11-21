#ifndef MECHWARRIOR_DOS_FILES_H
#define MECHWARRIOR_DOS_FILES_H
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

/* INT21 file boundary: true means Carry set; AX output is count/handle/error. */
typedef struct MwDosFileCalls
{
    bool (*openReadWrite)(const char *filename, uint16_t *handleOrError);
    bool (*read)(uint8_t *buffer, uint16_t byteCount, uint16_t fileHandle, uint16_t *bytesReadOrError);
    bool (*close)(uint16_t fileHandle);
    bool (*seekAbsolute)(uint32_t fileOffset, uint16_t fileHandle);
    bool (*create)(const char *filename,uint16_t *handleOrError);
    bool (*write)(const uint8_t *buffer,uint16_t byteCount,uint16_t fileHandle,uint16_t *bytesWrittenOrError);
    const char *(*getDtaFilename)(void);
    bool (*findFirst)(const char *pattern,uint16_t attributes);
    bool (*findNext)(void);
} MwDosFileCalls;
extern MwDosFileCalls mwDosFiles;
/* Host allocation validation after a real short read, before the copy.
 * This changes neither the native requested CX nor game resource decisions. */
extern void (*mwHostValidateFileDestination)(const uint8_t *buffer,uint16_t bytesRead);
bool MwHostSetOriginalGameDirectory(const char *directory);
bool MwHostSetCampaignSaveDirectory(const char *directory);
/* DIRECTORY/NAME for a flat DOS file name, as DOS finds it: the exact name
 * if it exists, else the one entry whose name matches ignoring case (an
 * install copied with upper- or mixed-case names on a case-sensitive file
 * system), else the exact name, so a missing file fails as before. False
 * only if the path does not fit. */
bool MwHostJoinGamePath(char *path,size_t pathBytes,const char *directory,const char *name);
void MwHostCloseProcessFiles(void);
#endif
