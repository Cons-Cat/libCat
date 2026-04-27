# This file is flagrantly "vibe-coded". It may not be up to the standards of most libCat code.

# Parallel driver for the =cat-format= and =cat-format-check= build targets. Same
# contract as the old =cmake -P= script block in =cmake/format.cmake=.
from __future__ import annotations

import os
import subprocess
import sys
from argparse import ArgumentParser, Namespace
from concurrent.futures import ThreadPoolExecutor
from typing import Tuple

# (kind, path, err)  kind: same, would, formatted, fail, skip
_FileResult = Tuple[str, str, str | None]

# Hard cap: many concurrent clang-format child processes are heavy in RAM
# (e.g. WSL2).
_MAX_PARALLEL = 4


def _cap_jobs(n: int) -> int:
    if n < 1:
        return 1
    return min(_MAX_PARALLEL, n)


def _default_jobs() -> int:
    raw = os.environ.get("CAT_LIBCAT_JOBS", "").strip()
    if raw and all(c.isdigit() for c in raw) and int(raw) > 0:
        return _cap_jobs(int(raw, 10))
    n = os.cpu_count() or 1
    return _cap_jobs(n)


def _one_file(path: str, mode: str, cf: str) -> _FileResult:
    if not path:
        return "skip", "", None
    try:
        p = subprocess.run(
            [cf, path], capture_output=True, text=True, check=False
        )
    except OSError as e:
        return "fail", path, str(e)
    if p.returncode != 0:
        b = p.stderr or p.stdout or ""
        return "fail", path, b.rstrip()
    out = p.stdout
    try:
        with open(path, encoding="utf-8", newline="") as f:
            current = f.read()
    except OSError as e:
        return "fail", path, str(e)
    if current == out:
        return "same", path, None
    if mode == "CHECK":
        return "would", path, None
    s = subprocess.run(
        [cf, "-i", path], capture_output=True, text=True, check=False
    )
    if s.returncode != 0:
        b = s.stderr or s.stdout or ""
        return "fail", path, b.rstrip()
    return "formatted", path, None


def _parse() -> Namespace:
    p = ArgumentParser()
    p.add_argument("mode", choices=["APPLY", "CHECK"])
    p.add_argument("clang_format", type=str)
    p.add_argument("paths", nargs="*", type=str, default=[])
    p.add_argument(
        "-j",
        "--jobs",
        type=int,
        default=None,
        help="concurrent clang-format calls (capped at 4, default: env "
        "CAT_LIBCAT_JOBS or min(4, CPU count)) ",
    )
    return p.parse_args(list(sys.argv[1:]) if len(sys.argv) > 1 else None)


def main() -> int:
    a = _parse()
    jcap = a.jobs if a.jobs is not None else _default_jobs()
    jcap = _cap_jobs(jcap)
    paths = [p for p in a.paths if p]
    if not paths:
        return 0
    w = max(1, min(jcap, len(paths)))
    with ThreadPoolExecutor(max_workers=w) as ex:
        outl: list[_FileResult] = list(
            ex.map(
                _one_file,
                paths,
                [a.mode] * len(paths),
                [a.clang_format] * len(paths),
            )
        )
    for kind, path, err in outl:
        if kind == "fail":
            print(
                f"cat-format: failed for `{path}`: {err or 'clang-format'}\n",
                file=sys.stderr,
            )
            return 1
    for kind, path, _ in outl:
        if kind == "formatted":
            print(f"formatted: {path}", file=sys.stderr)
        if kind == "would":
            print(f"would reformat: {path}", file=sys.stderr)
    n_would = sum(1 for k, _p, _e in outl if k == "would")
    if a.mode == "CHECK" and n_would:
        print(
            f"cat-format-check: {n_would} file(s) would be reformatted. "
            "Run `cmake --build <build> --target cat-format` to fix.",
            file=sys.stderr,
        )
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
