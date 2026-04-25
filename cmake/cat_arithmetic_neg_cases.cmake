# Negative probe sources for `execute_process` + `${CMAKE_CXX_COMPILER} -fsyntax-only` (included
# from `cat_arithmetic_negative.cmake` after
# `_cat_neg_try_fails` is defined). Each case must be ill-formed. Sources mirror the **inverse** of
# the positive / trait story in the `test_arithmetic` unit tests: `!is_assignable`, `!is_convertible`,
# `!is_implicitly_constructible`, `!can_brace_init`,
# `!can_{plus,minus,times}_assign`, and deleted `index` / `arithmetic` shapes.
#
# Operands that are **plain literals** (or prvalues that look like them) are often still constant
# expressions, so they can take a different `enable_if` / `consteval` path than the runtime
# `is_convertible` / `requires` story in the unit file. We wrap those with
# `cat_neg_probe::deconst_number` from `cmake/arithmetic_neg_deconst.hpp` (same idea as
# `deconst_number` in the `test_arithmetic` sources).
#
# **Not** every `static_assert` in the unit file. Type-only traits are still out. The `int1`..`int8` /
# `uint1`..`uint8` `=` / `icvt` / `+=` / `*=` / `-=` grids that are ill-formed are generated in
# `cat_arithmetic_neg_matrix.cmake` from the `deconst` / narrow-LHS / mixed-signed grid.

# Width-by-width OOR brace `uintN` / `intN` constants are emitted in `cat_arithmetic_negative.cmake`
# (needs `${n}` interpolation; CMake `[[` ... `]]` is literal). Here: additional braced inits
# with out-of-range constants. Keep those as **true** constant
# expressions: OOR is validated via `consteval` / deleted overloads, not the runtime `enable_if` path.

_cat_neg_try_fails("uint1-const-1000" [[#include <cat/arithmetic>
void t() { (void)cat::uint1{1000u}; }
]])

_cat_neg_try_fails("int1-brace-wider-int4-literal" [[#include <cat/arithmetic>
using namespace cat::literals;
void t() { (void)cat::int1{ 200_i4 }; }
]])

# Copy-initialization from *known* out-of-range **constants** for the implicit
# `enable_if(detail::raw_source_fits_implicit_storage<...>(other))` converting
# constructors (undefined policy), aligned with `arithmetic_detail_raw_implicit_storage` /
# `raw_source_fits` coverage: 300 for `uint1`, 300u for
# `int1`, 16'777'217 for `float4`, -1ll for `uint8`, -1 / -1ll for `idx` and
# `uintptr` (no `deconst_number` — we *want* these to stay constant
# expressions so the `enable_if` / `consteval` resolution matches the table).
# (No `intptr` = -1: `raw_source_fits` is true for that pairing in
# `arithmetic_detail_raw_implicit_storage` — the implicit ctor is *sound*.)
_cat_neg_try_fails("enable-if-uint1-implicit-int-300" [[#include <cat/arithmetic>
void t() { cat::uint1 a = 300; }
]])
_cat_neg_try_fails("enable-if-int1-implicit-uint-300" [[#include <cat/arithmetic>
void t() { cat::int1 a = 300u; }
]])
_cat_neg_try_fails("enable-if-float4-implicit-int-inexact" [[#include <cat/arithmetic>
void t() { cat::float4 f = 16'777'217; }
]])
_cat_neg_try_fails("enable-if-uint8-implicit-ll-neg1" [[#include <cat/arithmetic>
void t() { cat::uint8 u = -1ll; }
]])
_cat_neg_try_fails("enable-if-idx-implicit-int-neg1" [[#include <cat/arithmetic>
void t() { cat::idx i = -1; }
]])
_cat_neg_try_fails("enable-if-idx-implicit-ll-neg1" [[#include <cat/arithmetic>
void t() { cat::idx i = -1ll; }
]])
_cat_neg_try_fails("enable-if-uintptr-implicit-int-neg1" [[#include <cat/arithmetic>
void t() { cat::uintptr<void> p = -1; }
]])
_cat_neg_try_fails("enable-if-uintptr-implicit-ll-neg1" [[#include <cat/arithmetic>
void t() { cat::uintptr<void> p = -1ll; }
]])

# `__attribute__((enable_if(...)))` on `operator+=` / `operator=` (see
# `overflow_reference.hpp`, `index` in `arithmetic`): operands are **literals** or
# prvalues that stay constant expressions so the attribute / `consteval` delete
# on the inner `arithmetic` conversion is exercised (not `deconst_number`).
# `idx += -1` has no viable `+=` (no `add` that yields `index`).
_cat_neg_try_fails("idx-pluseq-negative-literal" [[#include <cat/arithmetic>
void t() { cat::idx i(0u); i += -1; }
]])
_cat_neg_try_fails("undef-int1-pluseq-uint-OOR" [[#include <cat/arithmetic>
void t() { auto r = cat::int1(0).undef(); r += 300u; }
]])
_cat_neg_try_fails("undef-uint1-pluseq-int-neg" [[#include <cat/arithmetic>
void t() { auto r = cat::uint1(0u).undef(); r += -1; }
]])

# --- `!is_convertible` (implicit) / `!is_implicitly_constructible`
_cat_neg_try_fails("cvt-idx-to-int" [[#include <cat/arithmetic>
#include "arithmetic_neg_deconst.hpp"
void t() { int x; x = cat::idx(cat_neg_probe::deconst_number(0u)); }
]])
_cat_neg_try_fails("cvt-idx-to-int2" [[#include <cat/arithmetic>
#include "arithmetic_neg_deconst.hpp"
void t() { cat::int2 x; x = cat::idx(cat_neg_probe::deconst_number(0u)); }
]])
_cat_neg_try_fails("cvt-idx-to-int4" [[#include <cat/arithmetic>
#include "arithmetic_neg_deconst.hpp"
void t() { cat::int4 x; x = cat::idx(cat_neg_probe::deconst_number(0u)); }
]])
_cat_neg_try_fails("cvt-idx-to-float4" [[#include <cat/arithmetic>
#include "arithmetic_neg_deconst.hpp"
void t() { cat::float4 f; f = cat::idx(cat_neg_probe::deconst_number(0u)); }
]])
# (No `x = cat::idx(0u)` to `int4` probe: resolution can match a conversion
# the positive suite flags as `!is_convertible` in the trait only; a plain
# assignment TUs is not a stable ill-formed `try_build` in all Clang front-end
# paths. `idx` → `int` remains below.)
# (`icvt` / narrow `int` / `uint` and `int` → `float4` / `float8` are in
# `cat_arithmetic_neg_matrix.cmake`.)

# Runtime `iword` / `uword` to `idx` (implicit)
_cat_neg_try_fails("icvt-uword-to-idx" [[#include <cat/arithmetic>
#include "arithmetic_neg_deconst.hpp"
void t() { cat::uword w(0u); cat::idx i = cat_neg_probe::deconst_number(w); }
]])
_cat_neg_try_fails("icvt-iword-to-idx" [[#include <cat/arithmetic>
#include "arithmetic_neg_deconst.hpp"
void t() { cat::iword w(0); cat::idx i = cat_neg_probe::deconst_number(w); }
]])
# Assignment without conversion from `iword` to `idx` (same as positive `!is_convertible<iword,idx>` for `=`)
_cat_neg_try_fails("asgn-idx-iword" [[#include <cat/arithmetic>
#include "arithmetic_neg_deconst.hpp"
void t() { cat::idx i(0u); cat::iword w(0); i = cat_neg_probe::deconst_number(w); }
]])

# `!is_convertible` to `arithmetic_ptr` (implicit)
_cat_neg_try_fails("icvt-unsigned-long-to-intptr" [[#include <cat/arithmetic>
#include "arithmetic_neg_deconst.hpp"
void t() { unsigned long u = cat_neg_probe::deconst_number(0ul); cat::intptr<void> p = u; }
]])
_cat_neg_try_fails("icvt-int-to-uintptr" [[#include <cat/arithmetic>
#include "arithmetic_neg_deconst.hpp"
void t() { int x = cat_neg_probe::deconst_number(0); cat::uintptr<void> p = x; }
]])
_cat_neg_try_fails("icvt-long-to-uintptr" [[#include <cat/arithmetic>
#include "arithmetic_neg_deconst.hpp"
void t() { long x = cat_neg_probe::deconst_number(0L); cat::uintptr<void> p = x; }
]])
_cat_neg_try_fails("icvt-uword-to-intptr" [[#include <cat/arithmetic>
#include "arithmetic_neg_deconst.hpp"
void t() { cat::uword w(0u); cat::intptr<void> p = cat_neg_probe::deconst_number(w); }
]])
_cat_neg_try_fails("icvt-iword-to-uintptr" [[#include <cat/arithmetic>
#include "arithmetic_neg_deconst.hpp"
void t() { cat::iword w(0); cat::uintptr<void> p = cat_neg_probe::deconst_number(w); }
]])

# `!has_binary_bit_{and,or}<uint4,uword>`: there is no ill-formed *expression* form
# in all builds that mirrors the trait; a dedicated `requires` suite can cover the
# type-trait, not a single -fsyntax-only TU.

# (Narrow-LHS `+=` / `*=` / `-=` / `undef() -=` for every wider `intN` / `uintN` pair
# is in `cat_arithmetic_neg_matrix.cmake`.)

# --- `index` deleted or absent operations
_cat_neg_try_fails("idx-minuseq" [[#include <cat/arithmetic>
#include "arithmetic_neg_deconst.hpp"
void t() { cat::idx i(cat_neg_probe::deconst_number(1u)); i -= cat_neg_probe::deconst_number(1u); }
]])
_cat_neg_try_fails("idx-predec" [[#include <cat/arithmetic>
#include "arithmetic_neg_deconst.hpp"
void t() { cat::idx i(cat_neg_probe::deconst_number(1u)); --i; }
]])
_cat_neg_try_fails("idx-postdec" [[#include <cat/arithmetic>
#include "arithmetic_neg_deconst.hpp"
void t() { cat::idx i(cat_neg_probe::deconst_number(1u)); i--; }
]])
_cat_neg_try_fails("idx-diveq-signed" [[#include <cat/arithmetic>
#include "arithmetic_neg_deconst.hpp"
void t() { cat::idx i(cat_neg_probe::deconst_number(6u)); int s = cat_neg_probe::deconst_number(2); i /= s; }
]])
_cat_neg_try_fails("idx-bitand" [[#include <cat/arithmetic>
#include "arithmetic_neg_deconst.hpp"
void t() { (void)(cat::idx(cat_neg_probe::deconst_number(1u)) & cat::idx(cat_neg_probe::deconst_number(1u))); }
]])

# `operator-` (binary): only the `is_integral` `__attribute__((enable_if(...)))` friend
# and `minus_interface` path through `subtract_by` (also integral) apply. Non-integer
# arithmetic and pointers have no `cat::` `operator-` and fail overload resolution
_cat_neg_try_fails("idx-minus-float" [[#include <cat/arithmetic>
void t() { (void)(cat::idx(0u) - 1.0f); }
]])
_cat_neg_try_fails("idx-minus-double" [[#include <cat/arithmetic>
void t() { (void)(cat::idx(0u) - 1.0); }
]])
_cat_neg_try_fails("idx-minus-int-ptr" [[#include <cat/arithmetic>
void t() { int a = 0; (void)(cat::idx(0u) - &a); }
]])

# `idx` / `uword` / `iword` : `deconst(1u)` on the RHS must not send the
# `operator-` result through a `uword` shape (so `f(uword)` = delete is not
# the best match for the prvalue)
_cat_neg_try_compiles("idx-minus-deconst-1u-binds-iword" [[#include <cat/arithmetic>
#include "arithmetic_neg_deconst.hpp"
void f(cat::iword) {}
void f(cat::uword) = delete;
void t() { f(cat::idx(0u) - cat_neg_probe::deconst_number(1u)); }
]])

# Mixed `uintptr` / `int` and `intptr` / wide unsigned (ill-formed `operator=`)
_cat_neg_try_fails("asgn-uintptr-int-negative" [[#include <cat/arithmetic>
#include "arithmetic_neg_deconst.hpp"
void t() { cat::uintptr<void> p; int x = cat_neg_probe::deconst_number(-1); p = x; }
]])
# (No `p = (unsigned long)-1` to `intptr`: the raw value is often a safe conversion;
#  the `!is_convertible` trait is not a single `=` probe.)
