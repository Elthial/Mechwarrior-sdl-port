#include "mechwarrior/combat_mechs.h"
#include "mechwarrior/native_address.h"
#include "mechwarrior/combat_import.h"
#include "mechwarrior/combat_installation.h"
#include "mechwarrior/combat_damage.h"
#include <string.h>

uint16_t resourceDiskPromptNumber,combatContinuationStage;
uint16_t salvageEntryCount;
uint16_t campaignUnitKindByCombatTemplate[COMBAT_MECH_TEMPLATES];
uint16_t actuatorIntegrityTierByCondition[5],systemDamageTierByHits[3],weaponIntegrityTierByOperational[2];
BtechMissionRules missionRules;
_Static_assert(offsetof(BtechMissionRules,fields.continuationProfileIndex)==0x48,"Mission36 aliases profile0974");
uint16_t reducedMemoryMode,combatTerrainDetailLevel,combatMissionDefinitionIndex,combatSoundEnabled;
uint16_t playbackCombatInputRequested,combatScenarioIndex;
uint16_t gunneryTargetNumbers[8],pilotingRecoveryBounds[8],activeCockpitLayoutIndex;
uint16_t combatToPilotRosterSlots[4],combatToOwnedMechSlots[4],deployedPilotIds[4];
BtechTemplateReservations mechTemplateReservations;
_Static_assert(sizeof(BtechTemplateReservations)==0x14,"Native reservation WORD aliases");
_Static_assert(offsetof(BtechTemplateReservations,fields.reservedCount)==0x0E,"Native reservation count alias");
uint32_t mechTemplateBudgetCostsBytes[8];
uint16_t hudColourSchemeIndex,specialTargetInteractionBlocked,sideObjectiveRules[2],combatLandscapeIndex;
BtechCombatStageProfile combatContinuationProfiles[9];
uint16_t opposingObjectiveRules[5];
uint16_t combatTemplateByCampaignUnitKind[10],weaponDisabledByDamageTier[4],systemHitsByDamageTier[4],actuatorLossByDamageTier[4];
uint16_t armorToBodyLocations[9],campaignBodyTierByArmorLocation[9];
uint16_t mechFallbackStartTemplates[8];
int16_t mechFallbackTemplateSteps[8];
uint8_t combatDefaultCommanderNameBytes[10];
uint16_t savedArgumentCount;
const char *const *savedArguments;
BtechUnitControlState *activeUnitControl;
uint16_t startupControlMovementAlias;
uint8_t playerDamageSuppressed;
uint16_t damageTransferArmorLocations[COMBAT_BODY_LOCATIONS];
uint16_t criticalTransferBodyLocations[COMBAT_BODY_LOCATIONS],actuatorIndicesByBodyLocation[4];

bool BtechImportCombatStageInitialData(const uint8_t *data,size_t byteCount)
{
    if (!BtechImportMechInitialData(data,byteCount)) return false;
    startupControlMovementAlias=MwReadNativeWord(data,0x0051);
    playerDamageSuppressed=data[0x0E1A];
    for (uint16_t location=0;location<COMBAT_BODY_LOCATIONS;++location)
    {
        damageTransferArmorLocations[location]=MwReadNativeWord(data,0x0E4C+location*2);
        criticalTransferBodyLocations[location]=MwReadNativeWord(data,0x0E28+location*2);
    }
    for (uint16_t location=0;location<4;++location) actuatorIndicesByBodyLocation[location]=MwReadNativeWord(data,0x0E5E+location*2);
    memcpy(combatDefaultCommanderNameBytes,data+0x0978,sizeof(combatDefaultCommanderNameBytes));
    salvageEntryCount=MwReadNativeWord(data,0x0976);
    for (uint16_t chassis=0;chassis<COMBAT_MECH_TEMPLATES;++chassis)
        campaignUnitKindByCombatTemplate[chassis]=MwReadNativeWord(data,0x09CC+chassis*2);
    for (uint16_t condition=0;condition<5;++condition)
        actuatorIntegrityTierByCondition[condition]=MwReadNativeWord(data,0x09DC+condition*2);
    for (uint16_t hits=0;hits<3;++hits)
        systemDamageTierByHits[hits]=MwReadNativeWord(data,0x09E6+hits*2);
    for (uint16_t operational=0;operational<2;++operational)
        weaponIntegrityTierByOperational[operational]=MwReadNativeWord(data,0x09EC+operational*2);
    for (uint16_t i=0;i<9;++i)
    {
        armorToBodyLocations[i]=MwReadNativeWord(data,0x0E3A+i*2);
        campaignBodyTierByArmorLocation[i]=MwReadNativeWord(data,0x08AE+i*2);
        for (uint16_t stage=0;stage<3;++stage)
            combatContinuationProfiles[i].missionCodes[stage]=MwReadNativeWord(data,0x08F6+i*6+stage*2);
    }
    for (uint16_t i=0;i<COMBAT_MISSION_COUNT;++i) combatMissionSideZeroRules[i]=MwReadNativeWord(data,0x092C+i*2);
    for (uint16_t i=0;i<5;++i) opposingObjectiveRules[i]=MwReadNativeWord(data,0x08D8+i*2);
    for (uint16_t i=0;i<10;++i) combatTemplateByCampaignUnitKind[i]=MwReadNativeWord(data,0x08E2+i*2);
    for (uint16_t i=0;i<4;++i)
    {
        weaponDisabledByDamageTier[i]=MwReadNativeWord(data,0x08C0+i*2);
        systemHitsByDamageTier[i]=MwReadNativeWord(data,0x08C8+i*2);
        actuatorLossByDamageTier[i]=MwReadNativeWord(data,0x08D0+i*2);
    }
    for (uint16_t i=0;i<8;++i)
    {
        mechFallbackStartTemplates[i]=MwReadNativeWord(data,0x0982+i*2);
        uint16_t step=MwReadNativeWord(data,0x0992+i*2);
        mechFallbackTemplateSteps[i]=(int16_t)(step<0x8000?step:(int32_t)step-0x10000);
        mechTemplateBudgetCostsBytes[i]=(uint32_t)MwReadNativeWord(data,0x09A2+i*4)|((uint32_t)MwReadNativeWord(data,0x09A4+i*4)<<16);
    }
    resourceDiskPromptNumber=MwReadNativeWord(data,0xCFD6);
    combatContinuationStage=MwReadNativeWord(data,0xC35A);
    combatContinuationProfileIndex=MwReadNativeWord(data,0x0974);
    combatMissionDefinitionIndex=MwReadNativeWord(data,0xC96E);
    return true;
}

BtechMechTemplate mechTemplates[COMBAT_MECH_TEMPLATES];
BtechWeaponDefinition weaponDefinitions[COMBAT_WEAPON_KINDS];
uint16_t templateMaximumCombatScores[COMBAT_MECH_TEMPLATES];
uint8_t unitVisibility[COMBAT_MECH_SLOTS][COMBAT_MECH_SLOTS];
BtechMechState mechStates[COMBAT_MECH_SLOTS];
BtechUnitBinding unitBindings[COMBAT_MECH_SLOTS];
BtechUnitControlState unitControls[COMBAT_MECH_SLOTS];
uint16_t heatAimJitterThresholds[COMBAT_HEAT_THRESHOLDS];
uint16_t heatSpeedPenalties[COMBAT_MECH_SLOTS],heatInhibitFlags[COMBAT_MECH_SLOTS];
uint16_t specialAssaultMode;
uint16_t randomStateWords[COMBAT_RANDOM_WORDS],randomWriteIndex,randomAddendIndex;

/* File decoder only. Runtime records need not inherit native unaligned WORDs
 * or DOS near references. Mech/binding/control state is initialized by native
 *206D/2C40, NOT cleared as a side effect of loading authored templates. */
bool BtechImportMechInitialData(const uint8_t *data,size_t byteCount)
{
    if (!data || byteCount<NATIVE_SEGMENT_BYTE_COUNT) return false;
    for (uint16_t tier=0;tier<COMBAT_HEAT_THRESHOLDS;++tier)
        heatAimJitterThresholds[tier]=MwReadNativeWord(data,0x271E+tier*2);
    for (uint16_t kind=0;kind<COMBAT_WEAPON_KINDS;++kind)
    {
        const uint8_t *bytes=data+0x0EEC+kind*0x16;
        BtechWeaponDefinition *weapon=&weaponDefinitions[kind];
        weapon->damageOrMissileCount=MwReadNativeWord(bytes,0);
        weapon->ammoFamily=(int16_t)MwReadNativeWord(bytes,2);
        weapon->reloadTicks=MwReadNativeWord(bytes,4);weapon->heatUnits=MwReadNativeWord(bytes,6);
        weapon->rangeClass=MwReadNativeWord(bytes,8);weapon->minimumRange=MwReadNativeWord(bytes,0x0A);
        weapon->shortRangeBoundary=MwReadNativeWord(bytes,0x0C);
        weapon->mediumRangeBoundary=MwReadNativeWord(bytes,0x0E);
        weapon->longRangeBoundary=MwReadNativeWord(bytes,0x10);
        weapon->criticalSlots=MwReadNativeWord(bytes,0x12);weapon->unidentifiedField10=MwReadNativeWord(bytes,0x14);
    }
    for (uint16_t index=0;index<COMBAT_MECH_TEMPLATES;++index)
    {
        const uint8_t *bytes=data+0x1178+index*0x00E0;
        BtechMechTemplate *mech=&mechTemplates[index];
        mech->tonnage=MwReadNativeWord(bytes,0);
        mech->displayScale=MwReadNativeWord(bytes,2);
        mech->firingMovementThreshold=MwReadNativeWord(bytes,4);
        mech->maximumSpeedScale=MwReadNativeWord(bytes,6);
        mech->jumpJetGroupCount=MwReadNativeWord(bytes,8);
        mech->jumpAcceleration=MwReadNativeWord(bytes,0x000A);
        mech->jumpSpeedLimit=MwReadNativeWord(bytes,0x000C);
        mech->unidentifiedField8=MwReadNativeWord(bytes,0x000E);
        mech->heatSinkCount=MwReadNativeWord(bytes,0x0010);
        mech->weaponMountCount=MwReadNativeWord(bytes,0x0012);
        for (uint16_t location=0;location<COMBAT_ARMOR_LOCATIONS;++location)
        {
            BtechBodyReferencePoint *point=&mech->bodyReferencePoints[location];
            point->componentA=(int16_t)MwReadNativeWord(bytes,0x0014+location*6);
            point->componentB=(int16_t)MwReadNativeWord(bytes,0x0016+location*6);
            point->componentC=(int16_t)MwReadNativeWord(bytes,0x0018+location*6);
            mech->maximumArmor[location]=MwReadNativeWord(bytes,0x00B2+location*2);
        }
        for (uint16_t mount=0;mount<COMBAT_WEAPON_MOUNTS;++mount)
        {
            BtechWeaponMount *weapon=&mech->weaponMounts[mount];
            weapon->weaponKind=(int16_t)MwReadNativeWord(bytes,0x004A+mount*8);
            weapon->bodyLocation=MwReadNativeWord(bytes,0x004C+mount*8);
            weapon->unidentifiedField4=MwReadNativeWord(bytes,0x004E+mount*8);
            weapon->unidentifiedField6=MwReadNativeWord(bytes,0x0050+mount*8);
        }
        for (uint16_t family=0;family<COMBAT_AMMO_FAMILIES;++family)
        {
            mech->ammoBodyLocations[family]=(int16_t)MwReadNativeWord(bytes,0x009A+family*2);
            mech->initialAmmoSalvos[family]=MwReadNativeWord(bytes,0x00D4+family*2);
        }
        for (uint16_t group=0;group<COMBAT_JET_GROUPS;++group)
        {
            mech->jumpJetGroups[group].bodyLocation=(int16_t)MwReadNativeWord(bytes,0x00A6+group*4);
            mech->jumpJetGroups[group].criticalSlots=MwReadNativeWord(bytes,0x00A8+group*4);
        }
        for (uint16_t location=0;location<COMBAT_BODY_LOCATIONS;++location)
            mech->maximumInternalStructure[location]=MwReadNativeWord(bytes,0x00C4+location*2);
    }
    for (uint16_t index=0;index<COMBAT_RANDOM_WORDS;++index)
        randomStateWords[index]=MwReadNativeWord(data,0x281C+index*2);
    randomWriteIndex=MwReadNativeWord(data,0x285C);
    randomAddendIndex=MwReadNativeWord(data,0x285E);
    specialAssaultMode=MwReadNativeWord(data,0xC7BE);
    return true;
}

/* EXEC/DFCE data projection only, not206D or a new game procedure. Clear
 * converted owners in the native4EDC..D5DF BSS interval; reload authored
 * scalars separately. Host bindings and campaign owners are untouched. */
void BtechInitializeFreshMechOwners(const uint8_t *data)
{
    resourceDiskPromptNumber=0;
    combatContinuationStage=0;
    reducedMemoryMode=0;
    combatTerrainDetailLevel=0;
    combatMissionDefinitionIndex=0;
    combatSoundEnabled=0;
    playbackCombatInputRequested=0;
    combatScenarioIndex=0;
    for (uint16_t slot=0;slot<8;++slot)
        gunneryTargetNumbers[slot]=0;
    for (uint16_t slot=0;slot<8;++slot)
        pilotingRecoveryBounds[slot]=0;
    activeCockpitLayoutIndex=0;
    for (uint16_t slot=0;slot<4;++slot)
        combatToPilotRosterSlots[slot]=0;
    for (uint16_t slot=0;slot<4;++slot)
        combatToOwnedMechSlots[slot]=0;
    for (uint16_t slot=0;slot<4;++slot)
        deployedPilotIds[slot]=0;
    mechTemplateReservations=(BtechTemplateReservations){0};
    hudColourSchemeIndex=0;
    savedArgumentCount=0;
    activeUnitControl=NULL;
    for (uint16_t slot=0;slot<COMBAT_MECH_SLOTS;++slot)
        mechStates[slot]=(BtechMechState){0};
    for (uint16_t slot=0;slot<COMBAT_MECH_SLOTS;++slot)
        unitBindings[slot]=(BtechUnitBinding){0};
    for (uint16_t slot=0;slot<COMBAT_MECH_SLOTS;++slot)
        unitControls[slot]=(BtechUnitControlState){0};
    specialAssaultMode=0;
    specialTargetInteractionBlocked=MwReadNativeWord(data,0x0EEA);
    for (uint16_t slot=0;slot<sizeof(sideObjectiveRules)/sizeof(sideObjectiveRules[0]);++slot) sideObjectiveRules[slot]=MwReadNativeWord(data,0x0ED0+slot*2);
    combatLandscapeIndex=MwReadNativeWord(data,0x1E86);
    for (uint16_t slot=0;slot<sizeof(combatDefaultCommanderNameBytes)/sizeof(combatDefaultCommanderNameBytes[0]);++slot) combatDefaultCommanderNameBytes[slot]=data[0x0978+slot*1];
    savedArguments=NULL;
}
