cmake_minimum_required(VERSION 3.24)

project(cat LANGUAGES CXX)

option(CAT_USE_SANITIZERS "Link ASan and UBSan" ON)

list(
  APPEND CAT_COMPILE_OPTIONS_COMMON
  # Disable linking libC symbols.
  -nostdlib
  # These features are pessimizations to libCat:
  -fno-exceptions -fno-rtti -fno-unwind-tables -fno-asynchronous-unwind-tables
  # `global_includes.hpp` must be available everywhere.
  -include global_includes.hpp
  # Enable CPU intrinsics:
  -msse4.2
  -mavx2
  -mfma
  -mlzcnt
  -mbmi
  -mbmi2
  -mfsgsbase

  # These flags cause clang-tidy to crash:
  # -fconcepts-diagnostics-depth=2
  -fmax-errors=4
  
  # Enable most warnings.
  -Wall -Wextra
  -Wno-unused-function
  -Wno-unknown-pragmas
  # Disable some Clang warnings.
  $<$<CXX_COMPILER_ID:Clang>:-Wno-unknown-attributes>
  $<$<CXX_COMPILER_ID:Clang>:-Wno-inline-new-delete>
  $<$<CXX_COMPILER_ID:Clang>:-Wno-unqualified-std-cast-call>
)

if(CAT_USE_SANITIZERS)
  list(
    APPEND CAT_COMPILE_OPTIONS_COMMON
    -fno-omit-frame-pointer # Required for UBsan.
    -fsanitize=undefined
    -fsanitize=address
    # Asan stack overflow checking does not work in libCat.
    # TODO: Explain why.
    --param asan-stack=0
  )
endif()

list(
  APPEND CAT_COMPILE_OPTIONS_DEBUG
  -ggdb3 -pipe
)

list(
  APPEND CAT_COMPILE_OPTIONS_RELEASE
  # Remove unused symbols.
  -ffunction-sections -fdata-sections
  -fvisibility=hidden -fvisibility-inlines-hidden
  -flto=auto -fwhole-file -fno-plt
)

list(
  APPEND CAT_COMPILE_OPTIONS_RELWITHDEBINFO
  ${CAT_COMPILE_OPTIONS_DEBUG}
  ${CAT_COMPILE_OPTIONS_RELEASE}
)

list(
  APPEND CAT_COMPILE_OPTIONS
  ${CAT_COMPILE_OPTIONS_COMMON}
  $<$<CONFIG:Debug>:${CAT_COMPILE_OPTIONS_DEBUG}>
  $<$<CONFIG:Release>:${CAT_COMPILE_OPTIONS_RELEASE}>
  $<$<CONFIG:RelWithDebInfo>:${CAT_COMPILE_OPTIONS_RELWITHDEBINFO}>
)

list(
  APPEND CAT_LINK_OPTIONS_COMMON
  # This is required to prevent linking 4 kibibytes of unnecessary symbols.
  -nostdlib
)

list(
  APPEND CAT_LINK_OPTIONS_RELEASE
  ${CAT_LINK_OPTIONS_COMMON}
  # Remove unused symbols.
  -Wl,-z,noseparate-code,--gc-sections
)

list(
  APPEND CAT_LINK_OPTIONS
  $<$<CONFIG:Debug>:${CAT_LINK_OPTIONS_COMMON}>
  $<$<CONFIG:Release>:${CAT_LINK_OPTIONS_RELEASE}>
  $<$<CONFIG:RelWithDebInfo>:${CAT_LINK_OPTIONS_RELEASE}>
)

add_library(cat INTERFACE)

# Everything building with `cat` should be at least C++23, with GNU extensions.
target_compile_features(cat INTERFACE cxx_std_23)

# Link the address and undefined behavior sanitizers.
if(CAT_USE_SANITIZERS)
  target_link_libraries(
    cat INTERFACE
    asan
    ubsan
  )
endif()

target_compile_options(cat INTERFACE ${CAT_COMPILE_OPTIONS})
target_link_options(cat INTERFACE ${CAT_LINK_OPTIONS})

# This is required to link `hello` with lld or mold, but not ld.
# This is also required to use SIMD intrinsics, even with ld.
# This adds superfluous code to Hello World.
target_link_libraries(cat INTERFACE gcc_s)

# `src/CMakeLists.txt` adds all of the source and headers files to `cat`.
add_subdirectory(src/)

# Tests can be run using ctest, or by manually running the `unit_tests`
# executable from your specified build output directory.
enable_testing()
# Build the tests.
add_subdirectory(tests/)

# Build the examples.
add_subdirectory(examples/)
