# Link `.gdbinit` to build directories.

# `Ninja Multi-Config` generator:
if(CMAKE_CONFIGURATION_TYPES)
  add_custom_target(
    cat-gdb-tests ALL
    COMMAND ${CMAKE_COMMAND} -E create_symlink
    ${PROJECT_SOURCE_DIR}/.gdbinit ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/.gdbinit
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>
  )
  add_dependencies(cat cat-gdb-tests)
endif() # For `Ninja` generator, this is done by `cat-gdb-examples`.

add_library(cat-tests INTERFACE) # Do not produce a `.so` or `.a` artifact.
target_link_libraries(cat-tests INTERFACE cat)

list(
  APPEND
  CAT_COMPILE_OPTIONS_TEST
  ${CAT_COMPILE_OPTIONS}

  # Tests should fail if they produce sanitizer warnings.
  # TODO: Re-enable this when threads consistently work.
  # -fno-sanitize-recover=all
  # Tests should be allowed to have unused variables.
  -Wno-unused-variable
  $<$<CXX_COMPILER_ID:Clang>:-Wno-self-assign-overloaded>
)

# Toggle to disable compiling unit tests.
option(CAT_BUILD_UNIT_TESTS "Compile all unit tests." ON)

if(CAT_BUILD_UNIT_TESTS)
  # Include `tests/unit_tests.hpp`.
  target_include_directories(cat-tests INTERFACE ${CMAKE_SOURCE_DIR}/tests/)

  # Add unit test implementation files.
  # Specifying each source file individually makes disabling tests convenient.
  target_sources(
    cat-tests INTERFACE
	${CMAKE_SOURCE_DIR}/tests/src/test_meta.cpp
    ${CMAKE_SOURCE_DIR}/tests/src/test_arithmetic.cpp
    #${CMAKE_SOURCE_DIR}/tests/src/test_alloc.cpp
    ${CMAKE_SOURCE_DIR}/tests/src/test_arrays.cpp
    ${CMAKE_SOURCE_DIR}/tests/src/test_compare_strings.cpp
    ${CMAKE_SOURCE_DIR}/tests/src/test_string_length.cpp
    ${CMAKE_SOURCE_DIR}/tests/src/test_format_strings.cpp
    ${CMAKE_SOURCE_DIR}/tests/src/test_linear_allocator.cpp
    ${CMAKE_SOURCE_DIR}/tests/src/test_pool_allocator.cpp
    ${CMAKE_SOURCE_DIR}/tests/src/test_list.cpp
    ${CMAKE_SOURCE_DIR}/tests/src/test_math.cpp
    ${CMAKE_SOURCE_DIR}/tests/src/test_maybe.cpp
    ${CMAKE_SOURCE_DIR}/tests/src/test_paging_memory.cpp
    # ${CMAKE_SOURCE_DIR}/tests/src/test_raii.cpp
    ${CMAKE_SOURCE_DIR}/tests/src/test_typelist.cpp
    ${CMAKE_SOURCE_DIR}/tests/src/test_scaredy.cpp
    ${CMAKE_SOURCE_DIR}/tests/src/test_set_memory.cpp
    ${CMAKE_SOURCE_DIR}/tests/src/test_simd.cpp
    ${CMAKE_SOURCE_DIR}/tests/src/test_tuple.cpp
    ${CMAKE_SOURCE_DIR}/tests/src/test_variant.cpp
    ${CMAKE_SOURCE_DIR}/tests/src/test_vec.cpp
    # ${CMAKE_SOURCE_DIR}/tests/src/test_ring.cpp
    ${CMAKE_SOURCE_DIR}/tests/src/test_invoke.cpp
    ${CMAKE_SOURCE_DIR}/tests/src/test_cast.cpp
    ${CMAKE_SOURCE_DIR}/tests/src/test_bit.cpp
    ${CMAKE_SOURCE_DIR}/tests/src/test_bitset.cpp
    ${CMAKE_SOURCE_DIR}/tests/src/test_stringify.cpp
    ${CMAKE_SOURCE_DIR}/tests/src/test_cpuid.cpp
    ${CMAKE_SOURCE_DIR}/tests/src/test_thread.cpp
  )

  add_executable(unit_tests unit_tests.cpp)
  
  target_compile_options(unit_tests PRIVATE ${CAT_COMPILE_OPTIONS_TEST})
  target_compile_definitions(unit_tests PRIVATE "NO_ARGC_ARGV")
  target_link_libraries(unit_tests PRIVATE cat-tests)
  target_link_options(unit_tests PRIVATE ${CAT_LINK_OPTIONS})

  # Enable running `unit_tests` through `ctest`.
  add_test(NAME UnitTests COMMAND unit_tests)
else()
  # Build something so that symlinking `.gdbinit` does not fail.
  add_executable(test_dummy ${CMAKE_SOURCE_DIR}/tests/src/test_dummy.cpp)
endif()
