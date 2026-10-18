#ifndef MECHWARRIOR_COMBAT_EFFECTS_H
#define MECHWARRIOR_COMBAT_EFFECTS_H
#include "mechwarrior/combat_world.h"
enum { COMBAT_EFFECT_SLOTS=0x30,COMBAT_IMPACT_EFFECT_SEQUENCE=0,
    COMBAT_MACHINE_GUN_EFFECT_SEQUENCE=1,COMBAT_EFFECT_SEQUENCE_END=0x17,
    COMBAT_BODY_EFFECT_SLOTS=0x20,COMBAT_BODY_EFFECT_FRAMES=4,COMBAT_BODY_EFFECT_INITIAL_RESOURCE=10,
    BODY_EMITTER_FIRST_TICK=22,BODY_EMITTER_RANDOM_TICKS=8,BODY_EMITTER_RANDOM_BOUND=3,
    BODY_EMITTER_ACCEPTED_MODE2=2,BODY_EMITTER_ACCEPTED_MODE7=7 };
typedef struct BtechEffectSequenceState {
    BtechPooledWorldObject *worldObject;
    uint16_t frameIndex,sequenceIndex;
} BtechEffectSequenceState;
extern BtechEffectSequenceState effectSequences[COMBAT_EFFECT_SLOTS];
extern uint16_t defaultEffectResourceDeltas[7],machineGunEffectResourceDeltas[3];
extern uint16_t *const effectResourceSequences[2];
typedef struct BtechBodyDamageEffect {
    uint32_t anchorPositionA,anchorPositionB;
    BtechPooledWorldObject *worldObject;
    uint16_t animationTicksRemaining;
} BtechBodyDamageEffect;
extern BtechBodyDamageEffect bodyDamageEffects[COMBAT_BODY_EFFECT_SLOTS];
extern uint16_t bodyDamageEffectResourceDeltas[COMBAT_BODY_EFFECT_FRAMES];
void BtechSpawnBodyDamageEffect(const BtechPosition3D *position);
void BtechResetBodyDamageEffectHandles(void);
void BtechUpdateBodyDestructionEffects(void);
void BtechAdvanceEffectResourceSequences(void);
void BtechSpawnSequencedWorldEffect(const BtechPosition3D *position,uint16_t sequenceIndex);
void BtechResetEffectObjectHandles(void);
#endif
