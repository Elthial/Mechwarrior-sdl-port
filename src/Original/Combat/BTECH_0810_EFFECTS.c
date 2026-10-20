#include "mechwarrior/combat_effects.h"
#include "mechwarrior/combat_resources.h"
#include "mechwarrior/combat_damage.h"
#include "mechwarrior/combat_ballistics.h"
#include "mechwarrior/combat_pose.h"
#include "mechwarrior/combat_render.h"
#include <stdlib.h>

/*0810:5B08. First free effect; anchor only A/B, retain height on world object. */
void BtechSpawnBodyDamageEffect(const BtechPosition3D *position)
{
    uint16_t freeObject=0;
    while (freeObject<COMBAT_POOLED_OBJECTS && pooledWorldObjects[freeObject].occupied) ++freeObject;
    if (freeObject==COMBAT_POOLED_OBJECTS || worldObjectCount>=COMBAT_WORLD_OBJECTS
        || cockpitWorldObjectCount>=COMBAT_WORLD_OBJECTS) return;
    for (uint16_t slot=0;slot<COMBAT_BODY_EFFECT_SLOTS;++slot)
    {
        BtechBodyDamageEffect *effect=&bodyDamageEffects[slot];
        if (effect->worldObject) continue;
        effect->worldObject=BtechAllocatePooledWorldObject(otherShapeResourceBase+COMBAT_BODY_EFFECT_INITIAL_RESOURCE,position);
        if (effect->worldObject)
        {
            effect->anchorPositionA=position->positionA;effect->anchorPositionB=position->positionB;
            effect->animationTicksRemaining=COMBAT_BODY_EFFECT_FRAMES;
            effect->worldObject->object.prefix.renderFlags=0;
        }
        return;
    }
}

/*0810:5B83. No pool release and no anchor/frame clearing. */
void BtechResetBodyDamageEffectHandles(void)
{
    for (uint16_t slot=0;slot<COMBAT_BODY_EFFECT_SLOTS;++slot) bodyDamageEffects[slot].worldObject=NULL;
    for (uint16_t slot=0;slot<COMBAT_DESTRUCTION_EMITTER_SLOTS;++slot) bodyDestructionEmitters[slot].ticksRemaining=0;
}

/*0810:5BB7. Emit first, then advance all effects, including newly spawned ones. */
void BtechUpdateBodyDestructionEffects(void)
{
    for (uint16_t slot=0;slot<COMBAT_DESTRUCTION_EMITTER_SLOTS;++slot)
    {
        BtechBodyDestructionEmitter *emitter=&bodyDestructionEmitters[slot];
        if (!emitter->ticksRemaining) continue;
        if (emitter->unitSlot>=COMBAT_MECH_SLOTS) abort();
        uint16_t mode=unitControls[emitter->unitSlot].movementMode;
        if (mode!=MOVEMENT_MODE_GROUNDED && mode!=BODY_EMITTER_ACCEPTED_MODE2 && mode!=BODY_EMITTER_ACCEPTED_MODE7)
        {emitter->ticksRemaining=0;continue;}
        --emitter->ticksRemaining;
        if ((int16_t)emitter->ticksRemaining>=BODY_EMITTER_FIRST_TICK) continue;
        if ((int16_t)emitter->ticksRemaining<=BODY_EMITTER_RANDOM_TICKS && !BtechRandomBelowBound(BODY_EMITTER_RANDOM_BOUND)) continue;
        BtechPosition3D *position=BtechGetUnitBodyReferencePosition(emitter->unitSlot,emitter->armorLocation);
        int16_t rise=(COMBAT_DESTRUCTION_EMITTER_TICKS-emitter->ticksRemaining)*2;
        position->positionC+=(uint32_t)rise;
        BtechSpawnBodyDamageEffect(position);
    }
    for (uint16_t slot=0;slot<COMBAT_BODY_EFFECT_SLOTS;++slot)
    {
        BtechBodyDamageEffect *effect=&bodyDamageEffects[slot];
        if (!effect->worldObject) continue;
        BtechPooledWorldObject *object=effect->worldObject;
        if (!effect->animationTicksRemaining)
        {
            if (object->occupied) {BtechReleasePooledWorldObject(object);effect->worldObject=NULL;}
            continue; /* Already-unoccupied object deliberately leaves stale handle. */
        }
        --effect->animationTicksRemaining;
        if (effect->animationTicksRemaining>=COMBAT_BODY_EFFECT_FRAMES) abort(); /* Adjacent-owner alias unsupported. */
        BtechWorldObjectPrefix *pose=&object->object.prefix;
        pose->resourceId=otherShapeResourceBase+bodyDamageEffectResourceDeltas[effect->animationTicksRemaining];
        uint32_t extent=BtechGetResourceScaledValue(pose->resourceId);
        pose->resourceScaledValue=(pose->resourceScaledValue&0xFFFF0000)|(extent&0xFFFF);
        pose->resourceScaledValue=(pose->resourceScaledValue&0xFFFF)|(extent&0xFFFF0000);
        int16_t heightStep=extent;
        int16_t spread=(int16_t)BtechShiftSignedDwordRight((uint32_t)heightStep,2);
        int16_t offsetA=(int16_t)BtechRandomSignedSpread(spread);
        pose->position.positionA=effect->anchorPositionA+(uint32_t)offsetA;
        int16_t offsetB=(int16_t)BtechRandomSignedSpread(spread);
        pose->position.positionB=effect->anchorPositionB+(uint32_t)offsetB;
        pose->position.positionC+=(uint32_t)heightStep;
    }
}

/*0810:08E4. Live frame/resource writes precede extent lookup and advancement. */
void BtechAdvanceEffectResourceSequences(void)
{
    for (uint16_t slot=0;slot<COMBAT_EFFECT_SLOTS;++slot)
    {
        BtechEffectSequenceState *effect=&effectSequences[slot];
        if (!effect->worldObject) continue;
        uint16_t sequence=effect->sequenceIndex;
        /* Native unchecked reads outside the two recovered arrays remain unsupported. */
        if (sequence>COMBAT_MACHINE_GUN_EFFECT_SEQUENCE
            || effect->frameIndex>=(sequence==COMBAT_IMPACT_EFFECT_SEQUENCE?7:3)) abort();
        uint16_t resourceDelta=effectResourceSequences[sequence][effect->frameIndex];
        if (resourceDelta==COMBAT_EFFECT_SEQUENCE_END)
        {
            BtechReleasePooledWorldObject(effect->worldObject);
            effect->worldObject=NULL;
        }
        else
        {
            BtechWorldObjectPrefix *object=&effect->worldObject->object.prefix;
            object->resourceId=otherShapeResourceBase+resourceDelta;
            uint32_t extent=BtechGetResourceScaledValue(object->resourceId);
            object->resourceScaledValue=(object->resourceScaledValue&0xFFFF0000)|(extent&0xFFFF);
            object->resourceScaledValue=(object->resourceScaledValue&0xFFFF)|(extent&0xFFFF0000);
            ++effect->frameIndex;
        }
    }
}

/*0810:5F1A. Reuse the first free sequence slot and a real pooled world object.
 * New effect orientation follows player body plus live relative torso twist. */
void BtechSpawnSequencedWorldEffect(const BtechPosition3D *position,uint16_t sequenceIndex)
{
    uint16_t slot=0;
    while (slot<COMBAT_EFFECT_SLOTS && effectSequences[slot].worldObject) ++slot;
    if (slot==COMBAT_EFFECT_SLOTS) return; /* Drop a cosmetic effect when all slots are busy. */
    uint16_t freeObject=0;
    while (freeObject<COMBAT_POOLED_OBJECTS && pooledWorldObjects[freeObject].occupied) ++freeObject;
    if (freeObject==COMBAT_POOLED_OBJECTS || worldObjectCount>=COMBAT_WORLD_OBJECTS
        || cockpitWorldObjectCount>=COMBAT_WORLD_OBJECTS) return;
    BtechEffectSequenceState *effect=&effectSequences[slot];
    effect->worldObject=BtechAllocatePooledWorldObject(0,position);
    BtechWorldObjectPrefix *object=&effect->worldObject->object.prefix;
    if (!unitBindings[0].positionedLeftLeg || !unitBindings[0].torso) abort();
    const BtechWorldObjectPrefix *player=&unitBindings[0].positionedLeftLeg->object.prefix;
    object->orientationA=player->orientationA;
    object->orientationB=player->orientationB;
    object->orientationC=player->orientationC;
    object->orientationC+=unitBindings[0].torso->object.prefix.orientationC;
    effect->sequenceIndex=sequenceIndex;
    object->renderFlags=0;
    effect->frameIndex=0;
}

/*0810:5F85. Drop only handles; frame/sequence words survive. No pool release. */
void BtechResetEffectObjectHandles(void)
{
    for (uint16_t slot=0;slot<COMBAT_EFFECT_SLOTS;++slot) effectSequences[slot].worldObject=NULL;
}
