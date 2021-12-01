#pragma once

enum class LinuxError;

// TODO: Make a locale header.
struct locale_map;

struct locale_struct {
    const struct locale_map* cat[6];
};

using locale_t = locale_struct;

struct pthread {
    struct pthread* self;
    usize* dtv;
    struct pthread *prev, *next;
    usize sysinfo;
    usize canary;

    i32 tid;
    LinuxError errno_val;
    volatile i32 detach_state;
    volatile i32 cancel;
    volatile unsigned char canceldisable, cancelasync;
    unsigned char tsd_used     : 1;
    unsigned char dlerror_flag : 1;
    unsigned char* map_base;
    usize map_size;
    void* stack;
    usize stack_size;
    usize guard_size;
    void* result;
    struct __ptcb* cancelbuf;  // NOLINT
    void** tsd;

    struct {
        volatile void* volatile head;
        long off;
        volatile void* volatile pending;
    } robust_list;

    i32 h_errno_val;
    volatile i32 timer_id;
    locale_t locale;
    volatile i32 killlock[1];
    char* dlerror_buf;
    void* stdio_locks;
};

auto get_pthread_pointer() -> pthread* {
    pthread* thread_ptr;
    asm(R"(mov %%fs:0, %0)" : "=r"(thread_ptr));
    return thread_ptr;
}
