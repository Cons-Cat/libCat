// The smallest program linked with `libcat.ld` whose `.got` and `.tdata` are
// both empty, but which still has a writable `PT_LOAD` after the RELRO band.
// `check_writable_segments.py` checks its layout.

int value = 1;

extern "C" [[noreturn]] void
_start() {
   // The linker relaxes this `.got` load into a `lea`, which leaves `.got`
   // empty without removing it.
   int* p_value;
   asm("movq value@GOTPCREL(%%rip), %0"
       : "=r"(p_value));
   asm volatile("syscall"
                :
                : "a"(60), "D"(*p_value - 1));
   __builtin_unreachable();
}
