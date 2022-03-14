#include <buffer>
#include <linux>
#include <memory>
#include <socket>
#include <string>

void meow() {
    SocketLocal<SocketType::stream> socket;
    char const* socket_name = "/tmp/temp.sock";
    FileDescriptor listening_socket;
    FileDescriptor recieving_socket;
    Buffer<char, 12> buffer;

    nix::unlink(socket_name).or_panic();

    // `1` is a streaming socket.
    listening_socket = nix::create_socket_local(1, 0).or_panic();
    std::copy_memory(socket_name, &socket.path_name,
                     sizeof(socket.path_name) - 1);
    nix::bind_socket(listening_socket, &socket, sizeof(socket)).or_panic();
    nix::listen_to_socket(listening_socket, 20).or_panic();

    while (true) {
        recieving_socket =
            nix::accept_socket(listening_socket, nullptr, nullptr).or_panic();
        bool exit = false;

        while (true) {
            nix::recieve_buffer(recieving_socket, &buffer, buffer.length)
                .or_panic();
            StringView input = buffer.to_string();

            if (std::compare_strings(input, "exit")) {
                std::print_line("Exiting.").or_panic();
                exit = true;
                break;
            }

            if (!std::compare_strings(input, "")) {
                std::print("Recieved: ").or_panic();
                std::print_line(input).or_panic();
                break;
            }
        }

        if (exit) {
            nix::close(recieving_socket).or_panic();
            break;
        }
    }

    nix::close(listening_socket).or_panic();
    nix::unlink(socket_name).or_panic();
    std::exit();
}
