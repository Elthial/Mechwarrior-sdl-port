# Reconstructed code guide

The DOS launcher orchestrated two programs: MW_MAIN campaign/RPG and BTECH
real-time combat. The port keeps that native chain in process, with an explicit
campaign/combat transfer contract rather than inventing a new game loop.

| Topic | Start here |
| --- | --- |
| Campaign startup/hub | `src/Original/Campaign/MW_MAIN_081B_STARTUP.c`, `MW_MAIN_081B_UI.c` |
| Contracts, travel, Mech services | `MW_MAIN_081B_CONTRACTS.c`, `MW_MAIN_081B_TRAVEL.c`, `MW_MAIN_081B_SERVICE.c` |
| Canonical campaign objects | `src/Original/Memory/MW_MAIN_1090.c`, `src/include/mechwarrior/campaign_mechanics.h` |
| Combat templates/state/control | `src/include/mechwarrior/combat_mechs.h`, `src/Original/Memory/BTECH_304E.c` |
| Firing, projectiles, damage | `src/Original/Combat/BTECH_0810_WEAPONS.c`, `BTECH_0810_PROJECTILES.c`, `BTECH_0810_DAMAGE.c` |
| Projection and EGA pixels | `src/Original/Geometry`, `src/Original/Video` |
| Audio | `src/Original/Audio`, `src/SDL/SDLBackend_Audio.c` |
| Timing | `docs/CPU_TIMING_PROFILE.md`, `MechWarrior.ini`, `src/SDL/SDLBackend_Time.c` |
| Optional test pilot | `src/Support/SupportPilot.c`, `docs/SUPPORT_PILOT.md` |

## Follow gameplay, not storage addresses

Start with [the campaign guide](CAMPAIGN_GUIDE.md) for company management and
[the combat guide](COMBAT_GUIDE.md) for missions. Their topic references name
the actual methods and explain what they do for Mechs, MechWarriors, weapons
and contracts. [The game-data guide](GAME_DATA.md) distinguishes the records.

`MwRunCampaign()` owns the campaign, while `BtechProgramMain()` owns real-time
combat. `MwApplyCurrentPlanetState()` applies the current world's local conditions;
`MwFinishPlanetArrivalContinuation()` refreshes local opportunities on arrival.
`BtechExportCombatResultsAndTransition()` returns mission results to the company.

Graphics and audio retain their own domain: methods describe drawing, projection,
sound sequencing or playback rather than forcing BattleTech terminology onto
hardware work. SDL presents and plays the results; it does not decide damage,
contract terms or enemy tactics. See [coding policy](CODE_STYLE.md) before edits.
