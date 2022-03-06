// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <linux>

// TODO: Socket descriptor types for `SOCK_STREAM` and `SOCK_SEQPACKET`.

// Basic Socket type that any other sockets must convert into.
struct Socket {
    uint2 family;
    uint1 data[14];
};

struct SocketLocal {
    uint2 const family = 1u;
    uint1 path_name[108];
};

// Create and return a `Socket`.
auto socket(int8 const protocol_family, int8 const type, int8 const protocol)
    -> Result<FileDescriptor> {
    return syscall3(41u, protocol_family, type, protocol);
}

// Create and return a `SocketLocal` (also known as Unix socket).
auto socket_local(int8 const type, int8 const protocol)
    -> Result<FileDescriptor> {
    return socket(1, type, protocol);
}

// Connect a `Socket` to an address.
auto connect(FileDescriptor const socket_descriptor, Socket const* p_socket)
    -> Result<> {
    return syscall3(42u, socket_descriptor, p_socket, sizeof(Socket));
}

// Make a connection over a `Socket`. This returns a new socket which has been
// connected to. This new `Socket` is not in a listening state.
// TODO: Add flags for Linux syscall `288u`.
auto accept(FileDescriptor const socket_descriptor,
            Socket const* __restrict p_socket,
            isize const* __restrict p_addr_len) -> Result<FileDescriptor> {
    return syscall3(43u, socket_descriptor, p_socket, p_addr_len);
}

auto recieve(FileDescriptor const socket_descriptor,
             void const* p_message_buffer, isize const buffer_length,
             Socket const* __restrict p_addr = nullptr,
             isize const* __restrict p_addr_length = nullptr) -> Result<isize> {
    return syscall5(45u, socket_descriptor, p_message_buffer, buffer_length,
                    p_addr, p_addr_length);
}

auto bind(FileDescriptor const socket_descriptor, Socket const* p_socket,
          isize const p_addr_len) -> Result<> {
    return syscall3(49u, socket_descriptor, p_socket, p_addr_len);
}

// Mark a socket as available to make connections with `accept()`.
auto listen(FileDescriptor const socket_descriptor, int8 const backlog)
    -> Result<> {
    return syscall2(50u, socket_descriptor, backlog);
}

// Returns the number of characters sent to `destination_socket`.
auto send_buffer(FileDescriptor const socket_descriptor,
                 void const* p_message_buffer, isize const buffer_length,
                 int4 const flags, Socket const* p_destination_socket = nullptr,
                 isize const addr_length = 0) -> Result<isize> {
    return syscall6(44u, socket_descriptor, p_message_buffer, buffer_length,
                    flags, p_destination_socket, addr_length);
}
