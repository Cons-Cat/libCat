# `.gdbinit` symlink for examples. `SINGLE_CONFIG` also claims the top-level
# `${CMAKE_BINARY_DIR}/.gdbinit` for single-config generators.
cat_gdb_symlink(cat-gdb-examples SINGLE_CONFIG)

add_library(cat-examples INTERFACE)

# In-tree examples need `cat-impl`'s `INTERNAL` flags so `REUSE_FROM cat-impl`
# below passes the PCH validator.
target_compile_options(cat-examples INTERFACE
  ${CAT_COMPILE_OPTIONS_INTERNAL}
  -ferror-limit=4
)

target_link_libraries(cat-examples INTERFACE cat)

option(CAT_BUILD_ALL_EXAMPLES "Compile all examples unconditionally." ON)

option(CAT_BUILD_EXAMPLE_HELLO "Compile hello.cpp." OFF)
if(CAT_BUILD_EXAMPLE_HELLO OR CAT_BUILD_ALL_EXAMPLES)
  add_executable(hello hello.cpp)
  # `NO_ARGC_ARGV` pinned to `_start.cpp` and gated on the `hello` target so it
  # doesn't contaminate sibling examples that share `TARGET_DIRECTORY`. See
  # `tests/CMakeLists.txt` for the `SKIP_PRECOMPILE_HEADERS` rationale.
  set_source_files_properties(
    ${CMAKE_SOURCE_DIR}/src/libraries/runtime/implementations/_start.cpp
    TARGET_DIRECTORY hello
    PROPERTIES
      COMPILE_DEFINITIONS
        "$<$<STREQUAL:$<TARGET_PROPERTY:NAME>,hello>:NO_ARGC_ARGV>"
      SKIP_PRECOMPILE_HEADERS ON)
  target_link_libraries(hello PRIVATE cat-examples)
  if(CAT_PCH)
    target_precompile_headers(hello REUSE_FROM cat-impl)
  endif()
endif()

option(CAT_BUILD_EXAMPLE_ECHO "Compile echo.cpp." OFF)
if(CAT_BUILD_EXAMPLE_ECHO OR CAT_BUILD_ALL_EXAMPLES)
  add_executable(echo echo.cpp)
  target_link_libraries(echo PRIVATE cat-examples)
  if(CAT_PCH)
    target_precompile_headers(echo REUSE_FROM cat-impl)
  endif()
endif()

option(CAT_BUILD_EXAMPLE_CLIENT_SERVER "Compile client.cpp and server.cpp." OFF)
if(CAT_BUILD_EXAMPLE_CLIENT_SERVER OR CAT_BUILD_ALL_EXAMPLES)
  add_executable(client client.cpp)
  target_link_libraries(client PRIVATE cat-examples)
  if(CAT_PCH)
    target_precompile_headers(client REUSE_FROM cat-impl)
  endif()

  add_executable(server server.cpp)
  target_link_libraries(server PRIVATE cat-examples)
  if(CAT_PCH)
    target_precompile_headers(server REUSE_FROM cat-impl)
  endif()
endif()

option(CAT_BUILD_EXAMPLE_WINDOW "Compile window.cpp." OFF)
if(CAT_BUILD_EXAMPLE_WINDOW OR CAT_BUILD_ALL_EXAMPLES)
  add_executable(window window.cpp)
  target_link_libraries(window PRIVATE cat-examples)
  if(CAT_PCH)
    target_precompile_headers(window REUSE_FROM cat-impl)
  endif()
endif()

option(CAT_BUILD_EXAMPLE_CAT "Compile cat.cpp." OFF)
if(CAT_BUILD_EXAMPLE_CAT OR CAT_BUILD_ALL_EXAMPLES)
  add_executable(unixcat cat.cpp)
  target_link_libraries(unixcat PRIVATE cat-examples)
  if(CAT_PCH)
    target_precompile_headers(unixcat REUSE_FROM cat-impl)
  endif()
  set_target_properties(unixcat PROPERTIES OUTPUT_NAME cat)
endif()

# A dummy project is required to guarantee that the directories are generated.
# The directories must be generated for symlinking `.gdbinit` to succeed.
# This can be skipped if one or more other examples are built.
if(
  NOT (CAT_BUILD_ALL_EXAMPLES
  OR CAT_BUILD_LIBC_EXAMPLES
  OR CAT_BUILD_EXAMPLE_CAT
  OR CAT_BUILD_EXAMPLE_CLIENT_SERVER
  OR CAT_BUILD_EXAMPLE_ECHO
  OR CAT_BUILD_EXAMPLE_HELLO
  OR CAT_BUILD_EXAMPLE_WINDOW)
)
  add_executable(dummy echo.cpp)
  target_link_libraries(dummy PRIVATE cat-examples)
  if(CAT_PCH)
    target_precompile_headers(dummy REUSE_FROM cat-impl)
  endif()
endif()

# libC executables are only configured for `Release`.
list(
  APPEND
  LIBC_RELEASE_OPTIONS
  -fno-exceptions -fno-rtti -fno-unwind-tables
  -fno-asynchronous-unwind-tables
  -no-pie
  -ffunction-sections -fdata-sections
  -fvisibility=hidden -fvisibility-inlines-hidden
  -flto -fwhole-file -fno-plt
  -Wl,-z,noseparate-code,--gc-sections
  $<$<CONFIG:Release>:-O3>
  $<$<CONFIG:RelSmall>:-Os>
)

# Do not build libC examples by default.
option(CAT_BUILD_LIBC_EXAMPLES "Enable libC examples to compare against libCat." OFF)

if(CAT_BUILD_LIBC_EXAMPLES)
  add_executable(hello_libc hello_libc.cpp)
  target_compile_options(
    hello_libc PRIVATE
    ${LIBC_RELEASE_OPTIONS}
  )
  target_link_options(
    hello_libc PRIVATE
    ${LIBC_RELEASE_OPTIONS}
  )

  add_executable(memcpy_libc memory_copy_libc.cpp)
  target_compile_options(
    memcpy_libc PRIVATE
    ${LIBC_RELEASE_OPTIONS}
  )
  target_link_options(
    memcpy_libc PRIVATE
    ${LIBC_RELEASE_OPTIONS}
  )
endif()
