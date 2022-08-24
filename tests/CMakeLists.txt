# Link `.gdbinit` to build directories.
add_custom_target(
  cat-unit-tests ALL
  COMMAND ${CMAKE_COMMAND} -E create_symlink
  # This requires CMake 3.20:
  ${PROJECT_SOURCE_DIR}/.gdbinit ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/.gdbinit
  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>
)
add_dependencies(cat cat-unit-tests)

add_library(cat-test INTERFACE)
target_link_libraries(cat-test INTERFACE cat)
target_include_directories(cat-test INTERFACE)

list(
  APPEND
  CAT_CXX_FLAGS_TEST
  $<$<CONFIG:Debug>:${CAT_CXX_FLAGS_DEBUG}>
  $<$<CONFIG:RelWithDebInfo>:${CAT_CXX_FLAGS_RELWITHDEBINFO}>
  $<$<CONFIG:Release>:${CAT_CXX_FLAGS_RELEASE}>
  # Tests should fail if they produce sanitizer warnings.
  -fno-sanitize-recover=all
  # Tests should be allowed to have unused variables.
  -Wno-unused-variable
)

link_libraries(cat-test)

# Enable all unit tests to compile regardless of their individual settings.
option(BUILD_ALL_TESTS "Compile all unit tests unconditionally." ON)

# This tests that `<cat/meta>` works.
option(BUILD_TEST_META "Compile metaprogramming tests." OFF)
if(BUILD_TEST_META OR BUILD_ALL_TESTS)
  add_executable(test_meta test_meta.cpp)
  target_compile_options(test_meta PRIVATE ${CAT_CXX_FLAGS_TEST})
  target_link_options(test_meta PRIVATE ${CAT_LINK_FLAGS})
  add_test(NAME Metaprogramming COMMAND test_meta)
endif()

# This tests that `cat::Numeral` works.
option(BUILD_TEST_NUMERALS "Compile Numerals tests." OFF)
if(BUILD_TEST_NUMERALS OR BUILD_ALL_TESTS)
  add_executable(test_numerals test_numerals.cpp)
  target_compile_options(test_numerals PRIVATE ${CAT_CXX_FLAGS_TEST})
  target_link_options(test_numerals PRIVATE ${CAT_LINK_FLAGS})
  add_test(NAME Numerals COMMAND test_numerals)
endif()

# This tests that `cat::Scaredy` works.
option(BUILD_TEST_SCAREDY "Compile Scaredy tests." OFF)
if(BUILD_TEST_SCAREDY OR BUILD_ALL_TESTS)
  add_executable(test_scaredy test_scaredy.cpp)
  target_compile_options(test_scaredy PRIVATE ${CAT_CXX_FLAGS_TEST})
  target_link_options(test_scaredy PRIVATE ${CAT_LINK_FLAGS})
  add_test(NAME Scaredy COMMAND test_scaredy)
endif()

# This tests that `cat::Array` works.
option(BUILD_TEST_ARRAY "Compile Array tests." OFF)
if(BUILD_TEST_ARRAY OR BUILD_ALL_TESTS)
  add_executable(test_arrays test_arrays.cpp)
  #target_compile_options(test_arrays PRIVATE ${CAT_CXX_FLAGS_TEST})
  target_link_options(test_arrays PRIVATE ${CAT_LINK_FLAGS})
  add_test(NAME Array COMMAND test_arrays)
endif()

# This tests that `cat::List` works.
option(BUILD_TEST_LIST "Compile List tests." OFF)
if(BUILD_TEST_LIST OR BUILD_ALL_TESTS)
  add_executable(test_list test_list.cpp)
  #target_compile_options(test_list PRIVATE ${CAT_CXX_FLAGS_TEST})
  target_link_options(test_list PRIVATE ${CAT_LINK_FLAGS})
  add_test(NAME List COMMAND test_list)
endif()

# This tests that `cat::Vector` works.
option(BUILD_TEST_VECTOR "Compile Tuple tests." OFF)
if(BUILD_TEST_VECTOR OR BUILD_ALL_TESTS)
  add_executable(test_vector test_vector.cpp)
  #target_compile_options(test_vector PRIVATE ${CAT_CXX_FLAGS_TEST})
  target_link_options(test_vector PRIVATE ${CAT_LINK_FLAGS})
  add_test(NAME Vector COMMAND test_vector)
endif()

# This tests that `cat::string_length()` works.
option(BUILD_TEST_STRING_LENGTH "Compile string_length() tests." OFF)
if(BUILD_TEST_STRING_LENGTH OR BUILD_ALL_TESTS)
  add_executable(test_string_length test_string_length.cpp)
  #target_compile_options(test_string_length PRIVATE ${CAT_CXX_FLAGS_TEST})
  target_link_options(test_string_length PRIVATE ${CAT_LINK_FLAGS})
  add_test(NAME StringLength COMMAND test_string_length)
endif()

# This tests that `cat::compare_strings()` works.
option(BUILD_TEST_COMPARE_STRINGS "Compile compare_strings() tests." OFF)
if(BUILD_TEST_COMPARE_STRINGS OR BUILD_ALL_TESTS)
  add_executable(test_compare_strings test_compare_strings.cpp)
  #target_compile_options(test_compare_strings PRIVATE ${CAT_CXX_FLAGS_TEST})
  target_link_options(test_compare_strings PRIVATE ${CAT_LINK_FLAGS})
  add_test(NAME CompareStrings COMMAND test_compare_strings)
endif()

# This tests that string formatting works.
option(BUILD_TEST_FORMAT_STRINGS "Compile string formatting tests." OFF)
if(BUILD_TEST_FORMAT_STRINGS OR BUILD_ALL_TESTS)
  add_executable(test_format_strings test_format_strings.cpp)
  #target_compile_options(test_format_strings PRIVATE ${CAT_CXX_FLAGS_TEST})
  target_compile_definitions(test_format_strings PRIVATE "NO_ARGC_ARGV" "INLINE_MAIN")
  target_link_options(test_format_strings PRIVATE ${CAT_LINK_FLAGS})
  add_test(NAME FormatStrings COMMAND test_format_strings)
endif()

# This tests that `cat::copy_memory()` works.
option(BUILD_TEST_COPY_MEMORY "Compile copy_memory() tests." OFF)
if(BUILD_TEST_COPY_MEMORY OR BUILD_ALL_TESTS)
  add_executable(test_copymem test_copy_memory.cpp)
  #target_compile_options(test_copymem PRIVATE ${CAT_CXX_FLAGS_TEST})
  target_link_options(test_copymem PRIVATE ${CAT_LINK_FLAGS})
  add_test(NAME CopyMemory COMMAND test_copymem)
endif()

# This tests that allocation member functions all compile.
option(BUILD_TEST_ALLOCATOR "Compile PageAllocator tests." OFF)
if(BUILD_TEST_ALLOCATOR OR BUILD_ALL_TESTS)
  add_executable(test_alloc test_alloc.cpp)
  target_compile_options(test_alloc PRIVATE ${CAT_CXX_FLAGS_TEST})
  target_link_options(test_alloc PRIVATE ${CAT_LINK_FLAGS})
  add_test(NAME Allocators COMMAND test_alloc)
endif()

# This tests that `cat::PageAllocator` works.
option(BUILD_TEST_PAGE_ALLOCATOR "Compile PageAllocator tests." OFF)
if(BUILD_TEST_PAGE_ALLOCATOR OR BUILD_ALL_TESTS)
  add_executable(test_paging test_paging_memory.cpp)
  #target_compile_options(test_paging PRIVATE ${CAT_CXX_FLAGS_TEST})
  target_link_options(test_paging PRIVATE ${CAT_LINK_FLAGS})
  add_test(NAME PagingMemory COMMAND test_paging)
endif()

# This tests that `cat::LinearAllocator` works.
option(BUILD_TEST_LINEAR_ALLOCATOR "Compile LinearAllocator tests." OFF)
if(BUILD_TEST_LINEAR_ALLOCATOR OR BUILD_ALL_TESTS)
  add_executable(test_linear test_linear_allocator.cpp)
  #target_compile_options(test_linear PRIVATE ${CAT_CXX_FLAGS_TEST})
  target_link_options(test_linear PRIVATE ${CAT_LINK_FLAGS})
  add_test(NAME LinearAllocator COMMAND test_linear)
endif()

# This tests that `cat::Thread`s works.
option(BUILD_TEST_THREAD "Compile Thread tests." OFF)
if(BUILD_TEST_THREAD OR BUILD_ALL_TESTS)
  add_executable(test_threads test_thread.cpp)
  #target_compile_options(test_threads PRIVATE ${CAT_CXX_FLAGS_TEST})
  target_link_options(test_threads PRIVATE ${CAT_LINK_FLAGS})
  add_test(NAME MultiThreading COMMAND test_threads)
endif()

# This tests that `cat::Unique` works.
option(BUILD_TEST_RAII "Compile ResultOk." OFF)
if(BUILD_TEST_RAII OR BUILD_ALL_TESTS)
  add_executable(test_raii test_raii.cpp)
  #target_compile_options(test_raii PRIVATE ${CAT_CXX_FLAGS_TEST})
  target_link_options(test_raii PRIVATE ${CAT_LINK_FLAGS})
  add_test(NAME Raii COMMAND test_raii)
endif()

# This tests that `cat::Optional<>` works.
option(BUILD_TEST_OPTIONAL "Compile Optional tests." OFF)
if(BUILD_TEST_OPTIONAL OR BUILD_ALL_TESTS)
  add_executable(test_optional test_optional.cpp)
  #target_compile_options(test_optional PRIVATE ${CAT_CXX_FLAGS_TEST})
  target_link_options(test_optional PRIVATE ${CAT_LINK_FLAGS})
  add_test(NAME Optional COMMAND test_optional)
endif()

# This tests that `cat::Simd` works.
option(BUILD_TEST_SIMD "Compile SIMD tests." OFF)
if(BUILD_TEST_SIMD OR BUILD_ALL_TESTS)
  add_executable(test_simd test_simd.cpp)
  #target_compile_options(test_simd PRIVATE ${CAT_CXX_FLAGS_TEST})
  target_link_options(test_simd PRIVATE ${CAT_LINK_FLAGS})
  add_test(NAME Simd COMMAND test_simd)
endif()

# This tests that type reflection works.
option(BUILD_TEST_REFLECTION "Compile type reflection tests." OFF)
if(BUILD_TEST_REFLECTION OR BUILD_ALL_TESTS)
  add_executable(test_reflection test_reflection.cpp)
  #target_compile_options(test_reflection PRIVATE ${CAT_CXX_FLAGS_TEST})
  target_link_options(test_reflection PRIVATE ${CAT_LINK_FLAGS})
  add_test(NAME TypeReflection COMMAND test_reflection)
endif()

# This tests that `cat::Variant` works.
option(BUILD_TEST_VARIANT "Compile type variant tests." OFF)
if(BUILD_TEST_VARIANT OR BUILD_ALL_TESTS)
  add_executable(test_variant test_variant.cpp)
  #target_compile_options(test_variant PRIVATE ${CAT_CXX_FLAGS_TEST})
  target_link_options(test_variant PRIVATE ${CAT_LINK_FLAGS})
  add_test(NAME TypeVariant COMMAND test_variant)
endif()

# This tests that `cat::Tuple` works.
option(BUILD_TEST_TUPLE "Compile Tuple tests." OFF)
if(BUILD_TEST_TUPLE OR BUILD_ALL_TESTS)
  add_executable(test_tuple test_tuple.cpp)
  #target_compile_options(test_tuple PRIVATE ${CAT_CXX_FLAGS_TEST})
  target_link_options(test_tuple PRIVATE ${CAT_LINK_FLAGS})
  add_test(NAME Tuples COMMAND test_tuple)
endif()

# This tests that `cat::set_memory()` etc. work.
option(BUILD_TEST_MEMORY "Compile Tuple tests." OFF)
if(BUILD_TEST_MEMORY OR BUILD_ALL_TESTS)
  add_executable(test_set_memory test_set_memory.cpp)
  #target_compile_options(test_set_memory PRIVATE ${CAT_CXX_FLAGS_TEST})
  target_link_options(test_set_memory PRIVATE ${CAT_LINK_FLAGS})
  add_test(NAME SetMemory COMMAND test_set_memory)
endif()

# A `dummy` project is required to guarantee that the directories are generated.
# The directories must be generated for symlinking `.gdbinit` to succeed.
# This can be skipped if one or more other tests are built.
if(
  NOT (BUILD_ALL_TESTS
  OR BUILD_TEST_META
  OR BUILD_TEST_NUMERALS
  OR BUILD_TEST_SCAREDY
  OR BUILD_TEST_ARRAY
  OR BUILD_TEST_LIST
  OR BUILD_TEST_VECTOR
  OR BUILD_TEST_STRING_LENGTH
  OR BUILD_TEST_COMPARE_STRINGS
  OR BUILD_TEST_FORMAT_STRINGS
  OR BUILD_TEST_COPY_MEMORY
  OR BUILD_TEST_ALLOCATOR
  OR BUILD_TEST_PAGE_ALLOCATOR
  OR BUILD_TEST_LINEAR_ALLOCATOR
  OR BUILD_TEST_THREAD
  OR BUILD_TEST_TEST_RAII
  OR BUILD_TEST_OPTIONAL
  OR BUILD_TEST_SIMD
  OR BUILD_TEST_REFLECTION
  OR BUILD_TEST_VARIANT
  OR BUILD_TEST_TUPLE
  OR BUILD_TEST_MEMORY)
)
  add_executable(test_dummy test_dummy.cpp)
  target_compile_options(test_dummy PRIVATE ${CAT_CXX_FLAGS_TEST})
  target_link_options(test_dummy PRIVATE ${CAT_LINK_FLAGS})
  add_test(NAME dummy COMMAND test_dummy)
endif()
