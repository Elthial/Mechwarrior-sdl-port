# Where the game's content lives

| Content | Files and role |
| --- | --- |
| Campaign pictures | MW_*PICS.BIN banks contain campaign illustrations and interface artwork. |
| Combat pictures | BMP/SCR resources contain cockpit and other combat artwork; they are not ordinary Windows bitmap files despite some extensions. |
| BattleMech models | Chassis PCK.TBL banks contain reusable 3D body parts. |
| Terrain and objectives | TERPCK/OTHPCK banks provide terrain, buildings and effect geometry. |
| Battlefield layout | GRD/WLD pairs describe terrain information and placed objects. |
| Landscape colours | PAL resources supply native palettes, including EGA colour choices. |
| Sound effects | Native sound banks supply sequenced sound events rather than recorded WAV clips. |
| Music | The reviewed MUS songs contain MIDI format-0 music sequences. |
| Campaign progress | GAM saves preserve company, MechWarrior, contract and owned-Mech state. |
| Chassis/equipment rules | Executable-authored definitions supply stats, mounts and other gameplay information absent from model banks. |

The tooling decodes these inputs without changing the original installation.
Its exporters produce editable formats; importers validate and write working
copies. File-format acceptance is not a guarantee that an edited asset will
work throughout a mission.

MWPCM is a project-defined sampled-audio format for interchange. It is not a
newly discovered original game sound format, and importing it does not by
itself install a sampled-audio backend in the game.
