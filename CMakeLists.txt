cmake_minimum_required(VERSION 3.21)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)
project(libcat LANGUAGES CXX ASM)

set(CMAKE_CONFIGURATION_TYPES Debug;Release;RelWithDebInfo;RelSmall)

# <cstdint> and <start_exit.hpp> are implicitly #included into every libCat program.
list(
  APPEND
  LIBCAT_CXX_FLAGS_COMMON
  -fno-exceptions -fno-rtti -fno-unwind-tables
  -fomit-frame-pointer # This is required for the stack frame to work.
  -march=native
  -fdiagnostics-color
  -include global_includes.hpp
)

list(
  APPEND
  LIBCAT_CXX_FLAGS_DEBUG
  -ggdb3 -pipe
  -fstack-protector-strong
  # -fsanitize=undefined
  # -fsanitize=address
  # -fsanitize=leak
  -Wall -Wextra
  -Wno-unused-function
)

# TODO: Move semantics fail when -static is added in -O3. That should be solved,
# because -static significantly reduces binary size.

# Set aggressive space optimizations. -Os is not favorable, however.
list(
  APPEND
  LIBCAT_CXX_FLAGS_RELEASE
  -no-pie -fno-stack-protector
  -fno-asynchronous-unwind-tables
  -s -fomit-frame-pointer
  -ffunction-sections -fdata-sections
  -fvisibility=hidden -fvisibility-inlines-hidden
  -flto -fwhole-file -fno-plt
  -Wl,-z,norelro,-z,noseparate-code,--gc-sections
)

list(
  APPEND
  LIBCAT_CXX_FLAGS_RELFAST
  ${LIBCAT_CXX_FLAGS_RELEASE}
  -O3
)

list(
  APPEND
  LIBCAT_CXX_FLAGS_RELSMAL
  ${LIBCAT_CXX_FLAGS_RELEASE}L
  -Os
)

list(
  APPEND LIBCAT_CXX_FLAGS
  ${LIBCAT_CXX_FLAGS_COMMON}
  $<$<CONFIG:Debug>:${LIBCAT_CXX_FLAGS_DEBUG}>
  $<$<CONFIG:RelWithDebInfo>:${LIBCAT_CXX_FLAGS_RELWITHDEBINFO}>
  $<$<CONFIG:Release>:${LIBCAT_CXX_FLAGS_RELFAST}>
  $<$<CONFIG:RelSmall>:${LIBCAT_CXX_FLAGS_RELSMALL}>
)

list(
  APPEND
  LIBCAT_COMMON_LINK_FLAGS
  -nostdlib
)

# I can't figure out which flags are for the linker and which
# are for the compiler, so I just send them all to both.
list(
  APPEND
  LIBCAT_RELEASE_LINK_FLAGS
  ${LIBCAT_COMMON_LINK_FLAGS}
  -no-pie -fno-stack-protector
  -fno-asynchronous-unwind-tables
  -fomit-frame-pointer
  -ffunction-sections -fdata-sections
  -fvisibility=hidden -fvisibility-inlines-hidden
  -flto -fwhole-file -fno-plt
  -Wl,-z,norelro,-z,noseparate-code,--gc-sections
)

list(
  APPEND LIBCAT_LINK_FLAGS
  $<$<CONFIG:Debug>:${LIBCAT_COMMON_LINK_FLAGS}>
  $<$<CONFIG:RelWithDebInfo>:${LIBCAT_COMMON_LINK_FLAGS}>
  $<$<CONFIG:Release>:${LIBCAT_RELEASE_LINK_FLAGS}>
  $<$<CONFIG:RelSmall>:${LIBCAT_RELEASE_LINK_FLAGS}>
)

add_library(
  libcat-std-static STATIC
  src/unistd/detail/syscall0.s
  src/unistd/detail/syscall1.s
  src/unistd/detail/syscall2.s
  src/unistd/detail/syscall3.s
  src/unistd/detail/syscall4.s
  src/unistd/detail/syscall5.s
)

add_library(libcat-std-headers INTERFACE)
target_include_directories(
  libcat-std-headers INTERFACE
  src/
  src/unistd/
  src/cstdlib/
  src/cstdint/
  src/cstdint/detail/
  src/cstdarg/
  src/type_traits/
  src/utility/
  src/start_exit/
  src/result/
  src/pthread/
  src/concepts/
  src/any/
  src/maybe/
)

# Combine static objects and C++ headers for convenience.
add_library(libcat-std INTERFACE)
target_link_libraries(libcat-std INTERFACE libcat-std-static)
target_link_libraries(libcat-std INTERFACE libcat-std-headers)
target_link_libraries(libcat-std INTERFACE gcc)
# Only link the sanitizers when debugging.
target_link_libraries(
  libcat-std INTERFACE
  # $<$<CONFIG:RelWithDebInfo>:asan>
  # $<$<CONFIG:RelWithDebInfo>:ubsan>
  # $<$<CONFIG:Debug>:asan>
  # $<$<CONFIG:Debug>:ubsan>
)

# Build the examples.
add_executable(hello examples/hello.cpp)
target_compile_options(hello PRIVATE ${LIBCAT_CXX_FLAGS})
target_link_libraries(hello PRIVATE libcat-std)
target_link_options(hello PRIVATE ${LIBCAT_LINK_FLAGS})

# add_executable(sizeof examples/sizeof.cpp)
# target_compile_options(sizeof PRIVATE ${LIBCAT_CXX_FLAGS})
# target_link_libraries(sizeof PRIVATE libcat-std)
# target_link_options(sizeof PRIVATE ${LIBCAT_LINK_FLAGS})

add_executable(exit_with_10 examples/exit_with_10.cpp)
target_compile_options(exit_with_10 PRIVATE ${LIBCAT_CXX_FLAGS})
target_link_libraries(exit_with_10 PRIVATE libcat-std)
target_link_options(exit_with_10 PRIVATE ${LIBCAT_LINK_FLAGS})

add_executable(error examples/error.cpp)
target_compile_options(error PRIVATE ${LIBCAT_CXX_FLAGS})
target_link_libraries(error PRIVATE libcat-std)
target_link_options(error PRIVATE ${LIBCAT_LINK_FLAGS})

add_executable(safe_arithmetic examples/safe_arithmetic.cpp)
target_compile_options(safe_arithmetic PRIVATE ${LIBCAT_CXX_FLAGS})
target_link_libraries(safe_arithmetic PRIVATE libcat-std)
target_link_options(safe_arithmetic PRIVATE ${LIBCAT_LINK_FLAGS})

add_executable(echo examples/echo.cpp)
target_compile_options(echo PRIVATE ${LIBCAT_CXX_FLAGS})
target_link_libraries(echo PRIVATE libcat-std)
target_link_options(echo PRIVATE ${LIBCAT_LINK_FLAGS})

# This executable is only configured for Release
add_executable(hello_libc examples/hello_libc.cpp)
target_compile_options(
  hello_libc PRIVATE
  -fno-exceptions -fno-rtti -fno-unwind-tables
  -no-pie -fno-stack-protector
  -fno-asynchronous-unwind-tables
  -s -fomit-frame-pointer
  -ffunction-sections -fdata-sections
  -fvisibility=hidden -fvisibility-inlines-hidden
  -flto -fwhole-file -fno-plt
  -Wl,-z,norelro,-z,noseparate-code,--gc-sections
  $<$<CONFIG:Release>:-O3>
  $<$<CONFIG:RelSmall>:-Os>
)
target_link_options(
  hello_libc PRIVATE
  -fno-exceptions -fno-rtti -fno-unwind-tables
  -no-pie -fno-stack-protector
  -fno-asynchronous-unwind-tables
  -s -fomit-frame-pointer
  -ffunction-sections -fdata-sections
  -fvisibility=hidden -fvisibility-inlines-hidden
  -flto -fwhole-file -fno-plt
  -Wl,-z,norelro,-z,noseparate-code,--gc-sections
)
