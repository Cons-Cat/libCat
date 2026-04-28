"""Restyle full-line comments.

This file is flagrantly "vibe-coded". It may not be up to the standards of most libCat
code.

Comment restyle rules, in order:

1. Process full-line comments only. C++ files use "//". CMake and Python files
   use "#". Python shebang lines are left unchanged.

2. Lowercase modelines. Do not restyle blank comment lines, "TODO" comment
   lines, NOLINT comment lines, formatter or linter directive lines, CMake
   "#include", "#include_next", and "#pragma" comment lines, Python "Usage:"
   stanzas, or preformatted stanzas. Add new directive patterns to
   `_LINTER_PREFIX_PATTERNS`.

3. Warn when a "NOLINTNEXTLINE" line is followed by another comment line.

4. Replace → with " -> ", ← with " <- ", and – or — with "--" outside
   backticks. Remove § section markers followed by digits or dots. Then warn
   when other characters outside the ASCII range appear outside backticks.
   Identifiers that need non-ASCII characters must be written in backticks.

5. Remove three or more "-" or "=" characters in a row when the run starts or
   ends a comment line. Non-empty lines changed this way become colon-ended
   headings. Then warn when comment text still contains such a run.

6. Code block comment lines are not prose-restyled. A line is a code block when
   it has at least three spaces of indentation after the comment token, ends
   with "{", "}", or ";", or when its contiguous sequence of code-token-bearing
   comment lines contains more than six code tokens from this set: "(", ")",
   "{", "}", ";", "=", and "cat::". A line ending with "(" or ":" also starts a
   code block when the following comment line has more than one space of
   indentation. If a line ending in "{", "}", or ";" is not preceded by a
   non-blank comment line, it starts a code block. Following comment lines are
   also code blocks until a blank comment line or a comment line with fewer than
   three spaces of indentation. A line beginning with lowercase "if (",
   "if constexpr (", "if consteval", "for (", or "while (" is also a code block
   when it is not preceded by a non-blank comment line. A line beginning with
   "--", "-f", "static_assert", "template <", or "using identifier =" is always
   a code block. A line beginning with lowercase "return" is also a code block
   in that position only when it contains ";" on the same line. Code block
   comments break after each `;` except inside `if` and `for` control
   parentheses or when the next non-space character is `}`.

7. In C++ comments, warn when a code block comment line exceeds 80 columns.
   `clang-format` will edit it.

8. In C++ comments, replace semicolons outside backticks with periods and
   capitalize the following plain word. Code block lines are unchanged.

9. In C++ comments, warn when comment text contains a colon outside backticks,
   double quotes, and URLs unless the colon is followed by a line break. Code
   block lines do not warn. TODO: Re-enable this for test_*.cpp files after
   existing comments are fixed.

10. Never create nested backticks. Existing backticked code stays unchanged
   except for the explicit unwrapping and spelling rules below.

11. Remove backticks around standalone numeric literals and most C++ operator
   tokens, so "`10`" becomes "10", "`-1ll`" becomes "-1ll", "`0b1'0000`"
   becomes "0b1'0000", "`1.f`" becomes "1.f", "`v3.29.2`" becomes
   "v3.29.2", and "`%=`" becomes "%=". Expressions such as "`10 + 1`" and
   "`a % b`" stay backticked. Bare numeric expressions and version numbers,
   including "v"-prefixed versions, do not get backticks, and punctuation inside
   them does not warn.

12. Outside backticks and file paths, spell libC, libM, and libCat exactly that
    way, without backticks.

13. Outside backticks and file paths, spell Clang, GCC, CRTP, LTO, REPL,
    SFINAE, SIMD, SWAR, and XOR exactly that way, without backticks. ISA
    extension titles such as AVX2 and SSE4* also stay out of backticks. Outside
    C++ asm code phrases, spell asm exactly that way, without backticks.

14. Write compiler versions with a space. "clang-23", "clang23", and
    "`clang-23`" become "Clang 23". "gcc12" and "gcc-12" become "GCC 12".

15. Put tool names and code-looking tokens in backticks. This includes `clang-*`
    tools, command-line flags such as "-T" and "--verbose", dotted names
    including "compile_commands.json", "::" names, "$"-prefixed tokens, calls
    like "expression()" and
    "snake_case(argument)", operator spellings such as `operator+`, include-like
    tokens such as "<cat/string>", and "[[...]]" pairs. `clang-*` tool names are
    lowercased. In CMake comments, this also includes solitary `Release`,
    `Debug`, and `RelWithDebInfo`. In C++ comments, this also includes
    fundamental type identifiers such as `bool` and `int`, plus signed or
    unsigned fundamental integer phrases such as `unsigned int`. Signed and
    unsigned keep their original capitalization. Standalone signed and unsigned
    stay out of backticks. Standalone `short`, `long`, and `double` do not get
    new backticks unless paired into type phrases such as `long double`, but
    existing backticks around those words are preserved. In all non-code block
    comments, `constexpr`, `consteval`, `nullptr`, and `nullopt` get backticks.
    Solitary capital `T` and `U` get backticks. `asm`, `asm volatile`, and
    `asm goto` are backticked and lowercased. In CMake, C++, and Python files,
    kebab-case identifiers get
    backticks only when they contain "cat" as a hyphen-separated segment or name
    a `clang-*` tool. A trailing "*" wildcard stays inside snake_case and
    kebab-case backticks. A solitary "-" or "--" token is not kebab-case. In all
    files, snake_case identifiers get backticks. Plural, possessive,
    past-tense, call, and hyphenated suffixes stay outside backticks but
    directly adjacent to them. Runs of two or more "." characters and "/"
    separators between backticked terms also stay directly adjacent to
    neighboring backticks.

16. Outside backticks, write value categories as lowercase hyphenated prose:
    l-value, x-value, r-value, gl-value, and pr-value, including plurals such as
    l-values.

17. Outside backticks, connect a number followed by singular "bit" or "byte"
    with a hyphen, such as "8-bit" and "4-byte". Plurals stay spaced.

18. Spell "move-only", "copy-only", "opt-in", and "opt-out" with a hyphen.

19. Hyphenate C++ trait prose, including "non-trivial", "non-trivially",
    "trivially-copyable", and "non-trivially-move-assignable".

20. Outside backticks, expand "ctor" to "constructor" and "dtor" to
    "destructor", preserving capitalization.

21. Spell X protocol without a hyphen.

22. Spell "deducing this" with a hyphen instead of a space. Preserve the
    originally written capitalization.

23. Spell "e.g.", "ex.", and "etc." exactly that way, without backticks.

24. Spell "signed-ness" and "unsigned-ness" with a hyphen.

25. The first letter of the first processed line in a comment block must be
    capitalized only when it is the first non-space character in the comment
    body. Later paragraphs in the same contiguous comment block are not
    auto-capitalized. Do not capitalize code-looking or fixed-spelling tokens.

26. Move a single period before ")" to after it before checking terminal
    punctuation, so ".)" becomes ").". Leave "...)" alone. Remove a period
    after "!)" or "?)".

27. The last non-code-block line of a processed comment paragraph must end with
    one of ".", "!", "?", ";", ":", "{", or "}".
    If none is present, "." is added by default.
    Do not add punctuation when the following comment body starts with "`",
    unless it starts with "TODO:". Do not add "." after URLs, and remove a final
    "." after a URL.

28. Wrap restyled prose greedily to the configured maximum column. Python uses
    88 columns, like `black`. C++ and CMake use 80 columns. Do not merge a line
    break after ".", "?", or "!". Do not break "http" or "https" URLs. Leave URL
    text unchanged.

29. Keep each paired-backtick span together while wrapping, unless the span is
    itself longer than the configured maximum column.
"""
from __future__ import annotations

import os
import re
import sys
from argparse import ArgumentParser
from collections.abc import Iterable
from concurrent.futures import ThreadPoolExecutor
from typing import Tuple

# `Whole-line` token check for a URL, same pattern as in `_words_from_text`.
_re_http_url = re.compile(r"https?://\S+")
# In the // line body, llvm-style nolint* tokens (not only the first \S+).
_re_nolint_token = re.compile(r"NOLINT\w*", re.IGNORECASE)
_re_nolintnextline_token = re.compile(r"\bNOLINTNEXTLINE\b", re.IGNORECASE)
_re_bad_rule_run = re.compile(r"-{3,}|={3,}")
_RULE_RUN_PATTERN = r"(?:-{3,}|={3,})"
_re_edge_rule_run_start = re.compile(
    rf"^([ \t]*)(?:`[ \t]*{_RULE_RUN_PATTERN}[ \t]*`|{_RULE_RUN_PATTERN})"
    rf"[ \t]*[.]?[ \t]*"
)
_re_edge_rule_run_end = re.compile(
    rf"[ \t]*(?:{_RULE_RUN_PATTERN}|`[ \t]*{_RULE_RUN_PATTERN}[ \t]*`)"
    rf"[ \t]*[.]?([ \t]*)$"
)

# First comment line token.
# Must not be merged with neighboring paragraphs.
# Extend for `project-specific` or new tools. Match `re.match` on the first \S+ in the
# comment body. Put more specific prefixes (e.g. `clang-tidy` before `clang-`).
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

# Hard cap.
# Many concurrent processes are heavy in RAM (e.g. WSL2).
_MAX_PARALLEL = 4
_DEFAULT_MAX_COL = 80
_PYTHON_MAX_COL = 88
_Warning = Tuple[int, str]
_TERMINAL_COMMENT_PUNCTUATION = (".", "!", "?", ";", ":", ",", "{", "}")
_YELLOW = "\033[33m"
_RESET = "\033[0m"


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


def _use_color(stream) -> bool:
    return os.environ.get("NO_COLOR") is None


def _warning_prefix(stream) -> str:
    prefix = "cat-restyle-comments: warning:"
    if _use_color(stream):
        return f"{_YELLOW}{prefix}{_RESET}"
    return prefix


def _col(s: str) -> int:
    return len(s.expandtabs(8))


def _is_hash_style_path(path: str) -> bool:
    return _is_python_path(path) or _is_cmake_path(path)


def _is_python_path(path: str) -> bool:
    return path.endswith(".py")


def _is_cmake_path(path: str) -> bool:
    return path.endswith(".cmake") or os.path.basename(path) == "CMakeLists.txt"


def _is_cmake_preprocessor_directive_line(L: str, style: str = "slash") -> bool:
    if style != "hash":
        return False
    return (
        re.match(r"^\s*#(?:include_next|include|pragma)\b", L.rstrip("\n\r"))
        is not None
    )


def _is_dotfile_path(path: str) -> bool:
    return os.path.basename(path).startswith(".")


def _is_test_cpp_path(path: str) -> bool:
    name = os.path.basename(path)
    return name.startswith("test_") and name.endswith(".cpp")


def _effective_max_col(path: str, max_col: int, python_max_col: int) -> int:
    if _is_python_path(path):
        return python_max_col
    return max_col


def _comment_prefix_pattern(style: str) -> str:
    if style == "hash":
        return r"#"
    return r"//(?!/)"


def _comment_any_prefix_pattern(style: str) -> str:
    if style == "hash":
        return r"#"
    return r"//+"


def _is_shebang_line(s: str) -> bool:
    return s.lstrip().startswith("#!")


def _is_empty_line_comment(s: str, style: str = "slash") -> bool:
    if style == "hash" and _is_shebang_line(s):
        return False
    pattern = _comment_prefix_pattern(style)
    return bool(re.match(rf"^\s*{pattern}\s*$", s.rstrip("\n\r")))


def _is_line_comment(s: str, style: str = "slash") -> bool:
    t = s.rstrip("\n\r")
    if style == "hash" and _is_shebang_line(t):
        return False
    pattern = _comment_prefix_pattern(style)
    return bool(re.match(rf"^\s*{pattern}\s*", t))


def _is_any_comment_line(s: str, style: str = "slash") -> bool:
    t = s.rstrip("\n\r")
    if style == "hash" and _is_shebang_line(t):
        return False
    pattern = _comment_any_prefix_pattern(style)
    return bool(re.match(rf"^\s*{pattern}", t))


def _comment_prefix(s: str, style: str = "slash") -> str | None:
    t = s.rstrip("\n\r")
    pattern = _comment_prefix_pattern(style)
    m = re.match(rf"^(\s*{pattern}\s?)", t)
    return m.group(1) if m is not None else None


def _comment_body(L: str, style: str = "slash", any_prefix: bool = False) -> str:
    t = L.rstrip("\n\r")
    pattern = (
        _comment_any_prefix_pattern(style)
        if any_prefix
        else _comment_prefix_pattern(style)
    )
    m = re.match(rf"^\s*{pattern}\s*(.*)$", t)
    return m.group(1) if m is not None else ""


def _comment_body_with_spacing(
    L: str, style: str = "slash", any_prefix: bool = False
) -> str:
    t = L.rstrip("\n\r")
    pattern = (
        _comment_any_prefix_pattern(style)
        if any_prefix
        else _comment_prefix_pattern(style)
    )
    m = re.match(rf"^\s*{pattern}(.*)$", t)
    return m.group(1) if m is not None else ""


def _leading_indentation_columns(s: str) -> int:
    return len(re.match(r"^[ \t]*", s).group(0).expandtabs(8))


def _is_indented_code_snippet_comment(L: str, style: str = "slash") -> bool:
    return _leading_indentation_columns(
        _comment_body_with_spacing(L, style, any_prefix=True)
    ) >= 3


def _is_lightly_indented_code_snippet_comment(L: str, style: str = "slash") -> bool:
    return _leading_indentation_columns(
        _comment_body_with_spacing(L, style, any_prefix=True)
    ) > 1


def _comment_body_ends_code_block_brace(L: str, style: str = "slash") -> bool:
    return _comment_body(L, style, any_prefix=True).rstrip().endswith(("{", "}"))


def _comment_body_ends_code_block_semicolon(L: str, style: str = "slash") -> bool:
    return _comment_body(L, style, any_prefix=True).rstrip().endswith(";")


def _comment_body_ends_code_block_starter(L: str, style: str = "slash") -> bool:
    return _comment_body(L, style, any_prefix=True).rstrip().endswith(("{", "}", ";"))


def _comment_body_ends_indented_code_block_starter(
    L: str, style: str = "slash", allow_colon: bool = False
) -> bool:
    body = _comment_body(L, style, any_prefix=True).rstrip()
    return body.endswith("(") or (allow_colon and body.endswith(":"))


def _is_preceded_by_nonblank_comment(
    lines: list[str], index: int, style: str = "slash"
) -> bool:
    return (
        index > 0
        and _is_line_comment(lines[index - 1], style)
        and not _is_empty_line_comment(lines[index - 1], style)
    )


def _comment_body_begins_code_keyword(L: str, style: str = "slash") -> bool:
    body = _comment_body(L, style, any_prefix=True).lstrip()
    if re.match(r"return\b", body) is not None:
        return ";" in body
    return re.match(
        r"(?:"
        r"if\s*\(|"
        r"if\s+constexpr\s*\(|"
        r"if\s+consteval\b|"
        r"(?:for|while)\s+\(|"
        r"(?:requires|noexcept)\b|"
        r"static_assert\b|"
        r"using\s+[A-Za-z_][A-Za-z0-9_]*\s*="
        r")",
        body,
    ) is not None


def _comment_body_begins_forced_code_keyword(L: str, style: str = "slash") -> bool:
    body = _comment_body(L, style, any_prefix=True).lstrip()
    return re.match(
        r"(?:--|-f|static_assert\b|template\s*<|using\s+[A-Za-z_][A-Za-z0-9_]*\s*=)",
        body,
    ) is not None


def _code_token_count(body: str) -> int:
    body = re.sub(r"`[^`\n]*`", "", body)
    return (
        body.count("(")
        + body.count(")")
        + body.count("{")
        + body.count("}")
        + body.count(";")
        + body.count("=")
        + body.count("cat::")
    )


def _is_code_block_comment_line(L: str, style: str = "slash") -> bool:
    return _is_indented_code_snippet_comment(
        L, style
    ) or _comment_body_ends_code_block_brace(
        L, style
    ) or _comment_body_ends_code_block_semicolon(L, style)


def _code_block_comment_line_indices(
    lines: list[str], style: str = "slash", allow_colon_code_intro: bool = False
) -> set[int]:
    code_lines: set[int] = set()
    n = len(lines)
    i = 0
    while i < n:
        if not _is_line_comment(lines[i], style):
            i += 1
            continue
        if _is_empty_line_comment(lines[i], style):
            i += 1
            continue
        if (
            _comment_body_ends_indented_code_block_starter(
                lines[i], style, allow_colon_code_intro
            )
            and i + 1 < n
            and _is_line_comment(lines[i + 1], style)
            and not _is_empty_line_comment(lines[i + 1], style)
            and _is_lightly_indented_code_snippet_comment(lines[i + 1], style)
        ):
            code_lines.add(i)
            j = i + 1
            while j < n and _is_line_comment(lines[j], style):
                if _is_empty_line_comment(lines[j], style):
                    break
                if not _is_lightly_indented_code_snippet_comment(lines[j], style):
                    break
                code_lines.add(j)
                j += 1
            i = j
            continue
        if _comment_body_ends_code_block_starter(lines[i], style):
            code_lines.add(i)
            if _is_preceded_by_nonblank_comment(lines, i, style):
                i += 1
                continue
            j = i + 1
            while j < n and _is_line_comment(lines[j], style):
                if _is_empty_line_comment(lines[j], style):
                    break
                if not _is_indented_code_snippet_comment(lines[j], style):
                    break
                code_lines.add(j)
                j += 1
            i = j
            continue
        if (
            not _is_preceded_by_nonblank_comment(lines, i, style)
            and _comment_body_begins_code_keyword(lines[i], style)
        ) or _comment_body_begins_forced_code_keyword(lines[i], style):
            code_lines.add(i)
            i += 1
            continue
        line_tokens = _code_token_count(
            _comment_body(lines[i], style, any_prefix=True)
        )
        if line_tokens == 0:
            if _is_indented_code_snippet_comment(lines[i], style):
                code_lines.add(i)
            i += 1
            continue
        j = i
        sequence: list[int] = []
        code_tokens = 0
        while j < n and _is_line_comment(lines[j], style):
            if _is_empty_line_comment(lines[j], style):
                break
            line_tokens = _code_token_count(
                _comment_body(lines[j], style, any_prefix=True)
            )
            if line_tokens == 0:
                break
            sequence.append(j)
            if _is_indented_code_snippet_comment(lines[j], style):
                code_lines.add(j)
            code_tokens += line_tokens
            j += 1
        if code_tokens > 6:
            code_lines.update(sequence)
        i = j if j > i else i + 1
    return code_lines


def _is_indented_code_block_intro_line(
    lines: list[str],
    index: int,
    style: str = "slash",
    allow_colon_code_intro: bool = False,
) -> bool:
    return (
        0 <= index < len(lines)
        and _comment_body_ends_indented_code_block_starter(
            lines[index], style, allow_colon_code_intro
        )
        and index + 1 < len(lines)
        and _is_line_comment(lines[index + 1], style)
        and not _is_empty_line_comment(lines[index + 1], style)
        and _is_lightly_indented_code_snippet_comment(lines[index + 1], style)
    )


def _control_paren_starts(body: str) -> set[int]:
    starts: set[int] = set()
    for match in re.finditer(r"\b(?:if|for)\s*\(", body):
        starts.add(body.find("(", match.start()))
    return starts


def _code_semicolon_split_positions(body: str) -> list[int]:
    positions: list[int] = []
    control_starts = _control_paren_starts(body)
    control_depths: list[int] = []
    depth = 0
    i = 0
    while i < len(body):
        c = body[i]
        if c == "`":
            j = body.find("`", i + 1)
            if j != -1:
                i = j + 1
                continue
        if c == "(":
            depth += 1
            if i in control_starts:
                control_depths.append(depth)
        elif c == ")":
            while control_depths and control_depths[-1] >= depth:
                control_depths.pop()
            if depth > 0:
                depth -= 1
        elif c == ";" and not control_depths:
            j = i + 1
            while j < len(body) and body[j] in " \t":
                j += 1
            if j == len(body) or body[j] != "}":
                positions.append(i)
        i += 1
    return positions


def _split_code_block_semicolon_comment_line(
    L: str, style: str = "slash"
) -> list[str]:
    t, ending = _line_ending(L)
    pattern = _comment_prefix_pattern(style)
    m = re.match(rf"^(\s*{pattern})(.*)$", t)
    if m is None:
        return [L]
    prefix = m.group(1)
    body = m.group(2)
    positions = _code_semicolon_split_positions(body)
    if not positions:
        return [L]
    leading = re.match(r"^[ \t]*", body).group(0)
    out: list[str] = []
    start = 0
    for position in positions:
        segment = body[start : position + 1].strip()
        if segment:
            out.append(prefix + leading + segment + ending)
        start = position + 1
        while start < len(body) and body[start] in " \t":
            start += 1
    tail = body[start:].strip()
    if tail:
        out.append(prefix + leading + tail + ending)
    return out or [L]


def _nolint_in_comment_body(L: str, style: str = "slash") -> bool:
    b = _comment_body(L, style)
    return _re_nolint_token.search(b) is not None


def _nolintnextline_in_comment_body(L: str, style: str = "slash") -> bool:
    b = _comment_body(L, style, any_prefix=True)
    return _re_nolintnextline_token.match(b.lstrip()) is not None


def _ineffectual_nolintnextline_warnings(
    lines: list[str], style: str = "slash"
) -> list[_Warning]:
    warnings: list[_Warning] = []
    for i, L in enumerate(lines[:-1]):
        if _nolintnextline_in_comment_body(L, style) and _is_any_comment_line(
            lines[i + 1], style
        ):
            warnings.append((i + 1, "NOLINTNEXTLINE is followed by a comment line"))
    return warnings


def _non_ascii_outside_backticks(body: str) -> list[str]:
    chars: list[str] = []
    i = 0
    n = len(body)
    while i < n:
        if body[i] == "`":
            j = body.find("`", i + 1)
            if j != -1:
                i = j + 1
                continue
        if ord(body[i]) > 0x7F and body[i] not in chars:
            chars.append(body[i])
        i += 1
    return chars


def _non_ascii_comment_warnings(
    lines: list[str], style: str = "slash"
) -> list[_Warning]:
    warnings: list[_Warning] = []
    for i, L in enumerate(lines):
        if not _is_any_comment_line(L, style):
            continue
        chars = _non_ascii_outside_backticks(
            _comment_body(L, style, any_prefix=True)
        )
        if chars:
            codes = ", ".join(f"{c} U+{ord(c):04X}" for c in chars)
            warnings.append(
                (
                    i + 1,
                    f"non-ASCII character outside backticks ({codes})",
                )
            )
    return warnings


def _is_modeline_comment_text(body: str) -> bool:
    text = body.lstrip()
    return re.search(r"-\*-\s", text) is not None or re.match(
        r"vim\s*:", text, re.IGNORECASE
    ) is not None


def _ascii_punctuation_comment_text(body: str) -> str:
    out: list[str] = []
    i = 0
    n = len(body)

    def append_spaced_sigil(sigil: str) -> int:
        while out and out[-1] in " \t":
            out.pop()
        out.append(f" {sigil} ")
        j = i + 1
        while j < n and body[j] in " \t":
            j += 1
        return j - 1

    while i < n:
        if body[i] == "`":
            j = body.find("`", i + 1)
            if j != -1:
                out.append(body[i : j + 1])
                i = j + 1
                continue
        if body[i] == "\u2192":
            i = append_spaced_sigil("->")
        elif body[i] == "\u2190":
            i = append_spaced_sigil("<-")
        elif body[i] in "\u2013\u2014":
            out.append("--")
        elif body[i] in "\u2018\u2019":
            out.append("'")
        elif body[i] in "\u201c\u201d":
            out.append('"')
        elif body[i] == "\u00a7" and i + 1 < n and body[i + 1] in ".0123456789":
            i += 1
            while i < n and body[i] in ".0123456789":
                i += 1
            if i < n and body[i] in " \t" and out and out[-1] in " \t":
                i += 1
            continue
        else:
            out.append(body[i])
        i += 1
    return "".join(out)


def _ascii_punctuation_comment_line(L: str, style: str = "slash") -> str:
    t, ending = _line_ending(L)
    pattern = _comment_any_prefix_pattern(style)
    m = re.match(rf"^(\s*{pattern}\s?)(.*)$", t)
    if m is None:
        return L
    return m.group(1) + _ascii_punctuation_comment_text(m.group(2)) + ending


def _ascii_punctuation_comment_lines(
    lines: list[str], style: str = "slash"
) -> list[str]:
    return [
        _ascii_punctuation_comment_line(L, style)
        if _is_any_comment_line(L, style)
        else L
        for L in lines
    ]


def _lowercase_modeline_comment_text(body: str) -> str:
    m = re.match(r"^([ \t]*)(.*)$", body)
    if m is None:
        return body
    leading = m.group(1)
    text = m.group(2)
    if _is_modeline_comment_text(text):
        return leading + text.lower()
    return body


def _lowercase_modeline_comment_line(L: str, style: str = "slash") -> str:
    t, ending = _line_ending(L)
    pattern = _comment_any_prefix_pattern(style)
    m = re.match(rf"^(\s*{pattern}\s?)(.*)$", t)
    if m is None:
        return L
    return m.group(1) + _lowercase_modeline_comment_text(m.group(2)) + ending


def _lowercase_modeline_comment_lines(
    lines: list[str], style: str = "slash"
) -> list[str]:
    return [
        _lowercase_modeline_comment_line(L, style)
        if _is_any_comment_line(L, style)
        else L
        for L in lines
    ]


def _preserved_cmake_preprocessor_line_indices(
    lines: list[str], style: str = "slash"
) -> set[int]:
    return {
        i
        for i, L in enumerate(lines)
        if _is_cmake_preprocessor_directive_line(L, style)
    }


def _restore_lines_at_indices(
    lines: list[str], original: list[str], indices: set[int]
) -> list[str]:
    if not indices:
        return lines
    return [original[i] if i in indices else L for i, L in enumerate(lines)]


def _rule_run_comment_warnings(
    lines: list[str], style: str = "slash"
) -> list[_Warning]:
    warnings: list[_Warning] = []
    for i, L in enumerate(lines):
        if not _is_any_comment_line(L, style):
            continue
        body = _comment_body(L, style, any_prefix=True)
        if _is_modeline_comment_text(body):
            continue
        match = _re_bad_rule_run.search(body)
        if match is not None:
            warnings.append(
                (
                    i + 1,
                    f"comment contains `{match.group(0)}` run that will not format correctly",
                )
            )
    return warnings


def _strip_edge_rule_run_comment_text(body: str) -> str:
    stripped = _re_edge_rule_run_start.sub(r"\1", body)
    stripped = _re_edge_rule_run_end.sub(r"\1", stripped)
    if stripped == body:
        return body
    text = stripped.rstrip()
    if not text:
        return stripped
    if text.endswith(":"):
        return _capitalize_paragraph_start_text(stripped)
    return _capitalize_paragraph_start_text(text.rstrip(".,;") + ":")


def _strip_edge_rule_run_comment_line(L: str, style: str = "slash") -> str:
    t, ending = _line_ending(L)
    pattern = _comment_any_prefix_pattern(style)
    m = re.match(rf"^(\s*{pattern}\s?)(.*)$", t)
    if m is None:
        return L
    return m.group(1) + _strip_edge_rule_run_comment_text(m.group(2)) + ending


def _strip_edge_rule_run_comment_lines(
    lines: list[str], style: str = "slash"
) -> list[str]:
    return [
        _strip_edge_rule_run_comment_line(L, style)
        if _is_any_comment_line(L, style)
        else L
        for L in lines
    ]


def _code_block_column_warnings(
    lines: list[str], style: str = "slash"
) -> list[_Warning]:
    warnings: list[_Warning] = []
    if style != "slash":
        return warnings
    code_lines = _code_block_comment_line_indices(lines, style)
    for i, L in enumerate(lines):
        if not _is_any_comment_line(L, style):
            continue
        if i not in code_lines:
            continue
        if _col(L.rstrip("\r\n")) > 80:
            warnings.append(
                (
                    i + 1,
                    "code block comment exceeds 80 columns. clang-format will edit it",
                )
            )
    return warnings


def _numeric_expression_spans(body: str) -> list[tuple[int, int]]:
    spans: list[tuple[int, int]] = []
    for match in _re_numeric_expression.finditer(body):
        start, end = match.span()
        if start > 0 and re.match(r"[A-Za-z0-9_]", body[start - 1]) is not None:
            continue
        if end < len(body) and re.match(r"[A-Za-z0-9_]", body[end]) is not None:
            continue
        spans.append((start, end))
    return spans


def _url_spans(body: str) -> list[tuple[int, int]]:
    return [match.span() for match in _re_http_url.finditer(body)]


def _double_quoted_spans(body: str) -> list[tuple[int, int]]:
    spans: list[tuple[int, int]] = []
    i = 0
    while i < len(body):
        if body[i] != '"':
            i += 1
            continue
        start = i
        i += 1
        escaped = False
        while i < len(body):
            if escaped:
                escaped = False
            elif body[i] == "\\":
                escaped = True
            elif body[i] == '"':
                spans.append((start, i + 1))
                i += 1
                break
            i += 1
        else:
            spans.append((start, len(body)))
    return spans


def _in_spans(position: int, spans: list[tuple[int, int]]) -> bool:
    return any(start <= position < end for start, end in spans)


def _punctuation_outside_backticks(
    body: str,
    punctuation: str,
    skip_numeric_expressions: bool = False,
    skip_urls: bool = False,
    skip_double_quotes: bool = False,
) -> list[int]:
    positions: list[int] = []
    skipped_spans = _numeric_expression_spans(body) if skip_numeric_expressions else []
    if skip_urls:
        skipped_spans.extend(_url_spans(body))
    if skip_double_quotes:
        skipped_spans.extend(_double_quoted_spans(body))
    i = 0
    n = len(body)
    while i < n:
        if body[i] == "`":
            j = body.find("`", i + 1)
            if j != -1:
                i = j + 1
                continue
        if body[i] == punctuation:
            if not _in_spans(i, skipped_spans):
                positions.append(i)
        i += 1
    return positions


def _replace_semicolon_sentence_breaks_text(body: str) -> str:
    out: list[str] = []
    skipped_spans = _numeric_expression_spans(body)
    capitalize_next = False
    i = 0
    n = len(body)
    while i < n:
        url = _re_http_url.match(body, i)
        if url is not None:
            out.append(url.group(0))
            i = url.end()
            if capitalize_next:
                capitalize_next = False
            continue
        if body[i] == "`":
            j = body.find("`", i + 1)
            if j != -1:
                out.append(body[i : j + 1])
                i = j + 1
                if capitalize_next:
                    capitalize_next = False
                continue
        if body[i] == ";" and not _in_spans(i, skipped_spans):
            out.append(".")
            capitalize_next = True
            i += 1
            continue
        if capitalize_next:
            if body[i].isalpha():
                out.append(body[i].upper())
                capitalize_next = False
                i += 1
                continue
            if body[i].isalnum() or body[i] == "_":
                capitalize_next = False
        out.append(body[i])
        i += 1
    return "".join(out)


def _colon_comment_warnings(
    lines: list[str], style: str = "slash"
) -> list[_Warning]:
    warnings: list[_Warning] = []
    if style != "slash":
        return warnings
    code_lines = _code_block_comment_line_indices(lines, style)
    for i, L in enumerate(lines):
        if not _is_any_comment_line(L, style):
            continue
        if i in code_lines:
            continue
        if _is_todo_comment_line(L, style):
            continue
        body = _comment_body(L, style, any_prefix=True)
        if _is_modeline_comment_text(body):
            continue
        for position in _punctuation_outside_backticks(
            body,
            ":",
            True,
            skip_urls=True,
            skip_double_quotes=True,
        ):
            if body[position + 1 :].strip():
                warnings.append(
                    (
                        i + 1,
                        "colon outside backticks should be followed by a line break",
                    )
                )
                break
    return warnings


def _is_todo_comment_line(L: str, style: str = "slash") -> bool:
    b = _comment_body(L, style)
    b = b.lstrip()
    if not b:
        return False
    return re.match(r"^TODO\b", b, re.IGNORECASE) is not None


def _is_usage_comment_line(L: str, style: str = "slash") -> bool:
    if style != "hash":
        return False
    return _comment_body(L, style).strip() == "Usage:"


def _is_linter_or_tool_directive_line(
    L: str,
    style: str = "slash",
    preserve_cmake_preprocessor_directives: bool = False,
) -> bool:
    t = L.rstrip("\n\r")
    pattern = _comment_prefix_pattern(style)
    m = re.match(rf"^\s*{pattern}\s*(\S+)(?:\s+(\S+))?", t)
    if m is None:
        return False
    w = m.group(1)
    arg = m.group(2) or ""
    if preserve_cmake_preprocessor_directives:
        if _is_cmake_preprocessor_directive_line(L, style):
            return True
    if re.match(r"^clang-format$", w, re.IGNORECASE) is not None:
        return arg.lower() in {"off", "on"}
    if re.match(r"^clang-", w, re.IGNORECASE) is not None:
        return False
    if re.match(r"^(?:GCC\b|G\+\+)", w, re.IGNORECASE) is not None:
        return False
    for pat in _LINTER_PREFIX_PATTERNS:
        if pat.match(w) is not None:
            return True
    return False


def _preserve_reflow_stanza_line(L: str, style: str = "slash") -> bool:
    u = _comment_body_with_spacing(L, style, any_prefix=True)
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


_re_value_category = re.compile(
    r"(?<![A-Za-z0-9_])(?P<category>lvalue|xvalue|rvalue|glvalue|prvalue)"
    r"(?P<suffix>s)?(?![A-Za-z0-9_])",
    re.IGNORECASE,
)
_VALUE_CATEGORY_SPELLING = {
    "lvalue": "l-value",
    "xvalue": "x-value",
    "rvalue": "r-value",
    "glvalue": "gl-value",
    "prvalue": "pr-value",
}
_re_const_word = re.compile(r"(?<![A-Za-z0-9_])(?:Const|const)(?![A-Za-z0-9_])")
_re_null_literal_word = re.compile(
    r"(?<![A-Za-z0-9_])(?:nullptr|nullopt)(?![A-Za-z0-9_])"
)
_re_if_constexpr_phrase = re.compile(
    r"(?<![A-Za-z0-9_])if[ \t]+constexpr(?![A-Za-z0-9_])",
    re.IGNORECASE,
)
_re_constexpr_consteval_word = re.compile(
    r"(?<![A-Za-z0-9_])(?:constexpr|consteval)(?![A-Za-z0-9_])",
    re.IGNORECASE,
)
_re_template_parameter_word = re.compile(
    r"(?<![A-Za-z0-9_`*:&<>])(?P<parameter>[TU])(?![A-Za-z0-9_`*:&<>])"
)
_re_compiler_version = re.compile(
    r"(?<![A-Za-z0-9_])(?P<name>clang|gcc)[- ]?(?P<version>\d+)(?![A-Za-z0-9_])",
    re.IGNORECASE,
)
_NUMERIC_LITERAL_PATTERN = (
    r"[+-]?(?:"
    r"0[xX][0-9A-Fa-f']+|"
    r"0[bB][01']+|"
    r"0[oO][0-7']+|"
    r"(?:\d[\d']*(?:\.(?:\d[\d']*)?)?|\.\d[\d']*)"
    r"(?:[eEpP][+-]?\d[\d']*)?"
    r")(?:[uUlLfFzZ]*)"
)
_re_numeric_literal = re.compile(_NUMERIC_LITERAL_PATTERN)
_re_version_number = re.compile(r"v?\d+(?:\.\d+)+", re.IGNORECASE)
_NUMERIC_EXPRESSION_OPERATOR_PATTERN = (
    r"(?:<<|>>|<=|>=|==|!=|&&|\|\||[-+*/%&|^<>:;])"
)
_re_numeric_expression = re.compile(
    rf"{_NUMERIC_LITERAL_PATTERN}(?:\s*{_NUMERIC_EXPRESSION_OPERATOR_PATTERN}\s*"
    rf"{_NUMERIC_LITERAL_PATTERN})+"
)
_re_numeric_bit_byte = re.compile(
    rf"(?<![A-Za-z0-9_])(?P<number>{_NUMERIC_LITERAL_PATTERN})[ \t]+"
    r"(?P<unit>bit|byte)(?![A-Za-z0-9_])",
    re.IGNORECASE,
)
_re_only_category = re.compile(
    r"(?<![A-Za-z0-9_])(?P<operation>copy|move)[ \t-]+only(?![A-Za-z0-9_])",
    re.IGNORECASE,
)
_re_opt_category = re.compile(
    r"(?<![A-Za-z0-9_])opt[ \t-]+(?P<direction>in|out)(?![A-Za-z0-9_])",
    re.IGNORECASE,
)
_re_non_trivial = re.compile(
    r"(?<![A-Za-z0-9_])(?P<prefix>non)[ \t-]+(?P<form>trivial(?:ly)?)(?![A-Za-z0-9_])",
    re.IGNORECASE,
)
_re_cpp_trait_phrase = re.compile(
    r"(?<![A-Za-z0-9_])"
    r"(?P<prefix>trivially|non-trivially)[ \t-]+"
    r"(?:(?P<operation>default|copy|move)[ \t-]+)?"
    r"(?P<trait>assignable|constructible|convertible|copyable|destructible|"
    r"relocatable)"
    r"(?![A-Za-z0-9_])",
    re.IGNORECASE,
)
_re_ctor_dtor_word = re.compile(
    r"(?<![A-Za-z0-9_])(?P<word>ctor|dtor)(?![A-Za-z0-9_])",
    re.IGNORECASE,
)
_re_x_protocol = re.compile(r"(?<![A-Za-z0-9_])x-protocol(?![A-Za-z0-9_])", re.IGNORECASE)
_re_deducing_this = re.compile(
    r"(?<![A-Za-z0-9_])deducing[- ]this(?![A-Za-z0-9_])",
    re.IGNORECASE,
)
_re_abbreviation = re.compile(
    r"(?<![A-Za-z0-9_])(?:e\.?g\.?|i\.?e\.?|ex\.?|etc\.?)(?![A-Za-z0-9_])",
    re.IGNORECASE,
)
_re_abbreviation_close_paren = re.compile(
    r"(?<![A-Za-z0-9_])(?:e\.g\.|i\.e\.|ex\.|etc\.)\)\.?",
    re.IGNORECASE,
)
_re_clang_word = re.compile(r"(?<![A-Za-z0-9_])clang(?![A-Za-z0-9_+-])", re.IGNORECASE)
_re_gcc_word = re.compile(r"(?<![A-Za-z0-9_])gcc(?![A-Za-z0-9_+-])", re.IGNORECASE)
_re_lib_word = re.compile(
    r"(?<![A-Za-z0-9_])(?P<name>libc|libm|libcat)(?![A-Za-z0-9_])",
    re.IGNORECASE,
)
_LIB_SPELLING = {
    "libc": "libC",
    "libm": "libM",
    "libcat": "libCat",
}
_ISA_EXTENSION_TITLE_PREFIXES = (
    "AES",
    "AMX",
    "AVX",
    "BMI",
    "F16C",
    "FMA",
    "LZCNT",
    "MMX",
    "NEON",
    "PCLMULQDQ",
    "POPCNT",
    "RVV",
    "SHA",
    "SME",
    "SSE",
    "SSSE",
    "SVE",
    "TZCNT",
    "VSX",
)
_re_acronym_word = re.compile(
    r"(?<![A-Za-z0-9_])(?P<word>crtp|lto|repl|sfinae|simd|swar|xor)"
    r"(?P<suffix>'s|'|es|s)?"
    r"(?![A-Za-z0-9_])",
    re.IGNORECASE,
)
_re_asm_word = re.compile(
    r"(?<![A-Za-z0-9_])asm(?P<suffix>'s|'|es|s)?(?![A-Za-z0-9_])",
    re.IGNORECASE,
)
_re_asm_phrase = re.compile(
    r"(?<![A-Za-z0-9_])asm(?:\s+(?:volatile|goto))?(?![A-Za-z0-9_])",
    re.IGNORECASE,
)
_SIGNED_UNSIGNED_WORD = r"(?:signed|unsigned)"
_FUNDAMENTAL_INTEGER_AFTER_SIGN = (
    r"(?:char|char8_t|char16_t|char32_t|wchar_t|short(?:\s+int)?|int|"
    r"long(?:\s+long)?(?:\s+int)?)"
)
_re_signed_unsigned_integer_phrase = re.compile(
    rf"(?<![A-Za-z0-9_])(?P<sign>{_SIGNED_UNSIGNED_WORD})\s+"
    rf"(?P<type>{_FUNDAMENTAL_INTEGER_AFTER_SIGN})(?![A-Za-z0-9_])",
    re.IGNORECASE,
)
_re_short_long_integer_phrase = re.compile(
    r"(?<![A-Za-z0-9_])(?P<type>short\s+int|long\s+(?:int|long(?:\s+int)?))"
    r"(?![A-Za-z0-9_])",
    re.IGNORECASE,
)
_re_long_double_phrase = re.compile(
    r"(?<![A-Za-z0-9_])(?P<type>long\s+double)(?![A-Za-z0-9_])",
    re.IGNORECASE,
)
_re_short_long_word = re.compile(r"(?<![A-Za-z0-9_])(?:short|long)(?![A-Za-z0-9_])")
_re_double_word = re.compile(r"(?<![A-Za-z0-9_])double(?![A-Za-z0-9_])", re.IGNORECASE)
_re_signedness_word = re.compile(
    rf"(?<![A-Za-z0-9_])(?P<sign>{_SIGNED_UNSIGNED_WORD})-?ness(?![A-Za-z0-9_])",
    re.IGNORECASE,
)
_re_signed_unsigned_word = re.compile(
    rf"(?<![A-Za-z0-9_]){_SIGNED_UNSIGNED_WORD}(?![A-Za-z0-9_])",
    re.IGNORECASE,
)
_re_fundamental_type_word = re.compile(
    r"(?<![A-Za-z0-9_])"
    r"(?:bool|char|char8_t|char16_t|char32_t|wchar_t|int|float|void)"
    r"(?![A-Za-z0-9_])",
    re.IGNORECASE,
)
_CPP_OPERATOR_TOKENS = frozenset(
    {
        "!",
        "!=",
        "%",
        "%=",
        "&",
        "&&",
        "&=",
        "*",
        "*=",
        "+",
        "++",
        "+=",
        ",",
        "-",
        "--",
        "-=",
        "->",
        "->*",
        ".*",
        "/",
        "/=",
        "<",
        "<<",
        "<<=",
        "<=",
        "<=>",
        "=",
        "==",
        ">",
        ">=",
        ">>",
        ">>=",
        "^",
        "^=",
        "|",
        "|=",
        "||",
        "~",
    }
)
_CPP_OPERATOR_NAME_TOKENS = _CPP_OPERATOR_TOKENS | {"()", "[]"}
_CPP_OPERATOR_NAME_TOKEN_PATTERN = "|".join(
    re.escape(token)
    for token in sorted(_CPP_OPERATOR_NAME_TOKENS, key=len, reverse=True)
)
_CPP_NAMED_OPERATOR_PATTERN = r"(?:new[ \t]*(?:\[\])?|delete[ \t]*(?:\[\])?|co_await)"
_re_operator_name = re.compile(
    rf"(?<![A-Za-z0-9_`])operator[ \t]*"
    rf"(?P<operator>{_CPP_OPERATOR_NAME_TOKEN_PATTERN}|{_CPP_NAMED_OPERATOR_PATTERN})"
    r"(?![A-Za-z0-9_`])"
)
_OFFICIAL_CLANG_TOOLS = frozenset(
    {
        "clang-apply-replacements",
        "clang-change-namespace",
        "clang-check",
        "clang-doc",
        "clang-extdef-mapping",
        "clang-format",
        "clang-include-cleaner",
        "clang-include-fixer",
        "clang-linker-wrapper",
        "clang-move",
        "clang-nvlink-wrapper",
        "clang-offload-bundler",
        "clang-offload-packager",
        "clang-query",
        "clang-refactor",
        "clang-rename",
        "clang-reorder-fields",
        "clang-scan-deps",
        "clang-tidy",
    }
)
_re_clang_tool = re.compile(r"(?<!\S)(\S*-\S*)(?!\S)")
_re_attribute_token = re.compile(r"\[\[[^\n`]*?\]\]")
_re_command_line_flag = re.compile(r"(?<!\S)(\S*-{1,2}\S*)(?!\S)")
_re_cmake_config_word = re.compile(
    r"(?<![A-Za-z0-9_])(?P<config>Release|Debug|RelWithDebInfo)(?![A-Za-z0-9_+-])"
)
_re_path_token = re.compile(r"(?<!\S)(\S*/\S*)(?!\S)")
_re_identifier_with_dollar = re.compile(r"(?<!\S)(\$\S*)(?!\S)")
_re_include_token = re.compile(r"(?<!\S)(\S*<[^<>\s]+>\S*)(?!\S)")
_re_identifier_with_kebab = re.compile(r"(?<!\S)(\S*-\S*)(?!\S)")
_re_identifier_with_dot = re.compile(r"(?<!\S)(\S*\.\S*)(?!\S)")
_re_identifier_with_scope = re.compile(r"(?<!\S)(\S*::\S*)(?!\S)")
_re_identifier_call = re.compile(
    r"(?<![A-Za-z0-9_])(?P<call>(?:[A-Za-z_][A-Za-z0-9_]*::)*"
    r"[A-Za-z_][A-Za-z0-9_]*\(\))(?P<suffix>es|s)?(?![A-Za-z0-9_])"
)
_re_identifier_with_underscore = re.compile(
    r"(?<![A-Za-z0-9_])((?:[A-Za-z_][A-Za-z0-9_]*::)*"
    r"[A-Za-z_][A-Za-z0-9_]*_[A-Za-z0-9_]*\*?)(?![A-Za-z0-9_])"
)


def _line_ending(s: str) -> tuple[str, str]:
    if s.endswith("\r\n"):
        return s[:-2], "\r\n"
    if s.endswith("\n"):
        return s[:-1], "\n"
    if s.endswith("\r"):
        return s[:-1], "\r"
    return s, ""


def _normalize_abbreviation(s: str) -> str | None:
    compact = s.lower().replace(".", "")
    if compact == "eg":
        return "e.g."
    if compact == "ie":
        return "i.e."
    if compact == "ex":
        return "ex."
    if compact == "etc":
        return "etc."
    return None


def _backtick_suffix_end(s: str, i: int) -> int:
    if s.startswith("'s", i):
        return i + 2
    if s.startswith("'d", i):
        return i + 2
    if s.startswith("'", i):
        return i + 1
    if s.startswith("ed", i) and (i + 2 == len(s) or not s[i + 2].isalnum()):
        return i + 2
    if s.startswith("es", i) and (i + 2 == len(s) or not s[i + 2].isalnum()):
        return i + 2
    if s.startswith("s", i) and (i + 1 == len(s) or not s[i + 1].isalnum()):
        return i + 1
    m = re.match(r"-[A-Za-z0-9][A-Za-z0-9-]*", s[i:])
    if m is not None:
        return i + m.end()
    if s.startswith("/", i):
        j = i + 1
        while j < len(s) and s[j] in " \t":
            j += 1
        if j < len(s) and s[j] == "`":
            k = s.find("`", j + 1)
            if k != -1:
                return _backtick_suffix_end(s, k + 1)
    return i


def _signed_unsigned_integer_phrase_spelling(match: re.Match[str]) -> str:
    return f"{match.group('sign')} {match.group('type').lower()}"


def _repair_redundant_backticks(s: str) -> str:
    s = re.sub(r"``[ \t]*`([^`\n]+)`[ \t]*``", r"`\1`", s)
    return re.sub(r"``[ \t]*([^`\n]+?)[ \t]*``", r"`\1`", s)


def _attach_backticked_snake_case_parentheses(s: str) -> str:
    out: list[str] = []
    i = 0
    core_pattern = re.compile(
        r"(?:[A-Za-z_][A-Za-z0-9_]*::)*[A-Za-z_][A-Za-z0-9_]*_[A-Za-z0-9_]*"
    )
    while i < len(s):
        if s[i] != "`":
            out.append(s[i])
            i += 1
            continue
        end_tick = s.find("`", i + 1)
        if end_tick == -1:
            out.append(s[i:])
            break
        core = s[i + 1 : end_tick]
        if (
            core_pattern.fullmatch(core) is None
            or end_tick + 1 >= len(s)
            or s[end_tick + 1] != "("
        ):
            out.append(s[i : end_tick + 1])
            i = end_tick + 1
            continue
        depth = 0
        j = end_tick + 1
        while j < len(s) and s[j] != "`":
            if s[j] == "(":
                depth += 1
            elif s[j] == ")":
                depth -= 1
                if depth == 0:
                    out.append(f"`{core}{s[end_tick + 1 : j + 1]}`")
                    i = j + 1
                    break
            j += 1
        else:
            out.append(s[i : end_tick + 1])
            i = end_tick + 1
    return "".join(out)


def _attach_backtick_suffixes(s: str) -> str:
    s = _repair_redundant_backticks(s)
    s = _attach_backticked_snake_case_parentheses(s)
    s = re.sub(
        rf"(?<![A-Za-z0-9_])(?P<sign>{_SIGNED_UNSIGNED_WORD})[ \t]+"
        rf"`(?P<type>{_FUNDAMENTAL_INTEGER_AFTER_SIGN})`(?![A-Za-z0-9_])",
        lambda match: f"`{match.group('sign')} {match.group('type').lower()}`",
        s,
        flags=re.IGNORECASE,
    )
    s = re.sub(r"[ \t]*<[ \t]*`operator-`[ \t]*", " <- ", s)
    suffix = r"'s|'d|'|ed|es|s"
    s = re.sub(rf"`([^`\n]+)`[ \t]+({suffix})(?=\W|$)", r"`\1`\2", s)
    s = re.sub(r"`([^`()\n]+)`[ \t]*\(\)", r"`\1()`", s)
    s = re.sub(r"`([^`\n]*[_-][^`\n]*)`[ \t]*\*", r"`\1*`", s)
    s = re.sub(rf"`([^`\n]+)`({suffix})?[ \t]+(-[A-Za-z0-9])", r"`\1`\2\3", s)
    s = re.sub(rf"`([^`\n]+)`({suffix})?[ \t]+(\.{{2,}})", r"`\1`\2\3", s)
    s = re.sub(r"(\.{2,})[ \t]+`([^`\n]+)`", r"\1`\2`", s)
    s = re.sub(r"`([^`\n]+)`[ \t]*/[ \t]*`([^`\n]+)`", r"`\1`/`\2`", s)
    s = re.sub(rf"`([^`\n]+)`({suffix})?[ \t]+([,.?!])", r"`\1`\2\3", s)
    return _repair_redundant_backticks(s)


def _split_backtick_suffix(s: str) -> tuple[str, str]:
    for suffix in ("'s", "'d", "'", "ed"):
        if s.endswith(suffix) and len(s) > len(suffix):
            return s[: -len(suffix)], suffix
    return s, ""


def _core_token(token: str) -> str:
    return token.strip("([{}]).,!?")


def _is_clang_tool_core(core: str) -> bool:
    core, _suffix = _split_backtick_suffix(core)
    lowered = core.lower()
    if lowered in _OFFICIAL_CLANG_TOOLS:
        return True
    if not lowered.startswith("clang-"):
        return False
    return any(part in {"lto", "repl"} for part in lowered.split("-")[1:])


def _is_isa_extension_title_core(core: str) -> bool:
    core, _suffix = _split_backtick_suffix(core)
    core = core.rstrip("*")
    if not core:
        return False
    if re.fullmatch(r"[A-Z][A-Z0-9.+-]*", core) is None:
        return False
    return any(core.startswith(prefix) for prefix in _ISA_EXTENSION_TITLE_PREFIXES)


def _is_kebab_case_word_core(core: str) -> bool:
    return (
        re.fullmatch(r"[A-Za-z][A-Za-z0-9]*(?:-[A-Za-z][A-Za-z0-9]*)+", core)
        is not None
    )


def _has_cat_kebab_segment(core: str) -> bool:
    return "cat" in core.lower().split("-")


def _is_cpp_operator_token(token: str) -> bool:
    return token in _CPP_OPERATOR_TOKENS


def _operator_name_spelling(operator: str) -> str:
    compact = re.sub(r"[ \t]+", "", operator)
    lowered = compact.lower()
    if lowered in {"new", "new[]", "delete", "delete[]", "co_await"}:
        return f"operator {lowered}"
    return f"operator{compact}"


def _restyle_plain_comment_text(
    t: str,
    protect_kebab_case: bool = False,
    protect_asm_phrases: bool = False,
) -> str:
    protected: list[str] = []

    def is_numeric_literal(s: str) -> bool:
        return _re_numeric_literal.fullmatch(s) is not None

    def is_numeric_literal_expression_or_version(s: str) -> bool:
        return (
            _re_numeric_literal.fullmatch(s) is not None
            or _re_numeric_expression.fullmatch(s) is not None
            or _re_version_number.fullmatch(s) is not None
        )

    def protect_token_core(token: str, lowercase_core: bool = False) -> str:
        if "`" in token or "\x00CATRESTYLE" in token:
            return token
        left = 0
        right = len(token)
        while left < right and token[left] in "([{":
            left += 1
        while left < right:
            c = token[right - 1]
            if c not in ".,!?)]}":
                break
            if c == ")" and right >= 2 and token[right - 2] == "(":
                break
            right -= 1
        core = token[left:right]
        core, suffix = _split_backtick_suffix(core)
        if lowercase_core:
            core = core.lower()
        protected.append(f"`{core}`{suffix}")
        marker = f"\x00CATRESTYLE{len(protected) - 1}\x00"
        return token[:left] + marker + token[right:]

    def protect(match: re.Match[str]) -> str:
        core = match.group(1)
        if "`" in core:
            return core
        protected.append(f"`{core}`")
        return f"\x00CATRESTYLE{len(protected) - 1}\x00"

    def find_matching_paren(text: str, start: int) -> int | None:
        depth = 0
        i = start
        while i < len(text):
            if text[i] == "\x00":
                marker = re.match(r"\x00CATRESTYLE\d+\x00", text[i:])
                if marker is not None:
                    i += marker.end()
                    continue
            if text[i] == "(":
                depth += 1
            elif text[i] == ")":
                depth -= 1
                if depth == 0:
                    return i
            i += 1
        return None

    def protect_snake_case_calls(text: str) -> str:
        out: list[str] = []
        i = 0
        pattern = re.compile(
            r"(?<![A-Za-z0-9_])"
            r"((?:[A-Za-z_][A-Za-z0-9_]*::)*"
            r"[A-Za-z_][A-Za-z0-9_]*_[A-Za-z0-9_]*)\("
        )
        while i < len(text):
            marker = re.match(r"\x00CATRESTYLE\d+\x00", text[i:])
            if marker is not None:
                out.append(marker.group(0))
                i += marker.end()
                continue
            match = pattern.match(text, i)
            if match is None:
                out.append(text[i])
                i += 1
                continue
            end = find_matching_paren(text, match.end() - 1)
            if end is None:
                out.append(text[i])
                i += 1
                continue
            core = text[i : end + 1]
            protected.append(f"`{core}`")
            out.append(f"\x00CATRESTYLE{len(protected) - 1}\x00")
            i = end + 1
        return "".join(out)

    def protect_scope(match: re.Match[str]) -> str:
        token = match.group(1)
        if "::" not in token:
            return token
        return protect_token_core(token)

    def protect_tool(match: re.Match[str]) -> str:
        token = match.group(1)
        if not _is_clang_tool_core(_core_token(token)):
            return token
        return protect_token_core(token, True)

    def protect_flag(match: re.Match[str]) -> str:
        token = match.group(1)
        if re.fullmatch(r"-[A-Za-z]|--[A-Za-z][A-Za-z-]*", _core_token(token)) is None:
            return token
        return protect_token_core(token)

    def preserve_token(match: re.Match[str]) -> str:
        token = match.group(1)
        if token == "/":
            return token
        if "`" in token:
            return token
        if "\x00CATRESTYLE" in token:
            return token
        core = _core_token(token)
        if core == "compile_commands.json" or core.endswith("/compile_commands.json"):
            return protect_token_core(token)
        protected.append(token)
        return f"\x00CATRESTYLE{len(protected) - 1}\x00"

    def protect_cmake_config(match: re.Match[str]) -> str:
        protected.append(f"`{match.group('config')}`")
        return f"\x00CATRESTYLE{len(protected) - 1}\x00"

    def protect_dollar(match: re.Match[str]) -> str:
        token = match.group(1)
        if "`" in token:
            return token
        left = 0
        right = len(token)
        while left < right and token[left] in "([":
            left += 1
        while left < right and token[right - 1] in ".,!?)]":
            right -= 1
        core = token[left:right]
        core, suffix = _split_backtick_suffix(core)
        protected.append(f"`{core}`{suffix}")
        marker = f"\x00CATRESTYLE{len(protected) - 1}\x00"
        return token[:left] + marker + token[right:]

    def protect_call(match: re.Match[str]) -> str:
        core = match.group("call")
        if "`" in core:
            return core + (match.group("suffix") or "")
        protected.append(f"`{core}`{match.group('suffix') or ''}")
        return f"\x00CATRESTYLE{len(protected) - 1}\x00"

    def protect_attribute(match: re.Match[str]) -> str:
        core = match.group(0)
        protected.append(f"`{core}`")
        return f"\x00CATRESTYLE{len(protected) - 1}\x00"

    def protect_asm_phrase(match: re.Match[str]) -> str:
        protected.append(f"`{match.group(0).lower()}`")
        return f"\x00CATRESTYLE{len(protected) - 1}\x00"

    def protect_signed_unsigned_integer_phrase(match: re.Match[str]) -> str:
        protected.append(f"`{_signed_unsigned_integer_phrase_spelling(match)}`")
        return f"\x00CATRESTYLE{len(protected) - 1}\x00"

    def protect_short_long_integer_phrase(match: re.Match[str]) -> str:
        protected.append(f"`{match.group('type').lower()}`")
        return f"\x00CATRESTYLE{len(protected) - 1}\x00"

    def protect_long_double_phrase(match: re.Match[str]) -> str:
        protected.append(f"`{match.group('type').lower()}`")
        return f"\x00CATRESTYLE{len(protected) - 1}\x00"

    def protect_fundamental_type(match: re.Match[str]) -> str:
        protected.append(f"`{match.group(0).lower()}`")
        return f"\x00CATRESTYLE{len(protected) - 1}\x00"

    def protect_kebab(match: re.Match[str]) -> str:
        token = match.group(1)
        core = _core_token(token)
        if (
            core.lower() in _VALUE_CATEGORY_SPELLING.values()
            or core.lower() == "deducing-this"
            or core in {"-", "--"}
            or is_numeric_literal_expression_or_version(core)
        ):
            return token
        if core != core.lower():
            return token
        if re.fullmatch(r"[a-z0-9][a-z0-9]*(?:-[a-z0-9]+)+(?:\*)?", core) is None:
            return token
        if not _has_cat_kebab_segment(core) and not _is_clang_tool_core(core):
            return token
        return protect_token_core(token)

    def protect_dot(match: re.Match[str]) -> str:
        token = match.group(1)
        core = _core_token(token)
        if "." not in core:
            return token
        if _is_isa_extension_title_core(core):
            return token
        if _normalize_abbreviation(core) is not None:
            return token
        if _normalize_abbreviation(core.lstrip("([{")) is not None:
            return token
        if is_numeric_literal_expression_or_version(core):
            return token
        return protect_token_core(token)

    def protect_snake_case(match: re.Match[str]) -> str:
        return protect(match)

    def protect_template_parameter(match: re.Match[str]) -> str:
        protected.append(f"`{match.group('parameter')}`")
        return f"\x00CATRESTYLE{len(protected) - 1}\x00"

    def protect_if_constexpr(match: re.Match[str]) -> str:
        protected.append("`if constexpr`")
        return f"\x00CATRESTYLE{len(protected) - 1}\x00"

    def protect_operator_name(match: re.Match[str]) -> str:
        protected.append(f"`{_operator_name_spelling(match.group('operator'))}`")
        return f"\x00CATRESTYLE{len(protected) - 1}\x00"

    def restyle_compiler_version(match: re.Match[str]) -> str:
        name = "Clang" if match.group("name").lower() == "clang" else "GCC"
        return f"{name} {match.group('version')}"

    def restyle_deducing_this(match: re.Match[str]) -> str:
        return re.sub(r"[- ]", "-", match.group(0))

    def restyle_numeric_bit_byte(match: re.Match[str]) -> str:
        return f"{match.group('number')}-{match.group('unit').lower()}"

    def restyle_only_category(match: re.Match[str]) -> str:
        return f"{match.group('operation')}-only"

    def restyle_opt_category(match: re.Match[str]) -> str:
        return f"opt-{match.group('direction').lower()}"

    def restyle_non_trivial(match: re.Match[str]) -> str:
        return f"{match.group('prefix')}-{match.group('form')}"

    def restyle_cpp_trait_phrase(match: re.Match[str]) -> str:
        parts = [match.group("prefix")]
        operation = match.group("operation")
        if operation is not None:
            parts.append(operation.lower())
        parts.append(match.group("trait").lower())
        return "-".join(parts)

    def restyle_ctor_dtor(match: re.Match[str]) -> str:
        word = match.group("word")
        replacement = "constructor" if word.lower() == "ctor" else "destructor"
        if word.isupper():
            return replacement.upper()
        if word[0].isupper():
            return replacement.capitalize()
        return replacement

    def restyle_x_protocol(match: re.Match[str]) -> str:
        return "X protocol"

    def restyle_acronym(match: re.Match[str]) -> str:
        return match.group("word").upper() + (match.group("suffix") or "")

    def restyle_lib(match: re.Match[str]) -> str:
        return _LIB_SPELLING[match.group("name").lower()]

    def restyle_asm(match: re.Match[str]) -> str:
        return "asm" + (match.group("suffix") or "").lower()

    def restyle_signedness(match: re.Match[str]) -> str:
        return f"{match.group('sign')}-ness"

    t = _re_deducing_this.sub(restyle_deducing_this, t)
    t = _re_x_protocol.sub(restyle_x_protocol, t)
    t = _re_numeric_bit_byte.sub(restyle_numeric_bit_byte, t)
    t = _re_only_category.sub(restyle_only_category, t)
    t = _re_opt_category.sub(restyle_opt_category, t)
    t = _re_non_trivial.sub(restyle_non_trivial, t)
    t = _re_cpp_trait_phrase.sub(restyle_cpp_trait_phrase, t)
    t = _re_ctor_dtor_word.sub(restyle_ctor_dtor, t)
    t = _re_abbreviation.sub(lambda match: _normalize_abbreviation(match.group(0)), t)
    t = _re_compiler_version.sub(restyle_compiler_version, t)
    t = _re_signedness_word.sub(restyle_signedness, t)
    t = _re_attribute_token.sub(protect_attribute, t)
    t = _re_if_constexpr_phrase.sub(protect_if_constexpr, t)
    if protect_asm_phrases:
        t = _re_asm_phrase.sub(protect_asm_phrase, t)
        t = _re_signed_unsigned_integer_phrase.sub(
            protect_signed_unsigned_integer_phrase, t
        )
        t = _re_short_long_integer_phrase.sub(protect_short_long_integer_phrase, t)
        t = _re_long_double_phrase.sub(protect_long_double_phrase, t)
        t = _re_fundamental_type_word.sub(protect_fundamental_type, t)
    if protect_kebab_case:
        t = _re_cmake_config_word.sub(protect_cmake_config, t)
    t = _re_constexpr_consteval_word.sub(
        lambda match: f"`{match.group(0).lower()}`", t
    )
    t = _re_operator_name.sub(protect_operator_name, t)
    t = _re_path_token.sub(preserve_token, t)
    t = _re_command_line_flag.sub(protect_flag, t)
    t = _re_identifier_with_dollar.sub(protect_dollar, t)
    t = _re_include_token.sub(protect_tool, t)
    t = _re_clang_tool.sub(protect_tool, t)
    t = _re_identifier_with_kebab.sub(protect_kebab, t)
    t = _re_identifier_with_dot.sub(protect_dot, t)
    t = protect_snake_case_calls(t)
    t = _re_identifier_with_underscore.sub(protect_snake_case, t)
    t = _re_identifier_with_scope.sub(protect_scope, t)
    t = _re_identifier_call.sub(protect_call, t)
    t = _re_template_parameter_word.sub(protect_template_parameter, t)
    t = _re_clang_word.sub("Clang", t)
    t = _re_gcc_word.sub("GCC", t)
    t = _re_lib_word.sub(restyle_lib, t)
    t = _re_acronym_word.sub(restyle_acronym, t)
    t = _re_asm_word.sub(restyle_asm, t)
    t = _re_const_word.sub("`const`", t)
    t = _re_null_literal_word.sub(lambda match: f"`{match.group(0)}`", t)

    def restyle_value_category(match: re.Match[str]) -> str:
        return _VALUE_CATEGORY_SPELLING[match.group("category").lower()] + (
            match.group("suffix") or ""
        ).lower()

    t = _re_value_category.sub(restyle_value_category, t)
    for i, replacement in enumerate(protected):
        t = t.replace(f"\x00CATRESTYLE{i}\x00", replacement)
    return _attach_backtick_suffixes(t)


def _restyle_backtick_text(t: str, normalize_asm_phrase: bool = False) -> str:
    if _re_numeric_literal.fullmatch(t) is not None:
        return t
    if _re_version_number.fullmatch(t) is not None:
        return t
    if _is_cpp_operator_token(t):
        return t
    signed_unsigned_integer = _re_signed_unsigned_integer_phrase.fullmatch(t)
    if signed_unsigned_integer is not None:
        return f"`{_signed_unsigned_integer_phrase_spelling(signed_unsigned_integer)}`"
    short_long_integer = _re_short_long_integer_phrase.fullmatch(t)
    if short_long_integer is not None:
        return f"`{short_long_integer.group('type').lower()}`"
    long_double = _re_long_double_phrase.fullmatch(t)
    if long_double is not None:
        return f"`{long_double.group('type').lower()}`"
    signedness = _re_signedness_word.fullmatch(t)
    if signedness is not None:
        return f"{signedness.group('sign')}-ness"
    if _re_signed_unsigned_word.fullmatch(t) is not None:
        return t
    if _re_short_long_word.fullmatch(t) is not None:
        return f"`{t}`"
    if _re_double_word.fullmatch(t) is not None:
        return f"`{t}`"
    if _re_constexpr_consteval_word.fullmatch(t) is not None:
        return f"`{t.lower()}`"
    operator_name = _re_operator_name.fullmatch(t)
    if operator_name is not None:
        return f"`{_operator_name_spelling(operator_name.group('operator'))}`"
    version = _re_compiler_version.fullmatch(t)
    if version is not None:
        name = "Clang" if version.group("name").lower() == "clang" else "GCC"
        return f"{name} {version.group('version')}"
    if _is_isa_extension_title_core(t):
        return t
    lib = _re_lib_word.fullmatch(t)
    if lib is not None and lib.group("name").lower() in {"libc", "libm"}:
        return _LIB_SPELLING[lib.group("name").lower()]
    if (
        _re_clang_word.fullmatch(t) is not None
        or _re_gcc_word.fullmatch(t) is not None
        or _re_lib_word.fullmatch(t) is not None
        or _re_acronym_word.fullmatch(t) is not None
    ):
        return f"`{t}`"
    if _re_clang_word.fullmatch(t) is not None:
        return "Clang"
    if _re_gcc_word.fullmatch(t) is not None:
        return "GCC"
    if lib is not None:
        return _LIB_SPELLING[lib.group("name").lower()]
    if _is_clang_tool_core(t):
        return f"`{t.lower()}`"
    acronym = _re_acronym_word.fullmatch(t)
    if acronym is not None:
        return acronym.group("word").upper() + (acronym.group("suffix") or "")
    if _is_kebab_case_word_core(t):
        return f"`{t}`"
    deducing_this = _re_deducing_this.fullmatch(t)
    if deducing_this is not None:
        return re.sub(r"[- ]", "-", deducing_this.group(0))
    abbreviation = _normalize_abbreviation(t)
    if abbreviation is not None:
        return abbreviation
    if normalize_asm_phrase and _re_asm_phrase.fullmatch(t) is not None:
        return f"`{t.lower()}`"
    return f"`{t}`"


def _restyle_comment_text(
    t: str,
    protect_kebab_case: bool = False,
    protect_asm_phrases: bool = False,
) -> str:
    out: list[str] = []
    plain: list[str] = []
    i = 0
    n = len(t)

    def flush_plain() -> None:
        if plain:
            out.append(
                _restyle_plain_comment_text(
                    "".join(plain),
                    protect_kebab_case,
                    protect_asm_phrases,
                )
            )
            plain.clear()

    while i < n:
        url = _re_http_url.match(t, i)
        if url is not None:
            flush_plain()
            out.append(url.group(0))
            i = url.end()
            continue
        if t[i] == "`":
            j = t.find("`", i + 1)
            if j != -1:
                flush_plain()
                body = t[i + 1 : j]
                replacement = _restyle_backtick_text(body, protect_asm_phrases)
                out.append(replacement)
                i = j + 1
                if replacement.endswith(".") and i < n and t[i] == ".":
                    i += 1
                continue
        plain.append(t[i])
        i += 1
    flush_plain()
    return _attach_backtick_suffixes("".join(out))


def _move_period_after_closing_paren_text(t: str) -> str:
    protected: list[str] = []

    def protect_abbreviation_close_paren(match: re.Match[str]) -> str:
        protected.append(match.group(0))
        return f"\x00CATRESTYLEABBRPAREN{len(protected) - 1}\x00"

    t = re.sub(
        r"\b((?:e\.g\.|i\.e\.|ex\.|etc\.)\))\.(?=[ \t]+[a-z])",
        r"\1",
        t,
        flags=re.IGNORECASE,
    )
    t = _re_abbreviation_close_paren.sub(protect_abbreviation_close_paren, t)
    t = t.replace("!).", "!)").replace("?).", "?)")
    t = re.sub(r"(\.{3,}\))\.$", r"\1", t)
    t = re.sub(r"(?<!\.)\.\)\.?", ").", t)
    t = re.sub(r"\)\.{4,}", ")...", t)
    for i, replacement in enumerate(protected):
        t = t.replace(f"\x00CATRESTYLEABBRPAREN{i}\x00", replacement)
    return t


def _move_period_after_closing_paren_line(L: str, style: str = "slash") -> str:
    t, ending = _line_ending(L)
    pattern = _comment_any_prefix_pattern(style)
    m = re.match(rf"^(\s*{pattern}\s?)(.*)$", t)
    if m is None:
        return L
    return m.group(1) + _move_period_after_closing_paren_text(m.group(2)) + ending


def _line_starts_with_punctuation_continuation(
    L: str | None, style: str = "slash"
) -> bool:
    if L is None:
        return False
    body = L.lstrip()
    if re.match(r"TODO:", body, re.IGNORECASE) is not None:
        return False
    if body.startswith(("`", "(")):
        return True
    if not _is_any_comment_line(L, style):
        return False
    comment_body = _comment_body(L, style, any_prefix=True).lstrip()
    if re.match(r"TODO:", comment_body, re.IGNORECASE) is not None:
        return False
    return comment_body.startswith(("`", "("))


def _restyle_comment_line(
    L: str,
    style: str = "slash",
    protect_kebab_case: bool = False,
) -> str:
    t, ending = _line_ending(L)
    pattern = _comment_prefix_pattern(style)
    m = re.match(rf"^(\s*{pattern}\s?)(.*)$", t)
    if m is None:
        return L
    body = m.group(2)
    body = _restyle_comment_text(body, protect_kebab_case, style == "slash")
    if style == "slash":
        body = _replace_semicolon_sentence_breaks_text(body)
    return (
        m.group(1)
        + _move_period_after_closing_paren_text(body)
        + ending
    )


def _repair_split_backtick_comment_block(
    block: list[str], style: str = "slash"
) -> list[str]:
    out = block[:]
    pattern = _comment_prefix_pattern(style)
    for i in range(len(out) - 1):
        t, ending = _line_ending(out[i])
        m = re.match(rf"^(\s*{pattern}\s?)(.*)$", t)
        if m is None:
            continue
        body = m.group(2)
        if body.count("`") % 2 == 1:
            next_t, next_ending = _line_ending(out[i + 1])
            next_m = re.match(rf"^(\s*{pattern}\s?)(.*)$", next_t)
            if next_m is not None:
                next_body = next_m.group(2)
                close = next_body.find("`")
                if close != -1:
                    start = body.rfind("`")
                    next_text = re.match(r"^(\s*)(.*)$", next_body)
                    if next_text is None:
                        continue
                    out[i] = m.group(1) + body[:start].rstrip() + ending
                    out[i + 1] = (
                        next_m.group(1)
                        + next_text.group(1)
                        + body[start:].rstrip()
                        + " "
                        + next_text.group(2).lstrip()
                        + next_ending
                    )
                    continue
        if not body.rstrip().endswith("``"):
            continue
        next_t, next_ending = _line_ending(out[i + 1])
        next_m = re.match(rf"^(\s*{pattern}\s?)(.*)$", next_t)
        if next_m is None:
            continue
        next_body = next_m.group(2)
        next_text = re.match(r"^(\s*)(\S.*)$", next_body)
        if next_text is None or next_text.group(2).startswith("`"):
            continue
        if re.match(r"\S*`[.,!?;:)\]}]*(?:\s|$)", next_text.group(2)) is None:
            continue
        out[i] = m.group(1) + body.rstrip()[:-2].rstrip() + ending
        out[i + 1] = (
            next_m.group(1)
            + next_text.group(1)
            + "`"
            + next_text.group(2)
            + next_ending
        )
    return out


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
        elif t[i] == "`":
            j = t.find("`", i + 1)
            if j != -1:
                end = _backtick_suffix_end(t, j + 1)
                while end < n and t[end] in ".,!?;:)]}":
                    end += 1
                words.append(t[i:end])
                i = end
            else:
                m2 = re.match(r"\S+", t[i:])
                if m2 is None:
                    i += 1
                    continue
                words.append(m2.group(0))
                i = i + m2.end()
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
            test_words = cur + [w]
            test = prefix + _join_reflow_words(test_words)
        if cur and _col(test) > max_col:
            out.append(prefix + _join_reflow_words(cur) + "\n")
            cur = [w]
        elif (not cur) and _is_url_token(w) and _col(test) > max_col:
            out.append(prefix + w + "\n")
        else:
            cur.append(w)
    if cur:
        out.append(prefix + _join_reflow_words(cur) + "\n")
    return out


def _words_from_block(body_lines: list[str], style: str = "slash") -> list[str]:
    words: list[str] = []
    for L in body_lines:
        t = _comment_body(L, style, any_prefix=True).strip()
        if t:
            words.extend(_words_from_text(t))
    return _repair_split_backtick_words(words)


def _repair_split_backtick_words(words: list[str]) -> list[str]:
    repaired: list[str] = []
    i = 0
    while i < len(words):
        word = words[i]
        while (
            i + 1 < len(words)
            and re.search(r"\.{2,}$", word) is not None
            and words[i + 1].startswith("`")
        ):
            word += words[i + 1]
            i += 1
        if (
            i + 2 < len(words)
            and word.endswith("`")
            and re.fullmatch(r"\.{2,}", words[i + 1]) is not None
            and words[i + 2].startswith("`")
        ):
            word += words[i + 1] + words[i + 2]
            i += 2
        if word != words[i]:
            repaired.append(word)
            i += 1
            continue
        next_fragment = words[i + 1] if i + 1 < len(words) else ""
        split_backtick = re.match(r"([^`]+`)([.,!?;:)\]}]*)$", next_fragment)
        if (
            split_backtick is not None
            and (words[i] == "``" or words[i].endswith("``"))
            and not next_fragment.startswith("`")
        ):
            if words[i] == "``":
                repaired.append("`" + split_backtick.group(1) + split_backtick.group(2))
            else:
                repaired.append(
                    words[i][:-1] + split_backtick.group(1) + split_backtick.group(2)
                )
            i += 2
            continue
        repaired.append(words[i])
        i += 1
    return repaired


def _space_between_reflow_words(left: str, right: str) -> bool:
    if re.search(r"\.{2,}$", left) is not None and right.startswith("`"):
        return False
    if left.endswith("`") and re.match(r"\.{2,}", right) is not None:
        return False
    return True


def _join_reflow_words(words: list[str]) -> str:
    if not words:
        return ""
    out = [words[0]]
    for word in words[1:]:
        if _space_between_reflow_words(out[-1], word):
            out.append(" ")
        out.append(word)
    return "".join(out)


def _comment_line_body_ends_hard_break(L: str, style: str = "slash") -> bool:
    body = _comment_body(L, style, any_prefix=True).rstrip()
    return body.endswith((".", "?", "!", ":", "?)", "!)", "...)")) or (
        _comment_body_ends_with_url(body)
        or _comment_body_ends_with_abbreviation_close_paren(body)
    )


def _comment_body_has_terminal_punctuation(body: str) -> bool:
    return (
        body.endswith(_TERMINAL_COMMENT_PUNCTUATION)
        or body.endswith(("?)", "!)", "...)"))
        or _comment_body_ends_with_abbreviation_close_paren(body)
        or re.search(r"`[^`\n]*;`$", body) is not None
    )


def _last_comment_body_token(body: str) -> str:
    parts = body.rstrip().rsplit(maxsplit=1)
    return parts[-1] if parts else ""


def _comment_body_ends_with_url(body: str) -> bool:
    return _re_http_url.fullmatch(_last_comment_body_token(body)) is not None


def _comment_body_ends_with_abbreviation_close_paren(body: str) -> bool:
    stripped = body.rstrip()
    match = _re_abbreviation_close_paren.search(stripped)
    return match is not None and match.end() == len(stripped)


def _comment_body_is_url(body: str) -> bool:
    return _re_http_url.fullmatch(body.strip()) is not None


def _comment_line_body_is_url(L: str, style: str = "slash") -> bool:
    return _comment_body_is_url(_comment_body(L, style, any_prefix=True))


def _comment_body_ends_with_url_period(body: str) -> bool:
    token = _last_comment_body_token(body)
    return token.endswith(".") and _re_http_url.fullmatch(token[:-1]) is not None


def _remove_terminal_url_period_line(L: str, style: str = "slash") -> str:
    t, ending = _line_ending(L)
    pattern = _comment_any_prefix_pattern(style)
    m = re.match(rf"^(\s*{pattern}\s?)(.*)$", t)
    if m is None:
        return L
    body = m.group(2)
    stripped = body.rstrip()
    if not stripped.endswith("."):
        return L
    return m.group(1) + stripped[:-1] + ending


def _is_terminal_punctuation_boundary(
    following_line: str | None,
    style: str = "slash",
    following_is_code_block: bool = False,
) -> bool:
    if following_line is None or following_is_code_block:
        return True
    if not following_line.strip():
        return True
    if _is_empty_line_comment(following_line, style):
        return True
    return not _is_line_comment(following_line, style)


def _ensure_terminal_comment_punctuation(
    lines: list[str],
    style: str = "slash",
    following_line: str | None = None,
    following_is_code_block: bool = False,
) -> list[str]:
    out = [_move_period_after_closing_paren_line(L, style) for L in lines]
    for i in range(len(out) - 1, -1, -1):
        body = _comment_body(out[i], style, any_prefix=True).rstrip()
        if not body:
            continue
        if _comment_body_ends_with_url_period(body):
            out[i] = _remove_terminal_url_period_line(out[i], style)
            return out
        if _comment_body_ends_with_url(body):
            return out
        if _comment_body_has_terminal_punctuation(body):
            return out
        if not _is_terminal_punctuation_boundary(
            following_line, style, following_is_code_block
        ):
            return out
        if _line_starts_with_punctuation_continuation(following_line, style):
            return out
        t, ending = _line_ending(out[i])
        pattern = _comment_any_prefix_pattern(style)
        m = re.match(rf"^(\s*{pattern}\s?)(.*)$", t)
        if m is None:
            return out
        out[i] = m.group(1) + m.group(2).rstrip() + "." + ending
        return out
    return out


def _reflow_comment_block(
    prefix: str, block: list[str], max_col: int, style: str = "slash"
) -> list[str]:
    out: list[str] = []
    segment: list[str] = []
    for L in block:
        if _comment_line_body_is_url(L, style):
            if segment:
                out.extend(
                    _reflow_words(prefix, _words_from_block(segment, style), max_col)
                )
                segment = []
            out.append(L)
            continue
        segment.append(L)
        if _comment_line_body_ends_hard_break(L, style):
            out.extend(
                _reflow_words(prefix, _words_from_block(segment, style), max_col)
            )
            segment = []
    if segment:
        out.extend(_reflow_words(prefix, _words_from_block(segment, style), max_col))
    return out


def _starts_comment_block(lines: list[str], index: int, style: str) -> bool:
    if index == 0:
        return True
    previous = lines[index - 1].rstrip("\n\r")
    if _is_any_comment_line(previous, style):
        return False
    pattern = _comment_any_prefix_pattern(style)
    return re.search(rf"{pattern}\s*\S", previous) is None


def _capitalization_core(token: str) -> str:
    return token.strip("'\"([{}]).,!?;:")


def _should_skip_paragraph_start_capitalization(core: str) -> bool:
    if not core:
        return True
    if (
        "_" in core
        or "-" in core
        or "." in core
        or "::" in core
        or "$" in core
        or "(" in core
        or ")" in core
        or "<" in core
        or ">" in core
    ):
        return True
    if _normalize_abbreviation(core) is not None:
        return True
    if _re_lib_word.fullmatch(core) is not None:
        return True
    if _re_asm_word.fullmatch(core) is not None:
        return True
    if core.lower() in _VALUE_CATEGORY_SPELLING.values():
        return True
    if core.lower() == "deducing-this":
        return True
    return False


def _capitalize_paragraph_start_text(body: str) -> str:
    if _is_modeline_comment_text(body):
        return body
    if re.match(r"^\s*`", body) is not None:
        return body
    match = re.match(r"^(\s*)(\S+)", body)
    if match is None:
        return body
    word_start = len(match.group(1))
    first = body[word_start]
    if not (first.isascii() and first.isalpha()):
        return body
    core = _capitalization_core(match.group(2))
    if _should_skip_paragraph_start_capitalization(core):
        return body
    return body[:word_start] + first.upper() + body[word_start + 1 :]


def _capitalize_paragraph_start_line(L: str, style: str = "slash") -> str:
    t, ending = _line_ending(L)
    pattern = _comment_prefix_pattern(style)
    m = re.match(rf"^(\s*{pattern}\s?)(.*)$", t)
    if m is None:
        return L
    return m.group(1) + _capitalize_paragraph_start_text(m.group(2)) + ending


def _is_paragraph_continuation_line(
    lines: list[str],
    index: int,
    prefix: str | None,
    style: str,
    code_lines: set[int],
    protect_kebab_case: bool,
    preserve_python_usage_stanza: bool,
) -> bool:
    if prefix is None or index >= len(lines):
        return False
    if not _is_line_comment(lines[index], style):
        return False
    if _is_empty_line_comment(lines[index], style):
        return False
    if _is_modeline_comment_text(_comment_body(lines[index], style, any_prefix=True)):
        return False
    if index in code_lines:
        return False
    if _nolint_in_comment_body(lines[index], style):
        return False
    if _is_todo_comment_line(lines[index], style):
        return False
    if preserve_python_usage_stanza and _is_usage_comment_line(lines[index], style):
        return False
    if _is_linter_or_tool_directive_line(lines[index], style, protect_kebab_case):
        return False
    return _comment_prefix(lines[index], style) == prefix


def process_lines(
    lines: list[str],
    max_col: int,
    style: str = "slash",
    protect_kebab_case: bool = False,
    preserve_python_usage_stanza: bool = False,
    allow_colon_code_intro: bool = False,
) -> list[str]:
    out: list[str] = []
    n = len(lines)
    code_lines = _code_block_comment_line_indices(lines, style, allow_colon_code_intro)
    i = 0
    while i < n:
        L = lines[i]
        if not _is_line_comment(L, style):
            out.append(L)
            i += 1
            continue
        if _is_empty_line_comment(L, style):
            out.append(L)
            i += 1
            continue
        if _is_modeline_comment_text(_comment_body(L, style, any_prefix=True)):
            out.append(L)
            i += 1
            continue
        if i in code_lines:
            out.extend(_split_code_block_semicolon_comment_line(L, style))
            i += 1
            continue
        if _nolint_in_comment_body(L, style):
            out.append(L)
            i += 1
            continue
        if _is_todo_comment_line(L, style):
            restyled = [_restyle_comment_line(L, style, protect_kebab_case)]
            out.extend(restyled)
            i += 1
            continue
        if preserve_python_usage_stanza and _is_usage_comment_line(L, style):
            out.append(L)
            i += 1
            while (
                i < n
                and _is_line_comment(lines[i], style)
                and not _is_empty_line_comment(lines[i], style)
            ):
                out.append(lines[i])
                i += 1
            continue
        if _is_linter_or_tool_directive_line(L, style, protect_kebab_case):
            out.append(L)
            i += 1
            continue
        j = i
        block: list[str] = []
        p0 = _comment_prefix(L, style)
        if p0 is None:
            out.append(L)
            i += 1
            continue
        while j < n and _is_line_comment(lines[j], style):
            if _is_empty_line_comment(lines[j], style):
                break
            if _is_modeline_comment_text(
                _comment_body(lines[j], style, any_prefix=True)
            ):
                break
            if j in code_lines:
                break
            if _nolint_in_comment_body(lines[j], style):
                break
            if _is_todo_comment_line(lines[j], style):
                break
            if preserve_python_usage_stanza and _is_usage_comment_line(
                lines[j], style
            ):
                break
            if _is_linter_or_tool_directive_line(
                lines[j], style, protect_kebab_case
            ):
                break
            if _comment_prefix(lines[j], style) != p0:
                break
            block.append(lines[j])
            j += 1
        if not block:
            out.append(L)
            i += 1
            continue
        prefix = p0
        block = _repair_split_backtick_comment_block(block, style)
        restyled_block = [
            _restyle_comment_line(b, style, protect_kebab_case) for b in block
        ]
        if _starts_comment_block(lines, i, style):
            restyled_block[0] = _capitalize_paragraph_start_line(
                restyled_block[0], style
            )
        if any(_preserve_reflow_stanza_line(b, style) for b in block):
            out.extend(restyled_block)
            i = j
            continue
        new_block = _reflow_comment_block(prefix, restyled_block, max_col, style)
        following_line = lines[j] if j < n else None
        out.extend(
            _ensure_terminal_comment_punctuation(
                new_block, style, following_line, j in code_lines
            )
        )
        i = j
    return out


def read_lines(path: str) -> list[str]:
    with open(path, encoding="utf-8", newline="") as f:
        return f.readlines()


def write_lines(path: str, lines: list[str]) -> None:
    with open(path, "w", encoding="utf-8", newline="") as f:
        f.writelines(lines)


def _colon_semicolon_punctuation_changed(old: list[str], new: list[str]) -> bool:
    return "".join(c for L in old for c in L if c in ":;") != "".join(
        c for L in new for c in L if c in ":;"
    )


_RestyleOne = Tuple[str, bool, str | None, list[_Warning], bool]


def _restyle_one(t: tuple[str, int, int, bool]) -> _RestyleOne:
    path, max_col, python_max_col, check_only = t
    if not path:
        return "", False, None, [], False
    if _is_dotfile_path(path):
        return path, False, None, [], False
    try:
        lines = read_lines(path)
    except OSError as e:
        return path, False, str(e), [], False
    style = "hash" if _is_hash_style_path(path) else "slash"
    preserved_indices = (
        _preserved_cmake_preprocessor_line_indices(lines, style)
        if _is_cmake_path(path)
        else set()
    )
    modeline_lines = _lowercase_modeline_comment_lines(lines, style)
    modeline_lines = _restore_lines_at_indices(modeline_lines, lines, preserved_indices)
    normalized_lines = _ascii_punctuation_comment_lines(modeline_lines, style)
    normalized_lines = _restore_lines_at_indices(
        normalized_lines, lines, preserved_indices
    )
    new = process_lines(
        normalized_lines,
        _effective_max_col(path, max_col, python_max_col),
        style,
        _is_cmake_path(path),
        _is_python_path(path),
        _is_python_path(path),
    )
    new = _strip_edge_rule_run_comment_lines(new, style)
    warnings = (
        _ineffectual_nolintnextline_warnings(new, style)
        + _non_ascii_comment_warnings(new, style)
        + _rule_run_comment_warnings(new, style)
        + _code_block_column_warnings(new, style)
        + (
            []
            if _is_test_cpp_path(path)
            else _colon_comment_warnings(new, style)
        )
    )
    punctuation_changed = _colon_semicolon_punctuation_changed(lines, new)
    if new == lines:
        return path, False, None, warnings, False
    if check_only:
        return path, True, None, warnings, punctuation_changed
    try:
        write_lines(path, new)
    except OSError as e:
        return path, False, str(e), warnings, punctuation_changed
    return path, True, None, warnings, punctuation_changed


def main(argv: Iterable[str] | None = None) -> int:
    p = ArgumentParser()
    p.add_argument("--max-col", type=int, default=_DEFAULT_MAX_COL)
    p.add_argument("--python-max-col", type=int, default=_PYTHON_MAX_COL)
    p.add_argument(
        "--check",
        action="store_true",
        help="read-only, exit 1 if any comment restyling would change a file",
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
    work = ((p, a.max_col, a.python_max_col, chflag) for p in paths)
    with ThreadPoolExecutor(max_workers=w) as ex:
        outs = list(ex.map(_restyle_one, work))
    for path, _ch, err, _warnings, _punctuation_changed in outs:
        if err:
            print(
                f"cat-restyle-comments: failed for `{path}`: {err}\n", file=sys.stderr
            )
            return 1
    for path, ch, _er, _warnings, _punctuation_changed in outs:
        if ch and a.check:
            print(f"would restyle: {path}", file=sys.stderr)
        if ch and not a.check:
            print(f"restyled: {path}", file=sys.stderr)
    if any(c and e is None and punc for _p, c, e, _w, punc in outs):
        print(
            "cat-restyle-comments: ':' or ';' punctuation changed. "
            "Manually review the diff.",
            file=sys.stderr,
        )
    n_would = sum(1 for _p, c, e, _w, _punc in outs if c and e is None)
    result = 1 if a.check and n_would else 0
    if a.check and n_would:
        print(
            f"cat-restyle-comments-check: {n_would} file(s) would be restyled. "
            "Run `cmake --build <build> --target cat-restyle-comments` to fix.",
            file=sys.stderr,
        )
    for path, _ch, _er, warnings, _punctuation_changed in outs:
        for line, message in warnings:
            print(
                f"{_warning_prefix(sys.stderr)} {path}:{line}: {message}",
                file=sys.stderr,
            )
    return result


if __name__ == "__main__":
    raise SystemExit(main())
