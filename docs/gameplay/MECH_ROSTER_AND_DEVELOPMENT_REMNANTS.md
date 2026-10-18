# Playable BattleMechs and development remnants

The playable roster is Locust, Jenner, Phoenix Hawk, Shadow Hawk, Rifleman,
Warhammer, Marauder and BattleMaster. The starting company receives a damaged
Jenner, not a factory-fresh Mech.

## Wasp and Wolverine

Campaign chassis names still include Wasp and Wolverine, but their existence
does not make them playable. `MwGenerateUnitStock()` replaces a selected Wasp
with a Locust and a selected Wolverine with a Phoenix Hawk when generating
stock. It also handles the forced starting Jenner separately.

This is evidence of retained development-era chassis references. It does not
establish when or why those Mechs were removed, nor prove that usable geometry,
animations, cockpit artwork and equipment definitions survive for them.

## Adding a Mech is more than importing a mesh

The workshop can rebuild or extend a model bank. A playable chassis also needs
equipment and armour definitions, body-part and animation references, cockpit
resources, and campaign/combat registration. Those are not created automatically
by OBJ import. Do not advertise an appended resource as a functioning ninth Mech.

See [model-bank inspection limits](MODEL_BANK_REMNANTS.md) for what the asset
search does and does not establish.
