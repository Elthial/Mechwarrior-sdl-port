#include "mechwarrior/combat_runtime.h"
#include "mechwarrior/combat_import.h"
#include "mechwarrior/combat_world.h"
#include "mechwarrior/combat_startup.h"
#include "mechwarrior/combat_platform.h"
#include "mechwarrior/combat_cockpit.h"
#include "mechwarrior/combat_orders.h"
#include "mechwarrior/combat_effects.h"
#include "mechwarrior/combat_projectiles.h"
#include "mechwarrior/combat_scene.h"
#include <stdlib.h>

/*0810:37F7. Result export, next-stage reload or final launcher exit.
 * One source WORD, not a DI argument/result. Transfer writes are BYTE stores. */
void BtechExportCombatResultsAndTransition(uint16_t resultCode)
{
    campaignResultExchange.soundDisabled=combatSoundEnabled==0;
    if (resultCode==COMBAT_RESULT_SUCCESS && (combatContinuationStage==0 || combatContinuationStage==1))
    {
        BtechDistributeCampaignReserveAmmo();
        BtechRecoverFriendlyMechCondition();
    }
    campaignResultExchange.result=(uint8_t)resultCode;
    uint16_t disabledEnemies=0;
    for (uint16_t slot=COMBAT_LANCE_SLOTS;slot<COMBAT_MECH_SLOTS;++slot)
        if (unitBindings[slot].mech && (uint16_t)unitControls[slot].combatValueOrBoundaryCode==COMBAT_DISABLED_UNIT_VALUE)
            ++disabledEnemies;
    campaignResultExchange.disabledEnemyCount.low=(uint8_t)disabledEnemies;
    for (uint16_t slot=0;slot<COMBAT_LANCE_SLOTS;++slot)
    {
        const BtechMechState *mech=unitBindings[slot].mech;
        if (!mech) continue;
        if (mech->templateIndex>=COMBAT_MECH_TEMPLATES || combatToOwnedMechSlots[slot]>=CAMPAIGN_OWNED_UNIT_SLOTS) abort();
        const BtechMechTemplate *chassis=&mechTemplates[mech->templateIndex];
        CampaignUnitDamage *damage=&campaignResultExchange.ownedDamage[combatToOwnedMechSlots[slot]];
        damage->engineHits=(uint8_t)mech->engineHits;
        if (damage->gyroDamageTier>=4) abort();
        if (systemHitsByDamageTier[damage->gyroDamageTier]!=mech->gyroHits)
        {
            if (mech->gyroHits>=3) abort();
            damage->gyroDamageTier=(uint8_t)systemDamageTierByHits[mech->gyroHits];
        }
        if (damage->sensorDamageTier>=4) abort();
        if (systemHitsByDamageTier[damage->sensorDamageTier]!=mech->sensorHits)
        {
            if (mech->sensorHits>=3) abort();
            damage->sensorDamageTier=(uint8_t)systemDamageTierByHits[mech->sensorHits];
        }
        if (damage->lifeSupportDamageTier>=4) abort();
        if (systemHitsByDamageTier[damage->lifeSupportDamageTier]!=mech->lifeSupportHits)
        {
            if (mech->lifeSupportHits>=3) abort();
            damage->lifeSupportDamageTier=(uint8_t)systemDamageTierByHits[mech->lifeSupportHits];
        }
        damage->destroyedJumpJetGroups=(uint8_t)mech->destroyedJumpJetGroups;
        damage->destroyedHeatSinks=(uint8_t)mech->destroyedHeatSinks;
        for (uint16_t actuator=0;actuator<COMBAT_ACTUATORS;++actuator)
        {
            if (mech->actuatorCondition[actuator]>=5) abort();
            damage->actuatorDamageTiers[actuator]=(uint8_t)(CAMPAIGN_FULL_BODY_INTEGRITY-actuatorIntegrityTierByCondition[mech->actuatorCondition[actuator]]);
        }
        for (int16_t mount=0;mount<(int16_t)chassis->weaponMountCount;++mount)
        {
            if (mount>=COMBAT_WEAPON_MOUNTS || damage->weaponDamageTiers[mount]>=4) abort();
            int16_t operational=(int8_t)mech->weapons[mount].operational;
            if ((uint16_t)(1-weaponDisabledByDamageTier[damage->weaponDamageTiers[mount]])!=(uint16_t)operational)
            {
                uint16_t integrity;
                if (operational==-1) integrity=systemDamageTierByHits[2]; /*09EA, the actual preceding WORD. */
                else if ((uint16_t)operational<2) integrity=weaponIntegrityTierByOperational[operational];
                else abort();
                damage->weaponDamageTiers[mount]=(uint8_t)(CAMPAIGN_FULL_BODY_INTEGRITY-integrity);
            }
        }
        uint16_t packedFamily=0;
        for (uint16_t family=0;family<COMBAT_AMMO_FAMILIES;++family)
            if (chassis->ammoBodyLocations[family]!=-1)
                campaignResultExchange.ammo[packedFamily++][combatToOwnedMechSlots[slot]].low=(uint8_t)mech->ammoSalvosRemaining[family];
        for (uint16_t armor=0;armor<COMBAT_ARMOR_LOCATIONS;++armor)
        {
            uint16_t body=armorToBodyLocations[armor],tier=campaignBodyTierByArmorLocation[armor];
            if (body>=COMBAT_BODY_LOCATIONS || tier>=CAMPAIGN_BODY_LOCATION_COUNT) abort();
            int16_t maximum=(int16_t)(uint16_t)(chassis->maximumInternalStructure[body]+chassis->maximumArmor[armor]);
            int16_t scaledRemaining=(int16_t)(uint16_t)((mech->currentInternalStructure[body]+mech->currentArmor[armor])*CAMPAIGN_FULL_BODY_INTEGRITY+CAMPAIGN_FULL_BODY_INTEGRITY);
            if (!maximum) abort();
            int32_t remainingTier=(int32_t)scaledRemaining/maximum;
            if (remainingTier<INT16_MIN || remainingTier>INT16_MAX) abort();
            damage->bodyDamageTiers[tier]=(uint8_t)(CAMPAIGN_FULL_BODY_INTEGRITY-remainingTier);
        }
    }
    for (uint16_t slot=COMBAT_LANCE_SLOTS;slot<COMBAT_MECH_SLOTS && (int16_t)salvageEntryCount<CAMPAIGN_ROSTER_SLOTS;++slot)
    {
        const BtechMechState *mech=unitBindings[slot].mech;
        if (mech && (uint16_t)unitControls[slot].combatValueOrBoundaryCode==COMBAT_DISABLED_UNIT_VALUE
            && resultCode==COMBAT_RESULT_SUCCESS && sideObjectiveRules[0]!=OBJECTIVE_WITHDRAW)
        {
            uint8_t condition=BtechCalculateSalvageConditionPercent(mech);
            if (salvageEntryCount>=CAMPAIGN_ROSTER_SLOTS) abort(); /* Native negative count corrupts earlier transfer bytes. */
            CampaignSalvageEntry *salvage=&campaignResultExchange.salvage[salvageEntryCount];
            salvage->unitKind=(uint8_t)campaignUnitKindByCombatTemplate[mech->templateIndex];
            salvage->leadingZeroByte=0;salvage->conditionPercent=condition;salvage->trailingZeroByte=0;
            ++salvageEntryCount;
        }
    }
    for (uint16_t rosterSlot=0;rosterSlot<CAMPAIGN_ROSTER_SLOTS;++rosterSlot)
        campaignResultExchange.pilotStatus[rosterSlot]=0;
    for (uint16_t slot=0;slot<COMBAT_LANCE_SLOTS;++slot)
    {
        const BtechMechState *mech=unitBindings[slot].mech;
        if (mech && (mech->lifeSupportHits==COMBAT_LIFE_SUPPORT_HIT_LIMIT || mech->cockpitDisabled
            || (mech->engineHits==COMBAT_ENGINE_HIT_LIMIT && slot)))
        {
            if (combatToPilotRosterSlots[slot]>=CAMPAIGN_ROSTER_SLOTS) abort();
            campaignResultExchange.pilotStatus[combatToPilotRosterSlots[slot]]=1;
        }
    }
    if ((int16_t)combatContinuationStage>=0 && (int16_t)combatContinuationStage<COMBAT_FINAL_CONTINUATION_STAGE
        && resultCode==COMBAT_RESULT_SUCCESS && (!specialAssaultMode || (int16_t)combatContinuationStage<1))
    {
        if (hudColourSchemeIndex && (combatDispatchState==PLAYER_SCREEN_TARGET_INSPECTION || combatDispatchState==PLAYER_SCREEN_INSPECTION))
        {
            BtechClearInspectionBackground();
            BtechApplyLandscapeCameraColours();
        }
        BtechReleaseOrderPanelBuffer();
        BtechUnloadCockpitHudResources();
        BtechReleaseCombatChassisResourceBanks();
        BtechReleaseHeapWorldObjectList();
        BtechResetWorldObjectPool();
        BtechResetBodyDamageEffectHandles();
        BtechResetEffectObjectHandles();
        BtechResetProjectileHandles();
        BtechResetCombatState();
        /*2C40's sound-loader stack alias is consumed only for a negative
         * initial stage. This continuation is proved nonnegative above. */
        BtechPrepareCampaignCombatStage(0);
        BtechInitializeUnitControlRecords();
        BtechLoadCockpitHudResources();
        selectedObjectCount=0;
        for (uint16_t object=0;object<COMBAT_WORLD_OBJECTS;++object)
        {
            BtechWorldObjectPrefix *registered=worldObjects[object];
            if (!registered) break;
            if ((int16_t)registered->resourceId<(int16_t)surfaceResourceBank.resourceCount && (int16_t)registered->resourceId>=0)
            {
                if (selectedObjectCount>=COMBAT_SELECTED_WORLD_OBJECTS) abort();
                selectedWorldObjects[selectedObjectCount++]=worldObjects[object];
            }
        }
        playerCameraFillASuppressed=BtechRandomBelowBound(3)<1;
        BtechApplyLandscapeCameraColours();
        BtechInitializeOrderPanelState();
        BtechResetMechInspectionState();
        activeUnitControl=&unitControls[0];
        BtechSynchronizeMechRenderParts();
        BtechShowCombatStageBriefing();
        BtechRefreshCockpitViewportAndGauges();
        return;
    }
    resourceDiskPromptNumber=1;
    /*CC6F's existing host stdio representation: E206 read-stream close is
     * collapsed here at IO, not an invented game helper or numeric FILE cast. */
    BtechCloseStreamFile(BtechOpenStreamWithRetry("mw.exe","rb"));
    BtechTerminateToLauncher(0);
}
