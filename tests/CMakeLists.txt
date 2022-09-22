# Link `.gdbinit` to build directories.
add_custom_target(
  cat-gdb-tests ALL
  COMMAND ${CMAKE_COMMAND} -E create_symlink
  # This requires CMake 3.20:
  ${PROJECT_SOURCE_DIR}/.gdbinit ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/.gdbinit
  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>
)
add_dependencies(cat cat-gdb-tests)

add_library(cat-tests INTERFACE)
target_link_libraries(cat-tests INTERFACE cat)

list(
  APPEND
  CAT_COMPILE_FLAGS_TEST
  ${CAT_COMPILE_FLAGS}
  # Tests should fail if they produce sanitizer warnings.
  -fno-sanitize-recover=all
  # Tests should be allowed to have unused variables.
  -Wno-unused-variable
)

# Toggle to disable compiling unit tests.
option(CAT_BUILD_UNIT_TESTS "Compile all unit tests." ON)

if(CAT_BUILD_UNIT_TESTS)
  # Include `tests/unit_tests.hpp`.
  target_include_directories(cat-tests INTERFACE ${CMAKE_SOURCE_DIR}/tests/)

  # Add unit test implementation files.
  target_sources(
    cat-tests INTERFACE
    # ${CMAKE_SOURCE_DIR}/tests/src/test_alloc.cpp
    # ${CMAKE_SOURCE_DIR}/tests/src/test_arrays.cpp
    # ${CMAKE_SOURCE_DIR}/tests/src/test_compare_strings.cpp
    # ${CMAKE_SOURCE_DIR}/tests/src/test_format_strings.cpp
    # ${CMAKE_SOURCE_DIR}/tests/src/test_linear_allocator.cpp
	
    # ${CMAKE_SOURCE_DIR}/tests/src/test_list.cpp
	
    # ${CMAKE_SOURCE_DIR}/tests/src/test_meta.cpp
    # ${CMAKE_SOURCE_DIR}/tests/src/test_numerals.cpp
    # ${CMAKE_SOURCE_DIR}/tests/src/test_optional.cpp
    # ${CMAKE_SOURCE_DIR}/tests/src/test_paging_memory.cpp
    # ${CMAKE_SOURCE_DIR}/tests/src/test_raii.cpp
    # ${CMAKE_SOURCE_DIR}/tests/src/test_reflection.cpp
    # ${CMAKE_SOURCE_DIR}/tests/src/test_scaredy.cpp
    # ${CMAKE_SOURCE_DIR}/tests/src/test_set_memory.cpp
    # ${CMAKE_SOURCE_DIR}/tests/src/test_simd.cpp
    # ${CMAKE_SOURCE_DIR}/tests/src/test_sockets.cpp
    # ${CMAKE_SOURCE_DIR}/tests/src/test_thread.cpp
    # ${CMAKE_SOURCE_DIR}/tests/src/test_tuple.cpp
    # ${CMAKE_SOURCE_DIR}/tests/src/test_variant.cpp
    # ${CMAKE_SOURCE_DIR}/tests/src/test_vector.cpp
  )

  add_executable(unit_tests unit_tests.cpp)
  # `ctor.ld` is required to set up tests as constructor functions.
  set_target_properties(
    unit_tests PROPERTIES
    LINK_DEPENDS ${CMAKE_SOURCE_DIR}/tests/ctor.ld
  )
  target_compile_options(unit_tests PRIVATE ${CAT_COMPILE_FLAGS})
  target_compile_definitions(unit_tests PRIVATE "NO_ARGC_ARGV" "INLINE_MAIN")
  target_link_libraries(unit_tests PRIVATE cat-tests)
  target_link_options(unit_tests PRIVATE ${CAT_LINK_FLAGS})

  # Enable running `unit_tests` through `ctest`.
  add_test(NAME UnitTests COMMAND unit_tests)
else()
  # Build something so that symlinking `.gdbinit` does not fail.
  add_executable(test_dummy ${CMAKE_SOURCE_DIR}/tests/src/test_dummy.cpp)
endif()
