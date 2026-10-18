# Current reconstruction priorities

The game is an experimental playable EGA/PC-speaker reconstruction. The
campaign reaches real combat through its deployment chain; this is no longer
a startup-only scaffold. Historical reports of individual startup gates are
kept in the reverse-engineering repository, not used as the current feature list.

## Preserve the company-to-mission round trip

Continue testing complete contracts: repair and rearm, assign MechWarriors,
negotiate, deploy, engage enemies, fulfil the objective and return to the
campaign. Check the resulting Mech condition, ammunition, crew and C-bills.
A successful briefing or stationary cockpit is not sufficient acceptance.

## Focus remaining gameplay investigations

- Reproduce and resolve remaining mission-specific crashes or incorrect behaviour
  from native evidence rather than altering inputs to avoid them.
- Compare player/enemy movement, weapon effects and sound during real missions.
- Expand manual coverage across contract objectives and chassis, including
  losses, retreat and campaign return—not only successful defence missions.
- Retain the approved game speed while separating any sound/rendering defects
  from actual gameplay rules.

## Keep evidence and reader documentation separate

Public guides describe what a method does for the mercenary company or a
combatant, using current source names. Research records keep the detailed
proof. Do not paste register arithmetic or storage addresses into a gameplay
explanation, or repeat an old report's incomplete status as current truth.

Detailed verification records and test harnesses remain in the reverse repository;
neither a documentation cleanup nor a successful mission proves all methods
correct. See [runtime status](RUNTIME_STATUS.md), [campaign guide](CAMPAIGN_GUIDE.md)
and [combat guide](COMBAT_GUIDE.md).
