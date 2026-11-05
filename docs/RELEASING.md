# Windows and Linux releases

The platform workflows build the playable SDL executable, package its shared
libraries and check startup from an extracted archive. They run on pushes to
main, develop and ci/** branches, pull requests, or manual dispatch. The existing
headless workflow remains available separately.

Windows uses MSVC x64 on Windows Server 2022. Linux uses GCC on Ubuntu 24.04
x86-64 (glibc 2.39+). SDL 3.2.0 is pinned by commit, matching the supported CMake
minimum; CMake 3.31.6 is pinned for the older dependency build scripts. Munt and
Nuked-OPL3 use the project's existing pinned commits. SDL and the LGPL audio
libraries are bundled as shared libraries. Windows also bundles the MSVC CRT.
Linux requires compatible system C/C++ runtime, desktop and audio libraries.

Packages contain the executable, MechWarrior.ini, runtime libraries and notices.
No original game installation, saves or synthesizer ROMs are packaged. Run from
a writable directory, supplying your original installation as the first argument.

Before tagging, integrate the strict-C fixes from PR #1 (or equivalent) and
require both platform builds to pass. Their packaged startup checks establish
loading and argument handling only; manually verify SDL initialization, campaign,
combat, input and audio on both platforms before calling a version validated.

Create a version tag such as v0.1.0 on the intended tested commit and push it.
The release workflow resolves the tag to a single commit and calls both reusable
platform workflows. Only after both succeed does it verify both archives and
their SHA-256 checksums and publish a release containing:

- MechWarrior-vX.Y.Z-windows-x64.zip
- MechWarrior-vX.Y.Z-linux-x86_64.tar.gz
- MechWarrior-vX.Y.Z-SHA256SUMS.txt

Tags with a prerelease suffix create GitHub prereleases. Alternatively dispatch
the release workflow with an existing version tag. It refuses to replace an
existing release; inspect and resolve an existing release explicitly before a
rerun. No release version is selected or tag created by this workflow change.

Repository Actions must allow artifact uploads and a contents:write token for
the release publishing job. Build jobs have read-only repository access and do
not receive publishing credentials. Fork pull-request approval requirements are
unchanged.

Local archive-fixture checks: python -m unittest discover -s tools -p 'test_build_package.py' -v
These check packaging only and are not a substitute for native compilation.
