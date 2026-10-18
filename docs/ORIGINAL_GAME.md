# Bring your own game

Supply a legally obtained MechWarrior (1989 DOS) installation using
`src/Run-MechWarrior.ps1 -GameDirectory PATH`, or place it in the ignored
`assets/original-game` directory.

The port reads supported original BTECH.EXE/MW_MAIN.EXE tables as well as resource
files. Copying only model or picture files is insufficient. Do not replace the
original executables with the port executable. No installation is bundled.

Saves, captures and logs go under ignored `local` directories rather than into
your original installation. Keep original media safe; test edited resources in
a private working copy. See [supported versions](SUPPORTED_VERSIONS.md).
