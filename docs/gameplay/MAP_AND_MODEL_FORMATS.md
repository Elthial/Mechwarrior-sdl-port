# Battlefield maps and BattleMech models

Battlefields and geometry are external assets, while chassis equipment and
animation assembly also depend on executable-authored definitions. They are
not hardcoded drawing routines for each individual Mech.

## Battlefield assets

- GRD files describe battlefield heights and collision information.
- WLD files place terrain features, buildings and other world objects.
- A scenario assembles four battlefield tiles and selects mission conditions.
- TERPCK supplies terrain geometry; OTHPCK supplies miscellaneous geometry,
  effects and the yellow/black objective building.
- Landscape palettes determine desert, tropical and Arctic appearance.

The campaign's interstellar map is separate. Travelling between worlds does
not move the BattleMech's combat position, and a planet-hub background is not
the battlefield map.

## BattleMech geometry and animation

Each chassis model bank contains reusable body-part shapes. The game selects
and assembles the appropriate parts for the current animation and pose. Weapon
mounts and combat damage are gameplay records, not polygon material properties.

The workshop can inspect models, animation membership and map placements; its
3D view is an orthographic inspection preview, not the game's full perspective
renderer. Dragging the map rotates the ground plane and its placed geometry
together. Height editing changes the stored battlefield information; the preview
does not invent a coloured height mesh to stand in for native ground rendering.

OBJ/MTL, PNG and working-copy workflows are documented separately in the asset
interchange guide. Exported geometry alone is insufficient to register a new
playable chassis: equipment, animation, cockpit and campaign/combat references
also need supported definitions.
