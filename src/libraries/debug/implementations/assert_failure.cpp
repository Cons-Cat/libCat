#include <cat/debug>
#include <cat/string>

[[noreturn]]
void
cat::detail::assert_failed(
   assert_handler p_assert_handler, source_location const& callsite
) {
   p_assert_handler(callsite);
   __builtin_unreachable();
}

[[noreturn]]
void
cat::detail::assert_failed(
   str_view const& error_string, assert_handler p_assert_handler,
   source_location const& callsite
) {
   auto _ = eprintln();
   eprintln(error_string).or_exit();
   p_assert_handler(callsite);
   __builtin_unreachable();
}
