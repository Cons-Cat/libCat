# Link `.gdbinit` to build directories.
add_custom_target(
  test_debug_target ALL
  COMMAND ${CMAKE_COMMAND} -E create_symlink
  # This requires CMake 3.20:
  ${PROJECT_SOURCE_DIR}/.gdbinit $<OUTPUT_CONFIG:$<CONFIG>>/.gdbinit
  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}
)

add_library(cat-test INTERFACE)
target_link_libraries(cat-test INTERFACE cat)
target_include_directories(cat-test INTERFACE)

list(
  APPEND
  CAT_CXX_FLAGS_TEST
  ${CAT_CXX_FLAGS_COMMON}
  $<$<CONFIG:Debug>:${CAT_CXX_FLAGS_DEBUG}>
  $<$<CONFIG:RelWithDebInfo>:${CAT_CXX_FLAGS_RELWITHDEBINFO}>
  $<$<CONFIG:Release>:${CAT_CXX_FLAGS_RELFAST}>
  $<$<CONFIG:RelSmall>:${CAT_CXX_FLAGS_RELSMALL}>
  -Wno-unused-variable
)

# TODO: Get these working:
# add_compile_options(${CAT_CXX_FLAGS})
link_libraries(cat-test)
# add_link_options(${CAT_LINK_FLAGS})

# This tests that `Result<void>` does not panic if it holds `okay`.
add_executable(test_result_ok result_success.cpp)
target_compile_options(test_result_ok PRIVATE ${CAT_CXX_FLAGS_TEST})
target_link_options(test_result_ok PRIVATE ${CAT_LINK_FLAGS})
add_test(NAME TestResultOk COMMAND test_result_ok)

# This tests that `Result<void>` will panic if it holds a `Failure`.
add_executable(test_result_fail result_failure.cpp)
target_compile_options(test_result_fail PRIVATE ${CAT_CXX_FLAGS_TEST})
target_link_options(test_result_fail PRIVATE ${CAT_LINK_FLAGS})
add_test(NAME TestResultFail COMMAND test_result_fail)
set_tests_properties(TestResultFail PROPERTIES WILL_FAIL TRUE)

# This tests if `<array>` works.
add_executable(test_arrays arrays.cpp)
target_compile_options(test_arrays PRIVATE ${CAT_CXX_FLAGS_TEST})
target_link_options(test_arrays PRIVATE ${CAT_LINK_FLAGS})
add_test(NAME TestArray COMMAND test_arrays)

# This tests if `cat::string_length()` works.
add_executable(test_strlen string_length.cpp)
target_compile_options(test_strlen PRIVATE ${CAT_CXX_FLAGS_TEST})
target_link_options(test_strlen PRIVATE ${CAT_LINK_FLAGS})
add_test(NAME TestStrlen COMMAND test_strlen)

# This tests if `cat::compare_strings()` works.
add_executable(test_strcmp compare_strings.cpp)
target_compile_options(test_strcmp PRIVATE ${CAT_CXX_FLAGS_TEST})
target_link_options(test_strcmp PRIVATE ${CAT_LINK_FLAGS})
add_test(NAME TestStrCmp COMMAND test_strcmp)

# This tests if `cat::copy_memory()` works.
add_executable(test_copymem copy_memory.cpp)
target_compile_options(test_copymem PRIVATE ${CAT_CXX_FLAGS_TEST})
target_link_options(test_copymem PRIVATE ${CAT_LINK_FLAGS})
add_test(NAME CopyMem COMMAND test_copymem)

# This tests if `PageAllocator` works.
add_executable(test_paging paging_memory.cpp)
target_compile_options(test_paging PRIVATE ${CAT_CXX_FLAGS_TEST})
target_link_options(test_paging PRIVATE ${CAT_LINK_FLAGS})
add_test(NAME PagingMemory COMMAND test_paging)

# This tests if `Thread`s basically work.
add_executable(test_threads thread.cpp)
target_compile_options(test_threads PRIVATE ${CAT_CXX_FLAGS_TEST})
target_link_options(test_threads PRIVATE ${CAT_LINK_FLAGS})
add_test(NAME MultiThreading COMMAND test_threads)

# This tests if `Raii<>` basically works.
add_executable(test_raii test_raii.cpp)
target_compile_options(test_raii PRIVATE ${CAT_CXX_FLAGS_TEST})
target_link_options(test_raii PRIVATE ${CAT_LINK_FLAGS})
add_test(NAME Raii COMMAND test_raii)

# This tests if `Optional<>` basically works.
add_executable(test_optional optional.cpp)
target_compile_options(test_optional PRIVATE ${CAT_CXX_FLAGS_TEST})
target_link_options(test_optional PRIVATE ${CAT_LINK_FLAGS})
add_test(NAME Maybe COMMAND test_optional)
