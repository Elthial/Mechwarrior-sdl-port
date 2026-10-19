# Behavioural preservation

This port aims to run the original MechWarrior behaviour on modern hardware and
make its code understandable. It is not a redesigned simulation.

Keep the company-to-mission round trip recognisable: contracts, Mech preparation,
deployment, combat and results must follow the original decisions. SDL replaces
platform access, not movement, enemy tactics, weapon effects, damage or economics.

Preserve native integer behaviour, random-draw order, aliases, partial work,
error paths and observable quirks. One original procedure should retain one
recognisable implementation boundary. Use typed C objects, ordinary references
and named constants rather than an emulated segment-memory registry.

Invalid native reads are not a compatibility goal. Use explicit target validity,
objective health, terrain bounds and resource metadata instead of reproducing
accidental dependencies on adjacent data, freed allocations or stack residue.

Original executable behaviour and assembly remain the authority for uncertain
logic. The detailed annotations, provenance ledgers, Reko projects and regression
harnesses are maintained separately in the reverse-engineering repository.

Build success is not a claim of complete campaign or audiovisual equivalence.
Exercise real contracts, losses, retreat, save/load and campaign return before
claiming gameplay coverage. See [current limits](RUNTIME_STATUS.md) and
[presentation checks](VISUAL_ACCEPTANCE.md).

Do not distribute original game executables, data, extracted resources, memory
dumps or private saves. A legally obtained original installation is required.
See [the rights notice](NOTICE.md) and [coding style](CODE_STYLE.md).
