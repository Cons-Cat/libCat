#include <cat/array>
#include <cat/bit>
#include <cat/memory>
#include <cat/simd>

// Copy some bytes from one address to another address.
// TODO: Make this `constexpr`.
void cat::copy_memory(void const* p_source, void* p_destination, ssize bytes) {
    using Vector = int8x_;

    unsigned char const* p_source_handle =
        static_cast<unsigned char const*>(p_source);
    unsigned char* p_destination_handle =
        cat::bit_cast<unsigned char*>(p_destination);
    constexpr ssize l3_cache_size = 2_mi;
    ssize padding;

    constexpr ssize step_size = ssizeof<Vector>() * 8;

    if (bytes <= step_size) {
        copy_memory_small(p_source, p_destination, bytes);
        return;
    }

    // Align source, destination, and bytes to the vector's optimal alignment.
    padding = static_cast<signed int long>(
        (alignof(Vector) -
         ((cat::bit_cast<__UINTPTR_TYPE__>(p_destination_handle)) &
          (alignof(Vector) - 1))) &
        (alignof(Vector) - 1));

    copy_memory_small(p_source, p_destination, padding);

    p_source_handle += padding;
    p_destination_handle += padding;
    bytes -= padding;
    Vector vectors[8];

    // This routine is optimized for buffers in L3 cache. Streaming is
    // slower there.
    if (bytes <= l3_cache_size) {
        while (bytes >= step_size) {
            // Load 8 vectors, then increment the source pointer by that
            // size.
#pragma GCC unroll 8
            for (int i = 0; i < 8; ++i) {
                vectors[i] = cat::bit_cast<Vector const*>(p_source_handle)[i];
            }
            cat::prefetch_for_one_read(p_source_handle + (step_size * 2));

#pragma GCC unroll 8
            for (int i = 0; i < 8; ++i) {
                cat::bit_cast<Vector*>(p_destination_handle)[i] = vectors[i];
            }
            p_source_handle += step_size;
            p_destination_handle += step_size;
            bytes -= step_size;
        }
    }

    // This routine is run when the memory source cannot fit in cache.
    else {
        cat::prefetch_for_one_read(p_source_handle + 512);
        // TODO: This code block has fallen far out of date.
        // TODO: This could be improved by using aligned-streaming when
        // possible.
        while (bytes >= 256) {
#pragma GCC unroll 8
            for (int i = 0; i < 8; ++i) {
                vectors[i] = cat::bit_cast<Vector*>(p_source_handle)[i];
            }
            cat::prefetch_for_one_read(p_source_handle + 512);
            p_source_handle += 256;
#pragma GCC unroll 8
            for (int i = 0; i < 8; ++i) {
                stream_in(p_destination_handle, &vectors[i]);
            }
            p_destination_handle += 256;
            bytes -= 256;
        }

        cat::sfence();
    }

    copy_memory_small(p_source_handle, p_destination_handle, bytes);
    cat::zero_upper_avx_registers();
}

/*
// Copy some bytes from one address to another address.
void copy_memory(void const* p_source, void* p_destination, ssize
bytes) { using Vector = int8x_;

    intptr p_source_handle = p_source;
    intptr p_destination_handle = p_destination;
    constexpr ssize l3_cache_size = 2_mi;
    intptr padding;

    constexpr ssize step_size = ssizeof<Vector>() * 8;

    if (bytes <= step_size) {
        copy_memory_small(p_source, p_destination, bytes);
        return;
    }

    // Align source, destination, and bytes to the vector's optimal
alignment.
    // TODO: Make a `uintptr`.
    padding = ((intptr(alignof(Vector)) -
                ((p_destination_handle) & (alignof(Vector) - 1))) &
               (alignof(Vector) - 1));

    copy_memory_small(p_source, p_destination, ssize{padding});

    p_source_handle += padding;
    p_destination_handle += padding;
    bytes -= padding;
    Array<Vector, 8> vectors;

    // This routine is optimized for buffers in L3 cache. Streaming is
    // slower there.
    if (bytes <= l3_cache_size) {
        while (bytes >= step_size) {
            // Load 8 vectors, then increment the source pointer by that
            // size.
#pragma GCC unroll 8
            for (int i = 0; i < 8; ++i) {
                vectors[i] = static_cast<Vector const*>(p_source_handle)[i];
            }
            prefetch_for_one_read(
                static_cast<void*>(p_source_handle + (step_size * 2)));

#pragma GCC unroll 8
            for (int i = 0; i < 8; ++i) {
                static_cast<Vector*>(p_destination_handle)[i] = vectors[i];
            }
            p_source_handle += step_size;
            p_destination_handle += step_size;
            bytes -= step_size;
        }
    }

    // This routine is run when the memory source cannot fit in cache.
    else {
        prefetch_for_one_read(static_cast<void*>(p_source_handle +
512));
        // TODO: This code block has fallen far out of date.
        // TODO: This could be improved by using aligned-streaming when
        // possible.
        while (bytes >= 256) {
#pragma GCC unroll 8
            for (int i = 0; i < 8; ++i) {
                vectors[i] = static_cast<Vector*>((p_source_handle))[i];
            }
            prefetch_for_one_read(
                static_cast<void*>(p_source_handle + 512));
            p_source_handle += 256;
#pragma GCC unroll 8
            for (int i = 0; i < 8; ++i) {
                stream_in(p_destination_handle, &vectors[i]);
            }
            p_destination_handle += 256;
            bytes -= 256;
        }

        sfence();
    }

    copy_memory_small(p_source_handle, p_destination_handle, bytes);
    zero_upper_avx_registers();
}
*/
