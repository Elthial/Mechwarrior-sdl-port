#include "mechwarrior/combat_pose.h"
#include "mechwarrior/native_address.h"

BtechPoseKeyframe playerPoseKeyframes[PLAYER_POSE_KEYFRAMES];
const BtechPoseKeyframe *playerPoseSequences[PLAYER_POSE_SEQUENCES];
uint16_t playerPoseFrameCounts[PLAYER_POSE_SEQUENCES];
uint16_t playerPoseDistanceAccumulator,playerPosePerturbationTicks;
uint16_t playerPoseSequence,playerPoseFrame,unidentifiedField26;
uint8_t animationFrameMapStorage[UNIT_ANIMATION_MAP_BYTES];
const uint8_t *animationFrameMaps[UNIT_ANIMATION_SEQUENCES];
uint8_t animationLogicalFrameCounts[UNIT_ANIMATION_SEQUENCES];
uint8_t animationPartFrameCounts[UNIT_ANIMATION_SEQUENCES];
uint16_t animationPrefixSequenceCounts[UNIT_ANIMATION_SEQUENCES];
uint8_t chassisAnimationAvailability[COMBAT_MECH_TEMPLATES][UNIT_ANIMATION_SEQUENCES];
BtechPooledWorldObject *activeUnitPositionedObject,*activeUnitTorsoObject;
BtechMechState *activeMechState;
BtechMechTemplate *activeMechTemplate;

/* Executable IO only: forty authored eight-byte records and their directory.
 * Runtime uses ordinary keyframe references, not a near-address registry. */
bool BtechImportPoseInitialData(const uint8_t *data,size_t bytes)
{
    if (!data || bytes<0xD1F0) return false;
    if (MwReadNativeWord(data,0x7750) || MwReadNativeWord(data,0xC35C)
        || MwReadNativeWord(data,0xC946) || MwReadNativeWord(data,0xD1EE)) return false;
    activeUnitPositionedObject=NULL;activeUnitTorsoObject=NULL;
    activeMechState=NULL;activeMechTemplate=NULL;
    for (uint16_t byte=0;byte<UNIT_ANIMATION_MAP_BYTES;++byte)
        animationFrameMapStorage[byte]=data[0x245E+byte];
    for (uint16_t sequence=0;sequence<UNIT_ANIMATION_SEQUENCES;++sequence)
    {
        uint16_t offset=MwReadNativeWord(data,0x248E+sequence*2);
        if (offset<0x245E || offset>=0x248E) return false;
        animationFrameMaps[sequence]=&animationFrameMapStorage[offset-0x245E];
        animationLogicalFrameCounts[sequence]=data[0x24A0+sequence];
        animationPartFrameCounts[sequence]=data[0x24AA+sequence];
        animationPrefixSequenceCounts[sequence]=MwReadNativeWord(data,0x24B4+sequence*2);
    }
    for (uint16_t chassis=0;chassis<COMBAT_MECH_TEMPLATES;++chassis)
        for (uint16_t sequence=0;sequence<UNIT_ANIMATION_SEQUENCES;++sequence)
            chassisAnimationAvailability[chassis][sequence]=data[0x24C6+chassis*UNIT_ANIMATION_SEQUENCES+sequence];
    for (uint16_t frame=0;frame<PLAYER_POSE_KEYFRAMES;++frame)
    {
        uint16_t offset=0x250E+frame*PLAYER_POSE_KEYFRAME_BYTES;
        playerPoseKeyframes[frame]=(BtechPoseKeyframe){(int16_t)MwReadNativeWord(data,offset),
            (int16_t)MwReadNativeWord(data,offset+2),(int16_t)MwReadNativeWord(data,offset+4),MwReadNativeWord(data,offset+6)};
    }
    for (uint16_t sequence=0;sequence<PLAYER_POSE_SEQUENCES;++sequence)
    {
        uint16_t offset=MwReadNativeWord(data,0x264E+sequence*2);
        if (offset<0x250E || offset>=0x264E || (offset-0x250E)%PLAYER_POSE_KEYFRAME_BYTES) return false;
        playerPoseSequences[sequence]=&playerPoseKeyframes[(offset-0x250E)/PLAYER_POSE_KEYFRAME_BYTES];
        playerPoseFrameCounts[sequence]=MwReadNativeWord(data,0x265E+sequence*2);
    }
    playerPoseDistanceAccumulator=MwReadNativeWord(data,0x5432);
    playerPosePerturbationTicks=MwReadNativeWord(data,0x5434);
    playerPoseSequence=MwReadNativeWord(data,0x5436);
    playerPoseFrame=MwReadNativeWord(data,0x5438);
    unidentifiedField26=MwReadNativeWord(data,0x543A);
    return true;
}
