# Third-party notices

The C17 port uses SDL3 under its zlib licence. SDL3 is not vendored in this
source snapshot. Supply an installed SDL3 CMake package or an SDL3 source tree;
include SDL's own licence when distributing its runtime library.

Original game material has separate ownership: see [the rights notice](NOTICE.md).

Roland campaign music uses Munt libmt32emu, LGPL-2.1-or-later, through its C API.
The pinned source revision is `6e7c01fba7e1d50c8fa705834889fd0eac136075`:
[source, copyright notices and licence](https://github.com/munt/munt/tree/6e7c01fba7e1d50c8fa705834889fd0eac136075/mt32emu).
The build copies its licence beside the executable. When distributing binaries,
provide corresponding library source availability and allow replacement of the
shared library; do not remove upstream notices. Roland CONTROL and PCM ROMs are
separate user-supplied material and are never bundled. See [audio configuration](AUDIO.md).
