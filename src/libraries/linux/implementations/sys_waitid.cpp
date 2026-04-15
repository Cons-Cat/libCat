#include <cat/linux>

auto
nix::sys_waitid(wait_id type, process_id pid, wait_options_flags options)
   -> scaredy_nix<process_id> {
   // `waitid` is `SYSCALL_DEFINE5`; pass a null `rusage` pointer so `%r8` is
   // defined at the syscall boundary (see `syscall5`).
   // TODO: `p_signal_info` should replace the `infop` `nullptr`.
   return syscall<process_id>(247, type, pid, nullptr, options, nullptr);
}
