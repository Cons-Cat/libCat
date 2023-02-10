#include <cat/linux>

auto nix::create_socket_local(cat::int8 type, cat::int8 protocol)
    -> nix::scaredy_nix<nix::file_descriptor> {
    return nix::sys_socket(1, type, protocol);
}
