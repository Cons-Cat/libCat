#pragma once

#include <cat/allocator_musl>
#include <cat/allocator_page>
#include <cat/arithmetic>
#include <cat/debug>
#include <cat/defer>
#include <cat/string>

// All unit tests have access to these symbols:
using namespace cat::literals;
using namespace cat::arithmetic;

class tests_allocator : public cat::allocator_interface<tests_allocator> {
 private:
   friend cat::allocator_interface<tests_allocator>;
   friend cat::allocator_dyn_friend;
   template <typename>
   friend class cat::allocator_ref;

   cat::allocator_musl* _Nonnull m_p_allocator;

   auto
   allocation_bytes(cat::ualign alignment, cat::idx requested_bytes)
      -> cat::maybe_non_zero<cat::idx> {
      return m_p_allocator->align_nalloc_multi<cat::byte>(
         alignment, requested_bytes
      );
   }

   auto
   allocate(cat::idx allocation_bytes) -> void* _Nullable {
      auto allocation =
         m_p_allocator->alloc_multi_uninit<cat::byte>(allocation_bytes);
      return allocation.has_value() ? allocation.value().data() : nullptr;
   }

   auto
   aligned_allocate(cat::ualign alignment, cat::idx allocation_bytes)
      -> void* _Nullable {
      auto allocation = m_p_allocator->align_alloc_multi_uninit<cat::byte>(
         alignment, allocation_bytes
      );
      return allocation.has_value() ? allocation.value().data() : nullptr;
   }

   void
   deallocate(void const* _Nonnull p_storage, cat::idx allocation_bytes) {
      m_p_allocator->free_multi_uninit(
         cat::span(
            const_cast<cat::byte*>(static_cast<cat::byte const*>(p_storage)),
            allocation_bytes
         )
      );
   }

 public:
   static constexpr cat::ualign min_alignment =
      cat::allocator_musl::min_alignment;
   static constexpr cat::idx min_allocation_bytes =
      cat::allocator_musl::min_allocation_bytes;

   explicit tests_allocator(cat::allocator_musl& allocator)
       : m_p_allocator(&allocator) {
   }
};

inline cat::allocator_musl g_tests_alloc_musl;
inline tests_allocator g_tests_alloc(g_tests_alloc_musl);
inline cat::allocator_page pager;

using constructor_fn = void (*_Nonnull const)();

void
register_test(constructor_fn p_test_fn);

void
run_test_prologue(cat::str_view label, constructor_fn p_test_fn);

// This macro declares a unit test named `test_name`, which is executed
// automatically in this program's constructor calls.
// Attributes can be placed before an instantiation of the macro to modify its
// behavior.
#define CAT_TEST(test_name)                                              \
   void test_##test_name();                                              \
   void test_##test_name##_prologue();                                   \
                                                                         \
   [[gnu::constructor]]                                                  \
   void cat_register_test##test_name() {                                 \
      register_test(test_##test_name##_prologue);                        \
   }                                                                     \
                                                                         \
   void test_##test_name##_prologue() {                                  \
      run_test_prologue(": test_" #test_name "...\n", test_##test_name); \
   }                                                                     \
   /* TODO: Debug IR passes. */                                          \
   void test_##test_name()

// `CAT_TEST` should never be `#undef`'d. The redefinable macro `test` exists
// to make this macro more ergonomic.
#pragma clang final(CAT_TEST)

#define $test CAT_TEST
