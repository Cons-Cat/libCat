// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <concepts>
#include <linux>

#include "linux_flags"

// TODO: Socket descriptor types for SOCK_STREAM and SOCK_SEQPACKET.

enum ProtocolFamilies
{
    AF_UNIX,       // Local communication.
    AF_LOCAL,      // Synonym for AF_UNIX.
    AF_INET,       // IPv4 Internet protocols.
    AF_AX2,        // Amateur radio AX.25 protocol.
    AF_IPX,        // IPX - Novell protocols.
    AF_APPLETALK,  // AppleTalk.
    AF_X25,        // ITU-T X.25 / ISO-8208 protocol.
    AF_INET6,      // IPv6 Internet protocols.
    AF_DECnet,     // DECet protocol sockets.
    AF_KEY,      /* Key management protocol, originally developed for usage with
                    IPsec. */
    AF_NETLINK,  // Kernel user interface device.
    AF_PACKET,   // Low-level packet interface.
    AF_RDS,      // Reliable Datagram Sockets (RDS) protocol.
    AF_PPPOX,    /* Generic PPP transport layer, for setting up L2 tunnels (L2TP
                    and PPPoE). */
    AF_LLC,      /* Logical link control (IEEE 802.2 LLC)  protocol. */
    AF_IB,       // InfiniBand native addressing.
    AF_MPLS,     // Multiprotocol Label Switching.
    AF_CAN,      /* Controller Area Network automotive bus protocol. */
    AF_TIPC,     // TIPC, "cluster domain sockets" protocol.
    AF_BLUETOOTH,  // Bluetooth low-level socket protocol.
    AF_ALG,        // Interface to kernel crypto API.
    AF_VSOCK,      /* VSOCK (originally "VMWare VSockets") protocol for
                    * hypervisor-guest communication. */
    AF_KCM, /* KCM (kernel connection multiplexer) inte versions. ?. versions. .
               versions. rface. */
    AF_XDP,  // XDP (express data path) interface.
};

auto socket(i4 protocol_family, i4 type, i4 protocol)
    -> Result<FileDescriptor> {
    return syscall3(41u, protocol_family, type, protocol);
}

struct SocketAddr {
    u2 socket_address_family;
    char socket_address_data[14];
};

auto connect(FileDescriptor socket_descriptor, SocketAddr* p_socket)
    -> Result<> {
    return syscall3(42u, socket_descriptor, p_socket, sizeof(SocketAddr));
}

// Make a connection over a socket. This returns a new socket which has been
// connected to. This new socket is not in a listening state.
// TODO: Add flags for Linux syscall `288u`.
auto accept(FileDescriptor socket_descriptor, SocketAddr* p_socket,
            meta::integral auto* p_addr_len) -> Result<FileDescriptor> {
    return syscall3(43u, socket_descriptor, p_socket, p_addr_len);
}

auto bind(FileDescriptor socket_descriptor, SocketAddr* p_socket,
          meta::integral auto p_addr_len) -> Result<> {
    return syscall4(49u, socket_descriptor, p_socket, p_addr_len);
}

// Mark a socket as available to make connections with `accept()`.
auto listen(FileDescriptor socket_descriptor, i4 backlog) -> Result<> {
    return syscall2(50u, socket_descriptor, backlog);
}

// Returns the number of characters sent to `destination_socket`.
auto send_buffer_to(FileDescriptor socket_descriptor,
                    void const* p_message_buffer, u8 buffer_length, i4 flags,
                    SocketAddr* p_destination_socket, u8 addr_length)
    -> Result<i8> {
    return syscall6(44u, socket_descriptor, p_message_buffer, buffer_length,
                    flags, p_destination_socket, addr_length);
}
