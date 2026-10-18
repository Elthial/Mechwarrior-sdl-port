# Combat gameplay: commanding the lance

`BtechProgramMain()` runs combat. It receives the campaign's deployment,
prepares the battlefield and combatants, presents the briefing and cockpit,
runs the mission and returns results to the campaign.

## BattleMech versus MechWarrior orders

`BtechMechTemplate` describes a chassis. `BtechMechState` describes the individual
Mech fighting now: its heat, remaining armour, internal structure, damaged
equipment and ammunition. `BtechUnitControlState` holds orders, targets,
remembered enemy positions and manoeuvre/animation state.

These are different responsibilities. A chassis specification must not become
an AI order record, and a displayed body part is not a second combatant.

## Weapons, damage and objectives

- [Weapons and mounts](gameplay/MECH_WEAPON_SCHEMAS.md) identifies readiness,
  firing and projectile methods.
- [Damage and body locations](gameplay/MECH_CONDITION_LOADOUT.md) identifies
  armour/internal damage and critical-loss methods.
- [Scenario forces and objectives](gameplay/SCENARIO_UNIT_SCHEMAS.md) explains
  deployment and result handoff. The objective building is not a ninth Mech.
- [Maps and assembled models](gameplay/MAP_AND_MODEL_FORMATS.md) separates the
  battlefield, rendered Mech parts and gameplay specifications.

The port keeps original movement, heat, ammunition, firing, damage and enemy
decisions in reconstructed game code. SDL handles the host's input, sound and
display. The optional Support pilot drives normal input for testing; it does
not replace enemy AI or set damage/mission results.

For controls and launch instructions, read [playing the build](PLAYABLE_TEST_BUILD.md).
Current qualification and unresolved issues are in [runtime status](RUNTIME_STATUS.md).
