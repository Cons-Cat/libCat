// #include "cstdlib"
void* syscall5(void* number, void* arg1, void* arg2, void* arg3, void* arg4,
               void* arg5);

typedef unsigned long int uintptr; /* size_t */
typedef long int intptr;           /* ssize_t */

static intptr write(int fd, void const* data, uintptr nbytes) {
    return (intptr)syscall5((void*)1, /* SYS_write */
                            (void*)(intptr)fd, (void*)data, (void*)nbytes,
                            0, /* ignored */
                            0  /* ignored */
    );
}

int main(int argc, char* argv[]) {
    write(1, "hello\n", 6);

    return 0;
}
/*

typedef long unsigned int size_t;
typedef long int ssize_t;

void* syscall5(void* p_number, void* p_arg1, void* p_arg2, void* p_arg3,
               void* p_arg4, void* p_arg5);

static ssize_t write(int file_descriptor, char const* p_string_buffer,
                     size_t string_length) {
    return (ssize_t)syscall5((void*)1, (void*)(ssize_t)(file_descriptor),
                             (void*)p_string_buffer, (void*)string_length, 0,
                             0);
}

int main() {
    write(1, "Hello, Conscat!\n", 16);
    return 0;
}
*/
