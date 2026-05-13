#include <cat/linux>

auto
nix::sys_getpgrp() -> process_id {
   // https://filippo.io/linux-syscall-table/
   return syscall<process_id>(111).value();
}
