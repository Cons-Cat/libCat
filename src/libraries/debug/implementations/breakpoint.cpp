#include <cat/debug>
#include <cat/linux>

void cat::breakpoint() {
    // TODO: Arm and Windows support.
#ifdef __x86_64__
    // For x86-64, insert a breakpoint interrupt instruction.
    asm volatile("int3");
#else
    // Without hardware support, raise SIGILL, which breaks in a debugger.
    nix::raise_here(nix::Signal::illegal_instruction);
#endif
}
