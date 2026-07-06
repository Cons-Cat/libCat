#include <cat/linux>

auto
nix::sys_waitid(wait_id type, process_id id, wait_options_flags options)
   -> scaredy_nix<process_id> {
   // `waitid` is `SYSCALL_DEFINE5`. Pass a null `rusage` pointer so `%r8` is
   // defined at the syscall boundary (see `syscall5`).
   // TODO: `p_signal_info` should replace the `infop` `nullptr`.
   // https://filippo.io/linux-syscall-table/
   return syscall_volatile<process_id>(
      247, type, id, nullptr, options, nullptr
   );
}
