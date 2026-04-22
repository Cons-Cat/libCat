# Pin script-mode invocations (`cmake -P`) to the same policy floor as
# module mode (CMP0057 / `foreach IN ZIP_LISTS`).
cmake_minimum_required(VERSION 3.24)

# `cat-intermediaries` -- recompile every libCat TU with `-save-temps=obj`
# and copy the resulting `.ii` / `.bc` / `.s` siblings into a flattened
# `<library>/<stem>.<ext>` layout under `${CMAKE_BINARY_DIR}`:
#
#   runtime/_start.ii
#   string/memset.{ii,bc,s}
#   math/sqrt.{ii,bc,s}
#   linux/syscall0.ii
#   ...
#
# Dual-mode file (`CMAKE_SCRIPT_MODE_FILE` is the discriminator):
#
# Module mode (`include()`): defines `cat-intermediaries-lib` (OBJECT
# shadow of `cat-impl`) + the `cat-intermediaries` custom target. The
# custom target deliberately has no `DEPENDS` -- it drives the lib's
# sub-build itself in script mode so a per-TU compile failure can't
# stop the copy step.
#
# Script mode (`cmake -P`):
#   1. Drives `cmake --build ... --target cat-intermediaries-lib` with
#      `-- -k 0` (Ninja) / `-- -k` (Make) so the inner build keeps
#      attempting other TUs after the first failure. Exit code
#      captured, not propagated yet.
#   2. Globs the OBJECT lib's binary dir and copies via
#      `configure_file COPYONLY`. Stale destination files (TUs removed
#      since the last build) are pruned individually, scoped to the
#      library dirs we just produced so the scan never reaches into
#      `CMakeFiles/.../source-temps` or unrelated artifacts.
#   3. Re-surfaces a non-zero sub-build exit via `FATAL_ERROR`, but
#      only after the copy has ferried whatever Clang wrote before
#      failing (a TU that fails at codegen still gets its `.ii` and
#      `.bc` deposited).
#
# OBJECT-shadow setup: links `cat` PRIVATE to inherit include dirs,
# `-include global_includes.hpp`, intrinsics, and `_start.cpp` (via
# `cat`'s `INTERFACE_SOURCES`); re-applies `CAT_COMPILE_OPTIONS_INTERNAL`
# because those are PRIVATE on `cat-impl`; leaves PCH off so each `.ii`
# carries the fully expanded header set; `EXCLUDE_FROM_ALL` keeps a
# plain `ninja` build off the hook.
#
# Glob + copy (rather than enumerating sources at configure time)
# because `_start.cpp` reaches us through `cat`'s `INTERFACE_SOURCES`
# wrapped in `$<BUILD_INTERFACE:...>`, which CMake won't evaluate inside
# a configure-time `foreach`.
#
# Incremental: Ninja skips up-to-date TU compiles; `configure_file
# COPYONLY` (= `copy_if_different`) skips unchanged temps so their
# destination mtimes stay put.
#
# Script-mode args (`-D`-passed):
#   CAT_INTER_OBJ_DIR     OBJECT lib's binary dir
#   CAT_INTER_DST_DIR     destination root for the copied temps
#   CAT_INTER_BUILD_DIR   build dir for the inner `cmake --build`
#   CAT_INTER_LIB_TARGET  target name for the inner `cmake --build`
#   CAT_INTER_GENERATOR   `CMAKE_GENERATOR`, picks the keep-going flag

if (NOT CMAKE_SCRIPT_MODE_FILE)
  # ===== Module mode =======================================================
  set(_cat_intermediaries_script "${CMAKE_CURRENT_LIST_FILE}")
  get_property(_cat_impl_sources_for_intermediaries TARGET cat-impl PROPERTY SOURCES)

  add_library(cat-intermediaries-lib OBJECT EXCLUDE_FROM_ALL
    ${_cat_impl_sources_for_intermediaries})
  target_link_libraries(cat-intermediaries-lib PRIVATE cat)
  target_compile_options(cat-intermediaries-lib PRIVATE
    ${CAT_COMPILE_OPTIONS_INTERNAL}
    -save-temps=obj)

  # No `DEPENDS cat-intermediaries-lib` -- script-mode drives the
  # sub-build itself with `-k 0` so a per-TU compile failure can't
  # stop the copy. `USES_TERMINAL` streams the inner ninja output
  # through to the user.
  add_custom_target(cat-intermediaries
    COMMAND ${CMAKE_COMMAND}
      -DCAT_INTER_OBJ_DIR=${CMAKE_BINARY_DIR}/CMakeFiles/cat-intermediaries-lib.dir
      -DCAT_INTER_DST_DIR=${CMAKE_BINARY_DIR}
      -DCAT_INTER_BUILD_DIR=${CMAKE_BINARY_DIR}
      -DCAT_INTER_LIB_TARGET=cat-intermediaries-lib
      -DCAT_INTER_GENERATOR=${CMAKE_GENERATOR}
      -P ${_cat_intermediaries_script}
    USES_TERMINAL
    COMMENT "Building cat-intermediaries-lib (failures tolerated) + copying .ii / .bc / .s into ${CMAKE_BINARY_DIR}/<library>/.")

  unset(_cat_impl_sources_for_intermediaries)
  return()
endif()

# ===== Script mode =========================================================

if (NOT CAT_INTER_OBJ_DIR OR NOT CAT_INTER_DST_DIR
    OR NOT CAT_INTER_BUILD_DIR OR NOT CAT_INTER_LIB_TARGET)
  message(FATAL_ERROR
    "cat-intermediaries: pass -DCAT_INTER_OBJ_DIR=<obj-dir> "
    "-DCAT_INTER_DST_DIR=<dst-dir> -DCAT_INTER_BUILD_DIR=<build-dir> "
    "-DCAT_INTER_LIB_TARGET=<target>")
endif()

# Drive the sub-build directly so a per-TU failure can't stop the copy
# below. `-- -k 0` / `-- -k` maximises the partial output by attempting
# every TU; exit code captured, re-surfaced at the very end.
set(_cat_inter_keep_going_args)
if (CAT_INTER_GENERATOR MATCHES "Ninja")
  list(APPEND _cat_inter_keep_going_args -- -k 0)
elseif (CAT_INTER_GENERATOR MATCHES "Makefiles")
  list(APPEND _cat_inter_keep_going_args -- -k)
endif()

execute_process(
  COMMAND ${CMAKE_COMMAND} --build "${CAT_INTER_BUILD_DIR}"
          --target "${CAT_INTER_LIB_TARGET}"
          ${_cat_inter_keep_going_args}
  RESULT_VARIABLE _cat_inter_build_rc)

file(GLOB_RECURSE _cat_inter_src_rel
  RELATIVE "${CAT_INTER_OBJ_DIR}"
  "${CAT_INTER_OBJ_DIR}/*.ii"
  "${CAT_INTER_OBJ_DIR}/*.bc"
  "${CAT_INTER_OBJ_DIR}/*.s")

# Flatten "src/libraries/<lib>/implementations/<stem>.<ext>" into
# "<lib>/<stem>.<ext>". Lists stay index-aligned for the ZIP_LISTS copy.
set(_cat_inter_dst_rel ${_cat_inter_src_rel})
list(TRANSFORM _cat_inter_dst_rel REPLACE "^src/libraries/" "")
list(TRANSFORM _cat_inter_dst_rel REPLACE "/implementations/" "/")

# Prune stale destination files (TUs removed since the last build),
# scoped to the library dirs we just produced so the scan never reaches
# the source temps under `CMakeFiles/.../` or unrelated `.{ii,bc,s}`
# elsewhere under the build root. Surgical `file(REMOVE)` (rather than
# a blanket `REMOVE_RECURSE`) keeps `COPYONLY`'s "skip if identical"
# check effective on the next run.
set(_cat_inter_owned_dirs "")
foreach(_dst IN LISTS _cat_inter_dst_rel)
  string(REGEX MATCH "^[^/]+" _lib "${_dst}")
  list(APPEND _cat_inter_owned_dirs "${_lib}")
endforeach()
list(REMOVE_DUPLICATES _cat_inter_owned_dirs)

set(_cat_inter_dst_existing "")
foreach(_lib IN LISTS _cat_inter_owned_dirs)
  if (EXISTS "${CAT_INTER_DST_DIR}/${_lib}")
    file(GLOB_RECURSE _cat_inter_glob
      RELATIVE "${CAT_INTER_DST_DIR}"
      "${CAT_INTER_DST_DIR}/${_lib}/*.ii"
      "${CAT_INTER_DST_DIR}/${_lib}/*.bc"
      "${CAT_INTER_DST_DIR}/${_lib}/*.s")
    list(APPEND _cat_inter_dst_existing ${_cat_inter_glob})
  endif()
endforeach()

foreach(_existing IN LISTS _cat_inter_dst_existing)
  if (NOT _existing IN_LIST _cat_inter_dst_rel)
    file(REMOVE "${CAT_INTER_DST_DIR}/${_existing}")
  endif()
endforeach()

foreach(_src_rel _dst_rel IN ZIP_LISTS _cat_inter_src_rel _cat_inter_dst_rel)
  configure_file("${CAT_INTER_OBJ_DIR}/${_src_rel}"
                 "${CAT_INTER_DST_DIR}/${_dst_rel}" COPYONLY)
endforeach()

list(LENGTH _cat_inter_src_rel _cat_inter_count)
message(STATUS
  "cat-intermediaries: copied ${_cat_inter_count} file(s) into ${CAT_INTER_DST_DIR}/<library>/")

# Re-surface the captured sub-build exit. The copy already ran, so the
# user gets both the compile errors (streamed via USES_TERMINAL) and
# access to whatever partial `.ii` / `.bc` / `.s` Clang wrote before
# failing.
if (NOT _cat_inter_build_rc EQUAL 0)
  message(FATAL_ERROR
    "cat-intermediaries-lib build returned exit ${_cat_inter_build_rc}; "
    "any partial .ii / .bc / .s output was still copied into "
    "${CAT_INTER_DST_DIR}/<library>/.")
endif()
