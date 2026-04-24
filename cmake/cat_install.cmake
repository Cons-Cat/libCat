# `install` / `find_package(cat)` support
#
# `cmake --install build [--prefix <sysroot>]` lays out the libCat tree
# so an external CMake project can do
#
#     find_package(cat REQUIRED)
#     target_link_libraries(my_target PRIVATE cat::cat)              # libcat.a
#     target_link_libraries(my_target PRIVATE cat::cat-impl-shared)  # libcat.so
#
# and inherit the same usage requirements (essential compile flags,
# `-include global_includes.hpp`, `_start.cpp`, `libcat.ld`) the in-tree
# tests / examples get from `cat`'s `INTERFACE`. The export set is named
# `catTargets`. `catConfig.cmake` / `catConfigVersion.cmake` are
# generated from `cmake/catConfig.cmake.in`.
#
# Layout under `${CMAKE_INSTALL_PREFIX}` after `cmake --install`:
#
#   <prefix>/lib/                                libcat.a [+ libcat.so]
#   <prefix>/lib/cmake/cat/                      catTargets*.cmake +
#                                                catConfig.cmake +
#                                                catConfigVersion.cmake
#   <prefix>/include/libcat/                     `src/` mirrored here
#   <prefix>/include/libcat/global_includes.hpp  force-included header
#   <prefix>/share/libcat/libcat.ld              custom linker script
#
# Registering these rules unconditionally has no cost on a regular
# build -- CMake only executes them on `cmake --install` / `ninja
# install`. Consumers who never install pay nothing.

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# `cat` and `cat-impl` always exist; `cat-impl-shared` only when
# `CAT_BUILD_SHARED` was on at configure time.
set(_cat_install_targets cat cat-impl)
if (TARGET cat-impl-shared)
  list(APPEND _cat_install_targets cat-impl-shared)
endif()

install(TARGETS ${_cat_install_targets}
  EXPORT catTargets
  LIBRARY  DESTINATION "${CMAKE_INSTALL_LIBDIR}"
  ARCHIVE  DESTINATION "${CMAKE_INSTALL_LIBDIR}"
  RUNTIME  DESTINATION "${CMAKE_INSTALL_BINDIR}"
  INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/libcat")

# Headers + .tpp files. `src/libraries/<lib>/` is mirrored verbatim under
# `<prefix>/include/libcat/libraries/<lib>/`. The `INSTALL_INTERFACE`
# include dirs in `src/CMakeLists.txt` point at exactly these paths, so
# the `.cpp` consumer of `cat::cat` resolves `#include <cat/...>` (and the
# `"./implementations/*.tpp"` relative includes from headers) without any
# extra flag wrangling.
foreach(_cat_subdir IN LISTS CAT_INCLUDE_SUBDIRS)
  install(DIRECTORY "${CMAKE_SOURCE_DIR}/src/libraries/${_cat_subdir}/"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/libcat/libraries/${_cat_subdir}"
    FILES_MATCHING
      PATTERN "*.hpp"
      PATTERN "*.tpp"
      # libCat's public API headers are extension-less (`cat/string`,
      # `cat/runtime`, ...) so the file pattern alone misses them. They
      # all live in a `cat/` directory, so a directory-level pattern is
      # the simplest way to grab them without sweeping in `.cpp` sources.
      PATTERN "cat/*" EXCLUDE)
  # The `cat/` directory is installed wholesale: extension-less public
  # API headers plus subdirectories like `cat/detail/` and `cat/implementations/`.
  install(DIRECTORY "${CMAKE_SOURCE_DIR}/src/libraries/${_cat_subdir}/cat"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/libcat/libraries/${_cat_subdir}")
endforeach()
unset(_cat_subdir)

# `_start.cpp` is on `cat`'s `INTERFACE_SOURCES` (see `src/CMakeLists.txt`),
# and the `INSTALL_INTERFACE` path expects it to live next to the rest of
# the runtime library files.
install(FILES
  "${CMAKE_SOURCE_DIR}/src/libraries/runtime/implementations/_start.cpp"
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/libcat/libraries/runtime/implementations")

# `global_includes.hpp` is force-included via `-include global_includes.hpp`
# in `CAT_COMPILE_OPTIONS_ESSENTIAL`. It lives at the top of `src/`, so
# the `${CMAKE_INSTALL_INCLUDEDIR}/libcat` include dir on `cat`'s
# `INSTALL_INTERFACE` is enough to make `-include` find it.
install(FILES "${CMAKE_SOURCE_DIR}/src/global_includes.hpp"
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/libcat")

# `libcat.ld` is referenced from `cat-impl` / `cat-impl-shared`'s
# `INSTALL_INTERFACE` link options as `${CMAKE_INSTALL_FULL_DATADIR}/libcat/libcat.ld`,
# so install it there.
install(FILES "${CMAKE_SOURCE_DIR}/src/libcat.ld"
  DESTINATION "${CMAKE_INSTALL_DATADIR}/libcat")

install(EXPORT catTargets
  FILE catTargets.cmake
  NAMESPACE cat::
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/cat")

# Generate `catConfigVersion.cmake` from libCat's project version.
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/catConfigVersion.cmake"
  VERSION "${PROJECT_VERSION}"
  COMPATIBILITY SameMajorVersion)

configure_package_config_file(
  "${CMAKE_SOURCE_DIR}/cmake/catConfig.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/catConfig.cmake"
  INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/cat")

install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/catConfig.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/catConfigVersion.cmake"
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/cat")

unset(_cat_install_targets)
