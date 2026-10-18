#ifndef MECHWARRIOR_COMBAT_SURFACE_H
#define MECHWARRIOR_COMBAT_SURFACE_H
#include "mechwarrior/combat_world.h"
#include "mechwarrior/combat_resources.h"
enum { SURFACE_MAXIMUM_BOUNDARIES=0x20,SURFACE_RESPONSE_QUARTER_TURN=0x40,
    SURFACE_RESPONSE_FRACTION_BITS=6,SURFACE_ORIENTATION_STEP_LIMIT=1 };
uint8_t BtechTestPatchBoundaryHalfPlanes(const BtechSurfacePatch *patch,uint16_t queryA,uint16_t queryB);
uint16_t BtechEvaluateSurfacePatchHeight(const BtechSurfacePatch *patch,uint16_t queryA,uint16_t queryB);
uint16_t BtechTestSurfacePatches(BtechSurfaceQueryResult *result,const BtechPosition3D *position,
    const BtechSurfaceDefinition *surface,BtechWorldObjectPrefix *object,uint16_t cachedPatchFirst);
uint16_t BtechQueryWorldSurface(BtechSurfaceQueryResult *result,const BtechPosition3D *position,
    BtechWorldObjectPrefix *const *objects,int16_t objectCount);
uint8_t BtechCalculateByteOrientationStep(uint16_t target,uint16_t current,uint16_t limit);
uint16_t BtechResolveGroundSurfaceAndSlope(BtechPooledWorldObject *object,int16_t startingHeight,
    BtechWorldObjectPrefix *const *objects,int16_t objectCount);
#endif
