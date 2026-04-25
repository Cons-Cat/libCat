# Exercises the full ill-formed `cat::[u]int{1,2,4,8}` cross-width and `int` to
# `float{2,4,8}` and `iword` on `int4` grids that the unit `can_*_assign` story
# allows as `-fsyntax-only` TUs. `index` and `arithmetic_ptr` are not a width
# product. The named probes for those stay in `cat_arithmetic_neg_cases.cmake`
# Rely on `_cat_neg_try_fails` in `cat_arithmetic_negative.cmake`

set(_m_hdr [[#include <cat/arithmetic>
#include "arithmetic_neg_deconst.hpp"
]])

# `uintA` = `deconst(intB)` and `intA` = `deconst(uintB)` (skip pairs that are well-formed)
set(_cat_neg_ok_u_eq_i 1-2 1-4 1-8 2-4 2-8 4-8)
set(_cat_neg_ok_i_eq_u 2-1 4-1 4-2 8-1 8-2 8-4)
foreach(ua IN ITEMS 1 2 4 8)
  foreach(ib IN ITEMS 1 2 4 8)
    set(_k "${ua}-${ib}")
    list(FIND _cat_neg_ok_u_eq_i "${_k}" _skip)
    if(_skip LESS 0)
      string(CONFIGURE "asgn-uint@ua@-int@ib@" _z @ONLY)
      string(CONFIGURE
        "${_m_hdr}void t() { cat::uint@ua@ u(0u); cat::int@ib@ x(0);
  u = cat_neg_probe::deconst_number(x); }"
        _src @ONLY)
      _cat_neg_try_fails("${_z}" "${_src}")
    endif()
  endforeach()
  foreach(ub IN ITEMS 1 2 4 8)
    set(_k "${ua}-${ub}")
    list(FIND _cat_neg_ok_i_eq_u "${_k}" _skip2)
    if(_skip2 LESS 0)
      string(CONFIGURE "asgn-int@ua@-uint@ub@" _z2 @ONLY)
      string(CONFIGURE
        "${_m_hdr}void t() { cat::int@ua@ a(0); cat::uint@ub@ b(0u);
  a = cat_neg_probe::deconst_number(b); }"
        _src2 @ONLY)
      _cat_neg_try_fails("${_z2}" "${_src2}")
    endif()
  endforeach()
endforeach()

# Implicit copy-init: narrow `intS` = wider `deconst(intL)` and same for `uint`
set(_cat_neg_narrow 1-2 1-4 1-8 2-4 2-8 4-8)
foreach(_p IN LISTS _cat_neg_narrow)
  string(REPLACE "-" ";" _ij "${_p}")
  list(GET _ij 0 s)
  list(GET _ij 1 l)
  string(CONFIGURE "icvt-impl-narrow-to-int@s@-int@l@" _nmid @ONLY)
  string(CONFIGURE
    "${_m_hdr}void t() { cat::int@l@ w(cat_neg_probe::deconst_number(0));
  cat::int@s@ a = w; }"
    _n1 @ONLY)
  _cat_neg_try_fails("${_nmid}" "${_n1}")
  string(CONFIGURE "icvt-impl-narrow-to-uint@s@-uint@l@" _nmid2 @ONLY)
  string(CONFIGURE
    "${_m_hdr}void t() { cat::uint@l@ w(cat_neg_probe::deconst_number(0u));
  cat::uint@s@ a = w; }"
    _n2 @ONLY)
  _cat_neg_try_fails("${_nmid2}" "${_n2}")
  unset(_nmid)
  unset(_nmid2)
endforeach()
unset(s)
unset(l)

# Mixed unsigned / signed: every `deconst(intA)` to `uintB` is ill-formed
foreach(ia IN ITEMS 1 2 4 8)
  foreach(ub IN ITEMS 1 2 4 8)
    string(CONFIGURE "icvt-impl-mix-int@ia@-to-uint@ub@" _mxn @ONLY)
    string(CONFIGURE
      "${_m_hdr}void t() { cat::int@ia@ i(cat_neg_probe::deconst_number(0));
  cat::uint@ub@ u = i; }"
      _mx1 @ONLY)
    _cat_neg_try_fails("${_mxn}" "${_mx1}")
    unset(_mxn)
  endforeach()
endforeach()
unset(ia)
unset(ub)

# `uintA` to `deconst(intB)` (skip well-formed)
set(_cat_neg_ok_uitoi 2-1 4-1 4-2 8-1 8-2 8-4)
foreach(ib IN ITEMS 1 2 4 8)
  foreach(ua IN ITEMS 1 2 4 8)
    set(_k "${ib}-${ua}")
    list(FIND _cat_neg_ok_uitoi "${_k}" _skip3)
    if(_skip3 LESS 0)
      string(CONFIGURE "icvt-impl-mix-uint@ua@-to-int@ib@" _mxn2 @ONLY)
      string(CONFIGURE
        "${_m_hdr}void t() { cat::uint@ua@ w(cat_neg_probe::deconst_number(0u));
  cat::int@ib@ a = w; }"
        _mx2 @ONLY)
      _cat_neg_try_fails("${_mxn2}" "${_mx2}")
      unset(_mxn2)
    endif()
  endforeach()
endforeach()

# `icvt` to `float2` / `float4` / `float8` (`int` RHS)
string(CONFIGURE
  "${_m_hdr}void t() { int v = cat_neg_probe::deconst_number(1);
  cat::float2 f = v; }"
  _f2 @ONLY)
_cat_neg_try_fails("icvt-impl-int-to-float2" "${_f2}")
string(CONFIGURE
  "${_m_hdr}void t() { int v = cat_neg_probe::deconst_number(1);
  cat::float4 f = v; }"
  _f4 @ONLY)
_cat_neg_try_fails("icvt-impl-int-to-float4" "${_f4}")
string(CONFIGURE
  "${_m_hdr}void t() { int v = cat_neg_probe::deconst_number(1);
  cat::float8 f = v; }"
  _f8 @ONLY)
_cat_neg_try_fails("icvt-impl-int-to-float8" "${_f8}")
unset(ua)
unset(ib)

# Undefined compound ops that promote (see `!can_{plus,times,minus}_assign` grid)
set(_m_hdr2 [[#include <cat/arithmetic>
#include "arithmetic_neg_deconst.hpp"
]])

foreach(_p IN LISTS _cat_neg_narrow)
  string(REPLACE "-" ";" _ij2 "${_p}")
  list(GET _ij2 0 s2)
  list(GET _ij2 1 l2)
  string(CONFIGURE "pluseq-int@s2@-int@l2@" _cid1 @ONLY)
  string(CONFIGURE
    "${_m_hdr2}void t() { cat::int@s2@ a{}; a += cat_neg_probe::deconst_number(
    cat::int@l2@{0}); }"
    _c1 @ONLY)
  _cat_neg_try_fails("${_cid1}" "${_c1}")
  string(CONFIGURE "muleq-int@s2@-int@l2@" _cid2 @ONLY)
  string(CONFIGURE
    "${_m_hdr2}void t() { cat::int@s2@ a{}; a *= cat_neg_probe::deconst_number(
    cat::int@l2@{0}); }"
    _c2 @ONLY)
  _cat_neg_try_fails("${_cid2}" "${_c2}")
  string(CONFIGURE "minuseq-int@s2@-int@l2@" _cid3 @ONLY)
  string(CONFIGURE
    "${_m_hdr2}void t() { cat::int@s2@ a{}; a -= cat_neg_probe::deconst_number(
    cat::int@l2@{0}); }"
    _c3 @ONLY)
  _cat_neg_try_fails("${_cid3}" "${_c3}")
  string(CONFIGURE "pluseq-uint@s2@-uint@l2@" _cid4 @ONLY)
  string(CONFIGURE
    "${_m_hdr2}void t() { cat::uint@s2@ a{}; a += cat_neg_probe::deconst_number(
    cat::uint@l2@{0u}); }"
    _c4 @ONLY)
  _cat_neg_try_fails("${_cid4}" "${_c4}")
  string(CONFIGURE "undef-minuseq-int@s2@-int@l2@" _cid5 @ONLY)
  string(CONFIGURE
    "${_m_hdr2}void t() { cat::int@s2@ a{}; a.undef() -=
    cat_neg_probe::deconst_number(cat::int@l2@{0}); }"
    _c5 @ONLY)
  _cat_neg_try_fails("${_cid5}" "${_c5}")
  unset(_cid1)
  unset(_cid2)
  unset(_cid3)
  unset(_cid4)
  unset(_cid5)
endforeach()
unset(s2)
unset(l2)

# `int4 -= iword` (`subtract_by` would widen)
string(CONFIGURE
  "${_m_hdr2}void t() { cat::int4 a{}; cat::iword w(0);
  a -= cat_neg_probe::deconst_number(w); }"
  _iwe @ONLY)
_cat_neg_try_fails("minuseq-int4-iword" "${_iwe}")
unset(_m_hdr)
unset(_m_hdr2)
unset(_c1)
unset(_c2)
unset(_c3)
unset(_c4)
unset(_c5)
unset(_f2)
unset(_f4)
unset(_f8)
unset(_iwe)
unset(_k)
unset(_mx1)
unset(_mx2)
unset(_n1)
unset(_n2)
unset(_p)
unset(_src)
unset(_src2)
unset(_ij)
unset(_ij2)
unset(_skip)
unset(_skip2)
unset(_skip3)
unset(_cat_neg_ok_u_eq_i)
unset(_cat_neg_ok_i_eq_u)
unset(_cat_neg_ok_uitoi)
unset(_cat_neg_narrow)
