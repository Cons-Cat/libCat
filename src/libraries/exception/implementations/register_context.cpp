#if __has_feature(cxx_exceptions)
#include <cat/detail/register_context.hpp>

// x86-64 SysV register capture and restore. The unwinder calls
// `capture` at the raise site and `install` at the landing-pad
// transfer.
namespace cat::detail {
extern "C" [[gnu::naked, gnu::returns_twice]]
void
cat_register_context_capture(register_context* /*context*/) {
   // Offsets in bytes into `register_context::registers`:
   //   rax=0  rdx=8  rcx=16 rbx=24 rsi=32 rdi=40 rbp=48 rsp=56
   //   r8=64  r9=72  r10=80 r11=88 r12=96 r13=104 r14=112 r15=120
   //   rip=128
   asm volatile(R"(movq %%rax,   0(%%rdi)
                   movq %%rdx,   8(%%rdi)
                   movq %%rcx,  16(%%rdi)
                   movq %%rbx,  24(%%rdi)
                   movq %%rsi,  32(%%rdi)
                   movq %%rdi,  40(%%rdi)
                   movq %%rbp,  48(%%rdi)
                   # Capture caller's rsp. Caller's rsp is rsp + 8,
                   # the return address slot.
                   leaq 8(%%rsp), %%rax
                   movq %%rax,  56(%%rdi)
                   movq %%r8,   64(%%rdi)
                   movq %%r9,   72(%%rdi)
                   movq %%r10,  80(%%rdi)
                   movq %%r11,  88(%%rdi)
                   movq %%r12,  96(%%rdi)
                   movq %%r13, 104(%%rdi)
                   movq %%r14, 112(%%rdi)
                   movq %%r15, 120(%%rdi)
                   # rip is the return address on the stack, the
                   # caller's program counter.
                   movq (%%rsp), %%rax
                   movq %%rax, 128(%%rdi)
                   xorl %%eax, %%eax
                   ret
                )"
                :
                :
                : "memory");
}

extern "C" [[gnu::naked, noreturn]]
void
cat_register_context_install(register_context* /*context*/) {
   asm volatile(R"(movq 128(%%rdi), %%rax
                   movq  56(%%rdi), %%rsp
                   # Reserve a slot and stash the target IP where `ret` will
                   # consume it. Has to happen after switching to the new stack
                   # so we don't disturb the raise-site frame.
                   subq  $8, %%rsp
                   movq %%rax, (%%rsp)
                   movq   8(%%rdi), %%rdx
                   movq  16(%%rdi), %%rcx
                   movq  24(%%rdi), %%rbx
                   movq  32(%%rdi), %%rsi
                   movq  48(%%rdi), %%rbp
                   movq  64(%%rdi), %%r8
                   movq  72(%%rdi), %%r9
                   movq  80(%%rdi), %%r10
                   movq  88(%%rdi), %%r11
                   movq  96(%%rdi), %%r12
                   movq 104(%%rdi), %%r13
                   movq 112(%%rdi), %%r14
                   movq 120(%%rdi), %%r15
                   movq   0(%%rdi), %%rax
                   movq  40(%%rdi), %%rdi
                   ret
                )"
                :
                :
                : "memory");
}

}  // namespace cat::detail

#endif
