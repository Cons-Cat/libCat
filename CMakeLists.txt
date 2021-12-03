cmake_minimum_required(VERSION 3.21)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)
project(kitten LANGUAGES CXX ASM)

set(CMAKE_CXX_FLAGS
  "-nostdlib -fno-exceptions -fno-rtti -fno-unwind-tables \
   -Wl,-e_start -Wl,-e_exit -Wl,-emain \
   -fno-asynchronous-unwind-tables -fno-stack-protector \
   -no-pie \
   -include cstdint -include start_exit.hpp -include result.hpp")
  
set(CMAKE_CXX_FLAGS_DEBUG
  "-O0 -ggdb3 \
    -fno-omit-frame-pointer \
   -fsanitize=undefined \
   -fsanitize=address \
   -fsanitize=leak \
   -Wall -Wextra")

# TODO: Move semantics fail when -static is added in -O3. That should be solved,
# because -static significantly reduces binary size.

# Set aggressive space optimizations. -Os is not favorable, however.
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO
  "-O3 -ggdb3 \
   -flto -fwhole-file -Wl,-z,norelro,-z,noseparate-code,--gc-sections \
   -fsanitize=undefined \
   -fsanitize=address \
   -fsanitize=leak \
   -ffunction-sections -fdata-sections -fomit-frame-pointer \
   -fvisibility=hidden -fvisibility-inlines-hidden")

set(CMAKE_CXX_FLAGS_RELEASE
  "-O3 \
   -flto -fwhole-file -Wl,-z,norelro,-z,noseparate-code,--gc-sections \
   -Wl,-e_start -Wl,-e_exit -Wl,-emain \
   -s -ffunction-sections -fdata-sections -fomit-frame-pointer \
   -fvisibility=hidden -fvisibility-inlines-hidden")

add_library(kitten-std-static STATIC
  src/crt/crt1.S
  )

add_library(kitten-std-headers INTERFACE)
target_include_directories(kitten-std-headers INTERFACE
  src/unistd/
  src/cstdlib/
  src/cstdint/
  src/cstdint/detail/
  src/type_traits/
  src/utility/
  src/start_exit/
  src/result/
  src/pthread/
  src/concepts
  )

# Combine static objects and C++ headers for convenience.
add_library(kitten-std INTERFACE)
target_link_libraries(kitten-std INTERFACE kitten-std-static)
target_link_libraries(kitten-std INTERFACE kitten-std-headers)
target_link_libraries(kitten-std INTERFACE gcc)
# Only link the sanitizers when debugging.
target_link_libraries(kitten-std INTERFACE
  $<$<CONFIG:RelWithDebInfo>:asan>
  $<$<CONFIG:RelWithDebInfo>:ubsan>
  $<$<CONFIG:Debug>:asan>
  $<$<CONFIG:Debug>:ubsan>)

# Build the examples.
add_executable(hello examples/hello.cpp)
target_link_libraries(hello PRIVATE kitten-std)

add_executable(sizeof examples/sizeof.cpp)
target_link_libraries(sizeof PRIVATE kitten-std)

add_executable(exit_with_10 examples/exit_with_10.cpp)
target_link_libraries(exit_with_10 PRIVATE kitten-std)

add_executable(error examples/error.cpp)
target_link_libraries(error PRIVATE kitten-std)

add_executable(safe_arithmetic examples/safe_arithmetic.cpp)
target_link_libraries(safe_arithmetic PRIVATE kitten-std)
