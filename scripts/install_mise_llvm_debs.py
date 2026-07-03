#!/usr/bin/env python3

# This file is flagrantly "vibe-coded". It may not be up to the standards of most libCat
#code.

import gzip
import hashlib
import os
import pathlib
import shutil
import subprocess
import sys
import urllib.request


LLVM_BASE_URL = "https://apt.llvm.org/unstable"
LLVM_PACKAGES_INDEX_URL = f"{LLVM_BASE_URL}/dists/llvm-toolchain/main/binary-amd64/Packages.gz"
LLVM_PACKAGE_NAMES = (
    "clang-23",
    "clang-format-23",
    "clangd-23",
    "clang-tidy-23",
    "clang-tools-23",
    "lld-23",
    "liblld-23",
    "llvm-23",
    "llvm-23-linker-tools",
    "llvm-23-runtime",
    "llvm-23-tools",
    "libclang-cpp23",
    # `libclang-rt-23-dev` is needed for sanitizer runtimes.
    "libclang-rt-23-dev",
    "libllvm23",
    "bolt-23",
)
DEBIAN_BASE_URL = "https://deb.debian.org/debian"
DEBIAN_PACKAGES_INDEX_URL = f"{DEBIAN_BASE_URL}/dists/unstable/main/binary-amd64/Packages.gz"
PACKAGE_SET_REVISION = "6"

DEBIAN_PACKAGE_NAMES = (
    "gcc-16-base",
    "libgcc-s1",
    "libstdc++6",
    "libc6",
    "libc-gconv-modules-extra",
    "libabsl20260107",
    "libbsd0",
    "libedit2",
    "libffi8",
    "libmd0",
    "libre2-11",
    "libgrpc++1.51t64",
    "libgrpc29t64",
    "libprotobuf32t64",
    "libprotoc32t64",
    "libcares2",
    "libssl3t64",
    "libxml2-16",
    "libzstd1",
    "libtinfo6",
    "libz3-4",
    "zlib1g",
)

# `LLVM_BIN_DIR` is the directory in the staged toolchain that ships every
# `clang*` / `llvm-*` / BOLT executable. `install_wrappers` enumerates it at
# install time so anything new the upstream debs add (e.g. `llvm-bolt`,
# `merge-fdata`, future tools) lands on PATH automatically.
LLVM_BIN_DIR = "usr/lib/llvm-23/bin"

# Required tools verified by `is_current`. The actual wrapper set is the full
# enumeration of `LLVM_BIN_DIR`; this list just guards against an upstream
# regression silently dropping something we depend on.
REQUIRED_WRAPPERS = (
    "clang",
    "clang++",
    "clang-cpp",
    "clang-format",
    "clangd",
    "clang-tidy",
    "run-clang-tidy",
    "clang-apply-replacements",
    "clang-repl",
    "lld",
    "ld.lld",
    "opt",
    "llvm-ar",
    "llvm-ranlib",
    "llvm-symbolizer",
    "llvm-objdump",
    "llvm-extract",
    "llvm-bolt",
)

RUNTIME_LIBRARIES = (
    "libLLVM-23.so",
    "libLLVM.so.23.0",
    "libre2.so.11",
    "libre2.so.11.0.0",
    "libstdc++.so.6",
    "libcrypto.so.3",
    "libssl.so.3",
    "libxml2.so.16",
)

RUNTIME_LIBRARY_PREFIXES = (
    "libabsl",
    "libaddress_sorting",
    "libcares",
    "libgpr",
    "libgrpc",
    "libprotobuf",
    "libprotoc",
    "libstdc++",
    "libxml2",
    "libupb",
)

REQUIRED_PREFIX_RUNTIME_LIBRARIES = (
    "libabsl_base.so.20260107",
    "libgrpc++.so.1.51",
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


def download_package(downloads: pathlib.Path, base_url: str, url_path: str, size: int, sha256: str) -> pathlib.Path:
    name = pathlib.PurePosixPath(url_path).name
    path = downloads / name
    if path.exists() and path.stat().st_size == size and sha256_file(path) == sha256:
        return path

    url = f"{base_url}/{url_path}"
    print(f"download {url}", flush=True)
    tmp = path.with_suffix(path.suffix + ".tmp")
    with urllib.request.urlopen(url) as response, tmp.open("wb") as file:
        shutil.copyfileobj(response, file)

    actual_size = tmp.stat().st_size
    actual_sha256 = sha256_file(tmp)
    if actual_size != size:
        tmp.unlink(missing_ok=True)
        raise SystemExit(f"{name}: expected {size} bytes, got {actual_size}")
    if actual_sha256 != sha256:
        tmp.unlink(missing_ok=True)
        raise SystemExit(f"{name}: checksum mismatch: {actual_sha256}")
    tmp.replace(path)
    return path


def deb_data_member(path: pathlib.Path) -> tuple[str, bytes]:
    data = path.read_bytes()
    if not data.startswith(b"!<arch>\n"):
        raise SystemExit(f"{path.name}: not a Debian ar archive")

    offset = 8
    while offset + 60 <= len(data):
        header = data[offset : offset + 60]
        name = header[:16].decode("ascii").strip()
        size = int(header[48:58].decode("ascii").strip())
        start = offset + 60
        end = start + size
        member = data[start:end]
        if name.endswith("/"):
            name = name[:-1]
        if name.startswith("data.tar."):
            return name, member
        offset = end + (size % 2)

    raise SystemExit(f"{path.name}: missing data.tar member")


def extract_deb(path: pathlib.Path, destination: pathlib.Path) -> None:
    dpkg_deb = shutil.which("dpkg-deb")
    if dpkg_deb is not None:
        print(f"extract {path.name}", flush=True)
        subprocess.run([dpkg_deb, "-x", str(path), str(destination)], check=True)
        return

    name, payload = deb_data_member(path)
    if name.endswith(".zst"):
        if shutil.which("zstd") is None:
            raise SystemExit(f"{path.name}: extracting {name} requires dpkg-deb or zstd")
        command = ["tar", "--zstd", "-xf", "-", "-C", str(destination)]
    elif name.endswith(".xz"):
        command = ["tar", "-xJf", "-", "-C", str(destination)]
    elif name.endswith(".gz"):
        command = ["tar", "-xzf", "-", "-C", str(destination)]
    else:
        raise SystemExit(f"{path.name}: unsupported data archive {name}")

    print(f"extract {path.name}", flush=True)
    subprocess.run(command, input=payload, check=True)


def wrapper_text(real_path: str) -> str:
    return f"""#!/usr/bin/env bash
set -e
root="$(CDPATH= cd -- "$(dirname -- "${{BASH_SOURCE[0]}}")/.." && pwd)"
target="${{root}}/{real_path}"
library_path="${{root}}/runtime/lib:${{root}}/usr/lib/llvm-23/lib${{LD_LIBRARY_PATH:+:${{LD_LIBRARY_PATH}}}}"
export LD_LIBRARY_PATH="${{library_path}}"
export PATH="${{root}}/usr/lib/llvm-23/bin:${{root}}/usr/bin:${{PATH}}"
exec "${{target}}" "$@"
"""


def install_runtime_library(source_dir: pathlib.Path, runtime_dir: pathlib.Path, library: str) -> None:
    source = source_dir / library
    if not source.exists() and not source.is_symlink():
        raise SystemExit(f"missing expected runtime library: {source}")
    target = runtime_dir / library
    target.unlink(missing_ok=True)
    if source.is_symlink():
        target.symlink_to(os.readlink(source))
    else:
        target.symlink_to(os.path.relpath(source, runtime_dir))


# Debian's `strip` invocation on `libclang-rt-23-dev` clears `sh_link` on the
# `SHT_LLVM_ADDRSIG` sections embedded in every compiler-rt archive (upstream
# llvm/llvm-project#98354). lld then warns once per affected member when
# `--icf=safe` runs against compiler-rt. Removing the broken section silences
# the warning and is a no-op for ICF (lld treats absent addrsig conservatively).
# The wrapper in `bin/` is required (not the raw `usr/lib/llvm-23/bin/`
# binary) because `llvm-objcopy` links against `libLLVM.so.23.0`, and only
# the wrapper sets `LD_LIBRARY_PATH` to the staged `runtime/lib`.
def strip_compiler_rt_addrsig(root: pathlib.Path) -> None:
    objcopy = root / "bin" / "llvm-objcopy"
    if not objcopy.exists():
        raise SystemExit(f"missing llvm-objcopy wrapper: {objcopy}")
    rt_dir = root / "usr" / "lib" / "llvm-23" / "lib" / "clang" / "23" / "lib" / "linux"
    archives = sorted(rt_dir.glob("*.a"))
    if not archives:
        return
    print(f"strip .llvm_addrsig from {len(archives)} compiler-rt archives", flush=True)
    for archive in archives:
        subprocess.run(
            [str(objcopy), "--remove-section=.llvm_addrsig", str(archive)],
            check=True,
        )


def install_runtime_libraries(root: pathlib.Path) -> None:
    source_dir = root / "usr" / "lib" / "x86_64-linux-gnu"
    runtime_dir = root / "runtime" / "lib"
    runtime_dir.mkdir(parents=True, exist_ok=True)
    for library in RUNTIME_LIBRARIES:
        install_runtime_library(source_dir, runtime_dir, library)
    for prefix in RUNTIME_LIBRARY_PREFIXES:
        matches = tuple(source_dir.glob(f"{prefix}*.so*"))
        if not matches:
            raise SystemExit(f"missing runtime libraries with prefix: {prefix}")
        for source in matches:
            install_runtime_library(source_dir, runtime_dir, source.name)


def install_wrappers(root: pathlib.Path) -> None:
    bin_dir = root / "bin"
    bin_dir.mkdir(parents=True, exist_ok=True)
    llvm_bin = root / LLVM_BIN_DIR
    if not llvm_bin.is_dir():
        raise SystemExit(f"missing LLVM bin dir: {llvm_bin}")

    # Stage a wrapper for every executable shipped by the LLVM debs. Skipping
    # `<name>-23` siblings keeps Debian's versioned aliases from getting their
    # own wrappers; we recreate them as symlinks so callers that look for
    # `clang-23`/`llvm-objdump-23` still resolve.
    staged: set[str] = set()
    for entry in sorted(llvm_bin.iterdir()):
        try:
            if not entry.is_file() or not os.access(entry, os.X_OK):
                continue
        except OSError:
            continue
        if entry.name.endswith("-23"):
            continue
        real_path = f"{LLVM_BIN_DIR}/{entry.name}"
        wrapper = bin_dir / entry.name
        wrapper.write_text(wrapper_text(real_path), encoding="utf-8")
        wrapper.chmod(0o755)
        suffixed = bin_dir / f"{entry.name}-23"
        if suffixed.is_symlink() or suffixed.exists():
            suffixed.unlink()
        suffixed.symlink_to(entry.name)
        staged.add(entry.name)

    missing = [name for name in REQUIRED_WRAPPERS if name not in staged]
    if missing:
        raise SystemExit(f"missing expected tool(s): {', '.join(missing)}")


def parse_debian_package_index(index_text: str) -> dict[str, dict[str, str]]:
    packages = {}
    for paragraph in index_text.split("\n\n"):
        fields = {}
        for line in paragraph.splitlines():
            if ": " not in line:
                continue
            key, value = line.split(": ", 1)
            fields[key] = value
        name = fields.get("Package")
        if name is not None:
            packages[name] = fields
    return packages


def resolve_package_index(
    index_url: str,
    base_url: str,
    package_names: tuple[str, ...],
    label: str,
) -> tuple[tuple[tuple[str, str, int, str], ...], tuple[tuple[str, str], ...]]:
    print(f"resolve {index_url}", flush=True)
    with urllib.request.urlopen(index_url) as response:
        packages = parse_debian_package_index(
            gzip.decompress(response.read()).decode("utf-8")
        )

    resolved_packages = []
    resolved_versions = []
    for package_name in package_names:
        fields = packages.get(package_name)
        if fields is None:
            raise SystemExit(f"missing {label} package metadata: {package_name}")
        version = fields["Version"].removeprefix("1:")
        resolved_versions.append((package_name, version))
        resolved_packages.append(
            (
                base_url,
                fields["Filename"],
                int(fields["Size"]),
                fields["SHA256"],
            )
        )

    return tuple(resolved_packages), tuple(resolved_versions)


def resolve_llvm_packages() -> tuple[str, tuple[tuple[str, str, int, str], ...]]:
    resolved_packages, package_versions = resolve_package_index(
        LLVM_PACKAGES_INDEX_URL,
        LLVM_BASE_URL,
        LLVM_PACKAGE_NAMES,
        "LLVM",
    )
    resolved_versions = set()
    for package_name, version in package_versions:
        resolved_versions.add(version)

    if len(resolved_versions) != 1:
        versions = ", ".join(sorted(resolved_versions))
        raise SystemExit(f"LLVM package metadata has mixed versions: {versions}")

    llvm_version = resolved_versions.pop()
    print(f"LLVM nightly {llvm_version}", flush=True)
    return llvm_version, resolved_packages


def resolve_debian_packages() -> tuple[str, tuple[tuple[str, str, int, str], ...]]:
    resolved_packages, package_versions = resolve_package_index(
        DEBIAN_PACKAGES_INDEX_URL,
        DEBIAN_BASE_URL,
        DEBIAN_PACKAGE_NAMES,
        "Debian",
    )
    version_text = "\n".join(f"{name}={version}" for name, version in package_versions)
    version_hash = hashlib.sha256(version_text.encode()).hexdigest()[:16]
    print(f"Debian runtime set {version_hash}", flush=True)
    return version_hash, resolved_packages


def is_current(root: pathlib.Path, stamp_version: str) -> bool:
    stamp = root / "VERSION"
    return (
        stamp.exists()
        and stamp.read_text(encoding="utf-8").strip() == stamp_version
        and all((root / "bin" / command).exists() for command in REQUIRED_WRAPPERS)
        and all((root / "runtime" / "lib" / library).exists() for library in RUNTIME_LIBRARIES)
        and all((root / "runtime" / "lib" / library).exists() for library in REQUIRED_PREFIX_RUNTIME_LIBRARIES)
    )


def main() -> int:
    llvm_version, llvm_packages = resolve_llvm_packages()
    debian_version, debian_packages = resolve_debian_packages()
    stamp_version = f"{llvm_version}+debian.{debian_version}+cat.{PACKAGE_SET_REVISION}"
    configured_version = os.environ.get("CAT_LLVM_VERSION")
    if configured_version not in (None, "unstable") and configured_version != llvm_version:
        raise SystemExit(
            f"mise CAT_LLVM_VERSION is {configured_version}, "
            f"but package metadata is {llvm_version}"
        )

    root = project_root() / ".cache" / "cat-llvm"
    downloads = project_root() / ".cache" / "cat-llvm-downloads"
    downloads.mkdir(parents=True, exist_ok=True)

    if is_current(root, stamp_version):
        subprocess.run([str(root / "bin" / "clang++"), "--version"], check=True)
        return 0

    if root.exists():
        shutil.rmtree(root)
    root.mkdir(parents=True)

    for base_url, url_path, size, sha256 in llvm_packages + debian_packages:
        package = download_package(downloads, base_url, url_path, size, sha256)
        extract_deb(package, root)

    install_runtime_libraries(root)
    install_wrappers(root)
    strip_compiler_rt_addrsig(root)
    (root / "VERSION").write_text(stamp_version + "\n", encoding="utf-8")

    subprocess.run([str(root / "bin" / "clang++"), "--version"], check=True)
    subprocess.run([str(root / "bin" / "clangd"), "--version"], check=True)
    subprocess.run([str(root / "bin" / "ld.lld"), "--version"], check=True)
    subprocess.run([str(root / "bin" / "clang-format"), "--version"], check=True)
    return 0


if __name__ == "__main__":
    sys.exit(main())
