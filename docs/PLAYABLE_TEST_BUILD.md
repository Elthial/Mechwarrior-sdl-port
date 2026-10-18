# Playing the experimental build

Build with the existing SDL3 dependency, then launch against your own original
installation:

```powershell
./src/Build.ps1 -Configuration Release -SDL3Directory 'D:/Libraries/SDL3/cmake'
./src/Run-MechWarrior.ps1 -GameDirectory 'D:/Games/MechWarrior'
```

The launcher uses `build-visual-studio/Release`, keeps private saves separately
and reports the capture/log directory under ignored `local` for the run. SDL3.dll must remain next
to the executable. Do not set SDL dummy drivers for interactive play.
Keep the built `mt32emu-2.dll` and `nukedopl.dll` beside it too. Campaign music
selection and optional user ROM paths go in the existing `MechWarrior.ini`:
see [sound configuration](AUDIO.md). The default remains speaker.

## Preparing for a mission

The company starts with a damaged Jenner. Visit the Mech Complex to inspect,
repair and rearm it. Confirm the commander's assignment before accepting and
deploying under a House contract. Browse the offered terms: payment, salvage
and advance are separate. Acknowledge the combat briefing to enter the cockpit.

## Controls

Campaign: arrows and Enter select choices. Combat: Up/Down adjust speed,
Left/Right turn, Enter selects a target, Space fires the selected weapon and
Tab fires all weapons. Watch heat and ammunition. C opens command, D own
damage, E target damage and Q returns to the cockpit. T selects the map
instrument, Z changes zoom and P pauses. F12 saves a real display capture.

The default `cpu_cycles_per_ms=2500` is the user-approved playable speed.
Keep it unless deliberately investigating timing; do not copy older test
configurations which used faster settings.

## Reporting a problem

Keep the runtime log and full F12 capture set privately. Report the planet,
contract objective, chassis, repair/rearm state, recent inputs and what happened
before the failure. Do not upload the game installation or saves publicly.

The complete campaign, every objective and all presentation/audio behaviour are
not yet qualified. See [current status](RUNTIME_STATUS.md). Existing recorded
screenshots are not evidence that a new run or a different contract completed.
