# Reflow full-line // comment blocks: same words, greedy wrap to max column.
# Prose is always re-merged to fill the width, including when every line is
# already under the limit, unless a block looks preformatted. Preformatted
# stanzas are left alone: body lines with 2+ leading spaces or a tab, or 2+
# consecutive space/tab in the body (paper lists, aligned columns, etc.) A
# block is only lines that share the exact same `^\s*//\s?` prefix, so a line
# with a different number of leading spaces (before `//`) is never rewrapped to
# match a neighbor's indent. `//` lines with llvm-style NOLINT suppressions
# (NOLINT, NOLINTBEGIN, NOLINTEND, NOLINTNEXTLINE, …) in the // body, any case,
# are not reflow-merged, like other tool lines. `//` lines whose first text token
# is a known formatter, linter, or static-analysis control (`clang-format`, cppcheck,
# etc., see _LINTER_PREFIX_PATTERNS) pass through and break a block, so the
# directive is not merged with prose. Bodies that begin with `TODO` (word) are
# not merged with other `//` lines. Add patterns there for new tools. Never
# breaks a http(s) URL
# (one unbroken line
# if it will not fit). This is *not* bit-for-bit identical to `clang-format`
# with `ReflowComments: true`. After any manual pass, run
# `cmake --build <build> --target cat-format` so `cat-format-check` and
# `cat-reflow-comments-check` stay green, and the tree matches `.clang-format`.
from __future__ import annotations

import os
import re
import sys
from argparse import ArgumentParser
from collections.abc import Iterable
from concurrent.futures import ThreadPoolExecutor
from typing import Tuple

# Whole-line token check for a URL, same pattern as in _words_from_text.
_re_http_url = re.compile(r"https?://\S+")
# In the // line body, llvm-style nolint* tokens (not only the first \S+).
_re_nolint_token = re.compile(r"NOLINT\w*", re.IGNORECASE)

# First `//` line token: must not be merged with neighboring comment paragraphs.
# Extend for project-specific or new tools. Match re.match on the first \S+ in
# the // body. Put more specific prefixes (e.g. `clang-tidy` before `clang-`).
_LINTER_PREFIX_PATTERNS: tuple[re.Pattern[str], ...] = (
    re.compile(r"^NOLINT\w*", re.IGNORECASE),
    re.compile(r"^NOSONAR", re.IGNORECASE),
    re.compile(r"^noqa", re.IGNORECASE),
    re.compile(r"^cppcheck", re.IGNORECASE),
    re.compile(r"^cpplint", re.IGNORECASE),
    re.compile(r"^clang-analyzer-"),
    re.compile(r"^clang-format", re.IGNORECASE),
    re.compile(r"^clang-tidy", re.IGNORECASE),
    re.compile(r"^clang-"),
    re.compile(r"^IWYU\b", re.IGNORECASE),
    re.compile(r"^GCC\b", re.IGNORECASE),
    re.compile(r"^G\+\+"),
    re.compile(r"^MSVC", re.IGNORECASE),
    re.compile(r"^coverity", re.IGNORECASE),
    re.compile(r"^PVS[-_]", re.IGNORECASE),
    re.compile(r"^pvs-"),
    re.compile(r"^sonar-", re.IGNORECASE),
    re.compile(r"^sonarlint", re.IGNORECASE),
    re.compile(r"^semgrep", re.IGNORECASE),
    re.compile(r"^flawfinder", re.IGNORECASE),
    re.compile(r"^lizard", re.IGNORECASE),
    re.compile(r"^coccinelle", re.IGNORECASE),
    re.compile(r"^LINT-"),
    re.compile(r"^HIDE-"),
    re.compile(r"^#pragma", re.IGNORECASE),
    re.compile(r"^pylint", re.IGNORECASE),
    re.compile(r"^pyright", re.IGNORECASE),
    re.compile(r"^mypy", re.IGNORECASE),
    re.compile(r"^ruff:", re.IGNORECASE),
    re.compile(r"^eslint-"),
    re.compile(
        r"^(?:Thread|Address|Memory|Undefined|Leak)Sanitizer",
        re.IGNORECASE,
    ),
    re.compile(
        r"^(?:ASan|TSan|MSan|UBSan|LSan|KASAN|KMSAN|KUBSAN|KCSAN)\b",
        re.IGNORECASE,
    ),
    re.compile(
        r"^(?:"
        r"codescene|"
        r"code-?climate|"
        r"lgtm[.\w-]*|"
        r"scan-build|"
        r"infer\b|"
        r"gosec|"
        r"shellcheck|"
        r"biome|"
        r"dprint|"
        r"cargo-clippy|"
        r"cargo-geiger|"
        r"qodana|"
        r"deepsource|"
        r"phpstan|"
        r"psalm|"
        r"phpcs|"
        r"clippy|"
        r"rustfmt|"
        r"bandit"
        r")",
        re.IGNORECASE,
    ),
)

# Hard cap: many concurrent processes are heavy in RAM (e.g. WSL2).
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


def _col(s: str) -> int:
    return len(s.expandtabs(8))


def _is_empty_line_comment(s: str) -> bool:
    return bool(re.match(r"^\s*//\s*$", s.rstrip("\n\r")))


def _is_line_comment(s: str) -> bool:
    t = s.rstrip("\n\r")
    return bool(re.match(r"^\s*//(?!/)\s*", t))


def _slash_prefix(s: str) -> str | None:
    t = s.rstrip("\n\r")
    m = re.match(r"^(\s*//\s?)", t)
    return m.group(1) if m is not None else None


def _nolint_in_slash_line_body(L: str) -> bool:
    t = L.rstrip("\n\r")
    m = re.match(r"^\s*//(?!/)\s*(.*)$", t)
    b = m.group(1) if m is not None else ""
    return _re_nolint_token.search(b) is not None


def _is_todo_slash_line(L: str) -> bool:
    t = L.rstrip("\n\r")
    m = re.match(r"^\s*//(?!/)\s*(.*)$", t)
    b = m.group(1) if m is not None else ""
    b = b.lstrip()
    if not b:
        return False
    return re.match(r"^TODO\b", b, re.IGNORECASE) is not None


def _is_linter_or_tool_directive_line(L: str) -> bool:
    t = L.rstrip("\n\r")
    m = re.match(r"^\s*//(?!/)\s*(\S+)", t)
    if m is None:
        return False
    w = m.group(1)
    for pat in _LINTER_PREFIX_PATTERNS:
        if pat.match(w) is not None:
            return True
    return False


def _preserve_reflow_stanza_line(L: str) -> bool:
    t = L.rstrip("\n\r")
    m = re.match(r"^\s*//\s?(.*)$", t)
    u = m.group(1) if m is not None else ""
    if u and u[0] == "\t":
        return True
    n = 0
    for c in u:
        if c == " ":
            n = n + 1
        else:
            break
    if n >= 2:
        return True
    if re.search(r"[ \t]{2,}", u) is not None:
        return True
    if re.search(r"-\*-\s", u) is not None:
        return True
    if re.match(r"vim\s*:", u.lstrip(), re.IGNORECASE) is not None:
        return True
    return False


def _is_url_token(s: str) -> bool:
    return bool(s and _re_http_url.fullmatch(s))


def _words_from_text(t: str) -> list[str]:
    words: list[str] = []
    i = 0
    n = len(t)
    while i < n:
        while i < n and t[i] in " \t":
            i += 1
        if i >= n:
            break
        m = re.match(r"https?://\S+", t[i:])
        if m is not None:
            words.append(m.group(0))
            i = i + m.end()
        else:
            m2 = re.match(r"\S+", t[i:])
            if m2 is None:
                i += 1
                continue
            words.append(m2.group(0))
            i = i + m2.end()
    return words


def _reflow_words(prefix: str, words: list[str], max_col: int) -> list[str]:
    if not words:
        p = prefix.rstrip("\n")
        return [p + "\n"]
    out: list[str] = []
    cur: list[str] = []
    for w in words:
        if not cur:
            test = prefix + w
        else:
            test = prefix + " ".join(cur) + " " + w
        if cur and _col(test) > max_col:
            out.append(prefix + " ".join(cur) + "\n")
            cur = [w]
        elif (not cur) and _is_url_token(w) and _col(test) > max_col:
            out.append(prefix + w + "\n")
        else:
            cur.append(w)
    if cur:
        out.append(prefix + " ".join(cur) + "\n")
    return out


def _words_from_block(body_lines: list[str]) -> list[str]:
    words: list[str] = []
    for L in body_lines:
        m = re.match(r"^\s*//\s?(.*)$", L.rstrip("\n\r"))
        t = (m.group(1) if m else L).strip()
        if t:
            words.extend(_words_from_text(t))
    return words


def process_lines(lines: list[str], max_col: int) -> list[str]:
    out: list[str] = []
    n = len(lines)
    i = 0
    while i < n:
        L = lines[i]
        if not _is_line_comment(L):
            out.append(L)
            i += 1
            continue
        if _is_empty_line_comment(L):
            out.append(L)
            i += 1
            continue
        if _nolint_in_slash_line_body(L):
            out.append(L)
            i += 1
            continue
        if _is_todo_slash_line(L):
            out.append(L)
            i += 1
            continue
        if _is_linter_or_tool_directive_line(L):
            out.append(L)
            i += 1
            continue
        j = i
        block: list[str] = []
        p0 = _slash_prefix(L)
        if p0 is None:
            out.append(L)
            i += 1
            continue
        while j < n and _is_line_comment(lines[j]):
            if _is_empty_line_comment(lines[j]):
                break
            if _nolint_in_slash_line_body(lines[j]):
                break
            if _is_todo_slash_line(lines[j]):
                break
            if _is_linter_or_tool_directive_line(lines[j]):
                break
            if _slash_prefix(lines[j]) != p0:
                break
            block.append(lines[j])
            j += 1
        if not block:
            out.append(L)
            i += 1
            continue
        prefix = p0
        if any(_preserve_reflow_stanza_line(b) for b in block):
            out.extend(block)
            i = j
            continue
        words = _words_from_block(block)
        new_block = _reflow_words(prefix, words, max_col)
        out.extend(new_block)
        i = j
    return out


def read_lines(path: str) -> list[str]:
    with open(path, encoding="utf-8", newline="") as f:
        return f.readlines()


def write_lines(path: str, lines: list[str]) -> None:
    with open(path, "w", encoding="utf-8", newline="") as f:
        f.writelines(lines)


_ReflowOne = Tuple[str, bool, str | None]  # path, changed, err


def _reflow_one(t: tuple[str, int, bool]) -> _ReflowOne:
    path, max_col, check_only = t
    if not path:
        return "", False, None
    try:
        lines = read_lines(path)
    except OSError as e:
        return path, False, str(e)
    new = process_lines(lines, max_col)
    if new == lines:
        return path, False, None
    if check_only:
        return path, True, None
    try:
        write_lines(path, new)
    except OSError as e:
        return path, False, str(e)
    return path, True, None


def main(argv: Iterable[str] | None = None) -> int:
    p = ArgumentParser()
    p.add_argument("--max-col", type=int, default=80)
    p.add_argument(
        "--check",
        action="store_true",
        help="read-only, exit 1 if any // comment reflow would change a file",
    )
    p.add_argument("paths", nargs="+")
    p.add_argument(
        "-j",
        "--jobs",
        type=int,
        default=None,
        help="concurrent file workers (capped at 4, default: env "
        "CAT_LIBCAT_JOBS or min(4, CPU count)) ",
    )
    a = p.parse_args(list(argv) if argv is not None else None)
    jcap = a.jobs if a.jobs is not None else _default_jobs()
    jcap = _cap_jobs(jcap)
    paths = [p for p in a.paths if p]
    if not paths:
        return 0
    w = max(1, min(jcap, len(paths)))
    chflag = a.check
    work = ((p, a.max_col, chflag) for p in paths)
    with ThreadPoolExecutor(max_workers=w) as ex:
        outs = list(ex.map(_reflow_one, work))
    for path, _ch, err in outs:
        if err:
            print(
                f"cat-reflow-comments: failed for `{path}`: {err}\n", file=sys.stderr
            )
            return 1
    for path, ch, _er in outs:
        if ch and a.check:
            print(f"would reflow: {path}", file=sys.stderr)
        if ch and not a.check:
            print(f"formatted: {path}", file=sys.stderr)
    n_would = sum(1 for _p, c, e in outs if c and e is None)
    if a.check and n_would:
        print(
            f"cat-reflow-comments-check: {n_would} file(s) would be reflowed. "
            "Run `cmake --build <build> --target cat-reflow-comments` to fix.",
            file=sys.stderr,
        )
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
