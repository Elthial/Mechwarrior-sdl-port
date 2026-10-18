# MechWarrior SDL port

Read docs/CODE_STYLE.md and docs/PRESERVATION_PLAN.md before changing game code.
Keep the implementation readable, C17 and assembly-loyal. Preserve native
procedure boundaries, numeric behaviour, aliases, RNG and write/call ordering.
Use proven BattleTech concepts and typed state, not a generic DOS-memory registry.

All implementation code, headers and build/run scripts belong under src.
Public documentation belongs under docs, with README.md at the root.
CMake and Visual Studio entry files remain at the root for build discovery.
Research tools, generators, decompiles, annotations, test suites and historical
records belong only in the separate reverse-engineering repository.

Never bundle original game files, extracted assets, private saves or captures.
Keep local output under ignored local directories. Do not change gameplay merely
to pass a fixture, and do not claim mission/visual acceptance from build success.
Ask before taking control of the user's shared computer.
