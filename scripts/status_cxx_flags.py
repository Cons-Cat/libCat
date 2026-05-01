#!/usr/bin/env python3

# This file is flagrantly "vibe-coded". It may not be up to the standards of most libCat
# code.

import json
import pathlib
import re
import shlex
import subprocess
import sys


BRBLACK = "\033[90m"
RESET = "\033[0m"
NA = f"{BRBLACK}n/a{RESET}"


MATH_FLAGS = (
    "associative-math",
    "fast-math",
    "finite-math-only",
    "honor-infinities",
    "honor-nans",
    "math-errno",
    "reciprocal-math",
    "rounding-math",
    "signed-zeros",
    "trapping-math",
    "unsafe-math-optimizations",
    "approx-func",
    "cx-limited-range",
    "protect-parens",
    "wrapv",
    "wrapv-pointer",
    "trapv",
    "strict-overflow",
    "strict-enums",
    "strict-bool",
)

MATH_FLAG_PREFIXES = (
    "denormal-fp-math=",
    "denormal-fp-math-f32=",
    "excess-precision=",
    "fp-contract=",
    "fp-eval-method=",
    "fp-model=",
)

EXCEPTION_FLAGS = (
    "exceptions",
    "assume-nothrow-exception-dtor",
    "async-exceptions",
    "asynchronous-unwind-tables",
    "cxx-exceptions",
    "dwarf-exceptions",
    "ignore-exceptions",
    "objc-arc-exceptions",
    "objc-exceptions",
    "seh-exceptions",
    "sjlj-exceptions",
    "wasm-exceptions",
    "emit-compact-unwind-non-canonical",
    "emit-dwarf-unwind",
    "unwind-tables",
)

EXCEPTION_FLAG_PREFIXES = (
    "winx64-eh-unwindv2=",
)

MODULE_FLAGS = (
    "builtin-module-map",
    "cxx-modules",
    "force-check-cxx20-modules-input-files",
    "implicit-module-maps",
    "implicit-modules",
    "module-file-deps",
    "module-header",
    "module-maps",
    "module-output",
    "module-private",
    "modules",
    "modules-decluse",
    "modules-disable-diagnostic-validation",
    "modules-driver",
    "modules-force-validate-user-headers",
    "modules-reduced-bmi",
    "modules-search-all",
    "modules-strict-decluse",
    "modules-validate-input-files-content",
    "modules-validate-once-per-build-session",
    "modules-validate-system-headers",
    "prebuilt-implicit-modules",
    "system-module",
)

MODULE_FLAG_PREFIXES = (
    "implicit-modules-lock-timeout=",
    "module-file=",
    "module-file-deps=",
    "module-header=",
    "module-map-file=",
    "module-name=",
    "module-output=",
    "modulemap-allow-subdirectory-search",
    "modules-cache-path=",
    "modules-embed-all-files",
    "modules-ignore-macro=",
    "modules-prune-after=",
    "modules-prune-interval=",
    "prebuilt-module-path=",
)

LANGUAGE_FLAGS = (
    "clangir",
    "defer-ts",
    "experimental-new-constant-interpreter",
)

LANGUAGE_FLAG_PREFIXES = ()

LANGUAGE_DIRECT_FLAGS = (
    "-clangir-disable-passes",
    "-clangir-disable-verifier",
    "-clangir-enable-idiom-recognizer",
    "-emit-cir",
)

DIAGNOSTIC_FLAGS = (
    "ansi-escape-codes",
    "caret-diagnostics",
    "color-diagnostics",
    "crash-diagnostics",
    "diagnostics-absolute-paths",
    "diagnostics-color",
    "diagnostics-fixit-info",
    "diagnostics-parseable-fixits",
    "diagnostics-print-source-range-info",
    "diagnostics-show-hotness",
    "diagnostics-show-inlining-chain",
    "diagnostics-show-line-numbers",
    "diagnostics-show-note-include-stack",
    "diagnostics-show-option",
    "diagnostics-show-template-tree",
    "diagnostics-urls",
    "show-column",
    "show-source-location",
    "spell-checking",
)

DIAGNOSTIC_FLAG_PREFIXES = (
    "caret-diagnostics-max-lines=",
    "constexpr-backtrace-limit=",
    "crash-diagnostics=",
    "crash-diagnostics-dir=",
    "diagnostics-color=",
    "diagnostics-format=",
    "diagnostics-hotness-threshold=",
    "diagnostics-misexpect-tolerance=",
    "diagnostics-show-category=",
    "error-limit=",
    "macro-backtrace-limit=",
    "message-length=",
    "spell-checking-limit=",
    "template-backtrace-limit=",
)

PROFILING_FLAGS = (
    "coverage-mapping",
    "coverage-mcdc",
    "create-profile",
    "cs-profile-generate",
    "debug-info-for-profiling",
    "instrument-function-entry-bare",
    "instrument-functions",
    "instrument-functions-after-inlining",
    "memory-profile",
    "profile-arcs",
    "profile-continuous",
    "profile-generate",
    "profile-generate-cold-function-coverage",
    "profile-instr-generate",
    "profile-instr-use",
    "profile-sample-accurate",
    "profile-use",
    "pseudo-probe-for-profiling",
    "sample-profile-use-profi",
    "temporal-profile",
    "test-coverage",
    "time-trace",
    "xray-always-emit-customevents",
    "xray-always-emit-typedevents",
    "xray-function-index",
    "xray-ignore-loops",
    "xray-instrument",
    "xray-link-deps",
    "xray-shared",
)

PROFILING_FLAG_PREFIXES = (
    "coverage-compilation-dir=",
    "coverage-prefix-map=",
    "cs-profile-generate=",
    "memory-profile=",
    "memory-profile-use=",
    "profile-dir=",
    "profile-exclude-files=",
    "profile-filter-files=",
    "profile-function-groups=",
    "profile-generate=",
    "profile-generate-cold-function-coverage=",
    "profile-instr-generate=",
    "profile-instr-use=",
    "profile-list=",
    "profile-remapping-file=",
    "profile-sample-use=",
    "profile-selected-function-group=",
    "profile-update=",
    "profile-use=",
    "time-trace-granularity=",
    "xray-always-instrument=",
    "xray-attr-list=",
    "xray-function-groups=",
    "xray-instruction-threshold=",
    "xray-instrumentation-bundle=",
    "xray-modes=",
    "xray-never-instrument=",
    "xray-selected-function-group=",
)

HEADER_PAIRED_FLAGS = (
    "-F",
    "-idirafter",
    "-iframework",
    "-iframeworkwithsysroot",
    "-imacros",
    "-include",
    "-iprefix",
    "-iquote",
    "-isysroot",
    "-isystem-after",
    "-isystem",
    "-iwithsysroot",
    "-iwithprefix",
    "-iwithprefixbefore",
    "-resource-dir",
    "--sysroot",
    "--system-header-prefix",
    "--no-system-header-prefix",
    "-dependency-dot",
    "-dependency-file",
    "-module-dependency-dir",
)

HEADER_PREFIX_FLAGS = (
    "-F",
    "-idirafter",
    "-iframework",
    "-iframeworkwithsysroot",
    "-imacros",
    "-iprefix",
    "-iquote",
    "-isysroot",
    "-isystem-after",
    "-isystem",
    "-iwithsysroot",
    "-iwithprefix",
    "-iwithprefixbefore",
    "-resource-dir",
    "-stdlib++-isystem",
    "--sysroot",
    "--system-header-prefix",
    "--no-system-header-prefix",
    "-dependency-dot",
    "-dependency-file",
    "-module-dependency-dir",
)

HEADER_FLAGS = (
    "builtin-headers-in-system-modules",
    "builtininc",
    "canonical-system-headers",
    "keep-system-includes",
    "modules-force-validate-user-headers",
    "modules-validate-system-headers",
    "modules-validate-textual-header-includes",
    "retain-comments-from-system-headers",
    "rewrite-includes",
    "show-skipped-includes",
)

HEADER_FLAG_PREFIXES = (
    "module-file-deps=",
)

OTHER_KINDS = (
    "language",
    "modules",
    "define",
    "headers",
    "diagnostics",
    "feat",
    "profiling",
    "isa",
    "exceptions",
    "rtti",
    "visibility",
    "lto",
    "math",
    "arch",
    "opt",
    "san",
    "lifetime",
    "dbg",
    "warn",
    "nowarn",
)

OTHER_SKIP_FLAGS = (
    "-c",
    "-o",
    "-x",
    "-Xclang",
    "-include-pch",
)

OTHER_SKIP_PREFIXES = (
    "-I",
)

OTHER_SKIP_NEXT_FLAGS = (
    "-o",
    "-x",
)


def f_flag_name(flag: str) -> str:
    if flag.startswith("-fno-"):
        return flag.removeprefix("-fno-")
    if flag.startswith("-f"):
        return flag.removeprefix("-f")
    return ""


def flag_matches(kind: str, flag: str) -> bool:
    if kind == "warn":
        return (
            flag.startswith("-W")
            and not flag.startswith("-Wno-")
            and not flag_matches("lifetime", flag)
        )
    if kind == "nowarn":
        return flag == "-w" or flag.startswith("-Wno-")
    if kind == "feat":
        return (
            flag.startswith("-f")
            and not flag_matches("dbg", flag)
            and not flag_matches("san", flag)
            and not flag_matches("diagnostics", flag)
            and not flag_matches("language", flag)
            and not flag_matches("modules", flag)
            and not flag_matches("profiling", flag)
            and not flag_matches("lto", flag)
            and not flag_matches("lifetime", flag)
            and not flag_matches("math", flag)
            and not flag_matches("exceptions", flag)
            and not flag_matches("rtti", flag)
            and not flag_matches("visibility", flag)
        )
    if kind in ("std", "language"):
        name = f_flag_name(flag)
        return (
            flag.startswith("-std=")
            or flag in LANGUAGE_DIRECT_FLAGS
            or flag
            in (
                "-nostdlib",
                "-nostdlib++",
                "-stdlib",
            )
            or flag.startswith(("--stdlib=", "-stdlib="))
            or name in LANGUAGE_FLAGS
            or name.startswith(LANGUAGE_FLAG_PREFIXES)
        )
    if kind == "modules":
        name = f_flag_name(flag)
        return (
            name in MODULE_FLAGS or name.startswith(MODULE_FLAG_PREFIXES)
        ) and not flag_matches("headers", flag)
    if kind == "define":
        return flag.startswith("-D")
    if kind == "headers":
        name = f_flag_name(flag)
        return (
            flag
            in (
                "-H",
                "--trace-includes",
                "-M",
                "-MD",
                "-MDD",
                "-MF",
                "-MG",
                "-MJ",
                "-MM",
                "-MMD",
                "-MP",
                "-MQ",
                "-MT",
                "-MV",
                "-P",
                "-print-include-stacks",
                "-Eonly",
                "-nostdinc",
                "-nostdinc++",
                "-nostdlibinc",
                "-nobuiltininc",
                "-ibuiltininc",
                "--cuda-include-ptx",
                "--no-cuda-include-ptx",
            )
            or flag in HEADER_PAIRED_FLAGS
            or flag.startswith(HEADER_PREFIX_FLAGS)
            or name in HEADER_FLAGS
            or name.startswith(HEADER_FLAG_PREFIXES)
        )
    if kind == "diagnostics":
        name = f_flag_name(flag)
        return (
            flag
            in (
                "-print-diagnostic-categories",
                "--print-diagnostic-categories",
                "-print-diagnostic-options",
                "--print-diagnostic-options",
            )
            or flag.startswith(("-gen-reproducer=", "-serialize-diagnostics"))
            or name in DIAGNOSTIC_FLAGS
            or name.startswith(DIAGNOSTIC_FLAG_PREFIXES)
        )
    if kind == "profiling":
        name = f_flag_name(flag)
        return (
            flag in ("-p", "-pg", "-noprofilelib")
            or flag.startswith(("-fprofile-", "-fno-profile-"))
            or name in PROFILING_FLAGS
            or name.startswith(PROFILING_FLAG_PREFIXES)
            or flag.startswith(("-fxray-", "-fno-xray-", "-fcs-profile-"))
            or flag.startswith(("-fcoverage-", "-fno-coverage-"))
            or flag.startswith(("-finstrument-", "-fno-instrument-"))
            or flag.startswith(("-ftime-trace", "-fno-time-trace"))
            or flag.startswith(("-m", "-mno-")) and "profiling" in flag
        )
    if kind == "isa":
        return flag.startswith("-march=") or flag in ("-maarch64",)
    if kind == "exceptions":
        name = f_flag_name(flag)
        return (
            name in EXCEPTION_FLAGS
            or name.startswith(EXCEPTION_FLAG_PREFIXES)
            or flag.startswith(("-unwindlib=", "--unwindlib="))
        )
    if kind == "rtti":
        return flag in ("-frtti", "-fno-rtti")
    if kind == "visibility":
        return flag.startswith("-fvisibility")
    if kind == "lto":
        return flag in (
            "-fwhole-program-vtables",
            "-ffat-lto-objects",
            "-ffunction-sections",
            "-fdata-sections",
        ) or flag.startswith("-flto")
    if kind == "math":
        name = f_flag_name(flag)
        return name in MATH_FLAGS or name.startswith(MATH_FLAG_PREFIXES)
    if kind == "arch":
        return (
            flag.startswith("-m")
            and flag != "-mllvm"
            and not flag_matches("isa", flag)
            and not flag_matches("profiling", flag)
        )
    if kind == "opt":
        return flag.startswith("-O")
    if kind == "san":
        return (
            flag.startswith(("-fsanitize", "-fno-sanitize"))
            or flag.startswith(
                (
                    "-fexperimental-sanitize-metadata",
                    "-fno-experimental-sanitize-metadata",
                    "-asan-",
                )
            )
        )
    if kind == "lifetime":
        return (
            flag in ("-Wdangling", "-Wdangling-gsl", "-fexperimental-bounds-safety")
            or flag.startswith("-Wlifetime")
            or flag.startswith("-lifetime")
            or flag.startswith("-fanalyzer")
        )
    if kind == "dbg":
        return (
            (flag.startswith("-g") or flag.startswith("-fdebug") or flag in ("-fjmc", "-fno-jmc"))
            and not flag_matches("profiling", flag)
            or flag.startswith("-fno-eliminate-unused-debug")
        )
    if kind == "other":
        return (
            flag.startswith("-")
            and flag not in OTHER_SKIP_FLAGS
            and not flag.startswith(OTHER_SKIP_PREFIXES)
            and not any(flag_matches(other_kind, flag) for other_kind in OTHER_KINDS)
        )
    raise SystemExit(f"unknown flag group: {kind}")


def command_flags(entry: dict[str, object]) -> list[str]:
    arguments = entry.get("arguments")
    if isinstance(arguments, list):
        return [str(argument) for argument in arguments][1:]
    command = entry.get("command")
    if isinstance(command, str):
        return shlex.split(command)[1:]
    return []


def cache_value(build_dir: pathlib.Path, name: str) -> str:
    cache = build_dir / "CMakeCache.txt"
    if not cache.exists():
        return ""
    prefix = f"{name}:"
    for line in cache.read_text(encoding="utf-8").splitlines():
        if line.startswith(prefix) and "=" in line:
            return line.split("=", 1)[1]
    return ""


def select_entry(entries: list[dict[str, object]], config: str) -> dict[str, object] | None:
    candidates = []
    for entry in entries:
        output = str(entry.get("output", ""))
        file = str(entry.get("file", ""))
        if "CMakeFiles/cat-impl.dir" not in output:
            continue
        if "cmake_pch" in output or "cmake_pch" in file:
            continue
        if config and f"/{config}/" not in output:
            continue
        candidates.append(entry)
    return candidates[0] if candidates else None


def parse_help_options(help_text: str) -> list[tuple[str, str]]:
    options = []
    current = None
    for line in help_text.splitlines():
        match = re.match(r"^\s{2}(-[^ ].*?)(?:\s{2,}(.+))?$", line)
        if match:
            option_text = match.group(1).strip()
            description = (match.group(2) or "").strip()
            for option in option_text.split(","):
                option = option.strip()
                if option:
                    options.append((option, description))
            current = len(options) - 1 if options else None
            continue
        if current is not None and line.startswith(" " * 26):
            continuation = line.strip()
            if continuation:
                option, description = options[current]
                options[current] = (option, f"{description} {continuation}".strip())
    return options


def option_matches_flag(option: str, flag: str) -> bool:
    option_head = option.split()[0]
    if option_head.startswith("-W<") and flag.startswith(("-Wa,", "-Wl,", "-Wp,")):
        return False
    if flag.startswith(option_head + "="):
        return True
    if option_head == flag:
        return True
    if "<" in option_head:
        prefix = option_head.split("<", 1)[0]
        return bool(prefix) and flag.startswith(prefix)
    if option_head.endswith("="):
        return flag.startswith(option_head)
    return False


def describe_flag(flag: str, options: list[tuple[str, str]]) -> str:
    for option, description in options:
        if option_matches_flag(option, flag):
            return description
    if flag.startswith("-fno-"):
        positive_flag = "-f" + flag.removeprefix("-fno-")
        for option, description in options:
            if option_matches_flag(option, positive_flag):
                return f"negative form of {positive_flag}: {description}"
    if flag.startswith("-W") and not flag.startswith(("-Wa,", "-Wl,", "-Wp,")):
        for option, description in options:
            if option.startswith("-W<"):
                return description
    return NA


def short_flag(flag: str) -> str:
    if flag.startswith("-std="):
        return flag.removeprefix("-std=")
    if flag.startswith("-D"):
        return flag.removeprefix("-D")
    if flag.startswith("--print-diagnostic-"):
        return flag.removeprefix("--")
    if flag.startswith("-print-diagnostic-"):
        return flag.removeprefix("-")
    if flag.startswith("-serialize-diagnostics="):
        return flag.removeprefix("-")
    if flag.startswith("-gen-reproducer="):
        return flag.removeprefix("-")
    if flag.startswith("--stdlib="):
        return flag.removeprefix("--")
    if flag.startswith("-stdlib="):
        return flag.removeprefix("-")
    if flag.startswith("-stdlib++-isystem="):
        return "stdlib++-isystem=" + display_path(
            flag.removeprefix("-stdlib++-isystem=")
        )
    if flag.startswith("-stdlib++-isystem"):
        return "stdlib++-isystem=" + display_path(
            flag.removeprefix("-stdlib++-isystem")
        )
    for option in HEADER_PAIRED_FLAGS:
        if flag.startswith(option + "="):
            return option.lstrip("-") + "=" + display_path(
                flag.removeprefix(option + "=")
            )
    for option in HEADER_PREFIX_FLAGS:
        if flag.startswith(option) and flag != option:
            return option.lstrip("-") + "=" + display_path(flag.removeprefix(option))
    if flag.startswith("--unwindlib="):
        return flag.removeprefix("--")
    if flag.startswith("-unwindlib="):
        return flag.removeprefix("-")
    if flag.startswith("-fexperimental-sanitize-metadata-ignorelist="):
        return "metadata-ignorelist=" + flag.removeprefix(
            "-fexperimental-sanitize-metadata-ignorelist="
        )
    if flag.startswith("-fno-experimental-sanitize-metadata="):
        return "no-metadata=" + flag.removeprefix("-fno-experimental-sanitize-metadata=")
    if flag.startswith("-fexperimental-sanitize-metadata="):
        return "metadata=" + flag.removeprefix("-fexperimental-sanitize-metadata=")
    if flag.startswith("-asan-"):
        return flag.removeprefix("-")
    if flag.startswith("-fno-sanitize-recover="):
        return "no-recover=" + flag.removeprefix("-fno-sanitize-recover=")
    if flag.startswith("-fsanitize-recover="):
        return "recover=" + flag.removeprefix("-fsanitize-recover=")
    if flag.startswith("-fsanitize-undefined-ignore-overflow-pattern="):
        return "ignore-overflow-pattern=" + flag.removeprefix(
            "-fsanitize-undefined-ignore-overflow-pattern="
        )
    if flag.startswith("-fsanitize="):
        return flag.removeprefix("-fsanitize=")
    if flag.startswith(("-Wa,", "-Wl,", "-Wp,")):
        return flag.removeprefix("-")
    if flag.startswith("-Wno-"):
        return flag.removeprefix("-Wno-")
    if flag.startswith("-Wno"):
        return flag.removeprefix("-Wno")
    if flag.startswith("-W"):
        return flag.removeprefix("-W")
    if flag.startswith("-f"):
        return flag.removeprefix("-f")
    if flag.startswith("-m"):
        return flag.removeprefix("-m")
    if flag.startswith("-"):
        return flag.removeprefix("-")
    return flag


def short_flags(flags: list[str]) -> str:
    return " ".join(short_flag(flag) for flag in flags)


def display_path(path: str) -> str:
    candidate = pathlib.Path(path)
    if not candidate.is_absolute():
        return path

    try:
        return str(candidate.resolve(strict=False).relative_to(pathlib.Path.cwd().resolve()))
    except ValueError:
        return path


def collect_flags(kind: str, all_flags: list[str]) -> list[str]:
    if kind == "other":
        flags = []
        index = 0
        while index < len(all_flags):
            flag = all_flags[index]
            if flag in OTHER_SKIP_NEXT_FLAGS and index + 1 < len(all_flags):
                index += 2
                continue
            if flag == "-Xclang":
                if (
                    index + 3 < len(all_flags)
                    and all_flags[index + 1] == "-include"
                    and all_flags[index + 2] == "-Xclang"
                    and "cmake_pch" in all_flags[index + 3]
                ):
                    index += 4
                    continue
                index += 1
                continue
            if flag_matches(kind, flag) and flag not in flags:
                flags.append(flag)
            index += 1
        return flags

    if kind != "headers":
        flags = []
        for flag in all_flags:
            if flag_matches(kind, flag) and flag not in flags:
                flags.append(flag)
        return flags

    flags = []
    index = 0
    while index < len(all_flags):
        flag = all_flags[index]
        if flag in HEADER_PAIRED_FLAGS and index + 1 < len(all_flags):
            if all_flags[index + 1] == "-Xclang" and index + 2 < len(all_flags):
                combined = f"{flag}={all_flags[index + 2]}"
                index += 3
            else:
                combined = f"{flag}={all_flags[index + 1]}"
                index += 2
            if flag == "-include" and "cmake_pch" in combined:
                continue
            if combined not in flags:
                flags.append(combined)
            continue
        if flag_matches(kind, flag) and flag not in flags:
            flags.append(flag)
        index += 1
    return flags


def compiler_target(build_dir: pathlib.Path) -> str:
    compiler = cache_value(build_dir, "CMAKE_CXX_COMPILER") or "clang++"
    result = subprocess.run(
        [compiler, "--version"],
        check=False,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
    )
    if result.returncode != 0:
        return NA
    for line in result.stdout.splitlines():
        if line.startswith("Target: "):
            return line.removeprefix("Target: ").strip() or NA
    return NA


def isa_name(isa: str) -> str:
    names = {
        "aarch64": "AArch64",
        "arm64": "AArch64",
    }
    return names.get(isa, isa)


def brblack_parens(text: str) -> str:
    return f"{BRBLACK}({RESET}{text}{BRBLACK}){RESET}"


def isa_value(build_dir: pathlib.Path, flags: list[str]) -> str:
    target = compiler_target(build_dir)
    target_suffix = "" if target == NA else f" {brblack_parens(target)}"
    for flag in flags:
        if flag == "-maarch64":
            return "AArch64"
        if flag.startswith("-march="):
            march = flag.removeprefix("-march=")
            if march == "native":
                return f"native{target_suffix}"
            return isa_name(march)
    return f"default {brblack_parens(target)}" if target != NA else NA


def default_cpp_standard(build_dir: pathlib.Path) -> str:
    compiler = cache_value(build_dir, "CMAKE_CXX_COMPILER") or "clang++"
    result = subprocess.run(
        [compiler, "-x", "c++", "-dM", "-E", "-"],
        check=False,
        input="",
        stdout=subprocess.PIPE,
        stderr=subprocess.DEVNULL,
        text=True,
    )
    if result.returncode != 0:
        return NA

    standards = {
        "199711L": "c++98",
        "201103L": "c++11",
        "201402L": "c++14",
        "201703L": "c++17",
        "202002L": "c++20",
        "202302L": "c++23",
        "202400L": "c++26",
    }
    value = NA
    strict = False
    for line in result.stdout.splitlines():
        if line == "#define __STRICT_ANSI__ 1":
            strict = True
        if line.startswith("#define __cplusplus "):
            value = standards.get(line.rsplit(maxsplit=1)[-1], line.rsplit(maxsplit=1)[-1])
    if value == NA:
        return NA
    if not strict and value.startswith("c++"):
        value = "gnu++" + value.removeprefix("c++")
    return value


def default_value(kind: str, build_dir: pathlib.Path) -> str:
    if kind in ("std", "language"):
        value = default_cpp_standard(build_dir)
        return f"default {brblack_parens(value)}" if value != NA else NA
    if kind in ("exceptions", "rtti"):
        return f"default {brblack_parens('enabled')}"
    return ""


def clang_help_options(build_dir: pathlib.Path) -> list[tuple[str, str]]:
    compiler = cache_value(build_dir, "CMAKE_CXX_COMPILER") or "clang++"
    result = subprocess.run(
        [compiler, "--help"],
        check=False,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
    )
    if result.returncode != 0:
        return []
    return parse_help_options(result.stdout)


def main() -> int:
    if len(sys.argv) not in (5, 6):
        raise SystemExit(
            "usage: status_cxx_flags.py <build-dir> <config> <kind> <label> [--verbose]"
        )

    build_dir = pathlib.Path(sys.argv[1])
    config = sys.argv[2]
    kind = sys.argv[3]
    label = sys.argv[4]
    verbose = len(sys.argv) == 6 and sys.argv[5] == "--verbose"
    database_path = build_dir / "compile_commands.json"

    value = ""
    flags = []
    if database_path.exists():
        entries = json.loads(database_path.read_text(encoding="utf-8"))
        entry = select_entry(entries, config)
        if entry is not None:
            flags = collect_flags(kind, command_flags(entry))
            if flags:
                value = short_flags(flags)
            else:
                value = default_value(kind, build_dir)

    if kind == "isa":
        print(f"\033[1m{label}: \033[0m{isa_value(build_dir, flags)}")
        return 0

    if verbose:
        print(f"\033[1m{label}: \033[0m")
    elif value:
        print(f"\033[1m{label}: \033[0m{value}")
    if verbose and flags:
        options = clang_help_options(build_dir)
        for flag in flags:
            if kind == "define":
                print(f"  {flag}")
            else:
                print(f"  {flag}: {describe_flag(flag, options)}")
    elif verbose and value:
        print(f"  {value}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
