cmake_minimum_required(VERSION 3.21)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)
project(libcat LANGUAGES CXX ASM)

# <cstdint> and <result.hpp> are implicitly #included into every LibCat program.
set(
  CMAKE_CXX_FLAGS
  "-nostdlib \
   -Wl,-e,_start \
   -include cstdint -include start_exit.hpp \
"
)

#  -fno-exceptions -fno-rtti -fno-unwind-tables \
#   -fno-asynchronous-unwind-tables -fno-stack-protector \

# set(
#   CMAKE_CXX_FLAGS_DEBUG
#   "-O0 -ggdb3 \
#    -Wall -Wextra \
# "
# )

# -fno-omit-frame-pointer \
# -fsanitize=undefined \
# -fsanitize=address \
# -fsanitize=leak \

# TODO: Move semantics fail when -static is added in -O3. That should be solved,
# because -static significantly reduces binary size.

# Set aggressive space optimizations. -Os is not favorable, however.
set(
  CMAKE_CXX_FLAGS_RELWITHDEBINFO
  "-O3 -ggdb3 \
   -flto -fwhole-file -Wl,-z,norelro,-z,noseparate-code,--gc-sections \
   -fnoomit-frame-pointer \
   -fsanitize=undefined \
   -fsanitize=address \
   -fsanitize=leak \
   -ffunction-sections -fdata-sections \
   -fvisibility=hidden -fvisibility-inlines-hidden \
"
)

set(
  CMAKE_CXX_FLAGS_RELEASE
  "-O3 \
   -no-pie \
   -flto -fwhole-file -Wl,-z,norelro,-z,noseparate-code,--gc-sections \
   -s -ffunction-sections -fdata-sections -fomit-frame-pointer \
   -fvisibility=hidden -fvisibility-inlines-hidden \
 "
)

add_library(
  libcat-std-static STATIC
  src/crt/crt1.S
)

add_library(libcat-std-headers INTERFACE)
target_include_directories(
  libcat-std-headers INTERFACE
  src/unistd/
  src/cstdlib/
  src/cstdint/
  src/cstdint/detail/
  src/type_traits/
  src/utility/
  src/start_exit/
  src/result/
  src/pthread/
  src/concepts/
  src/any/
)

# Combine static objects and C++ headers for convenience.
add_library(libcat-std INTERFACE)
target_link_libraries(libcat-std INTERFACE libcat-std-static)
target_link_libraries(libcat-std INTERFACE libcat-std-headers)
# target_link_libraries(libcat-std INTERFACE gcc)
# Only link the sanitizers when debugging.
target_link_libraries(
  libcat-std INTERFACE
  $<$<CONFIG:RelWithDebInfo>:asan>
  $<$<CONFIG:RelWithDebInfo>:ubsan>
  # $<$<CONFIG:Debug>:asan>
  # $<$<CONFIG:Debug>:ubsan>
)

# Build the examples.
add_executable(hello examples/hello.cpp)
target_link_libraries(hello PRIVATE libcat-std)

# add_executable(sizeof examples/sizeof.cpp)
# target_link_libraries(sizeof PRIVATE libcat-std)

# add_executable(exit_with_10 examples/exit_with_10.cpp)
# target_link_libraries(exit_with_10 PRIVATE libcat-std)

# add_executable(error examples/error.cpp)
# target_link_libraries(error PRIVATE libcat-std)

# add_executable(safe_arithmetic examples/safe_arithmetic.cpp)
# target_link_libraries(safe_arithmetic PRIVATE libcat-std)
