# This file is flagrantly "vibe-coded". It may not be up to the standards of
# most libCat code.

# `cat-ir` -- per-translation-unit IR fan-out for libCat.
#
# Defines four per-TU outputs (`.ii`, `.bc`, `.ll`, `.s`) for every source
# attached to `cat-impl`, `cat-tests`, or any `cat-examples` exec. Each
# output is a Ninja edge: independent, parallel, and only re-runs when its
# inputs (source, compile flags, function name, ...) change.
#
# Layout per output:
#   build/<lib>/<config>/<basename>[-<func>].<ext>      # impl  (lib = math, runtime, ...)
#   build/tests/<config>/<basename>[-<func>].<ext>
#   build/examples/<config>/<basename>[-<func>].<ext>
#
# Per-TU target names (basenames are unique across libCat):
#   pow-ii pow-bc pow-ll pow-s
# Per-domain meta:                  src-<kind> tests-<kind> examples-<kind>
# Per-meta-of-metas:                full-<kind>            (= src + tests + examples)
#
# `just ir` translates `<sel>-<kind>` requests directly to ninja target names.
#
# This file is the orchestrator: it discovers tools, builds the OBJECT shadow
# libraries, enumerates sources, and dispatches each (domain, source) pair to
# the per-kind helpers. The actual `add_custom_command` rules live next door:
#   cat_ir_ii.cmake   cat_ir_bc.cmake   cat_ir_ll.cmake   cat_ir_s.cmake
#
# Cache-var inputs (set by `just ir`, configure-time):
#   CAT_IR_FUNCTION             optional literal symbol name (e.g.
#                               `cat::pow`). Every output is narrowed to
#                               functions whose demangled name is shaped
#                               like `<FN>` followed by overload args
#                               `(...)`, template params `<...>`, or an
#                               LTO clone suffix (`.cold` / `.0` / ...).
#                               `.ii` slice via cat_ir_filter.py; `.bc` /
#                               `.ll` via cat_ir_extract.cmake (which
#                               feeds matched mangled names to
#                               `llvm-extract --func=`); `.s` via
#                               cat_ir_disasm.cmake (which feeds them to
#                               `llvm-objdump`).
#   CAT_IR_PASS                 LLVM pass pipeline. When set, the `.ll`
#                               output runs through `opt -passes=...`
#                               instead of `llvm-dis`.
#   CAT_IR_S_SYNTAX             `intel` (default) or `att`. Drives
#                               `--x86-asm-syntax=` for `llvm-objdump`.
#   CAT_IR_OBJDUMP_TRAILER      CMake list of extra args appended to every
#                               `llvm-objdump` invocation. Only meaningful
#                               when `s` is requested alone.
#   CAT_IR_FMT                  `ON`/`OFF`. Run `clang-format` over `.ii`
#                               outputs (default `ON`).
#
# LTO behaviour:
#   The shadow OBJECT libraries inherit the project's `-flto=...` setting via
#   `CAT_CXX_FLAGS_INTERNAL`, so under LTO each shadow `.cpp.o` is bitcode.
#   Only the `s` rule branches on this -- `bc`/`ll` always reflect the
#   pre-LTO per-TU bitcode (the optimizer's input), regardless of mode.

cmake_minimum_required(VERSION 3.25)

if (CMAKE_SCRIPT_MODE_FILE)
  message(FATAL_ERROR
    "cat_ir.cmake is module-only; the legacy `cmake -P` driver is gone.")
endif()

# ----------------------------------------------------------------------------
# Tools. `block(PROPAGATE)` keeps every helper variable local; only the
# cache vars (`CAT_LLVM_*_PATH`, `CAT_PYTHON3`) leave the block, since
# they're set via `find_program` / `set(... CACHE ...)`.
# ----------------------------------------------------------------------------
block()
  # Search the LLVM toolchain bundled next to the configured compiler in
  # addition to PATH so a mise-managed clang sysroot resolves correctly.
  # `find_program` caches NOTFOUND, so a pre-fix configure leaves a sticky
  # sentinel that future runs skip past -- drop those before re-searching.
  cmake_path(GET CMAKE_CXX_COMPILER PARENT_PATH _bin)
  foreach(_cv IN ITEMS
          CAT_LLVM_DIS_PATH CAT_LLVM_EXTRACT_PATH
          CAT_LLVM_OBJDUMP_PATH CAT_LLVM_NM_PATH
          CAT_LLVM_CXXFILT_PATH CAT_OPT_PATH)
    if (DEFINED ${_cv} AND ${_cv} MATCHES "-NOTFOUND$")
      unset(${_cv} CACHE)
    endif()
  endforeach()

  find_program(CAT_LLVM_DIS_PATH     NAMES llvm-dis
    HINTS "${_bin}" DOC "`llvm-dis` matching the configured Clang.")
  find_program(CAT_LLVM_EXTRACT_PATH NAMES llvm-extract
    HINTS "${_bin}" DOC "`llvm-extract` matching the configured Clang.")
  find_program(CAT_LLVM_OBJDUMP_PATH NAMES llvm-objdump
    HINTS "${_bin}" DOC "`llvm-objdump` matching the configured Clang.")
  find_program(CAT_LLVM_NM_PATH      NAMES llvm-nm
    HINTS "${_bin}" DOC "`llvm-nm` matching the configured Clang.")
  find_program(CAT_LLVM_CXXFILT_PATH NAMES llvm-cxxfilt
    HINTS "${_bin}" DOC "`llvm-cxxfilt` matching the configured Clang.")
  find_program(CAT_OPT_PATH          NAMES opt
    HINTS "${_bin}" DOC "`opt` matching the configured Clang.")
endblock()

if (NOT CAT_PYTHON3)
  find_program(CAT_PYTHON3 NAMES python3 DOC "Python 3 for libCat helper scripts.")
endif()

set(_cat_ir_filter_script      "${CMAKE_SOURCE_DIR}/scripts/cat_ir_filter.py")
set(_cat_ir_match_cmake        "${CMAKE_SOURCE_DIR}/scripts/cat_ir_match.cmake")
set(_cat_ir_disasm_cmake       "${CMAKE_SOURCE_DIR}/scripts/cat_ir_disasm.cmake")
set(_cat_ir_extract_cmake      "${CMAKE_SOURCE_DIR}/scripts/cat_ir_extract.cmake")
set(_cat_ir_normalize_cmake    "${CMAKE_SOURCE_DIR}/scripts/cat_ir_normalize.cmake")
set(_cat_ir_fn_validate_cmake  "${CMAKE_SOURCE_DIR}/scripts/cat_ir_fn_validate.cmake")

block()
  foreach(_required IN ITEMS
          CAT_LLVM_DIS_PATH CAT_LLVM_OBJDUMP_PATH CAT_LLVM_NM_PATH
          CAT_LLVM_CXXFILT_PATH CAT_LLVM_EXTRACT_PATH CAT_PYTHON3)
    if (NOT ${_required})
      message(FATAL_ERROR
        "cat-ir: required tool not found: ${_required}. "
        "Make sure the LLVM toolchain (and python3) is on PATH or next to "
        "${CMAKE_CXX_COMPILER}.")
    endif()
  endforeach()
  foreach(_script IN ITEMS
          _cat_ir_filter_script _cat_ir_match_cmake
          _cat_ir_disasm_cmake _cat_ir_extract_cmake
          _cat_ir_normalize_cmake _cat_ir_fn_validate_cmake)
    if (NOT EXISTS "${${_script}}")
      message(FATAL_ERROR
        "cat-ir: missing helper script: ${${_script}}")
    endif()
  endforeach()
endblock()

# ----------------------------------------------------------------------------
# Cache-var inputs (the justfile pokes these on every `just ir` invocation).
# ----------------------------------------------------------------------------

set(CAT_IR_FUNCTION ""
  CACHE STRING "Literal symbol name (e.g. `cat::pow`) that narrows every cat-ir output to matching functions.")
set(CAT_IR_PASS ""
  CACHE STRING "LLVM pass pipeline. Adds <basename>.passes.ll alongside the regular .ll.")
set(CAT_IR_S_SYNTAX "intel"
  CACHE STRING "x86 assembly syntax for .s output: 'intel' or 'att'.")
set(CAT_IR_OBJDUMP_TRAILER ""
  CACHE STRING "Extra args forwarded to llvm-objdump (CMake list).")
option(CAT_IR_FMT "Run clang-format over .ii outputs." ON)

if (CAT_IR_S_SYNTAX STREQUAL "")
  set(CAT_IR_S_SYNTAX "intel" CACHE STRING
    "x86 assembly syntax for .s output: 'intel' or 'att'." FORCE)
endif()
if (NOT CAT_IR_S_SYNTAX MATCHES "^(intel|att)$")
  message(FATAL_ERROR
    "CAT_IR_S_SYNTAX must be 'intel' or 'att' (got '${CAT_IR_S_SYNTAX}').")
endif()

if (CAT_IR_PASS AND NOT CAT_OPT_PATH)
  message(FATAL_ERROR
    "CAT_IR_PASS=${CAT_IR_PASS} requires `opt`; set CAT_OPT_PATH or place it on PATH.")
endif()

# Function-name suffix: sanitise the literal name to a filename-safe stub.
# `cat::pow` -> `cat__pow`, `pow(int)` -> `pow_int_`. Keeps enough of the
# original to disambiguate without escaping headaches downstream.
set(_cat_ir_func_suffix "")
if (CAT_IR_FUNCTION)
  string(REGEX REPLACE "[^A-Za-z0-9._-]+" "_" _cat_ir_func_suffix
    "${CAT_IR_FUNCTION}")
  string(REGEX REPLACE "^_+|_+$" "" _cat_ir_func_suffix
    "${_cat_ir_func_suffix}")
  if (_cat_ir_func_suffix)
    set(_cat_ir_func_suffix "-${_cat_ir_func_suffix}")
  endif()
endif()

# ----------------------------------------------------------------------------
# Per-config LTO detection. The shadow inherits the project's flags as-is, so
# whichever configs carry `-flto[=...]` produce bitcode `.cpp.o` (sanitisers
# disable LTO project-wide -- see top-level CMakeLists.txt -- so the check
# folds in automatically). `_cat_ir_any_lto` aggregates: when no active config
# uses LTO we skip the candidate-binary deps entirely, keeping non-LTO `s`
# strictly per-TU incremental.
# ----------------------------------------------------------------------------
block(PROPAGATE
       _cat_ir_any_lto
       _cat_ir_lto_Debug _cat_ir_lto_Release _cat_ir_lto_RelWithDebInfo)
  set(_cat_ir_any_lto FALSE)
  foreach(_cfg IN ITEMS Debug Release RelWithDebInfo)
    string(TOUPPER "${_cfg}" _cfg_upper)
    set(_cat_ir_lto_${_cfg} FALSE)
    foreach(_flag IN LISTS CAT_CXX_FLAGS_${_cfg_upper})
      if (_flag MATCHES "^-flto(=.*)?$")
        set(_cat_ir_lto_${_cfg} TRUE)
        set(_cat_ir_any_lto TRUE)
        break()
      endif()
    endforeach()
  endforeach()
endblock()

# ----------------------------------------------------------------------------
# Per-domain OBJECT shadow libraries.
#
# Each shadow inherits the regular target's flags (via `target_link_libraries
# PRIVATE`), then adds `-save-temps=obj -g` so `.cpp.ii` and `.cpp.bc` land
# next to the `.cpp.o`. PCH is reused from `cat-impl` so the compile cost is
# in the same league as the regular build. Sources are deduplicated across
# domains (impl wins) so `_start.cpp` only appears once.
# ----------------------------------------------------------------------------

# Impl shadow: cat-impl's PRIVATE sources plus cat's INTERFACE_SOURCES (which
# is `_start.cpp`). `target_link_libraries PRIVATE cat` re-applies the
# essentials and drags `_start.cpp` in via INTERFACE_SOURCES, so we don't list
# it twice.
get_property(_cat_ir_impl_sources TARGET cat-impl PROPERTY SOURCES)

# PCH is intentionally NOT reused from cat-impl. With `-save-temps=obj` clang
# replays `-include global_includes.hpp` against the PCH-loaded TU, double-
# defining its template specialisations. Skipping PCH keeps the shadow
# compile correct at the cost of one cold libCat header pass per TU.
add_library(cat-ir-impl-lib OBJECT EXCLUDE_FROM_ALL ${_cat_ir_impl_sources})
target_link_libraries(cat-ir-impl-lib PRIVATE cat)
target_compile_options(cat-ir-impl-lib PRIVATE
  ${CAT_CXX_FLAGS_INTERNAL}
  -save-temps=obj
  -Wa,--no-warn)

# Tests shadow (only when cat-tests has sources attached).
set(_cat_ir_tests_lib "")
if (TARGET cat-tests)
  get_property(_cat_ir_tests_sources TARGET cat-tests PROPERTY INTERFACE_SOURCES)
  if (EXISTS "${CMAKE_SOURCE_DIR}/tests/unit_tests.cpp")
    list(APPEND _cat_ir_tests_sources "${CMAKE_SOURCE_DIR}/tests/unit_tests.cpp")
  endif()
  if (_cat_ir_tests_sources)
    add_library(cat-ir-tests-lib OBJECT EXCLUDE_FROM_ALL ${_cat_ir_tests_sources})
    target_link_libraries(cat-ir-tests-lib PRIVATE cat-tests)
    target_compile_options(cat-ir-tests-lib PRIVATE
      -save-temps=obj
      -Wa,--no-warn)
    set(_cat_ir_tests_lib cat-ir-tests-lib)
  endif()
endif()

# Examples shadow: pull every example exec's direct sources, dedupe. Example
# basenames also drive owner-target lookup further down (so `hello-ii` depends
# on the regular `hello` exec, etc.); the first exec to claim a source wins.
set(_cat_ir_examples_lib "")
block(PROPAGATE _cat_ir_examples_lib
                _cat_ir_examples_sources
                _cat_ir_example_owner_keys
                _cat_ir_example_owner_vals)
  set(_cat_ir_examples_sources "")
  set(_cat_ir_example_owner_keys "")
  set(_cat_ir_example_owner_vals "")
  foreach(_ex IN ITEMS hello echo client server window unixcat dummy)
    if (NOT TARGET ${_ex})
      continue()
    endif()
    get_property(_srcs TARGET ${_ex} PROPERTY SOURCES)
    get_property(_dir  TARGET ${_ex} PROPERTY SOURCE_DIR)
    foreach(_s IN LISTS _srcs)
      if (NOT IS_ABSOLUTE "${_s}")
        set(_s "${_dir}/${_s}")
      endif()
      list(APPEND _cat_ir_examples_sources "${_s}")
      if (NOT _s IN_LIST _cat_ir_example_owner_keys)
        list(APPEND _cat_ir_example_owner_keys "${_s}")
        list(APPEND _cat_ir_example_owner_vals "${_ex}")
      endif()
    endforeach()
  endforeach()
  list(REMOVE_DUPLICATES _cat_ir_examples_sources)
  if (_cat_ir_examples_sources)
    add_library(cat-ir-examples-lib OBJECT EXCLUDE_FROM_ALL
      ${_cat_ir_examples_sources})
    target_link_libraries(cat-ir-examples-lib PRIVATE cat-examples)
    target_compile_options(cat-ir-examples-lib PRIVATE
      -save-temps=obj
      -Wa,--no-warn)
    set(_cat_ir_examples_lib cat-ir-examples-lib)
  endif()
endblock()

# Candidate binaries (paths only, computed as strings so the `s` rule doesn't
# pick up an implicit target dependency on every exec via `$<TARGET_FILE:>`).
# `RUNTIME_OUTPUT_DIRECTORY` is unset on libCat's execs, so CMake's default
# layout `${CMAKE_BINARY_DIR}/<source-subdir>[/<config>]/<output_name>`
# applies, and `unixcat` carries `OUTPUT_NAME cat`.
function(_cat_ir_exec_path tgt out_var)
  get_target_property(_src_dir ${tgt} SOURCE_DIR)
  cmake_path(RELATIVE_PATH _src_dir
             BASE_DIRECTORY "${CMAKE_SOURCE_DIR}" OUTPUT_VARIABLE _rel)
  set(_path "${CMAKE_BINARY_DIR}")
  if (_rel AND NOT _rel STREQUAL ".")
    set(_path "${_path}/${_rel}")
  endif()
  get_property(_multi GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
  if (_multi)
    set(_path "${_path}/$<CONFIG>")
  endif()
  get_target_property(_name ${tgt} OUTPUT_NAME)
  if (NOT _name)
    set(_name "${tgt}")
  endif()
  set(${out_var} "${_path}/${_name}" PARENT_SCOPE)
endfunction()

block(PROPAGATE _cat_ir_all_execs _cat_ir_all_exec_targets)
  set(_cat_ir_all_execs "")
  set(_cat_ir_all_exec_targets "")
  foreach(_ex IN ITEMS unit_tests hello echo client server window unixcat dummy)
    if (TARGET ${_ex})
      _cat_ir_exec_path(${_ex} _path)
      list(APPEND _cat_ir_all_execs "${_path}")
      list(APPEND _cat_ir_all_exec_targets ${_ex})
    endif()
  endforeach()
endblock()

# ----------------------------------------------------------------------------
# Per-domain `fn=` pre-flights (LTO only).
#
# Without these, every per-TU `s` rule independently scans its candidate
# binaries for `fn=`. When the symbol doesn't match (typo, dead code,
# anonymous-namespace name), every rule in the domain emits the same
# FATAL_ERROR -- 90+ for `src`, 33 for `tests`, 6 for `examples` -- and the
# per-TU error happens to mention whichever TU ninja scheduled first. Each
# pre-flight here collapses the domain to a single check: per-TU rules
# depend on the relevant stamp, so a no-match across the whole domain
# fails once with a domain-aware message before any per-TU work runs.
# ----------------------------------------------------------------------------

# Args: <domain> <out-stamp-var>; reads `_bins` from the surrounding scope.
function(_cat_ir_register_fn_preflight domain out_stamp)
  set(_stamp "${CMAKE_BINARY_DIR}/cat-ir-fn/$<CONFIG>/${domain}.stamp")
  add_custom_command(
    OUTPUT "${_stamp}"
    COMMAND "${CMAKE_COMMAND}"
            "-DOBJDUMP=${CAT_LLVM_OBJDUMP_PATH}"
            "-DFN=${CAT_IR_FUNCTION}"
            "-DBINS=${_bins}"
            "-DDOMAIN=${domain}"
            "-DSTAMP=${_stamp}"
            -P "${_cat_ir_fn_validate_cmake}"
    DEPENDS ${_bins}
            "${_cat_ir_fn_validate_cmake}"
            "${_cat_ir_match_cmake}"
    COMMENT "ir: validating fn=${CAT_IR_FUNCTION} against ${domain} candidates"
    VERBATIM)
  set(${out_stamp} "${_stamp}" PARENT_SCOPE)
endfunction()

set(_cat_ir_src_fn_stamp "")
set(_cat_ir_tests_fn_stamp "")
set(_cat_ir_examples_fn_stamp "")
if (CAT_IR_FUNCTION AND _cat_ir_any_lto)
  if (_cat_ir_all_exec_targets)
    set(_bins "${_cat_ir_all_execs}")
    _cat_ir_register_fn_preflight(src _cat_ir_src_fn_stamp)
  endif()
  if (TARGET unit_tests)
    _cat_ir_exec_path(unit_tests _ut_path)
    set(_bins "${_ut_path}")
    _cat_ir_register_fn_preflight(tests _cat_ir_tests_fn_stamp)
  endif()
  set(_ex_paths "")
  foreach(_ex IN ITEMS hello echo client server window unixcat dummy)
    if (TARGET ${_ex})
      _cat_ir_exec_path(${_ex} _p)
      list(APPEND _ex_paths "${_p}")
    endif()
  endforeach()
  if (_ex_paths)
    set(_bins "${_ex_paths}")
    _cat_ir_register_fn_preflight(examples _cat_ir_examples_fn_stamp)
  endif()
endif()

# ----------------------------------------------------------------------------
# Multi-config helpers.
# ----------------------------------------------------------------------------

get_property(_cat_ir_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)

# Compose the obj-dir path for a shadow lib. Multi-config slots a `$<CONFIG>`
# segment in; single-config doesn't. `GENERATOR_IS_MULTI_CONFIG` is a global
# property, so it's re-fetched here rather than relying on parent-scope
# variable inheritance, which is brittle across nested function calls.
function(_cat_ir_obj_dir lib out_var)
  set(_dir "${CMAKE_BINARY_DIR}/CMakeFiles/${lib}.dir")
  get_property(_multi GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
  if (_multi)
    set(_dir "${_dir}/$<CONFIG>")
  endif()
  set(${out_var} "${_dir}" PARENT_SCOPE)
endfunction()

# Compose the destination dir for an output (always per-config so Debug /
# Release artefacts can coexist).
function(_cat_ir_dst_dir subdir out_var)
  set(${out_var} "${CMAKE_BINARY_DIR}/${subdir}/$<CONFIG>" PARENT_SCOPE)
endfunction()

# Map an absolute libCat impl source to its `<lib>` destination subdir.
# `${CATLIB}/<lib>/implementations/foo.cpp` -> `<lib>`. Anything else (the rare
# top-level `src/` source, or future layouts) falls back to `src`.
function(_cat_ir_impl_subdir abs_src out_var)
  set(_subdir "src")
  if ("${abs_src}" MATCHES "/src/libraries/([^/]+)/[^/]+/[^/]+\\.cpp$")
    set(_subdir "${CMAKE_MATCH_1}")
  endif()
  set(${out_var} "${_subdir}" PARENT_SCOPE)
endfunction()

# ----------------------------------------------------------------------------
# Per-kind rule modules. Each defines `_cat_ir_register_<kind>` and pushes the
# resulting target name onto the GLOBAL property
# `_cat_ir_<domain>_targets_<kind>`, which the meta-target loop reads below.
# ----------------------------------------------------------------------------

include("${CMAKE_CURRENT_LIST_DIR}/cat_ir_ii.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/cat_ir_bc.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/cat_ir_ll.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/cat_ir_s.cmake")

# ----------------------------------------------------------------------------
# Per-TU dispatch. Computes the byproduct paths once and hands each kind off
# to its rule module. ii / bc / ll / s all anchor on the per-source `.cpp.o`
# ninja edge that `-save-temps=obj` writes alongside, so depending on the
# `.o` is enough to make ninja schedule just this TU's compile when any IR
# is requested -- the `.ii` / `.bc` byproducts are read at custom-command
# time.
# ----------------------------------------------------------------------------

# Args:
#   domain         "src" | "tests" | "examples".
#   abs_src        absolute path to the .cpp.
#   subdir         destination subdir (lib name for impl, "tests"/"examples" otherwise).
#   shadow_lib     OBJECT lib that owns the compile (cat-ir-impl-lib, ...).
#   owner_target   regular CMake target to depend on so `just ir` from a fresh
#                  clone builds the production artefact too.
function(_cat_ir_register_tu domain abs_src subdir shadow_lib owner_target)
  cmake_path(GET abs_src STEM _basename)
  _cat_ir_obj_dir("${shadow_lib}" _objdir)
  _cat_ir_dst_dir("${subdir}" _dstdir)

  # `<obj-dir>/<rel-source>.cpp.o` is CMake's compile output. clang's
  # `-save-temps=obj` writes its byproducts next to that, but with the source
  # extension stripped (`pow.cpp.o` next to `pow.ii` / `pow.bc`).
  cmake_path(RELATIVE_PATH abs_src
             BASE_DIRECTORY "${CMAKE_SOURCE_DIR}" OUTPUT_VARIABLE _rel)
  cmake_path(REMOVE_EXTENSION _rel LAST_ONLY OUTPUT_VARIABLE _rel_stem)
  set(_obj_o  "${_objdir}/${_rel}.o")
  set(_obj_ii "${_objdir}/${_rel_stem}.ii")
  set(_obj_bc "${_objdir}/${_rel_stem}.bc")

  set(_sfx "${_cat_ir_func_suffix}")

  _cat_ir_register_ii(${domain} ${_basename} "${_dstdir}"
    "${_dstdir}/${_basename}${_sfx}.ii"  "${_obj_o}" "${_obj_ii}"
    "${owner_target}")
  _cat_ir_register_bc(${domain} ${_basename} "${_dstdir}"
    "${_dstdir}/${_basename}${_sfx}.bc"  "${_obj_o}" "${_obj_bc}"
    "${owner_target}")
  _cat_ir_register_ll(${domain} ${_basename} "${_dstdir}"
    "${_dstdir}/${_basename}${_sfx}.ll"  "${_obj_o}" "${_obj_bc}"
    "${owner_target}")
  _cat_ir_register_s(${domain} ${_basename} "${_dstdir}"
    "${_dstdir}/${_basename}${_sfx}.s"   "${_obj_o}" "${_obj_o}"
    "${owner_target}")
endfunction()

# ----------------------------------------------------------------------------
# Enumerate sources per domain, dedupe across domains (impl > tests > examples).
# ----------------------------------------------------------------------------

set(_cat_ir_seen_sources "")

# Resolve the owner exec for an example source via the parallel key/value
# lists built above. Returns "" if the source isn't owned by any example.
function(_cat_ir_example_owner abs_src out_var)
  list(FIND _cat_ir_example_owner_keys "${abs_src}" _idx)
  if (_idx GREATER_EQUAL 0)
    list(GET _cat_ir_example_owner_vals ${_idx} _owner)
    set(${out_var} "${_owner}" PARENT_SCOPE)
  else()
    set(${out_var} "" PARENT_SCOPE)
  endif()
endfunction()

# Source registration. `block()` keeps the loop scratch (_src, _subdir, ...)
# from leaking; the per-domain target accumulators are GLOBAL properties set
# inside each per-kind module, so they're unaffected by scope.
block()
  set(_seen "")

  # Impl.
  foreach(_src IN LISTS _cat_ir_impl_sources)
    if (NOT IS_ABSOLUTE "${_src}")
      set(_src "${CMAKE_SOURCE_DIR}/${_src}")
    endif()
    if (_src IN_LIST _seen)
      continue()
    endif()
    list(APPEND _seen "${_src}")
    _cat_ir_impl_subdir("${_src}" _subdir)
    _cat_ir_register_tu(src "${_src}" "${_subdir}" cat-ir-impl-lib cat-impl)
  endforeach()

  # Pull `_start.cpp` (cat's INTERFACE_SOURCES) into the impl shadow so it
  # gets its own per-TU IR fan-out alongside the rest of cat-impl.
  get_property(_iface TARGET cat PROPERTY INTERFACE_SOURCES)
  foreach(_src IN LISTS _iface)
    # `cat::cat`'s INTERFACE_SOURCES uses generator expressions
    # ($<BUILD_INTERFACE:...> wrappers); strip those to a plain path.
    string(REGEX REPLACE "^\\$<BUILD_INTERFACE:(.*)>$" "\\1" _src "${_src}")
    string(REGEX REPLACE "^\\$<INSTALL_INTERFACE:.*>$" ""    _src "${_src}")
    if (NOT _src OR NOT IS_ABSOLUTE "${_src}")
      continue()
    endif()
    if (_src IN_LIST _seen)
      continue()
    endif()
    if (NOT _src MATCHES "\\.cpp$")
      continue()
    endif()
    list(APPEND _seen "${_src}")
    _cat_ir_impl_subdir("${_src}" _subdir)
    _cat_ir_register_tu(src "${_src}" "${_subdir}" cat-ir-impl-lib cat-impl)
  endforeach()

  # Tests.
  if (_cat_ir_tests_lib)
    get_property(_tests TARGET cat-tests PROPERTY INTERFACE_SOURCES)
    if (EXISTS "${CMAKE_SOURCE_DIR}/tests/unit_tests.cpp")
      list(APPEND _tests "${CMAKE_SOURCE_DIR}/tests/unit_tests.cpp")
    endif()
    foreach(_src IN LISTS _tests)
      if (NOT IS_ABSOLUTE "${_src}")
        set(_src "${CMAKE_SOURCE_DIR}/${_src}")
      endif()
      if (_src IN_LIST _seen)
        continue()
      endif()
      list(APPEND _seen "${_src}")
      _cat_ir_register_tu(tests "${_src}" tests
        "${_cat_ir_tests_lib}" unit_tests)
    endforeach()
  endif()

  # Examples.
  if (_cat_ir_examples_lib)
    foreach(_src IN LISTS _cat_ir_examples_sources)
      if (_src IN_LIST _seen)
        continue()
      endif()
      list(APPEND _seen "${_src}")
      _cat_ir_example_owner("${_src}" _owner)
      _cat_ir_register_tu(examples "${_src}" examples
        "${_cat_ir_examples_lib}" "${_owner}")
    endforeach()
  endif()
endblock()

# ----------------------------------------------------------------------------
# Per-domain meta + `full-<kind>` aggregates.
# ----------------------------------------------------------------------------

block()
  foreach(_kind IN ITEMS ii bc ll s)
    foreach(_dom IN ITEMS src tests examples)
      get_property(_targets GLOBAL PROPERTY _cat_ir_${_dom}_targets_${_kind})
      if (_targets)
        add_custom_target(${_dom}-${_kind} DEPENDS ${_targets})
      else()
        add_custom_target(${_dom}-${_kind})
      endif()
    endforeach()
    add_custom_target(full-${_kind}
      DEPENDS src-${_kind} tests-${_kind} examples-${_kind})
  endforeach()
endblock()
