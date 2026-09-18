// Clang `-verify` compile test. Not linked into `unit_tests`.
// Run with `just test arithmetic`.

#include <cat/arithmetic>
#include <cat/bit>

using namespace cat::literals;
using namespace cat::arithmetic;

void
oor_brace_constants() {
   // expected-error@+1 {{}}
   uint1{-1};
   // expected-error@+1 {{}}
   uint2{-1};
   // expected-error@+1 {{}}
   uint4{-1};
   // expected-error@+1 {{}}
   uint8{-1};
   // expected-error@+1 {{}}
   int1{1'000};
   // expected-error@+1 {{}}
   int2{100'000};
   // expected-error@+1 {{}}
   int4{5'000'000'000};
   // expected-error@+1 {{}}
   int8{18'446'744'073'709'551'616u};
   // expected-error@+1 {{}}
   uint1{1'000u};
   // expected-error@+1 {{}}
   int1{200_i4};
}

void
enable_if_implicit_and_brace() {
   // expected-error@+1 {{}}
   uint1 a = 300;
   // expected-error@+1 {{}}
   int1 b = 300u;
   // expected-error@+1 {{}}
   float4 c = 16'777'217;
   // expected-error@+1 {{}}
   float4{16'777'217};
   // expected-error@+1 {{}}
   float4{-16'777'217};
   // expected-error@+1 {{}}
   float8 d = 9'007'199'254'740'993LL;
   // expected-error@+1 {{}}
   float8{9'007'199'254'740'993LL};
   // expected-error@+1 {{}}
   float8{-9'007'199'254'740'993LL};
   // expected-error@+1 {{}}
   uint8 e = -1ll;
   // expected-error@+1 {{}}
   idx f = -1;
   // expected-error@+1 {{}}
   idx{-1};
   // expected-error@+1 {{}}
   idx g = -1ll;
   // expected-error@+1 {{}}
   uintptr<void> h = -1;
   // expected-error@+1 {{}}
   uintptr<void>{-1};
   // expected-error@+1 {{}}
   uintptr<void> i = -1ll;
}

void
enable_if_compound_and_bit() {
   idx i(0u);
   // expected-error@+1 {{}}
   i += -1;
   // expected-error@+1 2 {{}}
   cat::countl_zero_unchecked(0u);
   // expected-error@+1 2 {{}}
   cat::countl_one_unchecked(0xFFFFFFFFu);
   // expected-error@+1 2 {{}}
   cat::countr_zero_unchecked(0u);
   // expected-error@+1 2 {{}}
   cat::countr_zero_unchecked(idx{0u});
   // expected-error@+1 2 {{}}
   cat::countr_one_unchecked(0xFFFFFFFFu);
   // expected-error@+1 2 {{}}
   cat::bit_ceil_unchecked(0x80000001u);
   auto r = int1(0).undef();
   // expected-error@+1 {{}}
   r += 300u;
   auto s = uint1(0u).undef();
   // expected-error@+1 {{}}
   s += -1;
}

void
rejected_conversions() {
   int x;
   // expected-error@+1 {{}}
   x = idx(cat::deconst(0u));
   int2 y;
   // expected-error@+1 {{}}
   y = idx(cat::deconst(0u));
   int4 z;
   // expected-error@+1 {{}}
   z = idx(cat::deconst(0u));
   float4 f;
   // expected-error@+1 {{}}
   f = idx(cat::deconst(0u));
   float4 g(1.0f);
   // expected-error@+1 {{}}
   float8 h = g;
   float4_fast p(1.0f);
   // expected-error@+1 {{}}
   float8 q = p;
   float4 r(1.0f);
   // expected-error@+1 {{}}
   float8_fast t = r;

   uword w(0u);
   // expected-error@+1 {{}}
   idx i = cat::deconst(w);
   iword v(0);
   // expected-error@+1 {{}}
   idx j = cat::deconst(v);
   idx k(0u);
   iword m(0);
   // expected-error@+1 {{}}
   k = cat::deconst(m);

   unsigned long u = cat::deconst(0ul);
   // expected-error@+1 {{}}
   intptr<void> a = u;
   int b = cat::deconst(0);
   // expected-error@+1 {{}}
   uintptr<void> c = b;
   long d = cat::deconst(0L);
   // expected-error@+1 {{}}
   uintptr<void> e = d;
   uword n(0u);
   // expected-error@+1 {{}}
   intptr<void> o = cat::deconst(n);
   iword s(0);
   // expected-error@+1 {{}}
   uintptr<void> ww = cat::deconst(s);
}

void
idx_deleted_ops() {
   idx i(cat::deconst(1u));
   // expected-error@+1 {{}}
   i -= cat::deconst(1u);
   // expected-error@+1 {{}}
   --i;
   // expected-error@+1 {{}}
   i--;
   idx j(cat::deconst(6u));
   int s = cat::deconst(2);
   // expected-error@+1 {{}}
   j /= s;
   // expected-error@+1 {{}}
   idx(cat::deconst(1u)) & idx(cat::deconst(1u));
   // expected-error@+1 {{}}
   idx(0u) - 1.0f;
   // expected-error@+1 {{}}
   idx(0u) - 1.0;
   int a = 0;
   // expected-error@+1 {{}}
   idx(0u) - &a;
   // expected-error@+1 {{}}
   uword w = idx(0u) - cat::deconst(1u);
   uintptr<void> p;
   int x = cat::deconst(-1);
   // expected-error@+1 {{}}
   p = x;
}

void
assign_uint_from_int() {
   uint1 u1(0u);
   int1 i1(0);
   // expected-error@+1 {{}}
   u1 = cat::deconst(i1);
   uint2 u2(0u);
   // expected-error@+1 {{}}
   u2 = cat::deconst(i1);
   int2 i2(0);
   // expected-error@+1 {{}}
   u2 = cat::deconst(i2);
   uint4 u4(0u);
   // expected-error@+1 {{}}
   u4 = cat::deconst(i1);
   // expected-error@+1 {{}}
   u4 = cat::deconst(i2);
   int4 i4(0);
   // expected-error@+1 {{}}
   u4 = cat::deconst(i4);
   uint8 u8(0u);
   // expected-error@+1 {{}}
   u8 = cat::deconst(i1);
   // expected-error@+1 {{}}
   u8 = cat::deconst(i2);
   // expected-error@+1 {{}}
   u8 = cat::deconst(i4);
   int8 i8(0);
   // expected-error@+1 {{}}
   u8 = cat::deconst(i8);
}

void
assign_int_from_uint() {
   int1 a1(0);
   uint1 b1(0u);
   // expected-error@+1 {{}}
   a1 = cat::deconst(b1);
   uint2 b2(0u);
   // expected-error@+1 {{}}
   a1 = cat::deconst(b2);
   uint4 b4(0u);
   // expected-error@+1 {{}}
   a1 = cat::deconst(b4);
   uint8 b8(0u);
   // expected-error@+1 {{}}
   a1 = cat::deconst(b8);
   int2 a2(0);
   // expected-error@+1 {{}}
   a2 = cat::deconst(b2);
   // expected-error@+1 {{}}
   a2 = cat::deconst(b4);
   // expected-error@+1 {{}}
   a2 = cat::deconst(b8);
   int4 a4(0);
   // expected-error@+1 {{}}
   a4 = cat::deconst(b4);
   // expected-error@+1 {{}}
   a4 = cat::deconst(b8);
   int8 a8(0);
   // expected-error@+1 {{}}
   a8 = cat::deconst(b8);
}

void
implicit_narrow() {
   int2 w2(cat::deconst(0));
   // expected-error@+1 {{}}
   int1 a = w2;
   int4 w4(cat::deconst(0));
   // expected-error@+1 {{}}
   int1 b = w4;
   int8 w8(cat::deconst(0));
   // expected-error@+1 {{}}
   int1 c = w8;
   // expected-error@+1 {{}}
   int2 d = w4;
   // expected-error@+1 {{}}
   int2 e = w8;
   // expected-error@+1 {{}}
   int4 f = w8;
   uint2 u2(cat::deconst(0u));
   // expected-error@+1 {{}}
   uint1 g = u2;
   uint4 u4(cat::deconst(0u));
   // expected-error@+1 {{}}
   uint1 h = u4;
   uint8 u8(cat::deconst(0u));
   // expected-error@+1 {{}}
   uint1 i = u8;
   // expected-error@+1 {{}}
   uint2 j = u4;
   // expected-error@+1 {{}}
   uint2 k = u8;
   // expected-error@+1 {{}}
   uint4 l = u8;
}

void
implicit_mixed_sign() {
   int1 i1(cat::deconst(0));
   int2 i2(cat::deconst(0));
   int4 i4(cat::deconst(0));
   int8 i8(cat::deconst(0));
   // expected-error@+1 {{}}
   uint1 a = i1;
   // expected-error@+1 {{}}
   uint2 b = i1;
   // expected-error@+1 {{}}
   uint4 c = i1;
   // expected-error@+1 {{}}
   uint8 d = i1;
   // expected-error@+1 {{}}
   uint1 e = i2;
   // expected-error@+1 {{}}
   uint2 f = i2;
   // expected-error@+1 {{}}
   uint4 g = i2;
   // expected-error@+1 {{}}
   uint8 h = i2;
   // expected-error@+1 {{}}
   uint1 i = i4;
   // expected-error@+1 {{}}
   uint2 j = i4;
   // expected-error@+1 {{}}
   uint4 k = i4;
   // expected-error@+1 {{}}
   uint8 l = i4;
   // expected-error@+1 {{}}
   uint1 m = i8;
   // expected-error@+1 {{}}
   uint2 n = i8;
   // expected-error@+1 {{}}
   uint4 o = i8;
   // expected-error@+1 {{}}
   uint8 p = i8;

   uint1 u1(cat::deconst(0u));
   uint2 u2(cat::deconst(0u));
   uint4 u4(cat::deconst(0u));
   uint8 u8(cat::deconst(0u));
   // expected-error@+1 {{}}
   int1 q = u1;
   // expected-error@+1 {{}}
   int1 r = u2;
   // expected-error@+1 {{}}
   int1 s = u4;
   // expected-error@+1 {{}}
   int1 t = u8;
   // expected-error@+1 {{}}
   int2 v = u2;
   // expected-error@+1 {{}}
   int2 w = u4;
   // expected-error@+1 {{}}
   int2 x = u8;
   // expected-error@+1 {{}}
   int4 y = u4;
   // expected-error@+1 {{}}
   int4 z = u8;
   // expected-error@+1 {{}}
   int8 aa = u8;
}

void
implicit_int_to_float() {
   int v = cat::deconst(1);
   // expected-error@+1 {{}}
   float2 a = v;
   // expected-error@+1 {{}}
   float4 b = v;
   // expected-error@+1 {{}}
   float8 c = v;
}

void
compound_narrow() {
   int1 a{};
   // expected-error@+1 {{}}
   a += cat::deconst(int2{0});
   // expected-error@+1 {{}}
   a *= cat::deconst(int2{0});
   // expected-error@+1 {{}}
   a -= cat::deconst(int2{0});
   uint1 b{};
   // expected-error@+1 {{}}
   b += cat::deconst(uint2{0u});
   int1 c{};
   // expected-error@+1 {{}}
   c.undef() -= cat::deconst(int2{0});

   int1 d{};
   // expected-error@+1 {{}}
   d += cat::deconst(int4{0});
   // expected-error@+1 {{}}
   d *= cat::deconst(int4{0});
   // expected-error@+1 {{}}
   d -= cat::deconst(int4{0});
   uint1 e{};
   // expected-error@+1 {{}}
   e += cat::deconst(uint4{0u});
   int1 f{};
   // expected-error@+1 {{}}
   f.undef() -= cat::deconst(int4{0});

   int1 g{};
   // expected-error@+1 {{}}
   g += cat::deconst(int8{0});
   // expected-error@+1 {{}}
   g *= cat::deconst(int8{0});
   // expected-error@+1 {{}}
   g -= cat::deconst(int8{0});
   uint1 h{};
   // expected-error@+1 {{}}
   h += cat::deconst(uint8{0u});
   int1 i{};
   // expected-error@+1 {{}}
   i.undef() -= cat::deconst(int8{0});

   int2 j{};
   // expected-error@+1 {{}}
   j += cat::deconst(int4{0});
   // expected-error@+1 {{}}
   j *= cat::deconst(int4{0});
   // expected-error@+1 {{}}
   j -= cat::deconst(int4{0});
   uint2 k{};
   // expected-error@+1 {{}}
   k += cat::deconst(uint4{0u});
   int2 l{};
   // expected-error@+1 {{}}
   l.undef() -= cat::deconst(int4{0});

   int2 m{};
   // expected-error@+1 {{}}
   m += cat::deconst(int8{0});
   // expected-error@+1 {{}}
   m *= cat::deconst(int8{0});
   // expected-error@+1 {{}}
   m -= cat::deconst(int8{0});
   uint2 n{};
   // expected-error@+1 {{}}
   n += cat::deconst(uint8{0u});
   int2 o{};
   // expected-error@+1 {{}}
   o.undef() -= cat::deconst(int8{0});

   int4 p{};
   // expected-error@+1 {{}}
   p += cat::deconst(int8{0});
   // expected-error@+1 {{}}
   p *= cat::deconst(int8{0});
   // expected-error@+1 {{}}
   p -= cat::deconst(int8{0});
   uint4 q{};
   // expected-error@+1 {{}}
   q += cat::deconst(uint8{0u});
   int4 r{};
   // expected-error@+1 {{}}
   r.undef() -= cat::deconst(int8{0});

   int4 s{};
   iword w(0);
   // expected-error@+1 {{}}
   s -= cat::deconst(w);
}
