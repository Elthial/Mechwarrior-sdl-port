# BattleMechs, weapons and mounted equipment

A chassis specification, a particular BattleMech and a rendered body part are
different things. The Locust's specification describes what a Locust carries;
an individual company Locust has its own damage, ammunition and pilot.

## Main records

| Record | Meaning |
| --- | --- |
| `BtechMechTemplate` | Chassis specification: tonnage, movement and jump capability, heat sinks, armour, internal structure and equipment. |
| `BtechWeaponDefinition` | Shared weapon characteristics: heat, reload interval, range bands, ammunition family and damage or missile count. |
| `BtechWeaponMount` | A weapon fitted to a particular body location. Multiple mounts can use the same weapon definition. |
| `BtechMechState` | A BattleMech's current heat, armour, internal structure, component losses, ready weapons and remaining ammunition. |
| `BtechUnitBinding` | The connection between a combat BattleMech and the body parts used to display it. |

These records are not interchangeable with the campaign's compact condition
and repair records. Model banks supply geometry, not a complete chassis sheet.

## Firing methods

| Method | Gameplay purpose |
| --- | --- |
| `BtechIsPlayerWeaponMountReady()` | Checks whether a mounted weapon can fire, including its condition, reload and ammunition requirements. |
| `BtechFindNextReadyPlayerWeaponMount()` | Finds another ready mounted weapon for selection. |
| `BtechFirePlayerDirectWeapon()` | Resolves firing for a direct-fire weapon. |
| `BtechFirePlayerWeaponProjectile()` | Launches the selected weapon's projectile through the combat simulation. |
| `BtechUpdateProjectiles()` | Advances active projectiles and handles their ongoing flight and impact behaviour. |
| `BtechCalculateProjectileImpactDamage()` | Determines the damage delivered by a projectile impact. |

An SRM or LRM salvo contains missiles; its missile count must not be read as
the damage of one missile. Ammunition is shared by family, but operational and
reload state belong to each mount. A damaged weapon, an empty ammo supply and
a weapon still reloading are different reasons not to fire.

The game uses BattleTech concepts, but its range, heat and timing values must
not automatically be interpreted as tabletop hexes, turns or movement points.
