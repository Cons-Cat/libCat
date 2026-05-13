#include <cat/linux>

auto
nix::sys_getsockopt(file_descriptor socket_descriptor, cat::int4 level,
                    cat::int4 option_name, void* p_option_value,
                    cat::int4* p_option_length) -> nix::scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall<void>(55, socket_descriptor, level, option_name,
                             p_option_value, p_option_length);
}
