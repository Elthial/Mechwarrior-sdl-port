#ifndef MECHWARRIOR_COMBAT_PROJECTILES_H
#define MECHWARRIOR_COMBAT_PROJECTILES_H
#include "mechwarrior/combat_world.h"
enum { COMBAT_PROJECTILE_SLOTS=0x20,COMBAT_PROJECTILE_NO_TARGET=0xFFFF,
    COMBAT_PROJECTILE_PROFILES=5,COMBAT_PROJECTILE_ROLL_STEP=0x300,COMBAT_PROJECTILE_FALLEN_PITCH_BIAS=0x3FFC,
    COMBAT_PROJECTILE_MOVE_SUBSTEPS=5,COMBAT_PROJECTILE_TURN_LIMIT=0x38E,COMBAT_PROJECTILE_OVERSHOOT_ANGLE=0x5C6C,
    COMBAT_SRM_DAMAGE_SHIFT=1, /* Two damage points per SRM; LRM hits remain one each. */
    COMBAT_MISSILE_CLUSTER_ROWS=4,COMBAT_MISSILE_CLUSTER_ROLLS=12 };
typedef struct BtechProjectileProfile {
    uint16_t speedScale,weaponKind,lifetimeTicks,resourceIdDelta;
} BtechProjectileProfile;
typedef struct BtechProjectileState {
    BtechPooledWorldObject *worldObject;
    uint16_t profileIndex,lifetimeRemaining,firingUnitSlot,targetUnitSlot,targetArmorLocation;
} BtechProjectileState;
extern BtechProjectileState projectiles[COMBAT_PROJECTILE_SLOTS];
extern BtechProjectileProfile projectileProfiles[COMBAT_PROJECTILE_PROFILES];
extern uint16_t projectileProfileIndicesByWeapon[COMBAT_WEAPON_KINDS],bodyLocationReferenceIndices[COMBAT_BODY_LOCATIONS];
extern int16_t missileClusterRowByWeapon[COMBAT_WEAPON_KINDS];
extern uint16_t missileClusterRows[COMBAT_MISSILE_CLUSTER_ROWS][COMBAT_MISSILE_CLUSTER_ROLLS];
BtechProjectileState *BtechAllocateProjectileFromWeaponProfile(int16_t weaponKind,const BtechPosition3D *placement);
BtechProjectileState *BtechSpawnUnitWeaponProjectile(uint16_t unitSlot,int16_t weaponKind,const BtechPosition3D *placement);
void BtechFirePlayerWeaponProjectile(uint16_t mountIndex);
void BtechResetProjectileHandles(void);
void BtechUpdateProjectiles(void);
uint8_t BtechSteerProjectileAndTestOvershoot(BtechProjectileState *projectile);
uint8_t BtechTestProjectileAgainstSpecialObject(BtechProjectileState *projectile);
int16_t BtechCalculateProjectileImpactDamage(const BtechProjectileState *projectile);
void BtechDestroyProjectileAndSpawnEffect(BtechProjectileState *projectile);
#endif
