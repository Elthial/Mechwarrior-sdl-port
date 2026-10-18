# BattleMechs, weapons and game data

These are the recovered MechWarrior contracts, not layouts copied from the
Crescent Hawks games and not a claim of exact tabletop rules equivalence.

## Eight playable chassis

| Chassis | Tonnage | External model bank |
| --- | ---: | --- |
| Locust | 20 | LOCPCK.TBL |
| Jenner | 35 | JENPCK.TBL |
| Phoenix Hawk | 45 | PHAPCK.TBL |
| Shadow Hawk | 55 | SHAPCK.TBL |
| Rifleman | 60 | RIFPCK.TBL |
| Warhammer | 70 | HAMPCK.TBL |
| Marauder | 75 | MARPCK.TBL |
| BattleMaster | 85 | BMAPCK.TBL |

Bank spellings should be checked against the installation inventory; the
catalogue/parser is authoritative. A displayed chassis is assembled from native
body-part resources and animation selection tables, not one modern skeletal mesh.
Geometry is external; templates, mount lists and animation directories are
executable-authored. Importing a new model does not add a ninth playable chassis.

Campaign kind IDs include retained Wasp and Wolverine entries. Stock generation
substitutes Locust for Wasp and Phoenix Hawk for Wolverine. Their presence is
development evidence, not playable hidden Mechs or proof of surviving models.
The starting Jenner is deliberately damaged, not a fully functional test default.

## Different record domains

- Campaign owned/market slots store kind, condition and compact damage tiers;
  owned roster slot, pilot ID, chassis kind and combat slot are different indices.
- CampaignUnitDamage records engine/gyro/sensors/life support/heat sinks,
  actuators, jump jets, weapon damage and body damage. Repair tiers are not the
  same as combat armour points or remaining internal structure.
- BtechMechTemplate supplies tonnage, speed/jump parameters, heat sinks, body
  references, weapon mounts, armour, internal structure and initial ammunition.
- BtechMechState owns current heat, critical losses, armour/internal structure,
  per-mount operational/reload state and ammo salvos. BtechUnitControlState owns
  orders, targets, remembered positions, manoeuvre and animation state.
- BtechWeaponMount binds weapon kind to body location. Unproved geometry fields
  remain unidentified; they must not be relabelled muzzle coordinates by guesswork.

## Weapons and locations

The game includes lasers/PPC, autocannon, machine gun, SRM and LRM families.
BtechWeaponDefinition has damageOrMissileCount, ammoFamily, reloadTicks,
heatUnits, rangeClass, minimumRange, short/medium/long boundaries and critical
slots. Missile count is not single-projectile damage. Native timing/range scales
are not automatically seconds/metres/tabletop hexes. Ammunition families and
per-mount state are separate; an empty ammo bin and a disabled weapon differ.

Combat armour has nine locations: left/right arms, left/right legs, centre torso,
rear centre torso, left/right torsos and head. Internal-structure/body indices
have a different layout. Transfer and critical-hit tables must select the proper
domain, not reuse an armour enum blindly. The special objective slot is not a
ninth Mech; objective building geometry is OTHPCK resource 14.

## Maps and effects

GRD stores terrain height/collision information; WLD places world objects.
Scenarios assemble four tiles. TERPCK contains terrain geometry, OTHPCK contains
miscellaneous/objective/effect geometry, and chassis banks contain Mech parts.
PAL:EGA resources select native landscape/polygon colours, including dither
patterns; Arctic ground is not hardcoded green. BMP/SCR and MW_*PICS.BIN use
different image/container contracts. Sound effects use sequenced commands rather
than recordings; the reviewed MUS songs use MIDI format 0. MWPCM is a new
sampled-audio interchange format, not an original game asset format.

## Methods that connect the systems

| Method | Responsibility |
| --- | --- |
| `MwApplyCurrentPlanetState()` | Applies the current world's local market, House, facilities and scenery, ammunition prices and negotiation limit. |
| `MwFinishPlanetArrivalContinuation()` | Completes arrival and refreshes the local Mech market, contracts and recruitment. |
| `MwGenerateUnitStock()` | Generates a Mech's market/starting condition, including the deliberately damaged starting Jenner. |
| `MwQuoteMechRepairTotal()` | Quotes outstanding repairs for the selected Mech. |
| `MwAdvanceCampaignTimeAndChargeWages()` | Advances the calendar and applies company wages and due opportunity refreshes. |
| `BtechPrepareCampaignCombatStage()` | Prepares combat from the deployed company's state and selected scenario. |
| `BtechApplyArmorAndInternalDamage()` | Applies combat hits to armour and internal structure. |
| `BtechExportCombatResultsAndTransition()` | Returns mission results to the campaign. |

The topic guides explain these responsibilities without register/address
notation. Detailed reconstruction proof remains separate from gameplay explanations.
