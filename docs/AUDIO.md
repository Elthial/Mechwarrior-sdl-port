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
Combat now runs its own native Roland or AdLib card installation through the
same libraries. The installed `BTECHSND.SND` contains only PC-speaker effects,
so walking, weapons and impacts still use the speaker alongside the selected
card. The port does not substitute invented MIDI or FM effects for them.
The combat AdLib note/instrument driver and Roland request-driven MIDI path
are separate from campaign music. This is not a claim that the original game
contains a combat soundtrack, nor full general-purpose MPU-401 emulation.
AdLib retains the original instrument selection, volume, pitch-bend and
channel-stop behavior, including its unusual channel indexing. Roland direct
messages retain per-track running status and correctly framed system messages.
Neither path changes movement speed or supplies missing original resources.
The original game retains music-bank ownership, instrument uploads and cleanup
ordering. Those driver capabilities do not add a combat score to files that
contain none. Shared resource allocation preserves the original 16-bit limits
and reuse where playback depends on them, rather than granting unlimited host
memory or inventing missing instrument data.

## Listening and playtesting

MT-32 and AdLib are ready for listening/playtesting, not declared release-qualified.
Both use the same game speed: keep `cpu_cycles_per_ms=2500` while comparing them.
Set `sound_system=adlib` in your INI to test without ROMs, or use `mt32` and your
own ROM paths. Select the INI with the existing launcher option:

```powershell
.\src\Run-MechWarrior.ps1 -TimingConfiguration .\MechWarrior.ini
```

Restart the game after changing cards. The window title identifies the selected
sound system; the launcher prints the exact configuration and runtime-log paths.
Do not expect MT-32 and AdLib to sound identical: they use the game's respective
instrument/sequencer paths and different hardware emulators, not a shared arranger.

For a useful first test:

1. Let the title music play, then visit a Bar and listen to the change of cue.
2. Repair and rearm, accept a contract, and enter combat. Walk, turn, fire and
   listen for footfalls, weapon reports and impacts. These remain PC-speaker
   effects in the supplied original sound bank even with a card selected.
3. Return from the mission and revisit the Bar. Check that music resumes without
   stuck notes, growing delay or effects continuing from the previous mission.
4. Repeat with the other card using the same CPU setting and company save.

### Campaign music cues

| File | When to listen |
| --- | --- |
| `MECH.MUS` | Intro/title sequence |
| `MECHBAR.MUS` | Entering the Bar |
| `LAUNCH.MUS` | Successful contract debrief, despite its filename; not mission launch |
| `DEATH.MUS` | Lost lancemates during debrief, or the commander's death/restart event |
| `VICTORY.MUS` | Dark Wing story ending, not an ordinary contract victory |

A failed contract with the commander and lancemates surviving does not itself
start Death. Deployments resolved without combat do not start the successful
contract cue. Returning to the planet hub stops music, so dismissing a debrief
quickly can cut its cue short. These are original campaign decisions, not
sound-card differences. Leave the results screen open to hear its music.

The automated smoke check runs campaign → combat → Bar with one continuously
open SDL audio device for both cards, including the original fifteen effects.
It checks that output is present and that the audio queue stays bounded. It is
not a completed-mission or subjective listening acceptance test. Report any
audible issue with the selected card, screen/action and launcher runtime log.

### Supported scope and remaining limits

The five campaign music files, their debrief/death/ending triggers and the
fifteen installed combat effects have automated playback coverage. Trigger
checks enter the real campaign with controlled combat-return results; they
do not amount to a complete campaign playthrough.

Tandy PSG/DAC and sampled-speaker library paths are not supported playback
options. The MPU adapter implements the original game's required command
paths, not a general-purpose MPU-401 emulator. Arbitrary replacement banks,
malformed data and unknown stale-memory references are not guaranteed to work.
Speaker output is a digital approximation, not an analogue hardware recording.
The existing libraries can differ from physical hardware in acoustics and
headroom; these limits do not imply missing songs or alternative weapon effects.

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

### Supplying your own Roland ROMs

You must supply your own compatible CONTROL and PCM ROM images. This project
does not bundle, download or provide links to ROM downloads.
[ScummVM's MT-32 guidance](https://docs.scummvm.org/en/v2.8.0/advanced_topics/understand_audio.html#mt-32-device-emulation)
is to extract them from your own physical MT-32 module; it also supports ROMs
from a CM-32L. The ROMs remain copyrighted. Owning the hardware should not be
treated as blanket permission to download somebody else's dump; check the
rules applicable where you live before obtaining or extracting ROMs.

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
