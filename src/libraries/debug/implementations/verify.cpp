#include <cat/debug>
#include <cat/string>

// Check that an expression holds true in all builds. If it holds false, invoke
// `p_assert_handler`.
void
cat::verify(bool invariant_expression,
            void (*p_assert_handler)(source_location const&),
            source_location const& callsite) {
   if (invariant_expression) [[likely]] {
      return;
   }

   p_assert_handler(callsite);
   __builtin_unreachable();
}

// Check that an expression holds true in all builds. If it holds false, print
// `error_string` and invoke `p_assert_handler`.
void
cat::verify(bool invariant_expression, str_span const error_string,
            void (*p_assert_handler)(source_location const&),
            source_location const& callsite) {
   if (invariant_expression) [[likely]] {
      return;
   }
   auto _ = eprint("\n");
   auto _ = eprintln(error_string);

   p_assert_handler(callsite);
   __builtin_unreachable();
}
