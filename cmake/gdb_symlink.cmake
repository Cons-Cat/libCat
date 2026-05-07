# This file is flagrantly "vibe-coded". It may not be up to the standards of
# most libCat code.

# `cat_gdb_symlink()` -- keep `.gdbinit` next to the build's binaries.
#
# libCat ships a `.gdbinit` at the project root with pretty-printers and
# debugger shortcuts. `gdb` only loads it automatically when launched from a
# directory containing a copy, so this helper symlinks the source `.gdbinit`
# next to each build's binaries and wires the refresh in as a build-time
# dependency of `cat`.
#
# Usage:
#   cat_gdb_symlink(<target-name> [SINGLE_CONFIG])
#
# `SINGLE_CONFIG` opts into the single-config layout (Ninja, Make), which drops
# `.gdbinit` directly under `${CMAKE_BINARY_DIR}`. Only one subdirectory should
# claim that slot -- in libCat, `examples/` owns it and `tests/` defers via
# `add_dependencies`. Multi-config generators always need a per-subdir copy
# under `${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/`, so that path runs
# unconditionally.

function(cat_gdb_symlink target)
  cmake_parse_arguments(_cat_gdb "SINGLE_CONFIG" "" "" ${ARGN})

  # Multi-config drops `.gdbinit` per `<config>/`; single-config drops it once
  # at `${CMAKE_BINARY_DIR}/.gdbinit`. Bail otherwise -- the caller asked for
  # a single-config symlink under a multi-config layout (or vice versa).
  if (CMAKE_CONFIGURATION_TYPES)
    set(_link "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/.gdbinit")
  elseif (_cat_gdb_SINGLE_CONFIG)
    set(_link "${CMAKE_BINARY_DIR}/.gdbinit")
  else()
    return()
  endif()

  file(REAL_PATH "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../.gdbinit" _source)
  add_custom_command(
    OUTPUT ${_link}
    COMMAND ${CMAKE_COMMAND} -E create_symlink ${_source} ${_link}
    DEPENDS ${_source}
    COMMENT "Updating .gdbinit symlink.")
  add_custom_target(${target} ALL DEPENDS ${_link})
  add_dependencies(cat ${target})
endfunction()
