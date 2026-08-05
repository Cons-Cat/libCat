#include <cat/linux>

auto
nix::sys_pipe(file_descriptor (&pipefd)[2]) -> nix::scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<void>(22, &pipefd[0]);
}
