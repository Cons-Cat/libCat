#include <cat/debug>
#include <cat/string>

void
cat::detail::assert_failed(
   assert_handler p_assert_handler, source_location const& callsite
) noexcept(!__has_feature(cxx_exceptions)) {
   p_assert_handler(callsite);
}

void
cat::detail::assert_failed(
   str_view const& error_string, assert_handler p_assert_handler,
   source_location const& callsite
) noexcept(!__has_feature(cxx_exceptions)) {
   auto _ = eprintln();
   eprintln(error_string).or_exit();
   p_assert_handler(callsite);
}
