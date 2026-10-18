# Sound configuration

Use the existing `MechWarrior.ini`, alongside `cpu_cycles_per_ms`. Changing the
sound system does not change the approved CPU budget or PIT clock.

```ini
cpu_cycles_per_ms=2500
sound_system=speaker
mt32_control_rom=
mt32_pcm_rom=
```

`speaker` retains the original PC-speaker music and combat effects. `mt32`
selects the original Roland campaign music backend, synthesized by libmt32emu.
`adlib` selects the original AdLib campaign driver, synthesized by Nuked-OPL3
in OPL2-compatible mode, and requires no external ROM files.
AdLib retains a mono card output, mirrored to the stereo host stream; it does
not gain artificial stereo from the OPL3 core's two-output sample staggering.
Combat currently retains speaker effects: its separate intelligent-mode Roland
driver is not implemented. This setting does not claim Roland combat support.

For Roland campaign music, supply a compatible MT-32 CONTROL and PCM ROM pair:

```ini
sound_system=mt32
mt32_control_rom="D:/My ROMs/MT32_CONTROL.ROM"
mt32_pcm_rom="D:/My ROMs/MT32_PCM.ROM"
```

Paths can be absolute or relative to the INI file. These ROMs contain synthesizer
firmware and sample data; they are not MechWarrior game files. You must obtain
them lawfully yourself. They are not provided or downloaded by this project.
Missing, invalid or incompatible ROMs produce an error, not a fake MT-32 fallback.
Combined MAME-versioned files such as `mt32_ctrl_1_07.rom` and `mt32_pcm.rom`
also work without renaming: Munt identifies their contents. The two-path config
expects combined images, not separate even/odd chip dumps. Control v1.07 with
MT-32 PCM is a useful initial old-model baseline; other compatible pairs are
identified by Munt rather than forced to that version.

The game retains its own scores, SysEx instrument settings, note scheduling and
transmit queue. The library only emulates the receiving synthesizer. Stereo
output retains the score's pan/reverb settings. Original game assets remain required.

## Building and distributing

CMake fetches a pinned Munt revision into the build directory. For an offline
build, set `MECHWARRIOR_MT32_SOURCE_DIR` to an existing Munt source checkout.
The small C17 adapter lives in `src/MT32`; Munt itself is C++ behind its C API.
The build copies the shared library beside the executable; keep it there.

Nuked-OPL3 is pinned at `765ec962e473aeb767e4cba74ffdc8f588ffbfe8` and
built as a separate shared library. Set `MECHWARRIOR_NUKED_OPL_SOURCE_DIR` to an
existing checkout for offline builds. The C17 hardware adapter is in `src/Adlib`.
Keep `nukedopl.dll` beside the executable on Windows. The card's status timers
are separate from synthesis: original AdLib probing and delay calibration still
run. This uses a 1-microsecond ISA status-read model, not instruction-exact bus
emulation. No MIDI-to-FM arranger replaces the game's instruments.

libmt32emu is LGPL-2.1-or-later, not covered by the project's MIT notice. See
[Munt's source and licence](https://github.com/munt/munt/tree/6e7c01fba7e1d50c8fa705834889fd0eac136075/mt32emu).
Distributions must include the applicable notices, licence and corresponding
library source availability, and permit an interface-compatible replacement.
Do not distribute Roland ROMs with builds or source packages.
Nuked-OPL3 also declares LGPL-2.1-or-later:
[source and licence](https://github.com/nukeykt/Nuked-OPL3/tree/765ec962e473aeb767e4cba74ffdc8f588ffbfe8).
The build copies its licence beside the executable; the same notice, source
availability and replaceability obligations apply to distributions.
