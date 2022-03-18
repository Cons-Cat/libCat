#include <buffer>
#include <linux>
#include <memory>
#include <socket>
#include <string>

void meow() {
    SocketLocal<SocketType::stream> listening_socket;
    SocketLocal<SocketType::stream> recieving_socket;
    char const* socket_name = "/tmp/temp.sock";
    Buffer<char, 12> buffer;

    nix::unlink(socket_name).or_panic();

    listening_socket.create().or_panic();

    std::copy_memory(socket_name, &listening_socket.path_name,
                     sizeof(listening_socket.path_name) - 1);
    listening_socket.bind().or_panic();
    listening_socket.listen(20).or_panic();

    bool1 exit = false;
    while (!exit) {
        recieving_socket.accept(listening_socket).or_panic();

        while (true) {
            recieving_socket.recieve(&buffer, buffer.length).or_panic();

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
    }

    recieving_socket.close().or_panic();
    listening_socket.close().or_panic();
    nix::unlink(socket_name).or_panic();
    std::exit();
}
