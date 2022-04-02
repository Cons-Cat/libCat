#include <array>
#include <linux>
#include <memory>
#include <socket>
#include <string>

void meow() {
    SocketUnix<SocketType::stream> listening_socket;
    listening_socket.path_name = "/tmp/temp.sock";
    SocketUnix<SocketType::stream> recieving_socket;

    Array<char, 12> message_buffer;

    nix::unlink(listening_socket.path_name.p_data()).or_panic();

    listening_socket.create().or_panic();

    listening_socket.bind().or_panic();
    listening_socket.listen(20).or_panic();

    bool1 exit = false;
    while (!exit) {
        recieving_socket.accept(listening_socket).or_panic();

        while (true) {
            recieving_socket.recieve(&message_buffer, message_buffer.size())
                .or_panic();

            String input = message_buffer.to_string();

            if (cat::compare_strings(input, "exit")) {
                cat::print_line("Exiting.").or_panic();
                exit = true;
                break;
            }

            if (!cat::compare_strings(input, "")) {
                cat::print("Recieved: ").or_panic();
                cat::print_line(input).or_panic();
                // TODO: `cat::set_memory()` or something is needed to zero-out
                // `message_buffer` here.
                break;
            }
        }
    }

    recieving_socket.close().or_panic();
    listening_socket.close().or_panic();
    nix::unlink(listening_socket.path_name.p_data()).or_panic();
    cat::exit();
}
