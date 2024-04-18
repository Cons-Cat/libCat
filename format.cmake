# This script is executed by `CMakeLists.txt` for the `cat-format` target.

# `CMAKE_ARGV4` is a space-delimited list of files to format.
string(REPLACE " " ";" files ${CMAKE_ARGV4})
foreach(file IN ITEMS ${files})
    # `CMAKE_ARGV3` is a path to a `clang-format` executable, specified
    # By the `CAT_CLANG_FORMAT_PATH` variable.
    execute_process(
    COMMAND ${CMAKE_ARGV3} -i ${file}
  )
endforeach()
