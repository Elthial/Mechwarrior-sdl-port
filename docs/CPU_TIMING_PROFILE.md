# Game speed and sound synchronisation

`MechWarrior.ini` selects the host CPU-work budget. The approved default is:

```ini
cpu_cycles_per_ms=2500
```

Lower values slow CPU-bound game work; higher values accelerate it. This is a
measured reference-workload budget, not a promise of exact DOS instruction
timing. Do not infer campaign days, weapon reload intervals or BattleMech speed
directly from this setting.

The host timing code paces the preserved game loop. It does not replace the
BattleMech movement rules or give enemy Mechs a separate speed multiplier.
Audio retains its own hardware-rate scheduling. A slower weapon sound is not
fixed by changing weapon damage, ammunition or movement arithmetic.

The former large queued-audio delay and later long-run clock drift were addressed.
Actual-device listening and broader mission testing still matter: a synthetic
queue test cannot certify walking sounds or weapon-effect synchronisation.

For timing reports, record the configuration, mission and chassis, and compare
player movement, enemy movement, gait and weapon effects together. Preserve
the approved default while investigating unrelated gameplay faults.
