#include <cat/linux>

auto
nix::sys_pipe(cat::int4 (&pipefd)[2]) -> nix::scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<void>(22, &pipefd[0]);
}
