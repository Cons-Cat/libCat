#!/usr/bin/env python3

import pathlib
import sys

from status_cxx_flags import NA, cache_value, clang_help_options, describe_flag, short_flag


def block_value(lines: list[str], start: int, name: str) -> str:
    prefix = f"  {name} = "
    for line in lines[start + 1 :]:
        if line.startswith("build "):
            break
        if line.startswith(prefix):
            return line.removeprefix(prefix).strip()
    return ""


def find_link_block(lines: list[str], config: str) -> int | None:
    targets = []
    if config:
        targets.extend(
            (
                f"build examples/{config}/hello:",
                f"build tests/{config}/unit_tests:",
            )
        )
    targets.extend(("build examples/hello:", "build tests/unit_tests:"))

    for target in targets:
        for index, line in enumerate(lines):
            if line.startswith(target):
                return index
    return None


def link_file(build_dir: pathlib.Path, config: str) -> pathlib.Path:
    if config:
        return build_dir / "CMakeFiles" / f"impl-{config}.ninja"
    return build_dir / "build.ninja"


def link_values(build_dir: pathlib.Path, config: str) -> tuple[str, str]:
    path = link_file(build_dir, config)
    if not path.exists():
        return "", NA

    lines = path.read_text(encoding="utf-8").splitlines()
    start = find_link_block(lines, config)
    if start is None:
        return "", NA

    flags = block_value(lines, start, "LINK_FLAGS")
    libraries = block_value(lines, start, "LINK_LIBRARIES") or NA
    return flags, libraries


def selected_linker(flags: str) -> str:
    for flag in flags.split():
        if flag.startswith("-fuse-ld="):
            return flag.removeprefix("-fuse-ld=")
    return NA


def link_flags_without_linker(flags: str) -> str:
    return " ".join(flag for flag in flags.split() if not flag.startswith("-fuse-ld="))


def display_path(path: str) -> str:
    candidate = pathlib.Path(path)
    if not candidate.is_absolute():
        return path

    try:
        return str(candidate.resolve(strict=False).relative_to(pathlib.Path.cwd().resolve()))
    except ValueError:
        return path


def display_linker_parts(parts: list[str]) -> list[str]:
    shown = []
    index = 0
    while index < len(parts):
        part = parts[index]
        if part in ("-T", "-rpath", "-z") and index + 1 < len(parts):
            shown.append(f"{part.removeprefix('-')}={display_path(parts[index + 1])}")
            index += 2
        else:
            shown.append(display_path(part.removeprefix("-")))
            index += 1
    return shown


def display_link_flag(flag: str) -> list[str]:
    if flag.startswith("-Wl,"):
        return display_linker_parts(flag.removeprefix("-Wl,").split(","))
    return [display_path(short_flag(flag))]


def display_link_flags(flags: str) -> str:
    shown = []
    for flag in flags.split():
        shown.extend(display_link_flag(flag))
    return " ".join(shown)


def print_verbose(build_dir: pathlib.Path, flags: str) -> None:
    options = clang_help_options(build_dir)
    for flag in flags.split():
        print(f"  {flag}: {describe_flag(flag, options)}")


def display_library(build_dir: pathlib.Path, library: str) -> str:
    if library.startswith("-Wl,"):
        return ",".join(display_linker_parts(library.removeprefix("-Wl,").split(",")))
    if library.startswith("-"):
        return library
    if pathlib.Path(library).is_absolute():
        return display_path(library)
    if "/" in library or library.endswith((".a", ".so", ".dylib", ".lib")):
        return str(build_dir / library)
    return library


def display_libraries(build_dir: pathlib.Path, libraries: str) -> str:
    if libraries == NA:
        return libraries
    return " ".join(display_library(build_dir, library) for library in libraries.split())


def main() -> int:
    if len(sys.argv) not in (5, 6):
        raise SystemExit(
            "usage: status_link_flags.py <build-dir> <config> <label> <libs-label> [--verbose]"
        )

    build_dir = pathlib.Path(sys.argv[1])
    config = sys.argv[2]
    label = sys.argv[3]
    libs_label = sys.argv[4]
    verbose = len(sys.argv) == 6 and sys.argv[5] == "--verbose"
    flags, libraries = link_values(build_dir, config)
    flags = link_flags_without_linker(flags)

    if verbose:
        print(f"\033[1m{label}: \033[0m")
        if flags:
            print_verbose(build_dir, flags)
    else:
        value = display_link_flags(flags) if flags else ""
        if value:
            print(f"\033[1m{label}: \033[0m{value}")
    shown_libraries = libraries if verbose else display_libraries(build_dir, libraries)
    if verbose or shown_libraries != NA:
        print(f"\033[1m{libs_label}: \033[0m{shown_libraries}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
