#!/usr/bin/env python3
"""Fail if two writable `PT_LOAD` segments of an ELF file share a page.

musl's dynamic loader gives the unused rest of each writable segment's pages to
`malloc`, so a page that two writable segments share gets overwritten.
"""

import struct
import sys

PT_LOAD = 1
PF_W = 2
PAGE_SIZE = 4096


def writable_segments(path):
    with open(path, "rb") as file:
        data = file.read()
    if data[:4] != b"\x7fELF" or data[4] != 2:
        sys.exit(f"{path}: not a 64-bit ELF file")
    (program_header_offset,) = struct.unpack_from("<Q", data, 0x20)
    entry_size, entry_count = struct.unpack_from("<HH", data, 0x36)
    for index in range(entry_count):
        kind, flags, _, address, _, _, memory_size, _ = struct.unpack_from(
            "<IIQQQQQQ", data, program_header_offset + index * entry_size
        )
        if kind == PT_LOAD and flags & PF_W:
            yield address, address + memory_size


failed = False
for path in sys.argv[1:]:
    segments = sorted(writable_segments(path))
    for (_, end), (start, _) in zip(segments, segments[1:]):
        if (end - 1) // PAGE_SIZE == start // PAGE_SIZE:
            page = start // PAGE_SIZE * PAGE_SIZE
            print(f"{path}: writable segments share the page at {page:#x}")
            failed = True
sys.exit(failed)
