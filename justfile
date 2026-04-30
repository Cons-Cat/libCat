set unstable
set positional-arguments

default:
    @just --list

# Cache the last release mode.
saved_mode := `if test -f .cache/cat-build-mode; then cat .cache/cat-build-mode; else printf release; fi`
last_mode := env_var_or_default("CAT_BUILD_MODE", saved_mode)
multi_config := if env_var_or_default("CMAKE_GENERATOR", "") == "Ninja Multi-Config" { "true" } else { "false" }
mode(arg) := arg

# Get CMake target/config names for a release mode.
# TODO: `just` logical operators are WIP. When they're better, we can simplify this.
# https://github.com/casey/just/issues/3258
matches_mode(arg, release_mode) := if mode(arg) == release_mode { "true" } else if mode(arg) == "all" { "true" } else { "false" }
debug_config(arg) := if matches_mode(arg, "debug") == "true" { "Debug" } else { "" }
release_config(arg) := if matches_mode(arg, "release") == "true" { "Release" } else { "" }
relwithdebinfo_config(arg) := if matches_mode(arg, "relwithdebinfo") == "true" { "RelWithDebInfo" } else { "" }
cmake_config(arg) := debug_config(arg) + release_config(arg) + relwithdebinfo_config(arg)

# Get the build directory for a release mode.
build_dir(arg) := if cmake_config(arg) == "" { mode(arg) } else if multi_config == "true" { "build" } else { "build-" + mode(arg) }

# Get either a `CMAKE_BUILD_TYPE` or `--config` for this release mode.
configure_config(arg) := if cmake_config(arg) == "" { "" } else if multi_config == "true" { "" } else { "-DCMAKE_BUILD_TYPE=" + cmake_config(arg) }
build_config(arg) := if cmake_config(arg) == "" { "" } else if multi_config == "true" { "--config " + cmake_config(arg) } else { "" }

# Implement `-v`.
cmake_log(verbose) := if verbose == "-v" { "--log-level=VERBOSE" } else { "" }
build_verbose(verbose) := if verbose == "-v" { "--verbose" } else { "" }

# Set sanitizer flags.
sanitizer(flag) := if flag == "san" { "-DCAT_USE_SANITIZERS=ON" } else if flag == "nosan" { "-DCAT_USE_SANITIZERS=OFF" } else { "" }
# Changing sanitizer settings requires regenerating CMake.
can_skip_configure(flag) := if sanitizer(flag) == "" { "true" } else { "false" }

# Assemble a `cmake --build` command.
build *args:
    @san=""; verbose=""; no_warnings="false"; cxx_flags=""; modes=""; set -- "$@"; \
      while [ $# -gt 0 ]; do \
        case "$1" in \
          san|nosan) san="$1"; shift ;; \
          -v) verbose="$1"; shift ;; \
          -w) no_warnings="true"; shift ;; \
          -*) cxx_flags="${cxx_flags:+$cxx_flags }$1"; shift ;; \
          *) modes="${modes:+$modes }$1"; shift ;; \
        esac; \
      done; \
      if [ -z "$modes" ]; then modes="{{ last_mode }}"; fi; \
      for mode in $modes; do \
        just _build-mode "$mode" "$san" "$verbose" "$no_warnings" "$cxx_flags"; \
      done

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

intermediaries mode=last_mode fmt="no-fmt" verbose="":
    just _intermediaries-mode {{ mode }} {{ fmt }} {{ verbose }}

syntax mode=last_mode verbose="":
    just cmake_target cat-syntax {{ mode }} {{ verbose }}

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
_build-mode mode="release" san="" verbose="" no_warnings="false" cxx_flags="":
    @just {{ if debug_config(mode) == "" { "_noop" } else { "_build-config" } }} debug "{{ san }}" "{{ verbose }}" "{{ no_warnings }}" "{{ cxx_flags }}"
    @just {{ if release_config(mode) == "" { "_noop" } else { "_build-config" } }} release "{{ san }}" "{{ verbose }}" "{{ no_warnings }}" "{{ cxx_flags }}"
    @just {{ if relwithdebinfo_config(mode) == "" { "_noop" } else { "_build-config" } }} relwithdebinfo "{{ san }}" "{{ verbose }}" "{{ no_warnings }}" "{{ cxx_flags }}"
    @just {{ if cmake_config(mode) == "" { "_build-custom" } else { "_noop" } }} {{ mode }} "{{ san }}" "{{ verbose }}" "{{ no_warnings }}" "{{ cxx_flags }}"

[private]
_build-config mode="release" san="" verbose="" no_warnings="false" cxx_flags="":
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
      fi' _ "{{ no_warnings }}" "{{ cxx_flags }}"
    cmake --build {{ build_dir(mode) }} \
      {{ build_config(mode) }} {{ build_verbose(verbose) }}
    @mkdir -p .cache
    @printf '%s\n' "{{ mode(mode) }}" > .cache/cat-build-mode

[private]
_build-custom mode="" san="" verbose="" no_warnings="false" cxx_flags="":
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
      fi' _ "{{ no_warnings }}" "{{ cxx_flags }}"
    cmake --build {{ build_dir(mode) }} {{ build_verbose(verbose) }}

[private]
_clean-mode mode=last_mode verbose="":
    @just {{ if debug_config(mode) == "" { "_noop" } else { "_clean-config" } }} debug {{ verbose }}
    @just {{ if release_config(mode) == "" { "_noop" } else { "_clean-config" } }} release {{ verbose }}
    @just {{ if relwithdebinfo_config(mode) == "" { "_noop" } else { "_clean-config" } }} relwithdebinfo {{ verbose }}
    @just {{ if cmake_config(mode) == "" { "_clean-config" } else { "_noop" } }} {{ mode }} {{ verbose }}

[private]
_clean-config mode=last_mode verbose="":
    @test ! -f {{ build_dir(mode) }}/CMakeCache.txt \
      || cmake --build {{ build_dir(mode) }} {{ build_config(mode) }} \
        --target clean {{ build_verbose(verbose) }}

[private]
_intermediaries-mode mode=last_mode fmt="no-fmt" verbose="":
    @just {{ if debug_config(mode) == "" { "_noop" } else { "_intermediaries-config" } }} debug {{ fmt }} {{ verbose }}
    @just {{ if release_config(mode) == "" { "_noop" } else { "_intermediaries-config" } }} release {{ fmt }} {{ verbose }}
    @just {{ if relwithdebinfo_config(mode) == "" { "_noop" } else { "_intermediaries-config" } }} relwithdebinfo {{ fmt }} {{ verbose }}
    @just {{ if cmake_config(mode) == "" { "_intermediaries-config" } else { "_noop" } }} {{ mode }} {{ fmt }} {{ verbose }}

[private]
_intermediaries-config mode=last_mode fmt="no-fmt" verbose="":
    @test -d {{ build_dir(mode) }} \
      || cmake {{ cmake_log(verbose) }} -S . -B {{ build_dir(mode) }} \
        {{ configure_config(mode) }} -DCAT_FORMAT_INTERMEDIARIES={{ intermediary_format(fmt) }}
    cmake --build {{ build_dir(mode) }} {{ build_config(mode) }} \
      --target cat-intermediaries {{ build_verbose(verbose) }}

cmake_target target mode=last_mode verbose="":
    just _cmake_target_mode {{ target }} {{ mode }} {{ verbose }}

[private]
_cmake_target_mode target mode=last_mode verbose="":
    @just {{ if debug_config(mode) == "" { "_noop" } else { "_cmake_target_config" } }} {{ target }} debug {{ verbose }}
    @just {{ if release_config(mode) == "" { "_noop" } else { "_cmake_target_config" } }} {{ target }} release {{ verbose }}
    @just {{ if relwithdebinfo_config(mode) == "" { "_noop" } else { "_cmake_target_config" } }} {{ target }} relwithdebinfo {{ verbose }}
    @just {{ if cmake_config(mode) == "" { "_cmake_target_config" } else { "_noop" } }} {{ target }} {{ mode }} {{ verbose }}

[private]
_cmake_target_config target mode=last_mode verbose="":
    cmake --build {{ build_dir(mode) }} {{ build_config(mode) }} --target {{ target }} {{ build_verbose(verbose) }}

[private]
_repl mode san="" verbose="" *args:
    @test "{{ can_skip_configure(san) }}" = true \
      && test -x {{ build_dir(mode) }}/clang-repl-libcat \
      || cmake {{ cmake_log(verbose) }} -S . -B {{ build_dir(mode) }} \
        {{ configure_config(mode) }} -DCAT_BUILD_SHARED=ON {{ sanitizer(san) }}
    cmake --build {{ build_dir(mode) }} {{ build_config(mode) }} \
      --target cat-impl-shared {{ build_verbose(verbose) }}
    @set -- "$@"; shift 3; \
      {{ if mode == "" { build_dir(mode) + "/clang-repl-libcat" } else if multi_config == "true" { "CAT_REPL_CONFIG=" + cmake_config(mode) + " ./build/clang-repl-libcat" } else { build_dir(mode) + "/clang-repl-libcat" } }} \
        "$@"
