#pragma once

// NOLINTNEXTLINE
__attribute__((optimize(0))) void _exit(const i32&) {
    // clang-format off
	i64 exit_code = 0;
	asm(R"(
        syscall
        ret)"
		: // No outputs.
		:"D" (exit_code), "a"(60)
		: "memory");
    // clang-format on
}

// NOLINTNEXTLINE
__attribute__((optimize(0))) void _start() {
    // clang-format off
    asm(R"(
        .global _start
        _start:
        xorl %ebp, %ebp 
        movq %rsp, %rdi
        lea 8(%rsp), %rsi
        call main
        movq %rax, %rdi
        call _exit)");
    // clang-format on
}

// TODO: Is a variadic syscall possible?

extern "C" auto syscall5(void* p_number, void* p_arg1 = nullptr,
                         void* p_arg2 = nullptr, void* p_arg3 = nullptr,
                         void* p_arg4 = nullptr, void* p_arg5 = nullptr)
    -> void*;

auto write(int const& file_descriptor, char const* const& p_string_buffer,
           usize const& string_length) -> isize {
    return reinterpret_cast<isize>(
        syscall5(reinterpret_cast<void*>(1),
                 reinterpret_cast<void*>(static_cast<isize>(file_descriptor)),
                 (void*)(p_string_buffer),
                 reinterpret_cast<void*>(string_length), 0, 0));
}

// TODO: Overload write() with automatic string length.
