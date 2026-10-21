/* OPTIONAL HOST PILOT sensor adapter. Reads canonical owners only.
 * Native radar/terrain visibility supplies contacts; the renderer supplies
 * screen coordinates and the shot-selection object. No new LOS/game calls. */
#include "mechwarrior/support_pilot.h"
#include "mechwarrior/combat_startup.h"
#include "mechwarrior/combat_input.h"
#include "mechwarrior/combat_pose.h"
#include "mechwarrior/combat_render.h"
#include <string.h>

static PilotPoint ReadPosition(const BtechWorldObjectPrefix *object)
{
    return (PilotPoint){(int32_t)object->position.positionA,
        (int32_t)object->position.positionB,(int32_t)object->position.positionC};
}
void SupportPilotObserve(PilotObservation *view,bool combatActive)
{
    memset(view,0,sizeof(*view));view->tick=combatTickCounter;
    if(!combatActive || !playerUnitBinding || !playerMechState || !playerMechTemplate)return;
    const BtechPooledWorldObject *body=playerUnitBinding->positionedLeftLeg;
    if(!body || !playerUnitBinding->torso)return;
    view->cockpit=combatDispatchState==COMBAT_SCREEN_COCKPIT && combatTickCounter!=0;
    view->alive=unitControls[0].conditionScoreOrExitCode>COMBAT_CONDITION_SCORE_MINIMUM;
    view->missionComplete=sideCompletionFlags[0] || sideCompletionFlags[1];
    view->fallen=unitControls[0].movementMode==MOVEMENT_MODE_FALLEN_POSE;
    view->heatInhibited=heatInhibitFlags[0]!=0;view->heat=playerMechState->heatUnits;
    view->aws=mountGroupMode!=0;view->salvo=fireCycleActive!=0;
    view->position=ReadPosition(&body->object.prefix);
    view->heading=body->object.prefix.orientationC;
    view->torsoHeading=playerUnitBinding->torso->object.prefix.orientationC;
    view->turnRate=body->orientationSteps[2];view->speed=body->forwardStepScale;
    view->requestedSpeed=(int16_t)requestedPlayerForwardStepScale;
    view->maximumSpeed=(int16_t)playerMechTemplate->maximumSpeedScale;
    view->lockedTarget=playerTargetSlot;view->selectedWeapon=selectedWeaponMount;
    view->reticleX=screenCentreA;view->reticleY=(int16_t)aimReticleScreenY;
    for(unsigned slot=1;slot<COMBAT_MECH_SLOTS;++slot)
    {
        const BtechUnitBinding *binding=&unitBindings[slot];
        const BtechPooledWorldObject *drawn=unitLastDrawParts[slot];
        if(!binding->mech || !binding->positionedLeftLeg || !binding->torso)continue;
        PilotTarget *target=&view->targets[slot];
        target->visible=drawn && (drawn->object.prefix.flags&WORLD_OBJECT_TARGET_PROBE_VISIBLE)!=0
            && (drawn->object.prefix.renderFlags&OBJECT_PROJECTED_VISIBLE)!=0;
        target->known=target->visible || unitVisibility[0][slot]!=0;
        if(!target->known)continue;
        target->hostile=unitControls[slot].sideIndex!=unitControls[0].sideIndex;
        target->alive=unitControls[slot].conditionScoreOrExitCode>COMBAT_CONDITION_SCORE_MINIMUM;
        target->position=ReadPosition(&binding->positionedLeftLeg->object.prefix);
        if(target->visible)
        {
            /* CF5C stores the probe point only on OBJECT_LAST_MECH_DRAW_PART.
             * These are exactly the coordinates used by native target HUD6BCA;
             * the torso prefix's projected fields can legitimately remain zero. */
            target->screenX=drawn->object.projectedScreenA;
            target->screenY=drawn->object.projectedScreenB;
            const BtechPooledWorldObject *parts[]={binding->torso,binding->rightLeg,
                binding->positionedLeftLeg,binding->rightArm,binding->leftArm,binding->head};
            for(unsigned part=0;part<6;++part)
                if(parts[part] && selectedHitObject==&parts[part]->object.prefix)target->underReticle=true;
            for(unsigned i=0;i<COMBAT_ARMOR_LOCATIONS;++i)target->armor+=binding->mech->currentArmor[i];
            for(unsigned i=0;i<COMBAT_BODY_LOCATIONS;++i)target->internal+=binding->mech->currentInternalStructure[i];
        }
    }
    if(specialTargetObject && objectiveVisibilityObject &&
        (objectiveVisibilityObject->object.prefix.flags&WORLD_OBJECT_TARGET_PROBE_VISIBLE)
        && (objectiveVisibilityObject->object.prefix.renderFlags&OBJECT_PROJECTED_VISIBLE))
    {
        PilotTarget *target=&view->targets[COMBAT_MECH_SLOTS];
        target->known=target->visible=true;target->alive=specialTargetDamageRemaining!=0;
        target->hostile=sideObjectiveRules[0]==OBJECTIVE_SPECIAL_TARGET;
        target->position=ReadPosition(&specialTargetObject->object.prefix);
        target->screenX=objectiveVisibilityObject->object.projectedScreenA;
        target->screenY=objectiveVisibilityObject->object.projectedScreenB;
        target->underReticle=selectedHitObject==&specialTargetObject->object.prefix;
        target->internal=specialTargetDamageRemaining;
    }
    for(unsigned mount=0;mount<playerMechTemplate->weaponMountCount && mount<COMBAT_WEAPON_MOUNTS;++mount)
    {
        int kind=playerMechTemplate->weaponMounts[mount].weaponKind;
        if(kind<0 || kind>=COMBAT_WEAPON_KINDS)continue;
        const BtechWeaponDefinition *weapon=&weaponDefinitions[kind];
        const BtechWeaponState *state=&playerMechState->weapons[mount];
        bool ammunition=weapon->ammoFamily==-1 || (weapon->ammoFamily>=0 && weapon->ammoFamily<COMBAT_AMMO_FAMILIES &&
            playerMechState->ammoSalvosRemaining[weapon->ammoFamily]!=0);
        view->weapons[mount]=(PilotWeapon){state->operational && !state->reloadTicksRemaining && ammunition,
            weapon->heatUnits,(uint16_t)(weapon->longRangeBoundary*32*15)};
    }
}
