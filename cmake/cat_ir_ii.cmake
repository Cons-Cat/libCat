# This file is flagrantly "vibe-coded". It may not be up to the standards of
# most libCat code.

# `cat-ir` ii rule -- preprocessed source per TU.
#
# Pipeline (one ninja edge per source):
#   1. copy `<obj-dir>/<stem>.ii` -> `<dst>` (clang's `-save-temps=obj`
#      byproduct).
#   2. strip `# <linenum> <file>` cpp markers via `cat_ir_filter.py`.
#   3. if `fn=`: slice to the lines belonging to that function.
#   4. if `CAT_IR_FMT` (default ON) and clang-format is around: format in place.
#
# Reads file-scope vars set by `cat_ir.cmake`:
#   CAT_PYTHON3, CAT_CLANG_FORMAT_PATH, CAT_IR_FUNCTION, CAT_IR_FMT,
#   _cat_ir_filter_script.

function(_cat_ir_register_ii domain basename
                              dst_dir dst
                              compile_anchor obj_ii
                              owner_target)
  set(_cmds
    COMMAND ${CMAKE_COMMAND} -E make_directory "${dst_dir}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${obj_ii}" "${dst}"
    COMMAND "${CAT_PYTHON3}" "${_cat_ir_filter_script}" strip-cpp-markers
            "${dst}" "${dst}")
  if (CAT_IR_FUNCTION)
    list(APPEND _cmds COMMAND
      "${CAT_PYTHON3}" "${_cat_ir_filter_script}" ii
        "${dst}" "${dst}" "${CAT_IR_FUNCTION}")
  endif()
  if (CAT_IR_FMT AND CAT_CLANG_FORMAT_PATH)
    list(APPEND _cmds COMMAND "${CAT_CLANG_FORMAT_PATH}" -i "${dst}")
  endif()

  add_custom_command(OUTPUT "${dst}"
    ${_cmds}
    DEPENDS "${compile_anchor}" "${_cat_ir_filter_script}"
    COMMENT "ir ${domain}/${basename}.ii"
    VERBATIM)
  add_custom_target(${basename}-ii DEPENDS "${dst}")
  if (owner_target)
    add_dependencies(${basename}-ii ${owner_target})
  endif()

  set_property(GLOBAL APPEND PROPERTY
    _cat_ir_${domain}_targets_ii ${basename}-ii)
endfunction()
