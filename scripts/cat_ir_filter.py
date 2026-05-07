# Lossy textual filters used by `cat-ir` custom commands. Each mode is a
# small in-place transform on a single file.
#
# Modes:
#   strip-cpp-markers <src> <dst>          drop `# <lineno> <file> ...` cpp
#                                          line directives left over by clang
#                                          `-E` / `-save-temps=obj`.
#   ii <src> <dst> <name>                  best-effort slice of a `.ii` to
#                                          functions whose definition line
#                                          mentions `<name>` as a whole
#                                          token. `<name>` is treated as a
#                                          literal source-level identifier
#                                          (regex metacharacters get escaped
#                                          before wrapping in `\b...\b`),
#                                          matching the `cat-ir` rule that
#                                          `fn=` is a literal name, not a
#                                          regex.
from __future__ import annotations

import re
import sys


# Drop CPP line markers (`# <line> <file> ...`) so a sliced or formatted `.ii`
# is readable text rather than a litany of source-position breadcrumbs. Run on
# every `.ii` regardless of `fn=` (clang-format doesn't strip these on its
# own).
_CPP_MARKER_RX = re.compile(r"(^|\n)# [0-9]+[^\n]*")


def _strip_cpp_markers(text: str) -> str:
    return _CPP_MARKER_RX.sub(r"\1", text)


# `.ii` slice. `name` is a literal source-level identifier (typically
# namespace-qualified, like `cat::pow`); we wrap it in `\b...\b` so a bare
# `pow` does not splice into `power`. The function-definition heuristic is
# "header line contains the name and ends with `{`"; multi-line signatures
# (initializer lists, trailing `noexcept(...)`, parameter packs on their
# own line) are stitched in by walking back over `,` / `:` / `(` / `>` /
# `&` / `*` / `]]` / `))` continuation lines.
def _slice_ii(text: str, name: str) -> str:
    rx = re.compile(rf"\b{re.escape(name)}\b")
    out: list[str] = []
    lines = text.split("\n")
    n = len(lines)
    i = 0
    while i < n:
        line = lines[i]
        if rx.search(line) and line.rstrip().endswith("{"):
            block_start = i
            j = i - 1
            while j >= 0:
                tail = lines[j].rstrip()
                if tail.endswith((",", ":", "(", ">", "&", "*", "]]", "))")):
                    block_start = j
                    j -= 1
                    continue
                break
            block = list(lines[block_start:i + 1])
            depth = sum(s.count("{") - s.count("}") for s in block)
            i += 1
            while i < n and depth > 0:
                block.append(lines[i])
                depth += lines[i].count("{") - lines[i].count("}")
                i += 1
            out.append("\n".join(block))
        else:
            i += 1
    if not out:
        return ""
    return "\n\n".join(out) + "\n"


def main() -> int:
    if len(sys.argv) < 4:
        sys.stderr.write(
            f"usage: {sys.argv[0]} {{strip-cpp-markers|ii}} "
            "<src> <dst> [<name>]\n"
        )
        return 2
    kind = sys.argv[1]
    src = sys.argv[2]
    dst = sys.argv[3]
    name = sys.argv[4] if len(sys.argv) > 4 else ""
    with open(src, "r", encoding="utf-8", errors="replace") as f:
        text = f.read()
    if kind == "strip-cpp-markers":
        text = _strip_cpp_markers(text)
    elif kind == "ii":
        text = _slice_ii(text, name)
    else:
        sys.stderr.write(f"cat_ir_filter: unknown kind '{kind}'\n")
        return 2
    with open(dst, "w", encoding="utf-8") as f:
        f.write(text)
    return 0


if __name__ == "__main__":
    sys.exit(main())
