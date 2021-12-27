/* `__stack_chk_fail()` is called when stack overflow occurs in programs
 * compiled without `-fno-stack-protector`. */
extern "C" void __stack_chk_fail() {
    exit(1);
}
