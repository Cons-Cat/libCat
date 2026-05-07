# This file is flagrantly "vibe-coded". It may not be up to the standards of
# most libCat code.

# `cat-ir` ll rule -- textual LLVM IR per TU.
#
# Three paths in priority order (only one is taken):
#   - `pass=<pipeline>` (`CAT_IR_PASS`): run `opt -S -passes=...` over the
#     bitcode byproduct. `fn=` is ignored under `pass=` since narrowing the
#     output makes most pass output meaningless.
#   - `fn=<name>` (`CAT_IR_FUNCTION`): same path as `bc`, but with
#     `OUT_TEXT=ON` so `llvm-extract -S` writes textual IR.
#   - default: `llvm-dis` over the bitcode byproduct.
#
# Every `.ll` is then post-processed by `scripts/cat_ir_normalize.cmake` to
# strip absolute paths and the toolchain version string. The pass is purely
# textual and cheap, so it's unconditional -- ninja still rebuilds only when
# the source bitcode changes, and `.ll`'s reason for existing is to be human-
# readable and diff-stable across machines.
#
# Reads file-scope vars set by `cat_ir.cmake`:
#   CAT_LLVM_DIS_PATH, CAT_LLVM_EXTRACT_PATH, CAT_LLVM_NM_PATH,
#   CAT_LLVM_CXXFILT_PATH, CAT_OPT_PATH, CAT_IR_FUNCTION, CAT_IR_PASS,
#   _cat_ir_extract_cmake, _cat_ir_match_cmake, _cat_ir_normalize_cmake.

function(_cat_ir_register_ll domain basename
                              dst_dir dst
                              compile_anchor obj_bc
                              owner_target)
  set(_cmds COMMAND ${CMAKE_COMMAND} -E make_directory "${dst_dir}")
  set(_extra_deps "${_cat_ir_normalize_cmake}" "${_cat_ir_match_cmake}")
  set(_label "ir ${domain}/${basename}.ll")

  if (CAT_IR_PASS)
    list(APPEND _cmds COMMAND
      "${CAT_OPT_PATH}" -S -passes=${CAT_IR_PASS} "${obj_bc}" -o "${dst}")
    set(_label "ir ${domain}/${basename}.ll (pass=${CAT_IR_PASS})")
  elseif (CAT_IR_FUNCTION)
    list(APPEND _cmds COMMAND "${CMAKE_COMMAND}"
      "-DBC=${obj_bc}"
      "-DOUT=${dst}"
      "-DFN=${CAT_IR_FUNCTION}"
      "-DOUT_TEXT=ON"
      "-DEXTRACT=${CAT_LLVM_EXTRACT_PATH}"
      "-DNM=${CAT_LLVM_NM_PATH}"
      "-DCXXFILT=${CAT_LLVM_CXXFILT_PATH}"
      -P "${_cat_ir_extract_cmake}")
    list(APPEND _extra_deps "${_cat_ir_extract_cmake}")
  else()
    list(APPEND _cmds COMMAND
      "${CAT_LLVM_DIS_PATH}" "${obj_bc}" -o "${dst}")
  endif()

  list(APPEND _cmds COMMAND "${CMAKE_COMMAND}"
    "-DIN=${dst}"
    "-DOUT=${dst}"
    "-DPROJECT_ROOT=${CMAKE_SOURCE_DIR}"
    -P "${_cat_ir_normalize_cmake}")

  add_custom_command(OUTPUT "${dst}"
    ${_cmds}
    DEPENDS "${compile_anchor}" ${_extra_deps}
    COMMENT "${_label}"
    VERBATIM)
  add_custom_target(${basename}-ll DEPENDS "${dst}")
  if (owner_target)
    add_dependencies(${basename}-ll ${owner_target})
  endif()

  set_property(GLOBAL APPEND PROPERTY
    _cat_ir_${domain}_targets_ll ${basename}-ll)
endfunction()
