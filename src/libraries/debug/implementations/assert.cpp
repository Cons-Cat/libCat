#include <cat/debug>
#include <cat/string>

// Because `assert()`'s arguments are not passed into `verify()` when `NDEBUG`
// is defined, the compiler should not warn when they are unused.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

// Check that an expression holds true when `NDEBUG` is not defined. If it holds
// false, invoke an assert handler.
void cat::assert(bool invariant_expression,
                 void (*p_assert_handler)(SourceLocation const&),
                 SourceLocation const& callsite) {
#ifndef NDEBUG
    verify(invariant_expression, p_assert_handler, callsite);
#endif
}

// Check that an expression holds true when `NDEBUG` is not defined. If it
// holds false, print `error_string` and invoke an assert handler.
void cat::assert(bool invariant_expression, String const error_string,
                 void (*p_assert_handler)(SourceLocation const&),
                 SourceLocation const& callsite) {
#ifndef NDEBUG
    verify(invariant_expression, error_string, p_assert_handler, callsite);
#endif
}

#pragma GCC diagnostic pop
