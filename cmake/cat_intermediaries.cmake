# Pin script-mode invocations (`cmake -P`) to the same policy floor as
# module mode (CMP0057 / `foreach IN ZIP_LISTS`).
cmake_minimum_required(VERSION 3.24)

# `cat-intermediaries` -- recompile every libCat TU (impl, tests, examples)
# with `-save-temps=obj` and copy the resulting `.ii` / `.bc` / `.s` into
# a flattened per-domain layout under `${CMAKE_BINARY_DIR}`:
#
#   simd/is_avx_supported.{ii,bc,s}   # from cat-impl           (`impl`)
#   runtime/_start.{ii,bc,s}
#   tests/test_simd.{ii,bc,s}         # from cat-tests          (`tests`)
#   examples/hello.{ii,bc,s}          # from the example execs  (`examples`)
#   ...
#
# Each copied `.ii` is run through `clang-format -i` at the end of the
# target so the preprocessed source is human-readable.
#
# Dual-mode file (`CMAKE_SCRIPT_MODE_FILE` is the discriminator):
#
# Module mode (`include()`): defines a per-domain OBJECT "shadow" library
# (impl is always present, tests / examples are conditional on their
# interface library existing with sources), plus the `cat-intermediaries`
# custom target. The custom target has no `DEPENDS` -- script mode drives
# every shadow's sub-build itself with `-k 0` so a per-TU compile failure
# can't stop the copy step.
#
# Script mode (`cmake -P`):
#   1. Drives one `cmake --build ... --target <shadow> --target <shadow>`
#      with `-- -k 0` / `-- -k` so the inner build keeps attempting other
#      TUs after the first failure.
#   2. Per non-empty domain: globs the obj dir, applies the flatten rule
#      for that domain, prunes stale destination files (scoped to the
#      top-level dirs we just produced), and copies via
#      `configure_file COPYONLY`.
#   3. Runs `clang-format -i` on every copied `.ii` (sequential; dumb).
#   4. Re-surfaces a non-zero sub-build exit via `FATAL_ERROR`, but only
#      after the copy has ferried whatever Clang wrote before failing.
#
# OBJECT-shadow setup: each shadow links PRIVATE to its domain's interface
# library (cat / cat-tests / cat-examples) to inherit include dirs and
# compile options. The impl shadow re-applies `CAT_COMPILE_OPTIONS_INTERNAL`
# because `cat-impl`'s copy is PRIVATE; the tests / examples domains get
# those flags transitively via `cat-tests-base` / `cat-examples`'s
# INTERFACE. PCH is left off so each `.ii` carries the fully expanded
# header set; `EXCLUDE_FROM_ALL` keeps plain `ninja` off the hook.
#
# Incremental: Ninja skips up-to-date TU compiles; `configure_file COPYONLY`
# (= `copy_if_different`) skips unchanged temps so their destination mtimes
# stay put; `clang-format -i` is idempotent.
#
# Script-mode args (`-D`-passed):
#   CAT_INTER_IMPL_TARGET / CAT_INTER_IMPL_OBJ_DIR         \
#   CAT_INTER_TESTS_TARGET / CAT_INTER_TESTS_OBJ_DIR        > per-domain pairs
#   CAT_INTER_EXAMPLES_TARGET / CAT_INTER_EXAMPLES_OBJ_DIR /  (empty = absent)
#   CAT_INTER_DST_DIR       destination root for the copied temps
#   CAT_INTER_BUILD_DIR     build dir for the inner `cmake --build`
#   CAT_INTER_GENERATOR     `CMAKE_GENERATOR`, picks the keep-going flag
#   CAT_CLANG_FORMAT_PATH   clang-format binary (may be empty -- skipped then)

if (NOT CMAKE_SCRIPT_MODE_FILE)
  # ===== Module mode =======================================================
  set(_cat_intermediaries_script "${CMAKE_CURRENT_LIST_FILE}")

  # ---- impl shadow (always present) --------------------------------------
  get_property(_cat_impl_sources_for_intermediaries TARGET cat-impl PROPERTY SOURCES)
  add_library(cat-intermediaries-lib OBJECT EXCLUDE_FROM_ALL
    ${_cat_impl_sources_for_intermediaries})
  target_link_libraries(cat-intermediaries-lib PRIVATE cat)
  target_compile_options(cat-intermediaries-lib PRIVATE
    ${CAT_COMPILE_OPTIONS_INTERNAL}
    -save-temps=obj)
  unset(_cat_impl_sources_for_intermediaries)

  # ---- tests shadow (iff cat-tests has test sources attached) ------------
  set(_cat_inter_tests_target "")
  set(_cat_inter_tests_obj_dir "")
  if (TARGET cat-tests)
    get_property(_cat_tests_sources_for_intermediaries
      TARGET cat-tests PROPERTY INTERFACE_SOURCES)
    # `unit_tests.cpp` is the main entry point for the test binary; pick
    # it up too when it exists so its `.ii` is available alongside the
    # per-test files.
    if (EXISTS "${CMAKE_SOURCE_DIR}/tests/unit_tests.cpp")
      list(APPEND _cat_tests_sources_for_intermediaries
        "${CMAKE_SOURCE_DIR}/tests/unit_tests.cpp")
    endif()
    if (_cat_tests_sources_for_intermediaries)
      add_library(cat-intermediaries-tests-lib OBJECT EXCLUDE_FROM_ALL
        ${_cat_tests_sources_for_intermediaries})
      # `cat-tests-base` already carries `CAT_COMPILE_OPTIONS_INTERNAL`
      # via INTERFACE, so -- unlike the impl shadow -- we do not need to
      # re-apply them here. `-save-temps=obj` is the only per-shadow add.
      target_link_libraries(cat-intermediaries-tests-lib PRIVATE cat-tests)
      target_compile_options(cat-intermediaries-tests-lib PRIVATE -save-temps=obj)
      set(_cat_inter_tests_target cat-intermediaries-tests-lib)
      set(_cat_inter_tests_obj_dir
        "${CMAKE_BINARY_DIR}/CMakeFiles/cat-intermediaries-tests-lib.dir")
    endif()
    unset(_cat_tests_sources_for_intermediaries)
  endif()

  # ---- examples shadow (iff at least one example executable exists) ------
  set(_cat_inter_examples_target "")
  set(_cat_inter_examples_obj_dir "")
  if (TARGET cat-examples)
    set(_cat_examples_sources_for_intermediaries "")
    # Enumerate the direct sources of each example executable.
    # Absolutize each relative source against the target's SOURCE_DIR so
    # re-attaching to a shadow defined in the root doesn't resolve e.g.
    # `hello.cpp` against `${CMAKE_SOURCE_DIR}/`.
    foreach(_ex IN ITEMS hello echo client server window unixcat dummy)
      if (TARGET ${_ex})
        get_property(_ex_sources TARGET ${_ex} PROPERTY SOURCES)
        get_property(_ex_src_dir TARGET ${_ex} PROPERTY SOURCE_DIR)
        foreach(_s IN LISTS _ex_sources)
          if (IS_ABSOLUTE "${_s}")
            list(APPEND _cat_examples_sources_for_intermediaries "${_s}")
          else()
            list(APPEND _cat_examples_sources_for_intermediaries "${_ex_src_dir}/${_s}")
          endif()
        endforeach()
      endif()
    endforeach()
    list(REMOVE_DUPLICATES _cat_examples_sources_for_intermediaries)
    if (_cat_examples_sources_for_intermediaries)
      add_library(cat-intermediaries-examples-lib OBJECT EXCLUDE_FROM_ALL
        ${_cat_examples_sources_for_intermediaries})
      target_link_libraries(cat-intermediaries-examples-lib PRIVATE cat-examples)
      target_compile_options(cat-intermediaries-examples-lib PRIVATE -save-temps=obj)
      set(_cat_inter_examples_target cat-intermediaries-examples-lib)
      set(_cat_inter_examples_obj_dir
        "${CMAKE_BINARY_DIR}/CMakeFiles/cat-intermediaries-examples-lib.dir")
    endif()
    unset(_cat_examples_sources_for_intermediaries)
  endif()

  add_custom_target(cat-intermediaries
    COMMAND ${CMAKE_COMMAND}
      -DCAT_INTER_IMPL_TARGET=cat-intermediaries-lib
      -DCAT_INTER_IMPL_OBJ_DIR=${CMAKE_BINARY_DIR}/CMakeFiles/cat-intermediaries-lib.dir
      -DCAT_INTER_TESTS_TARGET=${_cat_inter_tests_target}
      -DCAT_INTER_TESTS_OBJ_DIR=${_cat_inter_tests_obj_dir}
      -DCAT_INTER_EXAMPLES_TARGET=${_cat_inter_examples_target}
      -DCAT_INTER_EXAMPLES_OBJ_DIR=${_cat_inter_examples_obj_dir}
      -DCAT_INTER_DST_DIR=${CMAKE_BINARY_DIR}
      -DCAT_INTER_BUILD_DIR=${CMAKE_BINARY_DIR}
      -DCAT_INTER_GENERATOR=${CMAKE_GENERATOR}
      -DCAT_CLANG_FORMAT_PATH=${CAT_CLANG_FORMAT_PATH}
      -P ${_cat_intermediaries_script}
    USES_TERMINAL
    COMMENT "Building cat-intermediaries shadow libs (failures tolerated) + copying .ii / .bc / .s into ${CMAKE_BINARY_DIR}/.")

  unset(_cat_inter_tests_target)
  unset(_cat_inter_tests_obj_dir)
  unset(_cat_inter_examples_target)
  unset(_cat_inter_examples_obj_dir)
  return()
endif()

# ===== Script mode =========================================================

if (NOT CAT_INTER_IMPL_TARGET OR NOT CAT_INTER_IMPL_OBJ_DIR
    OR NOT CAT_INTER_DST_DIR OR NOT CAT_INTER_BUILD_DIR)
  message(FATAL_ERROR
    "cat-intermediaries: missing required -D args "
    "(IMPL_TARGET, IMPL_OBJ_DIR, DST_DIR, BUILD_DIR).")
endif()

# Collect every non-empty shadow target and build them in a single inner
# `cmake --build` so ninja parallelises across domains. `-- -k 0` / `-- -k`
# maximises partial output even if some TU fails.
set(_cat_inter_targets "${CAT_INTER_IMPL_TARGET}")
if (CAT_INTER_TESTS_TARGET)
  list(APPEND _cat_inter_targets "${CAT_INTER_TESTS_TARGET}")
endif()
if (CAT_INTER_EXAMPLES_TARGET)
  list(APPEND _cat_inter_targets "${CAT_INTER_EXAMPLES_TARGET}")
endif()

set(_cat_inter_keep_going_args)
if (CAT_INTER_GENERATOR MATCHES "Ninja")
  list(APPEND _cat_inter_keep_going_args -- -k 0)
elseif (CAT_INTER_GENERATOR MATCHES "Makefiles")
  list(APPEND _cat_inter_keep_going_args -- -k)
endif()

set(_cat_inter_target_cli "")
foreach(_t IN LISTS _cat_inter_targets)
  list(APPEND _cat_inter_target_cli --target "${_t}")
endforeach()

execute_process(
  COMMAND ${CMAKE_COMMAND} --build "${CAT_INTER_BUILD_DIR}"
          ${_cat_inter_target_cli}
          ${_cat_inter_keep_going_args}
  RESULT_VARIABLE _cat_inter_build_rc)

# Per-domain processing: glob obj dir, flatten, prune stale, copy, format.
# Parallel lists drive one iteration per domain. Empty obj_dir entries
# (tests / examples absent) are skipped inside the loop.
set(_cat_inter_doms      "impl"
                         "tests"
                         "examples")
set(_cat_inter_obj_dirs  "${CAT_INTER_IMPL_OBJ_DIR}"
                         "${CAT_INTER_TESTS_OBJ_DIR}"
                         "${CAT_INTER_EXAMPLES_OBJ_DIR}")
# Per-domain "home prefix" regex applied to flattened dst paths. Anything
# matching is what the domain is responsible for; anything NOT matching
# is dropped (handled by another domain). Impl's home is empty == no
# filter, so it catches everything the libCat layout produces. Tests /
# examples drop `_start.cpp` that leaks in via `cat`'s INTERFACE_SOURCES
# -- otherwise their prune would stomp impl's `runtime/*.ii` copies.
set(_cat_inter_home_re   ""
                         "^tests/"
                         "^examples/")

foreach(_dom _obj_dir _home IN ZIP_LISTS
        _cat_inter_doms _cat_inter_obj_dirs _cat_inter_home_re)
  if (NOT _obj_dir OR NOT EXISTS "${_obj_dir}")
    continue()
  endif()

  file(GLOB_RECURSE _cat_inter_src_rel
    RELATIVE "${_obj_dir}"
    "${_obj_dir}/*.ii"
    "${_obj_dir}/*.bc"
    "${_obj_dir}/*.s")
  if (NOT _cat_inter_src_rel)
    continue()
  endif()

  # Flatten rules are anchored to mutually-exclusive path prefixes, so
  # applying all three to every domain is safe:
  #   * `src/libraries/<lib>/implementations/<stem>` -> `<lib>/<stem>`
  #   * `tests/src/<stem>` -> `tests/<stem>`
  #   * `examples/<stem>`  -> `examples/<stem>`  (identity; kept for
  #     symmetry so a future move of the examples dir only needs one
  #     regex update.)
  set(_cat_inter_dst_rel ${_cat_inter_src_rel})
  list(TRANSFORM _cat_inter_dst_rel REPLACE
    "^src/libraries/([^/]+)/implementations/" "\\1/")
  list(TRANSFORM _cat_inter_dst_rel REPLACE "^tests/src/" "tests/")
  list(TRANSFORM _cat_inter_dst_rel REPLACE "^examples/" "examples/")

  # Filter to this domain's home prefix (if any), dropping cross-domain
  # leak-through paths (`runtime/_start.ii` from the tests / examples
  # shadows). Index-aligned with `_cat_inter_src_rel` so the copy loop
  # below still has the right source path for each dst.
  if (_home)
    set(_cat_inter_filtered_src "")
    set(_cat_inter_filtered_dst "")
    foreach(_s _d IN ZIP_LISTS _cat_inter_src_rel _cat_inter_dst_rel)
      if (_d MATCHES "${_home}")
        list(APPEND _cat_inter_filtered_src "${_s}")
        list(APPEND _cat_inter_filtered_dst "${_d}")
      endif()
    endforeach()
    set(_cat_inter_src_rel ${_cat_inter_filtered_src})
    set(_cat_inter_dst_rel ${_cat_inter_filtered_dst})
    unset(_cat_inter_filtered_src)
    unset(_cat_inter_filtered_dst)
    if (NOT _cat_inter_src_rel)
      continue()
    endif()
  endif()

  # Prune stale destination files (TUs removed since the last build),
  # scoped to the top-level dirs we just produced so the scan never
  # reaches into `CMakeFiles/.../source-temps` or unrelated artifacts.
  # Surgical `file(REMOVE)` keeps `COPYONLY`'s "skip if identical" check
  # effective on the next run.
  set(_cat_inter_owned_dirs "")
  foreach(_dst IN LISTS _cat_inter_dst_rel)
    string(REGEX MATCH "^[^/]+" _lib "${_dst}")
    list(APPEND _cat_inter_owned_dirs "${_lib}")
  endforeach()
  list(REMOVE_DUPLICATES _cat_inter_owned_dirs)

  foreach(_lib IN LISTS _cat_inter_owned_dirs)
    if (EXISTS "${CAT_INTER_DST_DIR}/${_lib}")
      file(GLOB_RECURSE _cat_inter_dst_existing
        RELATIVE "${CAT_INTER_DST_DIR}"
        "${CAT_INTER_DST_DIR}/${_lib}/*.ii"
        "${CAT_INTER_DST_DIR}/${_lib}/*.bc"
        "${CAT_INTER_DST_DIR}/${_lib}/*.s")
      foreach(_existing IN LISTS _cat_inter_dst_existing)
        if (NOT _existing IN_LIST _cat_inter_dst_rel)
          file(REMOVE "${CAT_INTER_DST_DIR}/${_existing}")
        endif()
      endforeach()
    endif()
  endforeach()

  foreach(_src_rel _dst_rel IN ZIP_LISTS _cat_inter_src_rel _cat_inter_dst_rel)
    configure_file("${_obj_dir}/${_src_rel}"
                   "${CAT_INTER_DST_DIR}/${_dst_rel}" COPYONLY)
  endforeach()

  list(LENGTH _cat_inter_src_rel _cat_inter_count)
  message(STATUS
    "cat-intermediaries: ${_dom}: copied ${_cat_inter_count} file(s).")

  # Format the copied `.ii` files in place. `.bc` is binary and `.s` is
  # assembly, so neither is clang-formattable. Sequential and dumb;
  # clang-format is idempotent so subsequent runs are still correct,
  # just not free.
  if (CAT_CLANG_FORMAT_PATH)
    set(_cat_inter_ii_count 0)
    foreach(_dst_rel IN LISTS _cat_inter_dst_rel)
      if (_dst_rel MATCHES "\\.ii$")
        execute_process(
          COMMAND "${CAT_CLANG_FORMAT_PATH}" -i "${CAT_INTER_DST_DIR}/${_dst_rel}"
          RESULT_VARIABLE _cat_inter_fmt_rc)
        if (NOT _cat_inter_fmt_rc EQUAL 0)
          message(WARNING
            "cat-intermediaries: clang-format failed on `${_dst_rel}` (exit ${_cat_inter_fmt_rc})")
        endif()
        math(EXPR _cat_inter_ii_count "${_cat_inter_ii_count} + 1")
      endif()
    endforeach()
    message(STATUS
      "cat-intermediaries: ${_dom}: formatted ${_cat_inter_ii_count} .ii file(s).")
  endif()
endforeach()

# Re-surface the captured sub-build exit. The copy already ran, so the
# user gets both the compile errors (streamed via USES_TERMINAL) and
# access to whatever partial `.ii` / `.bc` / `.s` Clang wrote before
# failing.
if (NOT _cat_inter_build_rc EQUAL 0)
  message(FATAL_ERROR
    "cat-intermediaries shadow lib(s) build returned exit ${_cat_inter_build_rc}; "
    "any partial .ii / .bc / .s output was still copied into "
    "${CAT_INTER_DST_DIR}/.")
endif()
