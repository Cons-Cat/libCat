# `cat_gdb_symlink()` -- keep `.gdbinit` next to the build's binaries
#
# libCat ships a `.gdbinit` at the project root with pretty-printers and
# debugger shortcuts. `gdb` only loads it automatically when launched
# from a directory containing a copy, so this helper symlinks the source
# `.gdbinit` next to each build's binaries and wires the refresh in as a
# build-time dependency of `cat`.
#
# Usage:
#   cat_gdb_symlink(<target-name> [SINGLE_CONFIG])
#
# `SINGLE_CONFIG` opts into the single-config layout (Ninja, Make), which
# drops `.gdbinit` directly under `${CMAKE_BINARY_DIR}`. Only one
# subdirectory should claim that slot -- in libCat, `examples/` owns it
# and `tests/` defers via `add_dependencies`. Multi-config generators
# always need a per-subdir copy under
# `${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/`, so that path runs
# unconditionally.

function(cat_gdb_symlink target)
  cmake_parse_arguments(_cat_gdb "SINGLE_CONFIG" "" "" ${ARGN})

  if (CMAKE_CONFIGURATION_TYPES)
    add_custom_target(
      ${target} ALL
      COMMAND ${CMAKE_COMMAND} -E create_symlink
        ${PROJECT_SOURCE_DIR}/.gdbinit
        ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/.gdbinit
      DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>)
    add_dependencies(cat ${target})
  elseif (_cat_gdb_SINGLE_CONFIG)
    add_custom_target(
      ${target} ALL
      COMMAND ${CMAKE_COMMAND} -E create_symlink
        ${PROJECT_SOURCE_DIR}/.gdbinit
        ${CMAKE_BINARY_DIR}/.gdbinit
      DEPENDS ${CMAKE_BINARY_DIR})
    add_dependencies(cat ${target})
  endif()
endfunction()
