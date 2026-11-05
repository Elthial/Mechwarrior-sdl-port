"""Build and smoke-test a redistributable runtime, without original game data."""
import argparse
import hashlib
import os
from pathlib import Path
import re
import shutil
import subprocess
import tarfile
import zipfile


def run(*args, **kwargs):
    return subprocess.run(args, check=True, **kwargs)


def package(root, build, sdl, output, label):
    if not re.fullmatch(r"[A-Za-z0-9][A-Za-z0-9._-]*", label):
        raise ValueError("Invalid package label")
    windows = os.name == "nt"
    platform = "windows-x64" if windows else "linux-x86_64"
    name = f"MechWarrior-{label}-{platform}"
    stage = output / name
    if stage.exists():
        shutil.rmtree(stage)
    stage.mkdir(parents=True)
    args = ["cmake", "-S", str(root), "-B", str(build),
            "-DMECHWARRIOR_BUILD_SDL=ON", f"-DMECHWARRIOR_SDL_SOURCE_DIR={sdl}"]
    if windows:
        args += ["-A", "x64"]
    else:
        args += ["-DCMAKE_BUILD_TYPE=Release", "-DCMAKE_BUILD_RPATH=$ORIGIN",
                 "-DCMAKE_BUILD_RPATH_USE_ORIGIN=ON"]
    run(*args)
    run("cmake", "--build", str(build), "--config", "Release", "--target", "mechwarrior", "--parallel", "4")
    binary = build / "Release/mechwarrior.exe" if windows else build / "mechwarrior"
    shutil.copy2(binary, stage / binary.name)
    libraries = list(build.rglob("*.dll" if windows else "*.so*"))
    for lib in libraries:
        if lib.is_file():
            destination = stage / lib.name
            if destination.exists() and destination.read_bytes() != lib.read_bytes():
                raise RuntimeError(f"Conflicting runtime libraries: {lib.name}")
            shutil.copy2(lib.resolve(), destination)
    if windows:
        vswhere = Path(os.environ["ProgramFiles(x86)"]) / "Microsoft Visual Studio/Installer/vswhere.exe"
        vs = Path(subprocess.check_output([str(vswhere), "-latest", "-products", "*", "-requires",
                  "Microsoft.VisualStudio.Component.VC.Tools.x86.x64", "-property", "installationPath"], text=True).strip())
        crt_dirs = sorted((vs / "VC/Redist/MSVC").glob("*/x64/Microsoft.VC*.CRT"))
        if not crt_dirs:
            raise RuntimeError("Cannot locate redistributable MSVC runtime")
        for dll in crt_dirs[-1].glob("*.dll"):
            shutil.copy2(dll, stage / dll.name)
    for required in (["SDL3.dll", "mt32emu-*.dll", "nukedopl.dll"] if windows else ["libSDL3.so*", "libmt32emu.so*", "libnukedopl.so*"]):
        if not list(stage.glob(required)):
            raise RuntimeError(f"Missing runtime dependency: {required}")
    for source in [root / "LICENSE", root / "MechWarrior.ini"]:
        shutil.copy2(source, stage / source.name)
    notices = stage / "licenses"
    notices.mkdir()
    shutil.copytree(root / "docs/LICENSES", notices / "project")
    for source in [root / "docs/NOTICE.md", root / "docs/THIRD_PARTY_NOTICES.md", sdl / "LICENSE.txt",
                   build / "_deps/munt-src/mt32emu/COPYING.LESSER.txt", build / "_deps/nukedopl-src/LICENSE"]:
        prefix = "SDL-" if source.parent == sdl else "Munt-" if source.name == "COPYING.LESSER.txt" else "Nuked-" if source.name == "LICENSE" else ""
        shutil.copy2(source, notices / (prefix + source.name))
    (stage / "README.txt").write_text(
        "MechWarrior (1989) experimental SDL preservation port\n\n"
        "An original game installation is required and is not included.\n"
        "No game data, saves or MT-32 ROMs are included.\n"
        "Keep the configuration and shared libraries beside the executable.\n"
        "Run from a writable directory with: mechwarrior <game-directory> [capture-directory]\n"
        "Windows: use mechwarrior.exe. Linux: use ./mechwarrior.\n"
        "Linux target: Ubuntu 24.04 x86-64 (glibc 2.39+) with desktop/audio services.\n"
        "Support pilot is disabled by default. See the repository docs for controls.\n"
        "CI verifies compilation, linking and executable loading, not gameplay.\n",
        encoding="utf-8")
    archive = output / (name + (".zip" if windows else ".tar.gz"))
    if windows:
        with zipfile.ZipFile(archive, "w", zipfile.ZIP_DEFLATED) as target:
            for file in sorted(stage.rglob("*")):
                if file.is_file():
                    target.write(file, file.relative_to(output))
    else:
        with tarfile.open(archive, "w:gz") as target:
            target.add(stage, arcname=name)
    # Exercise the archive's actual layout rather than the build directory.
    extracted = output / "smoke"
    if extracted.exists():
        shutil.rmtree(extracted)
    extracted.mkdir()
    if windows:
        with zipfile.ZipFile(archive) as source:
            source.extractall(extracted)
    else:
        with tarfile.open(archive) as source:
            source.extractall(extracted, filter="data")
    executable = extracted / name / binary.name
    if not windows:
        run("ldd", str(executable))
    result = subprocess.run([str(executable)], cwd=executable.parent, capture_output=True, text=True, timeout=30)
    if result.returncode != 2 or "Usage: mechwarrior" not in result.stderr:
        raise RuntimeError(f"Packaged executable failed to load: {result.returncode}\n{result.stdout}\n{result.stderr}")
    digest = hashlib.sha256(archive.read_bytes()).hexdigest()
    (output / (archive.name + ".sha256")).write_text(f"{digest}  {archive.name}\n", encoding="utf-8")
    print(f"Packaged and startup-checked {archive}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(__doc__)
    parser.add_argument("--sdl", type=Path, required=True)
    parser.add_argument("--label", required=True)
    options = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    package(root, root / "build-package", options.sdl.resolve(), root / "dist", options.label)
