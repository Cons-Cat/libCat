#include <cat/array>
#include <cat/linux>
#include <cat/memory>
#include <cat/socket>
#include <cat/string>

void meow() {
    cat::SocketUnix<cat::SocketType::stream> listening_socket;
    listening_socket.path_name = "/tmp/temp.sock";
    cat::SocketUnix<cat::SocketType::stream> recieving_socket;

    cat::Array<char, 12> message_buffer;

    _ = nix::unlink(listening_socket.path_name.p_data()).or_panic();

    listening_socket.create().or_panic();

    listening_socket.bind().or_panic();
    listening_socket.listen(20).or_panic();

    bool1 exit = false;
    while (!exit) {
        recieving_socket.accept(listening_socket).or_panic();

        while (true) {
            _ = recieving_socket.recieve(&message_buffer, message_buffer.size())
                    .or_panic();

            cat::String input = message_buffer.to_string();

            if (cat::compare_strings(input, "exit")) {
                _ = cat::print_line("Exiting.").or_panic();
                exit = true;
                break;
            }

            if (!cat::compare_strings(input, "")) {
                _ = cat::print("Recieved: ").or_panic();
                _ = cat::print_line(input).or_panic();
                // TODO: `cat::set_memory()` or something is needed to zero-out
                // `message_buffer` here.
                break;
            }
        }
    }

    recieving_socket.close().or_panic();
    listening_socket.close().or_panic();
    _ = nix::unlink(listening_socket.path_name.p_data()).or_panic();
    cat::exit();
}
