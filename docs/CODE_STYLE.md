# Coding style

Write readable, assembly-loyal C17. Preserve the original procedure boundaries
and gameplay decisions rather than replacing them with a new engine.

- Use descriptive function and variable names. Use BattleTech terminology for
  established concepts: BattleMechs, MechWarriors, Gunnery, Piloting, armour,
  internal structure, heat sinks, critical hits, C-bills and Great Houses.
- Keep runtime identifiers free of native-address suffixes. Retain short native
  procedure provenance comments where they help compare the implementation.
- Define shared state once in its owning source file. Declare it in the relevant
  header under `src/include/mechwarrior`; make genuinely private state static.
- Use ordinary typed structures, arrays and references. Do not substitute a
  generic DOS-memory registry or address-switch accessor for a known record.
- Keep binary encoding at actual file/transfer boundaries. Preserve observable
  aliases, overlapping access and native numeric identities where required.
- Preserve integer widths, signedness, wrap, truncation, RNG order, partial writes
  and call order. Do not rely on signed overflow or host pointer sizes.
- Let typed declarations, assignments, returns and prototyped call arguments
  perform their normal final conversions. Keep a signed-byte/word interpretation
  when it is needed, without casting it again to the destination type. Avoid
  repeated signed/unsigned cast chains and casts back to an already-known type.
  Keep explicit conversions where an intermediate width or signed interpretation
  affects a comparison, shift, division or multiplication. Signed movement and
  damage budgets are not unsigned quantities; world coordinates and fixed-point
  products can require 32 bits even when the original CPU was 16-bit.
- Preserve meaningful game behaviour, not invalid memory reads. Missing targets,
  invalid resource sentinels, exhausted effect pools and map edges need explicit
  typed handling. Do not derive gameplay state from copyright text, executable
  bytes, unrelated variables or uninitialized native registers/stack contents.
- Name meaningful constants, flags and continuation destinations. Avoid magic
  numbers, redundant temporaries and invented pass-through Support helpers.
- Name palette colours as `Campaign_Pal_EGA_Green` or `Combat_Pal_EGA_Green`:
  identify the palette and the confirmed EGA colour. These are drawing indices,
  not interchangeable BIOS palette-register codes.
- Use readable loops, ++, -- and compound assignments when native-width behaviour
  is retained. Do not flatten genuine snapshots or alias-sensitive operations.
- Keep gameplay and original drawing/decoding/audio logic in `src/Original`.
  SDL implements the lowest practical OS/hardware boundary in `src/SDL`.
  Necessary host representation bridges belong in `src/Support`.
- Comments explain non-obvious purpose, ordering or quirks, not each assignment.
  Graphics and audio use their own terminology rather than forced gameplay names.

Research annotations, Reko metadata, generators and native comparison suites
belong in the reverse-engineering repository. They are not public-port source.
See [source layout](SOURCE_LAYOUT.md) and [preservation policy](PRESERVATION_PLAN.md).
