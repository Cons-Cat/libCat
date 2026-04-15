# This script is executed by `CMakeLists.txt` for the `cat-format` target.
# `CMAKE_ARGV3` is a path to a `clang-format` executable (`CAT_CLANG_FORMAT_PATH`).
# `CMAKE_ARGV4` and onward are source paths to format.

if(CMAKE_ARGC LESS 5)
  message(FATAL_ERROR "cat-format: expected clang-format path and at least one file.")
endif()

set(_clang_format "${CMAKE_ARGV3}")
set(_compare_tmp "${CMAKE_CURRENT_LIST_DIR}/.cat-format-compare.tmp")

set(_i 4)
while(_i LESS CMAKE_ARGC)
  set(_file "${CMAKE_ARGV${_i}}")
  math(EXPR _i "${_i} + 1")

  if(_file STREQUAL "")
    continue()
  endif()

  execute_process(
    COMMAND "${_clang_format}" "${_file}"
    OUTPUT_FILE "${_compare_tmp}"
    RESULT_VARIABLE _fmt_out_result
    ERROR_VARIABLE _fmt_out_stderr)
  if(NOT _fmt_out_result EQUAL 0)
    message(FATAL_ERROR "cat-format: clang-format failed for `${_file}`: ${_fmt_out_stderr}")
  endif()

  execute_process(
    COMMAND "${CMAKE_COMMAND}" -E compare_files "${_file}" "${_compare_tmp}"
    RESULT_VARIABLE _diff
    OUTPUT_QUIET
    ERROR_QUIET)

  if(_diff EQUAL 1)
    message(STATUS "formatted: ${_file}")
    execute_process(
      COMMAND "${_clang_format}" -i "${_file}"
      RESULT_VARIABLE _fmt_in_result
      ERROR_VARIABLE _fmt_in_stderr)
    if(NOT _fmt_in_result EQUAL 0)
      message(FATAL_ERROR "cat-format: clang-format -i failed for `${_file}`: ${_fmt_in_stderr}")
    endif()
  elseif(_diff EQUAL 2)
    message(FATAL_ERROR "cat-format: could not compare `${_file}` (missing file or read error).")
  endif()
endwhile()

file(REMOVE "${_compare_tmp}")
