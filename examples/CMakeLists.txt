# Link `.gdbinit` to build directories.
add_custom_target(
  cat-gdb-examples ALL
  COMMAND ${CMAKE_COMMAND} -E create_symlink
  # This requires CMake 3.20:
  ${PROJECT_SOURCE_DIR}/.gdbinit ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/.gdbinit
  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>
)
add_dependencies(cat cat-gdb-examples)

add_library(cat-examples INTERFACE)
target_link_libraries(cat-examples INTERFACE cat)

option(CAT_BUILD_ALL_EXAMPLES "Compile all examples unconditionally." ON)

option(CAT_BUILD_EXAMPLE_HELLO "Compile hello.cpp." OFF)
if(CAT_BUILD_EXAMPLE_HELLO OR CAT_BUILD_ALL_EXAMPLES)
  add_executable(hello hello.cpp)
  target_compile_options(hello PRIVATE ${CAT_COMPILE_FLAGS})
  target_compile_definitions(hello PRIVATE "NO_ARGC_ARGV" "INLINE_MAIN")
  target_link_libraries(hello PRIVATE cat-examples)
  target_link_options(hello PRIVATE ${CAT_LINK_FLAGS})
endif()

option(CAT_BUILD_EXAMPLE_ECHO "Compile echo.cpp." OFF)
if(CAT_BUILD_EXAMPLE_ECHO OR CAT_BUILD_ALL_EXAMPLES)
  add_executable(echo echo.cpp)
  target_compile_options(echo PRIVATE ${CAT_COMPILE_FLAGS})
  target_compile_definitions(echo PRIVATE "INLINE_MAIN")
  target_link_libraries(echo PRIVATE cat-examples)
  target_link_options(echo PRIVATE ${CAT_LINK_FLAGS})
endif()

option(CAT_BUILD_EXAMPLE_CLIENT_SERVER "Compile client.cpp and server.cpp." OFF)
if(CAT_BUILD_EXAMPLE_CLIENT_SERVER OR CAT_BUILD_ALL_EXAMPLES)
  add_executable(client client.cpp)
  target_compile_options(client PRIVATE ${CAT_COMPILE_FLAGS})
  target_link_libraries(client PRIVATE cat-examples)
  target_link_options(client PRIVATE ${CAT_LINK_FLAGS})

  add_executable(server server.cpp)
  target_compile_options(server PRIVATE ${CAT_COMPILE_FLAGS})
  target_link_libraries(server PRIVATE cat-examples)
  target_link_options(server PRIVATE ${CAT_LINK_FLAGS})
endif()

option(CAT_BUILD_EXAMPLE_WINDOW "Compile window.cpp." OFF)
if(CAT_BUILD_EXAMPLE_WINDOW OR CAT_BUILD_ALL_EXAMPLES)
  add_executable(window window.cpp)
  target_compile_options(window PRIVATE ${CAT_COMPILE_FLAGS})
  target_link_libraries(window PRIVATE cat-examples)
  target_link_options(window PRIVATE ${CAT_LINK_FLAGS})
endif()

option(CAT_BUILD_EXAMPLE_CAT "Compile cat-examples.cpp." OFF)
if(CAT_BUILD_EXAMPLE_CAT OR CAT_BUILD_ALL_EXAMPLES)
  add_executable(unixcat cat.cpp)
  target_compile_options(unixcat PRIVATE ${CAT_COMPILE_FLAGS})
  target_link_libraries(unixcat PRIVATE cat-examples)
  target_link_options(unixcat PRIVATE ${CAT_LINK_FLAGS})
  set_target_properties(unixcat PROPERTIES OUTPUT_NAME cat-examples)
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
  target_link_options(dummy PRIVATE ${CAT_LINK_FLAGS})
endif()

# libC executables are only configured for Release.
list(
  APPEND
  LIBC_RELEASE_FLAGS
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
    ${LIBC_RELEASE_FLAGS}
  )
  target_link_options(
    hello_libc PRIVATE
    ${LIBC_RELEASE_FLAGS}
  )

  add_executable(memcpy_libc memory_copy_libc.cpp)
  target_compile_options(
    memcpy_libc PRIVATE
    ${LIBC_RELEASE_FLAGS}
  )
  target_link_options(
    memcpy_libc PRIVATE
    ${LIBC_RELEASE_FLAGS}
  )
endif()
