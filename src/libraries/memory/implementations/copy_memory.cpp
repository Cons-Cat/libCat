// -*- mode: c++ -*-
// vim: set ft=cpp:
#include <memory>

// TODO: Make integers consistently signed.
/* Copy some bytes from one address to another address. */
void cat::copy_memory(void const* p_source, void* p_destination, ssize bytes) {
    // `Vector` is the width of a 32-byte AVX register.
    // `long long int` is required for some SIMD intrinsics.
    using Vector = cat::detail::Simd<long long int, 4, 4>;

    unsigned char const* p_source_handle =
        meta::bit_cast<unsigned char const*>(p_source);
    unsigned char* p_destination_handle =
        meta::bit_cast<unsigned char*>(p_destination);
    constexpr ssize cachesize = 0x200000;  // L3-cache size.
    ssize padding;

    if (bytes <= 256) {
        cat::copy_memory_small(p_source, p_destination, bytes);
    }

    // Align source, destination, and bytes to 16 bytes
    padding = (32 - ((meta::bit_cast<ssize>(p_destination_handle)) & 31)) & 31;

    Vector head = *meta::bit_cast<Vector const*>(p_source_handle);
    *static_cast<Vector*>(p_destination) = head;

    p_source_handle += padding;
    p_destination_handle += padding;
    bytes -= padding;

    Vector vectors[8];
    constexpr ssize step_size = sizeof(Vector) * 8;
    // This routine is optimized for buffers in L3 cache. Streaming is
    // slower.
    if (bytes <= cachesize) {
        while (bytes >= step_size) {
            /* Load 8 8x4 vectors, then increment the source pointer by that
             * size. */
#pragma GCC unroll 8
            for (int4 i = 0; i < 8; i++) {
                vectors[i] = meta::bit_cast<Vector*>(p_source_handle)[i];
            }
            simd::prefetch_for_one_read(p_source_handle + 512);
#pragma GCC unroll 8
            for (int4 i = 0; i < 8; i++) {
                meta::bit_cast<Vector*>(p_destination_handle)[i] = vectors[i];
            }
            p_source_handle += step_size;
            p_destination_handle += step_size;
            bytes -= step_size;
        }
    }
    // This routine is run when the memory source cannot fit in cache.
    else {
        simd::prefetch_for_one_read(p_source_handle + 512);
        /* TODO: This could be improved by using aligned-streaming when
         * possible. */
        while (bytes >= 256) {
#pragma GCC unroll 8
            for (int4 i = 0; i < 8; i++) {
                vectors[i] = meta::bit_cast<Vector*>((p_source_handle))[i];
            }
            simd::prefetch_for_one_read(p_source_handle + 512);
            p_source_handle += 256;
#pragma GCC unroll 8
            for (int4 i = 0; i < 8; i++) {
                simd::stream_in(p_destination_handle, &vectors[i]);
            }
            p_destination_handle += 256;
            bytes -= 256;
        }
        simd::sfence();
    }

    cat::copy_memory_small(p_source_handle, p_destination_handle, bytes);
    simd::zero_upper_avx_registers();
}  // namespace simd
