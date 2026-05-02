set unstable
set positional-arguments

default:
    @just --list

# Cache the last release mode.
saved_mode := `if test -f .cache/cat-build-mode; then cat .cache/cat-build-mode; else printf release; fi`
last_mode := env_var_or_default("CAT_BUILD_MODE", saved_mode)
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
repl_command(arg) := if mode(arg) == "" { repl_path(arg) } else { configured_repl_command(arg) }
repl_path(arg) := build_dir(arg) + "/clang-repl-libcat"
configured_repl_command(arg) := if multi_config == "true" { multi_repl_command(arg) } else { repl_path(arg) }
multi_repl_command(arg) := "CAT_REPL_CONFIG=" + cmake_config(arg) + " ./build/clang-repl-libcat"

# Implement `-v`.
cmake_log(verbose) := if verbose == "-v" { "--log-level=VERBOSE" } else { "" }
build_verbose(verbose) := if verbose == "-v" { "--verbose" } else { "" }
status_verbose(verbose) := if verbose == "-v" { "--verbose" } else { "" }

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
    just _clean-mode {{ mode }} {{ verbose }}

format mode=last_mode verbose="":
    just cmake_target cat-format {{ mode }} {{ verbose }}

format-check mode=last_mode verbose="":
    just cmake_target cat-format-check {{ mode }} {{ verbose }}

restyle-comments mode=last_mode verbose="":
    just cmake_target cat-restyle-comments {{ mode }} {{ verbose }}

restyle-comments-check mode=last_mode verbose="":
    just cmake_target cat-restyle-comments-check {{ mode }} {{ verbose }}

tidy mode=last_mode verbose="":
    just cmake_target cat-tidy {{ mode }} {{ verbose }}

tidy-check mode=last_mode verbose="":
    just cmake_target cat-tidy-check {{ mode }} {{ verbose }}

opt-report mode=last_mode verbose="":
    just cmake_target cat-opt-report {{ mode }} {{ verbose }}

intermediary_format(flag) := if flag == "fmt" { "ON" } else { "OFF" }

# Same keyword convention as =just build=: =san= / =nosan=, =fmt= / =no-fmt=, =-v=, optional
# =bc= / =ll= / =cir= outputs, one =pass= pipeline, one release mode (=debug=, =release=, …), and optional
# =tests/=, =examples/=, or library-name selectors.
intermediaries *args:
    @mode=""; selectors=""; fmt="no-fmt"; verbose=""; san=""; bc="OFF"; ll="OFF"; cir="OFF"; pass=""; set -- "$@"; \
      while [ $# -gt 0 ]; do \
        case "$1" in \
          san|nosan) san="$1"; shift ;; \
          fmt|no-fmt) fmt="$1"; shift ;; \
          bc) bc="ON"; shift ;; \
          ll) ll="ON"; shift ;; \
          cir) cir="ON"; shift ;; \
          pass=*) pass="${1#pass=}"; ll="ON"; shift ;; \
          -v) verbose="$1"; shift ;; \
          debug|release|relwithdebinfo|build|all) \
            if [ -n "${mode}" ]; then \
              printf '%s\n' "just intermediaries: multiple build modes: '${mode}' and '$1'" >&2; \
              exit 1; \
            fi; \
            mode="$1"; shift ;; \
          *) \
            selector="${1%/}"; \
            selectors="${selectors:+$selectors,}${selector}"; \
            shift ;; \
        esac; \
      done; \
      if [ -z "${mode}" ]; then mode="{{ last_mode }}"; fi; \
      just _intermediaries-mode "${mode}" "${san}" "${fmt}" "${verbose}" "${bc}" "${ll}" "${cir}" "${selectors}" "${pass}"

syntax mode=last_mode verbose="":
    @just _syntax-mode {{ mode }} {{ verbose }}
    @printf '\033[36m%s\033[0m\n' 'No syntax errors! :3'

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
        || value="$(sed -n 's/^CMAKE_CXX_COMPILER:FILEPATH=//p' "$cache")"; \
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
# `-Wno-everything`.
[private]
_build-config mode="release" san="" verbose="" no_warnings="false" cxx_flags="":
    @just _print-build-mode {{ mode }}

    @bash -c 'set -euo pipefail; no_warnings=$1; cxx_flags=$2; clear_cached=false; \
      cache={{ build_dir(mode) }}/CMakeCache.txt; \
      if [[ "${no_warnings}" == true ]]; then \
        cxx_flags="${cxx_flags:+${cxx_flags} }-Wno-everything"; \
      elif [[ -z "${cxx_flags}" && -f "${cache}" ]]; then \
        cached_flags=""; \
        while IFS= read -r line; do \
          case "${line}" in \
            CMAKE_CXX_FLAGS:STRING=*) cached_flags="${line#CMAKE_CXX_FLAGS:STRING=}" ;; \
          esac; \
        done < "${cache}"; \
        if [[ " ${cached_flags} " == *" -Wno-everything "* ]]; then \
          for flag in ${cached_flags}; do \
            [[ "${flag}" == "-Wno-everything" ]] || cxx_flags="${cxx_flags:+${cxx_flags} }${flag}"; \
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
        cxx_flags="${cxx_flags:+${cxx_flags} }-Wno-everything"; \
      elif [[ -z "${cxx_flags}" && -f "${cache}" ]]; then \
        cached_flags=""; \
        while IFS= read -r line; do \
          case "${line}" in \
            CMAKE_CXX_FLAGS:STRING=*) cached_flags="${line#CMAKE_CXX_FLAGS:STRING=}" ;; \
          esac; \
        done < "${cache}"; \
        if [[ " ${cached_flags} " == *" -Wno-everything "* ]]; then \
          for flag in ${cached_flags}; do \
            [[ "${flag}" == "-Wno-everything" ]] || cxx_flags="${cxx_flags:+${cxx_flags} }${flag}"; \
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

# =san= before =fmt= / =verbose= so we never drop an empty =verbose= and reorders =nosan=.
[private]
_intermediaries-mode mode=last_mode san="" fmt="no-fmt" verbose="" bc="OFF" ll="OFF" cir="OFF" selectors="" pass="":
    @just {{ if debug_config(mode) == "" { "_noop" } else { "_intermediaries-config" } }} \
      debug "{{ san }}" "{{ fmt }}" "{{ verbose }}" "{{ bc }}" "{{ ll }}" "{{ cir }}" "{{ selectors }}" "{{ pass }}"

    @just {{ if release_config(mode) == "" { "_noop" } else { "_intermediaries-config" } }} \
      release "{{ san }}" "{{ fmt }}" "{{ verbose }}" "{{ bc }}" "{{ ll }}" "{{ cir }}" "{{ selectors }}" "{{ pass }}"

    @just {{ if relwithdebinfo_config(mode) == "" { "_noop" } else { "_intermediaries-config" } }} \
      relwithdebinfo "{{ san }}" "{{ fmt }}" "{{ verbose }}" "{{ bc }}" "{{ ll }}" "{{ cir }}" "{{ selectors }}" "{{ pass }}"

    @just {{ if cmake_config(mode) == "" { "_intermediaries-config" } else { "_noop" } }} \
      {{ mode }} "{{ san }}" "{{ fmt }}" "{{ verbose }}" "{{ bc }}" "{{ ll }}" "{{ cir }}" "{{ selectors }}" "{{ pass }}"

[private]
_intermediaries-config mode=last_mode san="" fmt="no-fmt" verbose="" bc="OFF" ll="OFF" cir="OFF" selectors="" pass="":
    @cmake {{ cmake_log(verbose) }} -S . -B {{ build_dir(mode) }} \
      {{ configure_config(mode) }} -DCAT_FORMAT_INTERMEDIARIES={{ intermediary_format(fmt) }} \
      -DCAT_INTERMEDIARIES_BC={{ bc }} \
      -DCAT_INTERMEDIARIES_LL={{ ll }} \
      -DCAT_INTERMEDIARIES_CIR={{ cir }} \
      -DCAT_INTERMEDIARIES_SELECTORS={{ selectors }} \
      "-DCAT_INTERMEDIARIES_PASS={{ pass }}" \
      {{ sanitizer(san) }}

    @just status_san {{ mode }} "{{ san }}"
    cmake --build {{ build_dir(mode) }} {{ build_config(mode) }} \
      --target cat-intermediaries {{ build_verbose(verbose) }}

cmake_target target mode=last_mode verbose="":
    just _cmake_target_mode {{ target }} {{ mode }} {{ verbose }}

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
    cmake --build {{ build_dir(mode) }} {{ build_config(mode) }} \
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
