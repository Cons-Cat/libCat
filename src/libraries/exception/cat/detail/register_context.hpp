// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

// x86-64 register file for the unwinder. Numbering matches the SysV
// x86-64 psABI DWARF register mapping.
//
//    0 rax   1 rdx   2 rcx   3 rbx   4 rsi   5 rdi   6 rbp   7 rsp
//    8 r8    9 r9   10 r10  11 r11  12 r12  13 r13  14 r14  15 r15
//   16 rip  17 xmm0 ... 32 xmm15
//
// Only the 16 GPRs, rip, and the return address column, typically 16,
// are tracked. That matches the CFI Clang emits with
// `-fasynchronous-unwind-tables`. SIMD registers are omitted. C++
// unwinding never restores them.

#include <cat/arithmetic>

namespace cat::detail {

inline constexpr idx register_count = 17u;
inline constexpr idx register_rax = 0u;
inline constexpr idx register_rdx = 1u;
inline constexpr idx register_rcx = 2u;
inline constexpr idx register_rbx = 3u;
inline constexpr idx register_rsi = 4u;
inline constexpr idx register_rdi = 5u;
inline constexpr idx register_rbp = 6u;
inline constexpr idx register_rsp = 7u;
inline constexpr idx register_r8 = 8u;
inline constexpr idx register_r9 = 9u;
inline constexpr idx register_r10 = 10u;
inline constexpr idx register_r11 = 11u;
inline constexpr idx register_r12 = 12u;
inline constexpr idx register_r13 = 13u;
inline constexpr idx register_r14 = 14u;
inline constexpr idx register_r15 = 15u;
inline constexpr idx register_rip = 16u;

struct register_context {
   uword registers[register_count];
};

// `registers[16]` is the caller's return address, so unwinding
// starts at the caller's caller. This matches libgcc.
extern "C" [[gnu::returns_twice]]
void
cat_register_context_capture(register_context* _Nonnull p_context);

// Restores all 17 registers and jumps to `registers[16]`. Never
// returns.
extern "C" [[noreturn]]
void
cat_register_context_install(register_context* _Nonnull p_context);

}  // namespace cat::detail
