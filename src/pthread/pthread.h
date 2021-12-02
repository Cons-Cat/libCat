#pragma once

enum class LinuxError;

// TODO: Make a locale header.
struct locale_map;

struct locale_struct {
    const struct locale_map* cat[6];
};

using locale_t = locale_struct;

// TODO: union pthread with a safer and more readable type with the same layout.
struct pthread {
    struct pthread* self;
    size_t* dtv;
    struct pthread *prev, *next;
    size_t sysinfo;
    size_t canary;

    int32_t tid;
    LinuxError errno_val;
    volatile int32_t detach_state;
    volatile int32_t cancel;
    volatile unsigned char canceldisable, cancelasync;
    unsigned char tsd_used     : 1;
    unsigned char dlerror_flag : 1;
    unsigned char* map_base;
    size_t map_size;
    void* stack;
    size_t stack_size;
    size_t guard_size;
    void* result;
    struct __ptcb* cancelbuf;  // NOLINT
    void** tsd;

    struct {
        volatile void* volatile head;
        long off;
        volatile void* volatile pending;
    } robust_list;

    int32_t h_errno_val;
    volatile int32_t timer_id;
    locale_t locale;
    volatile int32_t killlock[1];
    char* dlerror_buf;
    void* stdio_locks;
};

auto get_pthread_pointer() -> pthread* {
    pthread* thread_ptr;
    asm(R"(mov %%fs:0, %0)" : "=r"(thread_ptr));
    return thread_ptr;
}
