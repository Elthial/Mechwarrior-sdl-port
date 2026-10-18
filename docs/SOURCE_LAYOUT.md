# Source ownership

| Directory | Responsibility |
| --- | --- |
| `src/Original/Campaign` | Company management, planets, contracts, Mech services, recruitment and events. |
| `src/Original/Combat` | Mission preparation, Mech controls, enemy decisions, firing, damage and results. |
| `src/Original/Geometry` | World projection and geometry processing. |
| `src/Original/Video` | Picture and drawing operations, including EGA. |
| `src/Original/Audio` | Music and sound-event logic. |
| `src/Original/Memory` | Canonical shared game objects and executable-derived initial state. |
| `src/Original/Launcher`, `Runtime` | Original transition and runtime behaviour. |
| `src/SDL` | Host input, files, pacing, sound and presentation. |
| `src/Support` | Necessary representation bridges and disabled-by-default automation. |
| `src/include/mechwarrior` | Shared domain records and interfaces. |
| `src/Build.ps1`, `src/Run-MechWarrior.ps1` | Windows build and launch entry points. |
| `docs` | Current gameplay, code, build and licensing guides. |

Each shared object has one defining owner. Headers declare it; callers use typed
fields and references. Binary encoding belongs at actual IO boundaries.

The public tree contains implemented source, not empty future-source reservations.
Research tooling, decompiles, conversion ledgers and comparison suites live only
in the separate reverse-engineering repository. Build outputs and local saves,
logs and captures are ignored.

See [code navigation](CODE_GUIDE.md) for gameplay entry points.
