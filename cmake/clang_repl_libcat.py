#!/usr/bin/env python3

# This file is flagrantly "vibe-coded". It may not be up to the standards of most libCat
# code.

"""Emit the `--Xcc` flags clang-repl needs to parse a libCat translation unit.

Reads `<build-dir>/compile_commands.json`, picks a representative `cat-impl`
source, strips the compiler driver / source / output tokens, and filters
out anything `clang-repl` rejects or that is meaningless for JIT parsing:
  * object-file plumbing (`-c`, `-o`, `-M*`),
  * LTO / profile / opt-record codegen flags,
  * CMake's auto-PCH wiring (the `.pch` was built without
    `-fincremental-extensions`, which `clang-repl` enables, so it would
    otherwise refuse to start).

Everything else is preserved so the REPL sees the same include paths,
macro settings, ABI options, sanitizers, and intrinsics that `libcat.so`
was compiled against. Each surviving token is emitted on its own line as
`--Xcc=<token>`, ready to be read into a bash array by the wrapper.

Invoked by the `${CMAKE_BINARY_DIR}/clang-repl-libcat` wrapper that
`cmake/cat_shared.cmake` generates; safe to run standalone too.
"""
from __future__ import annotations

import json
import shlex
import sys
from pathlib import Path


# Clang's driver explodes `-Xclang <arg>` into two positional tokens, so parse the
# command line back into "logical arguments" where each `-Xclang <arg>` pair is a single
# cc1 unit and plain driver flags stay as-is. Filtering then operates on whole units,
# which lets us drop a cc1 flag along with both its `-Xclang` prefix AND any follow-on
# value token it expects (e.g. `-Xclang -include-pch -Xclang <path>`).
def _to_logical(tokens: list[str]) -> list[tuple[str, str]]:
    out: list[tuple[str, str]] = []
    i = 1  # skip compiler driver
    while i < len(tokens):
        if tokens[i] == "-Xclang" and i + 1 < len(tokens):
            out.append(("cc1", tokens[i + 1]))
            i += 2
        else:
            out.append(("plain", tokens[i]))
            i += 1
    return out


_DROP_EXACT = {
    "-c", "-MD", "-MMD",
    # CMake PCH leftovers. `-Winvalid-pch` is harmless to keep but pointless once the
    # `-include-pch` it guards is gone.
    "-Winvalid-pch", "-fno-pch-timestamp",
}
# These flags consume the next logical argument as their value. Drop both.
_VALUE_TAKING = {"-o", "-MT", "-MF", "-MQ", "-include-pch"}
_STRIP_SUBSTRINGS = (
    "-flto", "-fsave-optimization-record",
    "-fprofile-", "-fcs-profile", "-fcoverage-",
)


def _pick_entry(data: list[dict]) -> dict:
    # Prefer a known stable file so flag derivation is deterministic across adds/removes
    # elsewhere in the tree. Fall back to any `cat-impl` object, then to any libCat
    # source.
    preferred = "src/libraries/string/implementations/println.cpp"
    for e in data:
        if e["file"].endswith(preferred):
            return e
    for e in data:
        if "/CMakeFiles/cat-impl.dir/" in e.get("output", ""):
            return e
    for e in data:
        if "/src/libraries/" in e["file"]:
            return e
    sys.exit("compile_commands.json contains no libCat translation unit.")


def build_xcc_args(build_dir: Path) -> list[str]:
    cc_json = build_dir / "compile_commands.json"
    if not cc_json.is_file():
        sys.exit(
            f"{cc_json} not found. Configure with "
            "-DCAT_BUILD_SHARED=ON (which forces "
            "CMAKE_EXPORT_COMPILE_COMMANDS=ON)."
        )
    entry = _pick_entry(json.loads(cc_json.read_text()))
    logical = _to_logical(shlex.split(entry["command"]))

    kept: list[tuple[str, str]] = []
    j = 0
    while j < len(logical):
        _, val = logical[j]
        nxt = logical[j + 1][1] if j + 1 < len(logical) else None

        if val in _DROP_EXACT:
            j += 1
            continue
        if any(s in val for s in _STRIP_SUBSTRINGS):
            j += 1
            continue
        if val.endswith((".cpp", ".cxx", ".cc")):
            j += 1
            continue
        if val in _VALUE_TAKING:
            j += 2
            continue
        # CMake force-includes `cmake_pch.hxx` so Clang picks up its sidecar `.pch`.
        # That `.pch` was not built with `-fincremental-extensions`, so `clang-repl`
        # refuses it. Drop the `-include` and the path that follows.
        if val == "-include" and nxt and "cmake_pch.hxx" in nxt:
            j += 2
            continue

        kept.append(logical[j])
        j += 1

    out: list[str] = []
    for kind, val in kept:
        if kind == "cc1":
            out.extend(["-Xclang", val])
        else:
            out.append(val)
    return out


def main() -> None:
    if len(sys.argv) != 2:
        sys.exit(f"usage: {Path(sys.argv[0]).name} <build-dir>")
    for f in build_xcc_args(Path(sys.argv[1])):
        print(f"--Xcc={f}")


if __name__ == "__main__":
    main()
