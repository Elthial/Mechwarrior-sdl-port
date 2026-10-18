#include "mechwarrior/dos_files.h"
#include "mechwarrior/original_data.h"
#include "mechwarrior/ega_campaign.h"
#include "mechwarrior/campaign_events.h"
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#if defined(_WIN32)
#include <io.h>
#else
#include <dirent.h>
#include <sys/stat.h>
#endif

enum { DOS_FIRST_FILE_HANDLE = 5, DOS_FILE_HANDLE_COUNT = 20, HOST_PATH_BYTES = 4096,
    DOS_ERROR_FILE_NOT_FOUND = 2, DOS_ERROR_PATH_NOT_FOUND = 3,
    DOS_ERROR_TOO_MANY_FILES = 4, DOS_ERROR_ACCESS_DENIED = 5,
    DOS_ERROR_INVALID_HANDLE = 6 };
static FILE *fileStreams[DOS_FILE_HANDLE_COUNT];
static char originalGameDirectory[HOST_PATH_BYTES];
static char campaignSaveDirectory[HOST_PATH_BYTES];
static char dtaFilename[13];
#if defined(_WIN32)
static intptr_t saveSearchHandle=-1;
static struct _finddata_t saveSearchEntry;
#else
static DIR *saveSearchDirectory;
#endif
void (*mwHostValidateFileDestination)(const uint8_t *buffer,uint16_t bytesRead);

bool MwHostSetOriginalGameDirectory(const char *directory)
{
    if (strlen(directory) >= sizeof(originalGameDirectory)) return false;
    strcpy(originalGameDirectory, directory); return true;
}
bool MwHostSetCampaignSaveDirectory(const char *directory)
{
    if (!directory || strlen(directory)>=sizeof(campaignSaveDirectory)) return false;
    strcpy(campaignSaveDirectory,directory);return true;
}
static uint16_t HostGetDosFileError(void)
{
    if (errno == ENOENT) return DOS_ERROR_FILE_NOT_FOUND;
    if (errno == ENOTDIR) return DOS_ERROR_PATH_NOT_FOUND;
    if (errno == EMFILE) return DOS_ERROR_TOO_MANY_FILES;
    return DOS_ERROR_ACCESS_DENIED;
}
static bool HostOpenFileReadWrite(const char *filename, uint16_t *handleOrError)
{
    char path[HOST_PATH_BYTES];
    size_t length=strlen(filename);
    bool saveFile=length>=4 && strcmp(filename+length-4,".GAM")==0;
    const char *directory=saveFile?campaignSaveDirectory:originalGameDirectory;
    if (saveFile && (!directory[0] || strpbrk(filename,"/\\:")))
    {*handleOrError=DOS_ERROR_ACCESS_DENIED;return true;} /* Confined host DOS save mount. */
    if (snprintf(path, sizeof(path), "%s%s%s", directory,
        directory[0] ? "/" : "", filename) >= (int)sizeof(path))
    { *handleOrError = DOS_ERROR_PATH_NOT_FOUND; return true; }
    for (uint16_t handle = DOS_FIRST_FILE_HANDLE; handle < DOS_FILE_HANDLE_COUNT; ++handle)
    {
        if (fileStreams[handle] != NULL) continue;
        fileStreams[handle] = fopen(path, "r+b"); /* Native AL=02, not read-only. */
        if (fileStreams[handle] == NULL) { *handleOrError = HostGetDosFileError(); return true; }
        *handleOrError = handle; return false;
    }
    *handleOrError = DOS_ERROR_TOO_MANY_FILES; return true;
}
static bool HostReadFile(uint8_t *buffer, uint16_t byteCount, uint16_t fileHandle, uint16_t *bytesReadOrError)
{
    if (fileHandle < DOS_FIRST_FILE_HANDLE || fileHandle >= DOS_FILE_HANDLE_COUNT || fileStreams[fileHandle] == NULL)
    { *bytesReadOrError = DOS_ERROR_INVALID_HANDLE; return true; }
    clearerr(fileStreams[fileHandle]);
    uint8_t *readBuffer=byteCount?malloc(byteCount):NULL;
    if (byteCount && !readBuffer) abort();
    size_t bytesRead = fread(readBuffer, 1, byteCount, fileStreams[fileHandle]);
    if (bytesRead)
    {
        if (mwHostValidateFileDestination) mwHostValidateFileDestination(buffer,(uint16_t)bytesRead);
        memcpy(buffer,readBuffer,bytesRead);
    }
    free(readBuffer);
    if (ferror(fileStreams[fileHandle])) { *bytesReadOrError = HostGetDosFileError(); return true; }
    *bytesReadOrError = (uint16_t)bytesRead; return false; /* Short read is success. */
}
static bool HostCloseFile(uint16_t fileHandle)
{
    if (fileHandle < DOS_FIRST_FILE_HANDLE || fileHandle >= DOS_FILE_HANDLE_COUNT || fileStreams[fileHandle] == NULL)
        return true;
    FILE *fileStream = fileStreams[fileHandle]; fileStreams[fileHandle] = NULL;
    return fclose(fileStream) != 0;
}
static bool HostSeekFile(uint32_t fileOffset, uint16_t fileHandle)
{
    if (fileHandle < DOS_FIRST_FILE_HANDLE || fileHandle >= DOS_FILE_HANDLE_COUNT || fileStreams[fileHandle] == NULL)
        return true;
    if (fileOffset > LONG_MAX) return true; /* Explicit host limit; current resources fit. */
    return fseek(fileStreams[fileHandle], (long)fileOffset, SEEK_SET) != 0;
}
static bool HostCreateFile(const char *filename,uint16_t *handleOrError)
{
    char path[HOST_PATH_BYTES];
    if (!campaignSaveDirectory[0] || strpbrk(filename,"/\\:"))
    {*handleOrError=DOS_ERROR_ACCESS_DENIED;return true;}
    if (snprintf(path,sizeof(path),"%s/%s",campaignSaveDirectory,filename)>=(int)sizeof(path))
    {*handleOrError=DOS_ERROR_PATH_NOT_FOUND;return true;}
    for (uint16_t handle=DOS_FIRST_FILE_HANDLE;handle<DOS_FILE_HANDLE_COUNT;++handle)
    {
        if (fileStreams[handle]) continue;
        fileStreams[handle]=fopen(path,"w+b"); /* Native3C truncates an existing save. */
        if (!fileStreams[handle]) {*handleOrError=HostGetDosFileError();return true;}
        *handleOrError=handle;return false;
    }
    *handleOrError=DOS_ERROR_TOO_MANY_FILES;return true;
}
static bool HostWriteFile(const uint8_t *buffer,uint16_t byteCount,uint16_t fileHandle,uint16_t *bytesWrittenOrError)
{
    if (fileHandle<DOS_FIRST_FILE_HANDLE || fileHandle>=DOS_FILE_HANDLE_COUNT || !fileStreams[fileHandle])
    {*bytesWrittenOrError=DOS_ERROR_INVALID_HANDLE;return true;}
    clearerr(fileStreams[fileHandle]);
    size_t written=fwrite(buffer,1,byteCount,fileStreams[fileHandle]);
    if (ferror(fileStreams[fileHandle])) {*bytesWrittenOrError=HostGetDosFileError();return true;}
    *bytesWrittenOrError=(uint16_t)written;return false;
}
static const char *HostGetDtaFilename(void) {return dtaFilename;}
static void HostCloseSaveSearch(void)
{
#if defined(_WIN32)
    if (saveSearchHandle!=-1) {_findclose(saveSearchHandle);saveSearchHandle=-1;}
#else
    if (saveSearchDirectory) {closedir(saveSearchDirectory);saveSearchDirectory=NULL;}
#endif
}
/* Host directory leaves become DOS8.3 names at the OS boundary. Do not sort
 * them: the campaign displays the actual native-style enumeration order. */
static bool HostAcceptSaveName(const char *name)
{
    const char *dot=strrchr(name,'.');
    if (!dot || dot==name || dot-name>8 || strchr(name,'.')!=dot || strlen(dot)!=4
        || strpbrk(name," /\\:*?\"<>|")) return false;
    if (toupper((unsigned char)dot[1])!='G' || toupper((unsigned char)dot[2])!='A' || toupper((unsigned char)dot[3])!='M') return false;
    for (size_t i=0;i<=strlen(name);++i) dtaFilename[i]=(char)toupper((unsigned char)name[i]);
    return true;
}
static bool HostFindNextSave(void)
{
#if defined(_WIN32)
    while (saveSearchHandle!=-1 && _findnext(saveSearchHandle,&saveSearchEntry)==0)
        if (!(saveSearchEntry.attrib&(_A_SUBDIR|_A_HIDDEN|_A_SYSTEM)) && HostAcceptSaveName(saveSearchEntry.name)) return false;
#else
    struct dirent *entry;
    while (saveSearchDirectory && (entry=readdir(saveSearchDirectory))!=NULL)
    {
        char path[HOST_PATH_BYTES];struct stat attributes;
        if (snprintf(path,sizeof(path),"%s/%s",campaignSaveDirectory,entry->d_name)>=(int)sizeof(path)) continue;
        if (stat(path,&attributes)==0 && S_ISREG(attributes.st_mode) && HostAcceptSaveName(entry->d_name)) return false;
    }
#endif
    HostCloseSaveSearch();return true;
}
static bool HostFindFirstSave(const char *pattern,uint16_t attributes)
{
    HostCloseSaveSearch();
    if (!campaignSaveDirectory[0] || strcmp(pattern,"*.GAM")!=0 || attributes!=0) return true;
#if defined(_WIN32)
    char path[HOST_PATH_BYTES];
    if (snprintf(path,sizeof(path),"%s/*",campaignSaveDirectory)>=(int)sizeof(path)) return true;
    saveSearchHandle=_findfirst(path,&saveSearchEntry);
    if (saveSearchHandle==-1) return true;
    if (!(saveSearchEntry.attrib&(_A_SUBDIR|_A_HIDDEN|_A_SYSTEM)) && HostAcceptSaveName(saveSearchEntry.name)) return false;
#else
    saveSearchDirectory=opendir(campaignSaveDirectory);
    if (!saveSearchDirectory) return true;
#endif
    return HostFindNextSave();
}
/* DOS process termination closes handles even where a native close hook is
 * deliberately RET-only. This is host process teardown, not a game procedure. */
void MwHostCloseProcessFiles(void)
{
    HostCloseSaveSearch();
    for (uint16_t handle=DOS_FIRST_FILE_HANDLE;handle<DOS_FILE_HANDLE_COUNT;++handle)
        if (fileStreams[handle]) (void)HostCloseFile(handle);
}
/* Temporary standard-C host file implementation; replace only this boundary
 * with SDL I/O later. No loader decisions or retries live here. */
MwDosFileCalls mwDosFiles = { HostOpenFileReadWrite, HostReadFile, HostCloseFile, HostSeekFile,HostCreateFile,HostWriteFile,
    HostGetDtaFilename,HostFindFirstSave,HostFindNextSave };

/* The original operating-system loader supplied executable-authored data.
 * Read the private installation at that boundary, decode named objects once,
 * then release the bytes. This is not a replacement campaign bootstrap. */
bool MwHostLoadCampaignInitialData(const char *executablePath)
{
    enum { MZ_HEADER_BYTES=0x001C, MZ_PARAGRAPH_BYTES=0x0010,
        ORIGINAL_LOAD_SEGMENT=0x0800, CAMPAIGN_DATA_SEGMENT=0x1090,
        CAMPAIGN_DATA_BYTES=0x10000 };
    uint8_t header[MZ_HEADER_BYTES];
    FILE *file=fopen(executablePath,"rb");
    if (!file) return false;
    bool loaded=false;
    uint8_t *data=NULL;
    if (fread(header,1,sizeof(header),file)!=sizeof(header)
        || header[0]!='M' || header[1]!='Z') goto finished;
    uint16_t headerParagraphs=(uint16_t)(header[8]|((uint16_t)header[9]<<8));
    if ((uint32_t)headerParagraphs*MZ_PARAGRAPH_BYTES<sizeof(header)) goto finished;
    long dataPosition=(long)headerParagraphs*MZ_PARAGRAPH_BYTES
        +(CAMPAIGN_DATA_SEGMENT-ORIGINAL_LOAD_SEGMENT)*MZ_PARAGRAPH_BYTES;
    if (fseek(file,dataPosition,SEEK_SET)!=0) goto finished;
    data=calloc(CAMPAIGN_DATA_BYTES,1);
    if (!data) goto finished;
    size_t bytesRead=fread(data,1,CAMPAIGN_DATA_BYTES,file);
    /* Named initial data ends at A150. Later image bytes belong to other
     * linked segments; the importer retains none of this temporary storage. */
    if (bytesRead<0xA150 || ferror(file)) goto finished;
    loaded=MwImportCampaignInitialData(data,CAMPAIGN_DATA_BYTES);
finished:
    free(data);
    if (fclose(file)!=0) loaded=false;
    return loaded;
}

bool MwHostLoadEgaInitialData(const char *executablePath)
{
    uint8_t header[0x001C];
    FILE *file=fopen(executablePath,"rb");
    if (!file) return false;
    uint8_t *data=NULL;
    bool loaded=false;
    if (fread(header,1,sizeof(header),file)!=sizeof(header)
        || header[0]!='M' || header[1]!='Z') goto finished;
    uint16_t headerParagraphs=(uint16_t)(header[8]|((uint16_t)header[9]<<8));
    if ((uint32_t)headerParagraphs*0x10<sizeof(header)
        || fseek(file,(long)headerParagraphs*0x10,SEEK_SET)!=0) goto finished;
    data=calloc(EGA_DATA_BYTE_COUNT,1);
    if (!data) goto finished;
    if (fread(data,1,EGA_DATA_BYTE_COUNT,file)<0x13DF || ferror(file)) goto finished;
    /* Representation decoding only: the named overlay objects and unavoidable
     * unchecked/font-code aliases all see the same original initial bytes. */
    for (uint32_t at=0;at<EGA_DATA_BYTE_COUNT;++at)
        EgaWriteOverlayByte((uint16_t)at,data[at]);
    loaded=true;
finished:
    free(data);
    if (fclose(file)!=0) loaded=false;
    return loaded;
}

bool MwHostLoadCampaignEventText(const char *executablePath)
{
    uint8_t header[0x001C];
    FILE *file=fopen(executablePath,"rb");
    if (!file) return false;
    bool loaded=false;
    uint8_t *data=NULL;
    if (fread(header,1,sizeof(header),file)!=sizeof(header) || header[0]!='M' || header[1]!='Z') goto finished;
    uint16_t paragraphs=(uint16_t)(header[8]|((uint16_t)header[9]<<8));
    long position=(long)paragraphs*0x10+(0x1AA5-0x0800)*0x10;
    if ((uint32_t)paragraphs*0x10<sizeof(header) || fseek(file,position,SEEK_SET)!=0) goto finished;
    data=malloc(EVENT_TEXT_STORAGE_END);
    if (!data) goto finished;
    if (fread(data,1,EVENT_TEXT_STORAGE_END,file)!=EVENT_TEXT_STORAGE_END || ferror(file)) goto finished;
    loaded=MwImportCampaignEventText(data,EVENT_TEXT_STORAGE_END);
finished:
    free(data);
    if (fclose(file)!=0) loaded=false;
    return loaded;
}

bool MwHostLoadCampaignMusicData(const char *executablePath)
{
    uint8_t header[0x1C],data[0x0A12];
    FILE *file=fopen(executablePath,"rb");
    if (!file) return false;
    bool loaded=false;
    if (fread(header,1,sizeof(header),file)!=sizeof(header) || header[0]!='M' || header[1]!='Z') goto finished;
    uint16_t paragraphs=(uint16_t)(header[8]|((uint16_t)header[9]<<8));
    long position=(long)paragraphs*0x10+(0x28BE-0x0800)*0x10;
    if ((uint32_t)paragraphs*0x10<sizeof(header) || fseek(file,position,SEEK_SET)!=0) goto finished;
    if (fread(data,1,sizeof(data),file)!=sizeof(data) || ferror(file)) goto finished;
    loaded=MwImportCampaignMusicData(data,sizeof(data));
finished:
    if (fclose(file)!=0) loaded=false;
    return loaded;
}
