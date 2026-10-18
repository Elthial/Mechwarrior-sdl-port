# Campaign artwork and the EGA drawing path

Campaign picture banks hold the illustrations and interface artwork used by
the planet hub, Mech Complex and other campaign screens. They are separate
from combat model banks and cockpit picture resources.

| Method | Graphics responsibility |
| --- | --- |
| `MwLoadIndexedPicture()` | Loads the selected campaign picture. |
| `MwDecodePictureBank()` | Decodes packed campaign picture data. |
| `MwDrawIndexedPicture()` | Draws a selected picture at the requested screen position. |
| `MwDrawGeneralPicture()` | Draws from the general campaign picture bank onto the chosen page. |
| `EgaFlipPages()` | Swaps the drawing and displayed pages for double-buffered presentation. |

Campaign methods decide which illustrations belong on a screen; the graphics
methods decode and paint them. SDL presents the resulting image rather than
substituting a new campaign interface.

The workshop's PNG workflow is intended for editors such as Aseprite. Keep
dimensions, transparency and allowed palette colours compatible with the target
asset. Save imports to a working copy and test that copy in the game; do not
overwrite the original bank.
