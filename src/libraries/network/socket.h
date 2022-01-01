// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <linux>

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

// TODO: Make this into a `concept`.
struct SocketAddr {
    u4 socket_address_family;    // address family.
    char socket_address_data[];  // socket address (variable-length data)
};

template <typename SocketType>
auto connect_to(FileDescriptor connecting_socket, SocketType* p_socket)
    -> Result<> {
    return syscall3(42u, connecting_socket, p_socket, sizeof(SocketType));
}

/* Returns a `FileDescriptor` for a socket that is requesting a connection, and
 * has been accepted. A `Failure` is returned if the request failed.
 *
 * `listening_socket` must be initialized and binded before calling `accept()`.
 * `p_accepted_socket` is a pointer to a socket `struct` which has the same
 * binary layout as that socket which this expects to connect with. A
 * `FileDescriptor` representing the accepted socket is returned.
 *
 * This function is side-effectful. `p_accepted_socket` is assigned the address
 * of an accepted socket. */
template <typename SocketType>
auto accept_connection(FileDescriptor listening_socket,
                       SocketType* p_accepted_socket, i4 flags = 0)
    -> Result<FileDescriptor> {
    // TODO: This seems to be blocked by atomic intrinsics.
    // TODO: Make an enum for `accept4()` Linux flags.
    isize socket_size = sizeof(SocketType);
    syscall4(288u, listening_socket, p_accepted_socket, &socket_size, flags);
    // TODO: Return file descriptor.
    // https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/tree/net/socket.c#n1838
    // https://docs.huihoo.com/doxygen/linux/kernel/3.7/include_2linux_2file_8h_source.html
    // https://docs.huihoo.com/doxygen/linux/kernel/3.7/fs_2file_8c_source.html#l00773
    return 0;
}

// Returns the number of characters sent to `destination_socket`.
template <typename SocketType>
auto send_buffer_to(FileDescriptor sending_socket, void const* p_message_buffer,
                    usize buffer_length, i4 flags,
                    SocketType const* p_destination_socket) -> Result<isize> {
    // TODO: This seems to be blocked by atomic intrinsics.
    // TODO: Implement this behavior.
    // https://filippo.io/linux-syscall-table/
    // https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/tree/net/socket.c#n2005
    return 0;
}
