# Runtime status

The Windows x64 EGA/PC-speaker build is experimentally playable, not a completed
campaign release. The approved default remains `cpu_cycles_per_ms=2500`.
Campaign music can also use MT-32 (Munt, user-supplied ROMs) or AdLib (Nuked-OPL3).
Both follow the original sequencing and have offline full-cue and dummy-SDL
integration checks, including the real campaign debrief/death/ending branches
with controlled combat-return results. Combat installs its native Roland or AdLib driver through
the same libraries; the shipped combat sound bank supplies speaker effects only.
No alternate MIDI/FM weapon sounds or combat score are invented. See [audio setup](AUDIO.md).

## Connected gameplay

The campaign supports planet services, repair/rearm/trading, recruitment and
Mech assignments, contract negotiation, travel/arrival and save/load. The real
campaign-to-combat chain reaches briefing, movement, firing, weapon effects,
command and damage screens, and returns mission results to the company.

Full-campaign acceptance, every mission objective and all chassis/scenario
combinations remain incomplete. Navigation and containment cases need more play.
CGA/Tandy and alternate sound backends are not release-qualified. Linux/macOS
playable builds have not been qualified. No complete DOS visual/audio equivalence
is claimed.

## Build and verification scope

This public repository builds the reconstructed game and its platform code.
It does not bundle decompiles, private comparison inputs, test fixtures or
research utilities. Those remain in the reverse-engineering repository; their
results do not automatically certify a particular public build.

Use [build and play instructions](PLAYABLE_TEST_BUILD.md) to exercise the real
campaign. Report contract, chassis, company state, inputs and runtime logs when
a fault occurs. See [presentation checks](VISUAL_ACCEPTANCE.md) for useful manual
coverage. A successful build alone is not a successful mission.
