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
    "llvm-23-dev",
    "llvm-23-linker-tools",
    "llvm-23-runtime",
    "llvm-23-tools",
    "libclang-23-dev",
    "libclang-common-23-dev",
    "libclang-cpp23",
    "libclang-rt-23-dev",
    "libclang1-23",
    "libllvm23",
)
DEBIAN_BASE_URL = "https://deb.debian.org/debian"
PACKAGE_SET_REVISION = "3"

PINNED_DEBIAN_PACKAGES = (
    (
        DEBIAN_BASE_URL,
        "pool/main/g/gcc-16/gcc-16-base_16-20260425-1_amd64.deb",
        35772,
        "4a7750990b378d0070bddd1b58aeaf9e4ebf7ec4d86340b25c715f36ecd86c6b",
    ),
    (
        DEBIAN_BASE_URL,
        "pool/main/g/gcc-16/libgcc-s1_16-20260425-1_amd64.deb",
        74016,
        "5ec8f55d82cdd8f1e4233010f26db877ec8c0c8fa9170bf91a84cbdef6636d39",
    ),
    (
        DEBIAN_BASE_URL,
        "pool/main/g/gcc-16/libstdc++6_16-20260425-1_amd64.deb",
        824052,
        "fea11955e18830d68c7535e295c638239ced00e8385bf0c5fd2e1d2a8c5ea7ee",
    ),
    (
        DEBIAN_BASE_URL,
        "pool/main/g/glibc/libc6_2.42-15_amd64.deb",
        1793792,
        "c15097e0318c663d076d87070df8352586de522fedf953881b7d0404b41058a9",
    ),
    (
        DEBIAN_BASE_URL,
        "pool/main/g/glibc/libc-gconv-modules-extra_2.42-15_amd64.deb",
        1101744,
        "aa7b969c40c886841b6d70cef3ee7ce13c467fae8e124492d0500ced72d513aa",
    ),
    (
        DEBIAN_BASE_URL,
        "pool/main/a/abseil/libabsl20260107_20260107.0-5_amd64.deb",
        549164,
        "78ff51f81522f068c393f0427d2cadec727508f19654ef1f9928075ad142f181",
    ),
    (
        DEBIAN_BASE_URL,
        "pool/main/libb/libbsd/libbsd0_0.12.2-2+b2_amd64.deb",
        131984,
        "ab3328a6a9c162f7ea4c1013189f4f9ed0596382fafb16e49ba1db8e362b5214",
    ),
    (
        DEBIAN_BASE_URL,
        "pool/main/libe/libedit/libedit2_3.1-20251016-1+b1_amd64.deb",
        93280,
        "1ce11f16c6bbb49481a961bd6ad07a002f1aa9468c8e1cf0c8f58bfe00be80ec",
    ),
    (
        DEBIAN_BASE_URL,
        "pool/main/libf/libffi/libffi8_3.5.2-4_amd64.deb",
        25180,
        "1883c3b720014b5fd7bed17f3a27afdc5af079517d762192f687b8dd407e5820",
    ),
    (
        DEBIAN_BASE_URL,
        "pool/main/libm/libmd/libmd0_1.1.0-2+b2_amd64.deb",
        36248,
        "3cb2308445d53d7e1108c83f0b82eea06e284668a03579d10d59106c31028d42",
    ),
    (
        DEBIAN_BASE_URL,
        "pool/main/r/re2/libre2-11_20251105-1_amd64.deb",
        168864,
        "f04d7a8b2176f9c193ec23053742ba2c29b2028bbc861d54d4ee2dee16084b12",
    ),
    (
        DEBIAN_BASE_URL,
        "pool/main/g/grpc/libgrpc++1.51t64_1.51.1-9_amd64.deb",
        498928,
        "cd9c3d8938b659fdbb57d052a7f24605f010b8396b733e2f0ff997110c7dcd07",
    ),
    (
        DEBIAN_BASE_URL,
        "pool/main/g/grpc/libgrpc29t64_1.51.1-9_amd64.deb",
        2892780,
        "a05fc5bc2621730f129786a947257e9293163766a8f52aa9bb76619c07ec082c",
    ),
    (
        DEBIAN_BASE_URL,
        "pool/main/p/protobuf/libprotobuf32t64_3.21.12-15+b1_amd64.deb",
        986364,
        "b5b6f0f29cc06b598a5d74dd463578ed9a1cf6e28bd61b5871f440780e195953",
    ),
    (
        DEBIAN_BASE_URL,
        "pool/main/p/protobuf/libprotoc32t64_3.21.12-15+b1_amd64.deb",
        937456,
        "672f41ecc298aee1357ddf03555f590870c4d89d4f21f135696dce95ca45ba81",
    ),
    (
        DEBIAN_BASE_URL,
        "pool/main/c/c-ares/libcares2_1.34.6-1+b1_amd64.deb",
        99132,
        "2b7dd3c2b8f34ae1466de09f91722724d9515b495f27ae99d2369aad1da4885d",
    ),
    (
        DEBIAN_BASE_URL,
        "pool/main/o/openssl/libssl3t64_3.6.2-1_amd64.deb",
        2482492,
        "a34acac9b52d64498ce8d98e23597ebbc9883af5f8c4fcf7802450945fb4c2d0",
    ),
    (
        DEBIAN_BASE_URL,
        "pool/main/libx/libxml2/libxml2-16_2.15.2+dfsg-0.1_amd64.deb",
        641024,
        "8571682a07f329bb462569502b57aced4866e5b95c2db3ec7e5414a5b3bbdc14",
    ),
    (
        DEBIAN_BASE_URL,
        "pool/main/libz/libzstd/libzstd1_1.5.7+dfsg-3+b2_amd64.deb",
        304640,
        "5ce884018be1a8bd7a3beb0db95c7b18e1d49246afdfd08889bc9faa48375933",
    ),
    (
        DEBIAN_BASE_URL,
        "pool/main/n/ncurses/libtinfo6_6.6+20251231-1+b1_amd64.deb",
        350360,
        "adc32779d98abd9fc7a0a76569abd114fe74e9fea8fa9da88a246b09295553ff",
    ),
    (
        DEBIAN_BASE_URL,
        "pool/main/z/z3/libz3-4_4.13.3-1+b1_amd64.deb",
        8646192,
        "6d3c96a1cfd28a8bd98647a8668857732b473e4e5281392c6d969314db1f2732",
    ),
    (
        DEBIAN_BASE_URL,
        "pool/main/z/zlib/zlib1g_1.3.dfsg+really1.3.2-3_amd64.deb",
        90912,
        "52c585b07bea72ef36df9ddd5d1937f4739d3caec057d827954baec256292651",
    ),
)

WRAPPERS = (
    ("clang", "usr/lib/llvm-23/bin/clang"),
    ("clang++", "usr/lib/llvm-23/bin/clang++"),
    ("clang-cpp", "usr/lib/llvm-23/bin/clang-cpp"),
    ("clang-format", "usr/lib/llvm-23/bin/clang-format"),
    ("clangd", "usr/lib/llvm-23/bin/clangd"),
    ("clang-tidy", "usr/lib/llvm-23/bin/clang-tidy"),
    ("run-clang-tidy", "usr/lib/llvm-23/bin/run-clang-tidy"),
    ("clang-apply-replacements", "usr/lib/llvm-23/bin/clang-apply-replacements"),
    ("clang-repl", "usr/lib/llvm-23/bin/clang-repl"),
    ("lld", "usr/lib/llvm-23/bin/lld"),
    ("ld.lld", "usr/lib/llvm-23/bin/ld.lld"),
    ("llvm-ar", "usr/lib/llvm-23/bin/llvm-ar"),
    ("llvm-ranlib", "usr/lib/llvm-23/bin/llvm-ranlib"),
    ("llvm-symbolizer", "usr/lib/llvm-23/bin/llvm-symbolizer"),
)

RUNTIME_LIBRARIES = (
    "libLLVM-23.so",
    "libLLVM.so.23.0",
    "libclang-23.so",
    "libclang-23.so.1",
    "libclang-23.so.23",
    "libclang-cpp.so.23.0",
    "libre2.so.11",
    "libre2.so.11.0.0",
    "libstdc++.so.6",
    "libstdc++.so.6.0.35",
    "libcrypto.so.3",
    "libssl.so.3",
    "libxml2.so.16",
    "libxml2.so.16.1.2",
)

RUNTIME_LIBRARY_PREFIXES = (
    "libabsl",
    "libaddress_sorting",
    "libcares",
    "libgpr",
    "libgrpc",
    "libprotobuf",
    "libprotoc",
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
    for command, real_path in WRAPPERS:
        target = root / real_path
        if not target.exists():
            raise SystemExit(f"missing expected tool: {target}")
        wrapper = bin_dir / command
        wrapper.write_text(wrapper_text(real_path), encoding="utf-8")
        wrapper.chmod(0o755)
        suffixed = bin_dir / f"{command}-23"
        if not suffixed.exists():
            suffixed.symlink_to(command)


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


def resolve_llvm_packages() -> tuple[str, tuple[tuple[str, str, int, str], ...]]:
    print(f"resolve {LLVM_PACKAGES_INDEX_URL}", flush=True)
    with urllib.request.urlopen(LLVM_PACKAGES_INDEX_URL) as response:
        packages = parse_debian_package_index(
            gzip.decompress(response.read()).decode("utf-8")
        )

    resolved_packages = []
    resolved_versions = set()
    for package_name in LLVM_PACKAGE_NAMES:
        fields = packages.get(package_name)
        if fields is None:
            raise SystemExit(f"missing LLVM package metadata: {package_name}")
        version = fields["Version"].removeprefix("1:")
        resolved_versions.add(version)
        resolved_packages.append(
            (
                LLVM_BASE_URL,
                fields["Filename"],
                int(fields["Size"]),
                fields["SHA256"],
            )
        )

    if len(resolved_versions) != 1:
        versions = ", ".join(sorted(resolved_versions))
        raise SystemExit(f"LLVM package metadata has mixed versions: {versions}")

    llvm_version = resolved_versions.pop()
    print(f"LLVM nightly {llvm_version}", flush=True)
    return llvm_version, tuple(resolved_packages)


def is_current(root: pathlib.Path, stamp_version: str) -> bool:
    stamp = root / "VERSION"
    return (
        stamp.exists()
        and stamp.read_text(encoding="utf-8").strip() == stamp_version
        and all((root / "bin" / command).exists() for command, _ in WRAPPERS)
        and all((root / "runtime" / "lib" / library).exists() for library in RUNTIME_LIBRARIES)
        and all((root / "runtime" / "lib" / library).exists() for library in REQUIRED_PREFIX_RUNTIME_LIBRARIES)
    )


def main() -> int:
    llvm_version, llvm_packages = resolve_llvm_packages()
    stamp_version = f"{llvm_version}+cat.{PACKAGE_SET_REVISION}"
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

    for base_url, url_path, size, sha256 in llvm_packages + PINNED_DEBIAN_PACKAGES:
        package = download_package(downloads, base_url, url_path, size, sha256)
        extract_deb(package, root)

    install_runtime_libraries(root)
    install_wrappers(root)
    (root / "VERSION").write_text(stamp_version + "\n", encoding="utf-8")

    subprocess.run([str(root / "bin" / "clang++"), "--version"], check=True)
    subprocess.run([str(root / "bin" / "clangd"), "--version"], check=True)
    subprocess.run([str(root / "bin" / "ld.lld"), "--version"], check=True)
    subprocess.run([str(root / "bin" / "clang-format"), "--version"], check=True)
    return 0


if __name__ == "__main__":
    sys.exit(main())
