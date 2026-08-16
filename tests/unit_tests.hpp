#pragma once

#include <cat/debug>

// All unit tests have access to these symbols:
using namespace cat::literals;
using namespace cat::arithmetic;

namespace cat {
class page_allocator;
}

extern cat::page_allocator pager;

using constructor_fn = void (*const)();

void
register_test(constructor_fn p_test_fn);

void
run_test_prologue(cat::str_view label, constructor_fn p_test_fn);

// This macro declares a unit test named `test_name`, which is executed
// automatically in this program's constructor calls.
// Attributes can be placed before an instantiation of the macro to modify its
// behavior.
#define CAT_TEST(test_name)                                                 \
   void test_##test_name();                                                 \
   void test_##test_name##_prologue();                                      \
                                                                            \
   [[gnu::constructor]]                                                     \
   void cat_register_test##test_name() {                                    \
      register_test(test_##test_name##_prologue);                            \
   }                                                                        \
                                                                            \
   void test_##test_name##_prologue() {                                     \
      run_test_prologue(": test_" #test_name "...\n", test_##test_name);     \
   }                                                                        \
   /* TODO: Debug IR passes. */                                             \
   void test_##test_name()

// `CAT_TEST` should never be `#undef`'d. The redefinable macro `test` exists
// to make this macro more ergonomic.
#pragma clang final(CAT_TEST)

#define $test CAT_TEST
