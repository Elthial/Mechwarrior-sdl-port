#include "mechwarrior/combat_import.h"
#include "mechwarrior/combat_scene.h"
#include "mechwarrior/combat_world.h"
#include "mechwarrior/combat_platform.h"
#include "mechwarrior/combat_runtime.h"
#include <stdlib.h>
#include <string.h>

/* BTECH0810:2C40..34A7. One native procedure, including shared3448 world join.
 * The host-only seed projects the preceding sound loader's OBSERVABLE local
 * alias; it is not a native source argument or a new304E field. */
void BtechPrepareCampaignCombatStage(uint16_t inheritedSoundStreamIdentity)
{
    BtechCampaignImport company;
    BtechDecodeCampaignImport(campaignCombatExchange,&company);
    resourceDiskPromptNumber=3;
    if (combatContinuationStage&0x8000)
    {
        for (uint16_t character=0;character<8;++character)
            if (company.commanderName[character]!=combatDefaultCommanderNameBytes[character])
                BtechTerminateToLauncher(1);
        memset(campaignCombatExchange+0x000D,0xFF,0x10);
        if ((uint8_t)company.gunneryTiers[0]>1) BtechIncreaseTemplateLegIntegrity();
        uint8_t command=' ';
        if (!(savedArgumentCount&0x8000) && savedArgumentCount>2)
        {
            if (!savedArguments || !savedArguments[2]) abort();
            command=BtechLowercaseCharacterByClassTable((int8_t)savedArguments[2][0]);
        }
        recordCombatInputRequested=command=='r';playbackCombatInputRequested=command=='p';
        BtechWaitScaledRuntimeDelay(COMBAT_PREPARE_DELAY);
        BtechCalibrateJoystickAxisPairs();
        uint16_t checksum=inheritedSoundStreamIdentity;
        for (uint16_t offset=0;offset<COMBAT_CHECKSUM_BYTES;++offset) checksum+=campaignCombatExchange[offset];
        BtechMixRandomStateWords(checksum);
        reducedMemoryMode=company.memoryTier==COMBAT_MEMORY_TIER_MINIMUM;
        combatTerrainDetailLevel=company.terrainDetailLevel;
        combatMissionDefinitionIndex=(uint16_t)(company.scenarioCode-1);
        if (company.scenarioCode==COMBAT_SCENARIO_DARK_WING_BASE)
        {specialAssaultMode=1;combatContinuationProfileIndex=8;combatMissionDefinitionIndex=COMBAT_PLANETARY_ASSAULT;}
    }
    combatSoundEnabled=company.soundDisabled==0;
    for (uint16_t slot=0;slot<COMBAT_MECH_SLOTS;++slot) mechStates[slot]=(BtechMechState){0};
    /*29CA is the genuine XOR AX,AX/RET. Its short-circuited replay predicate
     * can never select this join; no invented successful service is called. */
    if (combatMissionDefinitionIndex==COMBAT_NO_ASSIGNMENT) goto loadWorld;
    if (savedArgumentCount>3 && (combatContinuationStage&0x8000))
    {
        if (recordCombatInputRequested || playbackCombatInputRequested)
        {
            if (!savedArguments || !savedArguments[3]) abort();
            BtechConfigureCombatInputReplay(recordCombatInputRequested?COMBAT_INPUT_RECORD:COMBAT_INPUT_PLAYBACK,savedArguments[3]);
        }
    }
    for (uint16_t slot=0;slot<COMBAT_MECH_SLOTS;++slot)
    {gunneryTargetNumbers[slot]=COMBAT_DEFAULT_SKILL_PARAMETER;pilotingRecoveryBounds[slot]=COMBAT_DEFAULT_SKILL_PARAMETER;}
    if (combatMissionDefinitionIndex==COMBAT_EXTENDED_OFFENSIVE || combatMissionDefinitionIndex==COMBAT_EXTENDED_DEFENSIVE
        || combatMissionDefinitionIndex==COMBAT_EXTENDED_SIEGE || !(combatContinuationStage&0x8000) || specialAssaultMode)
    {
        ++combatContinuationStage;
        if (!combatContinuationStage && !specialAssaultMode) combatContinuationProfileIndex=BtechRandomBelowBound(8);
        if (combatContinuationProfileIndex>=COMBAT_STAGE_PROFILES || combatContinuationStage>=COMBAT_STAGES_PER_PROFILE) abort();
        combatMissionDefinitionIndex=combatContinuationProfiles[combatContinuationProfileIndex].missionCodes[combatContinuationStage]-1;
    }
    specialTargetInteractionBlocked=(!(combatMissionDefinitionIndex&0x8000)
        && ((combatMissionDefinitionIndex>=0x000A && combatMissionDefinitionIndex<=0x000D) || combatMissionDefinitionIndex>0x0022));
    if (combatMissionDefinitionIndex>=COMBAT_MISSION_COUNT) abort();
    sideObjectiveRules[0]=combatMissionSideZeroRules[combatMissionDefinitionIndex];
    if (sideObjectiveRules[0]>=5) abort();
    sideObjectiveRules[1]=opposingObjectiveRules[sideObjectiveRules[0]];
    combatLandscapeIndex=company.landscapeIndex;
    if (combatLandscapeIndex>2) combatLandscapeIndex=2;
    if (!(combatContinuationStage&0x8000) && combatContinuationStage>=1
        && combatContinuationProfileIndex!=4 && combatContinuationProfileIndex!=5)
    {
        uint16_t nextScenario=combatScenarioIndex+1;
        int32_t signedScenario=nextScenario<0x8000?nextScenario:(int32_t)nextScenario-0x10000;
        combatScenarioIndex=(uint16_t)(signedScenario%COMBAT_SCENARIO_COUNT);
    }
    else combatScenarioIndex=company.planetSelector%COMBAT_SCENARIO_COUNT;
    uint16_t deployedFriendlyCount=0;
    for (uint16_t rosterSlot=0;rosterSlot<COMBAT_LANCE_SLOTS;++rosterSlot)
    {
        uint16_t ownedUnitSlot=company.ownedMechSlots[rosterSlot];
        if (ownedUnitSlot==COMBAT_NO_ASSIGNMENT) continue;
        if (ownedUnitSlot>=12) abort();
        combatToPilotRosterSlots[deployedFriendlyCount]=rosterSlot;
        combatToOwnedMechSlots[deployedFriendlyCount]=ownedUnitSlot;
        uint16_t unitKind=company.ownedUnitKinds[ownedUnitSlot];
        if (unitKind>=10) abort();
        uint16_t templateIndex=combatTemplateByCampaignUnitKind[unitKind];
        if (templateIndex>=COMBAT_MECH_TEMPLATES) abort();
        BtechMechState *mech=BtechClaimFreeMechStateSlot(1);
        if (!mech) abort(); /* Native would write unrelated304E:0000; not a successful claim. */
        const BtechMechTemplate *chassis=&mechTemplates[templateIndex];
        const CampaignUnitDamage *damage=&company.ownedDamage[ownedUnitSlot];
        gunneryTargetNumbers[deployedFriendlyCount]=CAMPAIGN_SKILL_TO_COMBAT_BASE-(uint8_t)company.gunneryTiers[rosterSlot];
        pilotingRecoveryBounds[deployedFriendlyCount]=CAMPAIGN_SKILL_TO_COMBAT_BASE-(uint8_t)company.pilotingTiers[rosterSlot];
        if (rosterSlot) deployedPilotIds[deployedFriendlyCount]=(uint8_t)company.pilotIds[rosterSlot];
        mech->templateIndex=templateIndex;mech->engineHits=damage->engineHits;
        if (damage->sensorDamageTier>=4 || damage->gyroDamageTier>=4 || damage->lifeSupportDamageTier>=4) abort();
        mech->sensorHits=systemHitsByDamageTier[damage->sensorDamageTier];
        mech->gyroHits=systemHitsByDamageTier[damage->gyroDamageTier];
        mech->lifeSupportHits=systemHitsByDamageTier[damage->lifeSupportDamageTier];
        mech->destroyedHeatSinks=damage->destroyedHeatSinks;
        for (uint16_t actuator=0;actuator<COMBAT_ACTUATORS;++actuator)
        {
            uint8_t tier=damage->actuatorDamageTiers[actuator];if (tier>=4) abort();
            mech->actuatorCondition[actuator]=COMBAT_ACTUATOR_CONDITION_FULL-actuatorLossByDamageTier[tier];
        }
        mech->destroyedJumpJetGroups=damage->destroyedJumpJetGroups;
        if (chassis->jumpJetGroupCount) mech->jumpCharge=COMBAT_JUMP_CHARGE_CAPACITY;
        for (uint16_t mount=0;chassis->weaponMountCount<0x8000 && mount<chassis->weaponMountCount;++mount)
        {
            if (mount>=COMBAT_WEAPON_MOUNTS || damage->weaponDamageTiers[mount]>=4) abort();
            mech->weapons[mount].operational=(uint8_t)(1-(uint8_t)weaponDisabledByDamageTier[damage->weaponDamageTiers[mount]]);
        }
        for (uint16_t armor=0;armor<COMBAT_ARMOR_LOCATIONS;++armor)
        {
            uint16_t body=armorToBodyLocations[armor],tier=campaignBodyTierByArmorLocation[armor];
            if (body>=COMBAT_BODY_LOCATIONS || tier>=9) abort();
            uint16_t integrity=CAMPAIGN_FULL_BODY_INTEGRITY-damage->bodyDamageTiers[tier];
            uint16_t maximumStructure=chassis->maximumInternalStructure[body];
            uint16_t maximumSum=chassis->maximumArmor[armor]+maximumStructure;
            uint16_t product=(uint32_t)maximumSum*integrity;
            int32_t remaining=(product<0x8000?product:(int32_t)product-0x10000)/CAMPAIGN_FULL_BODY_INTEGRITY;
            int32_t structure=maximumStructure<0x8000?maximumStructure:(int32_t)maximumStructure-0x10000;
            if (remaining>structure)
            {mech->currentInternalStructure[body]=maximumStructure;mech->currentArmor[armor]=(uint16_t)(remaining-structure);}
            else {mech->currentInternalStructure[body]=(uint16_t)remaining;mech->currentArmor[armor]=0;}
        }
        if (!mech->currentInternalStructure[COMBAT_BODY_LEFT_LEG]) mech->currentInternalStructure[COMBAT_BODY_LEFT_LEG]=1;
        if (!mech->currentInternalStructure[COMBAT_BODY_RIGHT_LEG]) mech->currentInternalStructure[COMBAT_BODY_RIGHT_LEG]=1;
        /* Native passes the ROSTER index, NOT the compact claim slot, to4163. */
        if (!BtechIsMechOperational(mech) && rosterSlot) BtechClearUnitStateBinding(rosterSlot);
        else ++deployedFriendlyCount;
        uint16_t packedGroup=0;
        for (uint16_t family=0;family<COMBAT_AMMO_FAMILIES;++family)
            if (chassis->ammoBodyLocations[family]!=-1)
            {
                if (packedGroup>=6) abort();
                mech->ammoSalvosRemaining[family]=(uint8_t)company.ammoQuantities[packedGroup++][ownedUnitSlot];
            }
        uint16_t operationalGroups=chassis->jumpJetGroupCount-mech->destroyedJumpJetGroups;
        for (uint16_t group=0;operationalGroups<0x8000 && group<operationalGroups;++group)
        {if (group>=COMBAT_JET_GROUPS) abort();mech->jumpJetGroupOperational[group]=1;}
    }
    deployedPilotIds[0]=0;reservedMechTemplateCount=0;enemyMechPreparationCount=0;
    uint32_t availableBytes=BtechAllocateDosMemoryBlock(UINT32_MAX).availableBytes;
    availableBytes-=hudColourSchemeIndex?0x00014050:0x00019640;
    mechTemplateBudgetBytesLow=(uint16_t)availableBytes;
    mechTemplateBudgetBytesHigh=(uint16_t)(availableBytes>>16);
    for (uint16_t slot=1;slot<COMBAT_LANCE_SLOTS;++slot)
        if (unitBindings[slot].mech) (void)BtechTryReserveMechTemplate(unitBindings[slot].mech->templateIndex);
    if (specialAssaultMode)
    {
        (void)BtechPrepareEnemyMech(COMBAT_TEMPLATE_BATTLEMASTER);
        (void)BtechPrepareEnemyMech(COMBAT_TEMPLATE_BATTLEMASTER);
        (void)BtechPrepareEnemyMech(COMBAT_TEMPLATE_BATTLEMASTER);
        (void)BtechPrepareEnemyMech(COMBAT_TEMPLATE_WARHAMMER);
        for (uint16_t slot=4;slot<8;++slot) {gunneryTargetNumbers[slot]=0;pilotingRecoveryBounds[slot]=0;}
        distanceSortObjects[0]=NULL;activeCockpitLayoutIndex=0; /* Fifth native pair aliases these owners. */
    }
    else
    {
        uint16_t heavy=company.enemyHeavyCount,medium=company.enemyMediumCount,light=company.enemyLightCount;
        if (!(combatContinuationStage&0x8000))
        {
            uint16_t count=heavy-combatContinuationStage+2;heavy=(uint16_t)((count<0x8000?count:(int32_t)count-0x10000)/3);
            count=medium-combatContinuationStage+2;medium=(uint16_t)((count<0x8000?count:(int32_t)count-0x10000)/3);
            count=light-combatContinuationStage+2;light=(uint16_t)((count<0x8000?count:(int32_t)count-0x10000)/3);
        }
        uint16_t failed=0;
        while (!failed && (uint16_t)(heavy+medium+light)!=0)
        {
            if (heavy) {--heavy;if (!BtechPrepareEnemyMech(((uint8_t)BtechRandomBelowBound(0x80)&0x0C)/4+4)) failed=1;}
            if (medium && !failed) {--medium;if (!BtechPrepareEnemyMech(((uint8_t)BtechRandomBelowBound(0x80)&4)/4+2)) failed=1;}
            if (light && !failed) {--light;if (!BtechPrepareEnemyMech(((uint8_t)BtechRandomBelowBound(0x80)&4)/4)) failed=1;}
        }
    }
loadWorld:
    BtechLoadScenarioAndWorld(combatScenarioIndex);
    for (uint16_t slot=1;slot<COMBAT_LANCE_SLOTS;++slot)
    {
        const BtechMechState *mech=unitBindings[slot].mech;
        if (!mech) continue;
        if (!mech->currentInternalStructure[COMBAT_BODY_LEFT_ARM]) unitBindings[slot].leftArm->object.prefix.renderFlags|=COMBAT_WORLD_OBJECT_HIDDEN;
        if (!mech->currentInternalStructure[COMBAT_BODY_RIGHT_ARM]) unitBindings[slot].rightArm->object.prefix.renderFlags|=COMBAT_WORLD_OBJECT_HIDDEN;
    }
}

/* BTECH0810:4112..4162. The fallen actuator gate reads CURRENT control,
 * deliberately not a control record inferred from the supplied Mech. */
uint16_t BtechIsMechOperational(const BtechMechState *mech)
{
    if ((mech->engineHits<0x8000 && mech->engineHits>=COMBAT_ENGINE_HIT_LIMIT)
        || (mech->lifeSupportHits<0x8000 && mech->lifeSupportHits>=COMBAT_LIFE_SUPPORT_HIT_LIMIT)
        || mech->cockpitDisabled || !mech->currentInternalStructure[COMBAT_BODY_LEFT_LEG]
        || !mech->currentInternalStructure[COMBAT_BODY_RIGHT_LEG]) return 0;
    /* Initial1E84 calls2C40 BEFORE setting7752=D334. Near0000 is NOT a host
     * null dereference: its+0051 reads an authored304E WORD. Retain that owner
     * until actual control records are selected; never invent a zeroed Mech. */
    uint16_t movementMode=activeUnitControl?activeUnitControl->movementMode:startupControlMovementAlias;
    if (movementMode==COMBAT_MOVEMENT_FALLEN
        && (!mech->actuatorCondition[COMBAT_BODY_LEFT_LEG] || !mech->actuatorCondition[COMBAT_BODY_RIGHT_LEG])) return 0;
    return 1;
}

/* BTECH0810:3761..37F6. Exact equality is insufficient budget; signed HIGH
 * comparison precedes unsigned LOW, not a modern unsigned DWORD comparison. */
uint16_t BtechTryReserveMechTemplate(uint16_t templateIndex)
{
    uint16_t index=0;
    if (reservedMechTemplateCount && !(reservedMechTemplateCount&0x8000))
        while (reservedMechTemplateIds[index]!=templateIndex)
        {if (++index>=reservedMechTemplateCount) break;if (index>=10) abort();}
    if (index!=reservedMechTemplateCount) return 1;
    if (templateIndex>=8) abort();
    uint32_t cost=mechTemplateBudgetCostsBytes[templateIndex];
    uint32_t budget=(uint32_t)mechTemplateBudgetBytesLow|((uint32_t)mechTemplateBudgetBytesHigh<<16);
    int32_t costHigh=cost>>16,budgetHigh=budget>>16;
    if (costHigh>=0x8000) costHigh-=0x10000;if (budgetHigh>=0x8000) budgetHigh-=0x10000;
    if (!(costHigh<budgetHigh || (costHigh==budgetHigh && (uint16_t)cost<(uint16_t)budget))) return 0;
    index=reservedMechTemplateCount++;
    if (index>=10) abort(); /* Corrupt spill beyond the represented native neighbours. */
    reservedMechTemplateIds[index]=templateIndex;
    budget=(uint32_t)mechTemplateBudgetBytesLow|((uint32_t)mechTemplateBudgetBytesHigh<<16);
    budget-=cost; /* Native reload AFTER append: append itself may alias budget. */
    mechTemplateBudgetBytesLow=(uint16_t)budget;
    mechTemplateBudgetBytesHigh=(uint16_t)(budget>>16);
    return 1;
}

/* BTECH0810:350E..3582. Fallback ring is intentionally unbounded. */
uint16_t BtechPrepareEnemyMech(uint16_t preferredTemplate)
{
    if (!(enemyMechPreparationCount&0x8000) && enemyMechPreparationCount>3) return 0;
    ++enemyMechPreparationCount;
    if (!BtechTryReserveMechTemplate(preferredTemplate))
    {
        if (preferredTemplate>=8) abort();
        int16_t step=mechFallbackTemplateSteps[preferredTemplate];
        preferredTemplate=mechFallbackStartTemplates[preferredTemplate];
        while (!BtechTryReserveMechTemplate(preferredTemplate)) preferredTemplate=(uint16_t)(preferredTemplate+step)&7;
    }
    BtechInitializeMechConditionFromTemplate(BtechClaimFreeMechStateSlot(0),preferredTemplate);
    return 1;
}
