# BattleMech condition, armour and loadouts

The campaign needs to quote repairs and trade Mechs; combat needs to resolve
incoming fire. The two systems describe the same machines at different levels.

`CampaignUnitDamage` records repair condition for the engine, gyro, sensors,
life support, heat sinks, actuators, jump jets, weapon mounts and body damage.
`BtechMechState` records the BattleMech fighting a mission: remaining armour
and internal structure, accumulated critical losses, heat and ammunition.
Do not treat a campaign repair tier as a number of armour points.

## Body locations

Combat armour covers the left/right arms, left/right legs, centre torso, rear
centre torso, left/right torsos and head. Internal structure uses its own body
locations; rear torso armour does not imply an independent rear torso structure.
Weapon mounts refer to the location carrying that equipment.

## Damage methods

| Method | Gameplay purpose |
| --- | --- |
| `BtechApplyArmorAndInternalDamage()` | Applies a hit to the struck armour and internal structure, including the game's damage-transfer rules. |
| `BtechRollCriticalLosses()` | Resolves critical-loss attempts for the damaged location. |
| `BtechSelectAndApplyCriticalLoss()` | Selects and applies a component or equipment loss. |
| `BtechApplySpecialTargetDamage()` | Damages the mission's special objective rather than a BattleMech. |
| `BtechIsMechOperational()` | Checks whether a combat BattleMech remains operational. |
| `MwQuoteMechRepairTotal()` | Converts the campaign Mech's outstanding condition into a repair quote. |

The company's starting Jenner is damaged by design. It must not be silently
restored to factory condition during startup. Repairing a machine, rearming it
and assigning a MechWarrior are separate preparations for deployment.
