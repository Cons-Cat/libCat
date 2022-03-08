#include <buffer>
#include <linux>
#include <memory>
#include <socket.h>
#include <string>

void meow() {
    SocketLocal socket;
    char8_t const* socket_name = u8"/tmp/temp.sock";
    FileDescriptor listening_socket;
    FileDescriptor recieving_socket;
    Buffer<char8_t, 12> buffer;

    unlink(socket_name).discard_result();

    // `1` is a streaming socket.
    listening_socket = create_socket_local(1, 0).or_panic();
    std::copy_memory(socket_name, &socket.path_name,
                     sizeof(socket.path_name) - 1);
    bind_socket(listening_socket, &socket, sizeof(socket)).or_panic();
    listen_to_socket(listening_socket, 20).or_panic();

    while (true) {
        recieving_socket =
            accept_socket(listening_socket, nullptr, nullptr).or_panic();

        while (true) {
            recieve_buffer(recieving_socket, &buffer, 12).or_panic();
            buffer[11] = 0;  // Make `buffer` null-terminated.
            break;
        }

        close(recieving_socket).or_panic();
        break;
    }

    close(listening_socket).or_panic();
    unlink(socket_name).or_panic();
    exit();
}
