#include <cat/linux>

auto
nix::sys_pipe2(cat::int4 (&pipefd)[2], pipe2_flags flags)
   -> nix::scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<void>(293, &pipefd[0], flags);
}
