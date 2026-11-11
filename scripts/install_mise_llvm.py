#!/usr/bin/env python3

# Download and stage the pinned Clang toolchain from the `lilian` branch of
# Cons-Cat/llvm-project. Each GitHub Release there is built by
# `lilian/build-release.sh` in manylinux_2_28, so it runs on any distribution
# with GlibC 2.28 or newer. `CAT_LLVM_VERSION` (the release tag) and
# `CAT_LLVM_SHA256` are pinned in `mise.toml`.

import hashlib
import os
import pathlib
import shutil
import subprocess
import sys
import urllib.request


RELEASES_URL = "https://github.com/Cons-Cat/llvm-project/releases/download"

REQUIRED_TOOLS = (
    "clang",
    "clang++",
    "clang-cl",
    "clang-format",
    "clangd",
    "clang-tidy",
    "run-clang-tidy",
    "clang-apply-replacements",
    "clang-repl",
    "lld",
    "ld.lld",
    "lld-link",
    "opt",
    "llvm-ar",
    "llvm-ranlib",
    "llvm-symbolizer",
    "llvm-objdump",
    "llvm-extract",
)


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


def download(url: str, path: pathlib.Path, sha256: str) -> None:
    if path.exists() and sha256_file(path) == sha256:
        return
    print(f"download {url}", flush=True)
    tmp = path.with_suffix(path.suffix + ".tmp")
    with urllib.request.urlopen(url) as response, tmp.open("wb") as file:
        shutil.copyfileobj(response, file)
    actual = sha256_file(tmp)
    if actual != sha256:
        tmp.unlink(missing_ok=True)
        raise SystemExit(f"{path.name}: checksum mismatch: {actual}")
    tmp.replace(path)


def is_current(root: pathlib.Path, tag: str) -> bool:
    stamp = root / "VERSION"
    return (
        stamp.exists()
        and stamp.read_text(encoding="utf-8").strip() == tag
        and all((root / "bin" / tool).exists() for tool in REQUIRED_TOOLS)
    )


def main() -> int:
    tag = os.environ["CAT_LLVM_VERSION"]
    sha256 = os.environ["CAT_LLVM_SHA256"]
    revision = tag.removeprefix("lilian-")
    name = f"llvm-lilian-{revision}-x86_64-linux.tar.zst"

    root = project_root() / ".cache" / "cat-llvm"
    downloads = project_root() / ".cache" / "cat-llvm-downloads"
    downloads.mkdir(parents=True, exist_ok=True)

    if not is_current(root, tag):
        archive = downloads / name
        download(f"{RELEASES_URL}/{tag}/{name}", archive, sha256)
        if root.exists():
            shutil.rmtree(root)
        root.mkdir(parents=True)
        print(f"extract {name}", flush=True)
        subprocess.run(
            ["tar", "--zstd", "-xf", str(archive), "-C", str(root), "--strip-components=1"],
            check=True,
        )
        missing = [tool for tool in REQUIRED_TOOLS if not (root / "bin" / tool).exists()]
        if missing:
            raise SystemExit(f"missing expected tool(s): {', '.join(missing)}")        (root / "VERSION").write_text(tag + "\n", encoding="utf-8")

    for tool in ("clang++", "clangd", "ld.lld", "clang-format"):
        subprocess.run([str(root / "bin" / tool), "--version"], check=True)
    return 0


if __name__ == "__main__":
    sys.exit(main())
