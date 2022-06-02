# Link `.gdbinit` to build directories.
add_custom_target(
  cat-unit-tests ALL
  COMMAND ${CMAKE_COMMAND} -E create_symlink
  # This requires CMake 3.20:
  ${PROJECT_SOURCE_DIR}/.gdbinit ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/.gdbinit
  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>
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

# Enable all unit tests to compile regardless of their individual settings.
option(BUILD_ALL_TESTS "Compile all unit tests unconditionally." ON)

# A dummy project is required to guarantee that the directories are generated.
# The directories must be generated for symlinking `.gdbinit` to succeed.
add_executable(dummy result_success.cpp)
add_test(NAME dummy COMMAND echo)

# This tests that `Result<void>` does not panic if it holds `okay`.
option(BUILD_TEST_RESULT_OK "Compile TestResultOk." OFF)
if(BUILD_TEST_RESULT_OK OR BUILD_ALL_TESTS)
  add_executable(test_result_ok result_success.cpp)
  target_compile_options(test_result_ok PRIVATE ${CAT_CXX_FLAGS_TEST})
  target_link_options(test_result_ok PRIVATE ${CAT_LINK_FLAGS})
  add_test(NAME TestResultOk COMMAND test_result_ok)
endif()

# This tests that `Result<void>` will panic if it holds a `Failure`.
option(BUILD_TEST_RESULT_FAIL "Compile TestResultOk." OFF)
if(BUILD_TEST_RESULT_FAIL OR BUILD_ALL_TESTS)
  add_executable(test_result_fail result_failure.cpp)
  target_compile_options(test_result_fail PRIVATE ${CAT_CXX_FLAGS_TEST})
  target_link_options(test_result_fail PRIVATE ${CAT_LINK_FLAGS})
  add_test(NAME TestResultFail COMMAND test_result_fail)
  set_tests_properties(TestResultFail PROPERTIES WILL_FAIL TRUE)
endif()

# This tests that `cat::Scaredy` works.
option(BUILD_TEST_SCAREDY "Compile TestResultOk." OFF)
if(BUILD_TEST_SCAREDY OR BUILD_ALL_TESTS)
  add_executable(test_scaredy test_scaredy.cpp)
  target_compile_options(test_scaredy PRIVATE ${CAT_CXX_FLAGS_TEST})
  target_link_options(test_scaredy PRIVATE ${CAT_LINK_FLAGS})
  add_test(NAME TestScaredy COMMAND test_scaredy)
endif()

# This tests that `cat::Array` works.
option(BUILD_TEST_ARRAY "Compile Array tests." OFF)
if(BUILD_TEST_ARRAY OR BUILD_ALL_TESTS)
  add_executable(test_arrays arrays.cpp)
  target_compile_options(test_arrays PRIVATE ${CAT_CXX_FLAGS_TEST})
  target_link_options(test_arrays PRIVATE ${CAT_LINK_FLAGS})
  add_test(NAME TestArray COMMAND test_arrays)
endif()

# This tests that `cat::List` works.
option(BUILD_TEST_LIST "Compile List tests." OFF)
if(BUILD_TEST_LIST OR BUILD_ALL_TESTS)
  add_executable(test_list test_list.cpp)
  target_compile_options(test_list PRIVATE ${CAT_CXX_FLAGS_TEST})
  target_link_options(test_list PRIVATE ${CAT_LINK_FLAGS})
  add_test(NAME TestList COMMAND test_list)
endif()

# This tests that `cat::string_length()` works.
option(BUILD_TEST_STRING_LENGTH "Compile string_length() tests." OFF)
if(BUILD_TEST_STRING_LENGTH OR BUILD_ALL_TESTS)
  add_executable(test_string_length string_length.cpp)
  target_compile_options(test_string_length PRIVATE ${CAT_CXX_FLAGS_TEST})
  target_link_options(test_string_length PRIVATE ${CAT_LINK_FLAGS})
  add_test(NAME TestStrlen COMMAND test_string_length)
endif()

# This tests that `cat::compare_strings()` works.
option(BUILD_TEST_COMPARE_STRINGS "Compile compare_strings() tests." OFF)
if(BUILD_TEST_COMPARE_STRINGS OR BUILD_ALL_TESTS)
  add_executable(test_compare_strings compare_strings.cpp)
  target_compile_options(test_compare_strings PRIVATE ${CAT_CXX_FLAGS_TEST})
  target_link_options(test_compare_strings PRIVATE ${CAT_LINK_FLAGS})
  add_test(NAME TestStrCmp COMMAND test_compare_strings)
endif()

# This tests that `cat::copy_memory()` works.
option(BUILD_TEST_COPY_MEMORY "Compile copy_memory() tests." OFF)
if(BUILD_TEST_COPY_MEMORY OR BUILD_ALL_TESTS)
  add_executable(test_copymem copy_memory.cpp)
  target_compile_options(test_copymem PRIVATE ${CAT_CXX_FLAGS_TEST})
  target_link_options(test_copymem PRIVATE ${CAT_LINK_FLAGS})
  add_test(NAME CopyMem COMMAND test_copymem)
endif()

# This tests that `cat::PageAllocator` works.
option(BUILD_TEST_PAGE_ALLOCATOR "Compile PageAllocator tests." OFF)
if(BUILD_TEST_PAGE_ALLOCATOR OR BUILD_ALL_TESTS)
  add_executable(test_paging paging_memory.cpp)
  target_compile_options(test_paging PRIVATE ${CAT_CXX_FLAGS_TEST})
  target_link_options(test_paging PRIVATE ${CAT_LINK_FLAGS})
  add_test(NAME PagingMemory COMMAND test_paging)
endif()

# This tests that `LinearAllocator` works.
option(BUILD_TEST_LINEAR_ALLOCATOR "Compile LinearAllocator tests." OFF)
if(BUILD_TEST_LINEAR_ALLOCATOR OR BUILD_ALL_TESTS)
  add_executable(test_linear linear_allocator.cpp)
  target_compile_options(test_linear PRIVATE ${CAT_CXX_FLAGS_TEST})
  target_link_options(test_linear PRIVATE ${CAT_LINK_FLAGS})
  add_test(NAME LinearAllocator COMMAND test_linear)
endif()

# This tests that `Thread`s works.
option(BUILD_TEST_THREAD "Compile Thread tests." OFF)
if(BUILD_TEST_THREAD OR BUILD_ALL_TESTS)
  add_executable(test_threads thread.cpp)
  target_compile_options(test_threads PRIVATE ${CAT_CXX_FLAGS_TEST})
  target_link_options(test_threads PRIVATE ${CAT_LINK_FLAGS})
  add_test(NAME MultiThreading COMMAND test_threads)
endif()

# This tests that `Raii<>` works.
option(BUILD_TEST_OPTIONAL "Compile TestResultOk." OFF)
if(BUILD_TEST_OPTIONAL OR BUILD_ALL_TESTS)
  add_executable(test_raii test_raii.cpp)
  target_compile_options(test_raii PRIVATE ${CAT_CXX_FLAGS_TEST})
  target_link_options(test_raii PRIVATE ${CAT_LINK_FLAGS})
  add_test(NAME Raii COMMAND test_raii)
endif()

# This tests that `Optional<>` works.
option(BUILD_TEST_SCAREDY "Compile Optional tests." OFF)
if(BUILD_TEST_SCAREDY OR BUILD_ALL_TESTS)
  add_executable(test_optional optional.cpp)
  target_compile_options(test_optional PRIVATE ${CAT_CXX_FLAGS_TEST})
  target_link_options(test_optional PRIVATE ${CAT_LINK_FLAGS})
  add_test(NAME Optional COMMAND test_optional)
endif()

# This tests that SIMD works.
option(BUILD_TEST_SIMD "Compile SIMD tests." OFF)
if(BUILD_TEST_SIMD OR BUILD_ALL_TESTS)
  add_executable(test_simd simd.cpp)
  target_compile_options(test_simd PRIVATE ${CAT_CXX_FLAGS_TEST})
  target_link_options(test_simd PRIVATE ${CAT_LINK_FLAGS})
  add_test(NAME Simd COMMAND test_simd)
endif()

# This tests that type reflection works.
option(BUILD_TEST_REFLECTION "Compile type reflection tests." OFF)
if(BUILD_TEST_REFLECTION OR BUILD_ALL_TESTS)
  add_executable(test_reflection test_reflection.cpp)
  target_compile_options(test_reflection PRIVATE ${CAT_CXX_FLAGS_TEST})
  target_link_options(test_reflection PRIVATE ${CAT_LINK_FLAGS})
  add_test(NAME TypeReflection COMMAND test_reflection)
endif()

# This tests that tuples work.
option(BUILD_TEST_TUPLE "Compile Tuple tests." OFF)
if(BUILD_TEST_TUPLE OR BUILD_ALL_TESTS)
  add_executable(test_tuple test_tuple.cpp)
  target_compile_options(test_tuple PRIVATE ${CAT_CXX_FLAGS_TEST})
  target_link_options(test_tuple PRIVATE ${CAT_LINK_FLAGS})
  add_test(NAME Tuples COMMAND test_tuple)
endif()
