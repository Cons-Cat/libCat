cmake_minimum_required(VERSION 3.20)

# TODO: Make these apply to specific targets, not globally.
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

project(cat LANGUAGES CXX)

option(CAT_USE_SANITIZERS "Link ASan and UBSan" ON)

list(
  APPEND CAT_FLAGS_COMMON
  # Disable linking libC symbols.
  -nostdlib
  -Wno-builtin-declaration-mismatch
  # These are pessimizations to libCat:
  -fno-exceptions -fno-rtti -fno-unwind-tables -fno-asynchronous-unwind-tables
  # `global_includes.hpp` must be available everywhere.
  -include global_includes.hpp
  # Enable CPU intrinsics:
  -msse4.2
  -mavx2
  -mfma
  -mlzcnt
  -mfsgsbase
  # These flags cause clang-tidy 12 to crash:
  -fconcepts-diagnostics-depth=2
  -fmax-errors=4
  -fdiagnostics-color
  # Enable most warnings.
  -Wall -Wextra
  -Wno-unused-function
)

if(CAT_USE_SANITIZERS)
  list(
    APPEND CAT_FLAGS_COMMON
    -fno-omit-frame-pointer # Required for UBsan.
    -fsanitize=undefined
    -fsanitize=address
    # Asan stack overflow checking does not work in libCat.
    # TODO: Explain why.
    --param asan-stack=0
  )
endif()

list(
  APPEND CAT_FLAGS_DEBUG
  -ggdb3 -pipe
)

list(
  APPEND CAT_FLAGS_RELEASE
  -O3
  # Remove unused symbols.
  -ffunction-sections -fdata-sections
  -fvisibility=hidden -fvisibility-inlines-hidden
  -flto -fwhole-file -fno-plt
)

list(
  APPEND CAT_FLAGS_RELWITHDEBINFO
  ${CAT_FLAGS_DEBUG}
  ${CAT_FLAGS_RELEASE}
)

list(
  APPEND CAT_COMPILE_FLAGS
  ${CAT_FLAGS_COMMON}
  $<$<CONFIG:Debug>:${CAT_FLAGS_DEBUG}>
  $<$<CONFIG:Release>:${CAT_FLAGS_RELEASE}>
  $<$<CONFIG:RelWithDebInfo>:${CAT_FLAGS_RELWITHDEBINFO}>
)

list(
  APPEND CAT_COMMON_LINK_FLAGS
  # This is required to prevent linking 4 kibibytes of unnecessary symbols.
  -nostdlib
)

list(
  APPEND CAT_RELEASE_LINK_FLAGS
  ${CAT_COMMON_LINK_FLAGS}
  # Remove unused symbols.
  -Wl,-z,noseparate-code,--gc-sections
)

list(
  APPEND CAT_LINK_FLAGS
  $<$<CONFIG:Debug>:${CAT_COMMON_LINK_FLAGS}>
  $<$<CONFIG:Release>:${CAT_RELEASE_LINK_FLAGS}>
  $<$<CONFIG:RelWithDebInfo>:${CAT_RELEASE_LINK_FLAGS}>
)

add_library(cat INTERFACE)
set_target_properties(
  cat
  PROPERTIES
  OUTPUT_NAME "cat"
)

# `src/CMakeLists.txt` holds the recipes for `cat-cpp` and `cat-include`.
add_subdirectory(src/)

target_sources(cat INTERFACE ${cat-cpp})
target_include_directories(cat INTERFACE ${cat-include})

# This is required to link `hello` with lld or mold, but not ld.
# This is also required to use SIMD intrinsics, even with ld.
# This adds superfluous code to Hello World.
target_link_libraries(cat INTERFACE gcc_s)

# Link the address and undefined behavior sanitizers in debug builds.
if(CAT_USE_SANITIZERS)
  target_link_libraries(
    cat INTERFACE
    $<$<CONFIG:Debug>:asan>
    $<$<CONFIG:Debug>:ubsan>
    $<$<CONFIG:Release>:asan>
    $<$<CONFIG:Release>:ubsan>
    $<$<CONFIG:RelWithDebInfo>:asan>
    $<$<CONFIG:RelWithDebInfo>:ubsan>
  )
endif()

target_compile_options(cat INTERFACE ${CAT_COMPILE_FLAGS})
target_link_options(cat INTERFACE ${CAT_LINK_FLAGS})

# Build the tests.
enable_testing()
add_subdirectory(tests/)

# Build the examples.
add_subdirectory(examples/)
