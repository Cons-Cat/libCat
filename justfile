# This justfile is largely "vibe-coded". It may not be up to the standards of most libCat code.
set unstable
set positional-arguments

default:
    @just --list

# Cache the last release mode.
saved_mode := `if test -f .cache/cat-build-mode; then cat .cache/cat-build-mode; else printf release; fi`
last_mode := env_var_or_default("CAT_BUILD_MODE", saved_mode)

# The `mise.toml` defaults `CMAKE_GENERATOR` to `Ninja Multi-Config`.
generator := env_var_or_default("CMAKE_GENERATOR", "")
multi_config := if generator == "Ninja Multi-Config" { "true" } else { "false" }
mode(arg) := arg

# Get CMake target/config names for a release mode.
# TODO: `just` logical operators are WIP. When they're better, we can simplify this.
# https://github.com/casey/just/issues/3258
matches_mode(arg, release_mode) := if mode(arg) == release_mode { "true" } else { matches_all(arg) }
matches_all(arg) := if mode(arg) == "all" { "true" } else { "false" }
debug_config(arg) := if matches_mode(arg, "debug") == "true" { "Debug" } else { "" }
release_config(arg) := if matches_mode(arg, "release") == "true" { "Release" } else { "" }
relwithdebinfo_config(arg) := if relwithdebinfo_matches(arg) == "true" { "RelWithDebInfo" } else { "" }
relwithdebinfo_matches(arg) := matches_mode(arg, "relwithdebinfo")
cmake_config(arg) := debug_config(arg) + release_config(arg) + relwithdebinfo_config(arg)

# Get the build directory for a release mode.
build_dir(arg) := if cmake_config(arg) == "" { mode(arg) } else { cmake_build_dir(arg) }
cmake_build_dir(arg) := if multi_config == "true" { "build" } else { "build-" + mode(arg) }

# Get either a `CMAKE_BUILD_TYPE` or `--config` for this release mode.
# Handling, "Ninja", "Ninja Multi-config", and "Unix Makefiles" makes this fairly complex.
configure_config(arg) := if cmake_config(arg) == "" { "" } else { cmake_configure_config(arg) }
cmake_configure_config(arg) := if multi_config == "true" { "" } else { cmake_build_type(arg) }
cmake_build_type(arg) := "-DCMAKE_BUILD_TYPE=" + cmake_config(arg)
build_config(arg) := if cmake_config(arg) == "" { "" } else { cmake_build_config(arg) }
cmake_build_config(arg) := if multi_config == "true" { "--config " + cmake_config(arg) } else { "" }
test_config(arg) := if cmake_config(arg) == "" { "" } else { cmake_test_config(arg) }
cmake_test_config(arg) := if multi_config == "true" { "-C " + cmake_config(arg) } else { "" }
repl_command(arg) := if mode(arg) == "" { repl_path(arg) } else { configured_repl_command(arg) }
repl_path(arg) := build_dir(arg) + "/clang-repl-libcat"
configured_repl_command(arg) := if multi_config == "true" { multi_repl_command(arg) } else { repl_path(arg) }
multi_repl_command(arg) := "CAT_REPL_CONFIG=" + cmake_config(arg) + " ./build/clang-repl-libcat"

# Implement `-v`.
cmake_log(verbose) := if verbose == "-v" { "--log-level=VERBOSE" } else { "" }
build_verbose(verbose) := if verbose == "-v" { "--verbose" } else { "" }
status_verbose(verbose) := if verbose == "-v" { "--verbose" } else { "" }
ctest_verbose(verbose) := if verbose == "-v" { "--verbose" } else { "" }

# Set sanitizer flags.
sanitizer(flag) := if flag == "san" { "-DCAT_USE_SANITIZERS=ON" } else { nosan(flag) }
nosan(flag) := if flag == "nosan" { "-DCAT_USE_SANITIZERS=OFF" } else { "" }
sanitizer_status(flag) := if flag == "san" { "ON" } else if flag == "nosan" { "OFF" } else { "" }
cached_sanitizer_recipe := "_print-cached-sanitizer-status"
requested_sanitizer_recipe := "_print-requested-sanitizer-status"
sanitizer_status_recipe(flag) := if sanitizer_status(flag) == "" { cached_sanitizer_recipe } else { requested_sanitizer_recipe }
# Changing sanitizer settings requires regenerating CMake.
can_skip_configure(flag) := if sanitizer(flag) == "" { "true" } else { "false" }

# Assemble a `cmake --build` command.
build *args:
    @san=""; verbose=""; no_warnings="false"; cxx_flags=""; modes=""; unset CAT_JUST_BUILD_TOOL_TRAILER_B64; set -- "$@"; \
      while [ $# -gt 0 ]; do \
        case "$1" in \
          --) shift; \
            if [ $# -gt 0 ]; then \
              CAT_JUST_BUILD_TOOL_TRAILER_B64="$(printf '%s\0' "$@" | base64 -w0)"; \
              export CAT_JUST_BUILD_TOOL_TRAILER_B64; \
            fi; \
            break ;; \
          san|nosan) san="$1"; shift ;; \
          -v) verbose="$1"; shift ;; \
          -w) no_warnings="true"; shift ;; \
          -*) cxx_flags="${cxx_flags:+$cxx_flags }$1"; shift ;; \
          debug|release|relwithdebinfo|build|all) modes="${modes:+$modes }$1"; shift ;; \
          *) printf '%s\n' "just build: unknown flag '$1'!" >&2; exit 1 ;; \
        esac; \
      done; \
      if [ -z "$modes" ]; then modes="{{ last_mode }}"; fi; \
      for mode in $modes; do \
        just _build-mode "$mode" "$san" "$verbose" "$no_warnings" "$cxx_flags"; \
      done; \
      unset CAT_JUST_BUILD_TOOL_TRAILER_B64

# Raw Ninja with `-C`/`-f` into the build directory. After an optional mode, a word that matches a
# `ninja -t` subtool (see `ninja -t list`) is shorthand for `-t <word>`. Further arguments pass
# through unchanged.
ninja *args:
    @saved_mode=release; \
      if [ -f .cache/cat-build-mode ]; then read -r saved_mode < .cache/cat-build-mode || :; fi; \
      mode="${CAT_BUILD_MODE:-$saved_mode}"; \
      multi=false; \
      [ "${CMAKE_GENERATOR:-}" = "Ninja Multi-Config" ] && multi=true; \
      set -- "$@"; \
      case "${1-}" in \
        debug|release|relwithdebinfo|build) mode="$1"; shift ;; \
      esac; \
      cmake_cfg=; \
      case "$mode" in \
        debug) cmake_cfg=Debug ;; \
        release) cmake_cfg=Release ;; \
        relwithdebinfo) cmake_cfg=RelWithDebInfo ;; \
      esac; \
      dir=; mf_flags=; \
      if [ -n "$cmake_cfg" ]; then \
        if [ "$multi" = true ]; then \
          dir=build; mf_flags="-f build-${cmake_cfg}.ninja"; \
        else \
          dir="build-$mode"; \
        fi; \
      else \
        dir="$mode"; \
      fi; \
      tool_flags=; \
      case "${1-}" in \
        browse|clean|cleandead|commands|compdb|deps|graph|inputs|list|missingdeps|query|recompact|restat|rules|targets) \
          tool_flags="-t $1"; shift ;; \
      esac; \
      exec ninja -C "$dir" $mf_flags $tool_flags "$@"

clean mode=last_mode verbose="":
    @just _clean-mode {{ mode }} {{ verbose }}

format mode=last_mode verbose="":
    @just cmake_target cat-format {{ mode }} {{ verbose }}

format-check mode=last_mode verbose="":
    @just cmake_target cat-format-check {{ mode }} {{ verbose }}

restyle-comments mode=last_mode verbose="":
    @just cmake_target cat-restyle-comments {{ mode }} {{ verbose }}

restyle-comments-check mode=last_mode verbose="":
    @just cmake_target cat-restyle-comments-check {{ mode }} {{ verbose }}

tidy mode=last_mode verbose="":
    @just cmake_target cat-tidy {{ mode }} {{ verbose }}

tidy-check mode=last_mode verbose="":
    @just cmake_target cat-tidy-check {{ mode }} {{ verbose }}

opt-report mode=last_mode verbose="":
    @just cmake_target cat-opt-report {{ mode }} {{ verbose }}

# Per-TU IR fan-out. The grammar:
#
#   just ir <kind>+ <selector>+ [mode] [san|nosan] [fmt|no-fmt] [-v] \
#           [pass=<pipeline>] [fn=<regex>] [-flag ...] [-- <objdump flags>]
#
# Kinds (at least one required): ii s intel att bc ll.
#   `intel` / `att` are `s` specialised to a particular x86 syntax (Intel by
#   default).
# Selectors (at least one required) translate directly to ninja targets:
#   foo.cpp / foo               -> per-TU `<basename>-<kind>` target
#   src / tests / examples      -> per-domain `<dom>-<kind>` meta target
#   full                        -> `full-<kind>` (all domains).
# Other:
#   pass=<pipeline>             swaps `<basename>-ll` to `opt -passes=<...>`.
#   fn=<name>                   narrows every output to functions whose
#                               demangled name is `<name>` followed by an
#                               overload `(...)`, template `<...>`, or LTO
#                               clone (`.cold`, `.0`, ...) suffix. Treated
#                               as a literal symbol, not a regex; assume the
#                               user namespace-qualifies it (e.g. `cat::pow`).
#   -<flag> ...                 forwarded to the underlying CMake compile
#                               (same as `just build`).
#   -- <flags...>               forwarded verbatim to `llvm-objdump`. Only
#                               valid when `s` (or `intel`/`att`) is the
#                               *sole* requested kind.
#   fmt / no-fmt                run / skip clang-format on `.ii` (default
#                               `fmt`).
ir *args:
    @set -e; \
      mode=""; san=""; verbose=""; \
      kinds=""; selectors=""; cxx_flags=""; \
      fmt=""; pass=""; fn=""; syntax="intel"; \
      saw_trailer=false; trailer=""; \
      add_kind() { case " $kinds " in *" $1 "*) ;; *) kinds="${kinds:+$kinds }$1" ;; esac; }; \
      add_sel()  { case " $selectors " in *" $1 "*) ;; *) selectors="${selectors:+$selectors }$1" ;; esac; }; \
      while [ $# -gt 0 ]; do \
        case "$1" in \
          --) shift; saw_trailer=true; \
            for arg in "$@"; do \
              if [ -z "$trailer" ]; then trailer="$arg"; \
              else trailer="${trailer};${arg}"; fi; \
            done; \
            break ;; \
          san|nosan) san="$1"; shift ;; \
          fmt|no-fmt) fmt="$1"; shift ;; \
          ii|bc|ll|s) add_kind "$1"; shift ;; \
          intel) add_kind s; syntax="intel"; shift ;; \
          att)   add_kind s; syntax="att";   shift ;; \
          pass=*) pass="${1#pass=}"; add_kind ll; shift ;; \
          fn=*)   fn="${1#fn=}"; shift ;; \
          -v) verbose="$1"; shift ;; \
          -*) cxx_flags="${cxx_flags:+$cxx_flags }$1"; shift ;; \
          debug|release|relwithdebinfo|build|all) \
            if [ -n "$mode" ]; then \
              printf '%s\n' "just ir: multiple build modes: '$mode' and '$1'" >&2; \
              exit 1; \
            fi; \
            mode="$1"; shift ;; \
          src|tests|examples|full) add_sel "$1"; shift ;; \
          *) raw="${1%/}"; \
             case "$raw" in \
               "") printf '%s\n' "just ir: empty selector '$1'" >&2; exit 1 ;; \
               *.h|*.hh|*.hpp|*.hxx|*.tpp|*.ipp|*.inc) \
                 printf '%s\n' "just ir: '$1' is a header -- IR is per-TU, pass a .cpp that includes it (optionally narrowed with fn=<symbol>)" >&2; exit 1 ;; \
               *.cpp) sel="${raw##*/}"; sel="${sel%.cpp}" ;; \
               */*) printf '%s\n' "just ir: '$1' isn't a .cpp -- pass a source file, a basename, or src/tests/examples/full" >&2; exit 1 ;; \
               *.*) printf '%s\n' "just ir: '$1' has an unsupported extension -- pass a .cpp source, a basename, or src/tests/examples/full" >&2; exit 1 ;; \
               *) sel="$raw" ;; \
             esac; \
             add_sel "$sel"; shift ;; \
        esac; \
      done; \
      if [ -z "$kinds" ]; then \
        printf '%s\n' "just ir: pick at least one IR: ii, s/intel/att, bc, or ll" >&2; \
        exit 1; \
      fi; \
      if [ -z "$selectors" ]; then \
        printf '%s\n' "just ir: pick at least one selector (filename.cpp, src/tests/examples, full, or a target name)" >&2; \
        exit 1; \
      fi; \
      if [ "$saw_trailer" = true ]; then \
        case "$kinds" in \
          s) ;; \
          *) printf '%s\n' "just ir: '-- <flags>' trailer is forwarded to llvm-objdump and only valid when .s is the sole IR" >&2; exit 1 ;; \
        esac; \
      fi; \
      if [ -z "$mode" ]; then mode="{{ last_mode }}"; fi; \
      cmake_fmt=ON; if [ "$fmt" = "no-fmt" ]; then cmake_fmt=OFF; fi; \
      ninja_targets=""; \
      for sel in $selectors; do \
        for kind in $kinds; do \
          ninja_targets="${ninja_targets:+$ninja_targets }${sel}-${kind}"; \
        done; \
      done; \
      CAT_JUST_IR_FN="$fn" \
        CAT_JUST_IR_PASS="$pass" \
        CAT_JUST_IR_SYNTAX="$syntax" \
        CAT_JUST_IR_FMT="$cmake_fmt" \
        CAT_JUST_IR_TRAILER="$trailer" \
        just _ir-mode "$mode" "$san" "$verbose" "$cxx_flags" "$ninja_targets"; \
      configs=""; \
      case "$mode" in \
        debug)          configs="Debug" ;; \
        release)        configs="Release" ;; \
        relwithdebinfo) configs="RelWithDebInfo" ;; \
        all)            configs="Debug;Release;RelWithDebInfo" ;; \
      esac; \
      if [ -n "$configs" ]; then \
        cmake_selectors=$(printf '%s' "$selectors" | tr ' ' ';'); \
        cmake_kinds=$(printf '%s' "$kinds" | tr ' ' ';'); \
        cmake \
          "-DBUILD_ROOT=build" \
          "-DCONFIGS=$configs" \
          "-DSELECTORS=$cmake_selectors" \
          "-DKINDS=$cmake_kinds" \
          "-DFN=$fn" \
          -P scripts/cat_ir_paths.cmake; \
      fi

syntax mode=last_mode verbose="":
    @just _syntax-mode {{ mode }} {{ verbose }}
    @printf '\033[36m%s\033[0m\n' 'No syntax errors! :3'

# Run `sstrip` on libCat-produced executables to drop section headers,
# symbol tables, and any other non-loadable bytes. Selectors:
#   <name>          a specific executable (e.g. `hello`, `echo`, `unit_tests`)
#   examples        every example executable
#   tests           every test executable
#   full | all      examples + tests
# A mode token (`debug`/`release`/`relwithdebinfo`) selects the build dir;
# defaults to the cached last mode.
strip *args:
    @set -eu; modes=""; selectors=""; \
      add_selector() { \
        case " $selectors " in \
          *" $1 "*) ;; \
          *) selectors="${selectors:+$selectors }$1" ;; \
        esac; \
      }; \
      set -- "$@"; \
      while [ $# -gt 0 ]; do \
        case "$1" in \
          debug|release|relwithdebinfo) \
            modes="${modes:+$modes }$1"; shift ;; \
          examples|tests|full|all) add_selector "$1"; shift ;; \
          -*) printf '%s\n' "just strip: unknown flag '$1'!" >&2; \
             printf '%s\n' "options: <name>, examples, tests, full, all, debug, release, relwithdebinfo" >&2; \
             exit 1 ;; \
          *) add_selector "$1"; shift ;; \
        esac; \
      done; \
      if [ -z "$selectors" ]; then \
        printf '%s\n' "just strip: pick at least one selector (an executable name, examples, tests, or full)" >&2; \
        exit 1; \
      fi; \
      if [ -z "$modes" ]; then modes="{{ last_mode }}"; fi; \
      for mode in $modes; do \
        just _strip-mode "$mode" "$selectors"; \
      done

[private]
_strip-mode mode=last_mode selectors="":
    @set -eu; selectors="{{ selectors }}"; \
      examples_dir="{{ build_dir(mode) }}/examples"; \
      tests_dir="{{ build_dir(mode) }}/tests"; \
      if [ "{{ multi_config }}" = "true" ] && [ -n "{{ cmake_config(mode) }}" ]; then \
        examples_dir="$examples_dir/{{ cmake_config(mode) }}"; \
        tests_dir="$tests_dir/{{ cmake_config(mode) }}"; \
      fi; \
      paths=""; \
      add_path() { \
        case " $paths " in *" $1 "*) ;; *) paths="${paths:+$paths }$1" ;; esac; \
      }; \
      collect_dir() { \
        for f in "$1"/*; do \
          [ -f "$f" ] || continue; \
          [ -x "$f" ] || continue; \
          case "${f##*.}" in bc|o|s|ll|ii|txt|d) continue ;; esac; \
          [ "$(head -c4 "$f" 2>/dev/null | od -An -tx1 | tr -d " \n")" = "7f454c46" ] || continue; \
          add_path "$f"; \
        done; \
      }; \
      for sel in $selectors; do \
        case "$sel" in \
          examples) collect_dir "$examples_dir" ;; \
          tests) collect_dir "$tests_dir" ;; \
          full|all) collect_dir "$examples_dir"; collect_dir "$tests_dir" ;; \
          *) \
            if [ -f "$examples_dir/$sel" ]; then add_path "$examples_dir/$sel"; \
            elif [ -f "$tests_dir/$sel" ]; then add_path "$tests_dir/$sel"; \
            else \
              printf 'just strip: %s: not found under %s or %s\n' "$sel" "$examples_dir" "$tests_dir" >&2; \
              exit 1; \
            fi ;; \
        esac; \
      done; \
      if [ -z "$paths" ]; then \
        printf 'just strip: no executables matched\n' >&2; exit 1; \
      fi; \
      for p in $paths; do \
        before="$(wc -c < "$p")"; \
        sstrip "$p" >/dev/null; \
        after="$(wc -c < "$p")"; \
        printf '\033[1m%s\033[0m: %s -> \033[36m%s\033[0m bytes\n' "$p" "$before" "$after"; \
      done

# Run `elfls` on a libCat-produced executable. Selectors:
#   <name>          a specific executable (e.g. `hello`, `echo`, `unit_tests`)
#   <path>          a path to any ELF file
# A mode token (`debug`/`release`/`relwithdebinfo`) selects the build dir.
# Defaults to the cached last mode. With `-v`, run `readelf -a --demangle`
# instead of `elfls`.
elf *args:
    @set -eu; mode=""; verbose=""; selector=""; set -- "$@"; \
      while [ $# -gt 0 ]; do \
        case "$1" in \
          -v) verbose="-v"; shift ;; \
          debug|release|relwithdebinfo) \
            if [ -n "$mode" ]; then \
              printf '%s\n' "just elf: multiple build modes: '$mode' and '$1'" >&2; \
              exit 1; \
            fi; \
            mode="$1"; shift ;; \
          -*) printf '%s\n' "just elf: unknown flag '$1'!" >&2; exit 1 ;; \
          *) \
            if [ -n "$selector" ]; then \
              printf '%s\n' "just elf: multiple executables: '$selector' and '$1'" >&2; \
              exit 1; \
            fi; \
            selector="$1"; shift ;; \
        esac; \
      done; \
      if [ -z "$selector" ]; then \
        printf '%s\n' "just elf: pick an executable name (e.g. hello, echo, unit_tests) or a path" >&2; \
        exit 1; \
      fi; \
      if [ -z "$mode" ]; then mode="{{ last_mode }}"; fi; \
      just _elf-mode "$mode" "$verbose" "$selector"

[private]
_elf-mode mode=last_mode verbose="" selector="":
    @set -eu; selector="{{ selector }}"; verbose="{{ verbose }}"; \
      examples_dir="{{ build_dir(mode) }}/examples"; \
      tests_dir="{{ build_dir(mode) }}/tests"; \
      if [ "{{ multi_config }}" = "true" ] && [ -n "{{ cmake_config(mode) }}" ]; then \
        examples_dir="$examples_dir/{{ cmake_config(mode) }}"; \
        tests_dir="$tests_dir/{{ cmake_config(mode) }}"; \
      fi; \
      resolve_path() { \
        if [ -f "$1" ]; then printf '%s' "$1"; \
        elif [ -f "$examples_dir/$1" ]; then printf '%s' "$examples_dir/$1"; \
        elif [ -f "$tests_dir/$1" ]; then printf '%s' "$tests_dir/$1"; \
        fi; \
      }; \
      path="$(resolve_path "$selector")"; \
      if [ -z "$path" ]; then \
        case "$selector" in \
          */*) \
            printf 'just elf: %s: file not found\n' "$selector" >&2; \
            exit 1 ;; \
        esac; \
        just _cmake_target_config "$selector" {{ mode }} "$verbose"; \
        path="$(resolve_path "$selector")"; \
        if [ -z "$path" ]; then \
          printf 'just elf: %s: not found under %s or %s after build\n' \
            "$selector" "$examples_dir" "$tests_dir" >&2; \
          exit 1; \
        fi; \
      fi; \
      if [ "$verbose" = "-v" ]; then \
        llvm-readelf -a --demangle --wide "$path"; \
      else \
        elfls "$path"; \
        printf '\nRun \033[1mjust elf -v\033[0m for `llvm-readelf -a` (demangled) instead.\n'; \
      fi

test *args:
    @san=""; verbose=""; modes=""; tests=""; full="false"; list="false"; \
      unset CAT_JUST_TEST_TOOL_TRAILER_B64; \
      add_test_selector() { \
        case " $tests " in \
          *" $1 "*) ;; \
          *) tests="${tests:+$tests }$1" ;; \
        esac; \
      }; \
      set -- "$@"; \
      while [ $# -gt 0 ]; do \
        arg="$1"; \
        lower="$(printf '%s' "$arg" | tr '[:upper:]' '[:lower:]')"; \
        case "$lower" in \
          --) shift; \
            if [ $# -gt 0 ]; then \
              CAT_JUST_TEST_TOOL_TRAILER_B64="$(printf '%s\0' "$@" | base64 -w0)"; \
              export CAT_JUST_TEST_TOOL_TRAILER_B64; \
            fi; \
            break ;; \
          san|nosan) san="$1"; shift ;; \
          -v) verbose="$1"; shift ;; \
          debug|release|relwithdebinfo|build|all) modes="${modes:+$modes }$lower"; shift ;; \
          unit) \
            add_test_selector UnitTests; shift ;; \
          gdb|gdbprettyprinters) \
            add_test_selector GdbPrettyPrinters; shift ;; \
          arithmetic|arithmetictypecheck) \
            add_test_selector ArithmeticTypeCheck; shift ;; \
          full) full="true"; shift ;; \
          list) list="true"; shift ;; \
          *) printf '%s\n' "just test: unknown option '$arg'!" >&2; \
             printf '%s\n' "options: unit, gdb, arithmetic, full, list, san, nosan, -v, debug, release, relwithdebinfo, build, all" >&2; \
             exit 1 ;; \
        esac; \
      done; \
      if [ "$full" = "true" ] && [ -n "$tests" ]; then \
        printf '%s\n' "just test: 'full' cannot be combined with named tests!" >&2; \
        exit 1; \
      fi; \
      if [ "$list" = "true" ] && [ -z "$tests" ]; then full="true"; fi; \
      if [ "$full" = "false" ] && [ -z "$tests" ]; then tests="UnitTests"; fi; \
      regex=""; \
      if [ "$full" = "false" ]; then \
        for test_name in $tests; do \
          regex="${regex:+$regex|}$test_name"; \
        done; \
        regex="^($regex)$"; \
      fi; \
      if [ -z "$modes" ]; then modes="{{ last_mode }}"; fi; \
      for mode in $modes; do \
        just _test-mode "$mode" "$san" "$verbose" "$regex" "$list"; \
      done; \
      unset CAT_JUST_TEST_TOOL_TRAILER_B64

# Print out various properties of a release mode.
# Any empty fields are folded in non-verbose output.
# The potential fields are:
#   Build mode
#   CAT_USE_SANITIZERS
#   CMAKE_CXX_COMPILER
#   CMAKE_LINKER_TYPE (or "Linker" if specified by `-fuse-ld`)
#   ISA
#   Language
#   C++ modules
#   #define
#   Inclusion
#   Exceptions
#   RTTI
#   Warnings
#   Disabled warnings
#   Diagnostics
#   Lifetime/Analysis
#   Features
#   Architecture
#   Math
#   Optimization
#   Profiling
#   LTO
#   Visibility
#   Sanitizers
#   Debug
#   Other
#   Link options
#   Link libraries
#   Non-verbose status skips empty sections.
status *args:
    @mode=""; verbose=""; set -- "$@"; \
      while [ $# -gt 0 ]; do \
        case "$1" in \
          -v) verbose="$1"; shift ;; \
          debug|release|relwithdebinfo|build|all) \
            if [ -n "$mode" ]; then \
              printf '%s\n' "just status: multiple build modes: '$mode' and '$1'" >&2; \
              exit 1; \
            fi; \
            mode="$1"; shift ;; \
          *) printf '%s\n' "just status: unknown flag '$1'!" >&2; exit 1 ;; \
        esac; \
      done; \
      if [ -z "$mode" ]; then \
        mode="{{ last_mode }}"; \
        printf '\033[1mBuild mode: \033[0m%s \033[90m(default)\033[0m\n' "$mode"; \
      fi; \
      just status_san "$mode"; \
      just status_cxx "$mode" "$verbose"; \
      just status_linker "$mode"; \
      just status_isa "$mode"; \
      just status_std "$mode" "$verbose"; \
      just status_modules "$mode" "$verbose"; \
      just status_define "$mode" "$verbose"; \
      just status_headers "$mode" "$verbose"; \
      just status_exceptions "$mode" "$verbose"; \
      just status_rtti "$mode" "$verbose"; \
      just status_warn "$mode" "$verbose"; \
      just status_nowarn "$mode" "$verbose"; \
      just status_diagnostics "$mode" "$verbose"; \
      just status_life "$mode" "$verbose"; \
      just status_feat "$mode" "$verbose"; \
      just status_arch "$mode" "$verbose"; \
      just status_math "$mode" "$verbose"; \
      just status_opt "$mode" "$verbose"; \
      just status_profiling "$mode" "$verbose"; \
      just status_lto "$mode" "$verbose"; \
      just status_visibility "$mode" "$verbose"; \
      just status_san_flags "$mode" "$verbose"; \
      just status_dbg "$mode" "$verbose"; \
      just status_other "$mode" "$verbose"; \
      just status_link "$mode" "$verbose"; \
      if [ -z "$verbose" ]; then \
        printf '\nRun \033[1mjust status -v\033[0m to see all fields or flag descriptions.\n'; \
      fi

status_san mode=last_mode san="":
    @just _print-sanitizer-status {{ build_dir(mode) }} "{{ san }}"

status_cxx mode=last_mode verbose="":
    @cache="{{ build_dir(mode) }}/CMakeCache.txt"; \
      value="$(printf '\033[90mn/a\033[0m')"; \
      test ! -f "$cache" \
        || value="$(sed -n 's/^CMAKE_CXX_COMPILER:[^=]*=//p' "$cache")"; \
      if [ "{{ verbose }}" != "-v" ]; then \
        case "$value" in \
          "$PWD"/*) value=".${value#"$PWD"}" ;; \
        esac; \
      fi; \
      if [ -z "$value" ]; then value="$(printf '\033[90mn/a\033[0m')"; fi; \
      printf '\033[1mCMAKE_CXX_COMPILER: \033[0m%s\n' "$value"

status_linker mode=last_mode:
    @python3 scripts/status_linker.py "{{ build_dir(mode) }}" "{{ cmake_config(mode) }}"

status_isa mode=last_mode:
    @python3 scripts/status_cxx_flags.py "{{ build_dir(mode) }}" "{{ cmake_config(mode) }}" isa ISA

status_std mode=last_mode verbose="":
    @python3 scripts/status_cxx_flags.py "{{ build_dir(mode) }}" "{{ cmake_config(mode) }}" language Language {{ status_verbose(verbose) }}

status_modules mode=last_mode verbose="":
    @python3 scripts/status_cxx_flags.py "{{ build_dir(mode) }}" "{{ cmake_config(mode) }}" modules "C++ modules" {{ status_verbose(verbose) }}

status_define mode=last_mode verbose="":
    @python3 scripts/status_cxx_flags.py "{{ build_dir(mode) }}" "{{ cmake_config(mode) }}" define "#define" {{ status_verbose(verbose) }}

status_headers mode=last_mode verbose="":
    @python3 scripts/status_cxx_flags.py "{{ build_dir(mode) }}" "{{ cmake_config(mode) }}" headers Inclusion {{ status_verbose(verbose) }}

status_exceptions mode=last_mode verbose="":
    @python3 scripts/status_cxx_flags.py "{{ build_dir(mode) }}" "{{ cmake_config(mode) }}" exceptions "Exceptions" {{ status_verbose(verbose) }}

status_rtti mode=last_mode verbose="":
    @python3 scripts/status_cxx_flags.py "{{ build_dir(mode) }}" "{{ cmake_config(mode) }}" rtti "RTTI" {{ status_verbose(verbose) }}

status_warn mode=last_mode verbose="":
    @python3 scripts/status_cxx_flags.py "{{ build_dir(mode) }}" "{{ cmake_config(mode) }}" warn Warnings {{ status_verbose(verbose) }}

status_nowarn mode=last_mode verbose="":
    @python3 scripts/status_cxx_flags.py "{{ build_dir(mode) }}" "{{ cmake_config(mode) }}" nowarn "Disabled warnings" {{ status_verbose(verbose) }}

status_diagnostics mode=last_mode verbose="":
    @python3 scripts/status_cxx_flags.py "{{ build_dir(mode) }}" "{{ cmake_config(mode) }}" diagnostics Diagnostics {{ status_verbose(verbose) }}

status_life mode=last_mode verbose="":
    @python3 scripts/status_cxx_flags.py "{{ build_dir(mode) }}" "{{ cmake_config(mode) }}" lifetime "Lifetime/Analysis" {{ status_verbose(verbose) }}

status_feat mode=last_mode verbose="":
    @python3 scripts/status_cxx_flags.py "{{ build_dir(mode) }}" "{{ cmake_config(mode) }}" feat Features {{ status_verbose(verbose) }}

status_arch mode=last_mode verbose="":
    @python3 scripts/status_cxx_flags.py "{{ build_dir(mode) }}" "{{ cmake_config(mode) }}" arch Architecture {{ status_verbose(verbose) }}

status_math mode=last_mode verbose="":
    @python3 scripts/status_cxx_flags.py "{{ build_dir(mode) }}" "{{ cmake_config(mode) }}" math Math {{ status_verbose(verbose) }}

status_opt mode=last_mode verbose="":
    @python3 scripts/status_cxx_flags.py "{{ build_dir(mode) }}" "{{ cmake_config(mode) }}" opt Optimization {{ status_verbose(verbose) }}

status_profiling mode=last_mode verbose="":
    @python3 scripts/status_cxx_flags.py "{{ build_dir(mode) }}" "{{ cmake_config(mode) }}" profiling Profiling {{ status_verbose(verbose) }}

status_lto mode=last_mode verbose="":
    @python3 scripts/status_cxx_flags.py "{{ build_dir(mode) }}" "{{ cmake_config(mode) }}" lto LTO {{ status_verbose(verbose) }}

status_visibility mode=last_mode verbose="":
    @python3 scripts/status_cxx_flags.py "{{ build_dir(mode) }}" "{{ cmake_config(mode) }}" visibility Visibility {{ status_verbose(verbose) }}

status_san_flags mode=last_mode verbose="":
    @python3 scripts/status_cxx_flags.py "{{ build_dir(mode) }}" "{{ cmake_config(mode) }}" san Sanitizers {{ status_verbose(verbose) }}

status_dbg mode=last_mode verbose="":
    @python3 scripts/status_cxx_flags.py "{{ build_dir(mode) }}" "{{ cmake_config(mode) }}" dbg Debug {{ status_verbose(verbose) }}

status_other mode=last_mode verbose="":
    @python3 scripts/status_cxx_flags.py "{{ build_dir(mode) }}" "{{ cmake_config(mode) }}" other Other {{ status_verbose(verbose) }}

status_link mode=last_mode verbose="":
    @python3 scripts/status_link_flags.py "{{ build_dir(mode) }}" "{{ cmake_config(mode) }}" "Link options" "Link libraries" {{ status_verbose(verbose) }}

repl *args:
    @mode="{{ last_mode }}"; san=""; verbose=""; set -- "$@"; \
      while [ $# -gt 0 ]; do \
        case "$1" in \
          debug|release|relwithdebinfo) mode="$1"; shift ;; \
          san|nosan) san="$1"; shift ;; \
          -v) verbose="$1"; shift ;; \
          *) break ;; \
        esac; \
      done; \
      just _repl "$mode" "$san" "$verbose" "$@"

[private]
_noop *args:
    @:

[private]
_print-build-mode mode=last_mode:
    @printf '\033[1mBuild mode: \033[0m%s\n' "{{ mode(mode) }}"

[private]
_build-mode mode="release" san="" verbose="" no_warnings="false" cxx_flags="":
    @just {{ if debug_config(mode) == "" { "_noop" } else { "_build-config" } }} \
      debug "{{ san }}" "{{ verbose }}" "{{ no_warnings }}" "{{ cxx_flags }}"

    @just {{ if release_config(mode) == "" { "_noop" } else { "_build-config" } }} \
      release "{{ san }}" "{{ verbose }}" "{{ no_warnings }}" "{{ cxx_flags }}"

    @just {{ if relwithdebinfo_config(mode) == "" { "_noop" } else { "_build-config" } }} \
      relwithdebinfo "{{ san }}" "{{ verbose }}" "{{ no_warnings }}" "{{ cxx_flags }}"

    @just {{ if cmake_config(mode) == "" { "_build-custom" } else { "_noop" } }} \
      {{ mode }} "{{ san }}" "{{ verbose }}" "{{ no_warnings }}" "{{ cxx_flags }}"

# We need a shell script to handle my fairly complex flags passing logic.
# Everything that looks like a flag should be forwarded through to
# `CMAKE_CXX_FLAGS`, but `-w` specifically is a *non-stateful* sugar for
# `-Wno-everything -Wl,--no-warnings` (so both compiler and linker
# warnings are silenced).
[private]
_build-config mode="release" san="" verbose="" no_warnings="false" cxx_flags="":
    @just _print-build-mode {{ mode }}

    @bash -c 'set -euo pipefail; no_warnings=$1; cxx_flags=$2; clear_cached=false; \
      cache={{ build_dir(mode) }}/CMakeCache.txt; \
      if [[ "${no_warnings}" == true ]]; then \
        cxx_flags="${cxx_flags:+${cxx_flags} }-Wno-everything -Wl,--no-warnings"; \
      elif [[ -z "${cxx_flags}" && -f "${cache}" ]]; then \
        cached_flags=""; \
        while IFS= read -r line; do \
          case "${line}" in \
            CMAKE_CXX_FLAGS:STRING=*) cached_flags="${line#CMAKE_CXX_FLAGS:STRING=}" ;; \
          esac; \
        done < "${cache}"; \
        if [[ " ${cached_flags} " == *" -Wno-everything "* ]]; then \
          for flag in ${cached_flags}; do \
            case "${flag}" in \
              -Wno-everything|-Wl,--no-warnings) ;; \
              *) cxx_flags="${cxx_flags:+${cxx_flags} }${flag}" ;; \
            esac; \
          done; \
          clear_cached=true; \
        fi; \
      fi; \
      configure=(cmake {{ cmake_log(verbose) }} -S . -B {{ build_dir(mode) }} \
        {{ configure_config(mode) }} {{ sanitizer(san) }}); \
      if [[ -n "${cxx_flags}" || "${clear_cached}" == true ]]; then \
        configure+=("-DCMAKE_CXX_FLAGS=${cxx_flags}"); \
      fi; \
      if [[ "{{ can_skip_configure(san) }}" != true || -n "${cxx_flags}" \
          || "${clear_cached}" == true \
          || ! -d {{ build_dir(mode) }} ]]; then \
        "${configure[@]}"; \
      fi; \
      just status_san {{ mode }} "{{ san }}"; \
      trailer=(); \
      if [[ -n "${CAT_JUST_BUILD_TOOL_TRAILER_B64:-}" ]]; then \
        mapfile -d "" -t trailer \
          < <(base64 -d <<< "${CAT_JUST_BUILD_TOOL_TRAILER_B64}"); \
      fi; \
      if [[ ${#trailer[@]} -gt 0 ]]; then \
        cmake --build {{ build_dir(mode) }} \
          {{ build_config(mode) }} {{ build_verbose(verbose) }} \
          -- "${trailer[@]}"; \
      else \
        cmake --build {{ build_dir(mode) }} \
          {{ build_config(mode) }} {{ build_verbose(verbose) }}; \
      fi' _ "{{ no_warnings }}" "{{ cxx_flags }}"

    @mkdir -p .cache
    @printf '%s\n' "{{ mode(mode) }}" > .cache/cat-build-mode

[private]
_build-custom mode="" san="" verbose="" no_warnings="false" cxx_flags="":
    @just _print-build-mode {{ mode }}

    @bash -c 'set -euo pipefail; no_warnings=$1; cxx_flags=$2; clear_cached=false; \
      cache={{ build_dir(mode) }}/CMakeCache.txt; \
      if [[ "${no_warnings}" == true ]]; then \
        cxx_flags="${cxx_flags:+${cxx_flags} }-Wno-everything -Wl,--no-warnings"; \
      elif [[ -z "${cxx_flags}" && -f "${cache}" ]]; then \
        cached_flags=""; \
        while IFS= read -r line; do \
          case "${line}" in \
            CMAKE_CXX_FLAGS:STRING=*) cached_flags="${line#CMAKE_CXX_FLAGS:STRING=}" ;; \
          esac; \
        done < "${cache}"; \
        if [[ " ${cached_flags} " == *" -Wno-everything "* ]]; then \
          for flag in ${cached_flags}; do \
            case "${flag}" in \
              -Wno-everything|-Wl,--no-warnings) ;; \
              *) cxx_flags="${cxx_flags:+${cxx_flags} }${flag}" ;; \
            esac; \
          done; \
          clear_cached=true; \
        fi; \
      fi; \
      configure=(cmake {{ cmake_log(verbose) }} -S . -B {{ build_dir(mode) }} \
        {{ sanitizer(san) }}); \
      if [[ -n "${cxx_flags}" || "${clear_cached}" == true ]]; then \
        configure+=("-DCMAKE_CXX_FLAGS=${cxx_flags}"); \
      fi; \
      if [[ "{{ can_skip_configure(san) }}" != true || -n "${cxx_flags}" \
          || "${clear_cached}" == true \
          || ! -d {{ build_dir(mode) }} ]]; then \
        "${configure[@]}"; \
      fi; \
      just status_san {{ mode }} "{{ san }}"; \
      trailer=(); \
      if [[ -n "${CAT_JUST_BUILD_TOOL_TRAILER_B64:-}" ]]; then \
        mapfile -d "" -t trailer \
          < <(base64 -d <<< "${CAT_JUST_BUILD_TOOL_TRAILER_B64}"); \
      fi; \
      if [[ ${#trailer[@]} -gt 0 ]]; then \
        cmake --build {{ build_dir(mode) }} {{ build_verbose(verbose) }} \
          -- "${trailer[@]}"; \
      else \
        cmake --build {{ build_dir(mode) }} {{ build_verbose(verbose) }}; \
      fi' _ "{{ no_warnings }}" "{{ cxx_flags }}"

[private]
_print-sanitizer-status build_dir san="":
    @just {{ sanitizer_status_recipe(san) }} \
      "{{ build_dir }}" "{{ sanitizer_status(san) }}"

# Colors within the range of https://blog.xoria.org/terminal-colors/
[private]
_print-requested-sanitizer-status _build_dir status:
    @status="{{ status }}"; \
      case "$status" in \
        ON) status="$(printf '\033[36mON\033[0m')" ;; \
        OFF) status="$(printf '\033[91mOFF\033[0m')" ;; \
      esac; \
      printf '\033[1mCAT_USE_SANITIZERS: \033[0m%s \033[90m(requested)\033[0m\n' "$status"

[private]
_print-cached-sanitizer-status build_dir _status="":
    @status="$(printf '\033[90mn/a\033[0m')"; \
      test ! -f "{{ build_dir }}/CMakeCache.txt" \
        || status="$(sed -n 's/^CAT_USE_SANITIZERS:BOOL=//p' "{{ build_dir }}/CMakeCache.txt")"; \
      case "$status" in \
        ON) status="$(printf '\033[36mON\033[0m')" ;; \
        OFF) status="$(printf '\033[91mOFF\033[0m')" ;; \
      esac; \
      printf '\033[1mCAT_USE_SANITIZERS: \033[0m%s \033[90m(cached)\033[0m\n' "$status"

[private]
_clean-mode mode=last_mode verbose="":
    @just {{ if debug_config(mode) == "" { "_noop" } else { "_clean-config" } }} \
      debug {{ verbose }}
    @just {{ if release_config(mode) == "" { "_noop" } else { "_clean-config" } }} \
      release {{ verbose }}
    @just {{ if relwithdebinfo_config(mode) == "" { "_noop" } else { "_clean-config" } }} \
      relwithdebinfo {{ verbose }}
    @just {{ if cmake_config(mode) == "" { "_clean-config" } else { "_noop" } }} \
      {{ mode }} {{ verbose }}

[private]
_clean-config mode=last_mode verbose="":
    @test ! -f {{ build_dir(mode) }}/CMakeCache.txt \
      || cmake --build {{ build_dir(mode) }} {{ build_config(mode) }} \
        --target clean {{ build_verbose(verbose) }}

[private]
_ir-mode mode=last_mode san="" verbose="" cxx_flags="" targets="":
    @just {{ if debug_config(mode) == "" { "_noop" } else { "_ir-config" } }} \
      debug "{{ san }}" "{{ verbose }}" "{{ cxx_flags }}" "{{ targets }}"

    @just {{ if release_config(mode) == "" { "_noop" } else { "_ir-config" } }} \
      release "{{ san }}" "{{ verbose }}" "{{ cxx_flags }}" "{{ targets }}"

    @just {{ if relwithdebinfo_config(mode) == "" { "_noop" } else { "_ir-config" } }} \
      relwithdebinfo "{{ san }}" "{{ verbose }}" "{{ cxx_flags }}" "{{ targets }}"

    @just {{ if cmake_config(mode) == "" { "_ir-config" } else { "_noop" } }} \
      {{ mode }} "{{ san }}" "{{ verbose }}" "{{ cxx_flags }}" "{{ targets }}"

# Configures the build dir with the cache vars `cat_ir.cmake` reads, then
# fans out to one `cmake --build --target` per requested ninja target. CMake
# `--target` accepts multiple targets in one invocation, but splitting them
# keeps `--verbose` per-target output sane and matches `just build`'s
# "configure once, build deterministically" shape.
[private]
_ir-config mode="release" san="" verbose="" cxx_flags="" targets="":
    @just _print-build-mode {{ mode }}
    @bash -c 'set -euo pipefail; cxx_flags=$1; \
      configure=(cmake {{ cmake_log(verbose) }} -S . -B {{ build_dir(mode) }} \
        {{ configure_config(mode) }} {{ sanitizer(san) }} \
        "-DCAT_IR_FUNCTION=${CAT_JUST_IR_FN:-}" \
        "-DCAT_IR_PASS=${CAT_JUST_IR_PASS:-}" \
        "-DCAT_IR_S_SYNTAX=${CAT_JUST_IR_SYNTAX:-intel}" \
        "-DCAT_IR_FMT=${CAT_JUST_IR_FMT:-ON}" \
        "-DCAT_IR_OBJDUMP_TRAILER=${CAT_JUST_IR_TRAILER:-}"); \
      if [[ -n "${cxx_flags}" ]]; then \
        configure+=("-DCMAKE_CXX_FLAGS=${cxx_flags}"); \
      fi; \
      "${configure[@]}"' _ "{{ cxx_flags }}"
    @just status_san {{ mode }} "{{ san }}"
    @bash -c 'set -euo pipefail; targets=$1; \
      if [[ -z "${targets}" ]]; then exit 0; fi; \
      cmake --build {{ build_dir(mode) }} {{ build_config(mode) }} \
        --target ${targets} {{ build_verbose(verbose) }}' _ "{{ targets }}"

cmake_target target mode=last_mode verbose="":
    @just _cmake_target_mode {{ target }} {{ mode }} {{ verbose }}

[private]
_cmake_target_mode target mode=last_mode verbose="":
    @just {{ if debug_config(mode) == "" { "_noop" } else { "_cmake_target_config" } }} \
      {{ target }} debug {{ verbose }}

    @just {{ if release_config(mode) == "" { "_noop" } else { "_cmake_target_config" } }} \
      {{ target }} release {{ verbose }}

    @just {{ if relwithdebinfo_config(mode) == "" { "_noop" } else { "_cmake_target_config" } }} \
      {{ target }} relwithdebinfo {{ verbose }}

    @just {{ if cmake_config(mode) == "" { "_cmake_target_config" } else { "_noop" } }} \
      {{ target }} {{ mode }} {{ verbose }}

[private]
_cmake_target_config target mode=last_mode verbose="":
    @cmake --build {{ build_dir(mode) }} {{ build_config(mode) }} \
      --target {{ target }} {{ build_verbose(verbose) }}

[private]
_syntax-mode mode=last_mode verbose="":
    @just {{ if debug_config(mode) == "" { "_noop" } else { "_syntax-config" } }} \
      debug {{ verbose }}

    @just {{ if release_config(mode) == "" { "_noop" } else { "_syntax-config" } }} \
      release {{ verbose }}

    @just {{ if relwithdebinfo_config(mode) == "" { "_noop" } else { "_syntax-config" } }} \
      relwithdebinfo {{ verbose }}

    @just {{ if cmake_config(mode) == "" { "_syntax-config" } else { "_noop" } }} \
      {{ mode }} {{ verbose }}

[private]
_syntax-config mode=last_mode verbose="":
    @just _print-build-mode {{ mode }}
    @just status_san {{ mode }}
    @cmake --build {{ build_dir(mode) }} {{ build_config(mode) }} \
      --target cat-syntax {{ build_verbose(verbose) }}

    @mkdir -p .cache
    @printf '%s\n' "{{ mode(mode) }}" > .cache/cat-build-mode

[private]
_test-mode mode=last_mode san="" verbose="" test_regex="" list="false":
    @just {{ if debug_config(mode) == "" { "_noop" } else { "_test-config" } }} \
      debug "{{ san }}" "{{ verbose }}" "{{ test_regex }}" "{{ list }}"

    @just {{ if release_config(mode) == "" { "_noop" } else { "_test-config" } }} \
      release "{{ san }}" "{{ verbose }}" "{{ test_regex }}" "{{ list }}"

    @just {{ if relwithdebinfo_config(mode) == "" { "_noop" } else { "_test-config" } }} \
      relwithdebinfo "{{ san }}" "{{ verbose }}" "{{ test_regex }}" "{{ list }}"

    @just {{ if cmake_config(mode) == "" { "_test-config" } else { "_noop" } }} \
      {{ mode }} "{{ san }}" "{{ verbose }}" "{{ test_regex }}" "{{ list }}"

[private]
_test-config mode=last_mode san="" verbose="" test_regex="" list="false":
    @if [ "{{ list }}" != "true" ]; then \
      just _build-config {{ mode }} "{{ san }}" "{{ verbose }}" false ""; \
    fi

    @bash -c 'set -euo pipefail; test_regex="$1"; list="$2"; verbose="$3"; trailer=(); \
      if [[ -n "${CAT_JUST_TEST_TOOL_TRAILER_B64:-}" ]]; then \
        mapfile -d "" -t trailer \
          < <(base64 -d <<< "${CAT_JUST_TEST_TOOL_TRAILER_B64}"); \
      fi; \
      ctest_args=(ctest --test-dir {{ build_dir(mode) }} {{ test_config(mode) }} \
        --progress); \
      ctest_verbose=false; \
      if [[ "${verbose}" == "-v" ]]; then \
        ctest_args+=(--verbose); \
        ctest_verbose=true; \
      fi; \
      if [[ "${ctest_verbose}" != true ]]; then \
        ctest_args+=(--output-on-failure); \
      fi; \
      if [[ "${list}" == true ]]; then \
        ctest_args+=(-N); \
      fi; \
      if [[ -n "${test_regex}" ]]; then \
        ctest_args+=(-R "${test_regex}"); \
      fi; \
      "${ctest_args[@]}" "${trailer[@]}"' _ "{{ test_regex }}" "{{ list }}" "{{ verbose }}"

[private]
_repl mode san="" verbose="" *args:
    @just _print-build-mode {{ mode }}

    @test "{{ can_skip_configure(san) }}" = true \
      && test -x {{ build_dir(mode) }}/clang-repl-libcat \
      || cmake {{ cmake_log(verbose) }} -S . -B {{ build_dir(mode) }} \
        {{ configure_config(mode) }} -DCAT_BUILD_SHARED=ON {{ sanitizer(san) }}

    @just status_san {{ mode }} "{{ san }}"
    @cmake --build {{ build_dir(mode) }} {{ build_config(mode) }} \
      --target cat-impl-shared {{ build_verbose(verbose) }}

    @set -- "$@"; shift 3; \
      {{ repl_command(mode) }} \
        "$@"
