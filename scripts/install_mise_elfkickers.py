#!/usr/bin/env python3

# Download, build, and stage the pinned ELFkickers utilities (`sstrip` and
# `elfls`) so they're on PATH the same way the bundled Clang nightly is. The
# upstream `make all` still builds the full set; we only copy the two we use.
#
# Modeled after `install_mise_llvm_debs.py`. ELFkickers ships only as a tiny
# source tarball, so the script downloads it, verifies size + sha256, builds
# every program with the system C compiler (`cc`), and copies the binaries
# into `.cache/cat-elfkickers/bin/`. A `VERSION` stamp guards re-runs.

import hashlib
import os
import pathlib
import re
import shutil
import subprocess
import sys
import tarfile
import urllib.request


ELFKICKERS_VERSION = "3.2"
ELFKICKERS_URL = (
    "http://www.muppetlabs.com/~breadbox/pub/software/"
    f"ELFkickers-{ELFKICKERS_VERSION}.tar.gz"
)
ELFKICKERS_SIZE = 108555
ELFKICKERS_SHA256 = (
    "9b81e6c53e0c94fc198d9882eb737156f36d565152dc32118897c77b06a2687c"
)
ELFKICKERS_PROGRAMS = (
    "elfls",
    "sstrip",
)

# Bumped whenever this script's output layout changes (new binaries, new
# install path, etc.). Bumping forces a re-stage even if the upstream archive
# hasn't moved.
PACKAGE_REVISION = "3"


# Each per-program Makefile in the tarball declares its own
# `CFLAGS = -Wall -Wextra -I../elfrw` (or similar). A command-line
# `CFLAGS=-w` override would win, but it would also wipe `-I../elfrw`
# and break `elfls`/`sstrip`'s include resolution for `elfrw.h`.
# Patching the line in-place appends `-w` while preserving the include
# paths the upstream Makefiles set up.
def _silence_makefile_warnings(source_dir: pathlib.Path) -> None:
    pattern = re.compile(r"^(CFLAGS\s*=\s*[^\n]*?)\s*$", flags=re.MULTILINE)
    for mkfile in source_dir.rglob("Makefile"):
        text = mkfile.read_text(encoding="utf-8")
        new_text, count = pattern.subn(r"\1 -w", text)
        if count:
            mkfile.write_text(new_text, encoding="utf-8")


def project_root() -> pathlib.Path:
    root = os.environ.get("MISE_CONFIG_ROOT")
    if root is not None:
        return pathlib.Path(root).resolve()
    return pathlib.Path(__file__).resolve().parents[1]


def sha256_file(path: pathlib.Path) -> str:
    hasher = hashlib.sha256()
    with path.open("rb") as file:
        for chunk in iter(lambda: file.read(1024 * 1024), b""):
            hasher.update(chunk)
    return hasher.hexdigest()


def download_archive(downloads: pathlib.Path) -> pathlib.Path:
    name = pathlib.PurePosixPath(ELFKICKERS_URL).name
    path = downloads / name
    if (
        path.exists()
        and path.stat().st_size == ELFKICKERS_SIZE
        and sha256_file(path) == ELFKICKERS_SHA256
    ):
        return path

    print(f"download {ELFKICKERS_URL}", flush=True)
    tmp = path.with_suffix(path.suffix + ".tmp")
    with urllib.request.urlopen(ELFKICKERS_URL) as response, tmp.open("wb") as file:
        shutil.copyfileobj(response, file)

    actual_size = tmp.stat().st_size
    actual_sha256 = sha256_file(tmp)
    if actual_size != ELFKICKERS_SIZE:
        tmp.unlink(missing_ok=True)
        raise SystemExit(
            f"{name}: expected {ELFKICKERS_SIZE} bytes, got {actual_size}"
        )
    if actual_sha256 != ELFKICKERS_SHA256:
        tmp.unlink(missing_ok=True)
        raise SystemExit(f"{name}: checksum mismatch: {actual_sha256}")
    tmp.replace(path)
    return path


def find_compiler() -> str:
    # ELFkickers' Makefiles default to `cc`. The bundled Clang at
    # `.cache/cat-llvm/bin/clang` won't work here because the staged toolchain
    # ships the compiler-rt-only resource directory (no `stddef.h`); freestanding
    # C builds against the host headers need a host C compiler. Use whatever
    # `cc` (or `gcc`) is on PATH.
    for candidate in ("cc", "gcc"):
        found = shutil.which(candidate)
        if found is not None:
            return found
    raise SystemExit("no C compiler found on PATH (looked for `cc`, `gcc`)")


def is_current(root: pathlib.Path, stamp: str) -> bool:
    version_file = root / "VERSION"
    return (
        version_file.exists()
        and version_file.read_text(encoding="utf-8").strip() == stamp
        and all(
            (root / "bin" / program).exists() for program in ELFKICKERS_PROGRAMS
        )
    )


def main() -> int:
    project = project_root()
    stamp = f"{ELFKICKERS_VERSION}+cat.{PACKAGE_REVISION}"

    root = project / ".cache" / "cat-elfkickers"
    downloads = project / ".cache" / "cat-elfkickers-downloads"
    downloads.mkdir(parents=True, exist_ok=True)

    if is_current(root, stamp):
        print(f"ELFkickers {ELFKICKERS_VERSION} already staged", flush=True)
        return 0

    if root.exists():
        shutil.rmtree(root)
    root.mkdir(parents=True)

    archive = download_archive(downloads)

    build_dir = downloads / f"ELFkickers-{ELFKICKERS_VERSION}-build"
    if build_dir.exists():
        shutil.rmtree(build_dir)
    build_dir.mkdir(parents=True)

    print(f"extract {archive.name}", flush=True)
    with tarfile.open(archive, "r:gz") as tar:
        tar.extractall(build_dir)

    source_dir = build_dir / f"ELFkickers-{ELFKICKERS_VERSION}"
    if not source_dir.is_dir():
        raise SystemExit(f"unexpected archive layout: {source_dir}")

    cc = find_compiler()
    # `-w` disables all warnings. ELFkickers is third-party C from 2014 and
    # warns liberally on a modern host toolchain. Suppressing keeps CI logs
    # focused on libCat's own diagnostics. We patch each Makefile rather
    # than overriding `CFLAGS=` on the command line, because each subdir's
    # `CFLAGS = -Wall -Wextra -I../elfrw` carries the include path the
    # actual compile depends on -- a command-line override wipes it.
    _silence_makefile_warnings(source_dir)
    print(f"build ELFkickers with CC={cc}", flush=True)
    subprocess.run(
        ["make", f"CC={cc}", "all"],
        cwd=source_dir,
        check=True,
    )

    bin_dir = root / "bin"
    bin_dir.mkdir(parents=True, exist_ok=True)
    for program in ELFKICKERS_PROGRAMS:
        source_binary = source_dir / "bin" / program
        if not source_binary.exists():
            raise SystemExit(f"build did not produce: {source_binary}")
        target = bin_dir / program
        shutil.copy2(source_binary, target)
        target.chmod(0o755)

    (root / "VERSION").write_text(stamp + "\n", encoding="utf-8")
    print(
        f"installed ELFkickers {ELFKICKERS_VERSION} -> {bin_dir}", flush=True
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
