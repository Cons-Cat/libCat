#include <cat/linux>

auto nix::sys_waitid(nix::WaitId type, nix::ProcessId id,
                     nix::WaitOptionsFlags options)
    -> nix::ScaredyLinux<nix::ProcessId> {
    return nix::syscall<nix::ProcessId>(247, type, id, nullptr, options,
                                        nullptr);
}
