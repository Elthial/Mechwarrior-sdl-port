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

The game retains its own scores, SysEx instrument settings, note scheduling and
transmit queue. The library only emulates the receiving synthesizer. Stereo
output retains the score's pan/reverb settings. Original game assets remain required.

## Building and distributing

CMake fetches a pinned Munt revision into the build directory. For an offline
build, set `MECHWARRIOR_MT32_SOURCE_DIR` to an existing Munt source checkout.
The small C17 adapter lives in `src/MT32`; Munt itself is C++ behind its C API.
The build copies the shared library beside the executable; keep it there.

libmt32emu is LGPL-2.1-or-later, not covered by the project's MIT notice. See
[Munt's source and licence](https://github.com/munt/munt/tree/6e7c01fba7e1d50c8fa705834889fd0eac136075/mt32emu).
Distributions must include the applicable notices, licence and corresponding
library source availability, and permit an interface-compatible replacement.
Do not distribute Roland ROMs with builds or source packages.
