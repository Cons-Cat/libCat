/* libCat linker script for x86-64 ELF.

   Pass `-Wl,-T,path/to/libcat.ld`. This script replaces lld's
   built-in default linker script entirely. Every output section
   libCat ever produces is described here.

   Layout matches lld's x86-64 SysV order across all configurations:

     1. Static, non-PIE Release (`-static -Wl,--no-pie -Wl,-N`).
        With `-Wl,-N` lld merges every section into a single R+W+X
        LOAD. `_start.cpp` initialises `%fs` itself (gated on
        `CAT_STATIC_LINKED`).

     2. Dynamic, no sanitizers, no shared libs (Debug nosan).
        `ld.so` still runs because the kernel honours PT_INTERP, and
        `ld.so` initialises TLS for us.

     3. Dynamic + ASan (Debug + sanitizers). Adds `DT_NEEDED
        libclang_rt.asan-x86_64.so`, `asan_globals`, and a populated
        `.preinit_array` for ASan's startup callbacks.

     4. Dynamic + `libcat.so` (`CAT_USE_SHARED=ON`). Adds
        `DT_NEEDED libcat.so`. The `.so` itself is also produced via
        this script.

   References:
     MaskRay on section/segment layout:
       https://maskray.me/blog/2023-12-17-exploring-the-section-layout-in-linker-output
     Stargirl Flowers' commented script:
       https://blog.thea.codes/the-most-thoroughly-commented-linker-script/
*/

/* Entry point. `cat::detail::_start` is `extern "C"` so its symbol
   name is the unmangled `_start`. The kernel jumps here directly in
   static builds. `ld.so` jumps here after relocation in dynamic ones. */
ENTRY(_start)

/* PHDRS are intentionally omitted. lld synthesises them from the
   section flag bands below, which keeps `-Wl,-N` (omagic) effective:
   it merges adjacent compatible LOADs into a single R+W+X segment
   for the size-optimised Release configuration. */

SECTIONS {
   /* Image base. `SEGMENT_START("text-segment", 0x200000)` matches
      the classical x86-64 SysV ABI choice (2 MiB) for static non-PIE.
      Dynamic builds treat it as a soft preference (the runtime
      relocates anyway). `+ SIZEOF_HEADERS` bumps the cursor past the
      ELF header + program-header table that lld writes at the start
      of the file: without it, the first content section would land
      at vaddr 0x200000 while still living at file offset
      `sizeof(Ehdr) + N*sizeof(Phdr)`, and the kernel would reject
      the LOAD because `vaddr - file_offset` must be a multiple of
      the page size. */
   . = SEGMENT_START("text-segment", 0x200000) + SIZEOF_HEADERS;

   /* Read-only band ====================================================
      Headers, dynamic-link metadata, and constant data. In static
      builds the dynamic-link sections are empty and disappear. */

   .interp           : { *(.interp) }
   .note.gnu.build-id : { *(.note.gnu.build-id) }
   .gnu.hash         : { *(.gnu.hash) }
   .dynsym           : { *(.dynsym) }
   .dynstr           : { *(.dynstr) }
   .gnu.version      : { *(.gnu.version) }
   .gnu.version_r    : { *(.gnu.version_r) }
   .rela.dyn         : { *(.rela.* .rela_iplt_*) }
   .rela.plt         : { *(.rela.plt) }

   .rodata           : { *(.rodata .rodata.* .rodata.cst*) }

   /* Compiler-rt's ASan runtime ships its own `.eh_frame[_hdr]`. libCat
      itself doesn't (we build with `-fno-asynchronous-unwind-tables`),
      so these are empty in static Release. */
   .eh_frame_hdr     : { *(.eh_frame_hdr) }
   .eh_frame         : { KEEP(*(.eh_frame .eh_frame.*)) }

   /* Read-execute band =================================================
      Page-aligned away from the read-only band so `ld.so`'s later
      `mprotect` of `PT_GNU_RELRO` can't reach back into a shared
      page and flip text bytes read-only. `MAXPAGESIZE` resolves to
      4 KiB on x86-64. */
   . = ALIGN(CONSTANT(MAXPAGESIZE));

   .text             : { *(.text .text.*) }
   .plt              : { *(.plt) }

   /* Read-write band ===================================================
      Close to lld's x86-64 default, with two deliberate deviations
      forced by `-T` semantics:

        1. The TLS template lives below the RELRO band rather than
           above it. lld's own driver places `.tdata` inside
           `PT_GNU_RELRO` via internal logic that the script syntax
           cannot reproduce. Putting `.tdata` above RELRO under `-T`
           trips lld's RELRO contiguity check on large links
           because `.tbss` separates `.tdata` from the rest of the
           band. Putting `.tdata` below RELRO is only accepted
           while no explicit `.relro_padding` section is present
           (see point 2). With one, lld switches to strict mode and
           rejects `.tdata` as non-contiguous.

        2. No `.relro_padding`. lld emits one in its default layout
           to round RELRO up to a page boundary. Under `-T` adding
           it forces strict RELRO-contiguity checks that conflict
           with point 1. `ld.so` still mprotects whole RELRO pages
           without it, just stopping at the last full page.

      The leading page alignment keeps the trailing page of
      `.text`/`.plt` out of the RW LOAD that `ld.so` mprotects
      for RELRO. */
   . = ALIGN(CONSTANT(MAXPAGESIZE));

   /* RELRO band. `.preinit_array`, `.init_array`, `.fini_array`,
      `.data.rel.ro`, `.dynamic`, and `.got` all carry dynamic
      relocations that `ld.so` resolves once at startup, after which
      `PT_GNU_RELRO` mprotects the band back to read-only. `KEEP`
      survives `--gc-sections`. */
   .preinit_array    : { KEEP(*(.preinit_array)) }
   .init_array       : {
      __init_array_start = .;
      KEEP(*(SORT_BY_INIT_PRIORITY(.init_array.*)))
      KEEP(*(.init_array))
      __init_array_end = .;
   }
   .fini_array       : {
      KEEP(*(SORT_BY_INIT_PRIORITY(.fini_array.*)))
      KEEP(*(.fini_array))
   }

   /* `asan_globals` is emitted by `-fsanitize=address` and registers
      every instrumented global with the ASan runtime. */
   asan_globals      : { *(asan_globals) }

   .data.rel.ro      : { *(.data.rel.ro .data.rel.ro.*) }
   .dynamic          : { *(.dynamic) }
   .got              : { *(.got) }

   /* `.got.plt` is excluded from RELRO because the dynamic loader
      writes lazy PLT resolutions there at first call. */
   .got.plt          : { *(.got.plt) }

   /* TLS template. PT_TLS describes both `.tdata` and `.tbss`. The
      `__cat_tls_*` linker symbols at the bottom of this script
      expose SIZEOF/ADDR of these sections to libCat at runtime via
      `link_absolute_symbol`. The explicit ALIGN keeps `ADDR(.tdata)
      mod MAX(ALIGNOF(.tdata), ALIGNOF(.tbss)) == 0` so the
      `__cat_tls_memory_size` formula below stays invariant when
      `.tbss` is more strictly aligned than `.tdata`. */
   . = ALIGN(MAX(ALIGNOF(.tdata), ALIGNOF(.tbss)));
   .tdata            : { *(.tdata .tdata.*) }
   .tbss (NOLOAD)    : { *(.tbss .tbss.*) }

   /* Mutable data. */
   .data             : { *(.data .data.*) }
   .bss (NOLOAD)     : { *(.bss .bss.*) *(COMMON) }

   /* Discarded sections ================================================ */
   /DISCARD/         : {
      /* libCat is `-z noexecstack`. Drop the marker so a stray .o
         that defines it can't flip the segment to executable. */
      *(.note.GNU-stack)
      /* Stale LTO blobs from partial links. */
      *(.gnu.lto_*)
      *(.gnu.debuglto_*)
      *(__gnu_lto_*)
   }

   /* Non-loaded auxiliary band =========================================
      DWARF v5 plus tool metadata. Not part of any LOAD. Their
      addresses are 0 in the section header table. lld appends
      `.symtab`, `.shstrtab`, and `.strtab` automatically. We don't
      need to list them. */
   .comment          : { *(.comment) }
   .debug_info       : { *(.debug_info) }
   .debug_abbrev     : { *(.debug_abbrev) }
   .debug_addr       : { *(.debug_addr) }
   .debug_frame      : { *(.debug_frame) }
   .debug_line       : { *(.debug_line) }
   .debug_line_str   : { *(.debug_line_str) }
   .debug_loclists   : { *(.debug_loclists) }
   .debug_rnglists   : { *(.debug_rnglists) }
   .debug_str        : { *(.debug_str) }
   .debug_str_offsets : { *(.debug_str_offsets) }
}

/* Linker-supplied absolute symbols for libCat's TLS bookkeeping.
   `__cat_tls_initial_image` is the LMA of the TLS init image (i.e.
   `.tdata`). The size and alignment trio are the constants
   `init_parent_process_tls()` (static-linked main thread) and
   `nix::process::spawn_impl` (clone-time TLS slab carve-out) read via
   `link_absolute_symbol<T>(__cat_tls_*)` at runtime.

   These have to be defined OUTSIDE the SECTIONS block. Otherwise
   `SIZEOF(.tdata)` etc. would be evaluated at the cursor position
   inside SECTIONS rather than after the layout is finalised. */
__cat_tls_file_size    = SIZEOF(.tdata);
/* Must match the thread pointer lld picks for `R_X86_64_TPOFF32`: end of
   `.tbss` rounded up to the TLS template alignment (the natural max of
   the two section alignments, not the over-aligned 32 below). Skipping
   either the inter-section pad or this trailing pad mis-places the
   runtime image and every `thread_local` reads the wrong byte. */
__cat_tls_memory_size  =
  ALIGN(ADDR(.tbss) + SIZEOF(.tbss), MAX(ALIGNOF(.tdata), ALIGNOF(.tbss)))
  - ADDR(.tdata);
__cat_tls_alignment    = MAX(MAX(ALIGNOF(.tdata), ALIGNOF(.tbss)), 32);
__cat_tls_initial_image = ADDR(.tdata);
