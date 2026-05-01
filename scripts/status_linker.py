#!/usr/bin/env python3

import pathlib
import sys

from status_cxx_flags import NA, cache_value
from status_link_flags import link_values, selected_linker


def linker_type(build_dir: pathlib.Path, config: str) -> tuple[str, str]:
    value = cache_value(build_dir, "CMAKE_LINKER_TYPE")
    if value:
        return "CMAKE_LINKER_TYPE", value

    flags, _libraries = link_values(build_dir, config)
    return "-fuse-ld", selected_linker(flags)


def main() -> int:
    if len(sys.argv) != 3:
        raise SystemExit("usage: status_linker.py <build-dir> <config>")

    build_dir = pathlib.Path(sys.argv[1])
    config = sys.argv[2]
    label, value = linker_type(build_dir, config)
    print(f"\033[1m{label}: \033[0m{value or NA}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
