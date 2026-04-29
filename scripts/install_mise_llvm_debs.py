#!/usr/bin/env python3

# This file is flagrantly "vibe-coded". It may not be up to the standards of most libCat
#code.

import hashlib
import os
import pathlib
import shutil
import subprocess
import sys
import urllib.request


LLVM_BASE_URL = "https://apt.llvm.org/unstable/pool/main/l/llvm-toolchain-snapshot"
DEBIAN_BASE_URL = "https://deb.debian.org/debian"
VERSION = "23~++20260428111539+a24c1dedc6f5-1~exp1~20260428111719.3532"

PACKAGES = (
    (
        LLVM_BASE_URL,
        "clang-23_23~%2B%2B20260428111539%2Ba24c1dedc6f5-1~exp1~20260428111719.3532_amd64.deb",
        140172,
        "371b4ab1a77df9404aef77f53937133093f59be06133439f4ca5117fa3272487",
    ),
    (
        LLVM_BASE_URL,
        "clang-format-23_23~%2B%2B20260428111539%2Ba24c1dedc6f5-1~exp1~20260428111719.3532_amd64.deb",
        70520,
        "e42c296ecf0c32ec68e80bb492a08bc8b3443250a327a61a6c6c204b22a9c801",
    ),
    (
        LLVM_BASE_URL,
        "clangd-23_23~%2B%2B20260428111539%2Ba24c1dedc6f5-1~exp1~20260428111719.3532_amd64.deb",
        3465740,
        "93b1d66bd57f207b8ddd409e764ae0f311921be8a6fa27f69c7053c55b497f38",
    ),
    (
        LLVM_BASE_URL,
        "clang-tidy-23_23~%2B%2B20260428111539%2Ba24c1dedc6f5-1~exp1~20260428111719.3532_amd64.deb",
        2024836,
        "78ac69efb6366494339e876c71d9bd863476077bbf8b7b139d092a78b01c910e",
    ),
    (
        LLVM_BASE_URL,
        "clang-tools-23_23~%2B%2B20260428111539%2Ba24c1dedc6f5-1~exp1~20260428111719.3532_amd64.deb",
        9671104,
        "bc84cc445942dbe4a8506f85fcb326705f8b5bbf929412f4c9b26d6d2d58b743",
    ),
    (
        LLVM_BASE_URL,
        "lld-23_23~%2B%2B20260428111539%2Ba24c1dedc6f5-1~exp1~20260428111719.3532_amd64.deb",
        1486804,
        "3491f972902dc52cdab2b89850e1babc2b1cdd5028cc1f902af4627719c362cb",
    ),
    (
        LLVM_BASE_URL,
        "liblld-23_23~%2B%2B20260428111539%2Ba24c1dedc6f5-1~exp1~20260428111719.3532_amd64.deb",
        1902176,
        "db5e09e6971bf6465fd9400ca95a5da598547b7e6e19a6fffede37829095df78",
    ),
    (
        LLVM_BASE_URL,
        "llvm-23_23~%2B%2B20260428111539%2Ba24c1dedc6f5-1~exp1~20260428111719.3532_amd64.deb",
        19362404,
        "cc572ce5ef20ca57cb0275cf31a05ad008514ce042133bba72c2fa4c4e0e7823",
    ),
    (
        LLVM_BASE_URL,
        "llvm-23-dev_23~%2B%2B20260428111539%2Ba24c1dedc6f5-1~exp1~20260428111719.3532_amd64.deb",
        50766480,
        "363f640e7595fc5f1bb5f2713f8b982bdd434f9c5fe01c84b4ca4a04622ec1e6",
    ),
    (
        LLVM_BASE_URL,
        "llvm-23-linker-tools_23~%2B%2B20260428111539%2Ba24c1dedc6f5-1~exp1~20260428111719.3532_amd64.deb",
        1220096,
        "be6f5083a28e0be7a2bb962dca2af890eb9500b380b10481d59f44796b3ed9fc",
    ),
    (
        LLVM_BASE_URL,
        "llvm-23-runtime_23~%2B%2B20260428111539%2Ba24c1dedc6f5-1~exp1~20260428111719.3532_amd64.deb",
        576056,
        "ba3b4082b0c1d54d364a914a5895e58e062fad21eada4f3282ab31c829210b11",
    ),
    (
        LLVM_BASE_URL,
        "llvm-23-tools_23~%2B%2B20260428111539%2Ba24c1dedc6f5-1~exp1~20260428111719.3532_amd64.deb",
        465552,
        "7ad4264ac5111aba1bd03d184fad2c654d14426d98cb5086e06c682d89bbc84f",
    ),
    (
        LLVM_BASE_URL,
        "libclang-23-dev_23~%2B%2B20260428111539%2Ba24c1dedc6f5-1~exp1~20260428111719.3532_amd64.deb",
        32701072,
        "566a265c31c16bd1dc986c2900406e8b93e3a2da32b08d2fab4d5e2cfabfa65b",
    ),
    (
        LLVM_BASE_URL,
        "libclang-common-23-dev_23~%2B%2B20260428111539%2Ba24c1dedc6f5-1~exp1~20260428111719.3532_amd64.deb",
        776992,
        "55378e8213769b91477ec1f2f96f7a4c0468f4d365369a1c3f1eff1b209265c9",
    ),
    (
        LLVM_BASE_URL,
        "libclang-cpp23_23~%2B%2B20260428111539%2Ba24c1dedc6f5-1~exp1~20260428111719.3532_amd64.deb",
        13676972,
        "074083158e24666785c9faef3aebd87f4314f3f537afc924944be3154ab2e65a",
    ),
    (
        LLVM_BASE_URL,
        "libclang-rt-23-dev_23~%2B%2B20260428111539%2Ba24c1dedc6f5-1~exp1~20260428111719.3532_amd64.deb",
        4212856,
        "495180c4000c74c99f897d5d5299e62f15d353381c5115bed36e702c34bd73bb",
    ),
    (
        LLVM_BASE_URL,
        "libclang1-23_23~%2B%2B20260428111539%2Ba24c1dedc6f5-1~exp1~20260428111719.3532_amd64.deb",
        8448504,
        "088ceea88463f7f458779fbf99390a809494ee2aa2c78e75cc55f78b00dcdd6e",
    ),
    (
        LLVM_BASE_URL,
        "libllvm23_23~%2B%2B20260428111539%2Ba24c1dedc6f5-1~exp1~20260428111719.3532_amd64.deb",
        30112128,
        "0a16a9c3cc11c60f4d9c597549596e2d54931d473cba349405bbf1e1a6efdbff",
    ),
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
    "libxml2.so.16",
    "libxml2.so.16.1.2",
)

RUNTIME_LIBRARY_PREFIXES = (
    "libabsl",
)

REQUIRED_PREFIX_RUNTIME_LIBRARIES = (
    "libabsl_base.so.20260107",
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


def is_current(root: pathlib.Path) -> bool:
    stamp = root / "VERSION"
    return (
        stamp.exists()
        and stamp.read_text(encoding="utf-8").strip() == VERSION
        and all((root / "bin" / command).exists() for command, _ in WRAPPERS)
        and all((root / "runtime" / "lib" / library).exists() for library in RUNTIME_LIBRARIES)
        and all((root / "runtime" / "lib" / library).exists() for library in REQUIRED_PREFIX_RUNTIME_LIBRARIES)
    )


def main() -> int:
    configured_version = os.environ.get("CAT_LLVM_VERSION")
    if configured_version is not None and configured_version != VERSION:
        raise SystemExit(f"mise CAT_LLVM_VERSION is {configured_version}, but package metadata is {VERSION}")

    root = project_root() / ".cache" / "cat-llvm"
    downloads = project_root() / ".cache" / "cat-llvm-downloads"
    downloads.mkdir(parents=True, exist_ok=True)

    if is_current(root):
        subprocess.run([str(root / "bin" / "clang++"), "--version"], check=True)
        return 0

    if root.exists():
        shutil.rmtree(root)
    root.mkdir(parents=True)

    for base_url, url_path, size, sha256 in PACKAGES:
        package = download_package(downloads, base_url, url_path, size, sha256)
        extract_deb(package, root)

    install_runtime_libraries(root)
    install_wrappers(root)
    (root / "VERSION").write_text(VERSION + "\n", encoding="utf-8")

    subprocess.run([str(root / "bin" / "clang++"), "--version"], check=True)
    subprocess.run([str(root / "bin" / "clangd"), "--version"], check=True)
    subprocess.run([str(root / "bin" / "ld.lld"), "--version"], check=True)
    subprocess.run([str(root / "bin" / "clang-format"), "--version"], check=True)
    return 0


if __name__ == "__main__":
    sys.exit(main())
