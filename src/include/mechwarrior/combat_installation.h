#ifndef MECHWARRIOR_COMBAT_INSTALLATION_H
#define MECHWARRIOR_COMBAT_INSTALLATION_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum { BTECH_IMAGE_LOAD_SELECTOR=0x0810,BTECH_IMAGE_BYTES=0x2D2C0,
    BTECH_PACKED_BYTES=0x2A8BB,BTECH_DATA_IMAGE_OFFSET=0x283E0,
    BTECH_TONE_IMAGE_OFFSET=0x27DB0,BTECH_GEOMETRY_IMAGE_OFFSET=0x1AA60,
    BTECH_BSS_START=0x4EDC,BTECH_BSS_END=0xD5E0 };
/* Temporary binary IO result, never a live game memory mirror. Caller frees
 * the image after projecting its authored fields into their typed owners. */
bool BtechDecodeInstallationImage(const uint8_t *packed,size_t packedBytes,uint8_t **image);
bool BtechHostLoadInitialData(const char *executablePath);
/* Host EXEC/CRT data projections, NOT additional native game procedures. */
void BtechInitializeFreshMechOwners(const uint8_t *data);
void BtechInitializeFreshWorldOwners(const uint8_t *data);
void BtechInitializeFreshStartupOwners(const uint8_t *data);
void BtechHostDiscardChildData(void);
struct BtechResourceBank;
struct BtechSurfaceBank;
/* Host decoded allocations only; never release DOS paragraphs or run game logic. */
void BtechHostDiscardDecodedShapeBank(struct BtechResourceBank *bank);
void BtechHostDiscardDecodedSurfaceBank(struct BtechSurfaceBank *bank);
void BtechHostRetainDetachedShapeRegistry(void);
extern void (*btechHostReleaseChildDosAllocations)(void);
#endif
