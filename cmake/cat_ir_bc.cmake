# This file is flagrantly "vibe-coded". It may not be up to the standards of
# most libCat code.

# `cat-ir` bc rule -- LLVM bitcode per TU.
#
# Two paths:
#   - no `fn=`: copy the `-save-temps=obj` `<stem>.bc` byproduct unchanged.
#   - `fn=<name>`: hand off to `scripts/cat_ir_extract.cmake`, which translates
#     the literal name to one or more mangled symbols (via `nm | cxxfilt`) and
#     feeds them as `--func=` flags to `llvm-extract`.
#
# Reads file-scope vars set by `cat_ir.cmake`:
#   CAT_LLVM_EXTRACT_PATH, CAT_LLVM_NM_PATH, CAT_LLVM_CXXFILT_PATH,
#   CAT_IR_FUNCTION, _cat_ir_extract_cmake, _cat_ir_match_cmake.

function(_cat_ir_register_bc domain basename
                              dst_dir dst
                              compile_anchor obj_bc
                              owner_target)
  if (CAT_IR_FUNCTION)
    add_custom_command(OUTPUT "${dst}"
      COMMAND ${CMAKE_COMMAND} -E make_directory "${dst_dir}"
      COMMAND "${CMAKE_COMMAND}"
              "-DBC=${obj_bc}"
              "-DOUT=${dst}"
              "-DFN=${CAT_IR_FUNCTION}"
              "-DEXTRACT=${CAT_LLVM_EXTRACT_PATH}"
              "-DNM=${CAT_LLVM_NM_PATH}"
              "-DCXXFILT=${CAT_LLVM_CXXFILT_PATH}"
              -P "${_cat_ir_extract_cmake}"
      DEPENDS "${compile_anchor}" "${_cat_ir_extract_cmake}"
              "${_cat_ir_match_cmake}"
      COMMENT "ir ${domain}/${basename}.bc"
      VERBATIM)
  else()
    add_custom_command(OUTPUT "${dst}"
      COMMAND ${CMAKE_COMMAND} -E make_directory "${dst_dir}"
      COMMAND ${CMAKE_COMMAND} -E copy_if_different "${obj_bc}" "${dst}"
      DEPENDS "${compile_anchor}"
      COMMENT "ir ${domain}/${basename}.bc"
      VERBATIM)
  endif()
  add_custom_target(${basename}-bc DEPENDS "${dst}")
  if (owner_target)
    add_dependencies(${basename}-bc ${owner_target})
  endif()

  set_property(GLOBAL APPEND PROPERTY
    _cat_ir_${domain}_targets_bc ${basename}-bc)
endfunction()
