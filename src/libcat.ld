/* libCat: TLS layout constants for `clone` and `clone_flags::set_tls` setup.
   Pass `-Wl,-T,path/to/libcat.ld` (lld or bfd).

   We cannot use `SIZEOF(.tdata)` unless the linker output already has a
   `.tdata` section. Executables with no `thread_local` never create one, so
   `SIZEOF` and `ADDR` would fail. `INSERT AFTER .text` adds output sections
   that collect the usual TLS input sections; they may be empty, but
   `SIZEOF(.tdata)` and `SIZEOF(.tbss)` are then always defined. */

SECTIONS {
   .tdata : { *(.tdata .tdata.* .gnu.linkonce.td.*) }
   .tbss (NOLOAD) : { *(.tbss .tbss.* .gnu.linkonce.tb.*) }
}
INSERT AFTER .text;

__cat_tls_file_size = SIZEOF(.tdata);
__cat_tls_memory_size = SIZEOF(.tdata) + SIZEOF(.tbss);
__cat_tls_alignment = MAX(ALIGNOF(.tdata), 32);
__cat_tls_initial_image = ADDR(.tdata);
