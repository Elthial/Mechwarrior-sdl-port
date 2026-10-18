# Optional Support pilot

The source retains a disabled-by-default, input-only controller for supervised
mission debugging. It can track visible targets, keep range, orbit, navigate,
aim and fire subject to readiness and heat. It does not change damage,
ammunition, enemy positions or mission outcomes.

Its command-line supervision tools and detailed transport protocol are maintained
in the reverse-engineering repository, not distributed as part of this public
game. Normal play does not require them. The runtime interface remains available
for development without changing the original gameplay code.

F11 stops an enabled pilot; manual takeover and expired supervision release its
keys. Use `src/Run-MechWarrior.ps1 -DisableSupportPilot` to explicitly disable
transport. The approved game speed is unchanged.

Implementation: `src/Support/SupportPilot.c`,
`src/Support/SupportPilotObservation.c` and `src/SDL/SDLBackend_Pilot.c`.
