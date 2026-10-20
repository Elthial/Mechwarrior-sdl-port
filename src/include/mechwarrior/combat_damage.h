#ifndef MECHWARRIOR_COMBAT_DAMAGE_H
#define MECHWARRIOR_COMBAT_DAMAGE_H
#include "mechwarrior/combat_world.h"
enum { COMBAT_ARMOR_LEFT_ARM=0,COMBAT_ARMOR_RIGHT_ARM=1,COMBAT_ARMOR_LEFT_LEG=2,
    COMBAT_ARMOR_RIGHT_LEG=3,COMBAT_ARMOR_CENTER_TORSO=4,COMBAT_ARMOR_REAR_CENTER_TORSO=5,
    COMBAT_ARMOR_LEFT_TORSO=6,COMBAT_ARMOR_RIGHT_TORSO=7,COMBAT_ARMOR_HEAD=8,
    COMBAT_BODY_CENTER_TORSO=4,COMBAT_BODY_HEAD=7,COMBAT_SPECIAL_TARGET_SLOT=8,
    COMBAT_CRITICAL_ATTEMPT_THRESHOLD=7,COMBAT_CRITICAL_SUCCESS_COST=2,
    DICE_1D6=6,COMBAT_TWO_D6_MINIMUM=2,COMBAT_CRITICAL_DICE_BASE=COMBAT_TWO_D6_MINIMUM, /* Two zero-based d6 draws become 2..12. */
    COMBAT_CRITICAL_SLOT_ROLL_BOUND=12,COMBAT_ENGINE_CRITICAL_WEIGHT=6,COMBAT_GYRO_CRITICAL_WEIGHT=4,
    COMBAT_COCKPIT_CRITICAL_WEIGHT=1,COMBAT_AMMO_CRITICAL_WEIGHT=1,
    COMBAT_CENTER_TORSO_COCKPIT_TICK_MASK=7,COMBAT_CRITICAL_SOUND=10,COMBAT_CRITICAL_SOUND_PRIORITY=60,
    COMBAT_ENGINE_COCKPIT_ROLL_BOUND=10,COMBAT_ENGINE_COCKPIT_LOSS_ROLL=7,
    COMBAT_DESTRUCTION_EMITTER_SLOTS=8,COMBAT_DESTRUCTION_EMITTER_TICKS=25,
    COMBAT_HEAT_SINK_LOCATION_DIVISOR=7,
    COMBAT_DAMAGE_SOUND=7,COMBAT_DAMAGE_SOUND_PRIORITY=0x28,
    COMBAT_DESTRUCTION_HIGH_ALTITUDE=0x384,COMBAT_DESTRUCTION_LOW_ALTITUDE=0x190,
    COMBAT_DESTRUCTION_OFFSET=0xC8 };
extern uint8_t playerDamageSuppressed,targetDamagePanelDirty[COMBAT_MECH_SLOTS],specialTargetDamagePanelDirty;
extern uint16_t damageTransferArmorLocations[COMBAT_BODY_LOCATIONS];
extern uint16_t criticalTransferBodyLocations[COMBAT_BODY_LOCATIONS],actuatorIndicesByBodyLocation[4];
typedef struct BtechBodyDestructionEmitter {
    uint16_t unitSlot;
    int16_t armorLocation;
    uint16_t ticksRemaining;
} BtechBodyDestructionEmitter;
extern BtechBodyDestructionEmitter bodyDestructionEmitters[COMBAT_DESTRUCTION_EMITTER_SLOTS];
void BtechRollCriticalLosses(BtechMechState *mech,int16_t bodyLocation);
uint8_t BtechSelectAndApplyCriticalLoss(BtechMechState *mech,int16_t bodyLocation);
uint8_t BtechApplyDestroyedBodyLocationEffects(BtechMechState *mech,int16_t bodyLocation);
BtechPooledWorldObject *BtechGetUnitRenderPartByArmorLocation(uint16_t unitSlot,int16_t armorLocation);
void BtechQueueBodyDestructionEmitter(uint16_t unitSlot,int16_t armorLocation);
void BtechApplyArmorAndInternalDamage(uint16_t unitSlot,int16_t armorLocation,int16_t damage);
void BtechApplySpecialTargetDamage(int16_t damage);
#endif
