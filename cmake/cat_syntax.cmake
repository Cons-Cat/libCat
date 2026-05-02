# This file is flagrantly "vibe-coded". It may not be up to the standards of
# most libCat code.

# `cat-syntax` -- fast `-fsyntax-only` sweep over every libCat TU.
#
# Re-runs Clang's frontend (lex, parse, type-check, template instantiation) over
# `cat-impl`'s sources without any codegen. Writes nothing to disk, skips the
# linker entirely, and serves as a quick "did everything still compile?" check.
#
# Modeled after `cat_opt_report.cmake`: a parallel `OBJECT` library reuses
# `cat-impl`'s source list, copies `cat`'s usage requirements needed by the
# frontend, re-applies `CAT_CXX_FLAGS_INTERNAL` (PRIVATE on `cat-impl`, so it
# does not propagate), and adds `-fsyntax-only` PRIVATE. `EXCLUDE_FROM_ALL` keeps
# the shadow off the default build.
#
# `REUSE_FROM cat-impl` keeps the check fast.
#
# Building `cat-syntax` re-runs the check unconditionally: with `-fsyntax-only`
# writing no `.o`, Ninja sees the declared outputs missing on every invocation.
# That is the intended behaviour for an on-demand target.
#
# Scope: `cat-impl`'s direct sources only. `cat-impl-shared` differs only in
# `-fPIC` (no frontend effect), and tests / examples get full coverage from the
# regular build.

get_property(_cat_impl_sources_for_syntax TARGET cat-impl PROPERTY SOURCES)

add_library(cat-syntax-lib OBJECT EXCLUDE_FROM_ALL ${_cat_impl_sources_for_syntax})
target_compile_features(cat-syntax-lib PRIVATE ${CAT_CXX_STANDARD_FEATURE})
set_target_properties(cat-syntax-lib PROPERTIES CXX_EXTENSIONS ON)
target_include_directories(cat-syntax-lib PRIVATE
  $<TARGET_PROPERTY:cat,INTERFACE_INCLUDE_DIRECTORIES>)
target_compile_options(cat-syntax-lib PRIVATE
  $<TARGET_PROPERTY:cat,INTERFACE_COMPILE_OPTIONS>
  ${CAT_CXX_FLAGS_INTERNAL}
  -Wno-unused-command-line-argument
  -fsyntax-only)

if (CAT_PCH)
  target_precompile_headers(cat-syntax-lib REUSE_FROM cat-impl)
endif()

add_custom_target(cat-syntax
  DEPENDS cat-syntax-lib
  COMMENT "Syntax-checked every cat-impl TU with -fsyntax-only.")

unset(_cat_impl_sources_for_syntax)
